#include "regiloader.hpp"
#include "sdp.hpp"
#include "ymodem.hpp"
#include "tftp.hpp"
#include "debug_backtrace.hpp"
#include <iostream>
#include <sstream>
#include <list>
#include <vector>
#include <map>
#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time/local_time/local_time.hpp>

typedef std::list<std::string> Recipe;

static const std::string kConfName("regiloader.json");
static const std::string kTtyDev("ttyUSB0");
static const std::string kUBootImg("u-boot");
static const std::string kKernelImg("uImage");
static const std::string kRootFsImg("rootfs.tar.gz");
static const std::string kMtdUtils("mtd-utils.tar.gz");
static const std::string kIpkPath("output/packets");
static const std::string kRegigrafIpk("regigraf.ipk");
static const std::string kRegigrafLicense("license.xml");
static const std::string kRbfName("cortex_a8.regigraf.1772.53.UNIVERSAL-last.rbf");

static const std::string kHwAddr("00:60:2F:10:20:30");
static const std::string kRootFsTar("/tmp/rootfs.tar.gz");

static const std::string kUBootPswdPrompt("Enter password:");
static const std::string kUBootPrompt("Regiboard U-Boot >");
static const std::string kRamFsPrompt("(.*)regiboard \\(initramfs\\)");
static const std::string kRegigrafLogin("regigraf login:");
static const std::string kRegigrafPrompt("(.*)\\@regigraf");

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
typedef std::vector<std::string> VectorOfStrings;
typedef boost::asio::serial_port SerialPort;

template <typename Type>
struct Range {
  Range(const Type &_from, const Type &_to): from(_from), to(_to) {}
  Type Length() const {
    return (to - from);
  }
  
  Type from;
  Type to;
};

struct UBootInfo {
  std::string loadaddr;
};

struct Packet {
  typedef std::list<Packet> List;

  Packet(const std::string &_name, const std::string &_version)
      : name(_name),
        version(_version) {}
  operator std::string() const {
    std::stringstream buf;
    buf << name << ": " << version;
    return buf.str();
  }
  std::string name;
  std::string version;
};

struct Partition {
  typedef Range<uint64_t>      Area;
  typedef std::list<Partition> List;

  Partition(const std::string &_name, const Area &_area)
      : name(_name),
        area(_area) {}
  operator std::string() const {
    std::stringstream buf;
    buf << name << ": " << (unsigned long long)area.Length() << " bytes";    
    return buf.str();
  }
  std::string name;
  uint8_t     mtd_id;
  Area        area;
};

struct PeripheralsInfo {
  ListOfStrings   spi;
  ListOfStrings   uarts;
  Partition::List partitions;
  ListOfStrings   usb_uart;
  std::string     i2c;
  std::string     mmc;
  std::string     rtc;
  std::string     ts;
  std::string     lcd;
};

struct SysInfo {
  SysInfo(): wait_for_reboot(false), upload_fail(false), boot_from_nand(false) {}

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
  std::string     board_ip;
  std::string     board_hw_addr;
  UBootInfo       uboot;
  PeripheralsInfo kernel;
  Packet::List    packets;
  bool            wait_for_reboot;
  bool            upload_fail;
  bool            boot_from_nand;
};

static SysInfo g_sys_inf;

enum Color {
  kReset   = -1,
	kBlack   = 0,
	kRed     = 1,
	kGreen   = 2,
	kYellow  = 3,
	kBlue    = 4,
	kMagenta = 5,
	kCyan    = 6,
	kWhite   = 7
};

static std::string UseColor(int8_t id) {
  std::stringstream esc;
  if (id >= 0) {
    esc << "\033[1;" << (unsigned)(30 + (id % 8)) << "m";
  } else {
    esc << "\033[0m";
  }
  return esc.str();
}

class SysState {
public:
  SysState()
      : stand(1),
        batch(3),
        device(1),
        lcd_model("G150XGE-L04"),
        ts_model("ADS7843E"),
        verbose(false) {}
  
  void CheckBatchCapacity() {
    if (device < 256) {
      return;
    }
    batch++;
    device = 1;
  }
  
  void Save() {
    if (_file_path.size() == 0) {
      return;
    }
    using boost::property_tree::ptree;
    ptree pt;
    pt.put("stand_id",  (unsigned)stand);
    pt.put("batch_id",  (unsigned)batch);
    pt.put("device_id", (unsigned)device);
    pt.put("lcd_model", lcd_model);
    pt.put("ts_model",  ts_model);
    write_json(_file_path, pt);
  }
  
  bool LoadFrom(const std::string &path) {
    using boost::property_tree::ptree;
    ptree pt;
    _file_path = path;
    try {
      read_json(path, pt);
    } catch (...) {
      return false;
    }
    stand     = pt.get<unsigned>("stand_id");
    batch     = pt.get<unsigned>("batch_id");
    device    = pt.get<unsigned>("device_id");
    lcd_model = pt.get<std::string>("lcd_model");
    ts_model  = pt.get<std::string>("ts_model");
    return true;
  }
  
  std::string GetDeviceId() const {
    std::stringstream buf;
      buf << (unsigned)stand  << "."
          << (unsigned)batch  << "."
          << (unsigned)device << " ";
    return buf.str();
  }
  
  void Print() const {
    std::cout << "Конфигурация: \n"
              << "\t - Стэнд  : " << (unsigned)stand  << "\n"
              << "\t - Партия : " << (unsigned)batch  << "\n"
              << "\t - Плата  : " << (unsigned)device << "\n"
              << "\t - Дисплей: " << lcd_model << "\n"
              << "\t - Сенсор : " << ts_model << "\n"
              << "\t - Идентификатор: " << GetDeviceId() << "\n"
              << std::endl;
  }
  
  uint32_t    stand;
  uint32_t    batch;
  uint32_t    device;
  std::string lcd_model;
  std::string ts_model;
  bool        verbose;
private:
  std::string _file_path;
};

static SysState g_sys_state;

static std::string GetTime() {
  namespace pt = boost::posix_time;
  struct tm ltm = to_tm(pt::second_clock::local_time());
  return static_cast<std::stringstream&>(std::stringstream() 
    << (ltm.tm_year + 1900) << "-"
    << (ltm.tm_mon + 1)     << "-"
    << ltm.tm_mday          << " "
    << ltm.tm_hour          << ":"
    << ltm.tm_min           << ":"
    << ltm.tm_sec
  ).str();
}

template <typename ListType>
static void PrintList(const std::string         &pref, 
                      const std::list<ListType> &list) {
  for (auto it = list.begin(); it != list.end(); it++) {
    std::cout << pref << (std::string)*it << "\n";
  }
}

static Partition* FindPartition(const std::string name, Partition::List *list) {
  if (list == 0 || list->size() == 0) {
    return 0;
  }
  Partition::List::iterator it = list->begin();
  for (; it != list->end(); it++) {
    if (it->name == name) {
      return &(*it);
    }
  }
  return 0;
}

static void PrintDelimiter(const std::string &pref, const std::string &name, uint16_t len) {
  std::string part0("---");
  if (name.size() > 0) {
    part0 += " " + name + " ";
  }
  std::cout.fill('-');
  std::cout << UseColor(kYellow)
            << pref << part0 << std::setw(len - part0.size())
            << UseColor(kReset) << "\n";
  std::cout.fill(' ');
}

