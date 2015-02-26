#ifndef RD_FRAMEBUF_HPP_INCLUDED
#define RD_FRAMEBUF_HPP_INCLUDED

#define PNG_ZBUF_SIZE 102400

#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>
#include <linux/fb.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <memory>
#include "macro.hpp"
#include <png.h>
#include "boost/scoped_array.hpp"
#include "boost/shared_array.hpp"

namespace fb {

typedef uint64_t                       TimeVal;
typedef boost::scoped_array<png_byte*> PngRows;
typedef boost::shared_array<png_byte>  PngBytes;

struct PngUserData {
  PngUserData(): size(0), offs(0) {}
	PngUserData(PngBytes data_, int size_, int offs_)
			: data(data_),
   			size(size_),
   			offs(offs_) {}
  PngBytes data;
  int      size;
  int      offs;
};

static uint8_t       RGBIter   = 0;
static const uint8_t kRGBWeight[] = {28, 151, 77};

struct RGBQuad {
  RGBQuad(): r(RGBIter), g(RGBIter), b(RGBIter), reserved(0) { RGBIter++; }  
  RGBQuad(uint8_t _b, uint8_t _g, uint8_t _r)
      : r(_r), g(_g), b(_b), reserved(0) {}
  void operator= (png_byte *bm) {
    r = bm[0];
    g = bm[1];
    b = bm[2];
  }
  uint8_t GetId() const {
    return (uint32_t)((r * kRGBWeight[0]) +
                      (g * kRGBWeight[1]) +
                      (b * kRGBWeight[2])) >> 8;
  }
  uint8_t r,
          g,
          b,
          reserved;
};

class Screen {
	public:
		static const int     kPaletteColors = 256;
		static const uint8_t kPngDepth      = 4;
		static const uint8_t kBmpDepth      = 1;
		Screen()
			: _fbdev(-1),
			  _width(0),
			  _height(0),
			  _depth(0),
			  _fbmmap(0),
			  _size(0),
			  _png_ptr(0),
			  _info_ptr(0),
			  _last_up(0) { GeneratePalette(); }
		Screen(uint32_t w, uint32_t h, uint8_t d)
			: _fbdev(-1),
			  _width(w),
				_height(h),
				_depth(d),
				_fbmmap(0),
				_size(0),
			  _png_ptr(0),
			  _info_ptr(0),
			  _last_up(0) {}
		~Screen() { UnBind(); }
		void BindToFbDev(const char *fname);
		void UnBind();
		bool SendFrameAsPng(int socket);
		bool SendFrameAsBmp(int socket);
	private:
		bool   GetVideoMode();
		void   AllocatePngRows();
		bool   SetupPngFrame();
		void   DestroyPngFrame();
		bool   FrameTimeout();
		void   InitHttpHeader(int socket, const char *ctype, uint8_t color_depth);
		void   SendUData(int socket);
		size_t GetFrameSize(uint8_t color_depth) const;
		void   GeneratePalette();
		bool   Convert24ToPalette(uint8_t *dst, uint8_t *src, uint32_t body_sz);
		bool   Convert32ToPalette(uint8_t *dst, uint8_t *src, uint32_t body_sz);

		int         _fbdev;
		uint32_t    _width;
		uint32_t    _height;
		uint8_t     _depth;
		uint8_t    *_fbmmap;
		int         _size;
		PngRows     _png_row;
		PngBytes    _png_buf;
		PngUserData _png_udata;
		png_structp _png_ptr;
    png_infop   _info_ptr;
    TimeVal     _last_up;
    RGBQuad     _palette[kPaletteColors];
};

} // namespace fb

#endif // RD_FRAMEBUF_HPP_INCLUDED
