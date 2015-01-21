#include "sdp.hpp"
#include "ymodem.hpp"
#include <list>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>

static const std::string kTtyDev("ttyUSB0");

static bool UploadDCD(boost::asio::serial_port &port, ImxFirmware &firm) {
  const ImxFirmware::DCDCmd::List &cmds = firm.GetDCDCommands();
  ImxFirmware::DCDCmd::List::const_iterator cmd_it = cmds.begin();
  for (; cmd_it != cmds.end(); cmd_it++) {
    const ImxFirmware::DCDPair::List &pairs = cmd_it->pairs;
    ImxFirmware::DCDPair::List::const_iterator pair_it = pairs.begin();
    for (; pair_it != pairs.end(); pair_it++) {
      for (uint8_t try_num = 0; try_num < 1; try_num++) {
        PktWriteMem(pair_it->addr, pair_it->val_mask).Send(port);
        PktReadMem r_pkt(pair_it->addr, PktWriteMem::kWord);
        r_pkt.Send(port);
        const uint32_t kRegVal = r_pkt.GetValue();
        if ((kRegVal & pair_it->val_mask) != pair_it->val_mask) {
          /*
          std::cout << "Ошибка записи в регистр [" << (unsigned)try_num << "]:\n"
                    << std::hex 
                    << "\t адрес    : 0x" << (unsigned)pair_it->addr << ";\n"
                    << "\t значение : 0x" << (unsigned)pair_it->val_mask << ";\n"
                    << "\t результат: 0x" << (unsigned)kRegVal << "; "
                    << std::endl;
          */
          continue;
        }
        break;
      }
    }
  }
  return true;
}

typedef std::list<std::string> ListOfStrings;

struct UBootInfo {
  std::string loadaddr;
};

struct PeripheralsInfo {
  ListOfStrings spi;
  ListOfStrings uarts;
  ListOfStrings partitions;
  ListOfStrings usb_uart;
  std::string   i2c;
  std::string   mmc;
  std::string   rtc;
  std::string   ts;
};

struct SysInfo {
  std::string board;
  std::string     u_boot_build;
  std::string     cpu;
  std::string     ddr;
  std::string     dram_size;
  std::string     nand;
  std::string     nand_size;
  std::string     sd_mmc;
  std::string     eth;
  UBootInfo       uboot;
  PeripheralsInfo kernel;
};

static void PrintSysInfo(const SysInfo &inf) {
  std::cout << "--- Описание --------------------------" << std::endl
            << "Описание:\n"
            << "\t* Плата....: " << inf.board << "\n"
            << "\t* Загрузчик: " << inf.u_boot_build << " (U-Boot)\n"
            << "\t* Процессор: " << inf.cpu << "\n"
            << "\t* ОЗУ......: " << inf.dram_size << " (" << inf.ddr << ")\n"
            << "\t* ПЗУ......: " << inf.nand_size << " (" << inf.nand << ")\n"
            << "\t* SD/MMC...: " << inf.sd_mmc << "\n"
            << "\t* Ethernet.: " << inf.eth << "\n"
            << "\t--- UBOOT ---------------------------" << std::endl
            << "\t* loadaddr.: " << inf.uboot.loadaddr << "\n"
            << std::endl;
}

static bool Parse(const std::string &str,
                  const std::string &exp,
                  unsigned           id,
                  std::string       *out) {
  if (out != 0 && out->size() > 0)
    return false;
  boost::smatch res;
  // boost::match_default | boost::match_partial
  if (not boost::regex_match(str, res, boost::regex(exp)))
    return false;
  if (out != 0)
    *out = res[id];
  return true;
}

static void SendCmd(boost::asio::serial_port &port, const std::string &cmd) {
  boost::asio::write(port, boost::asio::buffer(cmd + "\n"));
}

static bool FillInSysInfo(const std::string &str, SysInfo &inf) {
  Parse(str, "Board:(\\s*)(.+)",         2, &inf.board);
  Parse(str, "U-Boot(\\s*)(.+)",         2, &inf.u_boot_build);
  Parse(str, "CPU:(\\s*)(.+)",           2, &inf.cpu);
  Parse(str, "ddr clock([^:]*): (.+)",   2, &inf.ddr);
  Parse(str, "DRAM:(\\s*)(.+)",          2, &inf.dram_size);
  Parse(str, "Description([^:]*): (.+)", 2, &inf.nand);
  Parse(str, "Chip Size([^:]*): (.+)",   2, &inf.nand_size);
  Parse(str, "MMC:(\\s*)([^:]+)(.*)",    2, &inf.sd_mmc);
  Parse(str, "FEC0(.*)",                 0, &inf.eth);
  Parse(str, "loadaddr=(.+)",            1, &inf.uboot.loadaddr);
  return true;
}

