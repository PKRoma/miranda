/*

Copyright 2005 Sam Kothari (egoDust)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "ackqueue.h"

void AckQueue_Create(ACKQUEUE * q)
{
	ZeroMemory(q, sizeof(ACKQUEUE));
}

void AckQueue_Destroy(ACKQUEUE * q)
{
}

void AckQueue_Insert(ACKQUEUE * q, HANDLE hContact, int type, HANDLE hSeq)
{
	if ( q->end > sizeof(q->queue)/sizeof(q->queue[0]) ) {
		return;
	}
	ACKINFO * p = &q->queue[q->end++];
	p->hContact=hContact;
	p->type=type;
	p->hSeq=hSeq;
}

// returns 1 if somethin was popped
int AckQueue_Remove(ACKQUEUE * q, HANDLE hContact, int type, ACKINFO * p)
{	
	int j;
	for (j=0 ; j < q->end; j++) {
		if ( q->queue[j].hContact == hContact && q->queue[j].type == type ) {
			memmove(p, &q->queue[j], sizeof(ACKINFO));
			memmove(&q->queue[j], &q->queue[j+1], sizeof(ACKINFO) * ( --q->end - j ) );
			return 1;
		}
	}
	return 0;
}
