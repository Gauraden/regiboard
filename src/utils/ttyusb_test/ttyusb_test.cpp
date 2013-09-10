#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <stdint.h>
#include "ftdi.hpp"
#include "usb.h"

struct EEPROMField {
	EEPROMField(const char *_name,
	            int32_t     _addr,
	            int32_t     _offs,
	            int32_t     _mask,
	            int32_t     _value = -1)
		: name(_name),
		  addr(_addr),
		  offs(_offs),
		  mask(_mask),
		  value(_value) {}
	void operator=(int new_value) { 
		value = new_value;
	}
	const char    *name;
	const int32_t  addr;
	const int32_t  offs;
	const int32_t  mask;
	int32_t        value;
};

static EEPROMField Config[] = {
	EEPROMField("A VCP driver...", 0x00, 0x3, 0x1), // DRIVER_VCP 0x08
	EEPROMField("B VCP driver...", 0x01, 0x3, 0x1),
	EEPROMField("C VCP driver...", 0x00, 0x7, 0x1),
	EEPROMField("D VCP driver...", 0x01, 0x7, 0x1),
	EEPROMField("vendor id......", 0x02, 0x0, 0xFFFF),
	EEPROMField("product id.....", 0x04, 0x0, 0xFFFF),
	EEPROMField("release num....", 0x06, 0x0, 0xFF),
	EEPROMField("chip type......", 0x07, 0x0, 0xFF),
	EEPROMField("remote wakeup..", 0x08, 0x5, 0x1),
	EEPROMField("self powered...", 0x08, 0x6, 0x1),
	EEPROMField("conf descriptor", 0x08, 0x7, 0x1),
	EEPROMField("max power......", 0x09, 0x0, 0xFF),
	EEPROMField("In isochronous.", 0x0A, 0x0, 0x1),
	EEPROMField("Out isochronous", 0x0A, 0x1, 0x1),
	EEPROMField("suspend pull...", 0x0A, 0x2, 0x1),
	EEPROMField("use serial.....", 0x0A, 0x3, 0x1),
	EEPROMField("change USB ver.", 0x0A, 0x4, 0x1),
	EEPROMField("A rs485........", 0x0B, 0x4, 0x1),
	EEPROMField("B rs485........", 0x0B, 0x5, 0x1),
	EEPROMField("C rs485........", 0x0B, 0x6, 0x1),
	EEPROMField("D rs485........", 0x0B, 0x7, 0x1),
	EEPROMField("USB ver........", 0x0C, 0x0, 0xFFFF),
	EEPROMField("func TXDEN.....", 0x15, 0x0, 0xF),
	EEPROMField("eeprom chip....", 0x18, 0x0, 0xFF)
};

enum ConfigFields {
	kADrvVCP = 0,
	kBDrvVCP,
	kCDrvVCP,
	kDDrvVCP,
	kVendorId,
	kProductId,
	kReleaseNum,
	kChipType,
	kRemWakeup,
	kSelfPower,
	kConfDescript,
	kMaxPower,
	kInIsochron,
	kOutIsochron,
	kSuspendPull,
	kUseSerial,
	kChangeUSBVer,
	kARS485,
	kBRS485,
	kCRS485,
	kDRS485,
	kUSBVer,
	kFuncTXDEN,
	kEEPROMChip
};

static const int kConfigSize = sizeof(Config) / sizeof(EEPROMField);
static const int kEEPROMSize = 128;
static int       NeedConfig  = -1;

void PrintEEPROM() {
	for (int id = 0; id < kConfigSize; id++) {
		EEPROMField &field = Config[id];
		std::cout << "\t> " << field.name << ": ";
		if (field.value < 0) {
			std::cout << "неопределено" << std::endl;
			continue;
		}
		if (field.mask > 0x9)
			std::cout << "0x" << std::hex;
		std::cout << (int)field.value << std::endl
		          << std::dec;
	}
}

void EncodeEEPROM(unsigned char *data, size_t size) {
	for (int id = 0; id < kConfigSize; id++) {
		EEPROMField &field = Config[id];
		if (field.offs >= size) {
			std::cout << "Ошибка кодирования конфигурации: " << field.name << ": "
			                                                 << field.offs
			                                                 << std::endl;
			continue;
		}
		if (field.value == -1)
			continue;

		if (field.mask <= 0xFF) {
			uint8_t *block = reinterpret_cast<uint8_t*>(&data[field.addr]);
			*block = (*block & ~(field.mask << field.offs)) | (field.value << field.offs);
			continue;
		}

		if (field.mask <= 0xFFFF) {
			uint16_t *block = reinterpret_cast<uint16_t*>(&data[field.addr]);
			*block = (*block & ~(field.mask << field.offs)) | (field.value << field.offs);
			continue;
		}
	}
 // контрольная сумма
 uint16_t   checksum = 0xAAAA;
 uint16_t  *wdata    = reinterpret_cast<uint16_t*>(data);
 const int  kSize    = (size / 2) - 1;
 for (int i = 0; i < kSize; i++) {
   checksum = wdata[i] ^ checksum;
   checksum = (checksum << 1) | (checksum >> 15);
 }
 wdata[kSize] = checksum;
}

