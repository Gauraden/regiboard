#include <stdio.h>
#include <unistd.h>
#include <++DFB/++dfb.h>
#include <directfb.h>
#include <time.h>
#include <string>

#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>

int  uart_handle[11];
char *uart_devs[11] = {"ttyUSB0", "ttyUSB1", "ttyUSB2",
                       "ttyUSB3", "ttyUSB4", "ttyUSB5",
                       "ttyUSB6", "ttyUSB7", "ttyUSB8",
                       "ttyUSB9", "ttyUSB10"};

void open_uarts() {
  for (int i = 0; i < 11; i++) {
    const std::string kDevPath(std::string("/dev/") + uart_devs[i]);
    uart_handle[i] = open(kDevPath.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);//O_RDWR | O_NOCTTY | O_NONBLOCK);
    std::cout << "Opened: " << kDevPath << "; res: " << uart_handle[i] << std::endl;
  }
}

void close_uart() {
  for (int i = 0; i < 10; i++)
    close(uart_handle[i]);
}

void check_ftdi() {
  struct timespec tm;
  tm.tv_sec  = 1;
  tm.tv_nsec = 0;
//  open_uarts();
  while (1) {
    // DEBUG --------------------
    std::cout << "DEBUG: " << __FUNCTION__ << ": " << __LINE__ << ": "
              << "OPENING"
              << std::endl;
    // --------------------------    
    open_uarts();
    /*if (m_events.WaitForEventWithTimeout(0, 0)) {
      while (m_events.GetEvent( DFB_EVENT(&event) )) {
        // DEBUG --------------------
        std::cout << "DEBUG: " << __FUNCTION__ << ": " << __LINE__ << ": "
                  << "GETTING EVENT FROM QUEUE"
                  << std::endl;
        // --------------------------
      }
    }
    */
//    nanosleep(&tm, 0);
    sleep(5);
    close_uart();
    // DEBUG --------------------
    std::cout << "DEBUG: " << __FUNCTION__ << ": " << __LINE__ << ": "
              << "CLOSED"
              << std::endl;
    // --------------------------
    sleep(5);
  }
//  close_uart();
}

class Color {
  public:
    Color(int r, int g, int b, int a): _r(r), _g(g), _b(b), _a(a) {}
    void ApplyTo(IDirectFBSurface *surf) const {
      surf->SetColor(_r, _g, _b, _a);
    }
  private:
    int _r;
    int _g;
    int _b;
    int _a;
};

struct Point {
  Point(): x(0), y(0) {}
  Point(int _x, int _y): x(_x), y(_y) {}
  int x;
  int y;
};

class Line {
  public:
    Line(const Point &p1, const Point &p2) {
      _pts[0] = p1;
      _pts[1] = p2;
    }
    void ApplyTo(IDirectFBSurface *surf) const {
      surf->DrawLine(_pts[0].x, _pts[0].y,
                     _pts[1].x, _pts[1].y);
    }
  private:
    Point _pts[2];
};

void check_aliasing(IDirectFBSurface *primary) {
    /*
    primary->SetColor(0xFF, 0x00, 0x00, 0xFF);
    primary->DrawLine(30, 10, 720, 300);
    
    DFBSurfaceRenderOptions render_options = DSRO_ANTIALIAS;
    primary->SetDrawingFlags(DSDRAW_BLEND);        
    primary->SetRenderOptions(render_options);
    primary->SetColor(0xFF, 0xFF, 0xFF, 0xFF);
    primary->DrawLine(10, 10, 700, 300);
    */
    DFBSurfaceRenderOptions render_options = DSRO_ANTIALIAS;
    primary->SetDrawingFlags(DSDRAW_BLEND);        
    primary->SetRenderOptions(render_options);
    const Color kRed(0xFF, 0x00, 0x00, 0xFF);
    const Color kWhite(0xFF, 0xFF, 0xFF, 0xFF);
    DFBRegion region;
    region.x1 = 10;
    region.y1 = 0;
    region.x2 = 700;
    region.y2 = 10;
    primary->SetClip(&region);
//    kRed.ApplyTo(primary);
//    Line(Point(30, 10), Point(720, 11)).ApplyTo(primary);
    kWhite.ApplyTo(primary);
    Line(Point(700, 11), Point(10, 9)).ApplyTo(primary);
    primary->Flip();
    sleep(15);
}

int main (int argc, char **argv) {
	IDirectFB             m_dfb;
	IDirectFBSurface      m_primary;
	IDirectFBEventBuffer  m_events;
  DFBInputEvent         event;
	DFBSurfaceDescription dsc;
	int w, h;
	
  DirectFB::Init(/*&argc, &argv*/);
  m_dfb     = DirectFB::Create();
  dsc.flags = DSDESC_CAPS;
	dsc.caps  = DSCAPS_PRIMARY;
  m_primary = m_dfb.CreateSurface(dsc);
  m_events  = m_dfb.CreateInputEventBuffer(DICAPS_ALL, DFB_TRUE);
  m_primary.GetSize(&w, &h);
//  check_ftdi();
  check_aliasing(&m_primary);
}
