#ifndef ACKQUEUE_H__
#define ACKQUEUE_H__ 1
#include "miranda.h"

typedef struct {
	int type;				// ACKTYPE_*
	HANDLE hContact;		// who its for
	HANDLE hSeq;			// what seq was given
	// key hContact, type
} ACKINFO;

typedef struct {
	ACKINFO queue[200];
	unsigned int end;
} ACKQUEUE;

void AckQueue_Create(ACKQUEUE * q);
void AckQueue_Destroy(ACKQUEUE * q);
void AckQueue_Insert(ACKQUEUE * q, HANDLE hContact, int type, HANDLE hSeq);
int AckQueue_Remove(ACKQUEUE * q, HANDLE hContact, int type, ACKINFO * p);

#endif // ACKQUEUE_H__



