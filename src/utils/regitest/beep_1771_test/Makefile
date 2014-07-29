CC=gcc
RM=rm -f
CFLAGS=-Wall
TARGET=regi_beep
SOURCES=regi_beep.c


all: $(TARGET)


clean:
	$(RM) *.o
	$(RM) *.core


distclean: clean
	$(RM) $(TARGET)


$(TARGET): $(SOURCES:.c=.o)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES:.c=.o)

