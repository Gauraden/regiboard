#include "framebuf.hpp"

namespace fb {

void Screen::AllocatePngRows() {
	const uint32_t kWSize = _width * _depth;
	_png_row.reset(new png_byte*[_height]);
	_png_buf.reset(new png_byte[_height * kWSize * 2]);
	uint32_t offs = 0;
	for (uint32_t i = 0; i < _height; i++, offs += kWSize)
		_png_row[i] = _fbmmap + offs;
	_png_udata.data = _png_buf;
	_png_udata.size = _size;
	_png_udata.offs = 0;
}

void Screen::BindToFbDev(const char *fname) {
	UnBind();
	_fbdev = open(fname, O_RDONLY);
	GetVideoMode();
	_size   = _width * _height * _depth;
	_fbmmap = (uint8_t*)mmap(0, _size, PROT_READ, MAP_SHARED, _fbdev, 0);
	AllocatePngRows();
}

static void PngToBuff(png_structp png_ptr, png_bytep data, png_size_t length) {
  PngUserData *udata = reinterpret_cast<PngUserData*>(png_get_io_ptr(png_ptr));
  png_byte    *frame = udata->data.get();
  const int    kSize = udata->size;
  const int    kOffs = udata->offs;
  if ((kOffs + length) > kSize)
  	length = kSize - kOffs;
 	memcpy(frame + kOffs, data, length);
 	udata->offs += length;
}

bool Screen::SetupPngFrame() {
  _png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
  if (_png_ptr == 0) {
		ERROR("Ошибка " << "png_ptr == 0");
    return false;
  }
  _info_ptr = png_create_info_struct(_png_ptr);
  if (_info_ptr == 0) {
		ERROR("Ошибка " << "info_ptr == 0");
    return false;
  }    
  if (setjmp(png_jmpbuf(_png_ptr))) {
		ERROR("Ошибка " << "setjmp");
    return false;
  }
	const uint8_t kColorType[] = {
		PNG_COLOR_TYPE_GRAY,    // TODO: не тестировалось
		PNG_COLOR_TYPE_PALETTE, // TODO: не тестировалось
		PNG_COLOR_TYPE_RGB,
		PNG_COLOR_TYPE_RGB_ALPHA
	};
  png_set_IHDR(_png_ptr,
               _info_ptr,
               _width,
               _height,
               8,
               kColorType[_depth - 1],//PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);
  png_set_write_fn(_png_ptr, &_png_udata, PngToBuff, 0);
  png_set_rows(_png_ptr, _info_ptr, _png_row.get());
  return true;
}

void Screen::DestroyPngFrame() {
	if (_png_ptr == 0 || _info_ptr == 0)
		return;
  png_destroy_write_struct(&_png_ptr, &_info_ptr);
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

bool Screen::ConvertToPng() {
//	ERROR("DEBUG: new PNG frame...: " << PNG_ZBUF_SIZE);
	SetupPngFrame();
  _png_udata.offs = 0;
  png_write_info(_png_ptr, _info_ptr);
  png_write_png(_png_ptr, _info_ptr, PNG_TRANSFORM_BGR, NULL);
  DestroyPngFrame();
  return true;
}

typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t  LONG;

struct __attribute__ ((__packed__)) Header {
	WORD  bfType;        // смещение 0 байт от начала файла
	DWORD bfSize;        // смещение 2 байта от начала файла, длина 4 байта
	WORD  bfReserved1; 
	WORD  bfReserved2; 
	DWORD bfOffBits;     // смещение 10 байтов от начала файла, длина 4 байта
};

struct __attribute__ ((__packed__)) Info {
	DWORD biSize; 
	LONG  biWidth; 
	LONG  biHeight; 
	WORD  biPlanes; 
	WORD  biBitCount; 
	DWORD biCompression; 
	DWORD biSizeImage; 
	LONG  biXPelsPerMeter; 
	LONG  biYPelsPerMeter; 
	DWORD biClrUsed; 
	DWORD biClrImportant; 
};

bool Screen::FrameTimeout() {
	timespec new_tm;
	clock_gettime(CLOCK_MONOTONIC, &new_tm);
	const uint64_t kNewTime = (new_tm.tv_sec * 1000000000) + new_tm.tv_nsec;
	if ((kNewTime - _last_up) < 500000000)
		return false;
	_last_up = kNewTime;
	return true;
}

bool Screen::ConvertToBmp(const char *ext_head, size_t ext_size) {
	if (not FrameTimeout())
		return true;
	Header header;
	Info   info;
	const int kWidth    = _width;
	const int kHeight   = _height;
	const int kDepth    = _depth;
	const int kHeadSize = sizeof(header) + sizeof(info);
	const int kBodySize = kWidth * kHeight * kDepth;
	header.bfType        = 0x42 | (0x4D << 8);   // тип файла, символы «BM» (в HEX: 0x42 0x4d).
	header.bfSize        = kHeadSize + kBodySize; // размер всего файла в байтах.
	header.bfReserved1   = 0;
	header.bfReserved2   = 0;
	header.bfOffBits     = kHeadSize;             // содержит смещение на данные изображения 
	info.biSize          = sizeof(info);
	info.biWidth         = kWidth;
	info.biHeight        = kHeight * (-1);
	info.biPlanes        = 1;
	info.biBitCount      = kDepth * 8;
	info.biCompression   = 0; // BI_RGB
	info.biSizeImage     = kBodySize;
	info.biXPelsPerMeter = 0;
	info.biYPelsPerMeter = 0;
	info.biClrUsed       = 0;
	info.biClrImportant  = 0;
//	ERROR("DEBUG: new BMP frame...: size: " << header.bfSize << ": offs: " << header.bfOffBits);
  std::cout << "w: " << kWidth << "; h: " << kHeight << "; d: " << kDepth << std::endl;
	_png_udata.offs = 0;
	if (ext_size > 0 && ext_head != 0) {
	 	memcpy(_png_buf.get(), ext_head, ext_size);
 		_png_udata.offs = ext_size;
	}
 	memcpy(_png_buf.get() + _png_udata.offs, (void*)&header, sizeof(header));
 	_png_udata.offs += sizeof(header);
 	memcpy(_png_buf.get() + _png_udata.offs, (void*)&info,   sizeof(info));
	_png_udata.offs += sizeof(info);
 	memcpy(_png_buf.get() + _png_udata.offs, _fbmmap, kBodySize);
	_png_udata.offs += kBodySize;
	return true;
}

size_t Screen::GetFrameSize() const {
	const int kHeadSize = sizeof(Header) + sizeof(Info);
	const int kBodySize = _width * _height * _depth;//_width * _height * _depth;
	return kHeadSize + kBodySize;
}

} // namespace fb
