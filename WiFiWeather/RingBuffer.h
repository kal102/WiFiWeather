/*
 * RingBuffer.h
 *
 * Created: 29.11.2017 20:28:30
 *  Author: Lukasz
 */ 


#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <stdint.h>
#include <stdbool.h>
#define BUFFER_MAXSIZE 350
typedef char cb_scalar;

enum bufferResult {BUFFER_FULL, BUFFER_OK};

typedef struct
{
	cb_scalar elements[BUFFER_MAXSIZE];
	uint16_t Begin;
	uint16_t Count;
} CircBuffer;

_Bool cbIsEmpty(CircBuffer *cb);
_Bool cbIsFull(CircBuffer *cb);
void cbClear(CircBuffer *cb);
enum bufferResult cbAdd(CircBuffer *cb, cb_scalar value);
cb_scalar cbRead(CircBuffer *cb);

#endif /* RINGBUFFER_H_ */