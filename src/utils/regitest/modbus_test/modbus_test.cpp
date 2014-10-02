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
  rc = modbus_read_registers(ctx, GET_FEID, 0x1, tab_reg);
  if (rc == -1) {
      fprintf(stderr, "READ ERROR: %s\n", modbus_strerror(errno));
      return -1;
  }
  for (i=0; i < rc; i++) {
      printf("reg[%d]=%d (0x%X)\n", i, tab_reg[i], tab_reg[i]);
  }
  modbus_close(ctx);
  modbus_free(ctx);
}
