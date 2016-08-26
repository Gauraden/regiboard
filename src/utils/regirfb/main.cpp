#include <string>
#include <sstream>
#include <fstream>
#include <boost/program_options.hpp>
#include <webapp/webapp_lib.hpp>
#include "ipc.hpp"
#include "framebuf.hpp"

static std::string JsonPair(const std::string &name, const std::string val) {
  return "\"" + name + "\": \"" + val + "\"";
}

static std::string GetDeviceInfo() {
  std::fstream proc("/proc/cmdline");
  std::string  cmdline_arg;
  std::string  display_orientation("horizontal");
  std::string  board_id("x.x.x");
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
  std::stringstream json;
  json << "{"
       << "\"display\": {"
         << JsonPair("orientation", display_orientation)
         << "},"
       << "\"board\": {"
         << JsonPair("id", board_id)
         << "}"
       << "}";
  return json.str();
}

static bool UpdateFirm(const std::string &url) {
  if (url.size() == 0) {
    return false;
  }
  std::stringstream str;
  str << "/etc/init.d/S39gui update '" << url << "'";
  std::cout << "regirfb: получен запрос прошивки, команда: '"
            << str.str() << "' - ";
  const int kRes = system(str.str().c_str());
  if (kRes == -1) {
    std::cout << "ошибка" << std::endl;
    return false;
  }
  std::cout << "выполнена" << std::endl;
  return true;
}

fb::Screen  screen;
std::string static_dir;

bool InitUI(const webapp::ProtocolHTTP::Uri::Path &path,
            const webapp::ProtocolHTTP::Request   &request,
                  webapp::ProtocolHTTP::Response  *response) {
  typedef webapp::ProtocolHTTP::Content::Type CType;
  response->UseFile(CType(CType::kText, "html"), static_dir + "/index.html");
  return true;
}

bool Update(const webapp::ProtocolHTTP::Uri::Path &path,
            const webapp::ProtocolHTTP::Request   &request,
                  webapp::ProtocolHTTP::Response  *response) {
  if (path.size() == 0) {
    return false;
  }
  if (path.at(0) == "mouse") {
    int x     = request.Get("x", (int)0);
    int y     = request.Get("y", (int)0);
    int state = request.Get("s", (int)0);
	  if (state == 1 && not SendNewCoords(x, y, state)) {
	    std::cout << "DEBUG: ошибка при отправке координат: "
	              << x << "x" << y
	              << std::endl;      
    }
  }
  if (path.at(0) == "firmware") {
    UpdateFirm(request.Get("from"));
  }
  response->SetHeader(webapp::ProtocolHTTP::k200);
  return true;
}

bool SendState(const webapp::ProtocolHTTP::Uri::Path &path,
               const webapp::ProtocolHTTP::Request   &request,
                     webapp::ProtocolHTTP::Response  *response) {
  response->SetBody(GetDeviceInfo());
  return true;
}

bool SendFrame(const webapp::ProtocolHTTP::Uri::Path &path,
               const webapp::ProtocolHTTP::Request   &request,
                     webapp::ProtocolHTTP::Response  *response) {
  if (screen.PrepareBMPFrame()) {
    response->SetHeader("bmp");
    response->UseCacheControl().NoStore();
    response->SetBody(screen.GetFrameData(), screen.GetFrameSize());
    return true;
  }
  return false;
}

static bool ParseArguments(int ac, char **av) {
  namespace po = boost::program_options;
  po::options_description desc("Программа для удалённого управления устройством"
                               ", по протоколу HTTP");
  desc.add_options()
    ("help",   "вывод описания аргументов")
    ("static", po::value<std::string>()->default_value("/var/www/data"),
               "путь к директории со статичными данными");
  po::variables_map vm;
  po::store(po::parse_command_line(ac, av, desc), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return false;
  }
  if (vm.count("static")) {
    static_dir = vm["static"].as<std::string>();
  }
  return true;
}

class Inspector : public webapp::Inspector {
  public:
    Inspector(): webapp::Inspector() {}
    virtual ~Inspector() {}

    virtual void RegisterError(const std::string &msg, const Error &error) {
      // do nothing
    }
    
    virtual void RegisterMessage(const std::string &msg) {
      // do nothing
    }
};

int main(int argc, char *argv[]) {
  if (not ParseArguments(argc, argv)) {
    return 0;
  }
	if (not screen.BindToFbDev("/dev/fb0")) {
	  std::cout << "Ошибка: не удалось открыть устройство fb0"
	            << std::endl;
	  return 1;
	}
	if (not InitIPC()) {
	  std::cout << "Ошибка: не удалось обнаружить очередь сообщений"
	            << std::endl;
	  return 1;
	}
  webapp::ProtocolHTTP::Router::Ptr router = webapp::ProtocolHTTP::Router::Create();
  webapp::ServerHttp srv(router);
  router->AddDirectoryFor("/static", static_dir);
  router->AddHandlerFor("/",                   InitUI);
  router->AddHandlerFor("/action/update",      Update);
  router->AddHandlerFor("/request/state.json", SendState);
  router->AddHandlerFor("/request/frame.bmp",  SendFrame);
  srv.BindTo(0x00000000, 80, webapp::Inspector::Ptr(new Inspector()));
  do {
    srv.Run();
  } while (1);
	TerminateIPC();
	return 0;
}

