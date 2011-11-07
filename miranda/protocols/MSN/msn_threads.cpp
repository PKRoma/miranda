/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-2011 Boris Krasnovskiy.
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
//	Keep-alive thread for the main connection

void __cdecl CMsnProto::msn_keepAliveThread(void*)
{
	bool keepFlag = true;

	hKeepAliveThreadEvt = CreateEvent(NULL, FALSE, FALSE, NULL);

	msnPingTimeout = 45;

	while (keepFlag)
	{
		switch (WaitForSingleObject(hKeepAliveThreadEvt, msnPingTimeout * 1000))
		{
			case WAIT_TIMEOUT:
				keepFlag = msnNsThread != NULL;
				if (usingGateway)
					msnPingTimeout = 45;
				else 
				{
					msnPingTimeout = 20;
					keepFlag = keepFlag && msnNsThread->send("PNG\r\n", 5);
				}
				p2p_clearDormantSessions();
				if (hHttpsConnection && (clock() - mHttpsTS) > 60 *	CLOCKS_PER_SEC)
				{
					HANDLE hConn = hHttpsConnection;
					hHttpsConnection = NULL;
					Netlib_CloseHandle(hConn);
				}
				if (mStatusMsgTS && (clock() - mStatusMsgTS) > 60 *	CLOCKS_PER_SEC)
				{
					mStatusMsgTS = 0;
					ForkThread(&CMsnProto::msn_storeProfileThread, NULL);
				}
				break;

			case WAIT_OBJECT_0:
				keepFlag = msnPingTimeout > 0;
				break;

			default:
				keepFlag = false;
				break;
		}	
	}

	CloseHandle(hKeepAliveThreadEvt); hKeepAliveThreadEvt = NULL;
	MSN_DebugLog("Closing keep-alive thread");
}

/////////////////////////////////////////////////////////////////////////////////////////
//	MSN server thread - read and process commands from a server

