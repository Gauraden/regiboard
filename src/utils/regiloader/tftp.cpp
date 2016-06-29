#include "tftp.hpp"
#include "regiloader.hpp"
#include <cstring>

// class Buffer
TFtp::Packet::Buffer::Buffer(): _size(0) {
  Clear();
}

TFtp::Packet::Buffer::~Buffer() {
}

bool TFtp::Packet::Buffer::set_data(const Byte *bytes, size_t size) {
  if (bytes == 0 || size == 0) {
    return false;
  }
  _size = (size < kMaxSize ? size : kMaxSize);
  memcpy(_data, bytes, _size);
  return true;
}

const TFtp::Byte* TFtp::Packet::Buffer::get_data() const {
  return _data;
}

size_t TFtp::Packet::Buffer::get_size() const {
  return _size;
}

bool TFtp::Packet::Buffer::AddArray(const Byte *array, size_t size) {
  if (array == 0 || size == 0) {
    return false;
  }
  const size_t kResSize = _size + size;
  for (; _size < kResSize && _size < kMaxSize; _size++) {
    _data[_size] = *array;
    array++;
  }
  return (kResSize <= kMaxSize);
}

bool TFtp::Packet::Buffer::GetArray(size_t offs, Byte *array, size_t size) const {
  if (array == 0 || size == 0 || offs >= kMaxSize) {
    return false;
  }
  const size_t kResSize = offs + size;
  for (auto i = offs; i < kResSize && i < kMaxSize; i++) {
    (*array) = _data[i];
    array++;
  }
  return (kResSize <= kMaxSize);
}

void TFtp::Packet::Buffer::Clear() {
  _size = 0;
  memset(_data, 0, kMaxSize);
}
// class Packet
TFtp::Packet::Packet(Opcode opc): _opcode(opc) {
}

TFtp::Packet::~Packet() {
}

void TFtp::Packet::LSB2MSB(Byte *bytes) const {
  if (bytes == 0) {
    return;
  }
  Byte t_byte = bytes[0];
  bytes[0] = bytes[1];
  bytes[1] = t_byte;  
}

template<>
bool TFtp::Packet::AddVal(const uint16_t &val) {
  FieldU16 t_val;
  t_val.value = val;
  LSB2MSB(t_val.bytes);
  return _buff.AddArray(t_val.bytes, sizeof(t_val.value));
}

template<>
bool TFtp::Packet::AddVal(const Byte &val) {
  return _buff.AddArray((const Byte*)&val, sizeof(val));
}

template<>
bool TFtp::Packet::AddVal(const std::string &val) {
  static const Byte kZero = 0;
  const size_t kSize = val.size();
  const bool   kRes  = _buff.AddArray((const Byte*)val.c_str(), kSize);
  return (kRes && AddVal(kZero));
}

template<>
size_t TFtp::Packet::GetVal(size_t offs, uint16_t *val) {
  if (offs < 0) {
    return -1;
  }
  FieldU16 t_val;
  static const size_t kValSize = sizeof(t_val.value);
  const size_t kOff = (_buff.GetArray(offs, t_val.bytes, kValSize) ? offs + kValSize: -1);
  LSB2MSB(t_val.bytes);
  (*val) = t_val.value;
  return kOff;
}

template<>
size_t TFtp::Packet::GetVal(size_t offs, Byte *val) {
  if (offs < 0) {
    return -1;
  }
  static const size_t kValSize = sizeof(Byte);
  return (_buff.GetArray(offs, (Byte*)val, kValSize) ? offs + kValSize: -1);
}

template<>
size_t TFtp::Packet::GetVal(size_t offs, std::string *val) {
  if (val == 0 || offs < 0 || offs >= kMaxSize) {
    return -1;
  }
  const Byte *data = _buff.get_data();
  size_t      i    = offs;
  while (data[i] != 0 && i < kMaxSize) {
    i++;
  }
  val->assign((const char*)&data[offs], i - offs);
  return (data[i] == 0 ? i + 1 : i);
}

bool TFtp::Packet::Read() {
  return true;
}

