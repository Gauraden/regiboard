#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <cstring>
#include <string>
#include <list>
#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/scoped_array.hpp>

/**
 * SDP - Serial Download Protocol. Packet.
 * Описание можно посмотреть в: iMX53RM.pdf
 * - описание образа...: стр. 518 (7.6 Program Image)
 * - описание протокола: стр. 529 (7.8.3 Serial Download Protocol)
 */
class SdpPacket {
  public:
    SdpPacket(uint8_t cmd, size_t response_sz, size_t request_sz = kPktSize);
    virtual ~SdpPacket();
    bool Send(boost::asio::serial_port &port);
    bool Send(boost::asio::serial_port &port,
              unsigned                  tries,
              const std::string        &msg);
  protected:
    static const size_t kPktSize = 16;

    enum Transfer {
      kStop     = 0,
      kContinue = 1,
      kError    = 2
    };

    enum ACK {
      kClosedSecurityConf = 0x12343412,
      kOtherwise          = 0x56787856
    };

    union FieldU16 {
      uint16_t value;
      uint8_t  bytes[2];
    };
      
    union FieldU32 {
      uint32_t value;
      uint8_t  bytes[4];
    };
    
    static std::string GetAckName(FieldU32 val);
    
    virtual bool Write(boost::asio::serial_port&) = 0;
    virtual bool Write() = 0;
    virtual Transfer Read() = 0;

    void AddVal(uint8_t offs, FieldU32 &val);
    void AddMsbVal(uint8_t offs, FieldU32 &val);
    void AddVal(uint8_t offs, uint8_t val);
    void GetVal(uint8_t offs, FieldU32 &val);
    void GetMsbVal(uint8_t offs, FieldU32 &val);
    void GetVal(uint8_t offs, uint8_t &val);
  private:
    typedef boost::asio::deadline_timer Timer;
    typedef boost::scoped_ptr<Timer>    TimerPtr;
    
    struct Session {
      typedef boost::scoped_ptr<Session> Ptr;
      
      Session(boost::asio::serial_port &_port, uint8_t *_out, size_t _out_size);
      
      boost::asio::serial_port &port;
      uint8_t                  *out;
      size_t                    out_size;
    };
  
    void AddArr(uint8_t offs, uint8_t *arr, size_t sz, bool msb = false);
    void GetArr(uint8_t offs, uint8_t *arr, size_t sz, bool msb = false);
    void Print(const std::string &pref, std::ostream &out);
    void HandlerTimeout(const boost::system::error_code &error);
    void HandlerRead(const boost::system::error_code &error,
                     std::size_t                      bytes_transferred);
    void HandlerWrite(const boost::system::error_code &error,
                      std::size_t                      bytes_transferred);
    bool ReadArray(boost::asio::serial_port &port,
                   size_t                    size,
                   uint8_t                  *out);
    bool TransferData(boost::asio::serial_port &port);
    void CreateTimer(boost::asio::io_service &io);
    void CreateSession(boost::asio::serial_port &port,
                       uint8_t                  *out,
                       size_t                    out_size);
    void DestroySession();
  
    Session::Ptr _session;
    TimerPtr     _timer;
    FieldU16     _cmd_id;
    uint8_t      _packet[kPktSize];
    size_t       _resp_size;
    size_t       _req_size;
    bool         _was_read;
    bool         _timeout;
};

class PktStatus : public SdpPacket {
  public:
    PktStatus();
    virtual ~PktStatus();
  protected:
    virtual bool     Write(boost::asio::serial_port&);
    virtual bool     Write();
    virtual Transfer Read();
};

class PktComplete : public SdpPacket {
  public:
    PktComplete();
    virtual ~PktComplete();
  protected:
    virtual bool     Write(boost::asio::serial_port&);
    virtual bool     Write();
    virtual Transfer Read();
};

class PktWriteMem : public SdpPacket {
  public:
    enum DataSize {
      kByte     = 0x08,
      kHalfWord = 0x10,
      kWord     = 0x20
    };
    
