#include "sdp.hpp"
#include <iomanip>
#include <fstream>
#include <boost/asio/basic_deadline_timer.hpp>
#include <boost/bind.hpp>

SdpPacket::SdpPacket(uint8_t cmd, size_t response_sz, size_t request_sz)
    : _resp_size(response_sz), _req_size(request_sz) {
  _cmd_id.value = (uint16_t)((cmd << 8) | cmd);
  if (_resp_size > kPktSize)
    _resp_size = kPktSize;
}

SdpPacket::~SdpPacket() {
}

void SdpPacket::Print(const std::string &pref, std::ostream &out) {
  out << pref << ": " << std::uppercase << std::setfill('0');
  for (size_t i = 0; i < kPktSize; i++) {
    out << std::hex << std::setw(2) << (unsigned)_packet[i] << "|";
  }
  out << std::endl;
}

void SdpPacket::HandlerRead(const boost::system::error_code &error,
                            std::size_t                      bytes_transferred) {
//  Print("Прочитан ответ", std::cout); // for DEBUG
  _was_read = (bytes_transferred == _resp_size);
}

void SdpPacket::HandlerTimeout(const boost::system::error_code &error) {
  _was_read = false;
  _timeout  = true;
}

bool SdpPacket::Send(boost::asio::serial_port &port) {
  memset(_packet, 0, kPktSize);
  if (not Write()) {
    return false;
  }
  using namespace std::placeholders;
  const unsigned kTimeOutSec       = 1;
  const unsigned kMaxAmountOfTries = 3;
  // ожидание данных с тайм-аутом
  boost::asio::deadline_timer timer(port.get_io_service());
  unsigned try_num = 1;
  for (; try_num <= kMaxAmountOfTries; try_num++) {
    std::cout << "\r\033[K"
              << "Попытка отправки команды " << try_num << " ...";
//    Print("Отправка запроса", std::cout); // for DEBUG
    memset(_packet, 0, kPktSize);
    AddArr(0, _cmd_id.bytes, 2);
    boost::asio::write(port, boost::asio::buffer(_packet, _req_size));
    timer.expires_from_now(boost::posix_time::seconds(kTimeOutSec));
    _was_read = false;
    _timeout  = false;
    timer.async_wait(boost::bind(&SdpPacket::HandlerTimeout,
        this,
        boost::asio::placeholders::error));
    memset(_packet, 0, kPktSize);
    boost::asio::async_read(port, boost::asio::buffer(_packet, _resp_size),
      boost::bind(&SdpPacket::HandlerRead,
        this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
    do {
      port.get_io_service().poll_one();
    } while (not _was_read && not _timeout);
    if (_was_read) {
      break;
    }
    port.cancel();
  }
  std::cout << "\r\033[K";
  if (try_num > kMaxAmountOfTries) {
    return false;  
  }
  const Transfer kAct = Read();
  if (kAct == kContinue) {
    return Write(port);
  }
  return (kAct == kStop);
}

std::string SdpPacket::GetAckName(FieldU32 val) {
  if (val.value == kClosedSecurityConf)
    return "closed security configuration";
  if (val.value == kOtherwise)
    return "otherwise";
  return "???";
}

void SdpPacket::AddVal(uint8_t offs, FieldU32 &val) {
  AddArr(offs, val.bytes, 4);
}

void SdpPacket::AddMsbVal(uint8_t offs, FieldU32 &val) {
  AddArr(offs, val.bytes, 4, true);
}
    
void SdpPacket::AddVal(uint8_t offs, uint8_t val) {
  AddArr(offs, &val, 1);
}

void SdpPacket::GetVal(uint8_t offs, FieldU32 &val) {
  GetArr(offs, val.bytes, 4);
}

void SdpPacket::GetMsbVal(uint8_t offs, FieldU32 &val) {
  GetArr(offs, val.bytes, 4, true);
}

void SdpPacket::GetVal(uint8_t offs, uint8_t &val) {
  GetArr(offs, &val, 1);
}

void SdpPacket::AddArr(uint8_t offs, uint8_t *arr, size_t sz, bool msb) {
  if (arr == 0 || sz == 0)
    return;
  offs += (msb ? sz - 1: 0);
  if (offs >= _req_size)
    return;
  for (uint8_t src = 0; src < sz; src++) {
    uint8_t dst = offs;
    if (msb)
      dst -= src;
    else
      dst += src;
    _packet[dst] = arr[src];
  }
}

void SdpPacket::GetArr(uint8_t offs, uint8_t *arr, size_t sz, bool msb) {
  if (arr == 0 || sz == 0)
    return;
  const uint8_t kDst = offs + (msb ? sz - 1: 0);
  if (kDst >= _req_size)
    return;
  for (uint8_t src = 0; src < sz; src++) {
    if (msb)
      arr[src] = _packet[kDst - src];
    else
      arr[src] = _packet[kDst + src];
  }
}
// class PktStatus
PktStatus::PktStatus(): SdpPacket(0x05, 4) {
}

PktStatus::~PktStatus() {
}

bool PktStatus::Write(boost::asio::serial_port&) {
  return true;
}

bool PktStatus::Write() {
  return true;
}

SdpPacket::Transfer PktStatus::Read() {
  static FieldU32 resp;
  GetVal(0, resp);
  return (resp.value == 0xF0F0F0F0 ? kStop : kError);
}
// class PktComplete
PktComplete::PktComplete(): SdpPacket(0x00, 4) {
}

PktComplete::~PktComplete() {
}

bool PktComplete::Write(boost::asio::serial_port&) {
  return true;
}

bool PktComplete::Write() {
  return true;
}

SdpPacket::Transfer PktComplete::Read() {
  static FieldU32 resp;
  GetVal(0, resp);
  std::cout << "PktComplete: 0x" << std::hex << (unsigned)resp.value
                                 << std::dec << std::endl;
  return (resp.value == 0x88888888 ? kStop : kError);
}
// class PktWriteMem
size_t PktWriteMem::GetSizeFor(DataSize sz_id) {
  switch (sz_id) {
    case kByte:
      return 1;
    case kHalfWord:
      return 2;
    case kWord:
      return 4;
    default:
      break;
  };
  return 4;
}

PktWriteMem::PktWriteMem(uint32_t addr, uint32_t data)
    : SdpPacket(0x02, 8),
      _addr(addr),
      _data(data) {
}

PktWriteMem::~PktWriteMem() {
}

bool PktWriteMem::Write(boost::asio::serial_port&) {
  return true;
}

bool PktWriteMem::Write() {
  FieldU32 addr;
  FieldU32 data;
  addr.value = _addr;
  data.value = _data;
  AddVal(2,  addr);
  AddVal(6,  (uint8_t)kWord);
  AddVal(11, data);
  return true;
}

SdpPacket::Transfer PktWriteMem::Read() {
  static FieldU32 ack;
  static FieldU32 res;
  GetVal(0, ack);
  GetVal(4, res);
  return (ack.value == kOtherwise && res.value == 0x128A8A12 ? kStop : kError);
}
// class PktReadMem
PktReadMem::PktReadMem(uint32_t addr, DataSize size)
    : SdpPacket(0x01, 4 + PktWriteMem::GetSizeFor(size)),
      _addr(addr),
      _size(size),
      _value(0) {
}

PktReadMem::~PktReadMem() {
}

uint32_t PktReadMem::GetValue() const {
  return _value;
}

bool PktReadMem::Write(boost::asio::serial_port&) {
  return true;
}

bool PktReadMem::Write() {
  FieldU32 addr;
  FieldU32 amount;
  addr.value   = _addr;
  amount.value = 1;
  AddVal   (2, addr);
  AddVal   (6, (uint8_t)_size);
  AddMsbVal(7, amount);
  return true;
}

SdpPacket::Transfer PktReadMem::Read() {
  FieldU32 ack;
  FieldU32 data;
  GetVal   (0, ack);
  GetMsbVal(4, data);
  _value = data.value;
  return (ack.value == kOtherwise ? kStop : kError);
}
// class PktWriteF
PktWriteF::PktWriteF(ImxFirmware *firm): SdpPacket(0x04, 4), _firm(firm) {
}

PktWriteF::~PktWriteF() {
}

std::string PktWriteF::GetFileTypeName(FileType ft) {
  switch (ft) {
    case kApplication:
      return "application";
    case kCSF:
      return "CSF (Command Sequence File)";
    case kDCD:
      return "DCD (Device Configuration Data)";
    default:
      break;
  };
  return "???";
}

bool PktWriteF::Write() {
  if (_firm == 0 || _firm->GetBootData() == 0)
    return false;
  const ImxFirmware::BootData *boot = _firm->GetBootData();
  FieldU32 target_addr;
  FieldU32 num_of_bytes;
  target_addr.value  = _firm->GetIvtData()->self_ptr;
  num_of_bytes.value = _firm->GetSize();
  std::cout << "Команда загрузки: \n"
            << std::dec
            << "\t * number of bytes: " << (unsigned)num_of_bytes.value << " байт\n"
            << std::hex
            << "\t * target address : 0x" << (unsigned)target_addr.value << "\n"
            << "\t * file type      : " << GetFileTypeName(kApplication)
            << std::endl;
  AddMsbVal(2,  target_addr);
  AddMsbVal(7,  num_of_bytes);
  AddVal   (15, (uint8_t)kApplication);
  return true;
}

SdpPacket::Transfer PktWriteF::Read() {
  FieldU32 ack;
  GetVal(0, ack);
  std::cout << "\t * Режим загрузки : " << GetAckName(ack) << std::endl;
  return (ack.value == kOtherwise ? kContinue : kError);
}

bool PktWriteF::Write(boost::asio::serial_port &port) {
  if (_firm == 0 || _firm->GetData() == 0)
    return false;
  const size_t kWasWrote = boost::asio::write(port, boost::asio::buffer(_firm->GetData(), _firm->GetSize()));
  std::cout << std::dec
            << "\t * Записано       : " << (unsigned)kWasWrote << " байт\n"
            << std::endl;
  return (kWasWrote == _firm->GetSize());
}
// struct ImxFirmware::DCDPair
ImxFirmware::DCDPair::DCDPair(): addr(0), val_mask(0) {
}
// struct ImxFirmware::DCDCmd
ImxFirmware::DCDCmd::DCDCmd(): tag(0), length(0), parameter(0) {
}
// struct ImxFirmware::DCD
ImxFirmware::DCD::DCD(): tag(0), length(0), version(0) {
}
// class ImxFirmware
ImxFirmware::ImxFirmware(): _exec(0), _header(0), _boot(0) {
}

ImxFirmware::~ImxFirmware() {
}

bool ImxFirmware::LoadFromFile(const std::string &path) {
  std::fstream firm_str(path.c_str());
  if (not firm_str.is_open())
    return false;
  firm_str.seekg(0, firm_str.end);
  const size_t kFirmSize = firm_str.tellg();
  firm_str.seekg(0, firm_str.beg);
  _fptr.reset(new uint8_t[kFirmSize]);
  firm_str.read((char*)_fptr.get(), kFirmSize);
  firm_str.close();
  _fsize = kFirmSize;
  _exec  = _fptr.get();
  return InitHeader();
}

void ImxFirmware::PrintStructures(std::ostream &out) const {
  // разбор заголовка
  if (_header == 0)
    return;
  out << "IVT: \n"
      << std::hex
      << "\t * start_addr  : 0x" << (unsigned)_header->start_addr    << " -> "
                                 << (unsigned)(_header->start_addr - _header->self_ptr) << "\n"
      << "\t * dcd         : 0x" << (unsigned)_header->dcd_ptr       << " -> "
                                 << (unsigned)(_header->dcd_ptr - _header->self_ptr) << "\n"
      << "\t * boot_data   : 0x" << (unsigned)_header->boot_data_ptr << " -> "
                                 << (unsigned)(_header->boot_data_ptr - _header->self_ptr) << "\n"
      << "\t * self        : 0x" << (unsigned)_header->self_ptr      << " -> "
                                 << (unsigned)(_header->self_ptr - _header->self_ptr) << "\n"
      << "\t * app_code_csf: 0x" << (unsigned)_header->app_code_csf  << "\n"
      << std::endl;
  out << "DCD:\n"
      << std::dec
      << "\t * length      : " << (unsigned)_dcd.length << " байт\n"
      << std::endl;
  if (_boot == 0)
    return;
  out << "Boot data:\n"
      << std::dec
      << "\t * image_len   : " << (unsigned)_boot->image_len << " байт\n"
      << std::hex
      << "\t * dest        : 0x" << (unsigned)_boot->dest      << "\n"
      << "\t * plugin      : 0x" << (unsigned)_boot->plugin    << "\n"
      << std::endl;
}

const ImxFirmware::DCDCmd::List& ImxFirmware::GetDCDCommands() const {
  return _dcd.cmd_list;
}

size_t ImxFirmware::GetSize() const {
  return _fsize;
}

const uint8_t* ImxFirmware::GetData() const {
  return _exec;
}

const ImxFirmware::BootData* ImxFirmware::GetBootData() const {
  return _boot;
}

const ImxFirmware::Imx53IVT* ImxFirmware::GetIvtData() const {
  return _header;
}

uint8_t* ImxFirmware::InitDCDListOfCommandsPairs(uint8_t *list_ptr, DCDCmd &cmd) {
  const size_t kAmount = (cmd.length - 4) / 8;
  DCDPair *pair = reinterpret_cast<DCDPair*>(list_ptr);
  for (size_t i = 0; i < kAmount; i++) {
    if (pair->addr == 0 && pair->val_mask == 0)
      break;
    cmd.pairs.push_back(*pair);
    pair++;
  }
  return reinterpret_cast<uint8_t*>(pair);
}

bool ImxFirmware::InitDCDListOfCommands(uint8_t *list_ptr) {
  if (_dcd.length == 4)
    return false;
  const uint8_t *kListEnd = list_ptr + (_dcd.length - 4);
  while (list_ptr < kListEnd) {
    DCDCmd *t_cmd = reinterpret_cast<DCDCmd*>(list_ptr);
    DCDCmd cmd;
    cmd.tag       = t_cmd->tag;
    cmd.length    = t_cmd->length;
    cmd.parameter = t_cmd->parameter;
    if (cmd.tag != DCDCmd::kTag)
      break;
    list_ptr = InitDCDListOfCommandsPairs(list_ptr + 4, cmd);
    _dcd.cmd_list.push_back(cmd);
  }
}

bool ImxFirmware::InitHeader() {
  if (_exec == 0)
    return false;
 	static const unsigned kHeaderMax = 0x800;
	static const unsigned kHeaderInc = 0x400;
	// инициализация структуры IVT
	unsigned hd_offs   = 0;
	unsigned exec_addr = 0;
  for (; hd_offs <= kHeaderMax; hd_offs += kHeaderInc) {
    Imx53IVT *t_hd = reinterpret_cast<Imx53IVT*>(_exec + hd_offs);
    if (t_hd->barker != kImx53Barker)
      continue;
    exec_addr += (t_hd->start_addr - t_hd->self_ptr);
    // инициализация структуры "BootData"
    _boot = reinterpret_cast<BootData*>(_exec + (t_hd->boot_data_ptr -
                                                 t_hd->self_ptr));
    // инициализация структуры "DCD"
    if (t_hd->dcd_ptr != 0) {
      uint8_t *dcd_ptr = _exec + (t_hd->dcd_ptr - t_hd->self_ptr);
      DCD *t_dcd = reinterpret_cast<DCD*>(dcd_ptr);
      _dcd.tag     = t_dcd->tag;
      _dcd.length  = t_dcd->length;
      _dcd.version = t_dcd->version;
      if (_dcd.tag != DCD::kTag)
        continue;
      InitDCDListOfCommands(dcd_ptr + 4);
      _header = t_hd;
    }
  }
}
