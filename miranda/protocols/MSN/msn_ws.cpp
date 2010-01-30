/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2006-2010 Boris Krasnovskiy.
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

	resetTimeout();

	if (proto->usingGateway && !(mType == SERVER_FILETRANS || mType == SERVER_P2P_DIRECT)) 
	{
		mGatewayTimeout = 2;
		MSN_CallService(MS_NETLIB_SETPOLLINGTIMEOUT, WPARAM(s), mGatewayTimeout);
	}

	int rlen = MSN_CallService(MS_NETLIB_SEND, (WPARAM)s, (LPARAM)&nlb);
	if (rlen == SOCKET_ERROR) 
	{
		// should really also check if sendlen is the same as datalen
		proto->MSN_DebugLog("Send failed: %d", WSAGetLastError());
		return FALSE;
	}

	return TRUE;
}

void ThreadData::resetTimeout(bool term)
{
	int timeout = term ? 10 : 120;
	mWaitPeriod = clock() + timeout * CLOCKS_PER_SEC;
}

bool ThreadData::isTimeout(void)
{
	bool res = false;

	if (mWaitPeriod >= clock()) return false;

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
		resetTimeout(true);
	}
	else
		resetTimeout();

	return false;
}

int ThreadData::recv(char* data, size_t datalen)
{
	NETLIBBUFFER nlb = { data, (int)datalen, 0 };

	if (!proto->usingGateway) 
	{
		resetTimeout();
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

LBL_RecvAgain:
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

	if (proto->usingGateway)
	{
		if (ret == 1 && *data == 0) 
		{
			if (sessionClosed || isTimeout()) return 0;
			if ((mGatewayTimeout += 2) > 20) mGatewayTimeout = 20;

			MSN_CallService(MS_NETLIB_SETPOLLINGTIMEOUT, WPARAM(s), mGatewayTimeout);
			goto LBL_RecvAgain;
		}
		else 
		{
			resetTimeout();
			mGatewayTimeout = 1;
			MSN_CallService(MS_NETLIB_SETPOLLINGTIMEOUT, WPARAM(s), mGatewayTimeout);
		}
	}

	return ret;
}
