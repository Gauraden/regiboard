#include "framebuf.hpp"
#include <sstream>
#include <sys/socket.h>
//#include <arm_neon.h>

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

bool Screen::SendFrameAsPng(int socket) {
  // FIXIT
  /*
	SetupPngFrame();
  _png_udata.offs = 0;
  png_write_info(_png_ptr, _info_ptr);
  png_write_png(_png_ptr, _info_ptr, PNG_TRANSFORM_BGR, NULL);
  DestroyPngFrame();
  */
  return true;
}
// Да, да. Это мерзкое говно. Но так проще оформлять/читать структуру в
// соответствии с документацией.
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
	WORD  biPlanes;        // В BMP допустимо только значение 1 
	WORD  biBitCount; 
	DWORD biCompression;   // Указывает на способ хранения пикселей
	DWORD biSizeImage;     // Размер пиксельных данных в байтах
	LONG  biXPelsPerMeter; // Количество пикселей на метр по горизонтали
	LONG  biYPelsPerMeter; // Количество пикселей на метр по вертикали
	DWORD biClrUsed;       // Размер таблицы цветов в ячейках
	DWORD biClrImportant;  // Количество ячеек от начала таблицы цветов до последней используемой (можно указывать 0, если используем все ячейки)
};

bool Screen::FrameTimeout() {
	timespec new_tm;
	clock_gettime(CLOCK_MONOTONIC, &new_tm);
	const uint64_t kNewTime = (new_tm.tv_sec * 1000000000) + new_tm.tv_nsec;
	if ((kNewTime - _last_up) < 250000000)
		return false;
	_last_up = kNewTime;
	return true;
}

void Screen::InitHttpHeader(int         socket,
                            const char *ctype,
                            uint8_t     color_depth) {
  std::stringstream head;
  head << "HTTP/1.1 200 OK\r\n"
		   << "Cache-Control: no-store\r\n" // , must-revalidate, max-age=0
       << "Pragma: no-store\r\n" // , no-cache
       << "Expires: 0\r\n"
	     << "Connection: close\r\n"
		   << "Content-Length: " << GetFrameSize(color_depth) << "\r\n"
		   <<	"Content-Type: " << ctype << "\r\n"
       << "\r\n";
  _png_udata.offs = 0;
  memcpy(_png_buf.get(), head.str().c_str(), head.str().size());
  _png_udata.offs = head.str().size();
}

void Screen::SendUData(int socket) {
  ssize_t sended  = 0;
  int     err_num = 0;
  while (sended < _png_udata.offs) {
    const ssize_t kResult = send(socket, _png_buf.get() + sended,
                                         _png_udata.offs - sended,
                                         MSG_NOSIGNAL);
    if (kResult < 0) {
      err_num++;
      if (err_num > 3) {
        ERROR("Ошибка при отправки кадра! sended = " << (long int)sended);
        break;
      }
      continue;
    }
    sended += kResult;
  }
}

void Screen::GeneratePalette() {
  static const RGBQuad kColors[] = {
    RGBQuad(0,   0,   0),
    RGBQuad(255, 255, 255),
    RGBQuad(189, 195, 199),
    RGBQuad(236, 240, 241),
    RGBQuad(127, 140, 141), 
    RGBQuad(149, 165, 166), 
    RGBQuad(225, 225, 225), 
    RGBQuad(150, 75,  0),
    RGBQuad(231, 76,  60),
    RGBQuad(225, 129, 128), 
    RGBQuad(255, 153, 0),   
    RGBQuad(255, 218, 0),
    RGBQuad(255, 255, 156),
    RGBQuad(85,  187, 119),
    RGBQuad(153, 255, 119),
    RGBQuad(41,  128, 185),
    RGBQuad(0,   191, 255),
    RGBQuad(210, 0,   255),
    RGBQuad(199, 103, 219),
    RGBQuad(52,  73,  94),
    RGBQuad(44,  62,  80),
    RGBQuad(99,  225, 255),
    RGBQuad(246, 165, 0)
  };
  static const int kAmount = sizeof(kColors) / sizeof(RGBQuad);
  for (int idx = 0; idx < kAmount; idx++)
    _palette[kColors[idx].GetId()] = kColors[idx];
}

