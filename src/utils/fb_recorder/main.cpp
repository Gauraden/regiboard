#include "framebuf.hpp"
#include <string>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[]) {
	if (argc < 2) {
		ERROR("Ошибка " << "укажите директорию для сохранения скриншотов");
	}
	const std::string kOutPath(argv[1]);
	fb::Screen screen;
	screen.BindToFbDev("/dev/fb0");
	while (1) {
	  timespec start;
	  clock_gettime(CLOCK_MONOTONIC, &start);
		screen.SaveToPngFile((kOutPath + "/screen.png").c_str());
		timespec end;
		clock_gettime(CLOCK_MONOTONIC, &end);
		ERROR("Debug: " << (unsigned)((end.tv_nsec - start.tv_nsec) / 1000000) << " msec");
		sleep(1);
	}
	return 0;
}