void __cdecl CMsnProto::MSNServerThread(void* arg)
{
	ThreadData* info = (ThreadData*)arg;

	if (info->mIsMainThread)
		isConnectSuccess = false;

	char* tPortDelim = strrchr(info->mServer, ':');
	if (tPortDelim != NULL)
		*tPortDelim = '\0';

	if (usingGateway) 
	{
		if (info->mServer[0] == 0)
			strcpy(info->mServer, MSN_DEFAULT_LOGIN_SERVER); 
		else if (info->mIsMainThread)
			strcpy(info->mGatewayIP, info->mServer);

		if (info->gatewayType)
			strcpy(info->mGatewayIP, info->mServer);
		else
		{
			if (info->mGatewayIP[0] == 0 && getStaticString(NULL, "GatewayServer", info->mGatewayIP, sizeof(info->mGatewayIP)))
				strcpy(info->mGatewayIP, MSN_DEFAULT_GATEWAY);
		}
	}
	else
	{
		if (info->mServer[0] == 0 && getStaticString(NULL, "DirectServer", info->mServer, sizeof(info->mServer)))
			strcpy(info->mServer, MSN_DEFAULT_LOGIN_SERVER);
	}

	NETLIBOPENCONNECTION tConn = { 0 };
	tConn.cbSize  = sizeof(tConn);
	tConn.flags   = NLOCF_V2;
	tConn.timeout = 5;

	if (usingGateway)
	{
		tConn.flags  |= NLOCF_HTTPGATEWAY;
		tConn.szHost  = info->mGatewayIP;
		tConn.wPort   = MSN_DEFAULT_GATEWAY_PORT;
	}
	else
	{
		tConn.szHost  = info->mServer;
		tConn.wPort   = MSN_DEFAULT_PORT;
		if (tPortDelim != NULL) 
		{
			int tPortNumber = atoi(tPortDelim + 1);
			if (tPortNumber)
				tConn.wPort = (WORD)tPortNumber;
		}	
	}

	MSN_DebugLog("Thread started: server='%s:%d', type=%d", tConn.szHost, tConn.wPort, info->mType);

	info->s = (HANDLE)MSN_CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)hNetlibUser, (LPARAM)&tConn);
	if (info->s == NULL) 
	{
		MSN_DebugLog("Connection Failed (%d) server='%s:%d'", WSAGetLastError(), tConn.szHost, tConn.wPort);

		switch (info->mType) 
		{
			case SERVER_NOTIFICATION: 
			case SERVER_DISPATCH:
				goto LBL_Exit; 
				break;

			case SERVER_SWITCHBOARD:
				if (info->mCaller) msnNsThread->sendPacket("XFR", "SB");
				break;
		}
		return;
	}

	if (usingGateway)
		MSN_CallService(MS_NETLIB_SETPOLLINGTIMEOUT, WPARAM(info->s), info->mGatewayTimeout);

	MSN_DebugLog("Connected with handle=%08X", info->s);

	if (info->mType == SERVER_DISPATCH || info->mType == SERVER_NOTIFICATION) 
	{
		info->sendPacket("VER", "MSNP18 MSNP17 CVR0");
	}
	else if (info->mType == SERVER_SWITCHBOARD)
	{
		info->sendPacket(info->mCaller ? "USR" : "ANS", "%s;%s %s", MyOptions.szEmail, MyOptions.szMachineGuid, info->mCookie);
	}
	else if (info->mType == SERVER_FILETRANS && info->mCaller == 0) 
	{
		info->send("VER MSNFTP\r\n", 12);
	}

	if (info->mIsMainThread) 
	{
		msnNsThread = info;
	}

	MSN_DebugLog("Entering main recv loop");
	info->mBytesInData = 0;
	for (;;) 
	{
		int recvResult = info->recv(info->mData + info->mBytesInData, sizeof(info->mData) - info->mBytesInData);
		if (recvResult == SOCKET_ERROR) 
		{
			MSN_DebugLog("Connection %08p [%08X] was abortively closed", info->s, GetCurrentThreadId());
			break;
		}

		if (!recvResult) 
		{
			MSN_DebugLog("Connection %08p [%08X] was gracefully closed", info->s, GetCurrentThreadId());
			break;
		}

		info->mBytesInData += recvResult;

		if (info->mCaller == 1 && info->mType == SERVER_FILETRANS) 
		{
			if (MSN_HandleMSNFTP(info, info->mData))
				break;
		}
		else 
		{
			for(;;) 
			{
				char* peol = strchr(info->mData, '\r');
				if (peol == NULL)
					break;

				if (info->mBytesInData < peol-info->mData + 2)
					break;  //wait for full line end

				char msg[sizeof(info->mData)];
				memcpy(msg, info->mData, peol-info->mData); msg[peol-info->mData] = 0;

				if (*++peol != '\n')
					MSN_DebugLog("Dodgy line ending to command: ignoring");
				else
					peol++;

				info->mBytesInData -= peol - info->mData;
				memmove(info->mData, peol, info->mBytesInData);
				MSN_DebugLog("RECV: %s", msg);

				if (!isalnum(msg[0]) || !isalnum(msg[1]) || !isalnum(msg[2]) || (msg[3] && msg[3] != ' ')) 
				{
					MSN_DebugLog("Invalid command name");
					continue;
				}

				if (info->mType != SERVER_FILETRANS) 
				{
					int handlerResult;
					if (isdigit(msg[0]) && isdigit(msg[1]) && isdigit(msg[2]))   //all error messages
						handlerResult = MSN_HandleErrors(info, msg);
					else
						handlerResult = MSN_HandleCommands(info, msg);

					if (handlerResult)
					{
						if (info->sessionClosed) goto LBL_Exit;
						info->sendTerminate();
					}
				}
				else 
					if (MSN_HandleMSNFTP(info, msg))
						goto LBL_Exit;
			}	
		}

		if (info->mBytesInData == sizeof(info->mData)) 
		{
			MSN_DebugLog("sizeof(data) is too small: the longest line won't fit");
			break;
		}	
	}

LBL_Exit:
	if (info->mIsMainThread) 
	{
		if (!isConnectSuccess && !usingGateway && m_iDesiredStatus != ID_STATUS_OFFLINE) 
		{ 
			msnNsThread = NULL;
			usingGateway = true; 

			ThreadData* newThread = new ThreadData;
			newThread->mType = SERVER_DISPATCH;
			newThread->mIsMainThread = true;

			newThread->startThread(&CMsnProto::MSNServerThread, this);
		}
		else
		{
			if (hKeepAliveThreadEvt) 
			{
				msnPingTimeout *= -1;
				SetEvent(hKeepAliveThreadEvt);
			}

			if (info->s == NULL)
				SendBroadcast(NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_NONETWORK);
			else
			{
				p2p_cancelAllSessions();
				MSN_CloseConnections();
			}

			if (hHttpsConnection)
			{
				Netlib_CloseHandle(hHttpsConnection);
				hHttpsConnection = NULL;
			}

			MSN_GoOffline();
			msnNsThread = NULL;
		}
	}

	MSN_DebugLog("Thread [%08X] ending now", GetCurrentThreadId());
}

