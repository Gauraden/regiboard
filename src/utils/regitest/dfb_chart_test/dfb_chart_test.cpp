#include <stdio.h>
#include "dfb_cairo.hpp"
#include <string>
#include <list>
#include <cmath>
#include <boost/shared_ptr.hpp>
#include "../regitest_time.hpp"

static int screen_w = 1;
static int screen_h = 1;

static const Color kBgColorCairo(180, 180, 180, 255);
static const Color kBgColorDfb(100, 100, 100, 255);
static const Color kTrendColor(255, 255, 255, 255);

class Trend {
  public:
    typedef int16_t                  Offs;
    typedef boost::shared_ptr<Trend> Ptr;
    
    static const Offs   kInvalidOffs  = -1;
    static const double kMaxDeviation = 0.1; // градусы
        
    struct Vertex {
      typedef std::list<Vertex> List;
      
      Vertex(): x(kInvalidOffs), y(kInvalidOffs) {}
      Vertex(Offs _x, Offs _y): x(_x), y(_y) {}
      bool IsValid() const {
        return (x != kInvalidOffs && y != kInvalidOffs);
      }
      Offs x;
      Offs y;
    };
    
    Trend(Offs limit, Offs approxim)
        : _offs_limit(limit), _approxim(approxim), _length(0) {
    }
    
    void PushPoint(const Offs &point) {
      Vertex &last_v = GetLastVertex(point);
      // определение направления вектора
      Vertex new_dir = VecSub(Vertex(last_v.x, point), last_v);
      double angle   = VecAngle(_direction, new_dir);
      if (angle != angle) // NaN
        angle = kMaxDeviation + 1;
      _direction = new_dir;
      if (angle <= kMaxDeviation && _vertices.size() > 1 && last_v.x < _approxim) {
        last_v.x++;
      } else {
        _vertices.push_front(Vertex(1, point));
      }
      _length++;
      // удаление старой точки
      if (_length > _offs_limit) {
        Vertex &oldest = _vertices.back();
        _length--;
        if (oldest.x > 0)
          oldest.x--;
        if (oldest.x == 0)
          _vertices.pop_back();
      }
    }
    
    Clock::USec DrawAt(UniSurface &surf) const {
      if (_vertices.size() == 0)
        return 0;
      const Clock kStarted;
      cairo_t *ctx = surf.get_cairo_ctx();
      cairo_save(ctx);
      // отрисовка
      Vertex::List::const_iterator v_it = _vertices.begin();
      const Vertex &kLastV = _vertices.front();
      Offs x_offs = screen_w + _approxim;
      cairo_move_to(ctx, x_offs, kLastV.y);
      for (; v_it != _vertices.end(); v_it++) {
        x_offs -= v_it->x;
        cairo_line_to(ctx, x_offs, v_it->y);
      }
      cairo_stroke(ctx);
      cairo_restore(ctx);
      return Clock().GetDiff(kStarted);
    }
    
    Clock::USec DrawAt(IDirectFBSurface &surf) const {
      if (_vertices.size() == 0)
        return 0;
      static const unsigned kVerOffs = screen_h / 2;
      const Clock kStarted;
      // отрисовка
      Vertex::List::const_iterator v_it = _vertices.begin();
      const Vertex &kLastV = _vertices.front();
      Offs x_offs = screen_w + _approxim;
      Offs y_offs = 0;
      for (; v_it != _vertices.end(); v_it++) {
        const Offs old_x = x_offs;
        x_offs -= v_it->x;
        surf.DrawLine(old_x, kVerOffs + y_offs, x_offs, kVerOffs + v_it->y);
        y_offs = v_it->y;
      }
      return Clock().GetDiff(kStarted);
    }
  private:
    Vertex& GetLastVertex(const Offs &new_point) {
      if (_vertices.size() == 0) {
        _direction = Vertex(1, new_point);
        _vertices.push_front(_direction);
      }
      return _vertices.front();
    }
    
    double VecLength(const Vertex &to) const {
      return sqrt((double)(to.x * to.x) + (double)(to.y * to.y));
    }
    
    Vertex VecSub(const Vertex &vec0, const Vertex &vec1) const {
      return Vertex(vec0.x - vec1.x, vec0.y - vec1.y);
    }
    
    Vertex VecAdd(const Vertex &vec0, const Vertex &vec1) const {
      return Vertex(vec0.x + vec1.x, vec0.y + vec1.y);
    }
    
    Offs VecScalMul(const Vertex &vec0, const Vertex &vec1) const {
      return ((vec0.x * vec1.x) + (vec0.y * vec1.y));
    }
    