void DecodeEEPROM(unsigned char *data, size_t size) {
	for (int id = 0; id < kConfigSize; id++) {
		EEPROMField &field = Config[id];
		if (field.offs >= size) {
			std::cout << "Ошибка декодирования конфигурации: " << field.name << ": "
			                                                   << field.offs
			                                                   << std::endl;
			continue;
		}
		
		if (field.mask <= 0xFF) {
			uint8_t *block = reinterpret_cast<uint8_t*>(&data[field.addr]);
			field.value = (*block >> field.offs) & field.mask;
			continue;
		}

		if (field.mask <= 0xFFFF) {
			uint16_t *block = reinterpret_cast<uint16_t*>(&data[field.addr]);
			field.value = (*block >> field.offs) & field.mask;
			continue;
		}
	}
}

void ReadFTDIConfig(Ftdi::Context *ctx, int eeprom_size) {
	using namespace Ftdi;
	Eeprom ftdi_conf(ctx);
	unsigned char eeprom[eeprom_size];
	ftdi_conf.set_size(eeprom_size);
	int conf_size = ftdi_conf.size(eeprom, eeprom_size);
	std::cout << "Чтение EEPROM: " << conf_size << " байт" << std::endl;
	if (ftdi_conf.read(eeprom) < 0) {
		std::cout << "Ошибка: " << ctx->error_string() << std::endl;
		return;
	}
	std::cout << "Ок" << std::endl;
	DecodeEEPROM(eeprom, conf_size);
}

void WriteFTDIConfig(Ftdi::Context *ctx, int eeprom_size) {
	using namespace Ftdi;
	Eeprom ftdi_conf(ctx);
	unsigned char eeprom[eeprom_size];
	ftdi_conf.set_size(eeprom_size);
	int conf_size = ftdi_conf.size(eeprom, eeprom_size);
	std::cout << "Запись EEPROM: " << conf_size << " байт" << std::endl;
	// прочитаем текущую конфигурацию
//	ftdi_conf.read(eeprom);
	// обнулим
	memset(eeprom, 0, eeprom_size);
	// исправим её
	EncodeEEPROM(eeprom, conf_size);
	// сохраним
	ftdi_conf.erase();
	if (ftdi_conf.write(eeprom) < 0) {
		std::cout << "Ошибка записи!" << std::endl;
		return;
	}
	std::cout << "Новая конфигурация записана" << std::endl;
}

void PrintFTDIDevice(Ftdi::Context *ctx, int eeprom_size) {
	ReadFTDIConfig(ctx, eeprom_size);
	// Описание устройства
	std::cout << "  * Vendor.....: " << ctx->vendor()      << "\n"
	          << "  * Description: " << ctx->description() << "\n"
	          << "  * Serial.....: " << ctx->serial()      << "\n"
	          << "  * EEPROM.....: "
	          << std::endl;
	PrintEEPROM();
}

void InteractWithFTDIDevice(Ftdi::Context *ctx, struct usb_device *usb_dev) {
	std::cout << "Открытие <" << (void*)ctx << ">: ";
	int err = 0;
	if (usb_dev != 0)
		err = ctx->open(usb_dev);
	else
		err = ctx->open();
	if (err != 0) {
		std::cout << "Ошибка: [" << err << "]: " << ctx->error_string() << std::endl;
		return;
	}
	std::cout << "Ок" << std::endl;
	// Чтение конфигурации
	PrintFTDIDevice(ctx, kEEPROMSize);
	// Запись новой конфигурации	
	Config[kADrvVCP]      = 1;
	Config[kBDrvVCP]      = 1;
	Config[kCDrvVCP]      = 1;
	Config[kDDrvVCP]      = 1;
	Config[kVendorId]     = usb_dev->descriptor.idVendor;
	Config[kProductId]    = usb_dev->descriptor.idProduct;
	Config[kReleaseNum]   = 0x0;
	Config[kChipType]     = 0x08; // 7- 2232H; 8 - 4232H; 9 - 232H
	Config[kSelfPower]    = 1;
	Config[kRemWakeup]    = 0;
	Config[kConfDescript] = 1; // always 1
	Config[kMaxPower]  	  = 100 / 2; // Max power: value * 2 mA
	Config[kInIsochron]   = 0;
	Config[kOutIsochron]  = 0;
	Config[kSuspendPull]  = 0;
	Config[kUseSerial]    = 1;
	Config[kChangeUSBVer] = 0;
	Config[kUSBVer]       = 0x0200;
	Config[kARS485]       = 1;
	Config[kBRS485]       = 1;
	Config[kCRS485]       = 1;
	Config[kDRS485]       = 1;
	Config[kEEPROMChip]   = 0x46; // EEPROM Type 0x46 for 93xx46, 0x56 for 93xx56 and 0x66 for 93xx66
	if (usb_dev->devnum == NeedConfig) {
		WriteFTDIConfig(ctx, kEEPROMSize);
		std::cout << "Проверка конфигурации..." << std::endl;
		PrintFTDIDevice(ctx, kEEPROMSize);
	}
	/*std::cout << "Установка режимов: " << std::endl;
	// HIGH/ON value configures a line as output.
	//         TX RX RTS CTS DTR DSR DCD EX_TX
	// D[0-7]: 1  0  1   0   1   0   0   1
 	const unsigned char kPinsMask = 0xFF;//0x95;
 	const unsigned char kBitMode  = BITMODE_RESET;
	std::cout << "\t * Mask: " << std::hex << "0x" << (int)kPinsMask << std::dec << "\n"
	          << "\t * Mode: " << (int)kBitMode << std::endl;
	ctx->reset();
	std::cout << "\t * A: ";
	ctx->set_interface(INTERFACE_A);
	ctx->set_bitmode(kPinsMask, kBitMode);
	std::cout << "OK" << std::endl;
	std::cout << "\t * B: ";
	ctx->set_interface(INTERFACE_B);
	ctx->set_bitmode(kPinsMask, kBitMode);
	std::cout << "OK" << std::endl;
	std::cout << "\t * C: ";
	ctx->set_interface(INTERFACE_C);
	ctx->set_bitmode(kPinsMask, kBitMode);	
	std::cout << "OK" << std::endl;
	std::cout << "\t * D: ";
	ctx->set_interface(INTERFACE_D);
	ctx->set_bitmode(kPinsMask, kBitMode);	
	std::cout << "OK" << std::endl;
	ctx->close();*/
}