static void PrintSysInfo(const SysInfo &inf) {
  const Color kGroupColor = kYellow;
  std::cout << "Описание:\n"
            << "\t * Плата.....: " << inf.board << "\n"
            << "\t * Процессор.: " << inf.cpu << "\n"
            << UseColor(kGroupColor)
            << "\t--- UBOOT ---------------------------" << std::endl
            << UseColor(kReset)
            << "\t * Версия....: " << inf.u_boot_build << "\n"
            << "\t * loadaddr..: " << inf.uboot.loadaddr << "\n"
            << UseColor(kGroupColor)
            << "\t--- Kernel (Linux) ------------------" << std::endl
            << UseColor(kReset)
            << "\t * Версия....: " << inf.kernel_build << "\n"
            << "\t * GCC.......: " << inf.gcc_ver << "\n"
            << "\t * Crosstool.: " << inf.ct_ng_ver << "(crosstool-NG)\n"
            << UseColor(kGroupColor)
            << "\t--- Периферия -----------------------" << std::endl
            << UseColor(kReset)
            << "\t * ОЗУ.......: " << inf.dram_size << " (" << inf.ddr << ")\n"
            << "\t * ПЗУ (NOR).: " << inf.nor_size << " (" << inf.nor << ")\n"
            << "\t * ПЗУ (NAND): " << inf.nand_size << " (" << inf.nand << ")\n"
            << "\t * SD/MMC....: " << inf.sd_mmc << "\n"
            << "\t * Ethernet..: " << inf.eth << "\n"
            << "\t * RTC.......: " << inf.kernel.rtc << "\n"
            << "\t * Сенсор.эк.: " << inf.kernel.ts << "\n"
            << "\t * Дисплей...: " << inf.kernel.lcd << "\n"
            << "\t * MAC-адрес.: " << inf.board_hw_addr << "\n";
  std::cout << "\t * SPI.......: " << (unsigned)inf.kernel.spi.size() << "\n";
  PrintList("\t\t - ", inf.kernel.spi);
  std::cout << "\t * UART......: " << (unsigned)inf.kernel.uarts.size() << "\n";
  PrintList("\t\t - ", inf.kernel.uarts);
  std::cout << "\t * USB-UART..: " << (unsigned)inf.kernel.usb_uart.size() << "\n";
  PrintList("\t\t - ", inf.kernel.usb_uart);
  std::cout << "\t * Разделы...: " << (unsigned)inf.kernel.partitions.size() << "\n";
  PrintList("\t\t - ", inf.kernel.partitions);
  std::cout << "\t * Пакеты....: " << (unsigned)inf.packets.size() << "\n";
  PrintList("\t\t - ", inf.packets);
  std::cout << std::endl;
}

static void SaveSysInfoToJson(const std::string &path,
                              const SysState    &state,
                              const SysInfo     &inf) {
  const std::string kFileName(static_cast<std::stringstream&>(
    std::stringstream() << path << "/board-"
                        << (unsigned)state.stand << "."
                        << (unsigned)state.batch << "."
                        << (unsigned)state.device << ".json"
  ).str());
  using boost::property_tree::ptree;
  ptree pt;
  pt.put("board",      inf.board);
  pt.put("cpu",        inf.cpu);
  pt.put("uboot",      inf.u_boot_build);
  pt.put("kernel",     inf.kernel_build);
  pt.put("gcc",        inf.gcc_ver);
  pt.put("crosstool",  inf.ct_ng_ver);
  pt.put("dram.size",  inf.dram_size);
  pt.put("dram.freq",  inf.ddr);
  pt.put("nor.size",   inf.nor_size);
  pt.put("nor.model",  inf.nor);
  pt.put("nand.size",  inf.nand_size);
  pt.put("nand.model", inf.nand);
  pt.put("mac_addr",   inf.board_hw_addr);
  pt.put("rtc",        inf.kernel.rtc);
  pt.put("ts",         inf.kernel.ts);
  pt.put("lcd",        inf.kernel.lcd);
  pt.put("spi",        (unsigned)inf.kernel.spi.size());
  pt.put("uart",       (unsigned)inf.kernel.uarts.size());
  pt.put("usb-uart",   (unsigned)inf.kernel.usb_uart.size());
  Partition::List::const_iterator part_it = inf.kernel.partitions.begin();
  for (; part_it != inf.kernel.partitions.end(); part_it++) {
    pt.put("partitions." + part_it->name, (unsigned)part_it->area.Length());
  }
  Packet::List::const_iterator pkt_it = inf.packets.begin();
  for (; pkt_it != inf.packets.end(); pkt_it++) {
    pt.put("packets." + pkt_it->name, pkt_it->version);
  }
  write_json(kFileName, pt);
}

static bool Parse(const std::string &str,
                  const std::string &exp,
                  unsigned           id,
                  std::string       *out) {
  if (out != 0 && out->size() > 0) {
    return false;
  }
  boost::smatch res;
  if (not boost::regex_match(str, res, boost::regex(exp), boost::match_default)) {
//    std::cout << str << " -> " << exp << std::endl;
    return false;
  }
  if (out != 0) {
    *out = res[id];
  }
  return true;
}

static bool Parse(const std::string &str,
                  const std::string &exp,
                  VectorOfStrings   *out) {
  if (out != 0 && out->size() > 0) {
    return false;
  }
  boost::smatch res;
  if (not boost::regex_match(str, res, boost::regex(exp), boost::match_default)) {
    return false;
  }
  if (out == 0) {
    return false;
  }
  for (uint8_t id = 0; id < res.size(); id++) {
    out->push_back(res[id]);
  }
  return true;
}

