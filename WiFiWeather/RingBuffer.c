/*
 * RingBuffer.c
 *
 * Created: 29.11.2017 20:31:15
 *  Author: Lukasz
 */ 

#include <stdint.h>
#include <stdbool.h>
#include <util/atomic.h>
#include "RingBuffer.h"

inline _Bool cbIsEmpty(CircBuffer *cb)
{
	return (cb->Count == 0);
}

inline _Bool cbIsFull(CircBuffer *cb)
{
	return (cb->Count == BUFFER_MAXSIZE);
}

void cbClear(CircBuffer *cb)
{
	while (!cbIsEmpty(cb))
	{
		cbRead(cb);
	}
}

enum bufferResult cbAdd(CircBuffer *cb, cb_scalar value)
{
	uint16_t cb_end;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if (cbIsFull(cb)) return BUFFER_FULL;
		cb_end = (cb->Begin + cb->Count) % BUFFER_MAXSIZE;
		cb->elements[cb_end] = value;
		(cb->Count)++;
	}
	return BUFFER_OK;
}

cb_scalar cbRead(CircBuffer *cb)
{
	cb_scalar element;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		if (cbIsEmpty(cb)) return NULL;
		element = cb->elements[cb->Begin];
		cb->Begin = (cb->Begin + 1) % BUFFER_MAXSIZE;
		(cb->Count)--;
	}
	return element;
}

