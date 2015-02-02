#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include "mbcomm_rot.h"
#include "../regitest_time.hpp"

//#define MODBUS_TCP
#define POLL_COLD_JUNCTION

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

int SendRequest_0x3(modbus_t *ctx, unsigned cmd_id, size_t amount, uint16_t *out) {
  const int kRes = modbus_read_registers(ctx, cmd_id, amount, out);
  if (kRes == -1) {
      std::cerr << "READ ERROR: [" << cmd_id << "]: " << modbus_strerror(errno)
                << std::endl;
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
  ctx = modbus_new_tcp("127.0.0.1", 4000);
#endif
  if (ctx == NULL) {
    fprintf(stderr, "Unable to create the libmodbus context\n");
    return -1;
  }
  if (modbus_set_slave(ctx, slave_id) == -1) {
    std::cout << "Set slave error" << std::endl;
  }
  std::cout << "Connecting to: " << buf.str() << " [" << slave_id<< "]" << std::endl;
  if (modbus_connect(ctx) == -1) {
      fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
      modbus_free(ctx);
      return -1;
  }
  modbus_set_response_timeout(ctx, 1, 0);
  // let's collect some data
  BoardData inf;
  float     last_cold_jun = FloatLimit::infinity();
  size_t    tries = 0;
  const Clock kBegin;
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
  modbus_close(ctx);
  modbus_free(ctx);
}