void  CMsnProto::MSN_InitThreads(void)
{
	InitializeCriticalSection(&sttLock);
}

void  CMsnProto::MSN_CloseConnections(void)
{
	EnterCriticalSection(&sttLock);

	NETLIBSELECTEX nls = {0};
	nls.cbSize = sizeof(nls);

	for (int i=0; i < sttThreads.getCount(); i++) 
	{
		ThreadData* T = &sttThreads[i];

		switch (T->mType) 
		{
		case SERVER_DISPATCH :
		case SERVER_NOTIFICATION :
		case SERVER_SWITCHBOARD :
			if (T->s != NULL && !T->sessionClosed && !T->termPending)
			{
				nls.hReadConns[0] = T->s;
				int res = MSN_CallService(MS_NETLIB_SELECTEX, 0, (LPARAM)&nls);
				if (res >= 0 || nls.hReadStatus[0] == 0)
					T->sendTerminate();
			}
			break;

		case SERVER_P2P_DIRECT :
			MSN_CallService(MS_NETLIB_SHUTDOWN, (WPARAM)T->s, 0);
			break;
		}	
	}

	LeaveCriticalSection(&sttLock);

	if (hHttpsConnection)
		MSN_CallService(MS_NETLIB_SHUTDOWN, (WPARAM)hHttpsConnection, 0);
}

void  CMsnProto::MSN_CloseThreads(void)
{
	for (unsigned j=6; --j;)
	{	
		EnterCriticalSection(&sttLock);

		bool opcon = false;
		for (int i=0; i < sttThreads.getCount(); i++)
			opcon |= (sttThreads[i].s != NULL);

		LeaveCriticalSection(&sttLock);
		
		if (!opcon) break;
		
		Sleep(250);
	}

	EnterCriticalSection(&sttLock);

	for (int i=0; i < sttThreads.getCount(); i++) 
	{
		ThreadData* T = &sttThreads[i];
		
		if (T->s != NULL)
			MSN_CallService(MS_NETLIB_SHUTDOWN, (WPARAM)T->s, 0);
	}

	LeaveCriticalSection(&sttLock);
}

void CMsnProto::Threads_Uninit(void)
{
	EnterCriticalSection(&sttLock);
	sttThreads.destroy();
	LeaveCriticalSection(&sttLock);
	DeleteCriticalSection(&sttLock);
}

ThreadData*  CMsnProto::MSN_GetThreadByContact(const char* wlid, TInfoType type)
{
	ThreadData* result = NULL;
	EnterCriticalSection(&sttLock);
	
	if (type == SERVER_P2P_DIRECT)
	{
		for (int i=0; i < sttThreads.getCount(); i++)
		{
			ThreadData* T = &sttThreads[i];
			if (T->mType != SERVER_P2P_DIRECT || !T->mJoinedIdentContactsWLID.getCount() || T->s == NULL)
				continue;

			if (_stricmp(T->mJoinedIdentContactsWLID[0], wlid) == 0)
			{
				result = T;
				break;
			}
		}
	}

	if (result == NULL)
	{
		char *szEmail = NULL;
		parseWLID(NEWSTR_ALLOCA(wlid), NULL, &szEmail, NULL);

		for (int i=0; i < sttThreads.getCount(); i++)
		{
			ThreadData* T = &sttThreads[i];
			if (T->mType != type || !T->mJoinedContactsWLID.getCount() || T->mInitialContactWLID || T->s == NULL)
				continue;

			if (_stricmp(T->mJoinedContactsWLID[0], szEmail) == 0 && T->mChatID[0] == 0)
			{
				result = T;
				break;
			}
		}
	}

	LeaveCriticalSection(&sttLock);
	return result;
}

ThreadData*  CMsnProto::MSN_GetThreadByChatId(const TCHAR* chatId)
{
	ThreadData* result = NULL;
	EnterCriticalSection(&sttLock);

	for (int i=0; i < sttThreads.getCount(); i++)
	{
		ThreadData* T = &sttThreads[i];

		if (_tcsicmp(T->mChatID, chatId) == 0)
		{
			result = T;
			break;
		}
	}

	LeaveCriticalSection(&sttLock);
	return result;
}

