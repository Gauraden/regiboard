#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/input.h>


void write_event(int fd, struct input_event ev)
{
	write(fd, &ev, sizeof(struct input_event));
}


int main(int argc, char **argv)
{
	int fd = -1;
	struct input_event ev;
	char def[] = "/dev/event1";
	char *fname;

	if (argc > 1)
		fname = argv[1];
	else
		fname = def;

	ev.type = EV_SND;
	ev.code = SND_TONE;

	fd = open(fname, O_WRONLY);
	if (fd == -1) {
		perror("open()");
		exit(1);
	}

	ev.value = 750;
	write_event(fd, ev);

	usleep(10000); // 10000

	ev.value = 0;
	write_event(fd, ev);

	close(fd);

	return 0;
}
