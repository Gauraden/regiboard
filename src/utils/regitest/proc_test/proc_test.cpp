#include <iostream>
#include <fstream>
#include <string>
#include <stdint.h>
#include <dirent.h>
#include <cstring>
#include <stdlib.h>

static const char kSysClassNet[] = "/sys/class/net/";

static bool GetEthDevicesThroughIOCtl() {
  struct dirent **namelist;
  int n = scandir(kSysClassNet, &namelist, 0, alphasort);
  if (n < 0) {
    return false;
  }
  while (n--) {
    const std::string kName(namelist[n]->d_name);
    if (kName != "." && kName != "..") {
      //std::cout << "\t * " << kName << std::endl;
    }
    free(namelist[n]);
  }
  free(namelist);
  return true;
}

static bool ReadLinesFromProc(const std::string &path) {
  const uint16_t kBuffSize = 128;
  char buff[kBuffSize];
  std::fstream fs;
  const uint16_t kFsBuffSize = 64;
  char fs_buff[kFsBuffSize];
  fs.rdbuf()->pubsetbuf(fs_buff, kFsBuffSize);
  fs.open(path.c_str(), std::fstream::in);
  if (not fs.is_open()) {
    return false;
  }
  size_t line = 0;
  while (1) {
    // DEBUG --------------------
    std::cout << "DEBUG: " << __FUNCTION__ << ": " << __LINE__ << ": ";
    // --------------------------
    try {
      fs.getline(buff, kBuffSize);
    } catch (...) {
      // DEBUG --------------------
      std::cout << __LINE__ << ": EXCEPT" << std::endl;
      // --------------------------
      return false;
    }
    // DEBUG --------------------
    std::cout << __LINE__ << ": ";
    // --------------------------
    const int32_t kLen = fs.gcount() - 1;
    // DEBUG --------------------
    std::cout << __LINE__ << ": ";
    // --------------------------
    if (not fs.good()) {
      // DEBUG --------------------
      std::cout << __LINE__ << " = " << (int)kLen << std::endl;
      //std::cout << __LINE__ << ": " << fs_buff << std::endl;
      // --------------------------
      break;
    }
    // DEBUG --------------------
    std::cout << __LINE__ << ": ";
    // --------------------------
    if (kLen < 1) {
      // DEBUG --------------------
      std::cout << __LINE__ << ": " << std::endl;
      // --------------------------
      continue;
    }
    // DEBUG --------------------
    std::cout << __LINE__ << " = " << (int)kLen << std::endl;
    std::cout << __LINE__ << ": " << buff << std::endl;
    //std::cout << __LINE__ << ": " << fs_buff << std::endl;
    // --------------------------
    GetEthDevicesThroughIOCtl();
    line++;
  }
  fs.close();
    // DEBUG --------------------
    std::cout << __LINE__ << " ---------------------- " << std::endl;
    // --------------------------
  return true;
}

int main(int argc, char** argv) {
  if (argc < 1) {
    std::cout << "Укажите файл в /proc/ для чтения!" << std::endl;
    return 1;
  }
  std::cout << "Чтение файла начато!" << std::endl;
  do {
    if (not ReadLinesFromProc(argv[1])) {
      break;
    }
  } while (1);
  std::cout << "Чтение файла завершено!" << std::endl;
  return 0; 
}
