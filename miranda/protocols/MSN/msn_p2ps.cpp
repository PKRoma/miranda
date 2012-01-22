/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-2012 Boris Krasnovskiy.
Copyright (c) 2003-2005 George Hazan.
Copyright (c) 2002-2003 Richard Hughes (original version).

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "msn_global.h"
#include "msn_proto.h"

/////////////////////////////////////////////////////////////////////////////////////////
// add file session to a list

void  CMsnProto::p2p_registerSession(filetransfer* ft)
{
	EnterCriticalSection(&sessionLock);
	sessionList.insert(ft);
	LeaveCriticalSection(&sessionLock);
}

/////////////////////////////////////////////////////////////////////////////////////////
// remove file session from a list

void  CMsnProto::p2p_unregisterSession(filetransfer* ft)
{
	EnterCriticalSection(&sessionLock);
	int idx = sessionList.getIndex(ft);
	if (idx > -1) sessionList.remove(idx);
	LeaveCriticalSection(&sessionLock);
}

/////////////////////////////////////////////////////////////////////////////////////////
// get session by some parameter

filetransfer*  CMsnProto::p2p_getSessionByID(unsigned id)
{
	if (id == 0)
		return NULL;

	filetransfer* ft = NULL;
	EnterCriticalSection(&sessionLock);

	for (int i=0; i < sessionList.getCount(); i++) 
	{
		filetransfer* FT = &sessionList[i];
		if (FT->p2p_sessionid == id) 
		{
			ft = FT;
			break;
		}	
	}

	LeaveCriticalSection(&sessionLock);
	if (ft == NULL)
		MSN_DebugLog("Ignoring unknown session id %08x", id);

	return ft;
}

filetransfer*  CMsnProto::p2p_getSessionByUniqueID(unsigned id)
{
	if (id == 0)
		return NULL;

	filetransfer* ft = NULL;
	EnterCriticalSection(&sessionLock);

	for (int i=0; i < sessionList.getCount(); i++) 
	{
		filetransfer* FT = &sessionList[i];
		if (FT->p2p_acksessid == id) 
		{
			ft = FT;
			break;
		}	
	}

	LeaveCriticalSection(&sessionLock);
	if (ft == NULL)
		MSN_DebugLog("Ignoring unknown unique id %08x", id);

	return ft;
}


bool  CMsnProto::p2p_sessionRegistered(filetransfer* ft)
{
	if (ft != NULL && ft->p2p_appID == 0)
		return true;

	EnterCriticalSection(&sessionLock);
	int idx = sessionList.getIndex(ft);
	LeaveCriticalSection(&sessionLock);
	return idx > -1;
}

filetransfer*  CMsnProto::p2p_getThreadSession(HANDLE hContact, TInfoType mType)
{
	EnterCriticalSection(&sessionLock);

	filetransfer* result = NULL;
	for (int i=0; i < sessionList.getCount(); i++) 
	{
		filetransfer* FT = &sessionList[i];
		if (FT->std.hContact == hContact && FT->tType == mType) 
		{
			result = FT;
			break;
		}	
	}

	LeaveCriticalSection(&sessionLock);
	return result;
}

filetransfer*  CMsnProto::p2p_getAvatarSession(HANDLE hContact)
{
	EnterCriticalSection(&sessionLock);

	filetransfer* result = NULL;
	for (int i=0; i < sessionList.getCount(); i++) 
	{
		filetransfer* FT = &sessionList[i];
		if (FT->std.hContact == hContact && !(FT->std.flags & PFTS_SENDING) && 
			FT->p2p_type == MSN_APPID_AVATAR) 
		{
			result = FT;
			break;
		}	
	}

	LeaveCriticalSection(&sessionLock);
	return result;
}

bool  CMsnProto::p2p_isAvatarOnly(HANDLE hContact)
{
	EnterCriticalSection(&sessionLock);

	bool result = true;
	for (int i=0; i < sessionList.getCount(); i++) 
	{
		filetransfer* FT = &sessionList[i];
		result &= FT->std.hContact != hContact || FT->p2p_type != MSN_APPID_FILE;
	}

	LeaveCriticalSection(&sessionLock);
	return result;
}

void  CMsnProto::p2p_clearDormantSessions(void)
{
	EnterCriticalSection(&sessionLock);

	time_t ts = time(NULL);
	for (int i=0; i < sessionList.getCount(); i++) 
	{
		filetransfer* FT = &sessionList[i];
		if (!FT->p2p_sessionid && !MSN_GetUnconnectedThread(FT->p2p_dest, SERVER_P2P_DIRECT))
			p2p_invite(FT->p2p_type, FT, NULL);
		else if (FT->p2p_waitack && (ts - FT->ts) > 60) 
		{
			FT->bCanceled = true;
			p2p_sendCancel(FT);
			LeaveCriticalSection(&sessionLock);
			p2p_unregisterSession(FT);
			EnterCriticalSection(&sessionLock);
			i = 0;
		}	
	}

	LeaveCriticalSection(&sessionLock);
}

