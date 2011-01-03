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
#include "sdk/m_smileyadd.h"

static const char sttP2Pheader[] =
	"Content-Type: application/x-msnmsgrp2p\r\n"
	"P2P-Dest: %s\r\n\r\n";

static const char sttVoidNonce[] = "{00000000-0000-0000-0000-000000000000}";
static const char szUbnCall[] = "{F13B5C79-0126-458F-A29D-747C79C56530}";

void CMsnProto::p2p_logHeader(P2P_Header* hdrdata)
{
	MSN_DebugLog("--- Printing message header");
	MSN_DebugLog("    SessionID = %08X", hdrdata->mSessionID);
	MSN_DebugLog("    MessageID = %08X", hdrdata->mID);
#ifndef __GNUC__
	MSN_DebugLog("    Offset of data = %I64u", hdrdata->mOffset);
	MSN_DebugLog("    Total amount of data = %I64u", hdrdata->mTotalSize);
#else
	MSN_DebugLog("    Offset of data = %llu", hdrdata->mOffset);
	MSN_DebugLog("    Total amount of data = %llu", hdrdata->mTotalSize);
#endif
	MSN_DebugLog("    Data in packet = %lu bytes", hdrdata->mPacketLen);
	MSN_DebugLog("    Flags = %08X", hdrdata->mFlags);
	MSN_DebugLog("    Acknowledged session ID: %08X", hdrdata->mAckSessionID);
	MSN_DebugLog("    Acknowledged message ID: %08X", hdrdata->mAckUniqueID);
#ifndef __GNUC__
	MSN_DebugLog("    Acknowledged data size: %I64u", hdrdata->mAckDataSize);
#else
	MSN_DebugLog("    Acknowledged data size: %llu", hdrdata->mAckDataSize);
#endif
	MSN_DebugLog("------------------------");
}

char* getNewUuid(void)
{
	BYTE* p;
	UUID id;

	UuidCreate(&id);
	UuidToStringA(&id, &p);
	size_t len = strlen((char*)p) + 3;
	char* result = (char*)mir_alloc(len);
	mir_snprintf(result, len, "{%s}", p);
	_strupr(result);
	RpcStringFreeA(&p);
	return result;
}

unsigned CMsnProto::p2p_getMsgId(HANDLE hContact, int inc)
{
	unsigned cid = getDword(hContact, "p2pMsgId", 0);
	unsigned res = cid ? cid + inc : (MSN_GenRandom()); 
	if (res != cid) setDword(hContact, "p2pMsgId", res);
	
	return res;
}

bool CMsnProto::p2p_createListener(filetransfer* ft, directconnection *dc, MimeHeaders& chdrs)
{
	if (MyConnection.extIP == 0) return false;
	
	NETLIBBIND nlb = {0};
	nlb.cbSize = sizeof(nlb);
	nlb.pfnNewConnectionV2 = MSN_ConnectionProc;
	nlb.pExtra = this;
	HANDLE sb = (HANDLE) MSN_CallService(MS_NETLIB_BINDPORT, (WPARAM) hNetlibUser, (LPARAM)&nlb);
	if (sb == NULL) 
	{
		MSN_DebugLog("Unable to bind the port for incoming transfers");
		return false;
	}

	ThreadData* newThread = new ThreadData;
	newThread->mType = SERVER_P2P_DIRECT;
	newThread->mCaller = 3;
	newThread->mIncomingBoundPort = sb;
	newThread->mIncomingPort = nlb.wPort;
	strncpy(newThread->mCookie, ft->p2p_callID, sizeof(newThread->mCookie));
	newThread->mInitialContact = ft->std.hContact;

	newThread->startThread(&CMsnProto::p2p_filePassiveThread, this);

	char hostname[256] = "";

	gethostname(hostname, sizeof(hostname));
	PHOSTENT he = gethostbyname(hostname);

	hostname[0] = 0;
	bool ipInt = false;
	for (unsigned i = 0; i < sizeof(hostname) / 16; ++i) 
	{
		const PIN_ADDR addr = (PIN_ADDR)he->h_addr_list[i];

		if (addr == NULL) break;
		if (i != 0) strcat(hostname, " ");
		ipInt |= (addr->S_un.S_addr == MyConnection.extIP);
		strcat(hostname, inet_ntoa(*addr));
	}

	chdrs.addString("Bridge", "TCPv1");
	chdrs.addBool("Listening", true);

	if (dc->useHashedNonce)
		chdrs.addString("Hashed-Nonce", dc->mNonceToHash(), 2);
	else
		chdrs.addString("Nonce", dc->mNonceToText(), 2);

	bool bUbnCall = !ft->p2p_sessionid;

	if (!ipInt)
	{
		chdrs.addString("IPv4External-Addrs", mir_strdup(MyConnection.GetMyExtIPStr()), bUbnCall ? 6 : 2);
		chdrs.addLong("IPv4External-Port", nlb.wExPort, bUbnCall ? 4 : 0);
	}
	chdrs.addString("IPv4Internal-Addrs", mir_strdup(hostname), bUbnCall ? 6 : 2);
	chdrs.addLong("IPv4Internal-Port", nlb.wPort, bUbnCall ? 4 : 0);
	chdrs.addULong("SessionID", ft->p2p_sessionid);
	chdrs.addLong("SChannelState", 0);
	chdrs.addLong("Capabilities-Flags", 1);

	return true;
}

bool p2p_IsDlFileOk(filetransfer* ft)
{
	mir_sha1_ctx sha1ctx;
	BYTE sha[MIR_SHA1_HASH_SIZE];
	mir_sha1_init(&sha1ctx);

	bool res = false;

	int fileId = _topen(ft->std.tszCurrentFile, O_RDONLY | _O_BINARY, _S_IREAD);
	if (fileId != -1) 
	{
		BYTE buf[4096];
		int bytes;

		while((bytes = _read(fileId, buf, sizeof(buf))) > 0)
			mir_sha1_append(&sha1ctx, buf, bytes);

		_close(fileId);
		mir_sha1_finish(&sha1ctx, sha);

		char *szSha = arrayToHex(sha, MIR_SHA1_HASH_SIZE);
		char *szAvatarHash = MSN_GetAvatarHash(ft->p2p_object);

		res = szAvatarHash != NULL && _stricmp(szAvatarHash, szSha) == 0;

		mir_free(szSha);
		mir_free(szAvatarHash);
	}
	return res;
}



/////////////////////////////////////////////////////////////////////////////////////////
// sttSavePicture2disk - final handler for avatars downloading

void CMsnProto::p2p_pictureTransferFailed(filetransfer* ft)
{
	switch(ft->p2p_type)
	{
	case MSN_APPID_AVATAR:
	case MSN_APPID_AVATAR2:
		{
			PROTO_AVATAR_INFORMATION AI = {0};
			AI.cbSize = sizeof(AI);
			AI.hContact = ft->std.hContact;
			deleteSetting(ft->std.hContact, "AvatarHash");
			SendBroadcast(AI.hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, &AI, 0);
		}
		break;
	}
	_tremove(ft->std.tszCurrentFile);
}

