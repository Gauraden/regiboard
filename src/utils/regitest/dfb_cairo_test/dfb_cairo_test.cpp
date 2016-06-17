#include <stdio.h>
#include <unistd.h>
#include <++DFB/++dfb.h>
#include <directfb.h>
#include <cairo/cairo-directfb.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
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
void show_svg(IDirectFB         *dfb,
              cairo_t           *ctx,
              IDirectFBSurface  *prim,
              IDirectFBSurface  *buf,
              const std::string &svg_file) {
  if (not dfb || not ctx || not prim || not buf) {
    return;
  }
  std::cout << "SVG: " << svg_file << std::endl;

  RsvgDimensionData dim;  
  RsvgHandle      *handle;
  cairo_surface_t *cairo_surf;
  cairo_t         *cr;
  GError          *error = NULL;
  boost::scoped_array<uint8_t> buffer;
  // загрузки пиктограммы
  handle = rsvg_handle_new_from_file(svg_file.c_str(), &error);
  rsvg_handle_get_dimensions(handle, &dim);
  static const unsigned kDepth = 4;
  static const unsigned kScale = 3;
  dim.width  *= kScale;
  dim.height *= kScale;
  DFBSurfaceDescription dsc;
  dsc.flags       = (DFBSurfaceDescriptionFlags)(DSDESC_WIDTH |
                                                 DSDESC_HEIGHT |
                                                 DSDESC_PIXELFORMAT |
                                                 DSDESC_CAPS |
                                                 DSDESC_PREALLOCATED
                                                 );
  dsc.caps        = DSCAPS_NONE;
  dsc.width       = dim.width;
  dsc.height      = dim.height;
  dsc.pixelformat = DSPF_ARGB;
  const uint32_t kBufSize = dim.width * dim.height * kDepth;
  buffer.reset(new uint8_t[kBufSize]);
  memset(buffer.get(), 0, kBufSize);

  dsc.preallocated[0].data  = buffer.get();
  dsc.preallocated[0].pitch = dsc.width * kDepth;
  dsc.preallocated[1].data  = NULL;
  dsc.preallocated[1].pitch = 0;
  IDirectFBSurface dfb_obj;
  dfb_obj = dfb->CreateSurface(dsc);
// variant 0
  int rowstride = dim.width * kDepth;
  cairo_surf = cairo_image_surface_create_for_data (buffer.get(),
                                                 CAIRO_FORMAT_ARGB32,
                                                 dim.width, 
                                                 dim.height, 
                                                 rowstride);
  cr = cairo_create (cairo_surf);
  cairo_surface_destroy (cairo_surf);
  cairo_scale(cr, kScale, kScale);
  rsvg_handle_render_cairo_sub (handle, cr, 0);
  //rsvg_cairo_to_pixbuf (buffer.get(), rowstride, dim.height);
  cairo_destroy (cr);
  //rsvg_cairo_to_pixbuf (pixels, rowstride, dimensions.height);
// variant 1
/*
  cairo_surf = cairo_image_surface_create_for_data(buffer.get(),
      CAIRO_FORMAT_ARGB32,
      dim.width,
      dim.height,
      cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, dim.width));
  cr = cairo_create(cairo_surf);
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_scale(cr, 3, 3);
  rsvg_handle_render_cairo(handle, cr);
  cairo_destroy(cr);
  cairo_surface_destroy(cairo_surf);
*/
// variant 2
/*
  GdkPixbuf *pix_buf = rsvg_handle_get_pixbuf(handle);
  struct Pixel {
    uint8_t r, g, b, a;
  };
  Pixel *src = reinterpret_cast<Pixel*>(gdk_pixbuf_get_pixels(pix_buf));
  Pixel *dst = reinterpret_cast<Pixel*>(buffer.get());
  
  for (size_t offs = 0; offs < (kBufSize / 4); offs++) {
    dst->r = src->b;
    dst->g = src->g;
    dst->b = src->r;
    dst->a = src->a;
    src++;
    dst++;
  }
*/  
  // заливка фона
 	cairo_save(ctx);
  cairo_rectangle(ctx, 10, 10, 400, 400);
  cairo_set_source_rgb(ctx, (double)94/255, (double)147/255, (double)203/255);
  cairo_fill(ctx);
  cairo_restore(ctx);
  prim->Blit(buf);
  // вывод пиктограммы
  DFBRectangle rect;
  rect.x = 0;
  rect.y = 0;
  rect.w = dim.width;
  rect.h = dim.height;
  DFBSurfaceBlittingFlags m_flags = DSBLIT_NOFX;
  DFB_ADD_BLITTING_FLAG(m_flags, DSBLIT_BLEND_ALPHACHANNEL);
  DFB_ADD_BLITTING_FLAG(m_flags, DSBLIT_DEMULTIPLY);
  /*
  if (alpha < 255) {
    DFB_ADD_BLITTING_FLAG(m_flags, DSBLIT_BLEND_COLORALPHA);
    surf->SetColor(0, 0, 0, alpha);
  }
  */
  prim->SetBlittingFlags(m_flags);
  prim->Blit(&dfb_obj, &rect, 20, 20);
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
    
//    show_rotated_text(cairo_context, primary, &dfb_buf);
//    show_trend(cairo_context, primary, &dfb_buf);
//    primary->Blit(dfb_buf);
    show_svg(dfb, cairo_context, primary, &dfb_buf, "icon_eth_port_highlight.svg");

    primary->Flip();
    
    std::cout << "Waiting for 5 seconds..." << std::endl;
    sleep(15);

    cairo_destroy(cairo_context);
    cairo_surface_destroy(cairo_surface);
    return true;
}

int main (int argc, char **argv) {
	IDirectFBSurface      m_primary;
	DFBSurfaceDescription dsc;
	int w, h;
	
  DirectFB::Init(/*&argc, &argv*/);
  g_type_init();
  IDirectFB m_dfb = DirectFB::Create();

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
}
