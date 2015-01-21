#include "dfb_cairo.hpp"
#include <math.h>

Color::Color(u8 r, u8 g, u8 b, u8 a): _r(r), _g(g), _b(b), _a(a) {
}

Color::operator DFBColor() const {
  DFBColor c;
  c.a = _a;
  c.r = _r;
  c.g = _g;
  c.b = _b;
  return c;
}

void Color::ApplyTo(IDirectFBSurface &surf) const {
  surf.SetColor(_r, _g, _b, _a);
}

UniSurface::UniSurface()
  : _dfb(0),
    _cairo_surf(0),
    _cairo_ctx(0),
    _ready(false) {
  _caps = DSCAPS_NONE;
}

UniSurface::UniSurface(const IDirectFB &dfb)
  : _dfb(dfb),
    _cairo_surf(0),
    _cairo_ctx(0),
    _ready(false) {
  _caps = DSCAPS_NONE;
}

UniSurface::~UniSurface() {
  cairo_destroy(_cairo_ctx);
  cairo_surface_destroy(_cairo_surf);
}

void UniSurface::Resize(uint32_t w, uint32_t h) {
  if (_dfb == 0)
    return;
  if (_ready) {
    cairo_destroy(_cairo_ctx);
    cairo_surface_destroy(_cairo_surf);
    _ready = false;
  }
  static const unsigned kDepth = 4;
  DFBSurfaceDescription dsc;
  dsc.flags       = (DFBSurfaceDescriptionFlags)(DSDESC_WIDTH |
                                                 DSDESC_HEIGHT |
                                                 DSDESC_PIXELFORMAT |
                                                 DSDESC_CAPS |
                                                 DSDESC_PREALLOCATED
                                                 );
  dsc.caps        = _caps;
  dsc.width       = w;
  dsc.height      = h;
  dsc.pixelformat = DSPF_RGB32;
  const uint32_t kBufSize = w * h * kDepth;
  _buffer.reset(new Byte[kBufSize]);
  dsc.preallocated[0].data  = _buffer.get();
  dsc.preallocated[0].pitch = dsc.width * kDepth;
  dsc.preallocated[1].data  = NULL;
  dsc.preallocated[1].pitch = 0;
  _dfb_surf   = _dfb.CreateSurface(dsc);
  _cairo_surf = cairo_image_surface_create_for_data(_buffer.get(),
        CAIRO_FORMAT_ARGB32,
        w,
        h,
        cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w));
  if (not _cairo_surf)
    return;
  _cairo_ctx = cairo_create(_cairo_surf);
  if (not _cairo_ctx)
    return;
  _ready = true;
  memset(_buffer.get(), 0, kBufSize);
}

cairo_t* UniSurface::get_cairo_ctx() {
  return _cairo_ctx;
}
IDirectFB* UniSurface::get_dfb() {
  return &_dfb;
}

IDirectFBSurface* UniSurface::get_ptr_dfb_surf() {
  return &_dfb_surf;
}

void UniSurface::set_dfb(const IDirectFB &dfb) {
  _dfb = dfb;
}

void UniSurface::set_caps(DFBSurfaceCapabilities caps) {
  _caps = caps;
}

IDirectFBSurface UniSurface::get_dfb_surf() {
  return _dfb_surf;
}

uint32_t UniSurface::GetBufSize() const {
  int w, h;
  const_cast<IDirectFBSurface&>(_dfb_surf).GetSize(&w, &h);
  // 4 - amount of bits for pixel (depth)
  return (w * h * 4);
}

bool UniSurface::IsReady() const {
  return _ready;
}

void UniSurface::CopyBitmapFrom(const UniSurface &src_surf) {
  if (not _buffer || not src_surf._buffer)
    return;
  const uint32_t kDstSz = GetBufSize();
  if (kDstSz != src_surf.GetBufSize())
    return;
  memcpy(_buffer.get(), src_surf._buffer.get(), kDstSz);
}

void UniSurface::UseColorRGB(const DFBColor &color) {
  if (_cairo_ctx == 0)
    return;
  cairo_set_source_rgb(_cairo_ctx, color.r / 255.0,
                                   color.g / 255.0,
                                   color.b / 255.0);
}

void UniSurface::UseColorRGBA(const DFBColor &color) {
  if (_cairo_ctx == 0)
    return;
  cairo_set_source_rgba(_cairo_ctx, color.r / 255.0,
                                    color.g / 255.0,
                                    color.b / 255.0,
                                    color.a / 255.0);
}

void UniSurface::DrawLine(const DFBRegion &line) {
  if (_cairo_ctx == 0)
    return;
  cairo_move_to(_cairo_ctx, line.x1, line.y1);
  cairo_line_to(_cairo_ctx, line.x2, line.y2);
  cairo_stroke (_cairo_ctx);
}

void UniSurface::DrawLine(double x0, double y0, double x1, double y1) {
  if (_cairo_ctx == 0)
    return;
  cairo_move_to(_cairo_ctx, x0, y0);
  cairo_line_to(_cairo_ctx, x1, y1);
  cairo_stroke (_cairo_ctx);
}

void UniSurface::DrawSubLine(double x0, double y0, double x1, double y1) {
  if (_cairo_ctx == 0)
    return;
  cairo_new_sub_path(_cairo_ctx);
  cairo_move_to(_cairo_ctx, x0, y0);
  cairo_line_to(_cairo_ctx, x1, y1);
}

void UniSurface::DrawArc(double x,
                         double y,
                         double radius,
                         double start_a,
                         double span_a) {
  if (_cairo_ctx == 0)
    return;
  static const double kGrad2Rad = M_PI / 180.0;
  const double kAngle0 = (-start_a) * kGrad2Rad;
  const double kAngle1 = (-(start_a + span_a)) * kGrad2Rad;
  cairo_arc_negative(_cairo_ctx, x, y, radius, kAngle0, kAngle1);
  cairo_stroke(_cairo_ctx);
}

void UniSurface::FlipTo(IDirectFBSurface &dfb) {
  dfb.SetBlittingFlags((DFBSurfaceBlittingFlags)(DSBLIT_NOFX));
  dfb.Blit(_dfb_surf);
  dfb.Flip();
}
