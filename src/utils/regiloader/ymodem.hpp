#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <boost/asio.hpp>
#include <boost/shared_array.hpp>
/**
 * Протокол:
 *   http://www.techfest.com/hardware/modem/xymodem.htm
 * Переписана данная реализация (автор - Adam Ficsor):
 *   https://sites.google.com/site/adamficsor1024/home/programming/xymodem
 * Она не совсем соответствует тому что написано в описании протокола, см.
 * пример передачи данных:
 *   Figure 3.  YMODEM Batch Transmission Session (1 file)
 */
class YModem {
  public:
    typedef boost::asio::serial_port SPort;
   
    YModem();
    ~YModem();
    bool SendFile(const std::string &path, SPort &port);
  private:
    typedef boost::shared_array<uint8_t> Array;

    class Crc {
      public:
        typedef uint16_t Value;
        
        static const uint16_t kSize = 2;
      
        enum InitialValue {
          kZeros,
          kNonZero1 = 0xffff,
          kNonZero2 = 0x1D0F
        };
        
        Crc(InitialValue val);
        ~Crc();
        
        Value Compute(uint8_t *data, size_t sz) const;
      private:
        static const Value  kPoly    = 0x1021;
        static const size_t kTableSz = 256;
        
        Value _table[kTableSz];
        Value _ini_value;
    };
    
    class Packet {
      public:
        typedef uint8_t Id;

        enum Signal {
          kSTX     = 0x02,
          kEOT     = 0x04,
          kACK     = 0x06,
          kNAK	   = 0x15,
          kRequest = 67   
        };
        
        class Sequence {
          public:
            Sequence(SPort *port);
            ~Sequence();
            
            bool Send(const Packet &pkt);
            SPort* get_port();
            void Reset();
          private:
            Id     _pkt_num;
            SPort *_port;
        };
        
        Packet(Signal type);
        Packet(Signal type, uint8_t *data, size_t sz);
        ~Packet();
        
        bool Send(SPort &port, Id pkt_num) const;
      private:
        bool SendArray(SPort &port, uint8_t *arr, size_t sz) const;
        
        template <typename DataType>
        bool SendScalar(SPort &port, DataType val) const {
          static const size_t kSize = sizeof(DataType);
          return SendArray(port, reinterpret_cast<uint8_t*>(&val), kSize);
        }
      
        const uint8_t  _type;
        uint8_t       *_data;
        size_t         _data_sz;
    };
  
    static const uint16_t kFrameSize = 1024;
  
    Array NewFrame() const;
    bool IsResponse(SPort &port, const char expect) const;
    bool WaitForResponse(SPort &port, const char expect) const;
    bool SendInitialPkt(Packet::Sequence &seq, const std::string *msg, size_t amount);
    bool SendClosingPkt(Packet::Sequence &seq);
};