ThreadData*  CMsnProto::MSN_GetThreadByTimer(UINT timerId)
{
	ThreadData* result = NULL;
	EnterCriticalSection(&sttLock);

	for (int i=0; i < sttThreads.getCount(); i++) 
	{
		ThreadData* T = &sttThreads[i];
		if (T->mType == SERVER_SWITCHBOARD && T->mTimerId == timerId) 
		{
			result = T;
			break;
		}	
	}

	LeaveCriticalSection(&sttLock);
	return result;
}

ThreadData*  CMsnProto::MSN_GetP2PThreadByContact(const char *wlid)
{
	ThreadData *result = NULL;

	EnterCriticalSection(&sttLock);

	for (int i=0; i < sttThreads.getCount(); i++)
	{
		ThreadData* T = &sttThreads[i];
		if (T->mType != SERVER_P2P_DIRECT || !T->mJoinedIdentContactsWLID.getCount())
			continue;

		if (_stricmp(T->mJoinedIdentContactsWLID[0], wlid) == 0)
		{
			result = T;
			break;
		}
	}

	if (result == NULL)
	{
		char *szEmail = NULL;
		parseWLID(NEWSTR_ALLOCA(wlid), NULL, &szEmail, NULL);

		for (int i=0; i < sttThreads.getCount(); i++)
		{
			ThreadData* T = &sttThreads[i];
			if (T->mJoinedContactsWLID.getCount() && !T->mInitialContactWLID &&
				_stricmp(T->mJoinedContactsWLID[0], szEmail) == 0) 
			{
				if (T->mType == SERVER_P2P_DIRECT)
				{
					result = T;
					break;
				}
				else if (T->mType == SERVER_SWITCHBOARD) 
					result = T;
			}
		}
	}

	LeaveCriticalSection(&sttLock);

	return result;
}


void  CMsnProto::MSN_StartP2PTransferByContact(const char* wlid)
{
	EnterCriticalSection(&sttLock);

	for (int i=0; i < sttThreads.getCount(); i++) 
	{
		ThreadData* T = &sttThreads[i];
		if (T->mType == SERVER_FILETRANS && T->hWaitEvent != INVALID_HANDLE_VALUE)
		{
			if ((T->mInitialContactWLID && !_stricmp(T->mInitialContactWLID, wlid)) || 
				(T->mJoinedContactsWLID.getCount() && !_stricmp(T->mJoinedContactsWLID[0], wlid)) ||
				(T->mJoinedIdentContactsWLID.getCount() && !_stricmp(T->mJoinedIdentContactsWLID[0], wlid)))
				ReleaseSemaphore(T->hWaitEvent, 1, NULL);
		}
	}

	LeaveCriticalSection(&sttLock);
}


ThreadData*  CMsnProto::MSN_GetOtherContactThread(ThreadData* thread)
{
	ThreadData* result = NULL;
	EnterCriticalSection(&sttLock);

	for (int i=0; i < sttThreads.getCount(); i++) 
	{
		ThreadData* T = &sttThreads[i];
		if (T->mJoinedContactsWLID.getCount() == 0 || T->s == NULL)
			continue;

		if (T != thread && _stricmp(T->mJoinedContactsWLID[0], thread->mJoinedContactsWLID[0]) == 0) 
		{
			result = T;
			break;
		}	
	}

	LeaveCriticalSection(&sttLock);
	return result;
}

ThreadData*  CMsnProto::MSN_GetUnconnectedThread(const char* wlid, TInfoType type)
{
	ThreadData* result = NULL;
	EnterCriticalSection(&sttLock);

	char* szEmail = (char*)wlid;
	
	if (type == SERVER_SWITCHBOARD && strchr(wlid, ';'))
		parseWLID(NEWSTR_ALLOCA(wlid), NULL, &szEmail, NULL);

	for (int i=0; i < sttThreads.getCount(); i++) 
	{
		ThreadData* T = &sttThreads[i];
 		if (T->mType == type && T->mInitialContactWLID && _stricmp(T->mInitialContactWLID, szEmail) == 0) 
		{
			result = T;
			break;
		}	
	}

	LeaveCriticalSection(&sttLock);
	return result;
}


ThreadData* CMsnProto::MSN_StartSB(const char* wlid, bool& isOffline)
{
	isOffline = false;
	ThreadData* thread = MSN_GetThreadByContact(wlid);
	if (thread == NULL)
	{
		HANDLE hContact = MSN_HContactFromEmail(wlid);
		WORD wStatus = getWord(hContact, "Status", ID_STATUS_OFFLINE);
		if (wStatus != ID_STATUS_OFFLINE)
		{
			if (MSN_GetUnconnectedThread(wlid) == NULL && MsgQueue_CheckContact(wlid, 5) == NULL)
				msnNsThread->sendPacket("XFR", "SB");
		}
		else
			isOffline = true;
	}
	return thread;
}



