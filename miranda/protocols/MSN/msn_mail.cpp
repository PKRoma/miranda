/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2007-2010 Boris Krasnovskiy.

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

static const char oimRecvUrl[] = "https://rsi.hotmail.com/rsi/rsi.asmx";
static const char mailReqHdr[] = 
	"SOAPAction: \"http://www.hotmail.msn.com/ws/2004/09/oim/rsi/%s\"\r\n";

ezxml_t CMsnProto::oimRecvHdr(const char* service, ezxml_t& tbdy, char*& httphdr)
{
	ezxml_t xmlp = ezxml_new("soap:Envelope");
	ezxml_set_attr(xmlp, "xmlns:xsi",  "http://www.w3.org/2001/XMLSchema-instance");
	ezxml_set_attr(xmlp, "xmlns:xsd",  "http://www.w3.org/2001/XMLSchema");
	ezxml_set_attr(xmlp, "xmlns:soap", "http://schemas.xmlsoap.org/soap/envelope/");
	
	ezxml_t hdr = ezxml_add_child(xmlp, "soap:Header", 0);
	ezxml_t cook = ezxml_add_child(hdr, "PassportCookie", 0);
	ezxml_set_attr(cook, "xmlns", "http://www.hotmail.msn.com/ws/2004/09/oim/rsi");
	ezxml_t tcook = ezxml_add_child(cook, "t", 0);
	ezxml_set_txt(tcook, tAuthToken ? tAuthToken : "");
	ezxml_t pcook = ezxml_add_child(cook, "p", 0);
	ezxml_set_txt(pcook, pAuthToken ? pAuthToken : "");

	ezxml_t bdy = ezxml_add_child(xmlp, "soap:Body", 0);
	
	tbdy = ezxml_add_child(bdy, service, 0);
	ezxml_set_attr(tbdy, "xmlns", "http://www.hotmail.msn.com/ws/2004/09/oim/rsi");

	size_t hdrsz = strlen(service) + sizeof(mailReqHdr) + 20;
	httphdr = (char*)mir_alloc(hdrsz);

	mir_snprintf(httphdr, hdrsz, mailReqHdr, service);

	return xmlp;
}


