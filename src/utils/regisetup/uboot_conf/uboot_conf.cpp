#include <iostream>
#include <fstream>
#include <string>
#include <boost/program_options.hpp>
#include <boost/scoped_array.hpp>

static const size_t      kSegmentSz = 512;
static const std::string kMtdDev("mtd1");
static const std::string kTxtDef("uboot.conf");

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
  if (argc < 3) {
    return Arguments();
  }
  std::string in;
  std::string out;
  unsigned    oper = Arguments::kUnknown;
  
  namespace po = boost::program_options;
  po::options_description desc("Программа чтения/прошивки конфигурации uboot");
	desc.add_options()
		("help", "описание аргументов")
		("if",  po::value<std::string>()->default_value("/dev/" + kMtdDev),
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
	  return false;
	}
	in   = vm["if"].as<std::string>();
	out  = vm["of"].as<std::string>();
  oper = vm["do"].as<std::string>();  
  return Arguments(in, out, (oper == Arguments::kWrite ? Arguments::kWrite : Arguments::kRead));
}

bool BinToText(std::istream &in, std::ostream &out) {
  in.seekg(4, in.beg);
  boost::scoped_array<uint8_t> seg(new uint8_t[kSegmentSz]);
  do {
    in.read(seg.get(), kSegmentSz);
    uint8_t end_of_conf = 0;
    for (size_t off = 0; off < kSegmentSz; off++) {
      if (seg[off] == 0)
        end_of_conf++;
      else
        end_of_conf = 0;
      // конец конфигурации
      if (end_of_conf > 1)
        return true;
    }
  } while (in.good());
  return true;
}

bool TextToBin(std::istream &in, std::ostream &out) {
  out.seekp(4, out.beg);
  
  return true;
}

int main(int argc, char **argv) {
  Arguments args(ParseArgs(argc, argv));
  if (args.oper == Arguments::kUnknown) {
    return 0;
  }
  std::fstream in(args.in.c_str());
  std::fstream out(args.out.c_str());
  if (args.oper == Arguments::kRead)
    BinToText(in, out);
  else
    TextToBin(in, out);
  return 0;
}
