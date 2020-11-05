#include <stdio.h>
#include "serial.h"

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
void ringbuf_init(RINGBUF_T *rb) {
	rb->buf[0] = 0;
	rb->bufC = 0;
	rb->bufIn = 0;
	rb->bufOut = 0;
}

// ---------------------------------------------------------------------
int ringbuf_in(RINGBUF_T *rb, char c) {
	if (rb->bufC < RINGBUF_SZ) {
		rb->bufC++;
		rb->buf[rb->bufIn] = c;
		rb->bufIn = (rb->bufIn+1) % RINGBUF_SZ;
		return 1;
	}
	return 0;
}

// ---------------------------------------------------------------------
char ringbuf_out(RINGBUF_T *rb) {
	char c = 0;
	if (rb->bufC > 0) {
		rb->bufC--;
		c = rb->buf[rb->bufOut];
		rb->bufOut = (rb->bufOut+1) % RINGBUF_SZ;
		return c;
	}
	return 0;
}

// ---------------------------------------------------------------------
int ringbuf_available(RINGBUF_T *rb) {
	return rb->bufC;
}

void ringbuf_dump(RINGBUF_T *rb) {
	printf(" count: %d,", rb->bufC);
	printf(" in: %d,", rb->bufIn);
	printf(" out: %d,", rb->bufOut);
	printf(" unread: ");

	int t = rb->bufOut;
	for (int i = 0; i < rb->bufC; i++) {
		char c = rb->buf[t];
		printf("%02x (%c) ", c, (c >= ' ') ? c : '.');
		t = (t+1) % RINGBUF_SZ;
	}
	printf("\n");
}

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
void Serial_init(SERIAL_T *port) {
	ringbuf_init(&port->leftSide);
	ringbuf_init(&port->rightSide);
}

// ---------------------------------------------------------------------
int Serial_write(SERIAL_T *port, char c, int isLTOR) {
	if (isLTOR == SERIAL_LTOR) {
		return ringbuf_in(&port->rightSide, c);
	} else {
		return ringbuf_in(&port->leftSide, c);
	}
}

// ---------------------------------------------------------------------
char Serial_read(SERIAL_T *port, int isLEFT) {
	if (isLEFT) {
		return ringbuf_out(&port->leftSide);
	} else {
		return ringbuf_out(&port->rightSide);
	}
}

// ---------------------------------------------------------------------
int Serial_available(SERIAL_T *port, int isLEFT) {
	if (isLEFT) {
		return ringbuf_available(&port->leftSide);
	} else {
		return ringbuf_available(&port->rightSide);
	}
}

void Serial_status(SERIAL_T *port) {
	printf("Left side\n");
	ringbuf_dump(&port->leftSide);
	printf("Right side\n");
	ringbuf_dump(&port->rightSide);
}