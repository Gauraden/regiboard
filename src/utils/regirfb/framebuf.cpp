#include "framebuf.hpp"
#include <sstream>
#include <sys/socket.h>
#include <sys/time.h>

long long GetMSec() {
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
}

namespace fb {

static uint8_t       RGBIter      = 0;
static const uint8_t kRGBWeight[] = {28, 151, 77};
// struct RGBQuad --------------------------------------------------------------
RGBQuad::RGBQuad(): r(RGBIter), g(RGBIter), b(RGBIter), reserved(0) { 
  RGBIter++;
}

RGBQuad::RGBQuad(Byte _b, Byte _g, Byte _r)
    : r(_r), g(_g), b(_b), reserved(0) {
}

void RGBQuad::operator= (Byte *bm) {
  r = bm[0];
  g = bm[1];
  b = bm[2];
}

uint8_t RGBQuad::GetId() const {
  return (uint32_t)((r * kRGBWeight[0]) +
                    (g * kRGBWeight[1]) +
                    (b * kRGBWeight[2])) >> 8;
}
// class Screen ----------------------------------------------------------------
Screen::Screen()
	: _fbdev(-1),
	  _width(0),
	  _height(0),
	  _depth(0),
	  _fbmmap(0),
	  _size(0),
	  _last_up(0),
	  _cur_frame(0) {
  GeneratePalette();
}

Screen::Screen(uint32_t w, uint32_t h, uint8_t d)
	: _fbdev(-1),
	  _width(w),
		_height(h),
		_depth(d),
		_fbmmap(0),
		_size(0),
	  _last_up(0),
	  _cur_frame(0) {
}
	  
Screen::~Screen() {
  UnBind();
}

bool Screen::BindToFbDev(const char *fname) {
	UnBind();
	_fbdev = open(fname, O_RDONLY);
	uint8_t try_n = 1;
  for (; try_n < 30; try_n++) {
	  GetVideoMode();
    if (_depth != 3 && _depth != 4) {
      // [©dsmover: 2015-10-22 Thu 04:57 PM]: должно быть 32 (или 24), если программа
      // регистратура уже стартанула, которая меняет видео режим на правильный
      // А если глубина цвета уже правильная до запуска главного ПО регистратора,
      // то тоже нормально. Главное чтобы глубина цвета была 4(32) (или 3 что 24 бит), 
      // с которой она только умеет работать
      sleep(1);
      continue;
    }
    break;
  }

  if (try_n == 30) {
    std::cerr << "> Ошибка: некорректный видео режим, глубина цвета: "
              << (int)_depth
              << std::endl;
    return false;
  }
  std::cout << "> Видео режим: "
            << _width << "x" << _height
            << " depth: " << (int) _depth
            << std::endl;

	_size   = _width * _height * _depth;
	_fbmmap = (Byte*)mmap(0, _size, PROT_READ, MAP_SHARED, _fbdev, 0);
  _frame_buf_0.reset(new Byte[_size + 1024]);
  _frame_buf_1.reset(new Byte[_size + 1024]);
  _frames[0].data = _frame_buf_0.get();
  _frames[1].data = _frame_buf_1.get();
  return true;
}

void Screen::UnBind() {
	if (_size == 0) {
		return;
  }
	munmap(_fbmmap, _size);
  close(_fbdev);
	_fbmmap = 0;
	_size   = 0;
	_fbdev  = -1;
}

bool Screen::GetVideoMode() {
	struct fb_var_screeninfo fb_info;
	if (ioctl(_fbdev, FBIOGET_VSCREENINFO, &fb_info) < 0) {
    std::cerr << "> Ошибка: при установке видеорежима: "
              << strerror(errno)
              << std::endl;
		return false;
	}
	_width  = fb_info.xres;
	_height = fb_info.yres;
	_depth  = fb_info.bits_per_pixel / 8;
	return true;
}
// Да, да. Это мерзкое говно. Но так проще оформлять/читать структуру в
// соответствии с документацией.
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t  LONG;

struct __attribute__ ((__packed__)) Header {
  static size_t Size() {
    static Header kHd;
    static size_t kSize = sizeof(kHd);
    return kSize;
  }

