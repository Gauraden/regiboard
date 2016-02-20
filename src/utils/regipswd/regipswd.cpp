#include <iostream>
#include <cstdlib>
#include <string>
#include <iomanip>
#include <boost/program_options.hpp>

// см. исходники u-boot (./build/u-boot-.../lib_generic/md5.c)
extern "C" {
  void md5(unsigned char *input, int len, unsigned char output[16]);
}

static bool GeneratePswd(unsigned char *out, unsigned len) {
  static const unsigned char kAlphabet[] = "A0bC4deF1gHijKl8mNOPq5rStuv6WxYzaBcDEfGhI"
                                           "JkL3MnopQ2RsT9UVw7XyZ";
  static const size_t kAlphabetSz = sizeof(kAlphabet);
  if (out == 0 || len == 0) {
    return false;
  }
  srand(time(NULL));
  for (unsigned i = 0; i < len; i++) {
    out[i] = kAlphabet[rand() % kAlphabetSz];
  }
  return true;
}

static void PrintChars(unsigned char *in, unsigned len, bool hex = false) {
  if (hex) {
    std::cout << std::hex << std::uppercase << "0x";
  }
  std::cout.fill('0');
  if (in != 0) {
    for (unsigned i = 0; i < len; i++) {
      if (hex) {
        std::cout << std::setw(2) << (unsigned)in[i];
        continue;
      }
      std::cout << in[i];
    }    
  }
  std::cout << std::endl;
  std::cout.fill(' ');
}

static const std::string kEmptyPswd(" ");
static std::string ext_pswd;

static bool ParseArgs(int argc, char **argv) {
  namespace po = boost::program_options;
  po::options_description desc("Генератор паролей для U-Boot.");
	desc.add_options()
		("help", "описание аргументов.")
		("pswd",  po::value<std::string>()->default_value(kEmptyPswd),
             "для указания пароля вручную. Генерация будет отключена. "
             "Необязательный аргумент.");
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);
	if (vm.count("help")) {
	  std::cout << desc << std::endl;
	  return false;
	}
	ext_pswd = vm["pswd"].as<std::string>();
	return true;
}

int main(int argc, char **argv) {
  if (not ParseArgs(argc, argv)) {
    return 0;
  }
  const size_t kPswdSz = 10;
  unsigned char pswd[kPswdSz];
  unsigned char hash[16];
  if (ext_pswd == kEmptyPswd) {
    GeneratePswd(pswd, kPswdSz);
    md5(pswd, kPswdSz, hash);
    std::cout << "\t* пароль: ";
    PrintChars(pswd, kPswdSz);
  } else {
    unsigned char c_pswd[ext_pswd.size()];
    memcpy(c_pswd, ext_pswd.c_str(), ext_pswd.size());
    md5(c_pswd, ext_pswd.size(), hash);
  }
  std::cout << "\t* md5   : ";
  PrintChars(hash, 16, true);
  return 0;
}
