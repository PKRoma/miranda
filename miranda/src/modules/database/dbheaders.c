/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

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

#include "../../core/commonheaders.h"
#include "database.h"

extern struct DBHeader dbHeader;
extern HANDLE hDbFile;

struct DBSignature {
  char name[15];
  BYTE eof;
};
static struct DBSignature dbSignature={"Miranda ICQ DB",0x1A};

//the cache has not been loaded when these functions are used

int CreateDbHeaders(void)
{
	struct DBContact user;
	DWORD bytesWritten;

	CopyMemory(dbHeader.signature,&dbSignature,sizeof(dbHeader.signature));
	dbHeader.version=DB_THIS_VERSION;
	dbHeader.ofsFileEnd=sizeof(struct DBHeader);
	dbHeader.slackSpace=0;
	dbHeader.contactCount=0;
	dbHeader.ofsFirstContact=0;
	dbHeader.ofsFirstModuleName=0;
	dbHeader.ofsUser=0;
	//create user
	dbHeader.ofsUser=dbHeader.ofsFileEnd;
	dbHeader.ofsFileEnd+=sizeof(struct DBContact);
	SetFilePointer(hDbFile,0,NULL,FILE_BEGIN);
	WriteFile(hDbFile,&dbHeader,sizeof(dbHeader),&bytesWritten,NULL);
	user.signature=DBCONTACT_SIGNATURE;
	user.ofsNext=0;
	user.ofsFirstSettings=0;
	user.eventCount=0;
	user.ofsFirstEvent=user.ofsLastEvent=0;
	SetFilePointer(hDbFile,dbHeader.ofsUser,NULL,FILE_BEGIN);
	WriteFile(hDbFile,&user,sizeof(struct DBContact),&bytesWritten,NULL);
	FlushFileBuffers(hDbFile);
	return 0;
}

int CheckDbHeaders(void)
{
	if(memcmp(dbHeader.signature,&dbSignature,sizeof(dbHeader.signature))) return 1;
	if(dbHeader.version!=DB_THIS_VERSION) return 2;
	if(dbHeader.ofsUser==0) return 3;
	return 0;
}

int InitialiseDbHeaders(void)
{
	return 0;
} 