static void SendCmd(SerialPort &port, const std::string &cmd) {
  boost::asio::write(port, boost::asio::buffer(cmd + "\r"));
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
    Parse(str, "(.+)\\(([a-z@A-Z]+)\\) \\(gcc(.+)",         2, &kernel_builder);
    Parse(str, "(.+)gcc version (.+) \\(prerelease\\)(.+)", 2, &inf.gcc_ver);
    Parse(str, "(.+)crosstool-NG ([^\\)]+)\\)(.+)",         2, &inf.ct_ng_ver);
    Parse(str, "(.+)PREEMPT (.*)",                          2, &kernel_build_dt);
    inf.kernel_build = kernel_ver + " (" + kernel_builder + ": " + kernel_build_dt + ")";
    return true;
  }
  if (Parse(str, "(.+)mxc_dataflash spi([^\\:]+): ([a-zA-Z0-9]+)(.+)", 3, &inf.nor)) {
    Parse(str, "(.+)\\(([^\\)]+)\\) pagesize(.+)", 2, &inf.nor_size);
    return true;
  }
  Parse(str, "(.+)input: ([^ ]+) Touchscreen as (.+)", 2, &inf.kernel.ts);
  Parse(str, "(.+)Regiboard LCD setup: (.+)", 2, &inf.kernel.lcd);
  Parse(str, "(.+)rtc-([^:]+): rtc core: registered ([^ ]+) as rtc0", 3, &inf.kernel.rtc);
  std::string t_str;
  if (Parse(str, "(.+)CSPI: ([^\\x20]+)(.+)", 2, &t_str)) {
    inf.kernel.spi.push_back(t_str);
    return true;
  }
  if (Parse(str, "(.+)mxcintuart.(\\d+): (\\w+)(.+)", 3, &t_str)) {
    inf.kernel.uarts.push_back(t_str);
    return true;
  }
  if (Parse(str, "(.+)USB Serial Device converter now attached to (\\w+)", 2, &t_str)) {
    inf.kernel.usb_uart.push_back(t_str);
    return true;
  }
  VectorOfStrings t_list;
  if (Parse(str, "(.+)0x([0-9a-f]+)-0x([0-9a-f]+) : \"(\\w+)\"", &t_list)) {
    const unsigned long long kFrom = std::stoull(t_list[2], 0, 16);
    const unsigned long long kTo   = std::stoull(t_list[3], 0, 16);
    inf.kernel.partitions.push_back(Partition(t_list[4], Partition::Area(kFrom, kTo)));
    return true;
  }
  t_list.clear();
  if (Parse(str, "mtd([0-9]+): ([0-9a-z]+) ([0-9a-z]+) \"([^\"]+)\"", &t_list)) {
    Partition *part = FindPartition(t_list[4], &inf.kernel.partitions);
    if (part != 0) {
      part->mtd_id = std::stoi(t_list[1]);
    }
  }
  if (Parse(str, "Lease of ([0-9\\.]+)(.*)", 1, &t_str)) {
    inf.board_ip = t_str;
    return true;
  }
  // Please Power Cycle the board for the change to take effect
  if (Parse(str, "Please Power Cycle the board(.*)", 1, &t_str)) {
    inf.wait_for_reboot = true;
    return true;
  }
  // ERROR: can't get kernel image
  if (Parse(str, "(.*)ERROR: can't get kernel image(.*)", 1, &t_str)) {
    inf.upload_fail = true;
    return true;
  }  
  t_list.clear();
  if (Parse(str, "(.+) - ([0-9\\-\\.]+) -(.*)", &t_list)) {
    inf.packets.push_back(Packet(t_list[1], t_list[2]));
    return true;
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
  std::string str;
  uint8_t resp[1] = {0};
  std::cout << "\033[1A";
  while (not Parse(str, wait_for + "(.*)", 0, 0)) {
    boost::asio::read(port, boost::asio::buffer(resp, 1));
    if (resp[0] == 0x0A) {
      if (g_sys_state.verbose) {
        std::cout << str << std::endl;
      } else {
        ShowProcess("\t * Ожидание исполнения команды");
      }
      if (out != 0) {
        FillInSysInfo(str, *out);
      }
      str = "";
    } else if (resp[0] != 0x0D) {
      str += (char)resp[0];
    }
  }
  std::cout << std::endl;
  return true;
}

static void SendToUBoot(SerialPort &port, const std::string &cmd, SysInfo *out) {
  SendCmd(port, cmd);
  ParseUntil(port, kUBootPrompt, out);
}

static std::string g_shell_prompt = kRamFsPrompt;

static void SwitchShell(const std::string &prompt) {
  g_shell_prompt = prompt;
}

static void SendToShell(SerialPort &port, const std::string &cmd, SysInfo *out) {
  SendCmd(port, cmd);
  ParseUntil(port, g_shell_prompt, out);
}

static void UpdateListOfPartitions(SerialPort &port) {
  static bool parts_are_ready = false;
  if (not parts_are_ready) {
    SendToShell(port, "cat /proc/mtd", &g_sys_inf);
    parts_are_ready = true;
  }
}

static bool UploadUBoot(SerialPort &port, const std::string &file) {
  static const unsigned kMaxTries = 3;
  if (not PktStatus().Send(port, kMaxTries, "Запрос состояния микропроцессора")) {
    return false;
  }
  ImxFirmware firm;
  std::cout << "Чтение образа..." << std::endl;
  firm.LoadFromFile(file);
  firm.PrintStructures(std::cout);
  std::cout << "Инициализация памяти..." << std::endl;
  UploadDCD(port, firm);
  std::cout << "Загрузка программы..." << std::endl;
  if (not PktWriteF(&firm).Send(port, kMaxTries, "Загрузка исполняемой программы")) {
    return false;
  }
  std::cout << "Запуск программы..." << std::endl;
  return PktComplete().Send(port, kMaxTries, "Запуск программы");
}

static bool already_has_been = false;

static bool WaitForWelcomeFromUBoot(SerialPort        &port,
                                    SysInfo           *inf,
                                    const std::string &pswd) {
  if (already_has_been && not inf->wait_for_reboot) {
    return true;
  }
  const bool kHasGot = ParseUntil(port, "Hit any key to stop autoboot", inf);
  if (not kHasGot) {
    return false;
  }
  already_has_been = true;
  if (pswd.size() > 0) {
    SendCmd(port, "");
    ParseUntil(port, kUBootPswdPrompt, 0);
    SendToUBoot(port, pswd, 0);
  } else {
    SendToUBoot(port, "", 0);
  }
  if (inf->wait_for_reboot) {
    SendToUBoot(port, "save", &g_sys_inf);
  }
  return true;
}

static bool SetupNor(SerialPort &port, const std::string &pswd) {
  if (not WaitForWelcomeFromUBoot(port, &g_sys_inf, pswd)) {
    return false;
  }
  SendToUBoot(port, "usedefenv", &g_sys_inf);
  SendToUBoot(port, "save", &g_sys_inf);
  if (g_sys_inf.wait_for_reboot) {
    std::cout << UseColor(kGreen)
              << "Требуется ручная перезагрузка платы!" << std::endl
              << "1. Перезагрузите плату (отключите питание и снова включите его)" << std::endl
              << "2. Нажмите [Enter] для продолжения работы программы..."
              << UseColor(kReset)
              << std::endl;
    std::cin.get();
    std::cout << "Повторная загрузка платы" << std::endl;
    return true;
  }
  return false;
}

static bool UploadKernelBegin(SerialPort       &port,
                              SysInfo          *inf,
                              const std::string &pswd) {
  if (inf == 0 || not WaitForWelcomeFromUBoot(port, inf, pswd)) {
    return false;
  }
  already_has_been = false;
  if (not inf->boot_from_nand) {
    // Убираем имя монтируемого устройства, для предотвращения загрузки с nand.
    // Таким образом загрузка всегда будет останавливаться на initramfs
    SendToUBoot(port, "setenv nandroot", inf);
  }
  SendToUBoot(port, "run conf_hw", inf);
  SendToUBoot(port, "run bootargs_nand", inf);
  std::cout << "Загрузка ядра Linux..." << std::endl;
  return true;
}

static bool UploadKernelEnd(SerialPort &port, SysInfo *inf) {
  if (inf == 0) {
    return false;
  }
  ParseUntil(port, "\\[([^\\]]+)\\] usb 2-1.3: FTDI USB Serial Device converter now attached to ttyUSB11", inf);
  inf->kernel.usb_uart.push_back("ttyUSB11");
  return true;
}

static std::string BootM(const SysInfo &inf) {
  return std::string("; bootm ${loadaddr}");
}

