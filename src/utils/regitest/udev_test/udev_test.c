#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>

typedef unsigned long long int SeqNum;
static SeqNum last_entry_val = 0;

void parse_device(struct udev_monitor *mon) {
	struct udev_device *dev      = udev_monitor_receive_device(mon);
	const char         *kDevNode = udev_device_get_devnode(dev);
	if (kDevNode) {
		printf(" > Got Device\n");
		printf("   Node     : %s\n", kDevNode);
		printf("   DevPath  : %s\n", udev_device_get_devpath(dev));
		printf("   Subsystem: %s\n", udev_device_get_subsystem(dev));
		printf("   Devtype  : %s\n", udev_device_get_devtype(dev));
		printf("   Action   : %s\n", udev_device_get_action(dev));
		printf("   Driver   : %s\n", udev_device_get_driver(dev));
		printf("   SysName  : %s\n", udev_device_get_sysname(dev));
  	printf("   SysPath  : %s\n", udev_device_get_syspath(dev));
  	printf("   SysNum   : %s\n", udev_device_get_sysnum(dev));
  	
  	struct udev_list_entry *list_entry;  	
  	printf("   DevLinks.: \n");
		udev_list_entry_foreach(list_entry, udev_device_get_devlinks_list_entry(dev))
			printf("    > '%s' [%s]\n", udev_list_entry_get_name(list_entry),
				                          udev_list_entry_get_value(list_entry));
  	printf("   Properts.: \n");
		udev_list_entry_foreach(list_entry, udev_device_get_properties_list_entry(dev))
			printf("    > '%s' [%s]\n", udev_list_entry_get_name(list_entry),
				                          udev_list_entry_get_value(list_entry));
				                          
  	printf("   Tags.....: \n");
		udev_list_entry_foreach(list_entry, udev_device_get_tags_list_entry(dev))
			printf("    > '%s' [%s]\n", udev_list_entry_get_name(list_entry),
				                          udev_list_entry_get_value(list_entry));
	}
	udev_device_unref(dev);
}

int parse_queue(struct udev_queue *udev_queue) {
  SeqNum                  seqnum;
  struct udev_list_entry *list_entry;

//  udev_queue = udev_queue_new(udev);
//  if (udev_queue == NULL)
//        return -1;
//  seqnum = udev_queue_get_kernel_seqnum(udev_queue);
//  printf("seqnum kernel: %llu\n", seqnum);
  if (udev_queue_get_queue_is_empty(udev_queue))
  	return 0;
    //printf("queue is empty\n");
//  seqnum = udev_queue_get_udev_seqnum(udev_queue);
//  printf("seqnum udev: %llu\n", seqnum);
/*  if (!udev_queue_get_seqnum_is_finished(udev_queue, seqnum))
  	return;
  	*/
//  printf("Queue:\n");
  udev_list_entry_foreach(list_entry, udev_queue_get_queued_list_entry(udev_queue)) {
  	const SeqNum kEntryVal = udev_list_entry_get_value(list_entry);
  	if (kEntryVal != last_entry_val)
			printf(" > '%s' [%s]\n", udev_list_entry_get_name(list_entry),
				                       kEntryVal);
		last_entry_val = kEntryVal;
	}
	/*
  printf("\n");
  printf("get queue list again\n");
  udev_list_entry_foreach(list_entry, udev_queue_get_queued_list_entry(udev_queue))
  printf("queued: '%s' [%s]\n", udev_list_entry_get_name(list_entry),
                                udev_list_entry_get_value(list_entry));
  printf("\n");*/
//  printf("get failed list\n");
//  udev_list_entry_foreach(list_entry, udev_queue_get_failed_list_entry(udev_queue))
//  printf("failed: '%s'\n", udev_list_entry_get_name(list_entry));
//  printf("\n");
/*
  list_entry = udev_queue_get_queued_list_entry(udev_queue);
  if (list_entry != NULL) {
        printf("event [%llu] is queued\n", seqnum);
        seqnum = strtoull(udev_list_entry_get_value(list_entry), NULL, 10);
        if (udev_queue_get_seqnum_is_finished(udev_queue, seqnum))
              printf("event [%llu] is not finished\n", seqnum);
        else
              printf("event [%llu] is finished\n", seqnum);
  }*/
//  printf("\n");
//  udev_queue_unref(udev_queue);
  return 1;
}

