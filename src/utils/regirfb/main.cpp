#include "framebuf.hpp"
#include <string>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <cstdlib>

#include <sys/ipc.h>
#include <sys/msg.h>

typedef struct message {
  long mtype;
	char cmd[128];
	char result[32];
} mess_t;

int nres;
int qid;
key_t msgkey;
mess_t sent, received;
const static int mes_length = sizeof(mess_t) - sizeof(long);

static void InitIPC() {
	msgkey = ftok("/home/regigraf",'a');
	qid = msgget(msgkey, IPC_CREAT | 0660);
	std::cout << "InitIPC: QID = " << qid << " mtype version" << std::endl;
}

static void TerminateIPC() {
	msgctl(qid, IPC_RMID, 0);
	std::cout << "TerminateIPC: QID = " << qid << std::endl;
}

static bool SendNewCoords(int x, int y, int what) {
  sent.mtype = 1; // always one
	snprintf(sent.cmd, 128, "got coords; x=%i y=%i what=%i", x, y, what);
	snprintf(sent.result, 5, "%s", "OK");
	nres = msgsnd(qid, &sent, mes_length, 0);
	if (nres == -1) {
    ERROR("Couldn't msgsng!");
	  return false;
	}
	return true;
}

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
		char request[1024];
		sockaddr_in clientAddr;
		socklen_t   sin_size = sizeof(struct sockaddr_in);
		int clientSock = accept(socket, (struct sockaddr*)&clientAddr, &sin_size);
		recv(clientSock, request, 512, 0);
		if (std::string(request, 7) == "GET /?x" && GetNumber(request) >= 0) {
      int x = GetNumber(0);
      int y = GetNumber(0);
      int s = GetNumber(0);
 		  if (s == 1)
	      SendNewCoords(x, y, s);
    }
    screen.SendFrameAsBmp(clientSock);
    usleep(10000);
		close(clientSock);
}

static fb::Screen *fb_screen = 0;
static bool        stop_sig  = false;

void SignalCatcher(int sig_num) {
	if (fb_screen == 0)
		return;
	switch (sig_num) {
		case SIGPWR:
			break;
		case SIGUSR1: {
			break;
		}
		case SIGUSR2: {
			break;
		}
		case SIGTERM:
		case SIGINT:
		  stop_sig = true;
		default:
			break;
	}
}

int main(int argc, char *argv[]) {
	InitIPC();
	fb::Screen screen;
	screen.BindToFbDev("/dev/fb0");
	const int kSocket = CreateServerSocket();
	fb_screen = &screen;
	signal(SIGTERM, SignalCatcher);
	signal(SIGINT,  SignalCatcher);
	while (not stop_sig)
		WaitForRequest(kSocket, screen);
	TerminateIPC();
	return 0;
}
