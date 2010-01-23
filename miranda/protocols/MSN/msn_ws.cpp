/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-2009 Boris Krasnovskiy.
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


//=======================================================================================

int ThreadData::send(const char data[], size_t datalen)
{
	NETLIBBUFFER nlb = { (char*)data, (int)datalen, 0 };

	mWaitPeriod = 60;

	if (proto->MyOptions.UseGateway && !(mType == SERVER_FILETRANS || mType == SERVER_P2P_DIRECT)) 
	{
		mGatewayTimeout = 2;

		if (!proto->MyOptions.UseProxy) {
			TQueueItem* tNewItem = (TQueueItem*)mir_alloc(datalen + sizeof(TQueueItem));
			tNewItem->datalen = datalen;
			memcpy(tNewItem->data, data, datalen);
			tNewItem->data[datalen] = 0;
			tNewItem->next = NULL;

			WaitForSingleObject(hQueueMutex, INFINITE);
			TQueueItem **p = &mFirstQueueItem;
			while (*p != NULL) p = &(*p)->next;
			*p = tNewItem;
			++numQueueItems;
			ReleaseMutex(hQueueMutex);

			return TRUE;
		}

		MSN_CallService(MS_NETLIB_SETPOLLINGTIMEOUT, WPARAM(s), mGatewayTimeout);
	}

	int rlen = MSN_CallService(MS_NETLIB_SEND, (WPARAM)s, (LPARAM)&nlb);
	if (rlen == SOCKET_ERROR) {
		// should really also check if sendlen is the same as datalen
		proto->MSN_DebugLog("Send failed: %d", WSAGetLastError());
		return FALSE;
	}

	return TRUE;
}

bool ThreadData::isTimeout(void)
{
	bool res = false;

	if (--mWaitPeriod > 0) return false;

	if (!mIsMainThread && (mJoinedCount <= 1 || mChatID[0] == 0)) 
	{
		if (mJoinedCount == 0 || termPending)
			res = true;
		else if (proto->p2p_getThreadSession(mJoinedContacts[0], mType) != NULL)
			res = false;
		else if (mType == SERVER_SWITCHBOARD) 
		{
			MessageWindowInputData msgWinInData = 
				{ sizeof(MessageWindowInputData), mJoinedContacts[0], MSG_WINDOW_UFLAG_MSG_BOTH };
			MessageWindowData msgWinData = {0};
			msgWinData.cbSize = sizeof(MessageWindowData);

			res = MSN_CallService(MS_MSG_GETWINDOWDATA, (WPARAM)&msgWinInData, (LPARAM)&msgWinData) != 0;
			res |= (msgWinData.hwndWindow == NULL);
			if (res) 
			{	
				msgWinInData.hContact = (HANDLE)MSN_CallService(MS_MC_GETMETACONTACT, (WPARAM)mJoinedContacts[0], 0);
				if (msgWinInData.hContact != NULL) 
				{
					res = MSN_CallService(MS_MSG_GETWINDOWDATA, (WPARAM)&msgWinInData, (LPARAM)&msgWinData) != 0;
					res |= (msgWinData.hwndWindow == NULL);
				}
			}
			if (res) 
			{	
				WORD status = proto->getWord(mJoinedContacts[0], "Status", ID_STATUS_OFFLINE);
				if ((status == ID_STATUS_OFFLINE || status == ID_STATUS_INVISIBLE || proto->m_iStatus == ID_STATUS_INVISIBLE)) 
					res = false;
			}
		}
		else
			res = true;
	}

	if (res) 
	{
		bool sbsess = mType == SERVER_SWITCHBOARD;

		proto->MSN_DebugLog("Dropping the idle %s due to inactivity", sbsess ? "switchboard" : "p2p");
		if (!sbsess || termPending) return true;

		if (proto->getByte("EnableSessionPopup", 0)) 
		{
			HANDLE hContact = mJoinedCount ? mJoinedContacts[0] : mInitialContact;
			proto->MSN_ShowPopup(hContact, TranslateT("Chat session dropped due to inactivity"), 0);
		}

		sendPacket("OUT", NULL);
		termPending = true;
		mWaitPeriod = 10;
	}
	else
		mWaitPeriod = 60;

	return false;
}


