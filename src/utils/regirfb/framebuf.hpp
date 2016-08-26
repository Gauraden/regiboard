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
#include "boost/scoped_array.hpp"
#include "boost/shared_array.hpp"

long long GetMSec();

namespace fb {

typedef uint8_t                   Byte;
typedef boost::shared_array<Byte> Bytes;
typedef uint64_t                  TimeVal;

struct RGBQuad {
  RGBQuad();
  RGBQuad(Byte _b, Byte _g, Byte _r);
  void operator= (Byte *bm);
  uint8_t GetId() const;
  
  uint8_t r,
          g,
          b,
          reserved;
};

class Screen {
	public:
		static const int kPaletteColors = 256;
		
		Screen();
		Screen(uint32_t w, uint32_t h, uint8_t d);
		~Screen();
		bool BindToFbDev(const char *fname);
		void UnBind();
		bool PrepareBMPFrame(const uint8_t kBmpDepth = 1);
		const Byte*    GetFrameData() const;
		const uint32_t GetFrameSize() const;
	private:
		bool GetVideoMode();
		bool FrameTimeout();
		void GeneratePalette();
		bool Convert24ToPalette(uint8_t *dst, uint8_t *src, uint32_t body_sz);
		bool Convert32ToPalette(uint8_t *dst, uint8_t *src, uint32_t body_sz);

		int       _fbdev;
		uint32_t  _width;
		uint32_t  _height;
		uint8_t   _depth;
		Byte     *_fbmmap;
		int       _size;
		Bytes     _frame_buf;
		uint32_t  _frame_off;
    TimeVal   _last_up;
    RGBQuad   _palette[kPaletteColors];
};

} // namespace fb

#endif // RD_FRAMEBUF_HPP_INCLUDED
