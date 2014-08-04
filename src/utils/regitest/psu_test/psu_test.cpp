#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <iostream>
#include <poll.h>
#include <sstream>
#include <signal.h>

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cout << "Укажите <номер порта> <номер ноги> GPIO интерфейса!" << std::endl;
    return 1;
  }
	int gpio_port = atoi(argv[1]);
	int gpio_pin  = atoi(argv[2]);
	if (gpio_port < 1) {
    std::cout << "GPIO порты нумеруются с 1!" << std::endl;
	  return 1;
	}
  std::cout << "Ожидание прерывания на GPIO" << gpio_port
                                      << "_" << gpio_pin << std::endl;
  int gpio_id = ((gpio_port - 1) * 32) + gpio_pin;
  std::stringstream gpio_dev_path;
  gpio_dev_path << "/sys/class/gpio/gpio" << gpio_id << "/value";
  std::cout << "Открытие интерфейса: " << gpio_dev_path.str() << std::endl;
  int fd = open(gpio_dev_path.str().c_str(), O_RDONLY);
	if (fd == -1) {
		std::cout << "Ошибка открытия интерфейса: " << fd << std::endl;
		return 1;
	}

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	std::cout << "Ожидание сигнала о потере питания!" << std::endl;
	char buf[10];
	int len = read(fd, buf, 10);
  std::cout << "read: " << (unsigned)buf[0] << std::endl;
	int res = select(fd + 1, NULL, NULL, &fds, NULL);
  std::cout << "Сигнал получен! " << res << std::endl;

	close(fd);
	return 0;
}