bool Screen::Convert24ToPalette(uint8_t *dst, uint8_t *src, uint32_t body_sz) {
  if (dst == 0 || src == 0 || body_sz == 0)
    return false;
  // Реализация алгоритма на асме + neon. Алгоритм изменён, расчёт ведётся с
  // учетом веса компоненты RGB
  __asm__ __volatile__ (
    " mov       r0, %[src]\n\t"
    " mov       r1, %[dst]\n\t"
    " mov       r2, %[size]\n\t"
    " lsr       r2, r2, #3\n\t" // shift right >> 3 bit
    " mov       r3, %[rw]\n\t"  // weight of R component
    " mov       r4, %[gw]\n\t"  // weight of G component
    " mov       r5, %[bw]\n\t"  // weight of B component
    " vdup.8    d3, r3\n\t"
    " vdup.8    d4, r4\n\t"
    " vdup.8    d5, r5\n\t"
    
    ".loop24bits: \n\t"
    " vld3.8    {d0-d2}, [r0]!\n\t" // vld3.8 для режима 24bit
    " vmull.u8  q3, d0, d3\n\t"
    " vmlal.u8  q3, d1, d4\n\t"
    " vmlal.u8  q3, d2, d5\n\t"
    " vshrn.u16 d6, q3, #8\n\t"
    " vst1.8	  {d6}, [r1]!\n\t"     
    " subs      r2, r2, #1\n\t"
    " bne       .loop24bits\n\t"
    : // no output
    : [size]  "r" (body_sz),
      [src]   "r" (src),
      [dst]   "r" (dst),
      [rw]    "r" (kRGBWeight[0]),
      [gw]    "r" (kRGBWeight[1]),
      [bw]    "r" (kRGBWeight[2])
    : "r0", "r1", "r2", "r3", "r4", "r5",
      "d0", "d1", "d2", "d3", "d4", "d5", "d6"
  );
  return true;
}

bool Screen::Convert32ToPalette(uint8_t *dst, uint8_t *src, uint32_t body_sz) {
  if (dst == 0 || src == 0 || body_sz == 0)
    return false;
  // Реализация алгоритма на асме + neon. Алгоритм изменён, расчёт ведётся с
  // учетом веса компоненты RGB
  __asm__ __volatile__ (
    " mov       r0, %[src]\n\t"
    " mov       r1, %[dst]\n\t"
    " mov       r2, %[size]\n\t"
    " lsr       r2, r2, #3\n\t" // shift right >> 3 bit (divide by 8)
    " mov       r3, %[rw]\n\t"  // weight of R component
    " mov       r4, %[gw]\n\t"  // weight of G component
    " mov       r5, %[bw]\n\t"  // weight of B component
    " vdup.8    d4, r3\n\t"
    " vdup.8    d5, r4\n\t"
    " vdup.8    d6, r5\n\t"
    
    ".loop32bits: \n\t"
    " vld4.8    {d0-d3}, [r0]!\n\t" // vld4.8 для режима 32bit
    " vmull.u8  q7, d0, d4\n\t"
    " vmlal.u8  q7, d1, d5\n\t"
    " vmlal.u8  q7, d2, d6\n\t"
    " vshrn.u16 d7, q7, #8\n\t"
    " vst1.8	  {d7}, [r1]!\n\t" // move 8 elements to dst (r1)
    " subs      r2, r2, #1\n\t"  // decrease by 1
    " bne       .loop32bits\n\t"
    : // no output
    : [size]  "r" (body_sz),
      [src]   "r" (src),
      [dst]   "r" (dst),
      [rw]    "r" (kRGBWeight[0]),
      [gw]    "r" (kRGBWeight[1]),
      [bw]    "r" (kRGBWeight[2])
    : "r0", "r1", "r2", "r3", "r4", "r5",
      "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7"
  );
  return true;
}