	WORD  bfType;        // смещение 0 байт от начала файла
	DWORD bfSize;        // смещение 2 байта от начала файла, длина 4 байта
	WORD  bfReserved1; 
	WORD  bfReserved2; 
	DWORD bfOffBits;     // смещение 10 байтов от начала файла, длина 4 байта
};

struct __attribute__ ((__packed__)) Info {
  static size_t Size() {
    static Info kInf;
    static size_t kSize = sizeof(kInf);
    return kSize;
  }
  
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
	if ((kNewTime - _last_up) < 250000000) {
		return false;
  }
	_last_up = kNewTime;
	return true;
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
  for (int idx = 0; idx < kAmount; idx++) {
    _palette[kColors[idx].GetId()] = kColors[idx];
  }
}

bool Screen::Convert24ToPalette(uint8_t *dst, uint8_t *src, uint32_t body_sz) {
  if (dst == 0 || src == 0 || body_sz == 0) {
    return false;
  }
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
  if (dst == 0 || src == 0 || body_sz == 0) {
    return false;
  }
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

uint8_t Screen::GetNextFrameId() const {
	if (_cur_frame == 0) {
	  return 1;
	}
	return 0;
}

void Screen::SwitchToNextFrame() {
  _cur_frame++;
  if (_cur_frame > 1) {
    _cur_frame = 0;
  }
}

bool Screen::PrepareBMPFrame(const uint8_t kBmpDepth) {
/*
	if (not FrameTimeout()) {
		return true;
  }
  */
  const long long kGrabBeg = GetMSec();
	Header header;
	Info   info;
	static const uint32_t kWidth       = _width;
	static const uint32_t kHeight      = _height;
	static const uint8_t  kDepth       = kBmpDepth;
	static const uint32_t kHeadSize    = Header::Size() + Info::Size();
	static const uint32_t kBodySize    = kWidth * kDepth * kHeight;
	static const uint32_t kPaletteSize = kPaletteColors * sizeof(RGBQuad);
	// тип файла, символы «BM» (в HEX: 0x42 0x4d)
	header.bfType        = 0x42 | (0x4D << 8);
	header.bfSize        = kHeadSize + kPaletteSize + kBodySize; // размер всего файла в байтах.
	header.bfReserved1   = 0;
	header.bfReserved2   = 0;
	header.bfOffBits     = kHeadSize + kPaletteSize; // смещение на данные
	info.biSize          = Info::Size();
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
 	Byte *dst_row = _frames[GetNextFrameId()].data;
 	Byte *src_row = _fbmmap;
	// инициализация заголовка BMP
 	memcpy(dst_row, (void*)&header, Header::Size());
 	dst_row += Header::Size();
 	memcpy(dst_row, (void*)&info, Info::Size());
	dst_row += Info::Size();
  // подготовка палитры палитры
 	memcpy(dst_row, (void*)&_palette, kPaletteSize);
	dst_row += kPaletteSize;
 	// конвертация кадра 
  switch (_depth) {
    case 3:
      Convert24ToPalette(dst_row, src_row, kBodySize);
      break;
    case 4:
      Convert32ToPalette(dst_row, src_row, kBodySize);
      break;
    default:
      return false;
  };
  _frame_off = (uint32_t)(dst_row - _frames[GetNextFrameId()].data) + kBodySize;
  _frames[GetNextFrameId()].size = _frame_off;
  SwitchToNextFrame();
	return true;
}

const Byte* Screen::GetFrameData() const {
  return _frames[_cur_frame].data;
}

const uint32_t Screen::GetFrameSize() const {
  return _frames[_cur_frame].size;
}

Screen::Frame Screen::GetFrame() const {
  return _frames[_cur_frame];
}

} // namespace fb
