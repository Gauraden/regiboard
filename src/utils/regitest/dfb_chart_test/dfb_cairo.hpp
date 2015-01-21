#include <unistd.h>
#include <++DFB/++dfb.h>
#include <directfb.h>
#include <cairo/cairo-directfb.h>
#include "boost/scoped_array.hpp"

class Color {
  public:
    Color(u8 r, u8 g, u8 b, u8 a);
    operator DFBColor() const;
    void ApplyTo(IDirectFBSurface &surf) const;
  private:
    u8 _r;
    u8 _g;
    u8 _b;
    u8 _a;
};

class UniSurface {
  public:
    UniSurface();
    UniSurface(const IDirectFB &dfb);
    ~UniSurface();
    void             Resize(uint32_t w, uint32_t h);
    cairo_t*         get_cairo_ctx();
    IDirectFB*       get_dfb();
    void             set_dfb(const IDirectFB &dfb);
    void             set_caps(DFBSurfaceCapabilities caps);
    IDirectFBSurface* get_ptr_dfb_surf();
    IDirectFBSurface  get_dfb_surf();
    bool             IsReady() const;
    void             CopyBitmapFrom(const UniSurface &src_surf);
    void             UseColorRGB(const DFBColor &color);
    void             UseColorRGBA(const DFBColor &color);
    void             DrawLine(const DFBRegion &line);
    void             DrawLine(double x0, double y0, double x1, double y1);
    void             DrawSubLine(double x0, double y0, double x1, double y1);
    void             DrawArc(double x,
                             double y,
                             double radius,
                             double start_a,
                             double span_a);
    void             FlipTo(IDirectFBSurface &dfb);
  protected:
    uint32_t GetBufSize() const;
  private:
    typedef unsigned char             Byte;
    typedef boost::scoped_array<Byte> ArrayOfBytes;

    UniSurface(const UniSurface&);
    void operator= (const UniSurface&);

    DFBSurfaceCapabilities  _caps;
    IDirectFB               _dfb;
    IDirectFBSurface        _dfb_surf;
    cairo_surface_t        *_cairo_surf;
    cairo_t                *_cairo_ctx;
    ArrayOfBytes            _buffer;
    bool                    _ready;
};
