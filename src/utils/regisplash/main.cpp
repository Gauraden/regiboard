#include <iostream>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <cstdlib>
#include <errno.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <string.h>
#include <memory>

#pragma pack(push, 1)

#define ERROR(msg) std::cout << __LINE__ << ": " << __FUNCTION__ << ": " << msg << std::endl

struct PixelRGB;

struct PixelRGBA {
	PixelRGBA(uint8_t _r = 0, uint8_t _g = 0, uint8_t _b = 0)
		: r(_r), g(_g), b(_b), a(0) {}
	void operator= (const PixelRGB &src);
	uint8_t r, g, b, a;
};

struct PixelRGB {
	PixelRGB(uint8_t _r = 0, uint8_t _g = 0, uint8_t _b = 0)
		: r(_r), g(_g), b(_b) {}
	void operator= (const PixelRGBA &src);
	uint8_t r, g, b;
};

void PixelRGBA::operator= (const PixelRGB &src) {
	r = src.r;
	g = src.g;
	b = src.b;
}

void PixelRGB::operator= (const PixelRGBA &src) {
	r = src.r;
	g = src.g;
	b = src.b;
}

struct Screen {
	Screen(): width(0), height(0), depth(0), buffer(0) {}
	Screen(uint32_t w, uint32_t h, uint8_t d)
		: width(w),
		  height(h),
		  depth(d),
		  buffer(0) {}
		  
	uint32_t  width;
	uint32_t  height;
	uint8_t   depth;
	uint8_t  *buffer;
};

struct TagBitMapFileHeader { 
  uint16_t bfType;        // смещение 0 от начала файла
  uint32_t bfSize;        // смещение 2 от начала файла, длина 4
  uint16_t bfReserved1; 
  uint16_t bfReserved2; 
  uint32_t bfOffBits;     // смещение 10 от начала файла, длина 4
};

struct TagBitMapInfoHeader{
  uint32_t biSize; 
  int32_t  biWidth; 
  int32_t  biHeight; 
  uint16_t biPlanes; 
  uint16_t biBitCount; 
  uint32_t biCompression; 
  uint32_t biSizeImage; 
  int32_t  biXPelsPerMeter; 
  int32_t  biYPelsPerMeter; 
  uint32_t biClrUsed; 
  uint32_t biClrImportant; 
};

typedef std::auto_ptr<uint8_t> BMPData;

bool GetVideoMode(int fbdev, struct fb_var_screeninfo *info) {
	if (ioctl(fbdev, FBIOGET_VSCREENINFO, info) < 0) {
		ERROR("Ошибка " << strerror(errno));
		return false;
	}
	std::cout << "> Видео режим: " << info->xres           << "x"
	                               << info->yres           << "x"
	                               << info->bits_per_pixel << std::endl;
	return true;
}

bool SetVideoMode(int fbdev, Screen *scr) {
  if (scr == 0 || fbdev < 0)
    return false;
	struct fb_var_screeninfo fb_scr_info;
	if (not GetVideoMode(fbdev, &fb_scr_info))
		return false;
	scr->width  = fb_scr_info.xres;
	scr->height = fb_scr_info.yres;
	fb_scr_info.bits_per_pixel = scr->depth * 8;
	fb_scr_info.activate       = FB_ACTIVATE_NOW;
	if (ioctl(fbdev, FBIOPUT_VSCREENINFO, &fb_scr_info) < 0) {
		ERROR("Ошибка " << strerror(errno));
		return false;
	}
	std::cout << "< Видео режим: " << fb_scr_info.xres           << "x"
	                               << fb_scr_info.yres           << "x"
	                               << fb_scr_info.bits_per_pixel << std::endl;
	return true;
}

void Copy24RowTo32(uint8_t *dst, PixelRGB *src, uint16_t w) {
  PixelRGBA *scr_px = (PixelRGBA*)dst;
	for (int col = 0; col < w; col++) {
		scr_px[col] = src[col];
	}
}

bool PrintBMP(const char *name, Screen *screen) {
	const int kBMPFile = open(name, O_RDONLY);
	if (kBMPFile == -1) {
		ERROR("Ошибка " << strerror(errno));
		return false;
	}
	TagBitMapFileHeader file_header;
	TagBitMapInfoHeader info_header;
	read(kBMPFile, &file_header, sizeof(file_header));
	read(kBMPFile, &info_header, sizeof(info_header));
	const int      kDepth       = info_header.biBitCount / 8;
	const unsigned kRowWidth    = info_header.biWidth * kDepth;
	const unsigned kExtRowWidth = info_header.biSizeImage / info_header.biHeight;
	const unsigned kScrXOffset  = ((screen->width - info_header.biWidth) / 2) * screen->depth;
	const unsigned kScrYOffset  = (screen->height - info_header.biHeight) / 2;
	BMPData data(new uint8_t[kExtRowWidth]);
	PixelRGB *bmp_px = (PixelRGB*)data.get();
	lseek(kBMPFile, file_header.bfOffBits, SEEK_SET);
	const unsigned kScrWidth = screen->width * screen->depth;
	for (int row = info_header.biHeight; row >= 0 ; row--) {
		read(kBMPFile, data.get(), kExtRowWidth);
		const unsigned kScrRowOffset = (row + kScrYOffset) * kScrWidth;
		if (kDepth == screen->depth) {
			memcpy(&screen->buffer[kScrRowOffset + kScrXOffset],
			       data.get(),
			       kRowWidth);
			continue;
		}
		switch (screen->depth) {
		  case 4:
		    Copy24RowTo32(&screen->buffer[kScrRowOffset + kScrXOffset],
		                   bmp_px,
		                   info_header.biWidth);
		    break;
		  default:
		    break;
		};
	}
	close(kBMPFile);
	return true;
}

int main(int argc, char *argv[]) {
	Screen screen(800, 600, 4);
	std::string splash_path="/root/vbr_splash.bmp";
	if (argc > 1) {
    splash_path = argv[1];
	}
	const int kFBDev = open("/dev/fb0", O_RDWR);
	if (not SetVideoMode(kFBDev, &screen)) {
		close(kFBDev);
		return 0;
	}
	const int kSize = screen.width * screen.height * screen.depth;
	screen.buffer = (uint8_t*)mmap(0, kSize, PROT_WRITE, MAP_SHARED, kFBDev, 0);
	PrintBMP(splash_path.c_str(), &screen);
	munmap(screen.buffer, kSize);
	close(kFBDev);
	return 0;
}