static bool UploadKernelOverUart(SerialPort        &port,
                                 const std::string &file,
                                 const std::string &pswd) {
  if (not UploadKernelBegin(port, &g_sys_inf, pswd)) {
    return false;
  }
  SendCmd(port, "loady ${loadaddr} 115200" + BootM(g_sys_inf));
  SkipLines(port, 2);
  YModem modem;
  modem.SendFile(file, port);
  return UploadKernelEnd(port, &g_sys_inf);
}

static bool UploadKernelOverEth(SerialPort        &port,
                                TFtp::Server      *srv,
                                const std::string &pswd) {
  if (srv == 0) {
    return false;
  }
  do {
    if (not UploadKernelBegin(port, &g_sys_inf, pswd)) {
      return false;
    }
    std::stringstream args;
    args << (unsigned)TFtp::Server::kPort;
    SendCmd(port, "setenv tftpdstp " + args.str());
    SendCmd(port, "dhcp ${loadaddr} " + srv->get_ip() + ":" + kKernelImg + 
            BootM(g_sys_inf));
    srv->Run();
    if (g_sys_inf.upload_fail) {
      std::cout << "Не удалось загрузить ядро, пробуем ещё раз" << std::endl;
    }
  } while (g_sys_inf.upload_fail);
  g_sys_inf.upload_fail = false;
  return UploadKernelEnd(port, &g_sys_inf);
}

static bool UploadKernelAndDebug(SerialPort        &port,
                                 TFtp::Server      *srv,
                                 const std::string &pswd) {
  if (not WaitForWelcomeFromUBoot(port, &g_sys_inf, pswd)) {
    return false;
  }
  SendToUBoot(port, "setenv dyndbg 'file i2c-imx.c +p'", 0);
  return UploadKernelOverEth(port, srv, pswd);
}

static bool UploadKernelWithOldSys(SerialPort        &port,
                                   TFtp::Server      *srv,
                                   const std::string &pswd) {
  if (not WaitForWelcomeFromUBoot(port, &g_sys_inf, pswd)) {
    return false;
  }
  g_sys_inf.boot_from_nand = true;
  return UploadKernelOverEth(port, srv, pswd);
}

static bool DownloadFromTFtp(const std::string &tftpd_ip,
                             const std::string &request,
                             const std::string &path,
                             SerialPort        &port,
                             TFtp::Server      *srv) {
  if (srv == 0 || tftpd_ip.size() == 0 || request.size() == 0 ||
      path.size() == 0) {
    return false;
  }
  static bool eth_is_ready = false;
  if (not eth_is_ready && g_shell_prompt != kRegigrafPrompt) {
    std::cout << "Инициализация сетевого интерфейса..." << std::endl;
    SendToShell(port, std::string("ifconfig eth0 up hw ether ") + kHwAddr, 0);
    SendToShell(port, "udhcpc", &g_sys_inf);
    std::cout << "\t HW: " << kHwAddr            << std::endl
              << "\t IP: " << g_sys_inf.board_ip << std::endl;
    SendToShell(port, "ifconfig eth0 " + g_sys_inf.board_ip, 0);
    eth_is_ready = true;
  }
  SendCmd(port, "tftp -l " + path + " -g -r " + request + " " + tftpd_ip);
  const bool kRunRes = srv->Run();
  ParseUntil(port, g_shell_prompt, 0);
  return kRunRes;
}

static bool UploadRootFsOverEth(const std::string &tftpd_ip,
                                SerialPort        &port,
                                TFtp::Server      *srv) {
  if (srv == 0) {
    return false;
  }
  std::cout << "Загрузка архива rootfs..." << std::endl;
  return DownloadFromTFtp(tftpd_ip, kRootFsImg, kRootFsTar, port, srv);
}

static bool MkUbiFsPartition(SerialPort        &port,
                             uint8_t            ubi_id,
                             const std::string &vol_name) {
  Partition *part = FindPartition(vol_name, &g_sys_inf.kernel.partitions);
  if (part == 0) {
    return false;
  }
  std::stringstream cmd;
  // отмонтирование существующего раздела
  cmd << "ubidetach /dev/ubi_ctrl -d " << (unsigned)ubi_id;
  SendToShell(port, cmd.str(), 0);
  cmd.str(std::string());
  // форматирование блочного устройства
  cmd << "ubiformat /dev/mtd" << (unsigned)part->mtd_id;
  SendToShell(port, cmd.str(), 0);
  cmd.str(std::string());
  // подключение устройства ubi
  cmd << "ubiattach /dev/ubi_ctrl -m " << (unsigned)part->mtd_id;
  SendToShell(port, cmd.str(), 0);
  cmd.str(std::string());
  // создание раздела ubi
  const uint64_t kMaxVolSize = part->area.Length();
  const uint64_t kVolSize    = kMaxVolSize - (kMaxVolSize * 0.04);
  cmd << "ubimkvol /dev/ubi" << (unsigned)ubi_id
                             << " -N " << vol_name
                             << " -s " << (unsigned long long)kVolSize;
  std::cout << "\t создание раздела: " << vol_name << " "
                                       << (unsigned)kVolSize << " bytes" << std::endl;
  SendToShell(port, cmd.str(), 0);
  return true;
}

static bool UnpackRootFs(SerialPort &port) {
  UpdateListOfPartitions(port);
  std::cout << "Подготовка разделов" << std::endl;
  SendToShell(port, "umount /mnt/", 0);
  MkUbiFsPartition(port, 0, "rootfs");
  SendToShell(port, "mount -t ubifs ubi0:rootfs /mnt/", 0);
  SendToShell(port, "umount /mnt/home", 0);
  MkUbiFsPartition(port, 1, "storage");
  SendToShell(port, "mkdir /mnt/home", 0);  
  SendToShell(port, "mount -t ubifs ubi1:storage /mnt/home", 0);
  std::cout << "Распаковка архива с корневой файловой системой" << std::endl;
  SendToShell(port, "gunzip " + kRootFsTar, 0);
  const std::string kPathTar(kRootFsTar.substr(0, kRootFsTar.find_last_of(".")));
  SendToShell(port, "cd /mnt && tar xvf " + kPathTar, 0);
  std::stringstream mac_str;
  mac_str.fill('0');
  mac_str << std::hex << std::uppercase
          << "00:60:2F:" << std::setw(2) << (unsigned)g_sys_state.stand << ":"
                         << std::setw(2) << (unsigned)g_sys_state.batch << ":"
                         << std::setw(2) << (unsigned)g_sys_state.device;
  std::cout << "Установка MAC-адреса: " << mac_str.str() << std::endl;
  g_sys_inf.board_hw_addr = mac_str.str();
  SendToShell(port, "echo " + mac_str.str() + " > /mnt/etc/hwaddr", 0);
  SendToShell(port, "sync", 0);
  return true;
}

static bool UploadAndInstallTools(const std::string &tftpd_ip,
                                  SerialPort        &port,
                                  TFtp::Server      *srv) {
  if (srv == 0 || tftpd_ip.size() == 0) {
    return false;
  }
  static const std::string kPathGz("/tmp/" + kMtdUtils);
  std::cout << "Загрузка специальных утилит..." << std::endl;
  if (not DownloadFromTFtp(tftpd_ip, kMtdUtils, kPathGz, port, srv)) {
    return false;
  }
  std::cout << "Установка специальных утилит..." << std::endl;
  SendToShell(port, "gunzip " + kPathGz, 0);
  const std::string kPathTar(kPathGz.substr(0, kPathGz.find_last_of(".")));
  SendToShell(port, "tar xvf " + kPathTar + " -C /", 0);
  return true;
}

