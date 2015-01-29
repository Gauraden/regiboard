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
#include "mbcomm_rot.h"

//#define MODBUS_TCP

void PrintRegs(uint16_t *regs, size_t amount) {
  for (size_t i = 0; i < amount; i++) {
    printf("reg[%d]=%d (0x%X)\n", i, regs[i], regs[i]);
  }
}

int SendRequest_0x3(modbus_t *ctx, unsigned cmd_id, size_t amount, uint16_t *out) {
  const int kRes = modbus_read_registers(ctx, cmd_id, amount, out);
  if (kRes == -1) {
      fprintf(stderr, "READ ERROR: [%u]: %s\n", cmd_id, modbus_strerror(errno));
      return kRes;
  }
  return kRes;
}

int SendRequest_0x4(modbus_t *ctx, unsigned cmd_id, size_t amount, uint16_t *out) {
  const int kRes = modbus_read_input_registers(ctx, cmd_id, amount, out);
  if (kRes == -1) {
      fprintf(stderr, "READ ERROR: [%u]: %s\n", cmd_id, modbus_strerror(errno));
      return kRes;
  }
  return kRes;
}

bool ReadID(modbus_t *ctx) {
  uint16_t tab_reg[64];
  const int kRes = SendRequest_0x3(ctx, GET_FEID, 0x1, tab_reg);
  if (kRes < 1)
    return false;
  PrintRegs(tab_reg, kRes);
  return true;
}

bool ReadColdJunction(modbus_t *ctx) {
  uint16_t tab_reg[64];
  const int kRes = SendRequest_0x4(ctx, GET_COLDJUNCTION_TEMP(1), 0x2, tab_reg);
  if (kRes < 1)
    return false;
  PrintRegs(tab_reg, kRes);
  union U16toFloat {
    uint16_t u16[2];
    float    real;
  };
  U16toFloat temp;
  temp.u16[0] = tab_reg[1];
  temp.u16[1] = tab_reg[0];
  //float *temp = reinterpret_cast<float*>(tab_reg);
  std::cout << "temp: " << std::fixed << temp.real
            << std::endl;
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
  ReadID(ctx);
  ReadColdJunction(ctx);
  modbus_close(ctx);
  modbus_free(ctx);
}
