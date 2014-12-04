#include <stdio.h>
#include <unistd.h>
#include <++DFB/++dfb.h>
#include <directfb.h>
#include <cairo/cairo-directfb.h>
#include "boost/scoped_array.hpp"

static unsigned kScrWidth  = 640;
static unsigned kScrHeight = 480;
static unsigned kScrDepth  = 4;

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

static
void show_rotated_text(cairo_t *ctx, IDirectFBSurface *prim, IDirectFBSurface *buf) {
    if (not ctx || not prim || not buf)
      return;
    
    float angle = 0;
    while (1) {
      cairo_save(ctx);
   	  cairo_set_font_size(ctx, 24);
 	    cairo_rectangle(ctx, 0, 0, 400, 400);
      cairo_set_source_rgb(ctx, 0.0, 0.0, 0.0);
      cairo_fill(ctx);
      
    	cairo_translate(ctx, 200.0, 200.0);
    	cairo_set_source_rgb(ctx, 0.5, 0.5, 1.0);
	    cairo_rotate(ctx, (angle * 0.0174));
     	cairo_translate(ctx, -100.0, -100.0);
	    cairo_show_text(ctx, "WAZZZZAAAAAAP!");
      cairo_restore(ctx);

      prim->Blit(*buf);
      prim->Flip();
      angle += 1;
      usleep(1000000 / 10);
    }
}

static
void show_trend(cairo_t *ctx, IDirectFBSurface *prim, IDirectFBSurface *buf) {
    if (not ctx || not prim || not buf)
      return;
    
    float angle = 0;
    double x=25.6,   y=128.0;
    double x1=102.4, y1=230.4,
           x2=153.6, y2=25.6,
           x3=230.4, y3=128.0;
    while (1) {
      cairo_save(ctx);
 	    cairo_rectangle(ctx, 0, 0, 250, 250);
      cairo_set_source_rgb(ctx, 0.0, 0.0, 0.0);
      cairo_fill(ctx);
      
      cairo_move_to (ctx, x, y);
      cairo_curve_to (ctx, x1, y1 + angle, x2, y2, x3, y3 - angle);
      cairo_set_source_rgb(ctx, 0.2, 1.0, 0.2);
      cairo_set_line_cap (ctx, CAIRO_LINE_CAP_ROUND);
      cairo_set_line_width (ctx, 10.0);
      cairo_stroke (ctx);
      cairo_restore(ctx);

      prim->Blit(*buf);
      prim->Flip();
      angle += 1;
      usleep(1000000 / 10);
    }
}

static
bool check_cairo(IDirectFB *dfb, IDirectFBSurface *primary) {
    boost::scoped_array<unsigned char> v_buf(new unsigned char[kScrWidth * kScrHeight * kScrDepth]);
    IDirectFBSurface      dfb_buf;
    DFBSurfaceDescription dsc;

    dsc.flags       = (DFBSurfaceDescriptionFlags)(DSDESC_WIDTH |
                                                   DSDESC_HEIGHT |
                                                   DSDESC_PIXELFORMAT |
                                                   DSDESC_CAPS |
                                                   DSDESC_PREALLOCATED); 
		dsc.caps        = DSCAPS_NONE;
	  dsc.width       = kScrWidth;
  	dsc.height      = kScrHeight;
	  dsc.pixelformat = DSPF_RGB32;
		dsc.preallocated[0].data  = v_buf.get();
		dsc.preallocated[0].pitch = dsc.width * kScrDepth;
		dsc.preallocated[1].data  = NULL;
		dsc.preallocated[1].pitch = 0;
    dfb_buf = dfb->CreateSurface(dsc);

    const Color kRed(0xFF, 0x00, 0x00, 0xFF);
    const Color kWhite(0xFF, 0xFF, 0xFF, 0xFF);
/*    DFBRegion region;
    region.x1 = 10;
    region.y1 = 0;
    region.x2 = 700;
    region.y2 = 10;
    dfb_buf.SetClip(&region);*/
    kWhite.ApplyTo(dfb_buf);
    Line(Point(10, 9), Point(700, 11)).ApplyTo(dfb_buf);
    primary->Blit(dfb_buf);
    primary->Flip();
    
    std::cout << "Creation of cairo surface..." << std::endl;
    cairo_surface_t *cairo_surface = cairo_image_surface_create_for_data(v_buf.get(),
      CAIRO_FORMAT_ARGB32,
      kScrWidth,
      kScrHeight,
      cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, kScrWidth));

    if (not cairo_surface) {
      std::cout << "cairo_surface is NULL" << std::endl;
      return false;
    }
    std::cout << "Creation of cairo context..." << std::endl;
    cairo_t *cairo_context = cairo_create(cairo_surface);

    if (not cairo_context) {
      std::cout << "cairo_context is NULL" << std::endl;
      return false;
    }
    
    show_rotated_text(cairo_context, primary, &dfb_buf);
