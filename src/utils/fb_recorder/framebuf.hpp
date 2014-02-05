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

class Screen {
	public:
		static const uint8_t kPngDepth = 3;
		Screen()
			: _fbdev(-1),
			  _width(0),
			  _height(0),
			  _depth(0),
			  _fbmmap(0),
			  _size(0),
			  _png_ptr(0),
			  _info_ptr(0),
			  _last_up(0) {}
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
		bool ConvertToPng();
		bool ConvertToBmp(const char *ext_head, size_t ext_size);
		PngUserData get_png_udata() { return _png_udata; }
		size_t GetFrameSize() const;
	private:
		bool GetVideoMode();
		void AllocatePngRows();
		bool SetupPngFrame();
		void DestroyPngFrame();
		bool FrameTimeout();

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
};

} // namespace fb

#endif // RD_FRAMEBUF_HPP_INCLUDED