static void SkipLines(boost::asio::serial_port &port, const size_t limit) {
  uint8_t resp[1] = {0};
  size_t lines = 0;
  do {
    boost::asio::read(port, boost::asio::buffer(resp, 1));
    if (resp[0] == 0x0D)
      lines++;
  } while (lines < limit);
}

static bool ParseUntil(boost::asio::serial_port &port,
                       const std::string        &wait_for,
                       SysInfo                  *out) {
  if (out == 0)
    return false;
  std::string str;
  uint8_t resp[1] = {0};
  while (not Parse(str, wait_for + "(.*)", 0, 0)) {
    boost::asio::read(port, boost::asio::buffer(resp, 1));
    if (resp[0] == 0x0A) {
      FillInSysInfo(str, *out);
      str = "";
    } else if (resp[0] != 0x0D) {
      str += (char)resp[0];
    }
  }
  return true;
}

static bool UploadUBoot(boost::asio::serial_port &port, const std::string &file) {
  ImxFirmware firm;
  std::cout << "Чтение образа..." << std::endl;
  firm.LoadFromFile(file);
  firm.PrintStructures(std::cout);
  std::cout << "Инициализация памяти..." << std::endl;
  UploadDCD(port, firm);
  std::cout << "Загрузка программы..." << std::endl;
  PktWriteF(&firm).Send(port);
  std::cout << "Запуск программы..." << std::endl;
  PktStatus().Send(port);
  return true;
}

static bool UploadKernel(boost::asio::serial_port &port, const std::string &file) {
  // ожидание приглашения u-boot
  SysInfo inf;
  if (not ParseUntil(port, "Hit any key to stop autoboot", &inf)) {
    return false;
  }
  SendCmd(port, "");
  ParseUntil(port, "Regiboard U-Boot >", &inf);
  SendCmd(port, "print");
  ParseUntil(port, "Regiboard U-Boot >", &inf);
  PrintSysInfo(inf);
  
  
  std::cout << "Загрузка ядра Linux..." << std::endl;
  SendCmd(port, "loady " + inf.uboot.loadaddr + " 115200; bootm " + inf.uboot.loadaddr);
  SkipLines(port, 2);
  YModem modem;
  modem.SendFile(file, port);
  return true;
}

struct Settings {
  std::string tty_dev;
  std::string imx_img;
  std::string kernel_img;
};

static bool ParseArgs(int argc, char **argv, Settings &set) {
  namespace po = boost::program_options;
  po::options_description desc("Программа загрузки плат Regiboard через COM порт");
	desc.add_options()
		("help", "описание аргументов")
		("tty",  po::value<std::string>()->default_value("/dev/" + kTtyDev),
             "путь к устройству COM")
		("img",  po::value<std::string>()->default_value("u-boot.imx"),
             "путь к образу загрузчика u-boot")
		("kernel", po::value<std::string>()->default_value("uImage.bin"),
             "путь к образу ядра Linux");
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);
	if (vm.count("help")) {
	  std::cout << desc << std::endl;
	  return false;
	}
	set.tty_dev    = vm["tty"].as<std::string>();
	set.imx_img    = vm["img"].as<std::string>();
  set.kernel_img = vm["kernel"].as<std::string>();
	return true;
}

int main(int argc, char **argv) {
  Settings set;
  if (not ParseArgs(argc, argv, set))
    return 0;
  boost::asio::io_service io_service;
  boost::asio::serial_port port(io_service, set.tty_dev);
  std::cout << "Порт \"" << kTtyDev << "\": "
            << (port.is_open() ? "открыт" : "закрыт") << std::endl;
  port.set_option( boost::asio::serial_port_base::baud_rate(115200) );
  port.set_option( boost::asio::serial_port_base::character_size(8) );
  port.set_option( boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one) );
  port.set_option( boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none) );
  port.set_option( boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none) );
  if (PktStatus().Send(port) && 
      UploadUBoot(port, set.imx_img)) {
    UploadKernel(port, set.kernel_img);
  }
  port.close();
  return 0;
}
