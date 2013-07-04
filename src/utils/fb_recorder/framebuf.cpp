#include "framebuf.hpp"

namespace fb {

void Screen::AllocatePngRows() {
//	const uint32_t kWSize = _width * kPngDepth;
	const uint32_t kWSize = _width * _depth;
	_png_row.reset(new png_byte*[_height]);
//	_png_buf.reset(new png_byte[_height * kWSize]);
	uint32_t offs = 0;
	for (uint32_t i = 0; i < _height; i++, offs += kWSize)
		_png_row[i] = _fbmmap + offs;
		//_png_row[i] = _png_buf.get() + offs;
}

void Screen::BindToFbDev(const char *fname) {
	UnBind();
	_fbdev = open(fname, O_RDONLY);
	GetVideoMode();
	_size   = _width * _height * _depth;
	_fbmmap = (uint8_t*)mmap(0, _size, PROT_READ, MAP_SHARED, _fbdev, 0);
	AllocatePngRows();
}

void Screen::UnBind() {
	if (_size == 0)
		return;
	munmap(_fbmmap, _size);
  close(_fbdev);
	_fbmmap = 0;
	_size   = 0;
	_fbdev  = -1;
}

bool Screen::GetVideoMode() {
	struct fb_var_screeninfo fb_info;
	if (ioctl(_fbdev, FBIOGET_VSCREENINFO, &fb_info) < 0) {
		ERROR("Ошибка " << strerror(errno));
		return false;
	}
	std::cout << "> Видео режим: " << fb_info.xres           << "x"
	                               << fb_info.yres           << "x"
	                               << fb_info.bits_per_pixel << std::endl;
	_width  = fb_info.xres;
	_height = fb_info.yres;
	_depth  = fb_info.bits_per_pixel / 8;
	return true;
}

bool Screen::SaveToPngFile(const char *fname) {
	png_structp  png_ptr  = 0;
  png_infop    info_ptr = 0;
  FILE        *png_file = fopen(fname, "wb");
  if (not png_file) {
		ERROR("Ошибка " << strerror(errno));
		return false;
	}
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
  if (png_ptr == 0) {
		ERROR("Ошибка " << "png_ptr == 0");
    return false;
  }    
  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == 0) {
		ERROR("Ошибка " << "info_ptr == 0");
    return false;
  }    
  if (setjmp(png_jmpbuf(png_ptr))) {
		ERROR("Ошибка " << "setjmp");
    return false;
  }
	const uint8_t kColorType[] = {
		PNG_COLOR_TYPE_GRAY,    // TODO: не тестировалось
		PNG_COLOR_TYPE_PALETTE, // TODO: не тестировалось
		PNG_COLOR_TYPE_RGB,
		PNG_COLOR_TYPE_RGB_ALPHA};
  png_set_IHDR(png_ptr,
               info_ptr,
               _width,
               _height,
               8,
               kColorType[_depth - 1],//PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);
  png_init_io(png_ptr, png_file);
  /*
  const uint8_t  kStep    = (_depth > kPngDepth ? kPngDepth : _depth);
	const uint32_t kWSize   = _width * kPngDepth;
	const uint32_t kFbWSize = _width * _depth;
	uint32_t fb_offs = 0;
	for (uint32_t y = 0; y < _height; y++) {
    for (uint32_t x = 0; x < kWSize; x       += kPngDepth,
                                     fb_offs += _depth)
			memcpy(_png_row[y] + x, _fbmmap + fb_offs, kStep);
  }*/
  png_set_rows(png_ptr, info_ptr, _png_row.get());
  png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
//  png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_ALPHA, NULL);
  png_destroy_write_struct(&png_ptr, &info_ptr);
  return true;
}

} // namespace fb
