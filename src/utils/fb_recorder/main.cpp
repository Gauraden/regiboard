#include "framebuf.hpp"
#include <string>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sstream>
#include <signal.h>
#include <cstdlib>

static int CreateServerSocket() {
	int serverSock = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in serverAddr;
	serverAddr.sin_family      = AF_INET;
	serverAddr.sin_port        = htons(8080);
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(struct sockaddr));
	listen(serverSock, 4);
	return serverSock;
}

static std::string GetDateTime() {
  time_t     rawtime;
  struct tm *timeinfo;
  char       buffer[80];
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(buffer, 80, "%a, %d %b %Y %X %Z", timeinfo);
  return std::string(buffer);
}

static int GetNumber(char *str) {
  const char *sub_str = strtok(str, "=");
  if (sub_str == 0)
    return -1;
  return atol(sub_str);
}

static void WaitForRequest(int socket, fb::Screen &screen) {
		char receivedStr[1000];
		sockaddr_in clientAddr;
		socklen_t   sin_size = sizeof(struct sockaddr_in);
		int clientSock = accept(socket, (struct sockaddr*)&clientAddr, &sin_size);
		recv(clientSock, receivedStr, 500, 0);
		
		int x = -1;
		int y = -1;
		int s = -1;
		if (std::string(receivedStr, 7) == "GET /?x") {
  		GetNumber(receivedStr);
      x = GetNumber(0);
      y = GetNumber(0);
      s = GetNumber(0);
  		std::cout << "recv: x=" << x << "; y=" << y << "; s=" << s << std::endl;
		}
    fb::PngUserData udata = screen.get_png_udata();
		std::stringstream head;
		head << "HTTP/1.1 200 OK\r\n"
			   << "Connection: close\r\n"
				 << "Content-Length: " << screen.GetFrameSize() << "\r\n"
//				 <<	"Content-Type: image/png\r\n"
				 <<	"Content-Type: image/bmp\r\n"
	       << "\r\n";
		screen.ConvertToBmp(head.str().c_str(), head.str().size());
 		ssize_t sended = send(clientSock, udata.data.get(), udata.offs, 0);
// 		usleep(50000);
//		close(clientSock);
}

static fb::Screen *fb_screen = 0;
void SignalCatcher(int sig_num) {
	if (fb_screen == 0)
		return;
	switch (sig_num) {
		case SIGPWR:
			break;
		case SIGUSR1: {/*
  		ERROR("DEBUG: updating frame...");
			std::stringstream head;
			head << "HTTP/1.1 200 OK\r\n"
				   << "Connection: close\r\n"
					 << "Content-Length: " << fb_screen->GetFrameSize() << "\r\n"
	//				 <<	"Content-Type: image/png\r\n"
					 <<	"Content-Type: image/bmp\r\n"
		       << "\r\n";
			fb_screen->ConvertToBmp(head.str().c_str(), head.str().size());*/
			break;
		}
		case SIGUSR2: {
			break;
		}
		case SIGTERM:
		case SIGINT:
		default:
			break;
	}
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		ERROR("Ошибка " << "укажите директорию для сохранения скриншотов");
	}
	const std::string kOutPath(argv[1]);
	const std::string kImageFile = kOutPath + "/screen.bmp";
	fb::Screen screen;
	screen.BindToFbDev("/dev/fb0");
	const int kSocket = CreateServerSocket();
	fb_screen = &screen;
	signal(SIGUSR1, SignalCatcher);
	while (1) {
		WaitForRequest(kSocket, screen);
//		screen.ConvertToBmp();
//    fb::PngUserData udata = screen.get_png_udata();
//		FILE *bmp_f = fopen(kImageFile.c_str(), "wb");
//		fwrite(udata.data.get(), sizeof(png_byte), udata.offs, bmp_f);
//		fclose(bmp_f);
	}
	return 0;
}
