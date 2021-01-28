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
#include <stdlib.h>

struct header {
	uint32_t msgno;
	uint32_t len;
};

#define BASE_ADDR 0x1000
#define COUNTER_REQ BASE_ADDR + 0x1
struct counter_req {
	struct header header;
};

#define COUNTER_RSP BASE_ADDR + 0x2
struct counter_rsp {
	struct header header;
	uint32_t value;
};

union usb_packets {
	struct header header;
	struct counter_req counter_req;
	struct counter_rsp counter_rsp;
};


char dev[] = "/dev/ttyACM0";

struct usb_data {
	int fd;
};

static void *flush_fd(void *in)
{
	struct usb_data *data = in;
	uint8_t read_data[512];

	while(1) {
		read(data->fd, read_data, sizeof(read_data));
	}

	return NULL;

}

static void flush_data(struct usb_data *data)
{
	pthread_t thread_id;
	pthread_create(&thread_id, NULL, flush_fd, data);
	/* just wait... */
	usleep(1000); /* 1ms */
	pthread_cancel(thread_id);
}

static void *com_listener(void *in)
{
	struct usb_data *data = in;
	int ret;

	union usb_packets rsp;

	int payload_sz = 0;
	uint8_t *rsp_buffer;


	while(1) {
		/* header is fetched*/
		ret = read(data->fd, &rsp, sizeof(struct header));
		if(ret == 0) {
			printf("Incorrect data...\n");
			continue;
		}

		/* The packet is larger than our union... bail out.. */
		if(rsp.header.len > sizeof(rsp)) abort();

		payload_sz = sizeof(rsp.counter_rsp) - sizeof(rsp.header);

		rsp_buffer = (uint8_t *)&rsp;
		rsp_buffer += sizeof(rsp.header);
		ret = read(data->fd, rsp_buffer, payload_sz);

		switch(rsp.header.msgno) {
		case COUNTER_RSP:
			printf("value: %d\n", rsp.counter_rsp.value);
			break;
		default:
			break;

		}

	}
	return NULL;
}

int main(int argc, char **argv) {
	int ret;
	struct usb_data data;
	union usb_packets req;
	int send_sz = 0;
	pthread_t thread_id;

	data.fd = open(dev, O_RDWR | O_NOCTTY);
	if(data.fd < 1) {
		printf("Couldn't open the fd\n");
		return -1;
	}

	flush_data(&data);

	pthread_create(&thread_id, NULL, com_listener, &data);

	req.header.msgno = COUNTER_REQ;
	send_sz = sizeof(req.counter_req);

	for(int i = 0 ; i < 5; i++) {
		usleep(10000);
		ret = write(data.fd, &req, send_sz);
		if(ret != send_sz) {
			printf("Write failed\n");
			return -1;
		}
	}

	usleep(10000);
	pthread_cancel(thread_id);
	pthread_join(thread_id, NULL);

}