int  CMsnProto::MSN_GetActiveThreads(ThreadData** parResult)
{
	int tCount = 0;
	EnterCriticalSection(&sttLock);

	for (int i=0; i < sttThreads.getCount(); i++)
	{
		ThreadData* T = &sttThreads[i];
		if (T->mType == SERVER_SWITCHBOARD && T->mJoinedContactsWLID.getCount() != 0 && T->mJoinedContactsWLID.getCount())
			parResult[tCount++] = T;
	}

	LeaveCriticalSection(&sttLock);
	return tCount;
}

ThreadData*  CMsnProto::MSN_GetThreadByConnection(HANDLE s)
{
	ThreadData* tResult = NULL;
	EnterCriticalSection(&sttLock);

	for (int i=0; i < sttThreads.getCount(); i++) 
	{
		ThreadData* T = &sttThreads[i];
		if (T->s == s) 
		{
			tResult = T;
			break;
		}	
	}

	LeaveCriticalSection(&sttLock);
	return tResult;
}

ThreadData*  CMsnProto::MSN_GetThreadByPort(WORD wPort)
{
	ThreadData* result = NULL;
	EnterCriticalSection(&sttLock);

	for (int i=0; i < sttThreads.getCount(); i++) 
	{
		ThreadData* T = &sttThreads[i];
		if (T->mIncomingPort == wPort)
		{
			result = T;
			break;
		}	
	}

	LeaveCriticalSection(&sttLock);
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////
// class ThreadData members

ThreadData::ThreadData()
{
	memset(&mInitialContactWLID, 0, sizeof(ThreadData) - 2*sizeof(STRLIST));
	mGatewayTimeout = 2;
	resetTimeout();
	hWaitEvent = CreateSemaphore(NULL, 0, MSN_PACKETS_COMBINE, NULL);
}

ThreadData::~ThreadData()
{
	int i;

	if (s != NULL) 
	{
		proto->MSN_DebugLog("Closing connection handle %08X", s);
		Netlib_CloseHandle(s);
	}

	if (mIncomingBoundPort != NULL)
	{
		Netlib_CloseHandle(mIncomingBoundPort);
	}

	if (mMsnFtp != NULL) 
	{
		delete mMsnFtp;
		mMsnFtp = NULL;
	}

	if (hWaitEvent != INVALID_HANDLE_VALUE)
		CloseHandle(hWaitEvent);

	if (mTimerId != 0) 
		KillTimer(NULL, mTimerId);

	if (mType == SERVER_SWITCHBOARD)
	{
		for (i=0; i<mJoinedContactsWLID.getCount(); ++i)
		{
			const char* wlid = mJoinedContactsWLID[i];
			HANDLE hContact = proto->MSN_HContactFromEmail(wlid); 
			int temp_status = proto->getWord(hContact, "Status", ID_STATUS_OFFLINE);
			if (temp_status == ID_STATUS_INVISIBLE && proto->MSN_GetThreadByContact(wlid) == NULL)
				proto->setWord(hContact, "Status", ID_STATUS_OFFLINE);
		}
	}

	mJoinedContactsWLID.destroy();
	mJoinedIdentContactsWLID.destroy();

	const char* wlid = NEWSTR_ALLOCA(mInitialContactWLID);
	mir_free(mInitialContactWLID); mInitialContactWLID = NULL;

	if (proto && mType == SERVER_P2P_DIRECT) 
		proto->p2p_clearDormantSessions();

	if (wlid != NULL && mType == SERVER_SWITCHBOARD && 
		proto->MSN_GetThreadByContact(wlid) == NULL &&
		proto->MSN_GetUnconnectedThread(wlid) == NULL)
	{
		proto->MsgQueue_Clear(wlid, true);
	}
}

void ThreadData::applyGatewayData(HANDLE hConn, bool isPoll)
{
	char szHttpPostUrl[300];
	getGatewayUrl(szHttpPostUrl, sizeof(szHttpPostUrl), isPoll);

	proto->MSN_DebugLog("applying '%s' to %08X [%08X]", szHttpPostUrl, this, GetCurrentThreadId());

	NETLIBHTTPPROXYINFO nlhpi = {0};
	nlhpi.cbSize = sizeof(nlhpi);
	nlhpi.flags = NLHPIF_HTTP11;
	nlhpi.szHttpGetUrl = NULL;
	nlhpi.szHttpPostUrl = szHttpPostUrl;
	nlhpi.combinePackets = 5;
	MSN_CallService(MS_NETLIB_SETHTTPPROXYINFO, (WPARAM)hConn, (LPARAM)&nlhpi);
}

void ThreadData::getGatewayUrl(char* dest, int destlen, bool isPoll)
{
	static const char openFmtStr[] = "http://%s/gateway/gateway.dll?Action=open&Server=%s&IP=%s";
	static const char pollFmtStr[] = "http://%s/gateway/gateway.dll?Action=poll&SessionID=%s";
	static const char cmdFmtStr[]  = "http://%s/gateway/gateway.dll?SessionID=%s";

	if (mSessionID[0] == 0)
	{
		const char* svr = mType == SERVER_NOTIFICATION || mType == SERVER_DISPATCH ? "NS" : "SB";
		mir_snprintf(dest, destlen, openFmtStr, mGatewayIP, svr, mServer);
	}
	else
		mir_snprintf(dest, destlen, isPoll ? pollFmtStr : cmdFmtStr, mGatewayIP, mSessionID);
}

void ThreadData::processSessionData(const char* str)
{
	char tSessionID[40], tGateIP[40];

	char* tDelim = (char*)strchr(str, ';');
	if (tDelim == NULL)
		return; 

	*tDelim = 0; tDelim += 2;

	if (!sscanf(str, "SessionID=%s", tSessionID))
		return;

	char* tDelim2 = strchr(tDelim, ';');
	if (tDelim2 != NULL)
		*tDelim2 = '\0';

	if (!sscanf(tDelim, "GW-IP=%s", tGateIP))
		return;

	strcpy(mGatewayIP, tGateIP);
	if (gatewayType) strcpy(mServer, tGateIP);
	strcpy(mSessionID, tSessionID);
}

/////////////////////////////////////////////////////////////////////////////////////////
// thread start code
/////////////////////////////////////////////////////////////////////////////////////////

void __cdecl CMsnProto::ThreadStub(void* arg)
{
	ThreadData* info = (ThreadData*)arg;

	MSN_DebugLog("Starting thread %08X (%08X)", GetCurrentThreadId(), info->mFunc);

	(this->*(info->mFunc))(info);

	MSN_DebugLog("Leaving thread %08X (%08X)", GetCurrentThreadId(), info->mFunc);

	EnterCriticalSection(&sttLock);
	sttThreads.LIST<ThreadData>::remove(info);
	LeaveCriticalSection(&sttLock);
	delete info;
}

void ThreadData::startThread(MsnThreadFunc parFunc, CMsnProto *prt)
{
	mFunc = parFunc;
	proto = prt;

	EnterCriticalSection(&proto->sttLock);
	proto->sttThreads.insert(this);
	LeaveCriticalSection(&proto->sttLock);

	proto->ForkThread(&CMsnProto::ThreadStub, this);
}

/////////////////////////////////////////////////////////////////////////////////////////
// HReadBuffer members

HReadBuffer::HReadBuffer(ThreadData* T, int iStart)
{
	owner = T;
	buffer = (BYTE*)T->mData;
	totalDataSize = T->mBytesInData;
	startOffset = iStart;
}

HReadBuffer::~HReadBuffer()
{
	if (totalDataSize > startOffset)
	{
		memmove(buffer, buffer + startOffset, (totalDataSize -= startOffset));
		owner->mBytesInData = (int)totalDataSize;
	}
	else
		owner->mBytesInData = 0;
}

BYTE* HReadBuffer::surelyRead(size_t parBytes)
{
	const size_t bufferSize = sizeof(owner->mData);

	if ((startOffset + parBytes) > bufferSize)
	{
		if (totalDataSize > startOffset) 
			memmove(buffer, buffer + startOffset, (totalDataSize -= startOffset));
		else
			totalDataSize = 0;

		startOffset = 0;

		if (parBytes > bufferSize) 
		{
			owner->proto->MSN_DebugLog("HReadBuffer::surelyRead: not enough memory, %d %d %d", parBytes, bufferSize, startOffset);
			return NULL;
		}
	}

	while ((startOffset + parBytes) > totalDataSize)
	{
		int recvResult = owner->recv((char*)buffer + totalDataSize, bufferSize - totalDataSize);

		if (recvResult <= 0)
			return NULL;

		totalDataSize += recvResult;
	}

	BYTE* result = buffer + startOffset; startOffset += parBytes;
	return result;
}
