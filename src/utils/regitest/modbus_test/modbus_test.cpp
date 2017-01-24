#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus/modbus.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <fcntl.h>
#include <termios.h>
#include "mbcomm_rot.h"
#include "../regitest_time.hpp"

//#define MODBUS_TCP
//#define POLL_COLD_JUNCTION

typedef std::numeric_limits<float> FloatLimit;

const int kValW = 5;

struct BoardData {
  BoardData()
      : id(0),
        cur_supp(FloatLimit::infinity()),
        cold_jun(FloatLimit::infinity()) {}
  unsigned id;
  float    cur_supp;
  float    cold_jun;
};

int kbhit(void) {
  struct termios oldt, newt;
  int ch;
  int oldf;
 
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
 
  ch = getchar();
 
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
 
  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }
 
  return 0;
}

uint16_t SwapBytes(const uint16_t src) {
  uint16_t fb = src & 0xFF;
  uint16_t sb = (src >> 8) & 0xFF;
  return (sb | (fb << 8));
}

void SwapBytesInArray(uint8_t amount, uint16_t *out) {
  for (uint8_t i = 0; i < amount; i++) {
    out[i] = SwapBytes(out[i]);
  }
}

float GetFloat(uint16_t *regs) {
  union U16toFloat {
    uint16_t u16[2];
    float    real;
  };
  U16toFloat val;
  val.u16[0] = regs[1];
  val.u16[1] = regs[0];
  return val.real;
}

unsigned long GetULong(uint16_t *regs) {
  union ULtoFloat {
    uint16_t      u16[2];
    unsigned long ul;
  };
  ULtoFloat val;
  val.u16[0] = regs[1];
  val.u16[1] = regs[0];
  return val.ul;
}

std::string PrintRegs(uint16_t *regs, size_t amount) {
  std::stringstream buf;
  buf << "[" << std::hex;
  for (size_t i = 0; i < amount; i++) {
    buf << "0x" << std::left << std::setw(4) << std::uppercase
        << (unsigned)regs[i] << (i != (amount - 1) ? ", " : "");
  } 
  buf << "]" << std::dec;
  return buf.str();
}

std::string PrintAddr(const std::string name, unsigned val) {
  std::stringstream buf;
  buf << std::setw(10) << name << " [" << std::setw(3) << val << "]: ";
  return buf.str();
}

std::string PrintFloat(uint16_t *regs) {
  std::stringstream buf;
  buf << std::setw(kValW) << std::fixed << std::setprecision(2) << GetFloat(regs);
  return buf.str();
}

static uint32_t errors_amount = 0;
int SendRequest_0x3(modbus_t *ctx, unsigned cmd_id, size_t amount, uint16_t *out) {
  const int kRes = modbus_read_registers(ctx, cmd_id, amount, out);
  if (kRes == -1) {
      std::cerr << "READ ERROR: ["
                << std::hex << cmd_id << std::dec << "]: " << modbus_strerror(errno)
                << std::endl;
      errors_amount++;
      return kRes;
  }
  return kRes;
}

int SendRequest_0x4(modbus_t *ctx, unsigned cmd_id, size_t amount, uint16_t *out) {
  const int kRes = modbus_read_input_registers(ctx, cmd_id, amount, out);
  if (kRes == -1) {
      std::cerr << "READ ERROR: [" << cmd_id << "]: " << modbus_strerror(errno)
                << std::endl;
      return kRes;
  }
  return kRes;
}

bool ReadID(modbus_t *ctx, BoardData &out) {
  uint16_t tab_reg[64];
  const int kRes = SendRequest_0x3(ctx, GET_FEID, 0x1, tab_reg);
  if (kRes < 1)
    return false;
  std::cout << PrintAddr("ID", GET_FEID)
            << std::setw(kValW) << (unsigned)tab_reg[0] << "; "
            << PrintRegs(tab_reg, kRes)
            << std::endl;
  out.id = tab_reg[0];
  return true;
}

