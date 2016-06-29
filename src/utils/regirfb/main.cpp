#include "framebuf.hpp"
#include <string>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <cstdlib>
#include <stdlib.h>

#include <sys/ipc.h>
#include <sys/msg.h>

#include <boost/asio.hpp>

using boost::asio::ip::tcp;

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

/*
static int CreateServerSocket() {
	int serverSock = socket(AF_INET, SOCK_STREAM, 0);

  int tmp = 1;
  setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(int));

	sockaddr_in serverAddr;
	serverAddr.sin_family      = AF_INET;
	serverAddr.sin_port        = htons(8080);
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(struct sockaddr)) < 0) {
	  std::cout << "Binding was failed!" << std::endl;
	  close(serverSock);
	  return -1;
	}
	listen(serverSock, 4);
	return serverSock;
}
*/

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
  if (sub_str == 0) {
    return -1;
  }
  return atol(sub_str);
}

static std::string JsonPair(const std::string &name, const std::string val) {
  return "\"" + name + "\": \"" + val + "\"";
}

static bool SendDeviceState(int socket) {
  std::fstream proc("/proc/cmdline");
  std::string cmdline_arg;
  std::string display_orientation("horizontal");
  std::string board_id("x.x.x");
  while (proc.good()) {
    proc >> cmdline_arg;
    if (cmdline_arg.find("lcd=") != std::string::npos &&
        cmdline_arg.find("_VERTICAL") != std::string::npos) {
      display_orientation = "vertical";
      continue;
    }
    size_t arg_off = cmdline_arg.find("board=");
    if (arg_off != std::string::npos) {
      board_id = cmdline_arg.substr(arg_off + 6);
      continue;
    }
  }
  proc.close();
  
  std::stringstream packet, json;
  json << "{"
       << "\"display\": {"
         << JsonPair("orientation", display_orientation)
         << "},"
       << "\"board\": {"
         << JsonPair("id", board_id)
         << "},"
       << "}";
  packet << "HTTP/1.1 200 OK\r\n"
         << "Accept-Ranges: bytes\r\n"
		     << "Content-Length: " << (unsigned)json.str().size() << "\r\n"
		     <<	"Content-Type: application/json\r\n"
		     << "Access-Control-Allow-Origin: *\r\n"
         << "\r\n" << json.str();
  ssize_t sended  = 0;
  int     err_num = 0;
  const std::string kPktStr = packet.str();
  const char *resp = kPktStr.c_str();
  while (sended < kPktStr.size()) {
    const ssize_t kResult = send(socket, resp,
                                         kPktStr.size() - sended,
                                         MSG_NOSIGNAL);
    if (kResult < 0) {
      err_num++;
      if (err_num > 3) {
        ERROR("Ошибка при отправки json! sended = " << (long int)sended);
        break;
      }
      continue;
    }
    sended += kResult;
    resp   += kResult;
  }
}

static void WaitForRequest(boost::asio::io_service &io_service, tcp::acceptor &acceptor, fb::Screen &screen) {
    static const std::string kGetCoords("GET /?x");
    static const std::string kGetState("GET /state.json");
    static const std::string kGetFirmware("GET /?firmware=");

    tcp::socket socket(io_service);

    try {
      acceptor.accept(socket);
    } catch(...) {
      return;
    }

		char request[1024] = { 0 };
    const long long kRcvBeg = GetMSec();

    boost::system::error_code error;
    size_t len;
    try {
      len = socket.read_some(boost::asio::buffer(request, 1024), error);
    } catch(...) {
      return;
    }

		//std::cout << "RECV: " << (GetMSec() - kRcvBeg) << " msec;"
		//          << std::endl;

		if (std::string(request, kGetCoords.size()) == kGetCoords && GetNumber(request) >= 0) {
      int x = GetNumber(0);
      int y = GetNumber(0);
      int s = GetNumber(0);
 		  if (s == 1) {
	      SendNewCoords(x, y, s);
	    }
    } else if (std::string(request, kGetState.size()) == kGetState) {
      std::cout << "получен запрос состояния регистратора!" << std::endl;
      SendDeviceState(socket.native_handle());
  		return;
    } else if (std::string(request, kGetFirmware.size()) == kGetFirmware &&
               strlen(request) > kGetFirmware.size() + 5) { // url прошивки уж точно не может быть меньше 5 байт
      // обрежем "мусор" что подставляет браузер за именем файла прошивки (там он другие параметры еще
      //     передает, нам неинтересные)
      const char *sub_str = strtok(request + kGetFirmware.size(), " ");
      if (sub_str) {
        std::stringstream str;
        str << "/etc/init.d/S39gui update '" << std::string(request + kGetFirmware.size()) << "'";
        std::cout << "regirfb: получен запрос прошивки. подготовилась команда на вызов: '"
            << str.str() << "'" << std::endl;
        int nRes = system(str.str().c_str());
        if (nRes == -1)
          std::cout << "regirfb: Error! Did not manage to execute S39gui update!" << std::endl;
      }
      return;
    }
    const long long kBmpBeg = GetMSec();
    screen.SendFrameAsBmp(socket.native_handle());
		//std::cout << "BMP: " << (GetMSec() - kBmpBeg) << " msec;"
		//          << std::endl;
}

static fb::Screen *fb_screen = 0;
static bool        stop_sig  = false;

/*
void SignalCatcher(int sig_num) {
	if (fb_screen == 0)
		return;
	switch (sig_num) {
		case SIGPWR:
			break;
		case SIGUSR1:
			break;
		case SIGUSR2:
			break;
		case SIGTERM:
		case SIGINT:
 	    stop_sig = true;
		default:
		  break;
	}
}
*/

void signal_handler(const boost::system::error_code & err, int signal)
{
 	stop_sig = true;
}

static void NetWork(fb::Screen &screen) {
  boost::asio::io_service io_service;

  boost::asio::signal_set sig(io_service, SIGINT, SIGTERM);
  sig.async_wait(signal_handler);

  tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 8080));

  boost::asio::socket_base::reuse_address option(true);
  acceptor.set_option(option);

  while(not stop_sig) {
    WaitForRequest(io_service, acceptor, screen);
  }
}

int main(int argc, char *argv[]) {

  std::cout << "regirfb (new version)" << std::endl;

	InitIPC();
	fb::Screen screen;
	screen.BindToFbDev("/dev/fb0");
  /*
	const int kSocket = CreateServerSocket();
	if (kSocket < 0)
	  return 1;
	fb_screen = &screen;
	signal(SIGTERM, SignalCatcher);
	signal(SIGINT,  SignalCatcher);
	while (not stop_sig) {
		WaitForRequest(kSocket, screen);
  }
	close(kSocket);
  */
  NetWork(screen);
	TerminateIPC();
	std::cout << "Работа завершена!" << std::endl;
	return 0;
}

