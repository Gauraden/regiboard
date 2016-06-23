#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <list>
#include <map>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_array.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace asio = ::boost::asio;
namespace sys  = ::boost::system;
/**
 * TFTP - Trivial File Transfer Protocol
 * Спецификация протокола:
 *   https://tools.ietf.org/html/rfc1350
 * Частичная реализация:
 * - обработка запроса на скачивание файла
 * - передача файла
 */
class TFtp {
  public:
    typedef uint8_t                             Socket;
    typedef uint8_t                             Byte;
    typedef std::map<std::string, std::string>  VirDir;
    typedef std::pair<std::string, std::string> VirRec;

    class Session;
    // Базовый класс пакетов
    class Packet {
      public:
        typedef uint8_t                   Byte;
        typedef uint16_t                  BlockId;
        typedef uint16_t                  OpcodeId;
        typedef uint16_t                  ErrorCode;
        typedef boost::shared_ptr<Packet> Ptr;
        
        static const size_t kMaxSize = 516;
        
        enum Opcode {
          kRRQ   = 1, // Read request
          kWRQ   = 2, // Write request
          kData  = 3, // Data
          kACK   = 4, // Acknowledgment
          kError = 5  // Error
        };
      
        union FieldU16 {
          uint16_t value;
          Byte     bytes[2];
        };
      
        Packet(Opcode opc);
        virtual ~Packet();
        OpcodeId get_opcode() const;
      protected:
        class Buffer {
          public:
            Buffer();
            ~Buffer();
            
            bool set_data(const Byte *bytes, size_t size);
            const Byte* get_data() const;
            size_t      get_size() const;
            bool AddArray(const Byte *array, size_t size);
            bool GetArray(size_t offs, Byte *array, size_t size) const;
            void Clear();
          private:
            Byte   _data[kMaxSize];
            size_t _size;
        };
      
        virtual bool Read();
        virtual bool Write();
        void Clear();

        template <typename ValueType>
        bool AddVal(const ValueType &val);
        template <typename ValueType>
        size_t GetVal(size_t offs, ValueType *val);
        
        Buffer _buff;
      private:
        friend class Session;
        
        bool StartTransaction(Session *sess);
        void LSB2MSB(Byte *bytes) const;
      
        OpcodeId _opcode;
    };
    // Пакет запроса на чтение данных
    class PacketRRQ : public Packet {
      public:
        enum Mode {
          kUnknown = 0,
          kNetASCII,
          kOctet, // обычные байтики
          kMail
        };
        
        PacketRRQ();
        PacketRRQ(const std::string &filename, Mode mode);
        virtual ~PacketRRQ();
        std::string get_filename() const;
        Mode        get_mode() const;
      private:
        virtual bool Read();
        virtual bool Write();
        
        std::string _filename;
        Mode        _mode;
    };
    // Пакет для передачи/приёма данных
    class PacketData : public Packet {
      public:
        PacketData();
        PacketData(BlockId id, Byte *data, size_t size);
        virtual ~PacketData();
        BlockId     get_block_id() const;
        const Byte* get_data() const;
        size_t      get_size() const;
        bool        IsLastBlock() const;
      private:
        virtual bool Read();
        virtual bool Write();
        
        BlockId _block_id;
    };
    // Пакет для подтверждения пересылки данных
    class PacketACK : public Packet {
      public:
        PacketACK();
        PacketACK(BlockId block_id);
        virtual ~PacketACK();
        BlockId get_block_id() const;
      private:
        virtual bool Read();
        virtual bool Write();
        
        BlockId _block_id;
    };
    // Пакет для передачи/получения сообщения об ошибке
    class PacketError : public Packet {
      public:
        PacketError();
        PacketError(ErrorCode code, const std::string &msg);
        virtual ~PacketError();
        ErrorCode   get_code() const;
        std::string get_msg() const;
      private:
        virtual bool Read();
        virtual bool Write();
        
        ErrorCode   _code;
        std::string _msg;
    };
        
    class Session {
      public:
        friend class Packet;
        
        typedef asio::ip::udp::socket      Socket;
        typedef asio::ip::udp::endpoint    EndPoint;
        typedef boost::shared_ptr<Session> Ptr;
        typedef std::list<Session::Ptr>    ListOfPtr;
      
        Session(asio::io_service &io);
        ~Session();
        bool IsAlive() const;
        bool Open(asio::ip::udp::endpoint *srv_endpt);
        void Close();
        void Process(const VirDir &dir);
        bool PushPacket(Packet::Ptr pkt);
        Packet::Ptr PopPacket();
        void AsyncAccept();
      protected:
        bool Send(const Byte *data, size_t size);
        bool Receive(Byte *data, size_t size);
      private:
        struct Task {
          typedef boost::scoped_ptr<std::fstream> FilePtr;
          
          enum Type {
            kIdling,
            kUploading,
            kWaiting
          };
          
          Task();
          Task(Type              _type,
               PacketRRQ::Mode   _mode,
               const std::string _path);
          ~Task();
          
          size_t DetectFileSize();
          void   PrintState() const;
          
          Type            type;
          PacketRRQ::Mode mode;
          FilePtr         file;
          size_t          file_sz;
        };
        
        static const size_t kBuffMaxSize = Packet::kMaxSize / 2;
        
        void HandlerWrite(const sys::error_code &e,
                          size_t                 bytes);
        void HandlerRead(const sys::error_code &e,
                         size_t                 bytes);
        void DetectInputPkt();
        bool ProcessTask(const Packet::BlockId block_id);
        
        Packet::Ptr _output;
        Packet::Ptr _input;
        Socket      _socket;
        EndPoint    _endpt;
        uint16_t    _buff[kBuffMaxSize];
        Task        _task;
    };
    
    class Server {
      public:
        static const int kPort = 6969;
        
        Server(const std::string &ip);
        ~Server();
        
        bool Run();
        bool PublishFile(const std::string &name, const std::string &path);
        const std::string& get_ip() const;
      private:
        std::string        _ip;
        asio::io_service   _io;
        Session::Ptr       _new_sess;
        Session::ListOfPtr _sessions;
        VirDir             _pub_files;
    };
  
    TFtp();
    ~TFtp();   
};
