#include <iostream>
#include <fstream>
#include <string>
#include <boost/program_options.hpp>
#include <boost/scoped_array.hpp>
#include <boost/regex.hpp>
#include "u-boot/zlib.h"
#include "configs/mx53_regigraf.h"

#define ENV_SIZE (CONFIG_ENV_SIZE - sizeof(uint32_t))

static const size_t      kSegmentSz = ENV_SIZE;
static const std::string kMtdDev("mtd1");
static const std::string kBinDef("uboot.bin");
static const std::string kTxtDef("uboot.conf");
static const std::string kUndef("X");
// см. исходники u-boot (./build/u-boot-.../lib_generic/crc32.c)
uint32_t crc32(uint32_t crc, const Bytef *buf, uInt len);

struct Arguments {
  enum Operation {
    kRead,
    kWrite,
    kUnknown
  };
  Arguments(): oper(kUnknown) {}
  Arguments(const std::string &_in, 
            const std::string &_out,
            Operation          _oper,
            const std::string &board,
            const std::string &lcd,
            const std::string &ts)
      : in(_in), 
        out(_out), 
        oper(_oper), 
        board_id(board), 
        lcd_model(lcd), 
        ts_model(ts) {}
  const std::string in;
  const std::string out;
  const Operation   oper;
  const std::string board_id;
  const std::string lcd_model;
  const std::string ts_model;
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
             "команда: r(чтение), w(запись)")
    ("board", po::value<std::string>()->default_value(kUndef),
             "идентификатор платы <Партия>.<Номер платы>")
    ("lcd", po::value<std::string>()->default_value(kUndef),
             "модель дисплея")
    ("ts", po::value<std::string>()->default_value(kUndef),
             "модель контроллера сенсорной панели");
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
  return Arguments(in, 
                   out,
                   (oper == Arguments::kWrite ? Arguments::kWrite : Arguments::kRead),
                   vm["board"].as<std::string>(),
                   vm["lcd"].as<std::string>(),
                   vm["ts"].as<std::string>());
}

std::string ApplyFilter(const std::string &in, const Arguments &args) {
  boost::smatch res;
  if (boost::regex_match(in, res, boost::regex("([^=]+)=(.*)"), boost::match_default)) {
    if (res[1] == "board_id" && args.board_id != kUndef) {
      return std::string("board_id=") + args.board_id;
    }
    if (res[1] == "lcd_model" && args.lcd_model != kUndef) {
      return std::string("lcd_model=") + args.lcd_model;
    }
    if (res[1] == "ts_model" && args.ts_model != kUndef) {
      return std::string("ts_model=") + args.ts_model;
    }
  }
  return in;
}

bool BinToText(const Arguments &args, std::istream &in, std::ostream &out) {
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
        out << ApplyFilter((char*)&seg[row_off], args) << "\n";
        row_off = off + 1;
      }
      // конец конфигурации
      if (end_of_conf > 1)
        return true;
    }
    // дописываем остатки
    if (row_off < kSegmentSz) {
      out << ApplyFilter(std::string((char*)&seg[row_off], kSegmentSz - row_off), args);
    }
  } while (in.good());
  return true;
}

bool TextToBin(const Arguments &args, std::istream &in, std::ostream &out) {
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
  std::cout << "записано: " << (unsigned)seg_off << " байт\n"
            << "crc32   : 0x" << std::hex << (unsigned)kCrc << std::dec
            << std::endl;
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
    BinToText(args, in, out);
  else
    TextToBin(args, in, out);
  return 0;
}