    double VecAngle(const Vertex &vec0, const Vertex &vec1) const {
      return acos(((double)VecScalMul(vec0, vec1)) /
                  (VecLength(vec0) * VecLength(vec1)));
    }
  
    Offs         _offs_limit;
    Offs         _approxim;
    Vertex::List _vertices;
    Vertex       _direction;
    size_t       _length;
};

class Chart {
  public:
    Chart(Trend::Offs limit): _limit(limit), _angle(0), _bak_offs(0) {}
    
    void AddTrend(Trend::Offs approxim) {
      _trends.push_back(Trend::Ptr(new Trend(_limit, approxim)));
    }
    
    void PushPoint(const Trend::Offs &point) {
      Trends::iterator t_it = _trends.begin();
      for (; t_it != _trends.end(); ++t_it) {
        (*t_it)->PushPoint(point);
      }
    }
    
    void PushHarmonicVal(double multiplier) {
      Trends::iterator t_it = _trends.begin();
      double a_offs  = 0.;
      double mul_div = 1;
      for (; t_it != _trends.end(); ++t_it) {
        (*t_it)->PushPoint(GetHarmonicVal(multiplier / mul_div, 0));
        a_offs  += 45.;
        mul_div += 1.;
      }
    }
    
    Clock::USec DrawAt(UniSurface &surf, IDirectFBSurface &dfb) const {
      cairo_t *ctx = surf.get_cairo_ctx();
      // фон
      cairo_rectangle(ctx, 0, 0, screen_w, screen_h);
      surf.UseColorRGB(kBgColorCairo);
      cairo_fill(ctx);
      // отрисовка
      cairo_translate(ctx, 0, screen_h / 2);
      Clock::USec            usec_sum = 0;
      Trends::const_iterator t_it     = _trends.begin();
      for (; t_it != _trends.end(); ++t_it) {
        surf.UseColorRGBA(kTrendColor);
        usec_sum += (*t_it)->DrawAt(surf);
      }
      // блитинг
      const Clock kBlitTm;
      dfb.Blit(surf.get_dfb_surf());
      const Clock::USec kBlited = Clock().GetDiff(kBlitTm);
      // флипинг
      const Clock::USec kFliped = Flip(dfb);
      /*
      std::cout << "DEBUG: " << __func__ << ": "
                << "draw: " << (usec_sum / _trends.size()) << " usec; "
                << "flip: " << kFliped  << " usec; "
                << "blit: " << kBlited  << " usec; "
                << std::endl;
                */
      return (usec_sum + kFliped + kBlited);
    }

    Clock::USec DrawTrendsAt(IDirectFBSurface &surf) const {
      // фон
      kBgColorDfb.ApplyTo(surf);
      surf.FillRectangle(0, 0, screen_w, screen_h);
      // отрисовка
      Clock::USec            usec_sum = 0;
      Trends::const_iterator t_it     = _trends.begin();
      for (; t_it != _trends.end(); ++t_it) {
        kTrendColor.ApplyTo(surf);
        usec_sum += (*t_it)->DrawAt(surf);
      }
      return usec_sum;
    }
    
    Clock::USec DrawAt(IDirectFBSurface &surf) const {
      Clock::USec usec_sum = DrawTrendsAt(surf);
      // флипинг
      const Clock::USec kFliped = Flip(surf);
      return (usec_sum + kFliped);
    }
    
    Clock::USec DrawBakAt(IDirectFBSurface &surf) {
      if (_bak_offs == 0)
        _bak_offs = screen_w - 1;
      // отрисовка
      Clock::USec            usec_sum = 0;
      Trends::const_iterator t_it     = _trends.begin();
      const Clock kBlitTm;
      _bak_surf.Blit(_bak_surf, NULL, -1, 0);
      surf.Blit(_bak_surf);
      usec_sum = Clock().GetDiff(kBlitTm);
      // флипинг
      const Clock::USec kFliped = Flip(surf);
      _bak_offs--;
      return (usec_sum + kFliped);
    }
    
    Clock::USec Flip(IDirectFBSurface &surf) const {
      const Clock kStarted;
      surf.Flip(0, (DFBSurfaceFlipFlags)(DSFLIP_WAIT | DSFLIP_ONSYNC)); // DSFLIP_NONE
      return Clock().GetDiff(kStarted);
    }
    
    Trend::Offs GetHarmonicVal(double multiplier, double a_offs) {
      static const double kRadRation = M_PI / 180.0;
      const Trend::Offs kVal = (Trend::Offs)(sin((double)(_angle + a_offs) * 
                                             kRadRation) * multiplier);
      _angle++;
      if (_angle > 360)
        _angle = 0;
      return kVal;
    }
    