bool Screen::SendFrameAsBmp(int socket) {
	if (not FrameTimeout()) {
	 	SendUData(socket);
		return true;
  }
	Header header;
	Info   info;
	static const uint32_t kWidth       = _width;
	static const uint32_t kHeight      = _height;
	static const uint8_t  kDepth       = kBmpDepth;
	static const uint32_t kHeadSize    = sizeof(header) + sizeof(info);
	static const uint32_t kBodySize    = kWidth * kDepth * kHeight;
	static const uint32_t kPaletteSize = kPaletteColors * sizeof(RGBQuad);
	header.bfType        = 0x42 | (0x4D << 8);    // тип файла, символы «BM» (в HEX: 0x42 0x4d).
	header.bfSize        = kHeadSize + kPaletteSize + kBodySize; // размер всего файла в байтах.
	header.bfReserved1   = 0;
	header.bfReserved2   = 0;
	header.bfOffBits     = kHeadSize + kPaletteSize; // смещение на данные
	info.biSize          = sizeof(info);
	info.biWidth         = kWidth;
	info.biHeight        = kHeight * (-1);
	info.biPlanes        = 1;
	info.biBitCount      = kDepth * 8;
	info.biCompression   = 0; // BI_RGB
	info.biSizeImage     = kBodySize;
	info.biXPelsPerMeter = 0;
	info.biYPelsPerMeter = 0;
	info.biClrUsed       = kPaletteSize;
	info.biClrImportant  = 0;
	// инициализация заголовков: HTTP, BMP
	InitHttpHeader(socket, "image/bmp", kBmpDepth);
 	memcpy(_png_buf.get() + _png_udata.offs, (void*)&header, sizeof(header));
 	_png_udata.offs += sizeof(header);
 	memcpy(_png_buf.get() + _png_udata.offs, (void*)&info,   sizeof(info));
	_png_udata.offs += sizeof(info);
  // подготовка палитры палитры
 	memcpy(_png_buf.get() + _png_udata.offs, (void*)&_palette, kPaletteSize);
	_png_udata.offs += kPaletteSize;
  // подготовка кадра
  // __attribute__ ((aligned(32)))
 	uint8_t *dst_row = _png_buf.get() + _png_udata.offs;
 	uint8_t *src_row = _fbmmap;
 	// конвертация кадра 
  switch (_depth) {
    case 3:
      Convert24ToPalette(dst_row, src_row, kBodySize);
      break;
    case 4:
      Convert32ToPalette(dst_row, src_row, kBodySize);
      break;
    default:
      break;
  };
  // Реализация алгоритма с помощью arm_neon.h
  /*uint8x8_t  out;
  uint8x8_t  alpha = vdup_n_u8(128);
  uint16x8_t tmp;
  for (int d_col = 0; d_col < kBodySize / 8; d_col += kDepth) {
    uint8x8x3_t rgb = vld3_u8(src_row);
    tmp = vaddl_u8(rgb.val[0], rgb.val[1]);
    tmp = vaddw_u8(tmp, rgb.val[2]);
    tmp = vaddw_u8(tmp, alpha);
    out = vshrn_n_u16(tmp, 2);
   	vst1_u8(dst_row, out);
    src_row += kSrcStep;
    dst_row += 8;
  }*/
	_png_udata.offs += kBodySize;
 	SendUData(socket);
	return true;
}

size_t Screen::GetFrameSize(uint8_t color_depth) const {
	static const int kHeadSize    = sizeof(Header) + sizeof(Info);
	static const int kPaletteSize = kPaletteColors * sizeof(RGBQuad);
	static const int kBodySize    = _width * _height * color_depth;
	return kHeadSize + kPaletteSize + kBodySize;
}

} // namespace fb
