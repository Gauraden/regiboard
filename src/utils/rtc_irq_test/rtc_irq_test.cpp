#include <stdio.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <iostream>

#define RTC_UIE_ON   _IO('p', 0x03)
#define RTC_UIE_OFF  _IO('p', 0x04)
#define RTC_TEMP_GET _IOR('p', 0x13, int)

void read_temp(int fd) {
	int temp;
	int err = ioctl(fd, RTC_TEMP_GET, &temp);
	if (err == 0)
		std::cout << "Temperature: " << (temp >> 2) << "." << ((temp & 0x3) * 25) << " C"
			        << std::endl;
}

int main(int /*argc*/, char** argv)
{
	int fd, retval, irqcount = 0;
	int countdown_val = atoi(argv[1]);
	int iters_amount  = atoi(argv[2]);

	tm result;

	char DEVICE[] = "/dev/rtc0";

	std::cout << "Open: " << DEVICE << std::endl;
	fd = open(DEVICE, O_RDONLY);
	if (fd == -1) {
		std::cout << "Open error: " << fd << std::endl;
		return 0;
	}

	read_temp(fd);

	std::cout << "Send: RTC_IRQP_SET: " << countdown_val << std::endl;
	retval = ioctl(fd, RTC_IRQP_SET, countdown_val);
	if (retval == -1)
		std::cout << "RTC_IRQP_SET not suported: " << retval << std::endl;

	std::cout << "Send: RTC_UIE_ON" << std::endl;
	retval = ioctl(fd, RTC_UIE_ON);
	if (retval == -1) {
		if (errno == ENOTTY)
			std::cout << "RTC_UIE_ON not suported: " << retval << std::endl;
		else
			std::cout << "RTC_UIE_ON failed: " << retval << "; errno: " << errno << std::endl;
		close(fd);
		return 0;
	}

	std::cout << "Reading " << iters_amount << " times ..." << std::endl;
	irqcount = 0;
	while (irqcount < iters_amount) {
		retval = ioctl(fd, RTC_RD_TIME, &result);
		if (retval == -1) {
			std::cout << "\t Error while reading: " << retval << std::endl;
			return 0;
		} else {
			std::cout << "\t OK: " << result.tm_hour << ":" << result.tm_min << ":" <<
			result.tm_sec << std::endl;
		}
		irqcount++;
	}

	std::cout << "Close" << std::endl;
	retval = ioctl(fd, RTC_UIE_OFF);
	read_temp(fd);
	close(fd);
	return 0;
}