void PrintAllFTDIDevices(int vendor, int product) {
	using namespace Ftdi;
	std::cout << "Поиск FTDI контроллеров:" << std::endl;
	List *ftdi_devs = List::find_all(vendor, product);
	for (List::iterator it = ftdi_devs->begin(); it != ftdi_devs->end(); it++)
		PrintFTDIDevice(&*it, kEEPROMSize);
	delete ftdi_devs;
}

void ScanUSBBus(int vendor, int product) {
	std::cout << "Сканирование USB шины:" << std::endl;
	usb_init();
	int bus_num = usb_find_busses();
	int dev_num = usb_find_devices();
	std::cout << " * Количество USB шин......: " << bus_num << std::endl;
	std::cout << " * Количество USB устройств: " << dev_num << std::endl;
	struct usb_bus *busses = usb_get_busses();
	for (struct usb_bus *bus = busses; bus; bus = bus->next) {
		std::cout << " * BUS [" << bus->location << "]: " << bus->dirname << std::endl;
		struct usb_device *dev;
		for (dev = bus->devices; dev; dev = dev->next) {
			if (NeedConfig != -1 & NeedConfig != dev->devnum)
				continue;
			char str_serial[512];
			usb_dev_handle *hndl = usb_open(dev);
			bool serial_ok = usb_get_string_simple(hndl, dev->descriptor.iSerialNumber, str_serial, 512) > 0;
			std::cout << " --------------------------------------------------------\n"
				        << "   * DEV [" << (unsigned)dev->devnum << "]: "<< dev->filename << "\n"
				        << "\t- Class........: " << (unsigned)dev->descriptor.bDeviceClass  << "\n"
				        << "\t- iManufacturer: " << (unsigned)dev->descriptor.iManufacturer << "\n"
				        << "\t- iProduct.....: " << (unsigned)dev->descriptor.iProduct      << "\n"
				        << "\t- Serial number: " << (serial_ok ? str_serial : "Ошибка")      << "\n"
				        << "\t- Vendor.......: 0x" << std::hex << (unsigned)dev->descriptor.idVendor  << "\n"
				        << "\t- idProduct....: 0x" << std::hex << (unsigned)dev->descriptor.idProduct << "\n"
				        << std::dec;
			usb_close(hndl);
			// Начинаем работу с найденым устройством
			if (dev->descriptor.idVendor  == vendor &&
			    dev->descriptor.idProduct == product) {
				Ftdi::Context ftdi_ctx;
				InteractWithFTDIDevice(&ftdi_ctx, dev);
			}
		}
	}
}

int main(int argc, char **argv) {
	int vendor  = 0x0403;
	int product = 0x6011;
	int t_arg   = 0;
	if (argc > 2) {
		if ((t_arg = strtol(argv[1], 0, 16)) >= 0)
			vendor = t_arg;
		if ((t_arg = strtol(argv[2], 0, 16)) >= 0)
			product = t_arg;
	}
	if (argc > 3 && (t_arg = strtol(argv[3], 0, 10)) >= 0)
		NeedConfig = t_arg;
	std::cout << "Входные данные:\n"
	          << " * Vendor.: " << std::hex << vendor  << "\n"
	          << " * Product: " << std::hex << product << "\n"
	          << std::dec
	          << std::endl;
	ScanUSBBus(vendor, product);
//	PrintAllFTDIDevices(vendor, product);
	return 0;
}