void CMsnProto::getOIMs(ezxml_t xmli)
{
	ezxml_t toki = ezxml_child(xmli, "M");
	if (toki == NULL) return;

	char* getReqHdr;
	ezxml_t reqmsg;
	ezxml_t xmlreq = oimRecvHdr("GetMessage", reqmsg, getReqHdr);

	ezxml_t reqmid = ezxml_add_child(reqmsg, "messageId", 0);
	ezxml_t reqmrk = ezxml_add_child(reqmsg, "alsoMarkAsRead", 0);
	ezxml_set_txt(reqmrk, "false");

	char* delReqHdr;
	ezxml_t delmsg;
	ezxml_t xmldel = oimRecvHdr("DeleteMessages", delmsg, delReqHdr);
	ezxml_t delmids = ezxml_add_child(delmsg, "messageIds", 0);

	while (toki != NULL)
	{
		const char* szId    = ezxml_txt(ezxml_child(toki, "I"));
		const char* szEmail = ezxml_txt(ezxml_child(toki, "E"));

		ezxml_set_txt(reqmid, szId);
		char* szData = ezxml_toxml(xmlreq, true);

		unsigned status;
		char* url = (char*)mir_strdup(oimRecvUrl);

		char* tResult = getSslResult(&url, szData, getReqHdr, status);

		free(szData);
		mir_free(url);

		if (tResult != NULL && status == 200)
		{
			ezxml_t xmlm = ezxml_parse_str(tResult, strlen(tResult));
			ezxml_t body = getSoapResponse(xmlm, "GetMessage");

			MimeHeaders mailInfo;
			const char* mailbody = mailInfo.readFromBuffer((char*)ezxml_txt(body));

			time_t evtm = time(NULL);
			const char* arrTime = mailInfo["X-OriginalArrivalTime"];
			if (arrTime != NULL)
			{
				char szTime[32], *p;
				txtParseParam(arrTime, "FILETIME", "[", "]", szTime, sizeof(szTime));

				unsigned filetimeLo = strtoul(szTime, &p, 16);
				if (*p == ':') 
				{ 
					unsigned __int64 filetime = strtoul(p+1, &p, 16);
					filetime <<= 32;
					filetime |= filetimeLo;
					filetime /= 10000000;
#ifndef __GNUC__
					filetime -= 11644473600ui64;
#else
					filetime -= 11644473600ull;
#endif
					evtm = (time_t)filetime;
				}
			}

			PROTORECVEVENT pre = {0};
			pre.szMessage = mailInfo.decodeMailBody((char*)mailbody);
			pre.flags = PREF_UTF /*+ ((isRtl) ? PREF_RTL : 0)*/;
			pre.timestamp = evtm;

			CCSDATA ccs = {0};
			ccs.hContact = MSN_HContactFromEmail(szEmail, szEmail, false, false);
			ccs.szProtoService = PSR_MESSAGE;
			ccs.lParam = (LPARAM)&pre;
			MSN_CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
			mir_free(pre.szMessage);

			ezxml_t delmid = ezxml_add_child(delmids, "messageId", 0);
			ezxml_set_txt(delmid, szId);
			
			ezxml_free(xmlm);
		}
		mir_free(tResult);
		toki = ezxml_next(toki);
	}
	ezxml_free(xmlreq);
	mir_free(getReqHdr);
	
	if (ezxml_child(delmids, "messageId") != NULL)
	{
		char* szData = ezxml_toxml(xmldel, true);
			
		unsigned status;
		char* url = (char*)mir_strdup(oimRecvUrl);

		char* tResult = getSslResult(&url, szData, delReqHdr, status);

		mir_free(url);
		mir_free(tResult);
		free(szData);
	}
	ezxml_free(xmldel);
	mir_free(delReqHdr);
}


void CMsnProto::getMetaData(void)
{
	char* getReqHdr;
	ezxml_t reqbdy;
	ezxml_t xmlreq = oimRecvHdr("GetMetadata", reqbdy, getReqHdr);

	char* szData = ezxml_toxml(xmlreq, true);
	ezxml_free(xmlreq);

	unsigned status;
	char* url = (char*)mir_strdup(oimRecvUrl);

	char* tResult = getSslResult(&url, szData, getReqHdr, status);

	mir_free(url);
	free(szData);
	mir_free(getReqHdr);

	if (tResult != NULL && status == 200)
	{
		ezxml_t xmlm = ezxml_parse_str(tResult, strlen(tResult));
		ezxml_t xmli = ezxml_get(xmlm, "soap:Body", 0, "GetMetadataResponse", 0, "MD", -1);
		getOIMs(xmli);
		ezxml_free(xmlm);
	}
	mir_free(tResult);
}


void CMsnProto::processMailData(char* mailData)
{
	if (strcmp(mailData, "too-large") == 0)
	{
		getMetaData();
	}
	else
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
}

/////////////////////////////////////////////////////////////////////////////////////////
// Processes e-mail notification