bool TFtp::Packet::Write() {
  Clear();
  AddVal((uint16_t)_opcode);
  return true;
}

void TFtp::Packet::Clear() {
  _buff.Clear();
}

bool TFtp::Packet::StartTransaction(Session *sess) {
  if (sess == 0 || _buff.get_size() == 0) {
    return false;
  }
  return sess->Send(_buff.get_data(), _buff.get_size());
}

TFtp::Packet::OpcodeId TFtp::Packet::get_opcode() const {
  return _opcode;
}
// class PacketRRQ
TFtp::PacketRRQ::PacketRRQ(): Packet(kRRQ) {
}

TFtp::PacketRRQ::PacketRRQ(const std::string &filename, Mode mode)
    : Packet(kRRQ),
      _filename(filename),
      _mode(mode) {
}

TFtp::PacketRRQ::~PacketRRQ() {
}

bool TFtp::PacketRRQ::Read() {
  std::string mode;
  size_t offs = 0;
  offs = GetVal(offs, &_filename);
  offs = GetVal(offs, &mode);
  if (mode == "netascii") {
    _mode = kNetASCII;
    return true;
  }
  if (mode == "octet") {
    _mode = kOctet;
    return true;
  }
  if (mode == "mail") {
    _mode = kMail;
    return true;
  }
  return false;
}

bool TFtp::PacketRRQ::Write() {
  Packet::Write();
}

std::string TFtp::PacketRRQ::get_filename() const {
  return _filename;
}

TFtp::PacketRRQ::Mode TFtp::PacketRRQ::get_mode() const {
  return _mode;
}
// class PacketData
TFtp::PacketData::PacketData(): Packet(kData) {
}

TFtp::PacketData::PacketData(BlockId id, Byte *data, size_t size)
    : Packet(kData),
      _block_id(id) {
  Packet::Write();
  AddVal((uint16_t)_block_id);
  _buff.AddArray(data, size);
}

TFtp::PacketData::~PacketData() {
}

bool TFtp::PacketData::Read() {
  return true;
}

bool TFtp::PacketData::Write() {
  return true;
}

TFtp::Packet::BlockId TFtp::PacketData::get_block_id() const {
  return _block_id;
}

const TFtp::Byte* TFtp::PacketData::get_data() const {
  return _buff.get_data();
}

size_t TFtp::PacketData::get_size() const {
  return _buff.get_size();
}

bool TFtp::PacketData::IsLastBlock() const {
  return (_buff.get_size() < 512);
}
// class PacketACK
TFtp::PacketACK::PacketACK(): Packet(kACK) {
}

TFtp::PacketACK::PacketACK(BlockId block_id)
    : Packet(kACK),
      _block_id(block_id) {
}

TFtp::PacketACK::~PacketACK() {
}

bool TFtp::PacketACK::Read() {
  size_t offs = 0;
  offs = GetVal(offs, (uint16_t*)&_block_id);
  return true;
}

bool TFtp::PacketACK::Write() {
  return true;
}

TFtp::Packet::BlockId TFtp::PacketACK::get_block_id() const {
  return _block_id;
}
// class PacketError
TFtp::PacketError::PacketError(): Packet(kError) {
}

TFtp::PacketError::PacketError(ErrorCode code, const std::string &msg)
    : Packet(kError), _code(code), _msg(msg) {
}

TFtp::PacketError::~PacketError() {
}

bool TFtp::PacketError::Read() {
  size_t offs = 0;
  offs = GetVal(offs, &_code);
  offs = GetVal(offs, &_msg);
  return true;
}

bool TFtp::PacketError::Write() {
  return true;
}

TFtp::Packet::ErrorCode TFtp::PacketError::get_code() const {
  return _code;
}

std::string TFtp::PacketError::get_msg() const {
  return _msg;
}
// struct Task
TFtp::Session::Task::Task()
    : type(kIdling),
      mode(PacketRRQ::kUnknown),
      file(new std::fstream()) {
}

