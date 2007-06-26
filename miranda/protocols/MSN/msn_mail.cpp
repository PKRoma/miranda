
#include "msn_global.h"

static const char oimRecvPacket[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>" 
"<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" 
		" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\""
		" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
	"<soap:Header>"
		"<PassportCookie xmlns=\"http://www.hotmail.msn.com/ws/2004/09/oim/rsi\">" 
			"<t>%s</t>"
			"<p>%s</p>" 
		"</PassportCookie>"
	"</soap:Header>"
	"<soap:Body>"
		"<%s xmlns=\"http://www.hotmail.msn.com/ws/2004/09/oim/rsi\">"
		"%s"
		"</%s>"
	"</soap:Body>"
"</soap:Envelope>";

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

static const char oimGetAction[] = "<messageId>%s</messageId> <alsoMarkAsRead>%s</alsoMarkAsRead>";
static const char oimDeleteAction[] = "<messageIds>%s</messageIds>";
static const char oimDeleteActionAtom[] = "<messageId>%s</messageId>"; 

// Global Email counters
int  mUnreadMessages = 0, mUnreadJunkEmails = 0;


void getOIMs(ezxml_t xmli)
{
	ezxml_t tokm = ezxml_get(xmli, "M", -1);
	if (tokm == NULL) return;

	SSLAgent mAgent;

	char* szDelAct = ( char* )mir_calloc( 1 );
	size_t lenDelAct = 1;

	while (tokm != NULL)
	{
		char szData1[1024], szData[2048];

		char* szId    = ezxml_txt(ezxml_get(tokm, "I", -1));
		char* szEmail = ezxml_txt(ezxml_get(tokm, "E", -1));

		mir_snprintf( szData1, sizeof( szData1 ), oimGetAction, szId, "false");
		mir_snprintf( szData, sizeof( szData ), oimRecvPacket, tAuthToken, pAuthToken, 
			"GetMessage", szData1, "GetMessage" );

		char* tResult = mAgent.getSslResult( "https://rsi.hotmail.com/rsi/rsi.asmx", szData,
			"SOAPAction: \"http://www.hotmail.msn.com/ws/2004/09/oim/rsi/GetMessage\"\r\n" );

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

			PROTORECVEVENT pre;
			pre.szMessage = szMsg;
			pre.flags = PREF_UTF /*+ (( isRtl ) ? PREF_RTL : 0)*/;
			pre.timestamp = evtm;
			pre.lParam = 0;

			CCSDATA ccs;
			ccs.hContact = MSN_HContactFromEmail( szEmail, szEmail, 0, 0 );
			ccs.szProtoService = PSR_MESSAGE;
			ccs.wParam = 0;
			ccs.lParam = ( LPARAM )&pre;
			MSN_CallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );

			lenDelAct += mir_snprintf( szData1, sizeof( szData1 ), oimDeleteActionAtom, szId);
			szDelAct = ( char* )mir_realloc( szDelAct, lenDelAct );
			strcat( szDelAct, szData1 );

			mir_free( szMsg );
			mir_free( tResult );
		}
		tokm = ezxml_next(tokm);
	}
	
	if ( lenDelAct > 1 )
	{
		lenDelAct += sizeof( oimDeleteAction );
		char* szDelActAll = ( char* )alloca ( lenDelAct );
		mir_snprintf( szDelActAll, lenDelAct, oimDeleteAction, szDelAct);
		
		lenDelAct += sizeof( oimRecvPacket ) + 28 + strlen( tAuthToken ) + strlen( pAuthToken );
		char* szDelPack = ( char* )alloca ( lenDelAct );
		mir_snprintf( szDelPack, lenDelAct, oimRecvPacket, tAuthToken, pAuthToken, "DeleteMessages", szDelActAll, "DeleteMessages" );
			
		char* tResult = mAgent.getSslResult( "https://rsi.hotmail.com/rsi/rsi.asmx", szDelPack,
			"SOAPAction: \"http://www.hotmail.msn.com/ws/2004/09/oim/rsi/DeleteMessages\"\r\n" );
		mir_free( tResult );
	}
	mir_free( szDelAct );
}

void processMailData(char* mailData)
{
	ezxml_t xmli = ezxml_parse_str(mailData, strlen(mailData));

	ezxml_t toke = ezxml_get(xmli, "E", -1);

	const char* szIU = ezxml_txt(ezxml_get(toke, "IU", -1));
	if (szIU != NULL) mUnreadMessages = atol(szIU);

	const char* szOU = ezxml_txt(ezxml_get(toke, "OU", -1));
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
	if ( !MSN_GetByte( "DisableHotmail", 1 ) && ShowPopUp) 
	{
		if ( mUnreadMessages != 0 || !MSN_GetByte( "DisableHotmailJunk", 0 )) 
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
