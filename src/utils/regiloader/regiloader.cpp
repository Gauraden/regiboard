#include "regiloader.hpp"
#include "sdp.hpp"
#include "ymodem.hpp"
#include "tftp.hpp"
#include <iostream>
#include <sstream>
#include <list>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>

static const std::string kTtyDev("ttyUSB0");
static const std::string kUBootImg("u-boot.imx");
static const std::string kKernelImg("uImage");
static const std::string kRamFsImg("ramFs");

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

typedef std::list<std::string>   ListOfStrings;
typedef boost::asio::serial_port SerialPort;

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
  std::string   lcd;
};

struct SysInfo {
  std::string     board;
  std::string     u_boot_build;
  std::string     kernel_build;
  std::string     gcc_ver;
  std::string     ct_ng_ver; // crosstool-NG
  std::string     cpu;
  std::string     ddr;
  std::string     dram_size;
  std::string     nor;
  std::string     nor_size;
  std::string     nand;
  std::string     nand_size;
  std::string     sd_mmc;
  std::string     eth;
  UBootInfo       uboot;
  PeripheralsInfo kernel;
};

static void PrintListOfStrings(const std::string   &pref, 
                               const ListOfStrings &list) {
  for (auto str_it = list.begin(); str_it != list.end(); str_it++) {
    std::cout << pref << *str_it << "\n";
  }
}

