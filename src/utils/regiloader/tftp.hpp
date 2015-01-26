#include <stdio.h>
#include <stdint.h>
#include <boost/asio.hpp>
#include <boost/shared_array.hpp>
/**
 * TFTP - Trivial File Transfer Protocol
 * Спецификация протокола:
 *   https://tools.ietf.org/html/rfc1350
 */
class TFtp {
  public:
    typedef uint8_t Socket;
    
    class Packet {
      public:
        typedef uint16_t OpcodeId;
        
        enum Opcode {
          kRRQ   = 1, // Read request
          kWRQ   = 2, // Write request
          kData  = 3, // Data
          kACK   = 4, // Acknowledgment
          kError = 5  // Error
        };
      
        Packet(Opcode opc);
        ~Packet();
        
        bool Read();
        bool Write();
      protected:

      private:
        const OpcodeId _opcode;
    };
    
    class Server {
      public:
        Server();
        ~Server();
    };
  
    TFtp();
    ~TFtp();
};