    PktWriteMem(uint32_t addr, uint32_t data);
    virtual ~PktWriteMem();
    static size_t GetSizeFor(DataSize sz_id);
  protected:
    virtual bool     Write(boost::asio::serial_port&);
    virtual bool     Write();
    virtual Transfer Read();
    
    uint32_t _addr;
    uint32_t _data;
};

class PktReadMem : public SdpPacket {
  public:
    typedef PktWriteMem::DataSize DataSize;
    
    PktReadMem(uint32_t addr, DataSize size);
    virtual ~PktReadMem();
    uint32_t GetValue() const;
  protected:
    virtual bool     Write(boost::asio::serial_port&);
    virtual bool     Write();
    virtual Transfer Read();
    
    uint32_t _addr;
    DataSize _size;
    uint32_t _value;
};

class ImxFirmware;

class PktWriteF : public SdpPacket {
  public:
    PktWriteF(ImxFirmware *firm);
    virtual ~PktWriteF();
  protected:
    /// Types of uploaded data
    enum FileType {
      kApplication = 0xAA, // terminates protocol
      kCSF         = 0xCC,
      kDCD         = 0xEE
    };
    
    static std::string GetFileTypeName(FileType ft);
    
    virtual bool     Write(boost::asio::serial_port &port);
    virtual bool     Write();
    virtual Transfer Read();
    
    ImxFirmware *_firm;
};
/// Firmware class for iMX devices
class ImxFirmware {
  public:
    enum Barker {
      kImx51Barker = 0xB1,
      kImx53Barker = 0x402000D1
    };
    // Boot Data Structure
    struct BootData {
	    uint32_t dest;      // Absolute address of the image
	    uint32_t image_len; // Size of the program image
	    uint32_t plugin;    // Plugin flag
    };
    // DCD pair: [Address][Value/Mask]
    struct DCDPair {
      typedef std::list<DCDPair> List;
      DCDPair();
      uint32_t addr;
      uint32_t val_mask;
    };
    // DCD CMD
    struct DCDCmd {
      typedef std::list<DCDCmd> List;
      
      static const uint8_t kTag = 0xCC;
      
      DCDCmd();
      uint8_t       tag;       // [0xCC]
      uint16_t      length;    // header + commands
      uint8_t       parameter; 
      DCDPair::List pairs;
    };
    // Device Configuration Data
    struct DCD {
      static const uint8_t kTag = 0xD2;
      
      DCD();
      uint8_t      tag;      // [0xD2]
      uint16_t     length;   // length of the DCD
      uint8_t      version;  // [0x40]
      DCDCmd::List cmd_list;
    };
    // iMX53: Image Vector Table Structure
    struct Imx53IVT {
	    uint32_t barker;        // [0xD1] [length of IVT] [0x40] (см. kImx53Barker)
	    uint32_t start_addr;    // Absolute address of the first instruction to execute from the image.
	    uint32_t reserv1;
	    uint32_t dcd_ptr;       // Absolute address of the image DCD.
	    uint32_t boot_data_ptr;	// Absolute address of the Boot Data. /* struct boot_data * */
	    uint32_t self_ptr;	    // Absolute address of the IVT. Used internally by the ROM /* struct imx_flash_header_v2 *, this - boot_data.start = offset linked at */
	    uint32_t app_code_csf;  // Absolute address of Command Sequence File.
	    uint32_t reserv2;
    };
    
    ImxFirmware();
    ~ImxFirmware();
    
    bool LoadFromFile(const std::string &path);
    void PrintStructures(std::ostream &out) const;
    const DCDCmd::List& GetDCDCommands() const;
    size_t GetSize() const;
    const uint8_t* GetData() const;
    const BootData* GetBootData() const;
    const Imx53IVT* GetIvtData() const;
  private:
    typedef boost::scoped_array<uint8_t> Array;
    
    uint8_t* InitDCDListOfCommandsPairs(uint8_t *list_ptr, DCDCmd &cmd);
    bool     InitDCDListOfCommands(uint8_t *list_ptr); 
    bool     InitHeader();
    
    size_t    _fsize;
    Array     _fptr;
    uint8_t  *_exec;
    Imx53IVT *_header;
    BootData *_boot;
    DCD       _dcd;
};