void test_udev_queue(void) {
	struct udev         *udev;
	struct udev_monitor *mon;
	struct udev_queue   *queue;
	int                  fd;
	
	printf("Regiboard implementation! Udev queue\n");
	udev  = udev_new();
	mon   = udev_monitor_new_from_netlink(udev, "udev");
	queue = udev_queue_new(udev);
	udev_monitor_enable_receiving(mon);
	fd = udev_monitor_get_fd(mon);
	while (1) {
		fd_set         fds;
		struct timeval tv;
		int            ret;		
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		tv.tv_sec  = 0;
		tv.tv_usec = 100;
		ret = select(fd + 1, &fds, NULL, NULL, &tv);
		if (ret > 0 && FD_ISSET(fd, &fds)/* && parse_queue(queue) == 1*/)
			parse_device(mon);
		/*
		dev = udev_monitor_receive_device(mon);
		const char *kDevNode = udev_device_get_devnode(dev);
		if (kDevNode) {
			printf("Got Device\n");
			printf("   Node: %s\n",      kDevNode);
			printf("   Subsystem: %s\n", udev_device_get_subsystem(dev));
			printf("   Devtype: %s\n",   udev_device_get_devtype(dev));
			printf("   Action: %s\n",    udev_device_get_action(dev));
			printf("   Driver: %s\n",    udev_device_get_driver(dev));
			printf("   SysName: %s\n",   udev_device_get_sysname(dev));
		}
		udev_device_unref(dev);
		*/
	}
	udev_queue_unref(queue);
	udev_monitor_unref(mon);
	udev_unref(udev);
}

int main (void) {
/*
	struct udev         *udev;
	struct udev_monitor *mon;
	int                  fd;
	struct udev_device  *dev;
	printf("Regiboard implementation!\n");
	// Create the udev object
	udev = udev_new();
	if (!udev) {
		printf("Can't create udev\n");
		exit(1);
	}
	mon = udev_monitor_new_from_netlink(udev, "udev");
	udev_monitor_enable_receiving(mon);
	fd = udev_monitor_get_fd(mon);
	
	while (1) {
		// Set up the call to select(). In this case, select() will
		//   only operate on a single file descriptor, the one
		//   associated with our udev_monitor. Note that the timeval
		//   object is set to 0, which will cause select() to not
		//   block. 
		fd_set         fds;
		struct timeval tv;
		int            ret;		
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		tv.tv_sec  = 1;
		tv.tv_usec = 0;
		printf("Select...\n");
		ret = select(fd + 1, &fds, NULL, NULL, &tv);
		// Check if our file descriptor has received data.
		if (ret > 0 && FD_ISSET(fd, &fds)) {
			//printf("\nselect() says there should be data\n");			
			// Make the call to receive the device.
			//   select() ensured that this will not block.
			dev = udev_monitor_receive_device(mon);
			const char *kDevNode = udev_device_get_devnode(dev);
			if (dev && kDevNode) {
				printf("Got Device\n");
				printf("   Node: %s\n",      kDevNode);
				printf("   Subsystem: %s\n", udev_device_get_subsystem(dev));
				printf("   Devtype: %s\n",   udev_device_get_devtype(dev));
				printf("   Action: %s\n",    udev_device_get_action(dev));
				printf("   Driver: %s\n",    udev_device_get_driver(dev));
				printf("   SysName: %s\n",   udev_device_get_sysname(dev));
				udev_device_unref(dev);
			} else {
				//printf("No Device from receive_device(). An error occured.\n");
			}					
		}
		//usleep(250 * 1000);
		printf("Sleep...\n");
		sleep(5);
		//printf(".");
		fflush(stdout);
	}
	udev_unref(udev);
	*/
	test_udev_queue();
	return 0;
}