    void SetupBakSurf(IDirectFB &dfb) {
      // создание поверхности
      DFBSurfaceDescription dsc;
	    dsc.caps = (DFBSurfaceCapabilities)(0x0
	                                       | DSCAPS_SYSTEMONLY
	                                       //| DSCAPS_SUBSURFACE
	                                       );
      dsc.flags = (DFBSurfaceDescriptionFlags)( DSDESC_CAPS
                                              | DSDESC_WIDTH
                                              | DSDESC_HEIGHT
                                              | DSDESC_PIXELFORMAT
                                              );
	    dsc.pixelformat = DSPF_RGB32;
      dsc.width       = screen_w;
      dsc.height      = screen_h;
      _bak_surf       = dfb.CreateSurface(dsc);
      DrawTrendsAt(_bak_surf);
    }
  private:
    typedef std::list<Trend::Ptr> Trends;
    
    Chart();
    
    Trend::Offs      _limit;
    Trends           _trends;
    uint16_t         _angle;
    uint16_t         _bak_offs;
    IDirectFBSurface _bak_surf;
};
// TESTS -----------------------------------------------------------------------
static const size_t kFilloutRepeat = 10;
  
static Clock::USec TestCairoScreenFillout(UniSurface       &surf,
                                          IDirectFBSurface &dfb_surf) {
  cairo_t *ctx = surf.get_cairo_ctx();
  const Clock kStarted;
  for (size_t i = 0; i < kFilloutRepeat; i++) {
    cairo_save(ctx);
    cairo_rectangle(ctx, 0, 0, screen_w, screen_h);
    surf.UseColorRGB(kBgColorCairo);
    cairo_fill(ctx);
    cairo_restore(ctx);
  }
  const Clock::USec kDrawTm = kStarted.MeasureAndPrint(__func__, "draw", kFilloutRepeat);
  const Clock kFlip;
  surf.FlipTo(dfb_surf);
  const Clock::USec kFlipTm = kStarted.MeasureAndPrint(__func__, "flip");  
  return kDrawTm + kFlipTm;
}

static Clock::USec TestDfbScreenFillout(IDirectFBSurface &surf) {
  const Clock kStarted;
  for (size_t i = 0; i < kFilloutRepeat; i++) {
    kBgColorDfb.ApplyTo(surf);
    surf.FillRectangle(0, 0, screen_w, screen_h);
  }
  const Clock::USec kDrawTm = kStarted.MeasureAndPrint(__func__, "draw", kFilloutRepeat);
  const Clock kFlip;
  surf.Flip();
  const Clock::USec kFlipTm = kStarted.MeasureAndPrint(__func__, "flip");
  return kDrawTm + kFlipTm;
}

static Clock::USec TestCairoChart(UniSurface       &surf,
                                  IDirectFBSurface &dfb,
                                  const Chart      &chart) {
  cairo_t *ctx = surf.get_cairo_ctx();
  cairo_save(ctx);
  // рисуем графики
  const Clock::USec kLeft = chart.DrawAt(surf, dfb);
  cairo_restore(ctx);
  std::cout << "\t * " << __func__ << ": "
            << "left = " << (unsigned)kLeft << " usec;"
            << std::endl;
  return kLeft;
}

static Clock::USec TestCairoScrollChart(UniSurface       &surf,
                                        IDirectFBSurface &dfb,
                                        Chart            *chart,
                                        int               fps) {
  static const int      k24FpsDelay  = 1000000 / fps;
  static const uint16_t kTotalFrames = fps * 20;
  const Trend::Offs     kVerOffs     = screen_h / 2;
  cairo_t *ctx = surf.get_cairo_ctx();
  Clock::USec left_sum = 0;
  for (uint16_t num = 0; num < kTotalFrames; num++) {
    chart->PushHarmonicVal(kVerOffs);
    cairo_save(ctx);
    // защищаем область отрисовки
    cairo_new_path(ctx);
    cairo_rectangle(ctx, 0, 0, screen_w, screen_h);
    cairo_close_path(ctx);
    cairo_clip(ctx);
    // рисуем графики
    left_sum += chart->DrawAt(surf, dfb);
    cairo_restore(ctx);
    usleep(k24FpsDelay);
  }
  const Clock::USec kAverRes = left_sum / kTotalFrames;
  std::cout << "\t * " << __func__ << ": "
          << "aver left = " << (unsigned)kAverRes << " usec;"
          << std::endl;
  return kAverRes;
}

