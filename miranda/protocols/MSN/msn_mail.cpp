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

// Global Email counters
int  mUnreadMessages = 0, mUnreadJunkEmails = 0;

ezxml_t oimRecvHdr(void)
{
	ezxml_t xmlp = ezxml_new("soap:Envelope");
	ezxml_set_attr(xmlp, "xmlns:xsi",  "http://www.w3.org/2001/XMLSchema-instance");
	ezxml_set_attr(xmlp, "xmlns:xsd",  "http://www.w3.org/2001/XMLSchema");
	ezxml_set_attr(xmlp, "xmlns:soap", "http://schemas.xmlsoap.org/soap/envelope/");
	
	ezxml_t hdr = ezxml_add_child(xmlp, "soap:Header", 0);
	ezxml_t cook = ezxml_add_child(hdr, "PassportCookie", 0);
	ezxml_set_attr(cook, "xmlns", "http://www.hotmail.msn.com/ws/2004/09/oim/rsi");
	ezxml_t tcook = ezxml_add_child(cook, "t", 0);
	ezxml_set_txt(tcook, tAuthToken);
	ezxml_t pcook = ezxml_add_child(cook, "p", 0);
	ezxml_set_txt(pcook, pAuthToken);

	ezxml_t bdy = ezxml_add_child(xmlp, "soap:Body", 0);

	return xmlp;
}

void getOIMs(ezxml_t xmli)
{
	ezxml_t toki = ezxml_child(xmli, "M");
	if (toki == NULL) return;

	SSLAgent mAgent;

	ezxml_t xmlreq = oimRecvHdr();
	ezxml_t reqbdy = ezxml_child(xmlreq, "soap:Body");
	ezxml_t reqmsg = ezxml_add_child(reqbdy, "GetMessage", 0);
	ezxml_set_attr(reqmsg, "xmlns", "http://www.hotmail.msn.com/ws/2004/09/oim/rsi");
	ezxml_t reqmid = ezxml_add_child(reqmsg, "messageId", 0);
	ezxml_t reqmrk = ezxml_add_child(reqmsg, "alsoMarkAsRead", 0);
	ezxml_set_txt(reqmrk, "false");

	ezxml_t xmldel = oimRecvHdr();
	ezxml_t delbdy = ezxml_child(xmldel, "soap:Body");
	ezxml_t delmsg = ezxml_add_child(delbdy, "DeleteMessages", 0);
	ezxml_set_attr(delmsg, "xmlns", "http://www.hotmail.msn.com/ws/2004/09/oim/rsi");
	ezxml_t delmids = ezxml_add_child(delmsg, "messageIds", 0);

	while (toki != NULL)
	{
		char* szId    = ezxml_txt(ezxml_child(toki, "I"));
		char* szEmail = ezxml_txt(ezxml_child(toki, "E"));

		ezxml_set_txt(reqmid, szId);
		char* szData = ezxml_toxml(xmlreq, true);

		char* tResult = mAgent.getSslResult( "https://rsi.hotmail.com/rsi/rsi.asmx", szData,
			"SOAPAction: \"http://www.hotmail.msn.com/ws/2004/09/oim/rsi/GetMessage\"\r\n" );

		free(szData);

		if (tResult != NULL)
		{
			unsigned status;
			char* htmlhdr = httpParseHeader( tResult, status );
			if (status == 200)
			{
				MimeHeaders httpInfo;
				char* htmlbody = (char*)httpInfo.readFromBuffer( htmlhdr );

				ezxml_t xmlm = ezxml_parse_str(htmlbody, strlen(htmlbody));
				ezxml_t tokm = ezxml_get(xmlm, "soap:Body", 0, "GetMessageResponse", 0, "GetMessageResult", -1);
				
				MimeHeaders mailInfo;
				char* mailbody = (char*)mailInfo.readFromBuffer(ezxml_txt(tokm));

				time_t evtm = time( NULL );
				const char* arrTime = mailInfo["X-OriginalArrivalTime"];
				if (arrTime != NULL)
				{
					char szTime[32], *p;
					txtParseParam( arrTime, "FILETIME", "[", "]", szTime, sizeof( szTime ));

					unsigned filetimeLo = strtoul( szTime, &p, 16 );
					if ( *p == ':' ) { 
						unsigned __int64 filetime = strtoul( p+1, &p, 16 );
						filetime <<= 32;
						filetime |= filetimeLo;
						filetime /= 10000000;
						filetime -= 11644473600ui64;
						evtm = ( time_t )filetime;
					}
				}

				char* szMsg = mailInfo.decodeMailBody( mailbody );

				PROTORECVEVENT pre = {0};
				pre.szMessage = szMsg;
				pre.flags = PREF_UTF /*+ (( isRtl ) ? PREF_RTL : 0)*/;
				pre.timestamp = evtm;

				CCSDATA ccs = {0};
				ccs.hContact = MSN_HContactFromEmail( szEmail, szEmail, 0, 0 );
				ccs.szProtoService = PSR_MESSAGE;
				ccs.lParam = ( LPARAM )&pre;
				MSN_CallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );

				ezxml_t delmid = ezxml_add_child(delmids, "messageId", 0);
				ezxml_set_txt(delmid, szId);
				
				mir_free( szMsg );
				ezxml_free(tokm);
			}
			mir_free( tResult );

		}
		toki = ezxml_next(toki);
	}
	ezxml_free(xmlreq);
	
	if (ezxml_child(delmids, "messageId") != NULL)
	{
		char* szData = ezxml_toxml(xmldel, true);
			
		char* tResult = mAgent.getSslResult( "https://rsi.hotmail.com/rsi/rsi.asmx", szData,
			"SOAPAction: \"http://www.hotmail.msn.com/ws/2004/09/oim/rsi/DeleteMessages\"\r\n" );
		mir_free( tResult );
		free(szData);
	}
	ezxml_free(xmldel);
}