//    show_trend(cairo_context, primary, &dfb_buf);
/*
  	cairo_save(cairo_context);
    cairo_rectangle(cairo_context, 10, 10, 100, 100);
    cairo_set_source_rgb(cairo_context, 1.0, 0.0, 1.0);
    cairo_fill(cairo_context);
    cairo_restore(cairo_context);
    */
   	cairo_save(cairo_context);
  	cairo_set_source_rgb(cairo_context, 1, 1, 1);
    cairo_move_to (cairo_context, 10, 15);
    cairo_line_to (cairo_context, 700, 16);
    cairo_set_line_width (cairo_context, 1.0);
    cairo_set_line_cap (cairo_context, CAIRO_LINE_CAP_ROUND);
    cairo_stroke (cairo_context);
    cairo_restore(cairo_context);
    /*
   	cairo_save(cairo_context);
  	cairo_move_to(cairo_context, 50.0, 50.0);
  	cairo_set_source_rgb(cairo_context, 1.0, 1.0, 1.0);
	  cairo_show_text(cairo_context, "Hello World!");
    cairo_restore(cairo_context);
    */	  
    /*
   	cairo_save(cairo_context);
	  cairo_set_source_rgb(cairo_context, 0.0, 1.0, 0.0);
    cairo_set_line_width(cairo_context, 1);
    cairo_move_to(cairo_context, 300.0, 300.0);
    cairo_arc(cairo_context, 300, 300, 50, 0, 2*3.14);
    cairo_stroke(cairo_context);
    cairo_restore(cairo_context);
	  
   	cairo_save(cairo_context);
	  cairo_set_font_size(cairo_context, 24);
  	cairo_move_to(cairo_context, 200.0, 200.0);
  	cairo_set_source_rgb(cairo_context, 0.5, 0.5, 1.0);
	  cairo_rotate(cairo_context, (45*0.0174) );
	  cairo_show_text(cairo_context, "WAZZZZAAAAAAP!");
    cairo_restore(cairo_context);
	  */
    primary->Blit(dfb_buf);
    primary->Flip();
    
    std::cout << "Waiting for 5 seconds..." << std::endl;
    sleep(30);

    cairo_destroy(cairo_context);
    cairo_surface_destroy(cairo_surface);
    return true;
}

static
void CairoMainSurf(IDirectFB *dfb) {
  DFBSurfaceDescription dsc;
	int scr_w,
	    scr_h;
  IDirectFBScreen screen = dfb->GetScreen(0);
  screen.GetSize(&scr_w, &scr_h);
  
  std::cout << "screen: " << scr_w << "x" << scr_h << std::endl;
  boost::scoped_array<unsigned char> v_buf(new unsigned char[scr_w * scr_h * kScrDepth]);
    
  dsc.flags = (DFBSurfaceDescriptionFlags)(DSDESC_CAPS
//                                        | DSDESC_WIDTH
//                                        | DSDESC_HEIGHT
//                                        | DSDESC_PIXELFORMAT
//                                          | DSDESC_PREALLOCATED 
                                          | DBDESC_MEMORY);
//	dsc.caps        = DSCAPS_NONE;
	dsc.caps        = DSCAPS_PRIMARY;
//	dsc.width       = scr_w;
//	dsc.height      = scr_h;
//	dsc.pixelformat = DSPF_RGB32;
	dsc.preallocated[0].data  = v_buf.get();
	dsc.preallocated[0].pitch = dsc.width * kScrDepth;
	dsc.preallocated[1].data  = NULL;
	dsc.preallocated[1].pitch = 0;
  IDirectFBSurface dfb_buf = dfb->CreateSurface(dsc);
  
  cairo_surface_t *cairo_surface = cairo_image_surface_create_for_data(v_buf.get(),
    CAIRO_FORMAT_ARGB32,
    scr_w,
    scr_h,
    cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, scr_w));
  if (not cairo_surface) {
    std::cout << "cairo_surface is NULL" << std::endl;
    return;
  }
  std::cout << "Creation of cairo context..." << std::endl;
  cairo_t *cairo_context = cairo_create(cairo_surface);
  if (not cairo_context) {
    std::cout << "cairo_context is NULL" << std::endl;
    return;
  }
  
 	cairo_save(cairo_context);
	cairo_set_source_rgb(cairo_context, 1, 1, 1);
  cairo_move_to (cairo_context, 10, 15);
  cairo_line_to (cairo_context, 700, 16);
  cairo_set_line_width (cairo_context, 1.0);
  cairo_set_line_cap (cairo_context, CAIRO_LINE_CAP_ROUND);
  cairo_stroke (cairo_context);
  cairo_restore(cairo_context);
  
  dfb_buf.Flip();
  const int kSleep = 5;  
  std::cout << "Waiting for " << kSleep << " seconds..." << std::endl;
  sleep(kSleep);
  
  cairo_destroy(cairo_context);
  cairo_surface_destroy(cairo_surface);
}

int main (int argc, char **argv) {
/*
	IDirectFBSurface      m_primary;
	DFBSurfaceDescription dsc;
	int w, h;
*/
  DirectFB::Init(/*&argc, &argv*/);
  IDirectFB m_dfb = DirectFB::Create();
  CairoMainSurf(&m_dfb);
  return 0;
/*  
  dsc.flags = (DFBSurfaceDescriptionFlags)(DSDESC_CAPS |
//                                           DSDESC_WIDTH |
//                                           DSDESC_HEIGHT |
                                           DSDESC_PIXELFORMAT);
	dsc.caps        = DSCAPS_PRIMARY;
//	dsc.width       = kScrWidth;
//	dsc.height      = kScrHeight;
	dsc.pixelformat = DSPF_RGB24;
  m_primary = m_dfb.CreateSurface(dsc);
  m_primary.GetSize(&w, &h);
    
  check_cairo(&m_dfb, &m_primary);
  */
}