static void PrintSysInfo(const SysInfo &inf) {
  std::cout << "Описание:\n"
            << "\t * Плата.....: " << inf.board << "\n"
            << "\t * Процессор.: " << inf.cpu << "\n"
            << "\t--- UBOOT ---------------------------" << std::endl
            << "\t * Версия....: " << inf.u_boot_build << "\n"
            << "\t * loadaddr..: " << inf.uboot.loadaddr << "\n"
            << "\t--- Kernel (Linux) ------------------" << std::endl
            << "\t * Версия....: " << inf.kernel_build << "\n"
            << "\t * GCC.......: " << inf.gcc_ver << "\n"
            << "\t * Crosstool.: " << inf.ct_ng_ver << "(crosstool-NG)\n"
            << "\t--- Периферия -----------------------" << std::endl
            << "\t * ОЗУ.......: " << inf.dram_size << " (" << inf.ddr << ")\n"
            << "\t * ПЗУ (NOR).: " << inf.nor_size << " (" << inf.nor << ")\n"
            << "\t * ПЗУ (NAND): " << inf.nand_size << " (" << inf.nand << ")\n"
            << "\t * SD/MMC....: " << inf.sd_mmc << "\n"
            << "\t * Ethernet..: " << inf.eth << "\n"
            << "\t * Сенсор.эк.: " << inf.kernel.ts << "\n"
            << "\t * Дисплей...: " << inf.kernel.lcd << "\n";
  std::cout << "\t * SPI.......: " << (unsigned)inf.kernel.spi.size() << "\n";
  PrintListOfStrings("\t\t - ", inf.kernel.spi);
  std::cout << "\t * UART......: " << (unsigned)inf.kernel.uarts.size() << "\n";
  PrintListOfStrings("\t\t - ", inf.kernel.uarts);
  std::cout << "\t * USB-UART..: " << (unsigned)inf.kernel.usb_uart.size() << "\n";
  PrintListOfStrings("\t\t - ", inf.kernel.usb_uart);
  std::cout << "\t * Разделы...: " << (unsigned)inf.kernel.partitions.size() << "\n";
  PrintListOfStrings("\t\t - ", inf.kernel.partitions);
  std::cout << std::endl;
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

static void SendCmd(SerialPort &port, const std::string &cmd) {
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
  // dmesg
  std::string kernel_ver;
  std::string kernel_builder;
  std::string kernel_build_dt;
  if (Parse(str, "(.+)Linux version ([0-9a-z\\.\\-]+)(.+)", 2, &kernel_ver)) {
    Parse(str, "(.+)\\(([a-z@A-Z]+)\\) \\(gcc(.+)",           2, &kernel_builder);
    Parse(str, "(.+)gcc version (.+) \\(prerelease\\)(.+)", 2, &inf.gcc_ver);
    Parse(str, "(.+)crosstool-NG ([^\\)]+)\\)(.+)",         2, &inf.ct_ng_ver);
    Parse(str, "(.+)PREEMPT (.*)",                          2, &kernel_build_dt);
    inf.kernel_build = kernel_ver + " (" + kernel_builder + ": " + kernel_build_dt + ")";
  }
  if (Parse(str, "(.+)mxc_dataflash spi([^\\:]+): ([a-z0-9]+)(.+)", 3, &inf.nor)) {
    Parse(str, "(.+)\\(([^\\)]+)\\) pagesize(.+)", 2, &inf.nor_size);
  }
  Parse(str, "(.+)Regiboard TS: (.+)",        2, &inf.kernel.ts);
  Parse(str, "(.+)Regiboard LCD setup: (.+)", 2, &inf.kernel.lcd);
  std::string t_str;
  if (Parse(str, "(.+)CSPI: ([^\\x20]+)(.+)", 2, &t_str)) {
    inf.kernel.spi.push_back(t_str);
  }
  if (Parse(str, "(.+)mxcintuart.(\\d+): (\\w+)(.+)", 3, &t_str)) {
    inf.kernel.uarts.push_back(t_str);
  }
  if (Parse(str, "(.+)USB Serial Device converter now attached to (\\w+)", 2, &t_str)) {
    inf.kernel.usb_uart.push_back(t_str);
  }
  if (Parse(str, "(.+)0x([0-9a-f]+)-0x([0-9a-f]+) : \"(\\w+)\"", 4, &t_str)) {
    inf.kernel.partitions.push_back(t_str);
  }
  return true;
}

static void SkipLines(SerialPort &port, const size_t limit) {
  uint8_t resp[1] = {0};
  size_t lines = 0;
  do {
    boost::asio::read(port, boost::asio::buffer(resp, 1));
    if (resp[0] == 0x0D)
      lines++;
  } while (lines < limit);
}

static bool ParseUntil(SerialPort        &port,
                       const std::string &wait_for,
                       SysInfo           *out) {
  if (out == 0) {
    return false;
  }
  std::string str;
  uint8_t resp[1] = {0};
  while (not Parse(str, wait_for + "(.*)", 0, 0)) {
    boost::asio::read(port, boost::asio::buffer(resp, 1));
    if (resp[0] == 0x0A) {
      ShowProcess("\t * Ожидание");
      // DEBUG --------------------
      // std::cout << "DEBUG: " << str << std::endl;
      // --------------------------
      FillInSysInfo(str, *out);
      str = "";
    } else if (resp[0] != 0x0D) {
      str += (char)resp[0];
    }
  }
  std::cout << std::endl;
  return true;
}

static bool UploadUBoot(SerialPort &port, const std::string &file) {
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

static bool UploadKernelBegin(SerialPort &port, SysInfo *inf) {
  // ожидание приглашения u-boot
  if (inf == 0 || not ParseUntil(port, "Hit any key to stop autoboot", inf)) {
    return false;
  }
  SendCmd(port, "");
  ParseUntil(port, "Regiboard U-Boot >", inf);
  SendCmd(port, "print");
  ParseUntil(port, "Regiboard U-Boot >", inf);
  std::cout << "Загрузка ядра Linux..." << std::endl;
  return true;
}

static bool UploadKernelEnd(SerialPort &port, SysInfo *inf) {
  if (inf == 0) {
    return false;
  }
  ParseUntil(port, "\\[([^\\]]+)\\] usb 2-1.3: FTDI USB Serial Device converter now attached to ttyUSB11", inf);
  inf->kernel.usb_uart.push_back("ttyUSB11");
  PrintSysInfo(*inf);
  return true;
}

static std::string BootM(const SysInfo &inf) {
  return std::string("; bootm ${loadaddr}");
}

static bool UploadKernelOverUart(SerialPort &port, const std::string &file) {
  SysInfo inf;
  if (not UploadKernelBegin(port, &inf)) {
    return false;
  }
  SendCmd(port, "loady ${loadaddr} 115200" + BootM(inf));
  SkipLines(port, 2);
  YModem modem;
  modem.SendFile(file, port);
  return UploadKernelEnd(port, &inf);
}

static bool UploadKernelOverEth(SerialPort   &port,
                                TFtp::Server *srv) {
  if (srv == 0) {
    return false;
  }
  SysInfo inf;
  if (not UploadKernelBegin(port, &inf)) {
    return false;
  }  
  SendCmd(port, "dhcp ${loadaddr} " + srv->get_ip() + ":" + kKernelImg + 
          BootM(inf));
  srv->Run();
  return UploadKernelEnd(port, &inf);
}

static bool UploadRamFsOverEth(SerialPort   &port,
                               TFtp::Server *srv) {
  // TODO
  if (srv == 0) {
    return false;
  }
  /*
  SysInfo inf;
  srv->Run();
  */
  return true;
}

struct Settings {
  std::string tty_dev;
  std::string imx_img;
  std::string kernel_img;
  std::string ramfs_img;
  std::string use_tftp;
  bool        just_eth;
};

static bool ParseArgs(int argc, char **argv, Settings &set) {
  namespace po = boost::program_options;
  po::options_description desc("Программа загрузки плат Regiboard через COM/Ethernet порты.");
	desc.add_options()
		("help", "описание аргументов.")
		("tty",  po::value<std::string>()->default_value("/dev/" + kTtyDev),
             "путь к устройству COM.")
		("img",  po::value<std::string>()->default_value(kUBootImg),
             "путь к образу загрузчика u-boot.")
		("kernel", po::value<std::string>()->default_value(kKernelImg),
             "путь к образу ядра Linux. Ядро не будет загружено если не указать файл.")
		("ramfs", po::value<std::string>()->default_value(kRamFsImg),
             "путь к образу файловой системы. Необязательный параметр, работает "
             "только совместно с флагом: --tftp.")
    ("tftp", po::value<std::string>()->default_value("0.0.0.0"),
             "использовать TFTP, необходимо указать IP интерфейса для запуска "
             "сервера.")
    ("just_eth", po::value<bool>()->default_value(false)->zero_tokens(),
                 "использовать только загрузку через Ethernet, не использовать"
                 "UART");
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
  set.ramfs_img  = vm["ramfs"].as<std::string>();
  set.use_tftp   = vm["tftp"].as<std::string>();
  set.just_eth   = vm["just_eth"].as<bool>();
	return true;
}

void ShowPercentage(const std::string &msg, uint8_t percent) {
  if (percent > 100) {
    percent = 100;
  }
  std::cout << "\r\033[K"
            << msg << ": "
            << (unsigned)percent << " %"
            << std::flush;
}

void ShowProcess(const std::string &msg) {
  static const uint8_t kMaxNum = 10;
  static uint8_t num = 1;
  std::stringstream dots;
  for (uint8_t i = 0; i < num; i++) {
    dots << ".";
  }
  std::cout << "\r\033[K" << msg << ": " << dots.str() << std::flush;
  num++;
  if (num > kMaxNum) {
    num = 1;
  }
}

int main(int argc, char **argv) {
  Settings set;
  if (not ParseArgs(argc, argv, set)) {
    return 0;
  }
  if (set.just_eth) {
    TFtp::Server srv(set.use_tftp);
    srv.PublishFile(kKernelImg, set.kernel_img);
    srv.PublishFile(kRamFsImg,  set.ramfs_img);
    if (set.kernel_img.size() > 0) {
      srv.Run();
    }
    return 0;
  }
  boost::asio::io_service io_service;
  boost::asio::serial_port port(io_service, set.tty_dev);
  std::cout << "Порт \"" << set.tty_dev << "\": "
            << (port.is_open() ? "открыт" : "закрыт") << std::endl;
  port.set_option( boost::asio::serial_port_base::baud_rate(115200) );
  port.set_option( boost::asio::serial_port_base::character_size(8) );
  port.set_option( boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one) );
  port.set_option( boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none) );
  port.set_option( boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none) );
  if (PktStatus().Send(port) && 
      UploadUBoot(port, set.imx_img) &&
      set.kernel_img.size() > 0 &&
      set.use_tftp.size() == 0) {
    UploadKernelOverUart(port, set.kernel_img);
  }
  // загрузка через Ethernet
  if (set.use_tftp.size() > 0) {
    TFtp::Server srv(set.use_tftp);
    srv.PublishFile(kKernelImg, set.kernel_img);
    srv.PublishFile(kRamFsImg,  set.ramfs_img);
    if (set.ramfs_img.size() > 0) {
      UploadRamFsOverEth(port, &srv);
    }
    if (set.kernel_img.size() > 0) {
      UploadKernelOverEth(port, &srv);
    }
  }
  port.close();
  return 0;
}
