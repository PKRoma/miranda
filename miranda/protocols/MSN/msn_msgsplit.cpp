/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2007 Boris Krasnovskiy.

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

#include "msn_global.h"


class chunkedmsg
{
private:
	char* id;
	char* msg;
	size_t size;
	size_t recvsz;
	bool bychunk;

public:
	chunkedmsg(const char* tid, const size_t totsz, const bool bychunk);
	~chunkedmsg();

	void add(const char* msg, const size_t offset, const size_t portion);
	bool compare(const char* tid);
	bool get(char*& tmsg, size_t& tsize);
};


inline chunkedmsg::chunkedmsg(const char* tid, const size_t totsz, const bool tbychunk)
	: bychunk(tbychunk), recvsz(0), size(totsz)
{
	id = mir_strdup(tid);
	msg = tbychunk ? NULL : (char*)mir_alloc(totsz + 1); 
}

inline chunkedmsg::~chunkedmsg()
{
	mir_free(id);
	mir_free(msg);
}

void chunkedmsg::add( const char* tmsg, const size_t offset, const size_t portion )
{
	if (bychunk) 
	{
		size_t oldsz = recvsz;
		recvsz += portion;
		msg  = (char*)mir_realloc(msg, recvsz + 1);
		memcpy( msg + oldsz, tmsg, portion );
		--size;
	}
	else {
		memcpy(msg + offset, tmsg, portion); 
		size_t newsz = offset + portion;
		if (newsz > recvsz) recvsz = newsz; 
	}
}

bool chunkedmsg::compare(const char* tid)
{ 
	return strcmp(id, tid) == 0;
}

bool chunkedmsg::get(char*& tmsg, size_t& tsize)
{
	bool alldata = bychunk ? size == 0 : recvsz == size;
	if (alldata) 
	{ 
		msg[recvsz] = 0;
		tmsg = msg; 
		tsize = recvsz;
		msg = NULL; 
	}

	return alldata;
}

static int CompareID(const chunkedmsg* p1, const chunkedmsg* p2)
{
	return p2 - p1;
}

static LIST<chunkedmsg> msgCache(10, CompareID);


int findChunky(const char* id)
{
	int res = -1;
	for ( int i = 0; i < msgCache.getCount(); ++i )
		if ( msgCache[i]->compare( id )) 
		{
			res = i; 
			break;
		}
	return res;
}

int addCachedMsg(const char* id, const char* msg, const size_t offset,
				 const size_t portion, const size_t totsz, const bool bychunk)
{
	chunkedmsg* chunky;

	int idx = findChunky(id);
	if ( idx == -1 ) 
	{
		chunky = new chunkedmsg(id, totsz, bychunk);
		msgCache.insert(chunky);
		idx = findChunky(id);
	}
	else
		chunky = msgCache[idx];

	chunky->add(msg, offset, portion);

	return idx;
}

bool getCachedMsg(int idx, char*& msg, size_t& size)
{
	bool res = msgCache[idx]->get(msg, size);
	if (res)
	{
		delete msgCache[idx]; 
		msgCache.remove(idx);
	}

	return res;
}

bool getCachedMsg(const char* id, char*& msg, size_t& size)
{
	int idx = findChunky(id);
	return idx != -1 && getCachedMsg(idx, msg, size);
}


void clearCachedMsg(int idx)
{
	if (idx != -1)
	{
		delete msgCache[idx]; 
		msgCache.remove(idx);
	}
	else
		for ( int i=0; i < msgCache.getCount(); i++ ) 
		{
			delete msgCache[i]; 
			msgCache.remove(i);
		}
}

void CachedMsg_Uninit(void)
{
	clearCachedMsg();
	msgCache.destroy();
}
