#include <iostream>
#include <libusb-1.0/libusb.h>
#include <usb.h>

enum UsbSessType {
  kNewUsbSession = 0,
  kDefUsbSession = 1
};

void TestLibUSBInit(UsbSessType sess_t) {
  static const char *kSessTypeName[] = {
    "new",
    "default"
  };
  std::cout << "* Testing lib USB init and exit with "
            << kSessTypeName[sess_t]
            << " session..." << std::endl;
  libusb_context *ctx;
  int err = libusb_init(sess_t == kNewUsbSession ? &ctx : 0);
  if (err != 0) {
    std::cout << "! libusb_init error: " << err << std::endl;
    return;
  }
  libusb_exit(sess_t == kNewUsbSession ? ctx : 0);
}

void TestUSBInit() {
  std::cout << "* Testing USB init..." << std::endl;
  usb_init();
}

int main() {
  std::cout << "Testing USB bus and devices..." << std::endl;
//  TestLibUSBInit(kDefUsbSession);
  TestUSBInit();
}