static bool UploadAndInstallKernel(const std::string &tftpd_ip,
                                   SerialPort        &port,
                                   TFtp::Server      *srv) {
  if (srv == 0 || tftpd_ip.size() == 0) {
    return false;
  }
  static const std::string kPath("/tmp/" + kKernelImg);
  std::cout << "Загрузка ядра Linux..." << std::endl;
  if (not DownloadFromTFtp(tftpd_ip, kKernelImg, kPath, port, srv)) {
    return false;
  }
  std::cout << "Установка ядра Linux..." << std::endl;
  UpdateListOfPartitions(port);
  Partition *part = FindPartition("kernel", &g_sys_inf.kernel.partitions);
  if (part == 0) {
    return false;
  }
  std::stringstream cmd;
  // очистка флэш памяти
  cmd << "mtd_debug erase /dev/mtd" << (unsigned)part->mtd_id
      << " 0 0x" << std::hex << part->area.Length() << std::dec;
  SendToShell(port, cmd.str(), 0);
  cmd.str(std::string());
  // запись образа
  cmd << "nandwrite -p /dev/mtd" << (unsigned)part->mtd_id
      << " " << kPath;
  SendToShell(port, cmd.str(), 0);
  return true;
}

static bool UploadAndInstallUBoot(const std::string &tftpd_ip,
                                  SerialPort        &port,
                                  TFtp::Server      *srv) {
  if (srv == 0 || tftpd_ip.size() == 0) {
    return false;
  }
  static const std::string kPath("/tmp/" + kUBootImg + ".bin");
  std::cout << "Загрузка U-Boot..." << std::endl;
  if (not DownloadFromTFtp(tftpd_ip, kUBootImg + ".bin", kPath, port, srv)) {
    return false;
  }
  std::cout << "Установка U-Boot..." << std::endl;
  UpdateListOfPartitions(port);
  Partition *part = FindPartition("bootloader", &g_sys_inf.kernel.partitions);
  if (part == 0) {
    return false;
  }
  std::stringstream cmd;
  cmd << "flashcp -v " << kPath << " /dev/mtd" << (unsigned)part->mtd_id;
  SendToShell(port, cmd.str(), 0);
  cmd.str("");
  std::cout << "Запись данных о периферии..." << std::endl;
  cmd << "uboot_conf --if=/dev/mtd1 --of=/tmp/uboot.conf "
      << "--lcd=" << g_sys_state.lcd_model << " "
      << "--ts=" << g_sys_state.ts_model << " ";
  SendToShell(port, cmd.str(), 0);
  SendToShell(port, "uboot_conf --of=/dev/mtd1 --if=/tmp/uboot.conf --do=w", 0);
  const std::string kTime = GetTime();
  std::cout << "Установка времени: " << kTime << std::endl;
  SendToShell(port, "date -s '" + kTime + "'", 0);
  SendToShell(port, "hwclock -w -f /dev/rtc0", 0);
  return true;
}

static bool UploadAndInstallPacket(const std::string &tftpd_ip,
                                   const std::string &ipk_name,
                                   SerialPort        &port,
                                   TFtp::Server      *srv) {
  const std::string kPath("/tmp/" + ipk_name);
  std::cout << "Загрузка пакета <" << ipk_name << ">..." << std::endl;
  if (not DownloadFromTFtp(tftpd_ip, ipk_name, kPath, port, srv)) {
    return false;
  }
  std::cout << "Установка пакета <" << ipk_name << ">..." << std::endl;
  SendToShell(port, "ipkg-cl install " + kPath, 0);  
  return true;
}

static bool SetupRegigrafLicense(const std::string &tftpd_ip,
                                 const std::string &path,
                                 SerialPort        &port,
                                 TFtp::Server      *srv) {
  std::cout << "Загрузка лицензии..." << std::endl;
  if (not DownloadFromTFtp(tftpd_ip, kRegigrafLicense, path + kRegigrafLicense, 
      port, srv)) {
    return false;
  }
  return true;
}

static bool UpdateListOfPackets(SerialPort &port) {
  g_sys_inf.packets.clear();
  SendToShell(port, "ipkg-cl list_installed", &g_sys_inf);
  return true;
}

static bool LoginToRegiboard(SerialPort &port) {
  static const std::string kUser("root");
  std::cout << "Авторизация пользователя: " << kUser << std::endl;
  SwitchShell(kRegigrafPrompt);
  SendToShell(port, kUser, 0);
  return true;
}

static bool UploadAndInstallRegigraf(const std::string &tftpd_ip,
                                     SerialPort        &port,
                                     TFtp::Server      *srv) {
  std::cout << "Смена корневого раздела" << std::endl;
  SendToShell(port, "mount -t ubifs ubi0:rootfs /mnt", 0);
  SendToShell(port, "mount --move /sys  /mnt/sys",  0);
  SendToShell(port, "mount --move /proc /mnt/proc", 0);
  SendToShell(port, "mount --move /dev  /mnt/dev",  0);
  SendToShell(port, "mount --move /tmp  /mnt/tmp",  0);
  // авторизация
  SendCmd(port, "exec switch_root /mnt /sbin/init");
  ParseUntil(port, kRegigrafLogin, 0);
  LoginToRegiboard(port);
  std::cout << "Установка пакетов" << std::endl;
  SendToShell(port, "mount -t ubifs ubi1:storage /home", 0);
  UploadAndInstallPacket(tftpd_ip, "++DFB.ipk",     port, srv);
  UploadAndInstallPacket(tftpd_ip, "regirfb.ipk",   port, srv);
  UploadAndInstallPacket(tftpd_ip, "regisetup.ipk", port, srv);
  UploadAndInstallPacket(tftpd_ip, kRegigrafIpk,    port, srv);
  UpdateListOfPackets(port);
  // установка лицензии
  return SetupRegigrafLicense(tftpd_ip, "/home/regigraf/", port, srv);
}

static bool UploadAndInstallFirmware(const std::string &tftpd_ip,
                                     SerialPort        &port,
                                     TFtp::Server      *srv) {
  std::cout << "Загрузка файла прошивки" << std::endl;
  const std::string kPath("/tmp/" + kRbfName);
  if (not DownloadFromTFtp(tftpd_ip, kRbfName, kPath, port, srv)) {
    return false;
  }
  std::cout << "Установка прошивки" << std::endl;
  std::stringstream cmd;
  cmd << "/home/regigraf/regiunpacker --file " << kPath 
      << " --exe 'ipkg-cl update && ipkg-cl upgrade'";
  SendToShell(port, cmd.str(), 0);
  UpdateListOfPackets(port);
  return true;
}

static bool SetupBooting(SerialPort &port) {
  std::cout << "Прошивка FUSE памяти" << std::endl;
  SendToShell(port, "rb_fuses_imx53v2_spi_flash.sh", 0);
  std::cout << "Проверка загрузки платы. Перезагрузка..." << std::endl;
  SendCmd(port, "reboot");
  ParseUntil(port, kRegigrafLogin, 0);
  std::cout << "Плата готова к работе!" << std::endl;
  return true;
}

