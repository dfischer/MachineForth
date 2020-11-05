// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
#ifndef __SERIAL_H__
#define __SERIAL_H__

#define RINGBUF_SZ 1024
#define STX 2
#define ETX 3

typedef struct {
	int bufC;
	int bufIn;
	int bufOut;
	char buf[RINGBUF_SZ];
} RINGBUF_T;

// ---------------------------------------------------------------------
void ringbuf_init(RINGBUF_T *rb);
int ringbuf_in(RINGBUF_T *rb, char c);
char ringbuf_out(RINGBUF_T *rb);
int ringbuf_available(RINGBUF_T *rb);
void ringbuf_dump(RINGBUF_T *rb);

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
#define SERIAL_LTOR 1
#define SERIAL_RTOL 0

typedef struct {
	RINGBUF_T leftSide;
	RINGBUF_T rightSide;
} SERIAL_T;

// ---------------------------------------------------------------------
void Serial_init(SERIAL_T *port);
int Serial_write(SERIAL_T *port, char c, int isLTOR);
char Serial_read(SERIAL_T *port, int isLEFT);
int Serial_available(SERIAL_T *port, int isLEFT);
void Serial_status(SERIAL_T *port);

#endif