bool ReadCurSupp(modbus_t *ctx, BoardData &out) {
  uint16_t tab_reg[64];
  const int kRes = SendRequest_0x4(ctx, r_CommCurrentSupply1, 0x2, tab_reg);
  if (kRes < 1)
    return false;
  std::cout << PrintAddr("Cur sup", r_CommCurrentSupply1)
            << PrintFloat(tab_reg) << "; "
            << PrintRegs(tab_reg, kRes)
            << std::endl;
  out.cur_supp = GetFloat(tab_reg);
  return true;
}

bool ReadFloatArray(modbus_t *ctx, int addr, size_t amount, const std::string title) {
  uint16_t tab_reg[64];
  if (amount > 8)
    amount = 8;
  const int kRes = SendRequest_0x4(ctx, addr, amount * 2, tab_reg);
  if (kRes < 1)
    return false;
  std::cout << PrintAddr(title, addr);
  for (size_t off = 0; off < kRes; off+=2) {
    std::cout << PrintFloat(&tab_reg[off]) << " | ";
  }
  std::cout << std::endl;
  return true;
}

bool ReadADC(modbus_t *ctx) {
  return ReadFloatArray(ctx, GET_ADCh(1), 8, "ADC");
}

bool ReadLastTm(modbus_t *ctx) {
  // TODO: вывод списка целых чисел, исп. GetULong
//  return ReadFloatArray(ctx, GET_LASTREADTIMECH(1), 4, "Last read");
  return false;
}

bool ReadADTemp(modbus_t *ctx) {
  return ReadFloatArray(ctx, GET_ADTEMPCH(1), 8, "ADTemp");
}

bool ReadADUsupp(modbus_t *ctx) {
  return ReadFloatArray(ctx, GET_ADVOLTAGESUPP(1), 8, "ADUsup");
}

bool ReadColdJunction(modbus_t *ctx, BoardData &out) {
  uint16_t tab_reg[64];
  const int kRes = SendRequest_0x4(ctx, GET_COLDJUNCTION_TEMP(1), 0x2, tab_reg);
  if (kRes < 1)
    return false;
  std::cout << PrintAddr("Temp", GET_COLDJUNCTION_TEMP(1))
            << PrintFloat(tab_reg) << "; "
            << PrintRegs(tab_reg, kRes)
            << std::endl;
  out.cold_jun = GetFloat(tab_reg);
  return true;
}
/*
bool ReadFirstBlock(modbus_t *ctx) {
  uint16_t tab_reg[128];
  const int kRes = SendRequest_0x3(ctx, GET_FEID, 13, tab_reg);
  if (kRes < 1)
    return false;
  PrintRegs(tab_reg, kRes);
  return true;
}
*/
// REGIGRAF modbus ---------------------
bool F1772ReadSerial(modbus_t *ctx) {
  uint16_t tab_reg[128];
  const int kRes = SendRequest_0x3(ctx, 0x110, 4, tab_reg);
  if (kRes < 1) {
    return false;
  }
  SwapBytesInArray(kRes, tab_reg);
  std::cout << "Serial: " << PrintRegs(tab_reg, kRes) << "; "
            << (char*)tab_reg
            << std::endl; 
  return true;
}

bool F1772ReadUserChannelValues(modbus_t *ctx) {
  uint16_t tab_reg[128];
  const int kRes = SendRequest_0x3(ctx, 0x200, 96, tab_reg);
  if (kRes < 1) {
    return false;
  }
  float *values = (float*)tab_reg;
  std::cout << "Channels: " << std::endl << "\t";
  std::cout.precision(1);
  for (uint8_t i = 0; i < (kRes / 2); i++) {
    std::cout << std::fixed << std::setw(3) << values[i] << "|";
  }
  std::cout << std::endl;
  return true;
}

