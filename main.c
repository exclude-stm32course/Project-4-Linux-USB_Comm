/*
 * main.c
 *
 *  Created on: Dec 26, 2020
 *      Author: evsejho
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <pthread.h>

#define BASE_ADDR 0x1000
#define COUNTER_REQ BASE_ADDR + 0x1
struct counter_req {
	uint32_t header;
};

#define COUNTER_RSP BASE_ADDR + 0x2
struct counter_rsp {
	uint32_t header;
	uint32_t value;
};

union usb_packets {
	uint32_t header;
	struct counter_req counter_req;
	struct counter_rsp counter_rsp;
};

char dev[] = "/dev/ttyACM0";

struct usb_data {
	int fd;
};

void *com_listener(void *in) {
	struct usb_data *data = in;
	int ret;

	union usb_packets rsp;


	while(1) {
		ret = read(data->fd, &rsp, sizeof(rsp));
		if(ret == 0) {
			printf("Incorrect data...\n");
			continue;
		}

		printf("value: %d\n", rsp.counter_rsp.value);
	}
	return NULL;
}

int main(int argc, char **argv) {
	int ret;
	struct usb_data data;
	union usb_packets req;

	data.fd = open(dev, O_RDWR | O_NOCTTY);
	if(data.fd < 1) {
		printf("Couldn't open the fd\n");
		return -1;
	}

	pthread_t thread_id;
	pthread_create(&thread_id, NULL, com_listener, &data);

	req.header = COUNTER_REQ;


	for(int i = 0 ; i < 5; i++) {
		usleep(10000);
		ret = write(data.fd, &req, sizeof(req));
		if(ret != sizeof(req)) {
			printf("Write failed\n");
			return -1;
		}
	}

	usleep(10000);
	pthread_cancel(thread_id);
	pthread_join(thread_id, NULL);

}