void CMsnProto::sttNotificationMessage(char* msgBody, bool isInitial)
{
	TCHAR tBuffer[512];
	TCHAR tBuffer2[512];
	int  UnreadMessages = mUnreadMessages;
	int  UnreadJunkEmails = mUnreadJunkEmails;
	bool ShowPopUp = isInitial;

	MimeHeaders tFileInfo;
	tFileInfo.readFromBuffer(msgBody);

	const char* From = tFileInfo["From"];
	const char* Subject = tFileInfo["Subject"];
	const char* Fromaddr = tFileInfo["From-Addr"];
	const char* MsgDelta = tFileInfo["Message-Delta"];
	const char* SrcFolder = tFileInfo["Src-Folder"];
	const char* DestFolder = tFileInfo["Dest-Folder"];
	const char* InboxUnread = tFileInfo["Inbox-Unread"];
	const char* FoldersUnread = tFileInfo["Folders-Unread"];
	
	if (InboxUnread != NULL)
		mUnreadMessages = atol(InboxUnread);
	if (FoldersUnread != NULL)
		mUnreadJunkEmails = atol(FoldersUnread);

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

		if (mUnreadJunkEmails < 0) mUnreadJunkEmails = 0;
		if (mUnreadMessages < 0) mUnreadMessages = 0;
	}

	if (From != NULL && Subject != NULL && Fromaddr != NULL) 
	{
		if (DestFolder != NULL && SrcFolder == NULL) 
		{
			mUnreadMessages += strcmp(DestFolder, "ACTIVE") == 0;
			mUnreadJunkEmails += strcmp(DestFolder, "HM_BuLkMail_") == 0;
		}

		wchar_t* mimeFromW = tFileInfo.decode(From);
		wchar_t* mimeSubjectW = tFileInfo.decode(Subject);

#ifdef _UNICODE
		mir_sntprintf(tBuffer2, SIZEOF(tBuffer2), TranslateT("Subject: %s"), mimeSubjectW);
#else
		mir_sntprintf(tBuffer2, SIZEOF(tBuffer2), TranslateT("Subject: %S"), mimeSubjectW);
#endif

#ifdef _UNICODE
		TCHAR* msgtxt = _stricmp(From, Fromaddr) ?
			TranslateT("Hotmail from %s (%S)") : TranslateT("Hotmail from %s");
#else
		TCHAR* msgtxt = _strcmpi(From, Fromaddr) ?
			TranslateT("Hotmail from %S (%s)") : TranslateT("Hotmail from %S");
#endif
		mir_sntprintf(tBuffer, SIZEOF(tBuffer), msgtxt, mimeFromW, Fromaddr);
		mir_free(mimeFromW);
		mir_free(mimeSubjectW);
		ShowPopUp = true;
	}
	else 
	{
		const char* MailData = tFileInfo["Mail-Data"];
		if (MailData != NULL) processMailData((char*)MailData);

		mir_sntprintf(tBuffer, SIZEOF(tBuffer), m_tszUserName);
		mir_sntprintf(tBuffer2, SIZEOF(tBuffer2), TranslateT("Unread mail is available: %d in Inbox and %d in other folders)."),
			mUnreadMessages, mUnreadJunkEmails);
	}

	if (UnreadMessages == mUnreadMessages && UnreadJunkEmails == mUnreadJunkEmails  && !isInitial)
		return;

	ShowPopUp &= mUnreadMessages != 0 || (mUnreadJunkEmails != 0 && !getByte("DisableHotmailJunk", 0));

	HANDLE hContact = MSN_HContactFromEmail(MyOptions.szEmail, NULL, false, false);
	if (hContact)
	{
		CallService(MS_CLIST_REMOVEEVENT, (WPARAM)hContact, (LPARAM) 1);
		displayEmailCount(hContact);

		if (ShowPopUp && !getByte("DisableHotmailTray", 1))
		{
			CLISTEVENT cle = {0};

			cle.cbSize = sizeof(cle);
			cle.hContact = hContact;
			cle.hDbEvent = (HANDLE) 1;
			cle.flags = CLEF_URGENT | CLEF_TCHAR;
			cle.hIcon = LoadSkinnedIcon(SKINICON_OTHER_SENDEMAIL);
			cle.ptszTooltip = tBuffer2;
			char buf[64];
			mir_snprintf(buf, SIZEOF(buf), "%s%s", m_szModuleName, MS_GOTO_INBOX);
			cle.pszService = buf;

			CallService(MS_CLIST_ADDEVENT, (WPARAM)hContact, (LPARAM)&cle);
		}
	}

	SendBroadcast(NULL, ACKTYPE_EMAIL, ACKRESULT_STATUS, NULL, 0);

	// Disable to notify receiving hotmail
	if (ShowPopUp && !getByte("DisableHotmail", 0))
	{
		SkinPlaySound(mailsoundname);

		MSN_ShowPopup(tBuffer, tBuffer2, 
			MSN_ALLOW_ENTER | MSN_ALLOW_MSGBOX | MSN_HOTMAIL_POPUP, 
			tFileInfo["Message-URL"]);
	}

	if (!getByte("RunMailerOnHotmail", 0) || !ShowPopUp || isInitial)
		return;

	char mailerpath[MAX_PATH];
	if (!getStaticString(NULL, "MailerPath", mailerpath, sizeof(mailerpath))) 
	{
		if (mailerpath[0]) 
		{
			char* tParams = NULL;
			char* tCmd = mailerpath;

			if (*tCmd == '\"') 
			{
				++tCmd; 
				char* tEndPtr = strchr(tCmd, '\"');
				if (tEndPtr != NULL) 
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

			MSN_DebugLog("Running mailer \"%s\" with params \"%s\"", tCmd, tParams);
			ShellExecuteA(NULL, "open", tCmd, tParams, NULL, TRUE);
		}
	}	
}