static bool RegisterBoard(SerialPort &port) {
  LoginToRegiboard(port);
  std::cout << "Запись идентификатора платы..." << std::endl;
  std::stringstream cmd;
  cmd << "uboot_conf --if=/dev/mtd1 --of=/tmp/uboot.conf "
      << "--board=" << g_sys_state.GetDeviceId();
  SendToShell(port, cmd.str(), 0);
  SendToShell(port, "uboot_conf --of=/dev/mtd1 --if=/tmp/uboot.conf --do=w", 0);
  std::cout << "Регистрация платы" << std::endl;  
  SaveSysInfoToJson("./tmp/", g_sys_state, g_sys_inf);
  g_sys_state.device++;
  g_sys_state.CheckBatchCapacity();
  g_sys_state.Save();
  return true;
}

template <typename DataType>
static bool CompareValues(const DataType    &standart,
                          const DataType    &value,
                          const std::string &err_comment) {
  if (standart == value) {
    return true;
  }
  std::cout << UseColor(kRed)
            << "\t * Ошибка: "
            << UseColor(kReset)
            << err_comment << ": "
            << UseColor(kRed) << value << UseColor(kReset) 
            << " вместо "
            << UseColor(kGreen) << standart << UseColor(kReset)
            << std::endl;
  return false;
}

static bool ValidateHardware(SerialPort &port) {
  std::cout << "Проверка наличия периферийных устройств..." << std::endl;
  bool check_res = CompareValues<std::string>("Freescale i.MX53 family 2.1V at 800 MHz", g_sys_inf.cpu, "неверная модель процессора");
  check_res &= CompareValues<std::string>("1 GB", g_sys_inf.dram_size, "неверный объём ОЗУ");
  check_res &= CompareValues<std::string>("332800000Hz", g_sys_inf.ddr, "неверная частота ОЗУ");
  check_res &= CompareValues<std::string>("1080 KBytes", g_sys_inf.nor_size, "неверный объём ПЗУ (NOR)");
  check_res &= CompareValues<std::string>("AT45DB321x",  g_sys_inf.nor, "неверная модель ПЗУ (NOR)");
  check_res &= CompareValues<std::string>("1 GiB",  g_sys_inf.nand_size, "неверный объём ПЗУ (NAND)");
  check_res &= CompareValues<std::string>("MT29F8G08ABABA",  g_sys_inf.nand, "неверная модель ПЗУ (NAND)");
  check_res &= CompareValues<std::string>("FEC0", g_sys_inf.eth, "необнаружен контроллер Ethernet");
  check_res &= CompareValues<std::string>("ADS7843", g_sys_inf.kernel.ts, "необнаружен контроллер сенсорного экрана");
  check_res &= CompareValues<std::string>("ds3231", g_sys_inf.kernel.rtc, "необнаружены часы реального времени");
    
  check_res &= CompareValues<unsigned>(3, g_sys_inf.kernel.spi.size(), "неверное количество SPI шин");
  check_res &= CompareValues<unsigned>(5, g_sys_inf.kernel.uarts.size(), "неверное количество шин UART");
  check_res &= CompareValues<unsigned>(12, g_sys_inf.kernel.usb_uart.size(), "неверное количество интерфейсов USB-UART");
  check_res &= CompareValues<unsigned>(5, g_sys_inf.kernel.partitions.size(), "неверное количество разделов (NAND)");
  if (check_res) {
    std::cout << "\t * Все устройства обнаружены!"
              << std::endl;
  }
  return true;
}
 
static bool TestHardware(SerialPort &port) {
  std::cout << "Тестирование работоспособности периферийных устройств..." << std::endl;
  // TODO
  std::cout << "\t ..." << std::endl;
  // TODO
  return true;
}

struct Settings {
  std::string tty_dev;
  std::string imx_img;
  std::string kernel_img;
  std::string rootfs_img;
  std::string mtd_utils;
  std::string use_tftp;
  std::string acts;
  std::string packets;
  std::string config;
  std::string rbf;
  std::string uboot_pswd;
};

static bool ParseArgs(int argc, char **argv, Settings &set) {
  namespace po = boost::program_options;
  po::options_description desc("Программа загрузки плат Regiboard через COM/Ethernet порты.");
	desc.add_options()
		("help", "описание аргументов.")
		("tty",  po::value<std::string>()->default_value("/dev/" + kTtyDev),
             "путь к устройству COM.")
		("img",  po::value<std::string>()->default_value(kUBootImg),
             "путь к образу загрузчика u-boot. Указывать необходимо путь + имя "
             "файла, но без разширения! В указанном месте должны располагаться "
             "2 файла: *.bin, *.imx")
		("kernel", po::value<std::string>()->default_value(kKernelImg),
             "путь к образу ядра Linux. Ядро не будет загружено если не указать файл.")
		("rootfs", po::value<std::string>()->default_value(kRootFsImg),
             "путь к образу файловой системы. Необязательный параметр, работает "
             "только совместно с флагом: --tftp.")
    ("utils", po::value<std::string>()->default_value(kMtdUtils),
             "путь к архиву, с утилитами для записи образов в ПЗУ платы. "
             "Необязательный параметр, работатет только совместно с флагом: --tftp." )
    ("acts", po::value<std::string>()->default_value("uboot"),
             "список действий (рецепт) для выполнения над платой Ф1772.")
    ("packets", po::value<std::string>()->default_value(kIpkPath),
             "путь к *.ipk файлам, для публикации на TFTP сервере."
             "Работатет только совместно с флагом: --tftp")
    ("tftp", po::value<std::string>()->default_value("0.0.0.0"),
             "использовать TFTP, необходимо указать IP интерфейса для запуска "
             "сервера.")
    ("conf", po::value<std::string>()->default_value("./"),
             std::string("путь к конфигурационному файлу <" + kConfName + ">").c_str())
    ("rbf", po::value<std::string>()->default_value(kRbfName),
             "путь к файлу прошивки <*.rbf>. Работает только с флагом --tftp")
    ("uboot_pswd", po::value<std::string>()->default_value(std::string()),
             "пароль для входа в U-Boot")
    ("verbose", po::value<bool>()->default_value(false)->zero_tokens(),
             "вывод все хданных, полученных через последовательный порт");
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
  set.rootfs_img = vm["rootfs"].as<std::string>();
  set.use_tftp   = vm["tftp"].as<std::string>();
  set.mtd_utils  = vm["utils"].as<std::string>();
  set.acts       = vm["acts"].as<std::string>();
  set.packets    = vm["packets"].as<std::string>();
  set.config     = vm["conf"].as<std::string>();
  set.rbf        = vm["rbf"].as<std::string>();
  set.uboot_pswd = vm["uboot_pswd"].as<std::string>();
  g_sys_state.verbose = vm["verbose"].as<bool>();
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
  std::cout.fill('.');
  std::cout << "\r\033[K"
            << UseColor(kBlue)
            << msg << std::setw(num) << " "
            << UseColor(kReset)
            << std::flush;
  std::cout.fill(' ');
  num++;
  if (num > kMaxNum) {
    num = 1;
  }
}

struct RegigrafLicense {
  RegigrafLicense()
    : math_chans(48),
      math_tables(12), 
      math_tables_sz(2500),
      analog_out(48),
      analog_in(48),
      discrete_in(12),
      interface_in(48),
      relays(32),
      events(32),
      setpoints(8),
      groups(16),
      channels(48),
      ethernet(true) {}
      
