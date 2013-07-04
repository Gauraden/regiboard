#include <iostream>
#include <cstdio>
#include <termios.h>
#include <fcntl.h>
#include "sys/ioctl.h"
#include "linux/ioctl.h"
#include "linux/serial.h"
#include <string>
#include <sstream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstring>

typedef int FileHandle;

static const unsigned kUartAmount = 8;
static const unsigned kBuffSize   = 32;

struct StatisticRec {
	StatisticRec(): usec(0.0), amount(0) {}	
	double   usec;
	unsigned amount;
};

struct RWStatisticRec {
	StatisticRec write;
	StatisticRec read;
};

static FileHandle     fhndls[kUartAmount];
static RWStatisticRec stats[kUartAmount];

unsigned GetCharacterSize(const struct termios& term) {
	switch (term.c_cflag & CSIZE) {
		case CS5: return 5;
		case CS6: return 6;
		case CS7: return 7;
		case CS8: return 8;
	};
}

int ParseSpeed(speed_t speed) {
	switch (speed) {
		case B0:     return 0;
		case B50:    return 50;
		case B75:    return 75;
		case B110:   return 110;
		case B134:   return 134;
		case B150:   return 150;
		case B200:   return 200;
		case B300:   return 300;
		case B600:   return 600;
		case B1200:  return 1200;
		case B1800:  return 1800;
		case B2400:  return 2400;
		case B4800:  return 4800;
		case B9600:  return 9600;
		case B19200: return 19200;
		case B38400: return 38400;
		default:     return -1;
	}
}

void GetSerialInfo(FileHandle hndl) {
	struct termios tios;
	
	tcgetattr(hndl, &tios);
	
	const unsigned kStopBits = (tios.c_cflag & CSTOPB) ? 2 : 1;
	const bool     kFlowCntl = tios.c_cflag & CRTSCTS;
	const int      kInSpeed  = ParseSpeed(cfgetispeed(&tios));
	const int      kOutSpeed = ParseSpeed(cfgetospeed(&tios));
	
	std::cout << "\t In speed : " << (kInSpeed / 8)             << " Bytes/sec"
			  << " (" << kInSpeed << " bps);\n"
	          << "\t Out speed: " << (kOutSpeed / 8)            << " Bytes/sec"
	          << " (" << kOutSpeed << " bps);\n"
	          << "\t Char size: " << GetCharacterSize(tios)     << ";\n"
	          << "\t Stop bits: " << kStopBits                  << ";\n"
	          << "\t Flow cntl: " << (kFlowCntl ? "on" : "off") << ";\n"
	          << std::endl;
}