static Clock::USec TestDfbChart(IDirectFBSurface &surf, const Chart &chart) {
  const Clock::USec kLeft = chart.DrawAt(surf);
  std::cout << "\t * " << __func__ << ": "
            << "left = " << (unsigned)kLeft << " usec;"
            << std::endl;
  return kLeft;
}

static Clock::USec TestDfbScrollChart(IDirectFBSurface &dfb,
                                      Chart            *chart,
                                      int               fps) {
  static const int      k24FpsDelay  = 1000000 / fps;
  static const uint16_t kTotalFrames = fps * 20;
  const Trend::Offs     kVerOffs     = screen_h / 2;
  Clock::USec left_sum = 0;
  for (uint16_t num = 0; num < kTotalFrames; num++) {
    chart->PushHarmonicVal(kVerOffs);
    // защищаем область отрисовки
    // ...
    // рисуем графики
    left_sum += chart->DrawAt(dfb);
    usleep(k24FpsDelay);
  }
  const Clock::USec kAverRes = left_sum / kTotalFrames;
  std::cout << "\t * " << __func__ << ": "
          << "aver left = " << (unsigned)(kAverRes) << " usec;"
          << std::endl;
  return kAverRes;
}

static Clock::USec TestDfbBlitScrollChart(IDirectFBSurface &dfb,
                                          Chart            *chart,
                                          int               fps) {
  static const int      k24FpsDelay  = 1000000 / fps;
  static const uint16_t kTotalFrames = fps * 20;
  const Trend::Offs     kVerOffs     = screen_h / 2;
  Clock::USec left_sum = 0;
  for (uint16_t num = 0; num < kTotalFrames; num++) {
    chart->PushHarmonicVal(kVerOffs);
    // защищаем область отрисовки
    // ...
    // рисуем графики
    left_sum += chart->DrawBakAt(dfb);
    usleep(k24FpsDelay);
  }
  const Clock::USec kAverRes = left_sum / kTotalFrames;
  std::cout << "\t * " << __func__ << ": "
          << "aver left = " << (unsigned)(kAverRes) << " usec;"
          << std::endl;
  return kAverRes;
}
// -----------------------------------------------------------------------------
std::string GetAccelerationInfo(const DFBAccelerationMask &mask) {
  std::string buf;
  if (mask & DFXL_NONE)
    buf += "none ";
  if (mask & DFXL_FILLRECTANGLE)
    buf += "fill_rect ";
 	if (mask & DFXL_DRAWRECTANGLE)
    buf += "draw_rect ";
 	if (mask & DFXL_DRAWLINE)
    buf += "draw_line ";
 	if (mask & DFXL_FILLTRIANGLE)
    buf += "fill_triangle ";
 	if (mask & DFXL_FILLTRAPEZOID)
    buf += "fill_trapez ";
 	if (mask & DFXL_BLIT)
    buf += "blit ";
 	if (mask & DFXL_STRETCHBLIT)
    buf += "strech_blit ";
 	if (mask & DFXL_TEXTRIANGLES)
    buf += "tex_triangle ";
 	if (mask & DFXL_BLIT2)
    buf += "blit2 ";
 	if (mask & DFXL_DRAWSTRING)
    buf += "draw_str ";
 	if (mask & DFXL_ALL)
    buf += "all ";
 	if (mask & DFXL_ALL_DRAW)
    buf += "all_draw ";
 	if (mask & DFXL_ALL_BLIT)
    buf += "all_blit ";
  return buf;
}

