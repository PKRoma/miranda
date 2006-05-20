/*
Miranda Database Tool
Copyright (C) 2001-2005  Richard Hughes

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
#include "dbtool.h"
#include <stddef.h>

struct ModChainEntry {
	DWORD ofsOld,ofsNew;
	int size;
} static *modChain=NULL;
static int modChainCount;
static DWORD ofsCurrent;
static int phase,iCurrentModName;

int WorkModuleChain(int firstTime)
{
	DBModuleName moduleName,*newModName;

	if(firstTime) {
		AddToStatus(STATUS_MESSAGE,"Processing module name chain");
		modChainCount=0;
		if(modChain!=NULL) {
			free(modChain);
			modChain=NULL;
		}
		phase=0;
		ofsCurrent=dbhdr.ofsFirstModuleName;
	}
	switch(phase) {
		case 0:
			if(ofsCurrent==0) {
				phase++;
				return ERROR_SUCCESS;
			}
			if(!SignatureValid(ofsCurrent,DBMODULENAME_SIGNATURE)) {
				AddToStatus(STATUS_ERROR,"Module chain corrupted, further entries ignored");
				phase++;
				return ERROR_SUCCESS;
			}
			if(PeekSegment(ofsCurrent,&moduleName,offsetof(DBModuleName,name))!=ERROR_SUCCESS) {
				phase++;
				return ERROR_SUCCESS;
			}
			if(moduleName.cbName>256)
				AddToStatus(STATUS_WARNING,"Unreasonably long module name, skipping");
			else {
				modChain=(ModChainEntry*)realloc(modChain,sizeof(ModChainEntry)*++modChainCount);
				modChain[modChainCount-1].ofsOld=ofsCurrent;
				modChain[modChainCount-1].size=offsetof(DBModuleName,name)+moduleName.cbName;
			}
			ofsCurrent=moduleName.ofsNext;
			break;
		case 1:
			iCurrentModName=0;
			dbhdr.ofsFirstModuleName=0;
			phase++;
		case 2:
			if(iCurrentModName>=modChainCount)
				return ERROR_NO_MORE_ITEMS;
			newModName=(DBModuleName*)malloc(modChain[iCurrentModName].size);
			if(ReadSegment(modChain[iCurrentModName].ofsOld,newModName,modChain[iCurrentModName].size)!=ERROR_SUCCESS)
				return ERROR_NO_MORE_ITEMS;
			if((modChain[iCurrentModName].ofsNew=WriteSegment(WSOFS_END,newModName,modChain[iCurrentModName].size))==WS_ERROR)
				return ERROR_HANDLE_DISK_FULL;
			if(iCurrentModName==0) dbhdr.ofsFirstModuleName=modChain[iCurrentModName].ofsNew;
			else
				if(WriteSegment(modChain[iCurrentModName-1].ofsNew+offsetof(DBModuleName,ofsNext),&modChain[iCurrentModName].ofsNew,sizeof(DWORD))==WS_ERROR)
					return ERROR_HANDLE_DISK_FULL;
			free(newModName);
			iCurrentModName++;
			break;
	}
	return ERROR_SUCCESS;
}

DWORD ConvertModuleNameOfs(DWORD ofsOld)
{
	int i;

	for(i=0;i<modChainCount;i++)
		if(modChain[i].ofsOld==ofsOld) return modChain[i].ofsNew;
	AddToStatus(STATUS_ERROR,"Invalid module name offset, skipping data");
	return 0;
}