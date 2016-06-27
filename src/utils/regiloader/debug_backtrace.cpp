/*
 * debug_backtrace.cpp
 *
 *  Created on: 11.11.2015
 *      Author: denis
 */
#include "debug_backtrace.hpp"
#include <execinfo.h>
#include <sstream>
#include <cstdlib>

namespace debug {
Backtrace::Backtrace() {
}

Backtrace::~Backtrace() {
}

bool Backtrace::KeepStack() {
  return ReadStackTo(&_stack);
}

bool Backtrace::PrintStackTo(std::ostream *out) {
  if (out == 0) {
    return false;
  }
  (*out) << _stack;
  return true;
}

bool Backtrace::ReadStack(std::string *out) {
  return ReadStackTo(out);
}

bool Backtrace::ReadStackTo(std::string *out) {
  if (out == 0) {
    return false;
  }
  static const unsigned kStackBufSz = 100;
  int    i,
         nptrs;
  void  *buf[kStackBufSz];
  char **strings;
  nptrs   = backtrace(buf, kStackBufSz);
  strings = backtrace_symbols(buf, nptrs);
  std::stringstream stack_log;
  if (strings != NULL) {
    for (i = 0; i < nptrs; i++) {
      stack_log << strings[i] << std::endl;
    }
    free(strings);
  } else {
    stack_log << "no symbols";
  }
  out->assign(stack_log.str());
  return true;
}

void Backtrace::Clear() {
  _stack.assign("");
}

} // namespace debug