static void GetDfbInfo(IDirectFB &dfb) {
  DFBGraphicsDeviceDescription inf;
  dfb.GetDeviceDescription(&inf);
  std::cout << "DirectFB info:\n"
            << "\t - name        : " << inf.name << ";\n"
            << "\t - vendor      : " << inf.vendor << ";\n"
            << "\t - driver: \n"
            << "\t\t - name  : " << inf.driver.name << ";\n"
            << "\t\t - vendor: " << inf.driver.vendor << ";\n"
            << "\t\t - ver   : " << inf.driver.major << "." << inf.driver.minor << ";\n"
            << "\t - video_memory: " << inf.video_memory << " byte;\n"
            << "\t - acceleration: " << GetAccelerationInfo(inf.acceleration_mask) << ";\n"
            << std::hex
            << "\t - blitting    : 0x" << inf.blitting_flags << ";\n"
            << "\t - drawing     : 0x" << inf.drawing_flags << ";\n"
            << std::dec
            << std::endl;
  IDirectFBDisplayLayer disp = dfb.GetDisplayLayer(DLID_PRIMARY);
  DFBDisplayLayerConfig disp_cnf;
  disp.SetCooperativeLevel(DLSCL_ADMINISTRATIVE);
  disp.GetConfiguration(&disp_cnf);
  disp_cnf.flags        = (DFBDisplayLayerConfigFlags)(0x0
                                                      | DLCONF_OPTIONS
                                                      | DLCONF_SURFACE_CAPS
                                                      | DLCONF_BUFFERMODE
                                                      );
  disp_cnf.surface_caps = (DFBSurfaceCapabilities)(0x0
                                                  //| DSCAPS_TRIPLE
                                                  | DSCAPS_DOUBLE
                                                  | DSCAPS_PRIMARY
                                                  );
//  disp_cnf.options      = (DFBDisplayLayerOptions)(DLOP_FLICKER_FILTERING);
  disp_cnf.buffermode   = (DFBDisplayLayerBufferMode)(0x0
                                                     //| DLBM_BACKSYSTEM
                                                     | DLBM_FRONTONLY
                                                     //| DLBM_BACKVIDEO
                                                     );
  disp.SetConfiguration(disp_cnf);
//  disp.SetCooperativeLevel(DLSCL_EXCLUSIVE);
  std::cout << "DFB display info: \n"
            << std::hex
            << "\t - buffer mode : 0x" << disp_cnf.buffermode << ";\n"
            << "\t - options     : 0x" << disp_cnf.options << ";\n"
            << "\t - capabilities: 0x" << disp_cnf.surface_caps << ";\n"
            << std::dec
            << std::endl;
}

static IDirectFBSurface SetupDfbSurface(IDirectFB &dfb) {
  // вывод инфы о directFB
  GetDfbInfo(dfb);
  // создание поверхности
  DFBSurfaceDescription dsc;
	dsc.caps = (DFBSurfaceCapabilities)( DSCAPS_PRIMARY
	                                   | DSCAPS_FLIPPING
	                                   | DSCAPS_SYSTEMONLY
	                                   );
  dsc.flags = (DFBSurfaceDescriptionFlags)( DSDESC_CAPS
                                          | DSDESC_PIXELFORMAT
                                          );
	dsc.pixelformat = DSPF_RGB32;
  return dfb.CreateSurface(dsc);
}

static void SetupUniSurface(IDirectFB *dfb, UniSurface *surf) {
  if (surf == 0 || dfb == 0)
    return;
  IDirectFBScreen screen = dfb->GetScreen(0);
  screen.GetSize(&screen_w, &screen_h);
  surf->Resize(screen_w, screen_h);
}

static void FillTheChart(Chart *chart, int amount) {
  if (chart == 0 || amount < 0)
    return;
  for (int i = 0; i < amount && i < screen_w; ++i) {
    chart->PushHarmonicVal(screen_h / 2);
  }
}

static const Trend::Offs kChartMix[]  = { 1,  2,  5, 10};
static const Trend::Offs kChart1[]    = { 1,  1,  1,  1};
static const Trend::Offs kChart2[]    = { 2,  2,  2,  2};
static const Trend::Offs kChart3[]    = { 3,  3,  3,  3};
static const Trend::Offs kChart5[]    = { 5,  5,  5,  5};
static const Trend::Offs kChart10[]   = {10, 10, 10, 10};

static const Trend::Offs kChartDos[]   = {2, 2};
static const Trend::Offs kChartOche[]  = {2, 2, 2, 2, 2, 2, 2, 2};
static const Trend::Offs kChartOche5[] = {2, 2, 2, 2, 2, 2, 2, 2};

int main (int argc, char **argv) {
  DirectFB::Init();
  IDirectFB        dfb      = DirectFB::Create();
  IDirectFBSurface dfb_surf = SetupDfbSurface(dfb);
  UniSurface uni_surf(dfb);
  SetupUniSurface(&dfb, &uni_surf);
  // создание трендов
  Chart chart(screen_w + 10);
  const Trend::Offs *kChartConf = kChartOche5;
  const size_t       kTrends    = sizeof(kChartOche5) / sizeof(Trend::Offs);
  for (size_t conf_id = 0; conf_id < kTrends; conf_id++) {
    chart.AddTrend(kChartConf[conf_id]);
  }
  FillTheChart(&chart, screen_w);
  chart.SetupBakSurf(dfb);
  // тесты
  TestCairoScreenFillout(uni_surf, dfb_surf);
  TestDfbScreenFillout(dfb_surf);
  TestCairoChart(uni_surf, dfb_surf, chart);
//  TestCairoScrollChart(uni_surf, dfb_surf, &chart, 10);
  TestDfbChart(dfb_surf, chart);
//  TestDfbScrollChart(dfb_surf, &chart, 10);
  TestDfbBlitScrollChart(dfb_surf, &chart, 10);
  return 0;
}
