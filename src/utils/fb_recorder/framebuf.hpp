#ifndef RD_FRAMEBUF_HPP_INCLUDED
#define RD_FRAMEBUF_HPP_INCLUDED

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

namespace fb {

typedef boost::scoped_array<png_byte*> PngRows;
typedef boost::scoped_array<png_byte>  PngBytes;

class Screen {
	public:
		static const uint8_t kPngDepth = 3;
		Screen()
			: _fbdev(-1),
			  _width(0),
			  _height(0),
			  _depth(0),
			  _fbmmap(0),
			  _size(0) {}
		Screen(uint32_t w, uint32_t h, uint8_t d)
			: _fbdev(-1),
			  _width(w),
				_height(h),
				_depth(d),
				_fbmmap(0),
				_size(0) {}
		~Screen() { UnBind(); }
		void BindToFbDev(const char *fname);
		void UnBind();
		bool SaveToPngFile(const char *fname);
	private:
		bool GetVideoMode();
		void AllocatePngRows();

		int       _fbdev;
		uint32_t  _width;
		uint32_t  _height;
		uint8_t   _depth;
		uint8_t  *_fbmmap;
		int       _size;
		PngRows   _png_row;
		PngBytes  _png_buf;
};

} // namespace fb

#endif // RD_FRAMEBUF_HPP_INCLUDED