  std::string GenXml() const {
    std::stringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        << "<regigraf board=\"1772\" "
        << "math_chans=\"" << (math_chans > 0 ? "TRUE" : "FALSE") << "\" "
        << "mchannels_count=\"" << (unsigned)math_chans << "\" "
        << "mtables_count=\"" << (unsigned)math_tables << "\" "
        << "mtables_max_lines=\"" << (unsigned)math_tables_sz << "\" "
        << "analogs_out_count=\"" << (unsigned)analog_out << "\" "
        << "analogs_in_count=\"" << (unsigned)analog_in << "\" "
        << "discretes_in_count=\"" << (unsigned)discrete_in << "\" "
        << "interfaces_in_count=\"" << (unsigned)interface_in << "\" "
        << "relays_count=\"" << (unsigned)relays << "\" "
        << "events_count=\"" << (unsigned)events << "\" "
        << "setpoints_count=\"" << (unsigned)setpoints << "\" "
        << "groups_count=\"" << (unsigned)groups << "\" "
        << "channels_count=\"" << (unsigned)channels << "\" "
        << "eth=\"" << (ethernet ? "TRUE" : "FALSE") << "\" "
        << "/>";
    return xml.str();
  }
      
  uint16_t math_chans;     // math_chans="TRUE", mchannels_count
  uint16_t math_tables;    // mtables_count
  uint16_t math_tables_sz; // mtables_max_lines
  uint16_t analog_out;     // analogs_out_count
  uint16_t analog_in;      // analogs_in_count
  uint16_t discrete_in;    // discretes_in_count
  uint16_t interface_in;   // interfaces_in_count
  uint16_t relays;         // relays_count
  uint16_t events;         // events_count
  uint16_t setpoints;      // setpoints_count
  uint16_t groups;         // groups_count
  uint16_t channels;       // channels_count
  bool     ethernet;       // eth  
};

struct RecipeAct {
  enum Id {
    kUnknown,
    kUBoot,
    kEnterUBoot,
    kSetupNor,
    kKernelUart,
    kKernelEth,
    kKernelDebug,
    kKernelOldSys,
    kRootFs,
    kMtdUtils,
    kInstallKernel,
    kInstallUBoot,
    kInstallRegigraf,
    kInstallFirmware,
    kUnpackRootFs,
    kSetupBooting,
    kRegisterBoard,
    kValidateHW,
    kTestHW
  };
  
  struct MetaData {
    MetaData(Id _id, const std::string &_about): id(_id), about(_about) {}
    Id          id;
    std::string about;
  };
  
  typedef std::map<std::string, MetaData>  Map;
  typedef std::pair<std::string, MetaData> Pair;

  RecipeAct() {
    actions.insert(Pair("uboot",            MetaData(kUBoot,           "загрузка UBoot")));
    actions.insert(Pair("enter_uboot",      MetaData(kEnterUBoot,      "вход в консоль UBoot")));
    actions.insert(Pair("setup_nor",        MetaData(kSetupNor,        "первичная настройка NOR памяти")));
    actions.insert(Pair("kernel_uart",      MetaData(kKernelUart,      "загрузка ядра Linux через Uart")));
    actions.insert(Pair("kernel_eth",       MetaData(kKernelEth,       "загрузка ядра Linux через Ethernet")));
    actions.insert(Pair("kernel_old_sys",   MetaData(kKernelOldSys,    "загрузка ядра Linux через Ethernet и запуск старой системы с Nand")));
    actions.insert(Pair("rootfs",           MetaData(kRootFs,          "загрузка образа rootfs")));
    actions.insert(Pair("mtd_utils",        MetaData(kMtdUtils,        "загрузка и установка служебных утилит")));
    actions.insert(Pair("install_kernel",   MetaData(kInstallKernel,   "установка ядра Linux")));
    actions.insert(Pair("install_uboot",    MetaData(kInstallUBoot,    "установка загрузчика UBoot")));
    actions.insert(Pair("install_regigraf", MetaData(kInstallRegigraf, "установка ПО regigraf")));
    actions.insert(Pair("install_firmware", MetaData(kInstallFirmware, "установка ПО из файла прошивки")));
    actions.insert(Pair("unpack_rootfs",    MetaData(kUnpackRootFs,    "подготовка разделов и распаковка образа rootfs")));
    actions.insert(Pair("setup_booting",    MetaData(kSetupBooting,    "настройка загрузки с NOR памяти")));
    actions.insert(Pair("register",         MetaData(kRegisterBoard,   "регистрация платы")));
    actions.insert(Pair("validate_hw",      MetaData(kValidateHW,      "проверка наличия периферийных устройств")));
    actions.insert(Pair("test_hw",          MetaData(kTestHW,          "тестирование периферийных устройств")));
    actions.insert(Pair("kernel_dbg",       MetaData(kKernelDebug,     "загрузка ядра, и вывод отладочных сообщений")));
  }
  
  MetaData Identify(const std::string &in) const {
    Map::const_iterator it = actions.find(in);
    if (it != actions.end()) {
      return it->second;
    }
    return MetaData(kUnknown, "неизвестное действие: " + in);
  }
  
  Map actions;
};

static bool InitUart(const Settings           &set,
                     boost::asio::serial_port *port) {
  if (not port->is_open()) {
    port->open(set.tty_dev);
  }
  std::cout << "Порт \"" << set.tty_dev << "\": "
            << (port->is_open() ? "открыт" : "закрыт") << std::endl;
  port->set_option( boost::asio::serial_port_base::baud_rate(115200) );
  port->set_option( boost::asio::serial_port_base::character_size(8) );
  port->set_option( boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one) );
  port->set_option( boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none) );
  port->set_option( boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none) );
}

static const RecipeAct kActions;

