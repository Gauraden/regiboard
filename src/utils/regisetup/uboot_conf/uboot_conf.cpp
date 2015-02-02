#include <iostream>
#include <fstream>
#include <string>
#include <boost/program_options.hpp>
#include <boost/scoped_array.hpp>
#include "u-boot/zlib.h"
#include "configs/mx53_regigraf.h"

#define ENV_SIZE (CONFIG_ENV_SIZE - sizeof(uint32_t))

static const size_t      kSegmentSz = ENV_SIZE;
static const std::string kMtdDev("mtd1");
static const std::string kBinDef("uboot.bin");
static const std::string kTxtDef("uboot.conf");
// см. исходники u-boot (./build/u-boot-.../lib_generic/crc32.c)
extern uint32_t crc32(uint32_t crc, const Bytef *buf, uInt len);

struct Arguments {
  enum Operation {
    kRead,
    kWrite,
    kUnknown
  };
  Arguments(): oper(kUnknown) {}
  Arguments(const std::string &_in, const std::string &_out, Operation _oper)
      : in(_in), out(_out), oper(_oper) {}
  const std::string in;
  const std::string out;
  const Operation   oper;  
};

Arguments ParseArgs(int argc, char **argv) {
  std::string in;
  std::string out;
  unsigned    oper = Arguments::kUnknown;
  
  namespace po = boost::program_options;
  po::options_description desc("Программа чтения/прошивки конфигурации uboot");
	desc.add_options()
		("help", "описание аргументов")
		("if",  po::value<std::string>()->default_value(kBinDef),
             "файл для чтения исходных данных")
		("of",  po::value<std::string>()->default_value(kTxtDef),
             "файл для записи результата")
		("do", po::value<std::string>()->default_value("r"),
             "команда: r(чтение), w(запись)");
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);
	if (vm.count("help")) {
	  std::cout << desc << std::endl;
	  return Arguments();
	}
	in   = vm["if"].as<std::string>();
	out  = vm["of"].as<std::string>();
  oper = vm["do"].as<std::string>() == "w" ? Arguments::kWrite : Arguments::kRead;  
  return Arguments(in, out, (oper == Arguments::kWrite ? Arguments::kWrite : Arguments::kRead));
}

bool BinToText(std::istream &in, std::ostream &out) {
  in.seekg(4, in.beg);
  boost::scoped_array<uint8_t> seg(new uint8_t[kSegmentSz]);
  do {
    in.read((char*)seg.get(), kSegmentSz);
    uint8_t end_of_conf = 0;
    size_t  row_off     = 0;
    for (size_t off = 0; off < kSegmentSz; off++) {
      if (seg[off] == 0)
        end_of_conf++;
      else
        end_of_conf = 0;
      // конец строки
      if (end_of_conf == 1) {
        out << (char*)&seg[row_off] << "\n";
        row_off = off + 1;
      }
      // конец конфигурации
      if (end_of_conf > 1)
        return true;
    }
    if (row_off < kSegmentSz) {
      out << std::string((char*)&seg[row_off], kSegmentSz - row_off);
    }
  } while (in.good());
  return true;
}

bool TextToBin(std::istream &in, std::ostream &out) {
  static const size_t kMaxRowLen = 255;
  char seg[kSegmentSz];
  char row[kMaxRowLen];
  size_t seg_off = 0;
  memset(seg, 0, kSegmentSz);
  do {
    in.getline(row, kMaxRowLen);
    memcpy(&seg[seg_off], row, in.gcount());
    seg_off += in.gcount();
  } while (in.good());
  const uint32_t kCrc = crc32(0, (const unsigned char*)seg, kSegmentSz);
  out.write((char*)&kCrc, sizeof(kCrc));
  out.write(seg,          kSegmentSz);
  return true;
}

int main(int argc, char **argv) {
  Arguments args(ParseArgs(argc, argv));
  if (args.oper == Arguments::kUnknown) {
    return 0;
  }
  std::fstream in(args.in.c_str(), std::ios_base::in | std::ios_base::binary);
  if (not in.good()) {
    std::cout << "Ошибка открытия файла для чтения: " << args.in << std::endl;
    return 1;
  }
  std::fstream out(args.out.c_str(), std::ios_base::binary | std::ios_base::out);
  if (not out.good()) {
    std::cout << "Ошибка открытия файла для записи: " << args.in << std::endl;
    return 1;
  }
  if (args.oper == Arguments::kRead)
    BinToText(in, out);
  else
    TextToBin(in, out);
  return 0;
}