TFtp::Session::Task::Task(Type              _type,
                          PacketRRQ::Mode   _mode,
                          const std::string _path)
    : type(_type),
      mode(_mode),
      file(new std::fstream(_path)) {
  DetectFileSize();
}

TFtp::Session::Task::~Task() {
}

size_t TFtp::Session::Task::DetectFileSize() {
  if (not file || not file->is_open()) {
    return 0;
  }
  file->seekg(0, std::ios_base::end);
  file_sz = file->tellg();
  file->seekg(0, std::ios_base::beg);
  return file_sz;
}

void TFtp::Session::Task::PrintState() const {
  if (not file || not file->is_open()) {
    return;
  }
  const ssize_t kOff = file->tellg();
  ShowPercentage("\t * Прогресс", (kOff > 0 ? ((kOff * 100) / file_sz) : 100));
}
// class Session
TFtp::Session::Session(asio::io_service &io)
    : _socket(io),
      _task(Task::kIdling, PacketRRQ::kUnknown, "") {
}

TFtp::Session::~Session() {
  if (IsAlive()) {
    _socket.close();
  }
}

bool TFtp::Session::IsAlive() const {
  return _socket.is_open();
}

bool TFtp::Session::Open(asio::ip::udp::endpoint *srv_endpt) {
  if (srv_endpt == 0) {
    return false;
  }
  sys::error_code err;
  if (IsAlive()) {
    _socket.close();
  }
  _socket.open(boost::asio::ip::udp::v4(), err);
  if (err) {
    return false;
  }
  _socket.set_option(asio::ip::udp::socket::reuse_address(true));
  _socket.bind(*srv_endpt);
  return true;
}

void TFtp::Session::Close() {
  _socket.close();
  _task.file->close();
}

bool TFtp::Session::PushPacket(Packet::Ptr pkt) {
  if (not pkt) {
    return false;
  }
  _output = pkt;
  pkt->Write();
  pkt->StartTransaction(this);
  return true;
}

TFtp::Packet::Ptr TFtp::Session::PopPacket() {
  TFtp::Packet::Ptr t_pkt = _input;
  _input.reset();
  return t_pkt;
}

void TFtp::Session::AsyncAccept() {
  if (_input) {
    return;
  }
  memset(_buff, 0, kBuffMaxSize);
  Receive((Byte*)_buff, kBuffMaxSize);
}

bool TFtp::Session::Send(const Byte *data, size_t size) {
  _input.reset();
//  std::cout << " - отправка данных: " << (unsigned)size << "; "
//            << std::endl;
  _socket.async_send_to(asio::buffer((const char*)data, size), _endpt,
    boost::bind(&TFtp::Session::HandlerWrite, this, 
                asio::placeholders::error,
                asio::placeholders::bytes_transferred));
}

bool TFtp::Session::Receive(Byte *data, size_t size) {
  _input.reset();
//  std::cout << " - ожидание данных: " << (unsigned)size << "; "
//            << std::endl;
  _socket.async_receive_from(asio::buffer((char*)data, size), _endpt,
    boost::bind(&TFtp::Session::HandlerRead, this, 
                asio::placeholders::error,
                asio::placeholders::bytes_transferred));
}

void TFtp::Session::HandlerWrite(const sys::error_code &e,
                                 size_t                 bytes) {
  if (e) {
    std::cerr << e.message() << std::endl;
  }
//  std::cout << " ! отправлены данные: " << (unsigned)bytes << "; "
//            << std::endl;
  AsyncAccept();
}

void TFtp::Session::HandlerRead(const sys::error_code &e,
                                size_t                 bytes) {
  if (e) {
    std::cerr << e.message() << std::endl;
    return;
  }
//  std::cout << " ! получены данные: " << (unsigned)bytes << "; "
//            << std::endl;
  DetectInputPkt();
}

