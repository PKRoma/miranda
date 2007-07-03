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

/*
static const char oimSendPacket[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
	"<soap:Header>"
		"<From memberName=\"%s\" friendlyName=\"=?utf-8?B?%s?=\" xml:lang=\"nl-nl\" proxy=\"MSNMSGR\" xmlns=\"http://messenger.msn.com/ws/2004/09/oim/\" msnpVer=\"MSNP13\" buildVer=\"8.0.0328\"/>"
		"<To memberName=\"%s\" xmlns=\"http://messenger.msn.com/ws/2004/09/oim/\"/>"
		"<Ticket passport=\"%s\" appid=\"PROD01065C%ZFN6F\" lockkey=\"%s\" xmlns=\"http://messenger.msn.com/ws/2004/09/oim/\"/>"
		"<Sequence xmlns=\"http://schemas.xmlsoap.org/ws/2003/03/rm\">"
			"<Identifier xmlns=\"http://schemas.xmlsoap.org/ws/2002/07/utility\">http://messenger.msn.com</Identifier&gt;"
			"<MessageNumber>%d</MessageNumber>"
		"</Sequence>"
	"</soap:Header>"
	"<soap:Body>"
		"<MessageType xmlns=\"http://messenger.msn.com/ws/2004/09/oim/\">text</MessageType>"
		"<Content xmlns=\"http://messenger.msn.com/ws/2004/09/oim/\">"
			"MIME-Version: 1.0\r\n"
			"Content-Type: text/plain; charset=UTF-8\r\n"
			"Content-Transfer-Encoding: base64\r\n"
			"X-OIM-Message-Type: OfflineMessage\r\n"
			"X-OIM-Run-Id: {3A3BE82C-684D-4F4F-8005-CBE8D4F82BAD}\r\n"
			"X-OIM-Sequence-Num: %d\r\n"
			"\r\n%s"
		"</Content>"
	"</soap:Body>"
"</soap:Envelope>";
*/

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
			mir_free( tResult );

			ezxml_free(tokm);
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
	if (szIU != NULL) mUnreadMessages = atol(szIU);

	const char* szOU = ezxml_txt(ezxml_child(toke, "OU"));
	if (szOU != NULL) mUnreadJunkEmails = atol(szOU);

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
		if (DestFolder && strcmp(DestFolder, "ACTIVE") == 0)
			mUnreadMessages += iDelta;
		if (SrcFolder && strcmp(SrcFolder, "HM_BuLkMail_") == 0)
			mUnreadJunkEmails -= iDelta;
		if (DestFolder && strcmp(DestFolder, "HM_BuLkMail_") == 0)
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
