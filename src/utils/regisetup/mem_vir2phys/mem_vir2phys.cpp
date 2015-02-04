#include <iostream>
#include <cstdlib>

int main(int argc, char **argv) {
  if (argc < 1)
    return 1;
  unsigned long vaddr = strtol(argv[0], 0, 16);
  unsigned long paddr = 0x0;
  std::cout << std::hex 
            << "vmem: 0x" << vaddr << "; "
            << "pmem: 0x" << paddr << "; "
            << std::endl;
  return 0;
}