//=======================================================================================
// Receving data
//=======================================================================================
int ThreadData::recv_dg(char* data, size_t datalen)
{
	NETLIBHTTPREQUEST nlhr = {0};

	// initialize the netlib request
	nlhr.cbSize = sizeof(nlhr);
	nlhr.requestType = REQUEST_POST;
	nlhr.flags = NLHRF_HTTP11 | NLHRF_DUMPASTEXT | NLHRF_PERSISTENT;
	nlhr.nlc = s;

	nlhr.headersCount = 5;
	nlhr.headers=(NETLIBHTTPHEADER*)alloca(sizeof(NETLIBHTTPHEADER) * nlhr.headersCount);
	nlhr.headers[0].szName   = "User-Agent";
	nlhr.headers[0].szValue = (char*)MSN_USER_AGENT;
	nlhr.headers[1].szName  = "Accept";
	nlhr.headers[1].szValue = "*/*";
	nlhr.headers[2].szName  = "Content-Type";
	nlhr.headers[2].szValue = "text/xml; charset=utf-8";
	nlhr.headers[3].szName  = "Cache-Control";
	nlhr.headers[3].szValue = "no-cache";
	nlhr.headers[4].szName  = "Accept-Encoding";
	nlhr.headers[4].szValue = "identity";


	time_t ts = time(NULL);
	for(;;)
	{
		if (mReadAheadBuffer != NULL) 
		{
			size_t tBytesToCopy = min(mEhoughData, datalen);

			if (tBytesToCopy == 0) 
			{
				mir_free(mReadAheadBuffer);
				mReadAheadBuffer = NULL;
				mReadAheadBufferPtr = NULL;
				if (sessionClosed) return 0;
			}
			else 
			{
				memcpy(data, mReadAheadBufferPtr, tBytesToCopy);
				mReadAheadBufferPtr += tBytesToCopy;
				mEhoughData -= tBytesToCopy;
				return (int)tBytesToCopy;
			}
		}
		else if (sessionClosed) return 0;

		char* tBuffer = NULL;
		size_t cbBytes = 0;
		while ((time(NULL) - ts)  < mGatewayTimeout) 
		{
			if (numQueueItems > 0) break;
			Sleep(1000);
			// Timeout switchboard session if inactive
			if (isTimeout()) return 0;
		}	
		ts = time(NULL);

		size_t np = 0, dlen = 0;
		
		WaitForSingleObject(hQueueMutex, INFINITE);
		TQueueItem* QI = mFirstQueueItem;
		while (QI != NULL && np < 5) { ++np; dlen += QI->datalen;  QI = QI->next;}

		char szHttpPostUrl[300];
		getGatewayUrl(szHttpPostUrl, sizeof(szHttpPostUrl), dlen == 0);

		if (dlen) tBuffer = (char*)alloca(dlen);
		for (unsigned j=0; j<np; ++j)
		{
			QI = mFirstQueueItem;
			memcpy(tBuffer + cbBytes, QI->data, QI->datalen);
			cbBytes += QI->datalen;

			mFirstQueueItem = QI->next;
			mir_free(QI);
			--numQueueItems;
		}
		ReleaseMutex(hQueueMutex);


		nlhr.szUrl = szHttpPostUrl;
		nlhr.dataLength = (int)cbBytes;
		nlhr.pData = tBuffer;

		NETLIBHTTPREQUEST *nlhrReply = (NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION, 
			(WPARAM)proto->hNetlibUser, (LPARAM)&nlhr);

		if (nlhrReply == NULL)
		{
			s = NULL;
			return SOCKET_ERROR;
		}

		s = nlhrReply->nlc;
		bool noMsnHdr = true;
		if (nlhrReply->resultCode == 200)
		{
			for (int i=0; i < nlhrReply->headersCount; i++)
			{
				NETLIBHTTPHEADER& tHeader = nlhrReply->headers[i];
				if (_stricmp(tHeader.szName, "X-MSN-Messenger") != 0)
					continue;

				noMsnHdr = false;
				sessionClosed = strstr(tHeader.szValue, "Session=close") != NULL;
				processSessionData(tHeader.szValue);

				mReadAheadBufferPtr = mReadAheadBuffer = nlhrReply->pData;
				mEhoughData = nlhrReply->dataLength;

				nlhrReply->pData = NULL;
				nlhrReply->dataLength = 0;
				break;
			}
		}

		if (mEhoughData)
		{
			mGatewayTimeout = 1;
			mWaitPeriod = 60;
		}
		else if (dlen == 0) 
			mGatewayTimeout = min(mGatewayTimeout + 2, 20);

		CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT, 0, (LPARAM)nlhrReply);

		if (noMsnHdr) return SOCKET_ERROR;
	}
}

int ThreadData::recv(char* data, size_t datalen)
{
	if (proto->MyOptions.UseGateway && !proto->MyOptions.UseProxy)
		if (mType != SERVER_FILETRANS && mType != SERVER_P2P_DIRECT)
			return recv_dg(data, datalen);

	NETLIBBUFFER nlb = { data, (int)datalen, 0 };

LBL_RecvAgain:
	if (!mIsMainThread && !proto->MyOptions.UseGateway && !proto->MyOptions.UseProxy) 
	{
		mWaitPeriod = 60;
		NETLIBSELECT nls = { 0 };
		nls.cbSize = sizeof(nls);
		nls.dwTimeout = 1000;
		nls.hReadConns[0] = s;

		for (;;)
		{
			int ret = MSN_CallService(MS_NETLIB_SELECT, 0, (LPARAM)&nls);
			if (ret < 0) 
			{
				proto->MSN_DebugLog("Connection abortively closed, error %d", WSAGetLastError());
				return ret;
			}
			else if (ret == 0) 
			{
				if (isTimeout()) return 0;
			}
			else
				break;
		}
	}

	int ret = MSN_CallService(MS_NETLIB_RECV, (WPARAM)s, (LPARAM)&nlb);
	if (ret == 0) 
	{
		proto->MSN_DebugLog("Connection closed gracefully");
		return 0;
	}

	if (ret < 0) 
	{
		proto->MSN_DebugLog("Connection abortively closed, error %d", WSAGetLastError());
		return ret;
	}

	if (proto->MyOptions.UseGateway)
	{
		if (ret == 1 && *data == 0) 
		{
			int tOldTimeout = MSN_CallService(MS_NETLIB_SETPOLLINGTIMEOUT, WPARAM(s), 2);
			tOldTimeout += 2;
			if (tOldTimeout > 20)
				tOldTimeout = 20;

			MSN_CallService(MS_NETLIB_SETPOLLINGTIMEOUT, WPARAM(s), tOldTimeout);
			goto LBL_RecvAgain;
		}
		else MSN_CallService(MS_NETLIB_SETPOLLINGTIMEOUT, WPARAM(s), 1);
	}

	return ret;
}
