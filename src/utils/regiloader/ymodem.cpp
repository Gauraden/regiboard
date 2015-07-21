#include "ymodem.hpp"
#include "regiloader.hpp"
#include <cstring>
#include <iomanip>
#include <fstream>

// class YModem::Crc
YModem::Crc::Crc(YModem::Crc::InitialValue val): _ini_value(0) {
  _ini_value = (Value)val;
  Value temp, a;
  for (size_t i = 0; i < kTableSz; i++) {
    temp = 0;
    a    = (Value)(i << 8);
    for (size_t j = 0; j < 8; j++) {
      if (((temp ^ a) & 0x8000) != 0) {
        temp = (Value)((temp << 1) ^ kPoly);
      } else {
        temp <<= 1;
      }
      a <<= 1;
    }
    _table[i] = temp;
  }
}

YModem::Crc::~Crc() {
}

YModem::Crc::Value YModem::Crc::Compute(uint8_t *data, size_t sz) const {
  Value crc = _ini_value;
  for (size_t i = 0; i < sz; i++) {
    crc = (Value)((crc << 8) ^ _table[((crc >> 8) ^ (0xff & data[i]))]);
  }
  return (crc >> 8) | ((crc & 0xFF) << 8);
}
// class YModem::Packet
YModem::Packet::Packet(YModem::Packet::Signal type)
    : _type(type), _data(0), _data_sz(0) {
}

YModem::Packet::Packet(YModem::Packet::Signal type, uint8_t *data, size_t sz)
    : _type(type), _data(data), _data_sz(sz) {
}

YModem::Packet::~Packet() {
}

bool YModem::Packet::SendArray(SPort &port, uint8_t *arr, size_t sz) const {
  return (boost::asio::write(port, boost::asio::buffer(arr, sz)) == sz);
}

bool YModem::Packet::Send(SPort &port, Id pkt_num) const {
  bool res = SendScalar(port, (uint8_t)_type);
  if (_data == 0 || _type != kSTX)
    return res;
  Crc crc(Crc::kZeros);
  const Crc::Value kCrcVal    = crc.Compute(_data, _data_sz);
  const Id         kInvPktNum = 255 - pkt_num;
  res = res && SendScalar(port, (uint8_t)pkt_num);
  res = res && SendScalar(port, (uint8_t)kInvPktNum);
  res = res && SendArray (port, _data, _data_sz);
  res = res && SendScalar(port, kCrcVal);
  return res;
}
// class YModem::Packet::Sequence
YModem::Packet::Sequence::Sequence(SPort *port): _pkt_num(0), _port(port) {
}

YModem::Packet::Sequence::~Sequence() {
}

bool YModem::Packet::Sequence::Send(const YModem::Packet &pkt) {
  if (_port == 0)
    return false;
  pkt.Send(*_port, _pkt_num);
  if (_pkt_num == 255)
    _pkt_num = 0;
  else
    _pkt_num++;
  return true;
}

YModem::SPort* YModem::Packet::Sequence::get_port() {
  return _port;
}

void YModem::Packet::Sequence::Reset() {
  _pkt_num = 0;
}
// class YModem
YModem::YModem() {
}

YModem::~YModem() {
}

bool YModem::IsResponse(SPort &port, const char expect) const {
  if (not port.is_open())
    return false;
  static const size_t kRespSz = 1;
  char resp[kRespSz];
  if (boost::asio::read(port, boost::asio::buffer(resp, kRespSz)) != kRespSz)
    return false;
  if (expect != (unsigned)resp[0]) {
    std::cout << "\t ! Неожиданный ответ: " << (unsigned)resp[0] << " вместо "
                                            << (unsigned)expect << std::endl;
  }
  return (expect == (unsigned)resp[0]);
}

bool YModem::WaitForResponse(SPort &port, const char expect) const {
  for (uint8_t tries = 0; tries < 255; tries++) {
    if (IsResponse(port, expect))
      return true;
  }
  return false;
}

YModem::Array YModem::NewFrame() const {
  Array frame(new uint8_t[kFrameSize]);
  memset(frame.get(), 0, kFrameSize);
  return frame;
}

bool YModem::SendInitialPkt(Packet::Sequence &seq, const std::string *msg, size_t amount) {
  if (msg == 0 || amount == 0)
    return false;
  Array frame(NewFrame());
  size_t total = 0;
  for (size_t msg_i = 0; msg_i < amount; msg_i++) {
    size_t i = 0;
    const std::string &kMsg = msg[msg_i];
    for (; i < kMsg.size(); i++) {
      frame.get()[i] = kMsg[i];
    }
    total += i;
    frame.get()[total] = 0;
    total++;
  }  
  return (seq.Send(Packet(Packet::kSTX, frame.get(), kFrameSize)) &&
          IsResponse(*seq.get_port(), Packet::kACK));
}

bool YModem::SendClosingPkt(Packet::Sequence &seq) {
  seq.Reset();
  Array frame(NewFrame());
  return (seq.Send(Packet(Packet::kSTX, frame.get(), kFrameSize)) && 
          IsResponse(*seq.get_port(), Packet::kACK));
}

bool YModem::SendFile(const std::string &path, YModem::SPort &out) {
  if (path.size() == 0 ||
      not out.is_open())
    return false;
  std::fstream firm_str(path.c_str());
  if (not firm_str.is_open())
    return false;
  firm_str.seekg(0, firm_str.end);
  const size_t kFileSize = firm_str.tellg();
  firm_str.seekg(0, firm_str.beg);
  // ожидание запроса
  WaitForResponse(out, Packet::kRequest);
  Packet::Sequence seq(&out);
  // инициализация соединения
  char buf[24];
  snprintf(buf, 24, "%u", (unsigned)kFileSize);
  const std::string kMsgs[] = {path, buf};
  SendInitialPkt(seq, kMsgs, 2);
  if (not IsResponse(out, Packet::kRequest)) {
    firm_str.close();
    return false;
  }
  // передача файла
  Array frame(NewFrame());
  size_t was_send     = 0;
  size_t last_percent = 0;
  do {
    firm_str.sync();
    firm_str.read((char*)frame.get(), kFrameSize);
    const size_t kWasRead = firm_str.gcount();
    if (kWasRead != kFrameSize) {
      memset(frame.get() + kWasRead, 0, kFrameSize - kWasRead);
    }
    seq.Send(Packet(Packet::kSTX, frame.get(), kFrameSize));
    was_send += kWasRead;
    if (not IsResponse(out, Packet::kACK)) {
      std::cout << "\t ! Ошибка передачи данных" << std::endl;
      break;
    }
    const size_t kPercent = was_send * 100 / kFileSize;
    if (kPercent > last_percent) {
      ShowPercentage("\t * Прогресс", kPercent);
      last_percent = kPercent;
    }
  } while (was_send < kFileSize);
  std::cout << std::endl;
  seq.Send(Packet(Packet::kEOT));
  IsResponse(out, Packet::kACK);
  seq.Send(Packet(Packet::kEOT));
  IsResponse(out, Packet::kACK);
  IsResponse(out, Packet::kRequest);
  SendClosingPkt(seq);
  firm_str.close();
  return true;
}