void CMsnProto::p2p_savePicture2disk(filetransfer* ft)
{
	ft->close();

	if (p2p_IsDlFileOk(ft))
	{
		int fileId = _topen(ft->std.tszCurrentFile, O_RDONLY | _O_BINARY, _S_IREAD);
		if (fileId == -1) 
		{
			p2p_pictureTransferFailed(ft);
			return;
		}

		const char* ext;
		int format;
		BYTE buf[6];

		int bytes = _read(fileId, buf, sizeof(buf));
		_close(fileId);
		if (bytes > 4)
			format = MSN_GetImageFormat(buf, &ext);
		else
		{
			p2p_pictureTransferFailed(ft);
			return;
		}

		switch(ft->p2p_type)
		{
		case MSN_APPID_AVATAR:
		case MSN_APPID_AVATAR2:
			{
				PROTO_AVATAR_INFORMATION AI = {0};
				AI.cbSize = sizeof(AI);
				AI.hContact = ft->std.hContact;
				MSN_GetAvatarFileName(AI.hContact, AI.filename, sizeof(AI.filename) - 3);

				AI.format = format;
				strcpy(strchr(AI.filename, '\0'), ext);

				TCHAR *aname = mir_a2t(AI.filename);
				_trename(ft->std.tszCurrentFile, aname);
				mir_free(aname);

				setString(ft->std.hContact, "PictSavedContext", ft->p2p_object);
				SendBroadcast(AI.hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, &AI, 0);

				// Store also avatar hash
				char *szAvatarHash = MSN_GetAvatarHash(ft->p2p_object);
				setString(ft->std.hContact, "AvatarSavedHash", szAvatarHash);
				MSN_DebugLog("Avatar for contact %08x saved to file '%s'", AI.hContact, AI.filename);
				mir_free(szAvatarHash);
			}
			break;

		case MSN_APPID_CUSTOMSMILEY:
		case MSN_APPID_CUSTOMANIMATEDSMILEY:
			{
				SMADD_CONT cont;
				cont.cbSize = sizeof(SMADD_CONT);
				cont.hContact = ft->std.hContact;
				cont.type = 1;

				TCHAR *extt = mir_a2t(ext);
				TCHAR* pathcpy = mir_tstrdup(ft->std.tszCurrentFile);
				_tcscpy(_tcsrchr(pathcpy, '.') + 1, extt);
				_trename(ft->std.tszCurrentFile, pathcpy);
				mir_free(extt);

				cont.path = pathcpy;

				MSN_CallService(MS_SMILEYADD_LOADCONTACTSMILEYS, 0, (LPARAM)&cont);
				mir_free(pathcpy);
			}
			break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendAck - sends MSN P2P acknowledgement to the received message

static const char sttVoidSession[] = "ACHTUNG!!! an attempt made to send a message via the empty session";

void  CMsnProto::p2p_sendMsg(HANDLE hContact, unsigned appId, P2P_Header& hdrdata, char* msgbody, size_t msgsz)
{
	unsigned msgType;

	ThreadData* info = MSN_GetP2PThreadByContact(hContact);
	if (info == NULL)
	{
		msgType = 0;
		bool isOffline;
		info = MSN_StartSB(hContact, isOffline);
	}
	else if (info->mType == SERVER_P2P_DIRECT) msgType = 1;
	else msgType = 2;

	const unsigned fportion = msgType == 1 ? 1352 : 1202;

	char* buf = (char*) alloca(sizeof(sttP2Pheader)+ MSN_MAX_EMAIL_LEN + 
		sizeof(P2P_Header) + 20 + fportion);

	size_t offset = 0;
	do 
	{
		size_t portion = msgsz - offset;
		if (portion > fportion) portion = fportion;

		char* p = buf;

		// add message header
		if (msgType == 1) 
		{
			*(unsigned*)p = (unsigned)(portion + sizeof(P2P_Header)); p += 4;
		}
		else
		{
			char email[MSN_MAX_EMAIL_LEN];
			if (getStaticString(hContact, "e-mail", email, sizeof(email)))
			{
				filetransfer *ft = p2p_getThreadSession(hContact, SERVER_SWITCHBOARD);
				if (ft == NULL) ft = p2p_getThreadSession(hContact, SERVER_DISPATCH);
				if (ft == NULL) return;
				p += sprintf(p, sttP2Pheader, _strlwr(ft->p2p_dest));
			}
			else
				p += sprintf(p, sttP2Pheader,  _strlwr(email));
		}

		// add message body
		P2P_Header* ph = (P2P_Header*)p; *ph = hdrdata; p += sizeof(P2P_Header);
		
		if (msgsz)
		{
			ph->mPacketLen = (unsigned)portion;
			ph->mOffset = offset;
			ph->mTotalSize = msgsz;
			
			memcpy(p, msgbody + offset, portion); p += portion;
		}

		// add message footer
		if (msgType != 1) 
		{ 
			*(unsigned*)p = htonl(appId);
			p += 4;
		}

		switch (msgType) 
		{
			case 0:
				MsgQueue_Add(hContact, 'D', buf, p - buf, NULL);
				break;

			case 1: 
				info->send(buf, p - buf);
				break;

			case 2:
				info->sendRawMessage('D', buf, p - buf);
				break;
		}
		offset += portion;
	} 
	while( offset < msgsz );
}


void  CMsnProto::p2p_sendAck(HANDLE hContact, P2P_Header* hdrdata)
{	
	if (hdrdata == NULL) return;

	P2P_Header tHdr = {0};

	tHdr.mSessionID = hdrdata->mSessionID;
	tHdr.mID = p2p_getMsgId(hContact, 1);
	tHdr.mAckDataSize = hdrdata->mTotalSize;
	tHdr.mFlags = 2;
	tHdr.mAckSessionID = hdrdata->mID;
	tHdr.mAckUniqueID = hdrdata->mAckSessionID;

	p2p_sendMsg(hContact, 0, tHdr, NULL, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendEndSession - sends MSN P2P file transfer end packet

void  CMsnProto::p2p_sendAbortSession(filetransfer* ft)
{
	if (ft == NULL) 
	{
		MSN_DebugLog(sttVoidSession);
		return;
	}

	P2P_Header tHdr = {0};

	tHdr.mSessionID = ft->p2p_sessionid;
	tHdr.mAckSessionID = ft->p2p_sendmsgid;

	if (ft->std.flags & PFTS_SENDING) 
	{
		tHdr.mFlags = 0x40;
		tHdr.mID = p2p_getMsgId(ft->std.hContact, 1);
		tHdr.mAckSessionID = tHdr.mID - 2;
	}
	else 
	{
		tHdr.mID = p2p_getMsgId(ft->std.hContact, 1);
		tHdr.mAckUniqueID = 0x8200000f;
		tHdr.mFlags = 0x80;
		tHdr.mAckDataSize = ft->std.currentFileSize;
	}

	p2p_sendMsg(ft->std.hContact, 0, tHdr, NULL, 0);
	ft->ts = time(NULL);
}

void  CMsnProto::p2p_sendRedirect(filetransfer* ft)
{
	if (ft == NULL) 
	{
		MSN_DebugLog(sttVoidSession);
		return;
	}

	P2P_Header tHdr = {0};

	tHdr.mSessionID = ft->p2p_sessionid;
	tHdr.mID = p2p_getMsgId(ft->std.hContact, 1);
	tHdr.mFlags = 0x01;
	tHdr.mAckSessionID = ft->p2p_sendmsgid;
	tHdr.mAckDataSize = ft->std.currentFileProgress;

	p2p_sendMsg(ft->std.hContact, 0, tHdr, NULL, 0);

	ft->tTypeReq = MSN_GetP2PThreadByContact(ft->std.hContact) ? SERVER_P2P_DIRECT : SERVER_SWITCHBOARD;
	ft->ts = time(NULL);
	ft->p2p_waitack = true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendSlp - send MSN P2P SLP packet

void  CMsnProto::p2p_sendSlp(
	filetransfer*	ft,
	MimeHeaders&	pHeaders,
	int				iKind,
	const char*		szContent,
	size_t			szContLen)
{
	if (ft == NULL) 
	{
		MSN_DebugLog(sttVoidSession);
		return;
	}

	char* buf = (char*)alloca(pHeaders.getLength() + szContLen + 1000);
	char* p = buf;

	if (ft->p2p_dest == NULL)
	{
		char szEmail[MSN_MAX_EMAIL_LEN];
		if (!getStaticString(ft->std.hContact, "e-mail", szEmail, sizeof(szEmail)))
			ft->p2p_dest = _strlwr(mir_strdup(szEmail));
	}

	switch (iKind) 
	{
		case -3:   p += sprintf(p, "ACK MSNMSGR:%s MSNSLP/1.0", ft->p2p_dest); break;
		case -2:   p += sprintf(p, "INVITE MSNMSGR:%s MSNSLP/1.0", ft->p2p_dest); break;
		case -1:   p += sprintf(p, "BYE MSNMSGR:%s MSNSLP/1.0", ft->p2p_dest); break;
		case 200:  p += sprintf(p, "MSNSLP/1.0 200 OK");	break;
		case 481:  p += sprintf(p, "MSNSLP/1.0 481 No Such Call"); break;
		case 500:  p += sprintf(p, "MSNSLP/1.0 500 Internal Error"); break;
		case 603:  p += sprintf(p, "MSNSLP/1.0 603 DECLINE"); break;
		case 1603: p += sprintf(p, "MSNSLP/1.0 603 Decline"); break;
		default: return;
	}
	
	if (iKind < 0) 
	{
		mir_free(ft->p2p_branch);
		ft->p2p_branch = getNewUuid();
	}

	p += sprintf(p,
		"\r\nTo: <msnmsgr:%s>\r\n"
		"From: <msnmsgr:%s>\r\n"
		"Via: MSNSLP/1.0/TLP ;branch=%s\r\n", ft->p2p_dest, MyOptions.szEmail, ft->p2p_branch);

	p = pHeaders.writeToBuffer(p);

	p += sprintf(p, "Content-Length: %d\r\n\r\n", szContLen + 2);
	memcpy(p, szContent, szContLen);
	p += szContLen;

	if (!(myFlags & 0x4000000) || ft->p2p_sessionid || 
		MSN_GetThreadByContact(ft->std.hContact, SERVER_P2P_DIRECT))
	{
		P2P_Header tHdr = {0};

		tHdr.mID = p2p_getMsgId(ft->std.hContact, 1);
		tHdr.mAckSessionID = ft->p2p_acksessid;

		switch (iKind) 
		{
			case -1: case 500: case 603: 
				ft->p2p_byemsgid  = tHdr.mID; 
				break;
		}

		p2p_sendMsg(ft->std.hContact, 0, tHdr, buf, p - buf);
		ft->p2p_waitack = true;
	}
	else
		msnNsThread->sendPacket("UUN", "%s 3 %d\r\n%s\r\n", ft->p2p_dest, p - buf + 1, buf);

	ft->ts = time(NULL);
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendBye - closes P2P session

void  CMsnProto::p2p_sendBye(filetransfer* ft)
{
	if (ft == NULL) 
	{
		MSN_DebugLog(sttVoidSession);
		return;
	}

	MimeHeaders tHeaders(5);
	tHeaders.addString("CSeq", "0 ");
	tHeaders.addString("Call-ID", ft->p2p_callID);
	tHeaders.addLong("Max-Forwards", 0);
	tHeaders.addString("Content-Type", "application/x-msnmsgr-sessionclosebody");

	char szContents[50];
	p2p_sendSlp(ft, tHeaders, -1, szContents,
		mir_snprintf(szContents, sizeof(szContents), "SessionID: %lu\r\nSChannelState: 0\r\n\r\n%c",
		ft->p2p_sessionid, 0));
}

void  CMsnProto::p2p_sendCancel(filetransfer* ft)
{
	p2p_sendBye(ft);
	p2p_sendAbortSession(ft);
}

void  CMsnProto::p2p_sendNoCall(filetransfer* ft)
{
	if (ft == NULL) 
	{
		MSN_DebugLog(sttVoidSession);
		return;
	}

	MimeHeaders tHeaders(5);
	tHeaders.addString("CSeq", "0 ");
	tHeaders.addString("Call-ID", ft->p2p_callID);
	tHeaders.addLong("Max-Forwards", 0);
	tHeaders.addString("Content-Type", "application/x-msnmsgr-session-failure-respbody");

	char szContents[50];
	p2p_sendSlp(ft, tHeaders, 481, szContents,
		mir_snprintf(szContents, sizeof(szContents), "SessionID: %lu\r\nSChannelState: 0\r\n\r\n%c",
		ft->p2p_sessionid, 0));
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendStatus - send MSN P2P status and its description

void  CMsnProto::p2p_sendStatus(filetransfer* ft, long lStatus)
{
	if (ft == NULL) 
	{
		MSN_DebugLog(sttVoidSession);
		return;
	}

	MimeHeaders tHeaders(5);
	tHeaders.addString("CSeq", "1 ");
	tHeaders.addString("Call-ID", ft->p2p_callID);
	tHeaders.addLong("Max-Forwards", 0);
	if (lStatus != 1603)
	{
		tHeaders.addString("Content-Type", "application/x-msnmsgr-sessionreqbody");

		char szContents[50];
		p2p_sendSlp(ft, tHeaders, lStatus, szContents,
			mir_snprintf(szContents, sizeof(szContents), "SessionID: %lu\r\nSChannelState: 0\r\n\r\n%c",
			ft->p2p_sessionid, 0));
	}
	else
	{
		tHeaders.addString("Content-Type", "application/x-msnmsgr-transrespbody");
		p2p_sendSlp(ft, tHeaders, lStatus, NULL, 0);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_connectTo - connects to a remote P2P server

static const char p2p_greeting[8] = { 4, 0, 0, 0, 'f', 'o', 'o', 0  };

static void sttSendPacket(ThreadData* T, P2P_Header& hdr)
{
	DWORD len = sizeof(P2P_Header);
	T->send((char*)&len, sizeof(DWORD));
	T->send((char*)&hdr, sizeof(P2P_Header));
}

bool CMsnProto::p2p_connectTo(ThreadData* info)
{
	NETLIBOPENCONNECTION tConn = {0};
	tConn.cbSize = sizeof(tConn);
	tConn.szHost = info->mServer;
	tConn.flags = NLOCF_V2;
	tConn.timeout = 5;

	char* tPortDelim = strrchr(info->mServer, ':');
	if (tPortDelim != NULL) 
	{
		*tPortDelim = '\0';
		tConn.wPort = (WORD)atol(tPortDelim + 1);
	}

	MSN_DebugLog("Connecting to %s:%d", tConn.szHost, tConn.wPort);

	directconnection *dc = p2p_getDCByCallID(info->mCookie, info->mInitialContact);
	if (dc == NULL) return false;

	info->s = (HANDLE)MSN_CallService(MS_NETLIB_OPENCONNECTION, (WPARAM)hNetlibUser, (LPARAM)&tConn);
	if (info->s == NULL)
	{
		TWinErrorCode err;
		MSN_DebugLog("Connection Failed (%d): %s", err.mErrorCode, err.getText());
		p2p_unregisterDC(dc);
		return false;
	}
	info->send(p2p_greeting, sizeof(p2p_greeting));

	P2P_Header reply = {0};
	reply.mID = p2p_getMsgId(info->mInitialContact, 1);
	reply.mFlags = 0x100;

	if (dc->useHashedNonce)
		memcpy(&reply.mAckSessionID, dc->mNonce, sizeof(UUID));
	else
		dc->xNonceToBin((UUID*)&reply.mAckSessionID);

	sttSendPacket(info, reply);

	long cbPacketLen;
	HReadBuffer buf(info, 0);
	BYTE* p;
	if ((p = buf.surelyRead(4)) == NULL) 
	{
		MSN_DebugLog("Error reading data, closing filetransfer");
		p2p_unregisterDC(dc);
		return false;
	}

	cbPacketLen = *(long*)p;
	if ((p = buf.surelyRead(cbPacketLen)) == NULL)
	{
		p2p_unregisterDC(dc);
		return false;
	}

	bool cookieMatch;
	P2P_Header* pCookie = (P2P_Header*)p;

	if (dc->useHashedNonce) 
	{
		char* hnonce = dc->calcHashedNonce((UUID*)&pCookie->mAckSessionID);
		cookieMatch = strcmp(hnonce, dc->xNonce) == 0;
		mir_free(hnonce);
	}
	else
		cookieMatch = memcmp(&pCookie->mAckSessionID, &reply.mAckSessionID, sizeof(UUID)) == 0;

	if (!cookieMatch) 
	{
		MSN_DebugLog("Invalid cookie received, exiting");
		p2p_unregisterDC(dc);
		return false;
	}

	p2p_unregisterDC(dc);
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_listen - acts like a local P2P server

bool CMsnProto::p2p_listen(ThreadData* info)
{
	directconnection *dc = p2p_getDCByCallID(info->mCookie, info->mInitialContact);
	if (dc == NULL) return false;

	switch(WaitForSingleObject(info->hWaitEvent, 6000)) 
	{
	case WAIT_TIMEOUT:
	case WAIT_FAILED:
		MSN_DebugLog("Incoming connection timed out, closing file transfer");
		MSN_StartP2PTransferByContact(info->mInitialContact);
LBL_Error:
		p2p_unregisterDC(dc);
		MSN_DebugLog("File listen failed");
		return false;
	}

	HReadBuffer buf(info, 0);
	BYTE* p;

	if ((p = buf.surelyRead(8)) == NULL)
		goto LBL_Error;

	if (memcmp(p, p2p_greeting, 8) != 0) 
	{
		MSN_DebugLog("Invalid input data, exiting");
		goto LBL_Error;
	}

	if ((p = buf.surelyRead(4)) == NULL) 
	{
		MSN_DebugLog("Error reading data, closing filetransfer");
		goto LBL_Error;
	}

	long cbPacketLen = *(long*)p;
	if ((p = buf.surelyRead(cbPacketLen)) == NULL)
		goto LBL_Error;

	bool cookieMatch;
	P2P_Header* pCookie = (P2P_Header*)p;

	if (dc->useHashedNonce) 
	{
		char* hnonce = dc->calcHashedNonce((UUID*)&pCookie->mAckSessionID);
		cookieMatch = strcmp(hnonce, dc->xNonce) == 0;
		mir_free(hnonce);
		memcpy(&pCookie->mAckSessionID, dc->mNonce, sizeof(UUID));
	}
	else
		cookieMatch = memcmp(&pCookie->mAckSessionID, dc->mNonce, sizeof(UUID)) == 0;

	if (!cookieMatch) {
		MSN_DebugLog("Invalid cookie received, exiting");
		goto LBL_Error;
	}

	pCookie->mID = p2p_getMsgId(info->mInitialContact, 1);
	sttSendPacket(info, *pCookie);

	p2p_unregisterDC(dc);
	return true;
}

LONG  CMsnProto::p2p_sendPortion(filetransfer* ft, ThreadData* T)
{
	LONG trid;
	char databuf[1500], *p = databuf;

	// Compute the amount of data to send
	const unsigned long fportion = T->mType == SERVER_P2P_DIRECT ? 1352 : 1202;
	const unsigned __int64 dt = ft->std.currentFileSize - ft->std.currentFileProgress;
	const unsigned long portion = dt > fportion ? fportion : dt;

	// Fill data size for direct transfer

	if (T->mType != SERVER_P2P_DIRECT)
		p += sprintf(p, sttP2Pheader, ft->p2p_dest);
	else
	{
		*(unsigned long*)p = portion + sizeof(P2P_Header);
		p += sizeof(unsigned long);
	}

	// Fill P2P header
	P2P_Header* H = (P2P_Header*) p;
	p += sizeof(P2P_Header);

	memset(H, 0, sizeof(P2P_Header));
	H->mSessionID = ft->p2p_sessionid;
	H->mID = ft->p2p_sendmsgid;
	H->mFlags = ft->p2p_appID == MSN_APPID_FILE ? 0x01000030 : 0x20;
	H->mTotalSize = ft->std.currentFileSize;
	H->mOffset = ft->std.currentFileProgress;
	H->mPacketLen = portion;
	H->mAckSessionID = ft->p2p_acksessid;

	// Fill data (payload) for transfer
	_read(ft->fileId, p, portion);
	p += portion;

	if (T->mType == SERVER_P2P_DIRECT)
		trid = T->send(databuf, p - databuf);
	else
	{
		// Define packet footer for server transfer
		*(unsigned long *)p = htonl(ft->p2p_appID);
		p += sizeof(unsigned long);

		trid = T->sendRawMessage('D', (char *)databuf, p - databuf);
	}

	if (trid != 0) 
	{
		ft->std.totalProgress += portion;
		ft->std.currentFileProgress += portion;
		if (ft->p2p_appID == MSN_APPID_FILE && clock() >= ft->nNotify)
		{
			SendBroadcast(ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, (LPARAM)&ft->std);
			ft->nNotify = clock() + 500;
		}
	}
	else
		MSN_DebugLog(" Error sending");
	ft->ts = time(NULL);
	ft->p2p_waitack = true;

	return trid;
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendFeedThread - sends a file via server

void __cdecl CMsnProto::p2p_sendFeedThread(void* arg)
{
	ThreadData* info = (ThreadData*)arg;

	HANDLE hContact = info->mInitialContact;
	info->contactJoined(hContact);
	info->mInitialContact = NULL;

	MSN_DebugLog("File send thread started");

	switch(WaitForSingleObject(info->hWaitEvent, 6000)) 
	{
		case WAIT_FAILED:
			MSN_DebugLog("File send wait failed");
			return;
	}

	HANDLE hLockHandle = NULL;

	filetransfer *ft = p2p_getSessionByCallID(info->mCookie, hContact);

	if (ft != NULL && WaitForSingleObject(ft->hLockHandle, 2000) == WAIT_OBJECT_0)
	{
		hLockHandle = ft->hLockHandle;

		if (ft->p2p_sendmsgid == 0)
			ft->p2p_sendmsgid = p2p_getMsgId(hContact, 1);

		ThreadData* T = MSN_GetP2PThreadByContact(hContact);
		if (T != NULL)
			ft->tType = T->mType;

		ReleaseMutex(hLockHandle);
	}
	else
		return;

	bool fault = false;
	while (WaitForSingleObject(hLockHandle, 2000) == WAIT_OBJECT_0 &&
		ft->std.currentFileProgress < ft->std.currentFileSize && !ft->bCanceled)
	{
		ThreadData* T = MSN_GetThreadByContact(ft->std.hContact, ft->tType);
		bool cfault = (T == NULL || p2p_sendPortion(ft, T) == 0);

		if (cfault) 
		{
			if (fault) 
			{
				MSN_DebugLog("File send failed");
				break;
			}
			else
				SleepEx(3000, TRUE);  // Allow 3 sec for redirect request
		}
		fault = cfault;

		ReleaseMutex(hLockHandle);

		if (T != NULL && T->mType != SERVER_P2P_DIRECT)
			WaitForSingleObject(T->hWaitEvent, 5000);
	}
	ReleaseMutex(hLockHandle);
	SendBroadcast(ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, (LPARAM)&ft->std);
	MSN_DebugLog("File send thread completed");
}


void  CMsnProto::p2p_sendFeedStart(filetransfer* ft)
{
	if (ft->std.flags & PFTS_SENDING)
	{
		ThreadData* newThread = new ThreadData;
		newThread->mType = SERVER_FILETRANS;
		strcpy(newThread->mCookie, ft->p2p_callID);
		newThread->mInitialContact = ft->std.hContact;
		newThread->startThread(&CMsnProto::p2p_sendFeedThread, this);
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendFileDirectly - sends a file via MSN P2P protocol

void CMsnProto::p2p_sendRecvFileDirectly(ThreadData* info)
{
	long cbPacketLen = 0;
	long state = 0;

	HReadBuffer buf(info, 0);

	HANDLE hContact = info->mInitialContact;
	info->contactJoined(hContact);
	info->mInitialContact = NULL;

	MSN_StartP2PTransferByContact(hContact);
	p2p_redirectSessions(hContact);
	p2p_startSessions(hContact);

	for (;;) 
	{
		long len = state ? cbPacketLen : 4;

		BYTE* p = buf.surelyRead(len);

		if (p == NULL)
			break;

		if (state == 0)
			cbPacketLen = *(long*)p;
		else
			p2p_processMsg(info, (char*)p);

		state = (state + 1) % 2;
	}

	info->contactLeft(hContact);
	p2p_redirectSessions(hContact);
}

/////////////////////////////////////////////////////////////////////////////////////////
// bunch of thread functions to cover all variants of P2P file transfers

void __cdecl CMsnProto::p2p_fileActiveThread(void* arg)
{
	ThreadData* info = (ThreadData*)arg;

	MSN_DebugLog("p2p_fileActiveThread() started: connecting to '%s'", info->mServer);

	if (p2p_connectTo(info))
		p2p_sendRecvFileDirectly(info);

	MSN_DebugLog("p2p_fileActiveThread() completed: connecting to '%s'", info->mServer);
}

void __cdecl CMsnProto::p2p_filePassiveThread(void* arg)
{
	ThreadData* info = (ThreadData*)arg;

	MSN_DebugLog("p2p_filePassiveThread() started: listening");

	if (p2p_listen(info))
		p2p_sendRecvFileDirectly(info);

	MSN_DebugLog("p2p_filePassiveThread() completed");
}


void CMsnProto::p2p_InitFileTransfer(
	ThreadData*		info,
	MimeHeaders&	tFileInfo,
	MimeHeaders&	tFileInfo2, 
	HANDLE hContact)
{
	if (info->mJoinedCount == 0)
		return;

	const char	*szCallID = tFileInfo["Call-ID"],
				*szBranch = tFileInfo["Via"];

	if (szBranch != NULL) {
		szBranch = strstr(szBranch, "branch=");
		if (szBranch != NULL)
			szBranch += 7;
	}
	if (szCallID == NULL || szBranch == NULL) {
		MSN_DebugLog("Ignoring invalid invitation: CallID='%s', szBranch='%s'", szCallID, szBranch);
		return;
	}

	const char	*szSessionID = tFileInfo2["SessionID"],
				*szEufGuid   = tFileInfo2["EUF-GUID"],
				*szContext   = tFileInfo2["Context"],
				*szAppId     = tFileInfo2["AppID"];

	int dwAppID = szAppId ? atol(szAppId) : -1;

	if (szSessionID == NULL || dwAppID == -1 || szEufGuid == NULL || szContext == NULL) 
	{
		MSN_DebugLog("Ignoring invalid invitation: SessionID='%s', AppID=%ld, Branch='%s',Context='%s'",
			szSessionID, dwAppID, szEufGuid, szContext);
		return;
	}

	szContext = MSN_Base64Decode(szContext);

	filetransfer* ft = new filetransfer(this);
	ft->p2p_acksessid = MSN_GenRandom();
	ft->p2p_sessionid = strtoul(szSessionID, NULL, 10);
	ft->p2p_appID = dwAppID == MSN_APPID_AVATAR ? MSN_APPID_AVATAR2 : dwAppID;
	ft->p2p_type = dwAppID;
	ft->p2p_ackID = dwAppID == MSN_APPID_FILE ? 2000 : 1000;
	replaceStr(ft->p2p_callID, szCallID);
	replaceStr(ft->p2p_branch, szBranch);
	ft->std.hContact = info->mJoinedContacts[0];

	p2p_registerSession(ft);

	switch (dwAppID)
	{
		case MSN_APPID_AVATAR:
		case MSN_APPID_AVATAR2:
			if (!_stricmp(szEufGuid, "{A4268EEC-FEC5-49E5-95C3-F126696BDBF6}")) {
				DBVARIANT dbv;
				bool pictmatch = !getString("PictObject", &dbv);
				if (pictmatch) 
				{
					UrlDecode(dbv.pszVal);

					ezxml_t xmlcon = ezxml_parse_str((char*)szContext, strlen(szContext));
					ezxml_t xmldb = ezxml_parse_str(dbv.pszVal, strlen(dbv.pszVal));

					const char *szCtBuf = ezxml_attr(xmlcon, "SHA1C");
					if (szCtBuf)
					{
						const char *szPtBuf = ezxml_attr(xmldb,  "SHA1C");
						pictmatch = szPtBuf && strcmp(szCtBuf, szPtBuf) == 0;
					}
					else
					{
						const char *szCtBuf = ezxml_attr(xmlcon, "SHA1D");
						const char *szPtBuf = ezxml_attr(xmldb,  "SHA1D");
						pictmatch = szCtBuf && szPtBuf && strcmp(szCtBuf, szPtBuf) == 0;
					}

					ezxml_free(xmlcon);
					ezxml_free(xmldb);
					MSN_FreeVariant(&dbv);
				}
				if (pictmatch) 
				{
					char szFileName[MAX_PATH];
					MSN_GetAvatarFileName(NULL, szFileName, SIZEOF(szFileName));
					ft->fileId = _open(szFileName, O_RDONLY | _O_BINARY, _S_IREAD);
					if (ft->fileId == -1) 
					{
						p2p_sendStatus(ft, 603);
						MSN_ShowError("Your avatar not set correctly. Avatar should be set in View/Change My Details | Avatar");
						MSN_DebugLog("Unable to open avatar file '%s', error %d", szFileName, errno);
						p2p_unregisterSession(ft);
					}
					else
					{
						mir_free(ft->std.tszCurrentFile);
						ft->std.tszCurrentFile = mir_a2t(szFileName);
//						MSN_DebugLog("My avatar file opened for %s as %08p::%d", szEmail, ft, ft->fileId);
						ft->std.totalBytes = ft->std.currentFileSize = _filelengthi64(ft->fileId);
						ft->std.flags |= PFTS_SENDING;

						//---- send 200 OK Message
						p2p_sendStatus(ft, 200);
						p2p_sendFeedStart(ft);
					}
				}
				else
				{
					p2p_sendStatus(ft, 603);
					MSN_DebugLog("Requested avatar does not match current avatar");
					p2p_unregisterSession(ft);
				}
			}
			break;

		case MSN_APPID_FILE:
			if (!_stricmp(szEufGuid, "{5D3E02AB-6190-11D3-BBBB-00C04F795683}")) 
			{
				wchar_t* wszFileName = ((HFileContext*)szContext)->wszFileName;
				{	for (wchar_t* p = wszFileName; *p != 0; p++)
					{	
						switch(*p) 
						{
						case ':': case '?': case '/': case '\\': case '*':
							*p = '_';
						}	
					}	
				}

				mir_free(ft->std.tszCurrentFile);
				ft->std.tszCurrentFile = mir_u2t(wszFileName);

				ft->std.totalBytes = ft->std.currentFileSize = ((HFileContext*)szContext)->dwSize;
				ft->std.totalFiles = 1;

				TCHAR tComment[40];
				mir_sntprintf(tComment, SIZEOF(tComment), TranslateT("%I64u bytes"), ft->std.currentFileSize);

				PROTORECVFILET pre = {0};
				pre.flags = PREF_TCHAR;
				pre.fileCount = 1;
				pre.timestamp = time(NULL);
				pre.tszDescription = tComment;
				pre.ptszFiles = &ft->std.tszCurrentFile;
				pre.lParam = (LPARAM)ft;

				CCSDATA ccs;
				ccs.hContact = ft->std.hContact;
				ccs.szProtoService = PSR_FILE;
				ccs.wParam = 0;
				ccs.lParam = (LPARAM)&pre;
				MSN_CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
			}
			break;

		case MSN_APPID_WEBCAM:
			if (!_stricmp(szEufGuid, "{4BD96FC0-AB17-4425-A14A-439185962DC8}")) {
				MSN_ShowPopup(ft->std.hContact,
					TranslateT("Contact tried to send its webcam data (currently not supported)"), 
					MSN_ALLOW_MSGBOX);
			}
			if (!_stricmp(szEufGuid, "{1C9AA97E-9C05-4583-A3BD-908A196F1E92}")) {
				MSN_ShowPopup(ft->std.hContact,
					TranslateT("Contact tried to view our webcam data (currently not supported)"), 
					MSN_ALLOW_MSGBOX);
			}
			p2p_sendStatus(ft, 603);
			p2p_unregisterSession(ft);
			break;

		default:
			p2p_sendStatus(ft, 603);
			p2p_unregisterSession(ft);
			MSN_DebugLog("Invalid or unknown AppID/EUF-GUID combination: %ld/%s", dwAppID, szEufGuid);
			break;
	}

	mir_free((void*)szContext);
}

void CMsnProto::p2p_InitDirectTransfer(MimeHeaders& tFileInfo, MimeHeaders& tFileInfo2, HANDLE hContact)
{
	const char	*szCallID = tFileInfo["Call-ID"],
				*szBranch = tFileInfo["Via"],
				*szConnType = tFileInfo2["Conn-Type"],
				*szUPnPNat = tFileInfo2["UPnPNat"],
				*szNetID = tFileInfo2["NetID"],
				*szICF = tFileInfo2["ICF"],
				*szHashedNonce = tFileInfo2["Hashed-Nonce"];

	if (szBranch != NULL) 
	{
		szBranch = strstr(szBranch, "branch=");
		if (szBranch != NULL)
			szBranch += 7;
	}
	if (szCallID == NULL || szBranch == NULL) 
	{
		MSN_DebugLog("Ignoring invalid invitation: CallID='%s', Branch='%s'", szCallID, szBranch);
		return;
	}

	if (szConnType == NULL || szUPnPNat == NULL || szICF == NULL || szNetID == NULL) 
	{
		MSN_DebugLog("Ignoring invalid invitation: ConnType='%s', UPnPNat='%s', ICF='%s', NetID='%s'",
			szConnType, szUPnPNat, szICF, szNetID);
		return;
	}

	filetransfer ftl(this), *ft = p2p_getSessionByCallID(szCallID, hContact);
	if (!ft || !ft->p2p_sessionid)
	{
		ft = &ftl;
		ft->std.hContact = hContact;
		replaceStr(ft->p2p_callID, szCallID);
		replaceStr(ft->p2p_branch, szBranch);
	}
	else
	{
		replaceStr(ft->p2p_callID, szCallID);
		replaceStr(ft->p2p_branch, szBranch);
		ft->p2p_acksessid = MSN_GenRandom();
/*
		if (p2p_isAvatarOnly(ft->std.hContact)) 
		{
	//		p2p_sendStatus(ft, 1603);
	//		return;
		}
		else
			SendBroadcast(ft->std.hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, ft, 0);
*/
	}

	directconnection *dc = p2p_getDCByCallID(szCallID, hContact);
	if (dc)
	{
		p2p_sendStatus(ft, 1603);
		return;
	}

	dc = new directconnection(szCallID, hContact);
	dc->useHashedNonce = szHashedNonce != NULL;
	dc->hContact = ft->std.hContact;
	if (dc->useHashedNonce)
		dc->xNonce = mir_strdup(szHashedNonce);
	p2p_registerDC(dc);

	MimeHeaders tResult(20);
	tResult.addString("CSeq", "1 ");
	tResult.addString("Call-ID", szCallID);
	tResult.addLong("Max-Forwards", 0);

	MyConnectionType conType = {0};

	conType.extIP = atol(szNetID);
	conType.SetUdpCon(szConnType);
	conType.upnpNAT = strcmp(szUPnPNat, "true") == 0;
	conType.icf = strcmp(szICF, "true") == 0;
	conType.CalculateWeight();

	MimeHeaders chdrs(12);
	bool listen = false;

	MSN_DebugLog("Connection weight, his: %d mine: %d", conType.weight, MyConnection.weight);
	if (conType.weight <= MyConnection.weight)
		listen = p2p_createListener(ft, dc, chdrs);

	if (!listen) 
	{
		chdrs.addString("Bridge", "TCPv1");
		chdrs.addBool("Listening", false);

		if (dc->useHashedNonce)
			chdrs.addString("Hashed-Nonce", dc->mNonceToHash(), 2);
		else
			chdrs.addString("Nonce", sttVoidNonce);

		chdrs.addULong("SessionID", ft->p2p_sessionid);
		chdrs.addLong("SChannelState", 0);
		chdrs.addLong("Capabilities-Flags", 1);
	}

	tResult.addString("Content-Type", "application/x-msnmsgr-transrespbody");

	size_t cbBody = chdrs.getLength() + 1;
	char* szBody = (char*)alloca(cbBody);
	chdrs.writeToBuffer(szBody);

	p2p_getMsgId(ft->std.hContact, -2);
	p2p_sendSlp(ft, tResult, 200, szBody, cbBody);
	p2p_getMsgId(ft->std.hContact, 1);
}


void CMsnProto::p2p_startConnect(HANDLE hContact, const char* szCallID, const char* addr, const char* port)
{
	if (port == NULL) return;

	while (addr != NULL) 
	{
		char* pSpace = (char*)strchr(addr, ' ');
		if (pSpace != NULL) *(pSpace++) = 0;

		ThreadData* newThread = new ThreadData;

		newThread->mType = SERVER_P2P_DIRECT;
		newThread->mInitialContact = hContact;
		mir_snprintf(newThread->mCookie, sizeof(newThread->mCookie), "%s", szCallID);
		mir_snprintf(newThread->mServer, sizeof(newThread->mServer), "%s:%s", addr, port);

		newThread->startThread(&CMsnProto::p2p_fileActiveThread, this);

		addr = pSpace;
	}
}

void CMsnProto::p2p_InitDirectTransfer2(MimeHeaders& tFileInfo, MimeHeaders& tFileInfo2, HANDLE hContact)
{
	const char  *szCallID = tFileInfo["Call-ID"],
				*szInternalAddress = tFileInfo2["IPv4Internal-Addrs"],
				*szInternalPort = tFileInfo2["IPv4Internal-Port"],
				*szExternalAddress = tFileInfo2["IPv4External-Addrs"],
				*szExternalPort = tFileInfo2["IPv4External-Port"],
				*szNonce = tFileInfo2["Nonce"],
				*szHashedNonce = tFileInfo2["Hashed-Nonce"],
				*szListening = tFileInfo2["Listening"];

	if ((szNonce == NULL && szHashedNonce == NULL) || szListening == NULL) 
	{
		MSN_DebugLog("Ignoring invalid invitation: Listening='%s', Nonce=%s", szListening, szNonce);
		return;
	}

	directconnection* dc = p2p_getDCByCallID(szCallID, hContact);
	if (dc == NULL) 
	{
		dc = new directconnection(szCallID, hContact);
		p2p_registerDC(dc);
	}

	dc->useHashedNonce = szHashedNonce != NULL;
	replaceStr(dc->xNonce, szHashedNonce ? szHashedNonce : szNonce);

	if (!strcmp(szListening, "true") && strcmp(dc->xNonce, sttVoidNonce)) 
	{
		p2p_startConnect(hContact, szCallID, szInternalAddress, szInternalPort);
		p2p_startConnect(hContact, szCallID, szExternalAddress, szExternalPort);
	}
}

void CMsnProto::p2p_AcceptTransfer(MimeHeaders& tFileInfo, MimeHeaders& tFileInfo2, HANDLE hContact)
{
	const char *szCallID = tFileInfo["Call-ID"];
	const char* szOldContentType = tFileInfo["Content-Type"];
	const char *szBranch = tFileInfo["Via"];

	if (szBranch != NULL) {
		szBranch = strstr(szBranch, "branch=");
		if (szBranch != NULL)
			szBranch += 7;
	}

	filetransfer ftl(this), *ft = p2p_getSessionByCallID(szCallID, hContact);

	if (!ft || !ft->p2p_sessionid)
	{
		ft = &ftl;
		replaceStr(ft->p2p_branch, szBranch);
		replaceStr(ft->p2p_callID, szCallID);
		ft->std.hContact = hContact;
	}
	else
	{
		if (!(ft->std.flags & PFTS_SENDING)) 
		{
			replaceStr(ft->p2p_branch, szBranch);
			replaceStr(ft->p2p_callID, szCallID);
		}
	}

	if (szCallID == NULL || szBranch == NULL || szOldContentType == NULL) 
	{
		MSN_DebugLog("Ignoring invalid invitation: CallID='%s', szBranch='%s'", szCallID, szBranch);
LBL_Close:
		p2p_sendStatus(ft, 500);
		return;
	}

	MimeHeaders tResult(20);
	tResult.addString("CSeq", "0 ");
	tResult.addString("Call-ID", ft->p2p_callID);
	tResult.addLong("Max-Forwards", 0);

	MimeHeaders chdrs(12);

	if (!strcmp(szOldContentType, "application/x-msnmsgr-sessionreqbody")) 
	{
		p2p_sendFeedStart(ft);

		ThreadData* T = MSN_GetP2PThreadByContact(hContact);
		if (T != NULL && T->mType == SERVER_P2P_DIRECT)
		{
			MSN_StartP2PTransferByContact(hContact);
			return;
		}

		if (usingGateway)
			MSN_StartP2PTransferByContact(hContact);

		if (ft->p2p_type == MSN_APPID_AVATAR || ft->p2p_type == MSN_APPID_AVATAR2)
			return;

		directconnection* dc = new directconnection(szCallID, hContact);
		p2p_registerDC(dc);

		tResult.addString("Content-Type", "application/x-msnmsgr-transreqbody");

		chdrs.addString("Bridges", "TCPv1");
		chdrs.addLong("NetID", MyConnection.extIP);
		chdrs.addString("Conn-Type", MyConnection.GetMyUdpConStr());
		chdrs.addBool("UPnPNat", MyConnection.upnpNAT);
		chdrs.addBool("ICF", MyConnection.icf);
		chdrs.addString("Hashed-Nonce", dc->mNonceToHash(), 2);
	}
	else if (!strcmp(szOldContentType, "application/x-msnmsgr-transrespbody")) 
	{
		const char	*szListening       = tFileInfo2["Listening"],
					*szNonce           = tFileInfo2["Nonce"],
					*szHashedNonce     = tFileInfo2["Hashed-Nonce"],
					*szExternalAddress = tFileInfo2["IPv4External-Addrs"],
					*szExternalPort    = tFileInfo2["IPv4External-Port" ],
					*szInternalAddress = tFileInfo2["IPv4Internal-Addrs"],
					*szInternalPort    = tFileInfo2["IPv4Internal-Port" ];
		if ((szNonce == NULL && szHashedNonce == NULL) || szListening == NULL) 
		{
			MSN_DebugLog("Invalid data packet, exiting...");
			goto LBL_Close;
		}

		directconnection* dc = p2p_getDCByCallID(szCallID, hContact);
		if (dc == NULL) return;

		dc->useHashedNonce = szHashedNonce != NULL;
		replaceStr(dc->xNonce, szHashedNonce ? szHashedNonce : szNonce);

		// another side reported that it will be a server.
		if (!strcmp(szListening, "true") && (szNonce == NULL || strcmp(szNonce, sttVoidNonce))) 
		{
			p2p_startConnect(hContact, szCallID, szInternalAddress, szInternalPort);
			p2p_startConnect(hContact, szCallID, szExternalAddress, szExternalPort);
			return;
		}

		// no, send a file via server
		if (!p2p_createListener(ft, dc, chdrs)) 
		{
			p2p_unregisterDC(dc);
			if (ft != &ftl)
				MSN_StartP2PTransferByContact(hContact);
			else
				p2p_startSessions(hContact);
			return;
		}

		tResult.addString("Content-Type", "application/x-msnmsgr-transrespbody");
	}
	else if (!strcmp(szOldContentType, "application/x-msnmsgr-transreqbody")) 
	{
		const char *szHashedNonce = tFileInfo2["Hashed-Nonce"];
		const char *szNonce       = tFileInfo2["Nonce"];

		directconnection* dc = p2p_getDCByCallID(szCallID, hContact);
		if (dc == NULL) 
		{
			dc = new directconnection(szCallID, hContact);
			p2p_registerDC(dc);
		}

		dc->useHashedNonce = szHashedNonce != NULL;
		replaceStr(dc->xNonce, szHashedNonce ? szHashedNonce : szNonce);

		// no, send a file via server
		if (!p2p_createListener(ft, dc, chdrs)) 
		{
			MSN_StartP2PTransferByContact(hContact);
			return;
		}

		tResult.addString("Content-Type", "application/x-msnmsgr-transrespbody");
	}
	else 
		return;

	size_t cbBody = chdrs.getLength() + 1;
	char* szBody = (char*)alloca(cbBody);
	chdrs.writeToBuffer(szBody);

	p2p_getMsgId(hContact, -2);
	p2p_sendSlp(ft, tResult, -2, szBody, cbBody);
	p2p_getMsgId(hContact, 1);
}


/////////////////////////////////////////////////////////////////////////////////////////
// p2p_processSIP - processes all MSN SIP commands

void CMsnProto::p2p_processSIP(ThreadData* info, char* msgbody, void* hdr, HANDLE hContact)
{
	P2P_Header* hdrdata = (P2P_Header*)hdr;

	int iMsgType = 0;
	if (!memcmp(msgbody, "INVITE MSNMSGR:", 15))
		iMsgType = 1;
	else if (!memcmp(msgbody, "MSNSLP/1.0 200 ", 15))
		iMsgType = 2;
	else if (!memcmp(msgbody, "BYE MSNMSGR:", 12))
		iMsgType = 3;
	else if (!memcmp(msgbody, "MSNSLP/1.0 603 ", 15))
		iMsgType = 4;
	else if (!memcmp(msgbody, "MSNSLP/1.0 481 ", 15))
		iMsgType = 4;
	else if (!memcmp(msgbody, "MSNSLP/1.0 500 ", 15))
		iMsgType = 4;

	char* peol = strstr(msgbody, "\r\n");
	if (peol != NULL)
		msgbody = peol+2;

	MimeHeaders tFileInfo, tFileInfo2;
	msgbody = tFileInfo.readFromBuffer(msgbody);
	msgbody = tFileInfo2.readFromBuffer(msgbody);

	const char* szContentType = tFileInfo["Content-Type"];
	if (szContentType == NULL) 
	{
		MSN_DebugLog("Invalid or missing Content-Type field, exiting");
		return;
	}

	if (hdrdata)
	{
		if (iMsgType == 2 || (iMsgType == 1 && !strcmp(szContentType, "application/x-msnmsgr-transreqbody")))
			p2p_getMsgId(hContact, 1);
		p2p_sendAck(hContact, hdrdata);
	}

	switch(iMsgType) 
	{
	case 1:
		if (!strcmp(szContentType, "application/x-msnmsgr-sessionreqbody"))
			p2p_InitFileTransfer(info, tFileInfo, tFileInfo2, hContact);
		else if (!strcmp(szContentType, "application/x-msnmsgr-transreqbody"))
			p2p_InitDirectTransfer(tFileInfo, tFileInfo2, hContact);
		else if (!strcmp(szContentType, "application/x-msnmsgr-transrespbody"))
			p2p_InitDirectTransfer2(tFileInfo, tFileInfo2, hContact);
		break;

	case 2:
		p2p_AcceptTransfer(tFileInfo, tFileInfo2, hContact);
		break;

	case 3:
		if (!strcmp(szContentType, "application/x-msnmsgr-sessionclosebody"))
		{
			filetransfer* ft = p2p_getSessionByCallID(tFileInfo["Call-ID"], hContact);
			if (ft != NULL)
			{
				if (ft->std.currentFileProgress < ft->std.currentFileSize)
				{
					p2p_sendAbortSession(ft);
					ft->bCanceled = true;
				}
				else
					if (!(ft->std.flags & PFTS_SENDING)) ft->bCompleted = true;

				p2p_sessionComplete(ft);
			}
		}
		break;

	case 4:
		{
			const char* callID = tFileInfo["Call-ID"];
			
  			directconnection *dc = p2p_getDCByCallID(callID, hContact);
			if (dc != NULL)
			{
				p2p_unregisterDC(dc);
				break;
			}

			filetransfer* ft = p2p_getSessionByCallID(callID, hContact);
			if (ft == NULL)
				break;

			ft->close();
			if (!(ft->std.flags & PFTS_SENDING)) _tremove(ft->std.tszCurrentFile);

			p2p_unregisterSession(ft);
		}
		break;
	}
}


/////////////////////////////////////////////////////////////////////////////////////////
// p2p_processMsg - processes all MSN P2P incoming messages

void  CMsnProto::p2p_processMsg(ThreadData* info,  char* msgbody)
{
	P2P_Header* hdrdata = (P2P_Header*)msgbody; msgbody += sizeof(P2P_Header);
	p2p_logHeader(hdrdata);

	//---- if we got a message
	if (LOWORD(hdrdata->mFlags) == 0 && hdrdata->mSessionID == 0)
	{
		if (hdrdata->mPacketLen < hdrdata->mTotalSize)
		{
			char msgid[32];
			mir_snprintf(msgid, sizeof(msgid), "%08p_%08x", info->mJoinedContacts[0], hdrdata->mID);
			int idx = addCachedMsg(msgid, msgbody, (size_t)hdrdata->mOffset, hdrdata->mPacketLen, 
				(size_t)hdrdata->mTotalSize, false);

			char* newbody;
			size_t newsize;
			if (getCachedMsg(idx, newbody, newsize))
			{
				p2p_processSIP(info, newbody, hdrdata, info->mJoinedContacts[0]);
				mir_free(newbody);
			}
			else
			{
				if (hdrdata->mOffset + hdrdata->mPacketLen >= hdrdata->mTotalSize)
					clearCachedMsg(idx);
			}
		}
		else
			p2p_processSIP(info, msgbody, hdrdata, info->mJoinedContacts[0]);

		return;
	}

	filetransfer* ft = p2p_getSessionByID(hdrdata->mSessionID);
	if (ft == NULL)
		ft = p2p_getSessionByUniqueID(hdrdata->mAckUniqueID);

	if (ft == NULL) return;

	ft->ts = time(NULL);

	//---- receiving redirect -----------
	if (hdrdata->mFlags == 0x01) 
	{
		if (WaitForSingleObject(ft->hLockHandle, INFINITE) == WAIT_OBJECT_0) 
		{
			__int64 dp = (__int64)(ft->std.currentFileProgress - hdrdata->mAckDataSize);
			ft->std.totalProgress -= dp ;
			ft->std.currentFileProgress -= dp;
			_lseeki64(ft->fileId, ft->std.currentFileProgress, SEEK_SET);
			ft->tType = info->mType;
			ReleaseMutex(ft->hLockHandle);
		}	
	}

	//---- receiving ack -----------
	if (hdrdata->mFlags == 0x02) 
	{
		ft->p2p_waitack = false;

		if (hdrdata->mAckSessionID == ft->p2p_sendmsgid) 
		{
			if (ft->p2p_appID == MSN_APPID_FILE) 
			{
				ft->bCompleted = true;
				p2p_sendBye(ft);
			}
			return;
		}

		if (hdrdata->mAckSessionID == ft->p2p_byemsgid)
		{
			p2p_sessionComplete(ft);
			return;
		}

		switch(ft->p2p_ackID) 
		{
		case 1000:
			{
				//---- send Data Preparation Message
				P2P_Header tHdr = {0};
				tHdr.mSessionID = ft->p2p_sessionid;
				tHdr.mID = p2p_getMsgId(ft->std.hContact, 1);
				tHdr.mAckSessionID = ft->p2p_acksessid;
				unsigned body = 0;

				p2p_sendMsg(ft->std.hContact, ft->p2p_appID, tHdr, (char*)&body, sizeof(body));

				ft->ts = time(NULL);
				ft->p2p_waitack = true;
			}
			break;

		case 1001:
			//---- send Data Messages
			MSN_StartP2PTransferByContact(ft->std.hContact);
			break;
		}

		ft->p2p_ackID++;
		return;
	}

	if (LOWORD(hdrdata->mFlags) == 0) 
	{
		//---- accept the data preparation message ------
//		const unsigned* pLongs = (unsigned*)msgbody;
		if (hdrdata->mPacketLen == 4 && hdrdata->mTotalSize == 4) 
		{
			p2p_sendAck(ft->std.hContact, hdrdata);
			return;
		}
		else
			hdrdata->mFlags = 0x20;
	}

	//---- receiving data -----------
	if (LOWORD(hdrdata->mFlags) == 0x20 || LOWORD(hdrdata->mFlags) == 0x30) 
	{
		if (hdrdata->mOffset + hdrdata->mPacketLen > hdrdata->mTotalSize)
			hdrdata->mPacketLen = DWORD(hdrdata->mTotalSize - hdrdata->mOffset);

		if (ft->tTypeReq == 0 || ft->tTypeReq == info->mType) 
		{
			ft->tType = info->mType;
			ft->p2p_sendmsgid = hdrdata->mID;
		}

		__int64 dsz = ft->std.currentFileSize - hdrdata->mOffset;
		if (dsz > hdrdata->mPacketLen) dsz = hdrdata->mPacketLen;

		if (ft->tType == info->mType) 
		{
			if (dsz > 0) 
			{
				if (ft->lstFilePtr != hdrdata->mOffset)
					_lseeki64(ft->fileId, hdrdata->mOffset, SEEK_SET);
				_write(ft->fileId, msgbody, (unsigned int)dsz);

				ft->lstFilePtr = hdrdata->mOffset + dsz;

				__int64 dp = ft->lstFilePtr - ft->std.currentFileProgress;
				if (dp > 0) 
				{
					ft->std.totalProgress += dp;
					ft->std.currentFileProgress += dp;

					if (ft->p2p_appID == MSN_APPID_FILE && clock() >= ft->nNotify)
					{
						SendBroadcast(ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, (LPARAM)&ft->std);
						ft->nNotify = clock() + 500;
					}
				}

				//---- send an ack: body was transferred correctly
				MSN_DebugLog("Transferred %I64u bytes out of %I64u", ft->std.currentFileProgress, hdrdata->mTotalSize);
			}

			if (ft->std.currentFileProgress >= hdrdata->mTotalSize) 
			{
				SendBroadcast(ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, (LPARAM)&ft->std);
				p2p_sendAck(ft->std.hContact, hdrdata);
				if (ft->p2p_appID == MSN_APPID_FILE)
				{
					ft->ts = time(NULL);
					ft->p2p_waitack = true;
					ft->complete();
				}
				else 
				{
					p2p_savePicture2disk(ft);
					p2p_sendBye(ft);
				}
			}	
		}	
	}

	if (hdrdata->mFlags == 0x40 || hdrdata->mFlags == 0x80) 
	{
		p2p_sendAbortSession(ft);
		p2p_unregisterSession(ft);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_invite - invite another side to transfer an avatar

void  CMsnProto::p2p_invite(HANDLE hContact, int iAppID, filetransfer* ft)
{
	const char* szAppID;
	switch(iAppID) 
	{
	case MSN_APPID_FILE:			        szAppID = "{5D3E02AB-6190-11D3-BBBB-00C04F795683}";	break;
	case MSN_APPID_AVATAR:			        szAppID = "{A4268EEC-FEC5-49E5-95C3-F126696BDBF6}";	break;
	case MSN_APPID_CUSTOMSMILEY:	        szAppID = "{A4268EEC-FEC5-49E5-95C3-F126696BDBF6}";	break;
	case MSN_APPID_CUSTOMANIMATEDSMILEY:	szAppID = "{A4268EEC-FEC5-49E5-95C3-F126696BDBF6}";	break;
	default: return;
	}

	if (ft == NULL) 
	{
		ft = new filetransfer(this);
		ft->std.hContact = hContact;
	}

	ft->p2p_type = iAppID;
	ft->p2p_acksessid = MSN_GenRandom();
	mir_free(ft->p2p_callID);
	ft->p2p_callID = getNewUuid();

	char*  pContext = NULL;
	size_t cbContext = 0;

	switch (iAppID) 
	{
		case MSN_APPID_FILE:
			{
				cbContext = sizeof(HFileContext);
				pContext = (char*)malloc(cbContext);
				HFileContext* ctx = (HFileContext*)pContext;
				memset(pContext, 0, cbContext);
				ctx->len = (unsigned)cbContext;
				ctx->ver = 3; 
				ctx->type = MSN_TYPEID_FTNOPREVIEW;
				ctx->dwSize = ft->std.currentFileSize;
				ctx->id = 0xffffffff;

				TCHAR* pszFiles = _tcsrchr(ft->std.tszCurrentFile, '\\');
				if (pszFiles)
					pszFiles++;
				else
					pszFiles = ft->std.tszCurrentFile;

				wchar_t *fname = mir_t2u(pszFiles);
				wcsncpy(ctx->wszFileName, fname, MAX_PATH);
				mir_free(fname);

				ft->p2p_appID = MSN_APPID_FILE;
			}
			break;

		default:
			ft->p2p_appID = MSN_APPID_AVATAR2;

			{
				ezxml_t xmlo = ezxml_parse_str(NEWSTR_ALLOCA(ft->p2p_object), strlen(ft->p2p_object));
				ezxml_t xmlr = ezxml_new("msnobj");
				
				const char* p;
				p = ezxml_attr(xmlo, "Creator");
				if (p != NULL)
					ezxml_set_attr(xmlr, "Creator", p);
				p = ezxml_attr(xmlo, "Size");
				if (p != NULL) {
					ezxml_set_attr(xmlr, "Size", p);
					ft->std.totalBytes = ft->std.currentFileSize = _atoi64(p);
				}
				p = ezxml_attr(xmlo, "Type");
				if (p != NULL)
					ezxml_set_attr(xmlr, "Type", p);
				p = ezxml_attr(xmlo, "Location");
				if (p != NULL)
					ezxml_set_attr(xmlr, "Location", p);
				p = ezxml_attr(xmlo, "Friendly");
				if (p != NULL)
					ezxml_set_attr(xmlr, "Friendly", p);
				p = ezxml_attr(xmlo, "SHA1D");
				if (p != NULL)
					ezxml_set_attr(xmlr, "SHA1D", p);
				p = ezxml_attr(xmlo, "SHA1C");
				if (p != NULL)
					ezxml_set_attr(xmlr, "SHA1C", p);

				pContext = ezxml_toxml(xmlr, false);
				cbContext = strlen(pContext)+1;

				ezxml_free(xmlr);
				ezxml_free(xmlo);
			}

			if (ft->fileId == -1 && ft->create() == -1) 
			{
				MSN_DebugLog("Avatar creation failed for MSNCTX=\'%s\'", pContext);
				free(pContext);
				delete ft;
				return;
			}

			break;
	}

	if (!p2p_sessionRegistered(ft)) 
	{
		p2p_registerSession(ft);

		if ((myFlags & 0x4000000) && !MSN_GetThreadByContact(hContact, SERVER_P2P_DIRECT))
		{
			DWORD dwFlags = getDword(hContact, "FlagBits", 0);
			if ((dwFlags & 0x4000000) && (dwFlags >> 28) < 11)
			{
				p2p_inviteDc(ft);
				free(pContext);
				return;
			}
		}
	}

	ft->p2p_sessionid = MSN_GenRandom();

	size_t cbBody = Netlib_GetBase64EncodedBufferSize(cbContext) + 1000;
	char* body = (char*)alloca(cbBody);
	int tBytes = mir_snprintf(body, cbBody,
		"EUF-GUID: %s\r\n"
		"SessionID: %lu\r\n"
		"AppID: %d\r\n"
		"Context: ",
		szAppID, ft->p2p_sessionid, ft->p2p_appID);

	NETLIBBASE64 nlb = { body+tBytes, (int)cbBody - tBytes, (PBYTE)pContext, (int)cbContext };
	MSN_CallService(MS_NETLIB_BASE64ENCODE, 0, LPARAM(&nlb));
	cbBody = tBytes + nlb.cchEncoded;
	strcpy(body + cbBody - 1, "\r\n\r\n");
	cbBody += 4;

	MimeHeaders tResult(20);
	tResult.addString("CSeq", "0 ");
	tResult.addString("Call-ID", ft->p2p_callID);
	tResult.addLong("Max-Forwards", 0);
	tResult.addString("Content-Type", "application/x-msnmsgr-sessionreqbody");

	p2p_sendSlp(ft, tResult, -2, body, cbBody);
	free(pContext);
}


void CMsnProto::p2p_inviteDc(filetransfer* ft)
{
	directconnection* dc = new directconnection(szUbnCall, ft->std.hContact);
	p2p_registerDC(dc);

	MimeHeaders tResult(20);
	tResult.addString("CSeq", "0 ");
	tResult.addString("Call-ID", dc->callId);
	tResult.addLong("Max-Forwards", 0);
	tResult.addString("Content-Type", "application/x-msnmsgr-transreqbody");

	MimeHeaders chdrs(12);

	chdrs.addString("Bridges", "TCPv1 SBBridge");
	chdrs.addLong("NetID", MyConnection.extIP);
	chdrs.addString("Conn-Type", MyConnection.GetMyUdpConStr());
	chdrs.addBool("UPnPNat", MyConnection.upnpNAT);
	chdrs.addBool("ICF", MyConnection.icf);
	chdrs.addString("Hashed-Nonce", dc->mNonceToHash(), 2);
	chdrs.addString("SessionID", "0");
	chdrs.addString("SChannelState", "0");
	chdrs.addString("Capabilities-Flags", "1");

	size_t cbBody = chdrs.getLength() + 1;
	char* szBody = (char*)alloca(cbBody);
	chdrs.writeToBuffer(szBody);

	p2p_sendSlp(ft, tResult, -2, szBody, cbBody);
}
/*
void CMsnProto::p2p_sendSessionAck(filetransfer* ft)
{
	MimeHeaders tResult(8);
	tResult.addString("CSeq", "0 ");
	tResult.addString("Call-ID", "{00000000-0000-0000-0000-000000000000}");
	tResult.addLong("Max-Forwards", 0);
	tResult.addString("Content-Type", "application/x-msnmsgr-transdestaddrupdate");

	MimeHeaders chdrs(8);

	chdrs.addString("IPv4ExternalAddrsAndPorts", mir_strdup(MyConnection.GetMyExtIPStr()), 6);
	chdrs.addString("IPv4InternalAddrsAndPorts", mir_strdup(MyConnection.GetMyExtIPStr()), 6);
	chdrs.addString("SessionID", "0");
	chdrs.addString("SChannelState", "0");
	chdrs.addString("Capabilities-Flags", "1");

	size_t cbBody = chdrs.getLength() + 1;
	char* szBody = (char*)alloca(cbBody);
	chdrs.writeToBuffer(szBody);

	p2p_sendSlp(ft, tResult, -3, szBody, cbBody);
}
*/
void  CMsnProto::p2p_sessionComplete(filetransfer* ft)
{
	if (ft->std.flags & PFTS_SENDING) 
	{
		if (ft->openNext() == -1) 
		{
			bool success = ft->std.currentFileNumber >= ft->std.totalFiles && ft->bCompleted;
			SendBroadcast(ft->std.hContact, ACKTYPE_FILE, success ? ACKRESULT_SUCCESS : ACKRESULT_FAILED, ft, 0);
			p2p_unregisterSession(ft);
		}
		else 
		{
			SendBroadcast(ft->std.hContact, ACKTYPE_FILE, ACKRESULT_NEXTFILE, ft, 0);
			p2p_invite(ft->std.hContact, ft->p2p_appID, ft);
		}
	}
	else 
	{
		SendBroadcast(ft->std.hContact, ACKTYPE_FILE, ft->bCompleted ? ACKRESULT_SUCCESS : ACKRESULT_FAILED, ft, 0);
		p2p_unregisterSession(ft);
	}
}