void TFtp::Session::DetectInputPkt() {
  Packet::FieldU16 op;
  op.value = _buff[0];
  const Packet::OpcodeId kOperation = op.bytes[1]; 
  switch (kOperation) {
    case Packet::kRRQ:
      _input.reset(new PacketRRQ());
      break;
    case Packet::kData:
      _input.reset(new PacketData());
      break;
    case Packet::kACK:
      _input.reset(new PacketACK());
      break;
    case Packet::kError:
      _input.reset(new PacketError());
      break;
    default:
      return;
  };
  _input->_buff.set_data((Byte*)&_buff[1], kBuffMaxSize * sizeof(uint16_t));
  _input->Read();
}

bool TFtp::Session::ProcessTask(const Packet::BlockId block_id) {
  if (_task.type != Task::kUploading || 
      not _task.file ||
      not _task.file->is_open() ||
      not _task.file->good()) {
    std::cout << std::endl;
    return false;
  }
  static const size_t kSize = Packet::kMaxSize - 4;
  Byte data[kSize];
  _task.file->read((char*)&data, kSize);
  const size_t kBlockSz = _task.file->gcount();
  _task.PrintState();
  PushPacket(Packet::Ptr(new PacketData(block_id, data, kBlockSz)));
  return true;
}

void TFtp::Session::Process(const VirDir &dir) {
  Packet::Ptr pkt_ptr = PopPacket();
  if (not pkt_ptr) {
    return;
  }
  switch (pkt_ptr->get_opcode()) {
    case Packet::kRRQ: {
      const PacketRRQ &rrq = static_cast<const PacketRRQ&>(*pkt_ptr);
      VirDir::const_iterator f_it = dir.find(rrq.get_filename());
      if (f_it == dir.end()) {
        // TODO: отправка ошибки!
        std::cout << "Файл не найден! "
                  << std::endl;
        break;
      }
      if (_task.type == Task::kIdling) {
        _task.type = Task::kUploading;
        _task.mode = rrq.get_mode();
        _task.file->open(f_it->second, std::fstream::in | std::fstream::binary);
        _task.DetectFileSize();
        ProcessTask(1);
      }
      return;
    }
    case Packet::kACK: {
      const PacketACK &ack = static_cast<const PacketACK&>(*pkt_ptr);
      if (_task.type == Task::kUploading && 
          ProcessTask(ack.get_block_id() + 1)) {
        return;
      }
      break;
    }
    case Packet::kError: {
      const PacketError &err = static_cast<const PacketError&>(*pkt_ptr);
      std::cout << "Ошибка передачи данных: " << (unsigned)err.get_code() << ": "
                << err.get_msg() << "; "
                << std::endl;
      break;    
    }
    default:
      break;
  };
  Close();
}
// class Server
TFtp::Server::Server(const std::string &ip): _ip(ip) {
}

TFtp::Server::~Server() {
}

const std::string& TFtp::Server::get_ip() const {
  return _ip;
}

bool TFtp::Server::PublishFile(const std::string &name,
                               const std::string &path) {
  if (path.size() == 0) {
    return false;
  }
//  std::string name(path, path.find_last_of('/') + 1, std::string::npos);
  if (name.size() == 0) {
    return false;
  }
  _pub_files.insert(VirRec(name, path));
  return true;
}

bool TFtp::Server::Run() {
  sys::error_code         err;
  asio::ip::udp::endpoint endpt(asio::ip::address::from_string(_ip), kPort);
  while (1) {
    _io.reset();
    // ожидание новых подключений
    if (not _new_sess) {
      _new_sess.reset(new Session(_io));
      _new_sess->Open(&endpt);
      _new_sess->AsyncAccept();
      _sessions.push_front(_new_sess);
    }
    // полинг сессий
    for (auto sess_it = _sessions.begin(); sess_it != _sessions.end(); sess_it++) {
      Session::Ptr sess_ptr = *sess_it;
      if (not sess_ptr) {
        continue;
      }
      sess_ptr->Process(_pub_files);
      if (not sess_ptr->IsAlive()) {
        sess_it = _sessions.erase(sess_it);
        _new_sess.reset();
        _io.stop();
        return true;
      }
    }
    _io.run(err);
    if (err) {
      std::cout << "Ошибка сервера: " << err << "; "
                << std::endl;
      break;
    }
  }
  _io.stop();
  return false;
}