void  CMsnProto::p2p_redirectSessions(const char *wlid)
{
	EnterCriticalSection(&sessionLock);

	ThreadData* T = MSN_GetP2PThreadByContact(wlid);
	for (int i=0; i < sessionList.getCount(); i++) 
	{
		filetransfer* FT = &sessionList[i];
		if (_stricmp(FT->p2p_dest, wlid) == 0 && 
			FT->std.currentFileProgress < FT->std.currentFileSize &&
			(T == NULL || (FT->tType != T->mType && FT->tType != 0)))
		{
			if (FT->p2p_isV2)
			{
				if ((FT->std.flags & PFTS_SENDING) && T)
					FT->tType = T->mType;
			}
			else
			{
				if (!(FT->std.flags & PFTS_SENDING))  
					p2p_sendRedirect(FT);
			}
		}
	}

	LeaveCriticalSection(&sessionLock);
}

void  CMsnProto::p2p_startSessions(const char* wlid)
{
	EnterCriticalSection(&sessionLock);

	char* szEmail;
	parseWLID(NEWSTR_ALLOCA(wlid), NULL, &szEmail, NULL);

	for (int i=0; i < sessionList.getCount(); i++) 
	{
		filetransfer* FT = &sessionList[i];
		if (!FT->bAccepted  && !_stricmp(FT->p2p_dest, szEmail))
		{
			if (FT->p2p_appID == MSN_APPID_FILE && (FT->std.flags & PFTS_SENDING))
				p2p_invite(FT->p2p_type, FT, wlid);
			else if (FT->p2p_appID != MSN_APPID_FILE && !(FT->std.flags & PFTS_SENDING))
				p2p_invite(FT->p2p_type, FT, wlid);
		}
	}

	LeaveCriticalSection(&sessionLock);
}

void  CMsnProto::p2p_cancelAllSessions(void)
{
	EnterCriticalSection(&sessionLock);

	for (int i=0; i < sessionList.getCount(); i++) 
	{
		sessionList[i].bCanceled = true;
		p2p_sendCancel(&sessionList[i]);
	}

	LeaveCriticalSection(&sessionLock);
}

filetransfer*  CMsnProto::p2p_getSessionByCallID(const char* CallID, const char* wlid)
{
	if (CallID == NULL)
		return NULL;

	EnterCriticalSection(&sessionLock);

	filetransfer* ft = NULL;
	char* szEmail = NULL;
	for (int i=0; i < sessionList.getCount(); i++) 
	{
		filetransfer* FT = &sessionList[i];
		if (FT->p2p_callID && !_stricmp(FT->p2p_callID, CallID)) 
		{
 			if (_stricmp(FT->p2p_dest, wlid))
			{
				if (!szEmail)
					parseWLID(NEWSTR_ALLOCA(wlid), NULL, &szEmail, NULL);
				if (_stricmp(FT->p2p_dest, szEmail))
					continue;
			}
			ft = FT;
			break;
		}	
	}

	LeaveCriticalSection(&sessionLock);
	if (ft == NULL)
		MSN_DebugLog("Ignoring unknown session call id %s", CallID);

	return ft;
}


void  CMsnProto::p2p_registerDC(directconnection* dc)
{
	EnterCriticalSection(&sessionLock);
	dcList.insert(dc);
	LeaveCriticalSection(&sessionLock);
}

void  CMsnProto::p2p_unregisterDC(directconnection* dc)
{
	EnterCriticalSection(&sessionLock);
	int idx = dcList.getIndex(dc);
	if (idx > -1) 
	{
		dcList.remove(idx);
	}
	LeaveCriticalSection(&sessionLock);
}

directconnection*  CMsnProto::p2p_getDCByCallID(const char* CallID, const char* wlid)
{
	if (CallID == NULL)
		return NULL;

	EnterCriticalSection(&sessionLock);

	directconnection* dc = NULL;
	for (int i=0; i < dcList.getCount(); i++) 
	{
		directconnection* DC = &dcList[i];
		if (DC->callId != NULL && !strcmp(DC->callId, CallID) && !strcmp(DC->wlid, wlid)) 
		{
			dc = DC;
			break;
		}	
	}

	LeaveCriticalSection(&sessionLock);

	return dc;
}

/////////////////////////////////////////////////////////////////////////////////////////
// external functions

void CMsnProto::P2pSessions_Init(void)
{
	InitializeCriticalSection(&sessionLock);
}

void CMsnProto::P2pSessions_Uninit(void)
{
	EnterCriticalSection(&sessionLock);

	sessionList.destroy();
	dcList.destroy();

	LeaveCriticalSection(&sessionLock);
	DeleteCriticalSection(&sessionLock);
}
