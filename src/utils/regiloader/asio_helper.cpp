#include "asio_helper.hpp"
#include <boost/bind.hpp>
#include <iostream>

AsioIface::AsioIface(boost::asio::serial_port *port,
                     unsigned                  timeout,
                     unsigned                  tries)
    : kTimeOutSec(timeout),
      kAmountOfTries(tries),
      _was_processed(0),
      _try_num(0),
      _port(port) {
  if (port != 0) {
    _timer.reset(new Timer(port->get_io_service()));
  }
}

AsioIface::~AsioIface() {
}

bool AsioIface::ReadingWasFailed() const {
  return (_try_num >= kAmountOfTries);
}

void AsioIface::CreateTimer() {
  _timer->expires_from_now(boost::posix_time::seconds(kTimeOutSec));
  _timer->async_wait(boost::bind(&AsioIface::TimerHandler,
        this,
        boost::asio::placeholders::error));
}

size_t AsioIface::Read(void *buff, size_t amount, Handler hndlr) {
  _try_num       = 0;
  _was_processed = 0;
  if (not _timer || _port == 0) {
    return 0;
  }
  _read_fail_handler = hndlr;
  boost::asio::io_service &io = _port->get_io_service();
  do {
    _port->async_read_some(boost::asio::buffer(buff, amount),
      boost::bind(&AsioIface::ReadHandler,
        this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred)); 
    CreateTimer();
    io.run();
    io.reset();
    if (_try_num > 0 && _read_fail_handler != 0) {
      _read_fail_handler(_try_num);
    }
  } while (_try_num < kAmountOfTries && _was_processed < amount);
  return _was_processed;
}

void AsioIface::TimerHandler(const boost::system::error_code &err) {
  const bool exp = _timer->expires_at() <= boost::asio::deadline_timer::traits_type::now();
  if (not err && exp) {
    _try_num++;
    if (_port) {
      _port->cancel();
    }
    return;
  }
  _try_num = 0;
}

void AsioIface::ReadHandler(const boost::system::error_code &err,
                            std::size_t                      amount) {
  if (not err && amount == 1) {
    _was_processed = amount;
    _timer->cancel();
    return;
  }
}
    
