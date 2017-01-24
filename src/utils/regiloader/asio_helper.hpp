#include <boost/asio/deadline_timer.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/asio.hpp>

class AsioIface {
  public:
    typedef void (*Handler)(unsigned try_num);
  
    AsioIface(boost::asio::serial_port *port, unsigned timeout, unsigned tries);
    ~AsioIface();

    size_t Read(void *buff, size_t amount, Handler hndlr);
    bool   ReadingWasFailed() const;
  private:
    typedef boost::asio::deadline_timer Timer;
    typedef boost::scoped_ptr<Timer>    TimerPtr;

    void CreateTimer();
    void TimerHandler(const boost::system::error_code &err);
    void ReadHandler(const boost::system::error_code &err,
                     std::size_t                      amount);

    const unsigned kTimeOutSec;
    const unsigned kAmountOfTries;
    
    size_t                    _was_processed;
    unsigned                  _try_num;
    TimerPtr                  _timer;
    boost::asio::serial_port *_port;
    Handler                   _read_fail_handler;
};