static void TruncUtf8(char *str, size_t sz)
{
	size_t len = strlen(str);
	if (sz > len) sz = len;

	size_t cntl = 0, cnt = 0;
	for (;;)
	{
		unsigned char p = (unsigned char)str[cnt];
		
		if (p >= 0xE0) cnt += 3;
		else if (p >= 0xC0) cnt += 2;
		else if (p != 0) ++cnt;
		else break;
		
		if (cnt <= sz) cntl = cnt;
		else break;
	}
	str[cntl] = 0;
}

int CMsnProto::MSN_SendOIM(const char* szEmail, const char* msg)
{
	char num[32];
	mir_snprintf(num, sizeof(num), "%u", ++oimMsgNum);

	ezxml_t xmlp = ezxml_new("soap:Envelope");
	ezxml_set_attr(xmlp, "xmlns:xsi",  "http://www.w3.org/2001/XMLSchema-instance");
	ezxml_set_attr(xmlp, "xmlns:xsd",  "http://www.w3.org/2001/XMLSchema");
	ezxml_set_attr(xmlp, "xmlns:soap", "http://schemas.xmlsoap.org/soap/envelope/");
	
	ezxml_t hdr = ezxml_add_child(xmlp, "soap:Header", 0);
	ezxml_t from = ezxml_add_child(hdr, "From", 0);
	ezxml_set_attr(from, "memberName", MyOptions.szEmail);

	if (*oimUID == 0)
	{
		char* uid = getNewUuid();
		mir_snprintf(oimUID, sizeof(oimUID), "%s", uid);
		mir_free(uid);
	}

	{
		DBVARIANT dbv;
		char *mynick, *mynickenc;
		if (!getStringUtf("Nick", &dbv))
			mynick = dbv.pszVal;
		else 
		{
			mynick = NEWSTR_ALLOCA(MyOptions.szEmail);
			dbv.pszVal = NULL;
		}
		TruncUtf8(mynick, 48);

		size_t omlen = strlen(mynick);
		size_t emlen = Netlib_GetBase64EncodedBufferSize(omlen);
		mynickenc =(char*)alloca(emlen + 20);
		strcpy(mynickenc, "=?utf-8?B?");
		NETLIBBASE64 nlb = { mynickenc+10, (int)emlen, (PBYTE)mynick, (int)omlen };
		MSN_CallService(MS_NETLIB_BASE64ENCODE, 0, LPARAM(&nlb));
		if (dbv.pszVal != NULL) MSN_FreeVariant(&dbv);
		strcat(mynickenc, "?=");

		ezxml_set_attr(from, "friendlyName", mynickenc);
	}
	
	char lang[5]="en", cntr[5]="US", langcd[15];
	GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, lang, sizeof(lang));
	GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, cntr, sizeof(cntr));
	mir_snprintf(langcd, sizeof(langcd), "%s-%s", lang, cntr);

	ezxml_set_attr(from, "xml:lang", langcd);
	ezxml_set_attr(from, "proxy", "MSNMSGR");
	ezxml_set_attr(from, "xmlns", "http://messenger.msn.com/ws/2004/09/oim/");
	ezxml_set_attr(from, "msnpVer", msnProtID);
	ezxml_set_attr(from, "buildVer", msnProductVer);

	ezxml_t to = ezxml_add_child(hdr, "To", 0);
	ezxml_set_attr(to, "memberName", szEmail);
	ezxml_set_attr(to, "xmlns", "http://messenger.msn.com/ws/2004/09/oim/");

	ezxml_t tick = ezxml_add_child(hdr, "Ticket", 0);
	ezxml_set_attr(tick, "passport", oimSendToken ? oimSendToken : "");
	
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

		char* msgenc = (char*)alloca(mhdrs.getLength() + emlen + 20 + emlen / 38);
		char* msgenc2 = mhdrs.writeToBuffer(msgenc);
		
		strcpy(msgenc2, "\r\n"); msgenc2 += 2;

		NETLIBBASE64 nlb = { msgenc2, (int)emlen, (PBYTE)msg, (int)omlen };
		MSN_CallService(MS_NETLIB_BASE64ENCODE, 0, LPARAM(&nlb));

		char *pos = msgenc2;
		for (int i=0; i<nlb.cchEncoded; i+=76)
		{
			memmove(pos+2, pos, nlb.cchEncoded - i + 1);
			memcpy(pos, "\r\n", 2);
			pos += 78;
		}

		ezxml_set_txt(msgc, msgenc);
	}

	int success = -1;
	bool retry = true;
	for (unsigned i=0; i<3 && retry; ++i)
	{
		retry = false;

		char* szData = ezxml_toxml(xmlp, true);
		
		unsigned status;
		char* url = (char*)mir_strdup("https://ows.messenger.msn.com/OimWS/oim.asmx");

		char* tResult = getSslResult(&url, szData,
			"SOAPAction: \"http://messenger.live.com/ws/2006/09/oim/Store2\"\r\n",
			status);

		mir_free(url);
		free(szData);

		if (tResult != NULL)
		{
			if (status == 500)
			{
				ezxml_t xmlm = ezxml_parse_str(tResult, strlen(tResult));
				ezxml_t flt = getSoapFault(xmlm, false);
				const char* szFltCode = ezxml_txt(ezxml_child(flt, "faultcode"));
				
				if (strcmp(szFltCode, "q0:AuthenticationFailed") == 0)
				{
					ezxml_t det = ezxml_child(flt, "detail");
					const char* szAuthChl = ezxml_txt(ezxml_child(det, "RequiredAuthPolicy"));
					const char* szChl   = ezxml_txt(ezxml_child(det, "LockKeyChallenge"));

					if (*szAuthChl)
					{
						MSN_GetPassportAuth();
						retry = true;
					}
					if (*szChl)
					{
						MSN_MakeDigest(szChl, oimDigest);
						retry = true;
					}
				}
				else if (strcmp(szFltCode, "q0:SenderThrottleLimitExceeded") == 0)
					success = -2;
				else if (strcmp(szFltCode, "q0:SystemUnavailable") == 0)
					success = -3;

				ezxml_free(xmlm);
			}
			else if (status == 200)
				success = 0;

			mir_free(tResult);
		}
	}

	ezxml_free(xmlp);

	return success ? success : oimMsgNum;
}

void CMsnProto::displayEmailCount(HANDLE hContact)
{
	if (!emailEnabled || getByte("DisableHotmailCL", 0)) return;

	TCHAR* name = MSN_GetContactNameT(hContact);
	if (name == NULL) return;

	TCHAR* ch = name-1;
	do
	{
		ch = _tcschr(ch+1, '[');
	}
	while (ch && !_istdigit(ch[1]));  
	if (ch) *ch = 0;
	rtrim(name);

	TCHAR szNick[128];
	mir_sntprintf(szNick, SIZEOF(szNick), 
		getByte("DisableHotmailJunk", 0) ? _T("%s [%d]") : _T("%s [%d][%d]"), name, mUnreadMessages, mUnreadJunkEmails);

	nickChg = true;
	DBWriteContactSettingTString(hContact, "CList", "MyHandle", szNick);
	nickChg = false;
}