void OpenAllUART() {
	std::cout << "Opening " << kUartAmount
	          << " tty devices..." << std::endl;
	for (unsigned i = 0; i < kUartAmount; i++) {
		std::stringstream tmp;
		tmp << "/dev/ttyUSB" << i;
		fhndls[i] = open(tmp.str().c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
		if (fhndls[i] != 0) {
			std::cout << "Device: ttyUSB" << i << std::endl;
			//ioctl(fhndls[i], TCFLSH, TCIOFLUSH);
			//GetSerialInfo(fhndls[i]);
		}
	}
	std::cout << "---------------------------------------" << std::endl;
}

void CloseAllUART() {
	std::cout << "Closing tty devices..." << std::endl;
	for (unsigned i = 0; i < kUartAmount; i++) {
		if (fhndls[i] != 0)
			close(fhndls[i]);
	}
}

void WriteToAllUART() {
	char kBuff[kBuffSize];
	kBuff[kBuffSize - 1] = '\r'; // carriage return 13
	for (unsigned i = 0; i < kUartAmount; i++) {
		if (fhndls[i] == 0)
			continue;
		// запись
		const int kWClk    = 0;//clock();
		const int kWResult = write(fhndls[i], kBuff, kBuffSize);
		const int kWDiff   = 0;//clock() - kWClk;
		stats[i].write.usec += ((double)kWDiff) / CLOCKS_PER_SEC;
		stats[i].write.amount++;
		// чтение
		/*
		const int kRClk    = clock();
		const int kRResult = read(fhndls[i], kBuff, kBuffSize);
		const int kRDiff   = clock() - kRClk;
		stats[i].read.usec += ((double)kRDiff) / CLOCKS_PER_SEC;
		stats[i].read.amount++;	
		*/	
		const int kRResult = 0;
		const int kRDiff   = 0;
		printf("№%u \t: Wclocks = %d; Wusec = %lf; Wamount = %u; Wresult = %d; Rclocks = %d; Rusec = %lf; Ramount = %u; Rresult = %d\n", 
			i,
			kWDiff,
			stats[i].write.usec,
			stats[i].write.amount,
			kWResult,
			kRDiff,
			stats[i].read.usec,
			stats[i].read.amount,
			kRResult);
		if (kWResult != kBuffSize)
			printf("№%u \t: не удалось записать все данные: %d из %u\n",
				i, kWResult, kBuffSize);
		if (kRResult != kBuffSize)
			printf("№%u \t: не удалось прочитать все данные: %d из %u\n",
				i, kRResult, kBuffSize);
	}	
}

void ReadFromAllUART() {
	
}

void PrintStatistics() {
	// перевод курсора
//	printf("\033[s");
	printf("\n---------------------------------------------------\n");
	for (unsigned i = 0; i < kUartAmount; i++) {
		// очистка строки
//		printf("\033[2K");
		printf("ttyUSB%u: ", i);
		if (stats[i].write.amount != 0 && stats[i].read.amount != 0) {
			const double kAverWTime = stats[i].write.usec / stats[i].write.amount;
			const double kAverRTime = stats[i].read.usec / stats[i].read.amount;
			printf("W[%7u b/s] R[%7lu b/s] ", (unsigned long)((double)kBuffSize / kAverWTime),
			                                 (unsigned long)((double)kBuffSize / kAverRTime));
		}
		if (((i + 1) % 4) == 0)
			printf("\n");
		else
			printf(" ");
	}
	printf("\n---------------------------------------------------\n");
}

int main(int argc, char *argv[]) {
	/*
	int repeats_num = 1;
    if (argc > 1)
		repeats_num = atoi(argv[1]);
	std::cout << "Число повторов: " << repeats_num << std::endl;			
	OpenAllUART();
	for (int i = 0; i < repeats_num; i++) {
		std::cout << "Итерация №" << i << std::endl;
		WriteToAllUART();
	}
	PrintStatistics();
	CloseAllUART();
	std::cout << "Exit..." << std::endl;
	return 0;
	*/
// ---------------------------------------------------------------------
    std::cout << "Let's test dev: " << argv[1] << std::endl;
    std::string out_buf("test message");
    if (argc > 2)
		out_buf = argv[2];
    std::vector<char> in_buf(out_buf.size() + 1);
    //O_RDWR | O_NDELAY | O_NOCTTY );
    FileHandle fhndl = open(argv[1], O_RDWR | O_NOCTTY | O_NDELAY);
	if (fhndl == 0)
		return 0;
	GetSerialInfo(fhndl);
//	ioctl(fhndl, TCFLSH, TCIOFLUSH);

	out_buf += "\r";	
//	std::vector<char> packet(out_buf.size() + 1);
//	memcpy(packet.data(), out_buf.c_str(), out_buf.size());
//	packet.data()[out_buf.size()] = '\r';
//	const int WResult = write(fhndl, packet.data(), packet.size());
	std::cout << "writing..." << std::endl;
	const int WResult = write(fhndl, out_buf.c_str(), out_buf.size());
	std::cout << "size: " << WResult << "/" << out_buf.size() <<  "; "
	          << "data: " << out_buf << std::endl;
/*
	int in_bytes, out_bytes;
	ioctl(fhndl, TIOCINQ,  &in_bytes);
	ioctl(fhndl, TIOCOUTQ, &out_bytes);
	std::cout << "in: "  << in_bytes  << " bytes; " 
	          << "out: " << out_bytes << " bytes; "
	          << std::endl;
*/
	std::cout << "reading..." << std::endl;
	const int RResult = read(fhndl, in_buf.data(), in_buf.size());
	std::cout << "size: " << RResult << "/" << in_buf.size() << "; "
	          << "data: " << in_buf.data() << std::endl;
	close(fhndl);
    return 0;
}
