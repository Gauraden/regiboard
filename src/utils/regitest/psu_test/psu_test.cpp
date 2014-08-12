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


//#include <sys/epoll.h>

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
  int fd = open(gpio_dev_path.str().c_str(), O_RDONLY | O_NONBLOCK);
	if (fd == -1) {
		std::cout << "Ошибка открытия интерфейса: " << fd << std::endl;
		return 1;
	}
	/*
	int epfd = epoll_create(1);
  struct epoll_event ev;
  struct epoll_event events;
  ev.events  = EPOLLPRI;
  ev.data.fd = fd;
	std::cout << "Ожидание сигнала о потере питания!" << std::endl; 
  epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
  
  while (1) {
    int n = epoll_wait(epfd, &events, 1, -1);
    if (n > 0) {
      std::cout << "Сигнал получен! " << std::endl;
      char buf = 0;
      lseek(fd, 0, SEEK_SET);
      read(fd, &buf, 1);
      std::cout << "read: " << buf << std::endl;
      break;
    }
  }*/
/*
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	std::cout << "Ожидание сигнала о потере питания!" << std::endl;
	char buf;
	lseek(fd, 0, SEEK_SET);
	read(fd, &buf, 1);
  std::cout << "0 read: " << buf << std::endl;
	int res = select(fd + 1, NULL, NULL, &fds, NULL);
  std::cout << "Сигнал получен! " << res << std::endl;
  lseek(fd, 0, SEEK_SET);
  read(fd, &buf, 1);
  std::cout << "1 read: " << buf << std::endl;
  */
 	std::cout << "Ожидание сигнала о потере питания!" << std::endl;
  char buf;
	lseek(fd, 0, SEEK_SET);
	read(fd, &buf, 1);
  std::cout << "0 read: " << buf << std::endl;
  struct pollfd pfds[1];
  pfds[0].fd     = fd;
  pfds[0].events = POLLPRI | POLLERR;
  int pres = poll(pfds, (nfds_t)1, -1);
  if (pres > 0) {
	  lseek(fd, 0, SEEK_SET);
	  read(fd, &buf, 1);
    std::cout << "1 read: " << buf << std::endl;
  }
	close(fd);
	
	return 0;
}