static bool DoAction(const Settings           &set,
                     const std::string        &act,
                     TFtp::Server             *srv,
                     boost::asio::serial_port *port) {
  if (srv == 0 || port == 0) {
    return false;
  }
  std::stringstream tftp_args;
  tftp_args << set.use_tftp << ":" << (unsigned)TFtp::Server::kPort;
  switch (kActions.Identify(act).id) {
    case RecipeAct::kUBoot:
      return UploadUBoot(*port, set.imx_img + ".imx");
    case RecipeAct::kEnterUBoot:
      return WaitForWelcomeFromUBoot(*port, &g_sys_inf, set.uboot_pswd);
    case RecipeAct::kSetupNor:
      if (SetupNor(*port, set.uboot_pswd)) {
        port->close();
        InitUart(set, port);
        while (not UploadUBoot(*port, set.imx_img + ".imx")) {
          std::cout << "попытка загрузиться..." << std::endl;
        }
      }
      return true;
    case RecipeAct::kKernelUart:
      if (set.kernel_img.size() > 0) {
        return UploadKernelOverUart(*port, set.kernel_img, set.uboot_pswd);
      }
      break;
    case RecipeAct::kKernelEth:
      if (set.kernel_img.size() > 0 && set.use_tftp.size() > 0) {
        return UploadKernelOverEth(*port, srv, set.uboot_pswd);
      }
      break;
    case RecipeAct::kKernelDebug:
      return UploadKernelAndDebug(*port, srv, set.uboot_pswd);
    case RecipeAct::kKernelOldSys:
      return UploadKernelWithOldSys(*port, srv, set.uboot_pswd);
    case RecipeAct::kRootFs:
      if (set.rootfs_img.size() > 0 && tftp_args.str().size() > 0) {
        return UploadRootFsOverEth(tftp_args.str(), *port, srv);
      }
      break;
    case RecipeAct::kMtdUtils:
      if (set.mtd_utils.size() > 0 && tftp_args.str().size() > 0) {
        return UploadAndInstallTools(tftp_args.str(), *port, srv);
      }
    case RecipeAct::kInstallKernel:
      if (set.mtd_utils.size() > 0 && tftp_args.str().size() > 0) {
        return UploadAndInstallKernel(tftp_args.str(), *port, srv);
      }
    case RecipeAct::kInstallUBoot:
      if (set.mtd_utils.size() > 0 && tftp_args.str().size() > 0) {
        return UploadAndInstallUBoot(tftp_args.str(), *port, srv);
      }
    case RecipeAct::kInstallRegigraf:
      if (tftp_args.str().size() > 0) {
        return UploadAndInstallRegigraf(tftp_args.str(), *port, srv);
      }
    case RecipeAct::kInstallFirmware:
      if (tftp_args.str().size() > 0) {
        return UploadAndInstallFirmware(tftp_args.str(), *port, srv);
      }      
    case RecipeAct::kUnpackRootFs:
      if (set.rootfs_img.size() > 0) {
        return UnpackRootFs(*port);
      }
      break;
    case RecipeAct::kSetupBooting:
      return SetupBooting(*port);
    case RecipeAct::kRegisterBoard:
      return RegisterBoard(*port);
    case RecipeAct::kValidateHW:
      return ValidateHardware(*port);
    case RecipeAct::kTestHW:
      return TestHardware(*port);
    case RecipeAct::kUnknown:
    default:
      break;
  }
  return false;
}

static void PublishDir(const std::string &dir, TFtp::Server *srv) {
  using namespace boost::filesystem;
  path p(dir);
  if (not is_directory(p) || srv == 0) {
    return;
  }
  directory_iterator dir_it(p);
  for (; dir_it != directory_iterator(); dir_it++) {
    const directory_entry &entry = *dir_it;
    if (entry.path().extension() != ".ipk") {
      continue;
    }
    if (Parse(entry.path().stem().string(), "regigraf(.*)", 0, 0)) {
      srv->PublishFile(kRegigrafIpk, entry.path().string());
      continue;
    }
    srv->PublishFile(entry.path().filename().string(), entry.path().string());
  }
}

static void PublishRegigrafLicense(const RegigrafLicense &license,
                                   TFtp::Server          *srv) {
  if (srv == 0) {
    return;
  }
  static const std::string kPath("/tmp/regigraf_license.xml");
  std::fstream out(kPath, std::fstream::out | std::fstream::trunc);
  out << license.GenXml();
  out.close();
  srv->PublishFile(kRegigrafLicense, kPath);
}

static bool ExecuteRecipe(const Recipe             &recipe,
                          const Settings           &set,
                          TFtp::Server             *srv,
                          boost::asio::serial_port *port) {
  if (srv == 0 || port == 0) {
    return false;
  }
  PrintDelimiter("\n", "Для выполнения рецепта", 80);
  std::cout << UseColor(kGreen)
            << "\t 1. Подайте напряжение на плату\n"
            << "\t 2. Нажмите [Enter] для начала загрузки..." 
            << UseColor(kReset)
            << std::endl;
  std::cin.get();
  // вывод конфигурации
  g_sys_state.Print();
  // приготовление рецепта
  Recipe::const_iterator act_it = recipe.begin();
  bool recipe_fail = false;
  for (; act_it != recipe.end(); act_it++) {
    if (not DoAction(set, *act_it, srv, port)) {
      std::cout << UseColor(kRed)
                << "Невозможно продолжить исполнение рецепта!"
                << UseColor(kReset)
                << std::endl;
      recipe_fail = true;
      break;
    }
  }
  // вывод общей информации
  if (not recipe_fail) {
    PrintSysInfo(g_sys_inf);
  }
  std::cout << UseColor(kGreen)
            << "Продолжить работу с данным рецептом? (Y/n) + [Enter]: "
            << UseColor(kReset)
            << std::flush;
  char answer = 0;
  std::cin.get(answer);
  // если ответ - нет
  if (answer != 10 && answer != 'y' && answer != 'Y') {
    return false; 
  }
  return true;
}

static void KernelSignalCatcher(int sig_num) {
  switch (sig_num) {
    case SIGSEGV: {
      //debug::Backtrace bt;
      //bt.KeepStack();
      //bt.PrintStackTo(&std::cout);
      break;
    }
    default:
      std::cout << "Получен неизвестный сигнал: " << sig_num << std::endl;
      break;
  };
}

int main(int argc, char **argv) {
  Settings set;
  signal(SIGSEGV, KernelSignalCatcher);
  if (not ParseArgs(argc, argv, set)) {
    return 0;
  }
  // чтение конфигурации
  if (not g_sys_state.LoadFrom(set.config + "/" + kConfName)) {
    std::cout << "Неудалось прочитать конфигурацию из <" << kConfName << ">!" << std::endl;
    return 0;
  }
  // подготовка лицензии
  RegigrafLicense license;
  // разбор рецепта
  Recipe recipe;
  const std::string kDelimiter("->");
  std::size_t found = 0;
  PrintDelimiter("", "Разбор рецепта", 80);
  do {
    const std::size_t kFromOff = found;
    found = set.acts.find(kDelimiter, found);
    const std::string kAction(set.acts.substr(kFromOff, (found - kFromOff)));
    std::cout << "\t - " << kActions.Identify(kAction).about << std::endl;
    recipe.push_back(kAction);
    if (found == std::string::npos) {
      break;
    }
    found += 2;
  } while (1);
  // инициализация сервера TFTP
  TFtp::Server srv(set.use_tftp);
  srv.PublishFile(kUBootImg + ".bin", set.imx_img + ".bin");
  srv.PublishFile(kKernelImg, set.kernel_img);
  srv.PublishFile(kRootFsImg, set.rootfs_img);
  srv.PublishFile(kMtdUtils,  set.mtd_utils);
  srv.PublishFile(kRbfName,   set.rbf);
  PublishDir(set.packets, &srv);
  PublishRegigrafLicense(license, &srv);
  // инициализация COM порта
  boost::asio::io_service io_service;
  boost::asio::serial_port port(io_service, set.tty_dev);
  // приготовление рецепта
  while (ExecuteRecipe(recipe, set, &srv, &port)) {
    g_sys_inf = SysInfo();
    PrintDelimiter("\n", "Подготовка к работе со следующей платой", 80);
    std::cout << UseColor(kGreen)
              << "\t 1. Отключите питание\n"
              << "\t 2. Замените процессорную плату\n"
              << "\t 3. Установите перемычку (джампер) \"on/off\"\n"
              << "\t 4. Подключите провод к разъёму \"debug_uart\""
              << UseColor(kReset)
              << std::endl;
    port.close();
    InitUart(set, &port);
  }
  port.close();
  return 0;
}