bool F1772ReadDiscreteInValues(modbus_t *ctx) {
  uint16_t tab_reg[1];
  const int kRes = SendRequest_0x3(ctx, 0x600, 1, tab_reg);
  if (kRes < 1) {
    return false;
  }
  std::cout << "Discrete: " << std::endl << "\t";
  std::cout.precision(1);
  uint8_t offs = 0;
  while (offs < 16) {
    std::cout << (unsigned)((tab_reg[0] >> offs) & 0x1) << "|";
    offs++;
  }
  std::cout << std::endl;
  return true;
}

bool F1772ReadRelay(modbus_t *ctx) {
  uint16_t tab_reg[2];
  const int kRes = SendRequest_0x3(ctx, 0xC00, 2, tab_reg);
  if (kRes < 1) {
    return false;
  }
  std::cout << "Relay: " << std::endl << "\t";
  std::cout.precision(1);
  for (uint8_t i = 0; i < 2; i++) {
    uint8_t offs = 0;
    while (offs < 16) {
      std::cout << (unsigned)((tab_reg[i] >> offs) & 0x1) << "|";
      offs++;
    }
  }
  std::cout << std::endl;
  return true;
}

int main(int argc, char *argv[]) {
  uint16_t           tab_reg[64];
  std::stringstream  buf;
  modbus_t          *ctx;
  int                rc;
  int                i;
  int                tty_dev_id = 0;
  int                slave_id   = 0;
  if (argc > 1) {
    tty_dev_id = atoi(argv[1]);
  }
  if (argc > 2) {
    slave_id = atoi(argv[2]);
  }
  buf << "/dev/ttyUSB" << tty_dev_id;
#ifndef MODBUS_TCP
  ctx = modbus_new_rtu(buf.str().c_str(), 115200, 'N', 8, 1);
#else
  ctx = modbus_new_tcp("192.168.6.2", 502);
#endif
  if (ctx == NULL) {
    fprintf(stderr, "Unable to create the libmodbus context\n");
    return -1;
  }
#ifndef MODBUS_TCP
  if (modbus_set_slave(ctx, slave_id) == -1) {
    std::cout << "Set slave error" << std::endl;
  }
#endif
  std::cout << "Connecting to: " << buf.str() << " [" << slave_id<< "]" << std::endl;
  if (modbus_connect(ctx) == -1) {
      fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
      modbus_free(ctx);
      return -1;
  }
  modbus_set_response_timeout(ctx, 1, 0);
  //modbus_set_debug(ctx, true);
  // let's collect some data
  BoardData inf;
  float     last_cold_jun = FloatLimit::infinity();
  size_t    tries = 0;
  const Clock kBegin;
/*
// команды для опроса измерительных плат регистратора
#ifdef POLL_COLD_JUNCTION
  do {
    ReadID(ctx, inf);
    ReadCurSupp(ctx, inf);
    ReadColdJunction(ctx, inf);
    ReadADC(ctx);
    ReadLastTm(ctx);
    ReadADTemp(ctx);
    if (tries == 0) {
      last_cold_jun = inf.cold_jun;
    }
    if (last_cold_jun != inf.cold_jun)
      break;
    tries++;
  } while (1);
  std::cout << "The temperature was updated after:\n"
            << "\t tries: " << (unsigned)tries << ";\n"
            << "\t spent: " << (Clock().GetDiff(kBegin) / 1000) << " msec;\n"
            << std::endl;
#else
  ReadID(ctx, inf);
  ReadCurSupp(ctx, inf);
  ReadColdJunction(ctx, inf);
  ReadADC(ctx);
  ReadLastTm(ctx);
  ReadADTemp(ctx);
#endif
*/
  int key        = 0;
  int req_amount = 0;
  F1772ReadSerial(ctx);
  do {
    F1772ReadUserChannelValues(ctx);
    F1772ReadDiscreteInValues(ctx);
    //F1772ReadRelay(ctx);
    if (kbhit()) {
      key = getchar();
    }
    req_amount++;
    usleep(100000);
  } while (key != 27 && req_amount < 50);
  modbus_close(ctx);
  modbus_free(ctx);
  
  std::cout << "Amount of errors: " << (unsigned)errors_amount << std::endl;
}