void processMailData(char* mailData)
{
	ezxml_t xmli = ezxml_parse_str(mailData, strlen(mailData));

	ezxml_t toke = ezxml_child(xmli, "E");

	const char* szIU = ezxml_txt(ezxml_child(toke, "IU"));
	if (*szIU) mUnreadMessages = atol(szIU);

	const char* szOU = ezxml_txt(ezxml_child(toke, "OU"));
	if (*szOU) mUnreadJunkEmails = atol(szOU);

	getOIMs(xmli);

	ezxml_free(xmli);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Processes e-mail notification

void sttNotificationMessage( char* msgBody, bool isInitial )
{
	TCHAR tBuffer[512];
	TCHAR tBuffer2[512];
	int  UnreadMessages = mUnreadMessages;
	int  UnreadJunkEmails = mUnreadJunkEmails;
	bool ShowPopUp = isInitial;

	MimeHeaders tFileInfo;
	tFileInfo.readFromBuffer( msgBody );

	const char* From = tFileInfo[ "From" ];
	const char* Subject = tFileInfo[ "Subject" ];
	const char* Fromaddr = tFileInfo[ "From-Addr" ];
	const char* MsgDelta = tFileInfo[ "Message-Delta" ];
	const char* SrcFolder = tFileInfo[ "Src-Folder" ];
	const char* DestFolder = tFileInfo[ "Dest-Folder" ];
	const char* InboxUnread = tFileInfo[ "Inbox-Unread" ];
	const char* FoldersUnread = tFileInfo[ "Folders-Unread" ];
	
	if ( InboxUnread != NULL )
		mUnreadMessages = atol( InboxUnread );
	if ( FoldersUnread != NULL )
		mUnreadJunkEmails = atol( FoldersUnread );

	if (MsgDelta != NULL)
	{
		int iDelta = atol(MsgDelta);
		if (SrcFolder && strcmp(SrcFolder, "ACTIVE") == 0)
			mUnreadMessages -= iDelta;
		else if (DestFolder && strcmp(DestFolder, "ACTIVE") == 0)
			mUnreadMessages += iDelta;
		if (SrcFolder && strcmp(SrcFolder, "HM_BuLkMail_") == 0)
			mUnreadJunkEmails -= iDelta;
		else if (DestFolder && strcmp(DestFolder, "HM_BuLkMail_") == 0)
			mUnreadJunkEmails += iDelta;
	}

	if ( From != NULL && Subject != NULL && Fromaddr != NULL ) 
	{
		if ( DestFolder != NULL && SrcFolder == NULL ) 
		{
			mUnreadMessages += strcmp( DestFolder, "ACTIVE" ) == 0;
			mUnreadJunkEmails += strcmp( DestFolder, "HM_BuLkMail_" ) == 0;
		}

		wchar_t* mimeFromW = tFileInfo.decode(From);
		wchar_t* mimeSubjectW = tFileInfo.decode(Subject);

#ifdef _UNICODE
		mir_sntprintf( tBuffer2, SIZEOF( tBuffer2 ), TranslateT("Subject: %s"), mimeSubjectW );
#else
		mir_sntprintf( tBuffer2, SIZEOF( tBuffer2 ), TranslateT("Subject: %S"), mimeSubjectW );
#endif

#ifdef _UNICODE
		TCHAR* msgtxt = _stricmp( From, Fromaddr ) ?
			TranslateT("Hotmail from %s (%S)") : TranslateT( "Hotmail from %s" );
		mir_sntprintf( tBuffer, SIZEOF( tBuffer ), msgtxt, mimeFromW, Fromaddr );
#else
		TCHAR* msgtxt = strcmpi( From, Fromaddr ) ?
			TranslateT("Hotmail from %S (%s)") : TranslateT( "Hotmail from %S" );
		mir_sntprintf( tBuffer, SIZEOF( tBuffer ), msgtxt, mimeFromW, Fromaddr );
#endif
		mir_free(mimeFromW);
		mir_free(mimeSubjectW);
		ShowPopUp = true;
	}
	else 
	{
		const char* MailData = tFileInfo[ "Mail-Data" ];
		if ( MailData != NULL ) processMailData((char*)MailData);

		mir_sntprintf( tBuffer, SIZEOF( tBuffer ), TranslateT( "Hotmail" ));
		mir_sntprintf( tBuffer2, SIZEOF( tBuffer2 ), TranslateT( "Unread mail is available: %d messages (%d junk e-mails)." ),
			mUnreadMessages, mUnreadJunkEmails );
	}

	if (UnreadMessages == mUnreadMessages && UnreadJunkEmails == mUnreadJunkEmails  && !isInitial)
		return;

	MSN_SendBroadcast( NULL, ACKTYPE_EMAIL, ACKRESULT_STATUS, NULL, 0 );

	// Disable to notify receiving hotmail
	if ( !MSN_GetByte( "DisableHotmail", 1 ) && ShowPopUp && 
		(mUnreadMessages != 0 || 
		(mUnreadJunkEmails != 0 && !MSN_GetByte( "DisableHotmailJunk", 0 ))))
	{
		SkinPlaySound( mailsoundname );
		MSN_ShowPopup( tBuffer, tBuffer2, 
			MSN_ALLOW_ENTER | MSN_ALLOW_MSGBOX | MSN_HOTMAIL_POPUP, 
			tFileInfo[ "Message-URL" ]);
	}

	if ( !MSN_GetByte( "RunMailerOnHotmail", 0 ) || !ShowPopUp || isInitial )
		return;

	char mailerpath[MAX_PATH];
	if ( !MSN_GetStaticString( "MailerPath", NULL, mailerpath, sizeof( mailerpath ))) 
	{
		if ( mailerpath[0] ) 
		{
			char* tParams = NULL;
			char* tCmd = mailerpath;

			if ( *tCmd == '\"' ) 
			{
				++tCmd; 
				char* tEndPtr = strchr( tCmd, '\"' );
				if ( tEndPtr != NULL ) 
				{
					*tEndPtr = 0;
					tParams = tEndPtr+1;
				}	
			}

			if (tParams == NULL)
			{
				tParams = strchr(tCmd, ' ');
				tParams = tParams ? tParams + 1 : strchr(tCmd, '\0');
			}

			while (*tParams == ' ') ++tParams;

			MSN_DebugLog( "Running mailer \"%s\" with params \"%s\"", tCmd, tParams );
			ShellExecuteA( NULL, "open", tCmd, tParams, NULL, TRUE );
		}
	}	
}

static char oimDigest[64] = "";
static unsigned oimMsgNum = 0;
static char oimUID[64] = "";

int MSN_SendOIM(char* szEmail, char* msg)
{
	char szAuth[530];
	char num[32];
	mir_snprintf(num, sizeof(num), "%u", ++oimMsgNum);

	ezxml_t xmlp = ezxml_new("soap:Envelope");
	ezxml_set_attr(xmlp, "xmlns:xsi",  "http://www.w3.org/2001/XMLSchema-instance");
	ezxml_set_attr(xmlp, "xmlns:xsd",  "http://www.w3.org/2001/XMLSchema");
	ezxml_set_attr(xmlp, "xmlns:soap", "http://schemas.xmlsoap.org/soap/envelope/");
	
	ezxml_t hdr = ezxml_add_child(xmlp, "soap:Header", 0);
	ezxml_t from = ezxml_add_child(hdr, "From", 0);
	ezxml_set_attr(from, "memberName", MyOptions.szEmail);

	{
		DBVARIANT dbv;
		char *mynick, *mynickenc;
		if (!DBGetContactSettingStringUtf( NULL, msnProtocolName, "Nick", &dbv ))
			mynick = dbv.pszVal;
		else 
		{
			mynick = MyOptions.szEmail;
			dbv.pszVal = NULL;
		}

		size_t omlen = strlen(mynick);
		size_t emlen = Netlib_GetBase64EncodedBufferSize(omlen);
		mynickenc =(char*)alloca(emlen + 20);
		strcpy(mynickenc, "=?utf-8?B?");
		NETLIBBASE64 nlb = { mynickenc+10, emlen, (PBYTE)mynick, omlen };
		MSN_CallService( MS_NETLIB_BASE64ENCODE, 0, LPARAM( &nlb ));
		if (dbv.pszVal != NULL) MSN_FreeVariant( &dbv );
		strcat(mynickenc, "?=");

		ezxml_set_attr(from, "friendlyName", mynickenc);
	}
	
	ezxml_set_attr(from, "xml:lang", "en-US");
	ezxml_set_attr(from, "proxy", "MSNMSGR");
	ezxml_set_attr(from, "xmlns", "http://messenger.msn.com/ws/2004/09/oim/");
	ezxml_set_attr(from, "msnpVer", "MSNP13");
	ezxml_set_attr(from, "buildVer", "8.0.0812");

	ezxml_t to = ezxml_add_child(hdr, "To", 0);
	ezxml_set_attr(to, "memberName", szEmail);
	ezxml_set_attr(to, "xmlns", "http://messenger.msn.com/ws/2004/09/oim/");

	ezxml_t tick = ezxml_add_child(hdr, "Ticket", 0);
	ezxml_set_attr(tick, "passport", szAuth);
	
	ezxml_set_attr(tick, "appid", msnProductID);
	ezxml_set_attr(tick, "lockkey", oimDigest);
	ezxml_set_attr(tick, "xmlns", "http://messenger.msn.com/ws/2004/09/oim/");

	ezxml_t seq = ezxml_add_child(hdr, "Sequence", 0);
	ezxml_set_attr(seq, "xmlns", "http://schemas.xmlsoap.org/ws/2003/03/rm");

	ezxml_t ident = ezxml_add_child(seq, "Identifier", 0);
	ezxml_set_attr(ident, "xmlns", "http://schemas.xmlsoap.org/ws/2002/07/utility");
	ezxml_set_txt(ident, "http://messenger.msn.com");
	ezxml_t msgn = ezxml_add_child(seq, "MessageNumber", 0);
	ezxml_set_txt(msgn, num);

	ezxml_t bdy = ezxml_add_child(xmlp, "soap:Body", 0);
	ezxml_t msgt = ezxml_add_child(bdy, "MessageType", 0);
	ezxml_set_attr(msgt, "xmlns", "http://messenger.msn.com/ws/2004/09/oim/");
	ezxml_set_txt(msgt, "text");
	ezxml_t msgc = ezxml_add_child(bdy, "Content", 0);
	ezxml_set_attr(msgc, "xmlns", "http://messenger.msn.com/ws/2004/09/oim/");

	{
		MimeHeaders mhdrs(7);
		mhdrs.addString("MIME-Version", "1.0");
		mhdrs.addString("Content-Type", "text/plain; charset=UTF-8");
		mhdrs.addString("Content-Transfer-Encoding", "base64");
		mhdrs.addString("X-OIM-Message-Type", "OfflineMessage");
		mhdrs.addString("X-OIM-Run-Id", oimUID);
		mhdrs.addString("X-OIM-Sequence-Num", num);

		size_t omlen = strlen(msg);
		size_t emlen = Netlib_GetBase64EncodedBufferSize(omlen);

		char* msgenc = (char*)alloca(mhdrs.getLength() + emlen + 20);
		char* msgenc2 = mhdrs.writeToBuffer(msgenc);
		
		strcpy(msgenc2, "\r\n"); msgenc2 += 2;

		NETLIBBASE64 nlb = { msgenc2, emlen, (PBYTE)msg, omlen };
		MSN_CallService( MS_NETLIB_BASE64ENCODE, 0, LPARAM( &nlb ));

		ezxml_set_txt(msgc, msgenc);
	}

	SSLAgent mAgent;

	bool success = false;
	bool retry = true;
	for (unsigned i=0; i<2 && retry; ++i)
	{
		retry = false;

		mir_snprintf(szAuth, sizeof(szAuth), "t=%s&p=%s", tAuthToken, pAuthToken);
		char* szData = ezxml_toxml(xmlp, true);
		
		char* tResult = mAgent.getSslResult( "https://ows.messenger.msn.com/OimWS/oim.asmx", szData,
			"SOAPAction: \"http://messenger.msn.com/ws/2004/09/oim/Store\"\r\n" );

		free(szData);

		if (tResult != NULL)
		{
			unsigned status;
			char* htmlhdr = httpParseHeader( tResult, status );

			if (status == 500)
			{
				MimeHeaders httpInfo;
				char* htmlbody = (char*)httpInfo.readFromBuffer( htmlhdr );

				ezxml_t xmlm = ezxml_parse_str(htmlbody, strlen(htmlbody));
				ezxml_t det = ezxml_get(xmlm, "soap:Body", 0, "soap:Fault", 0, "detail", -1);
				char* szTwChl = ezxml_txt(ezxml_child(det, "TweenerChallenge"));
				char* szChl   = ezxml_txt(ezxml_child(det, "LockKeyChallenge"));

				if (*szTwChl) 
					MSN_GetPassportAuth(szTwChl);
				if (*szChl)
				{
					MSN_MakeDigest(szChl, oimDigest);
					retry = true;
				}
				ezxml_free(xmlm);
			}
			else if (status == 200)
				success = true;

			mir_free(tResult);
		}
	}

	ezxml_free(xmlp);

	return success ? oimMsgNum : -1;
}
