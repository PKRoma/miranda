/*

Jabber Protocol Plugin for Miranda IM
Copyright (C) 2002-04  Santithorn Bunchua
Copyright (C) 2005     George Hazan

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

#include "jabber.h"
#include "jabber_ssl.h"
#include "jabber_list.h"
#include "sha1.h"

extern CRITICAL_SECTION mutex;
extern UINT jabberCodePage;

static CRITICAL_SECTION serialMutex;
static unsigned int serial;

void JabberSerialInit(void)
{
	InitializeCriticalSection(&serialMutex);
	serial = 0;
}

void JabberSerialUninit(void)
{
	DeleteCriticalSection(&serialMutex);
}

unsigned int JabberSerialNext(void)
{
	unsigned int ret;

	EnterCriticalSection(&serialMutex);
	ret = serial;
	serial++;
	LeaveCriticalSection(&serialMutex);
	return ret;
}

#ifdef _DEBUG
void JabberLog(const char *fmt, ...)
{
	char *str;
	va_list vararg;
	int strsize;
	char *text;
	char *p, *q;
	int extra;

	va_start(vararg, fmt);
	str = (char *) malloc(strsize=2048);
	while (_vsnprintf(str, strsize, fmt, vararg) == -1)
		str = (char *) realloc(str, strsize+=2048);
	va_end(vararg);

	extra = 0;
	for (p=str; *p!='\0'; p++)
		if (*p=='\n' || *p=='\r')
			extra++;
	text = (char *) malloc(strlen(jabberProtoName)+2+strlen(str)+2+extra);
	wsprintf(text, "[%s]", jabberProtoName);
	for (p=str,q=text+strlen(text); *p!='\0'; p++,q++) {
		if (*p == '\r') {
			*q = '\\';
			*(q+1) = 'r';
			q++;
		}
		else if (*p == '\n') {
			*q = '\\';
			*(q+1) = 'n';
			q++;
		}
		else
			*q = *p;
	}
	*q = '\n';
	*(q+1) = '\0';
	OutputDebugString(text);
	free(text);
	free(str);
}
#endif

// Caution: DO NOT use JabberSend() to send binary (non-string) data
int JabberSend(HANDLE hConn, const char *fmt, ...)
{
	char *str;
	int size;
	va_list vararg;
	int result;
	PVOID ssl;
	char *szLogBuffer;

	EnterCriticalSection(&mutex);

	va_start(vararg,fmt);
	size = 512;
	str = (char *) malloc(size);
	while (_vsnprintf(str, size, fmt, vararg) == -1) {
		size += 512;
		str = (char *) realloc(str, size);
	}
	va_end(vararg);

	JabberLog("SEND:%s", str);
	size = strlen(str);
	if ((ssl=JabberSslHandleToSsl(hConn)) != NULL) {
		if (DBGetContactSettingByte(NULL, "Netlib", "DumpSent", TRUE) == TRUE) {
			if ((szLogBuffer=(char *)malloc(size+32)) != NULL) {
				strcpy(szLogBuffer, "(SSL) Data sent\n");
				memcpy(szLogBuffer+strlen(szLogBuffer), str, size+1 /* also copy \0 */);
				Netlib_Logf(hNetlibUser, "%s", szLogBuffer);	// %s to protect against when fmt tokens are in szLogBuffer causing crash
				free(szLogBuffer);
			}
		}
		result = pfn_SSL_write(ssl, str, size);
	}
	else
		result = JabberWsSend(hConn, str, size);
	LeaveCriticalSection(&mutex);

	free(str);
	return result;
}

HANDLE JabberHContactFromJID(const char *jid)
{
	HANDLE hContact, hContactMatched;
	DBVARIANT dbv;
	char *szProto;
	char *s, *p, *q;
	int len;
	char *s2;

	if (jid == NULL) return (HANDLE) NULL;

	s = _strdup(jid); _strlwr(s);
	// Strip resource name if any
	if ((p=strchr(s, '@')) != NULL) {
		if ((q=strchr(p, '/')) != NULL)
			*q = '\0';
	}
	len = strlen(s);

	hContactMatched = NULL;
	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL) {
		szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if (szProto!=NULL && !strcmp(jabberProtoName, szProto)) {
			if (!DBGetContactSetting(hContact, jabberProtoName, "jid", &dbv)) {
				if ((p=dbv.pszVal) != NULL) {
					if (!stricmp(p, jid)) {	// exact match (node@domain/resource)
						hContactMatched = hContact;
						DBFreeVariant(&dbv);
						break;
					}
					// match only node@domain part
					if ((int)strlen(p)>=len && (p[len]=='\0'||p[len]=='/') && !strncmp(p, s, len)) {
						hContactMatched = hContact;
					}
				}
				DBFreeVariant(&dbv);
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}

	if (hContactMatched != NULL) {
		free(s);
		return hContactMatched;
	}

	// The following is for the transition to storing JID and resource in UTF8 format.
	// If we can't find the particular JID, we ut8decode the JID and try again below.
	// If found, we update the JID using the utf8 format.
	s2 = JabberTextDecode(s);
	len = strlen(s2);
	hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL) {
		szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if (szProto!=NULL && !strcmp(jabberProtoName, szProto)) {
			if (!DBGetContactSetting(hContact, jabberProtoName, "jid", &dbv)) {
				p = dbv.pszVal;
				if (p && (int)strlen(p)>=len && (p[len]=='\0'||p[len]=='/') && !strncmp(p, s2, len)) {
					DBFreeVariant(&dbv);
					// Update with the utf8 format
					DBWriteContactSettingString(hContact, jabberProtoName, "jid", s);
					free(s);
					free(s2);
					return hContact;
				}
				DBFreeVariant(&dbv);
			}
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}
	free(s2);
	free(s);
	return NULL;
}

char *JabberNickFromJID(const char *jid)
{
	char *p;
	char *nick;

	if ((p=strchr(jid, '@')) == NULL)
		p = strchr(jid, '/');
	if (p != NULL) {
		if ((nick=(char *) malloc((p-jid)+1)) != NULL) {
			strncpy(nick, jid, p-jid);
			nick[p-jid] = '\0';
		}
	}
	else {
		nick = _strdup(jid);
	}

	return nick;
}

char *JabberLocalNickFromJID(const char *jid)
{
	char *p;
	char *localNick;

	p = JabberNickFromJID(jid);
	localNick = JabberTextDecode(p);
	free(p);
	return localNick;
}

void JabberUrlDecode(char *str)
{
	char *p, *q;

	if (str == NULL)
		return;

	for (p=q=str; *p!='\0'; p++,q++) {
		if (*p == '&') {
			if (!strncmp(p, "&amp;", 5)) {	*q = '&'; p += 4; }
			else if (!strncmp(p, "&apos;", 6)) { *q = '\''; p += 5; }
			else if (!strncmp(p, "&gt;", 4)) { *q = '>'; p += 3; }
			else if (!strncmp(p, "&lt;", 4)) { *q = '<'; p += 3; }
			else if (!strncmp(p, "&quot;", 6)) { *q = '"'; p += 5; }
			else { *q = *p;	}
		}
		else {
			*q = *p;
		}
	}
	*q = '\0';
}

char *JabberUrlEncode(const char *str)
{
	char *s, *p, *q;
	int c;

	if (str == NULL)
		return NULL;

	for (c=0,p=(char *)str; *p!='\0'; p++) {
		switch (*p) {
		case '&': c += 5; break;
		case '\'': c += 6; break;
		case '>': c += 4; break;
		case '<': c += 4; break;
		case '"': c += 6; break;
		default: c++; break;
		}
	}
	if ((s=(char *) malloc(c+1)) != NULL) {
		for (p=(char *) str,q=s; *p!='\0'; p++) {
			switch (*p) {
			case '&': strcpy(q, "&amp;"); q += 5; break;
			case '\'': strcpy(q, "&apos;"); q += 6; break;
			case '>': strcpy(q, "&gt;"); q += 4; break;
			case '<': strcpy(q, "&lt;"); q += 4; break;
			case '"': strcpy(q, "&quot;"); q += 6; break;
			default: *q = *p; q++; break;
			}
		}
		*q = '\0';
	}

	return s;
}

char *JabberUtf8Decode(const char *str)
{
	int i, len;
	char *p;
	WCHAR *wszTemp;
	char *szOut;

	if (str == NULL) return NULL;

	len = strlen(str);

	// Convert utf8 to unicode
	if ((wszTemp=(WCHAR *) malloc(sizeof(WCHAR) * (len + 1))) == NULL)
		return NULL;
	p = (char *) str;
	i = 0;
	while (*p) {
		if ((*p & 0x80) == 0)
			wszTemp[i++] = *(p++);
		else if ((*p & 0xe0) == 0xe0) {
			wszTemp[i] = (*(p++) & 0x1f) << 12;
			wszTemp[i] |= (*(p++) & 0x3f) << 6;
			wszTemp[i++] |= (*(p++) & 0x3f);
		}
		else {
			wszTemp[i] = (*(p++) & 0x3f) << 6;
			wszTemp[i++] |= (*(p++) & 0x3f);
		}
	}
	wszTemp[i] = '\0';

	// Convert unicode to local codepage
	if ((len=WideCharToMultiByte(jabberCodePage, 0, wszTemp, -1, NULL, 0, NULL, NULL)) == 0)
		return NULL;
	if ((szOut=(char *) malloc(len)) == NULL)
		return NULL;
	WideCharToMultiByte(jabberCodePage, 0, wszTemp, -1, szOut, len, NULL, NULL);
	free(wszTemp);

	return szOut;

/*
	for(p=q=str; *p!='\0'; p++,q++) {
		if(p[1]!='\0' && (p[0]&0xE0)==0xC0 && (p[1]&0xC0)==0x80) {
			// 110xxxxx 10xxxxxx
			*q = (char) (BYTE) (((p[0]&0x1F)<<6)|(p[1]&0x3F));
			p++;
		}
		else if (p[1]!='\0' && p[2]!='\0' && (p[0]&0xF0)==0xE0 && (p[1]&0xC0)==0x80 && (p[2]&0xC0)==0x80) {
			// 1110xxxx 10xxxxxx 10xxxxxx
			unicode=(((WORD)(p[0]&0x0F))<<12) |
				    (((WORD)(p[1]&0x3F))<<6) |
					((WORD)(p[2]&0x3F));
			// Thai (convert to TIS-620)
			if (unicode>=0x0E00 && unicode<=0x0E7F) {
				*q = (char) ((unicode&0x7F)+0xA0);
			}
			// Default (useless anyway)
			else {
				*q = (char) (unicode&0x7F);
			}
			p += 2;
		}
		else {
			*q = *p;
		}
	}
	*q = '\0';
*/
}

char* JabberUtf8EncodeW( const WCHAR* wstr )
{
	const WCHAR* w;

	// Convert unicode to utf8
	int len = 0;
	for ( w = wstr; *w; w++ ) {
		if (*w < 0x0080) len++;
		else if (*w < 0x0800) len += 2;
		else len += 3;
	}

	unsigned char* szOut = ( unsigned char* )malloc( len+1);
	if ( szOut == NULL )
		return NULL;

	int i = 0;
	for ( w = wstr; *w; w++ ) {
		if (*w < 0x0080)
			szOut[i++] = (unsigned char) *w;
		else if (*w < 0x0800) {
			szOut[i++] = 0xc0 | ((*w) >> 6);
			szOut[i++] = 0x80 | ((*w) & 0x3f);
		}
		else {
			szOut[i++] = 0xe0 | ((*w) >> 12);
			szOut[i++] = 0x80 | (((*w) >> 6) & 0x3f);
			szOut[i++] = 0x80 | ((*w) & 0x3f);
	}	}

	szOut[ i ] = '\0';
	return (char *) szOut;
}

char* JabberUtf8Encode(const char *str)
{
	if ( str == NULL )
		return NULL;

	// Convert local codepage to unicode
	int len = strlen( str );
	WCHAR* wszTemp = ( WCHAR* )alloca( sizeof( WCHAR )*( len+1 ));
	MultiByteToWideChar( jabberCodePage, 0, str, -1, wszTemp, len+1 );

	return JabberUtf8EncodeW( wszTemp );
}

char *JabberSha1(char *str)
{
	SHA1Context sha;
	uint8_t digest[20];
	char *result;
	int i;

	if (str==NULL)
		return NULL;
	if (SHA1Reset(&sha))
		return NULL;
	if (SHA1Input(&sha, ( const unsigned __int8* )str, strlen(str)))
		return NULL;
	if (SHA1Result(&sha, digest))
		return NULL;
	if ((result=(char *)malloc(41)) == NULL)
		return NULL;
	for (i=0; i<20; i++)
		sprintf(result+(i<<1), "%02x", digest[i]);
	return result;
}

char *JabberUnixToDos(const char *str)
{
	char *p, *q, *res;
	int extra;

	if (str==NULL || str[0]=='\0')
		return NULL;

	extra = 0;
	for (p=(char *) str; *p!='\0'; p++) {
		if (*p == '\n')
			extra++;
	}
	if ((res=(char *)malloc(strlen(str)+extra+1)) != NULL) {
		for (p=(char *) str,q=res; *p!='\0'; p++,q++) {
			if (*p == '\n') {
				*q = '\r';
				q++;
			}
			*q = *p;
		}
		*q = '\0';
	}
	return res;
}

char *JabberHttpUrlEncode(const char *str)
{
	unsigned char *p, *q, *res;

	if (str == NULL) return NULL;
	res = (BYTE*) malloc(3*strlen(str) + 1);
	for (p=(BYTE*)str,q=res; *p!='\0'; p++,q++) {
		if ((*p>='A' && *p<='Z') || (*p>='a' && *p<='z') || (*p>='0' && *p<='9') || strchr("$-_.+!*'(),", *p)!=NULL) {
			*q = *p;
		}
		else {
			sprintf((char*)q, "%%%02X", *p);
			q += 2;
		}
	}
	*q = '\0';
	return ( char* )res;
}

void JabberHttpUrlDecode(char *str)
{
	unsigned char *p, *q;
	unsigned int code;

	if (str == NULL) return;
	for (p=q=(BYTE*)str; *p!='\0'; p++,q++) {
		if (*p=='%' && *(p+1)!='\0' && isxdigit(*(p+1)) && *(p+2)!='\0' && isxdigit(*(p+2))) {
			sscanf((char*)p+1, "%2x", &code);
			*q = (unsigned char) code;
			p += 2;
		}
		else {
			*q = *p;
		}
	}
	*q = '\0';
}

int JabberCombineStatus(int status1, int status2)
{
	// Combine according to the following priority (high to low)
	// ID_STATUS_FREECHAT
	// ID_STATUS_ONLINE
	// ID_STATUS_DND
	// ID_STATUS_AWAY
	// ID_STATUS_NA
	// ID_STATUS_INVISIBLE (valid only for TLEN_PLUGIN)
	// ID_STATUS_OFFLINE
	// other ID_STATUS in random order (actually return status1)
	if (status1==ID_STATUS_FREECHAT || status2==ID_STATUS_FREECHAT)
		return ID_STATUS_FREECHAT;
	if (status1==ID_STATUS_ONLINE || status2==ID_STATUS_ONLINE)
		return ID_STATUS_ONLINE;
	if (status1==ID_STATUS_DND || status2==ID_STATUS_DND)
		return ID_STATUS_DND;
	if (status1==ID_STATUS_AWAY || status2==ID_STATUS_AWAY)
		return ID_STATUS_AWAY;
	if (status1==ID_STATUS_NA || status2==ID_STATUS_NA)
		return ID_STATUS_NA;
	if (status1==ID_STATUS_INVISIBLE || status2==ID_STATUS_INVISIBLE)
		return ID_STATUS_INVISIBLE;
	if (status1==ID_STATUS_OFFLINE || status2==ID_STATUS_OFFLINE)
		return ID_STATUS_OFFLINE;
	return status1;
}

struct tagErrorCodeToStr {
	int code;
	char *str;
} JabberErrorCodeToStrMapping[] = {
	{JABBER_ERROR_REDIRECT,					"Redirect"},
	{JABBER_ERROR_BAD_REQUEST,				"Bad request"},
	{JABBER_ERROR_UNAUTHORIZED,				"Unauthorized"},
	{JABBER_ERROR_PAYMENT_REQUIRED,			"Payment required"},
	{JABBER_ERROR_FORBIDDEN,				"Forbidden"},
	{JABBER_ERROR_NOT_FOUND,				"Not found"},
	{JABBER_ERROR_NOT_ALLOWED,				"Not allowed"},
	{JABBER_ERROR_NOT_ACCEPTABLE,			"Not acceptable"},
	{JABBER_ERROR_REGISTRATION_REQUIRED,	"Registration required"},
	{JABBER_ERROR_REQUEST_TIMEOUT,			"Request timeout"},
	{JABBER_ERROR_CONFLICT,					"Conflict"},
	{JABBER_ERROR_INTERNAL_SERVER_ERROR,	"Internal server error"},
	{JABBER_ERROR_NOT_IMPLEMENTED,			"Not implemented"},
	{JABBER_ERROR_REMOTE_SERVER_ERROR,		"Remote server error"},
	{JABBER_ERROR_SERVICE_UNAVAILABLE,		"Service unavailable"},
	{JABBER_ERROR_REMOTE_SERVER_TIMEOUT,	"Remote server timeout"},
	{-1, "Unknown error"}
};

char *JabberErrorStr(int errorCode)
{
	int i;

	for (i=0; JabberErrorCodeToStrMapping[i].code!=-1 && JabberErrorCodeToStrMapping[i].code!=errorCode; i++);
	return JabberErrorCodeToStrMapping[i].str;
}

char *JabberErrorMsg(XmlNode *errorNode)
{
	char *errorStr, *str;
	int errorCode;

	errorStr = (char *) malloc(256);
	if (errorNode == NULL) {
		_snprintf(errorStr, 256, "%s -1: %s", Translate("Error"), Translate("Unknown error message"));
		return errorStr;
	}

	errorCode = -1;
	if ((str=JabberXmlGetAttrValue(errorNode, "code")) != NULL)
		errorCode = atoi(str);
	if ((str=errorNode->text) != NULL)
		_snprintf(errorStr, 256, "%s %d: %s\r\n%s", Translate("Error"), errorCode, Translate(JabberErrorStr(errorCode)), str);
	else
		_snprintf(errorStr, 256, "%s %d: %s", Translate("Error"), errorCode, Translate(JabberErrorStr(errorCode)));
	return errorStr;
}

void JabberSendVisibleInvisiblePresence(BOOL invisible)
{
	JABBER_LIST_ITEM *item;
	HANDLE hContact;
	WORD apparentMode;
	int i;

	if (!jabberOnline) return;

	i = 0;
	while ((i=JabberListFindNext(LIST_ROSTER, i)) >= 0) {
		if ((item=JabberListGetItemPtrFromIndex(i)) != NULL) {
			if ((hContact=JabberHContactFromJID(item->jid)) != NULL) {
				apparentMode = DBGetContactSettingWord(hContact, jabberProtoName, "ApparentMode", 0);
				if (invisible==TRUE && apparentMode==ID_STATUS_OFFLINE) {
					JabberSend(jabberThreadInfo->s, "<presence to='%s' type='invisible'/>", item->jid);
				}
				else if (invisible==FALSE && apparentMode==ID_STATUS_ONLINE) {
					JabberSend(jabberThreadInfo->s, "<presence to='%s'/>", item->jid);
				}
			}
		}
		i++;
	}
}

char* JabberTextEncode( const char* str )
{
	if ( str == NULL )
		return NULL;

	char* s1 = JabberUrlEncode( str );
	if ( s1 == NULL )
		return NULL;

	// Convert invalid control characters to space
	if ( *s1 ) {
		char *p, *q;

		for ( p = s1; *p != '\0'; p++ )
			if ( *p > 0 && *p < 0x20 && *p != 0x09 && *p != 0x0a && *p != 0x0d )
				*p = ( char )0x20;

		for ( p = q = s1; *p!='\0'; p++ ) {
			if ( *p != '\r' ) {
				*q = *p;
				q++;
		}	}

		*q = '\0';
	}

	char* s2 = JabberUtf8Encode( s1 );
	free( s1 );
	return s2;
}

char* JabberTextEncodeW( const wchar_t* str )
{
	if ( str == NULL )
		return NULL;

	wchar_t* s1 = ( wchar_t* )alloca(( wcslen( str )+1 )*sizeof( wchar_t ));
	wchar_t *p, *q;

	for ( p = ( WCHAR* )str, q = s1; *p; p++ )
		if ( *p != '\r' ) {
			*q = *p;
			q++;
		}

	*q = '\0';

	for ( p = s1; *p != '\0'; p++ )
		if ( *p > 0 && *p < 0x20 && *p != 0x09 && *p != 0x0a && *p != 0x0d )
			*p = ( char )0x20;


	return JabberUtf8EncodeW( s1 );
}

char *JabberTextDecode(const char *str)
{
	char *s1;
	char *s2;

	if (str == NULL) return NULL;
	if ((s1=JabberUtf8Decode(str)) == NULL)
		return NULL;
	JabberUrlDecode(s1);
	if ((s2=JabberUnixToDos(s1)) == NULL) {
		free(s1);
		return FALSE;
	}
	free(s1);
	return s2;
}

static char b64table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char *JabberBase64Encode(const char *buffer, int bufferLen)
{
	int n;
	unsigned char igroup[3];
	char *p, *peob;
	char *res, *r;
	int c = 0;

	if (buffer==NULL || bufferLen<=0) return NULL;
	if ((res=(char *) malloc((((bufferLen+2)/3)*4) + 1)) == NULL) return NULL;

	for (p=(char*)buffer,peob=p+bufferLen,r=res; p<peob;) {
		igroup[0] = igroup[1] = igroup[2] = 0;
		for (n=0; n<3; n++) {
			if (p >= peob) break;
			igroup[n] = (unsigned char) *p;
			p++;
		}
		if (n > 0) {
			r[0] = b64table[ igroup[0]>>2 ];
			r[1] = b64table[ ((igroup[0]&3)<<4) | (igroup[1]>>4) ];
			r[2] = b64table[ ((igroup[1]&0xf)<<2) | (igroup[2]>>6) ];
			r[3] = b64table[ igroup[2]&0x3f ];
			if (n < 3) {
				r[3] = '=';
				if (n < 2)
					r[2] = '=';
			}
			r += 4;
		}
	}
	*r = '\0';

	return res;
}

static unsigned char b64rtable[256];

char *JabberBase64Decode(const char *str, int *resultLen)
{
	char *res;
	unsigned char *p, *r, igroup[4], a[4];
	int n, num, count;

	if (str==NULL || resultLen==NULL) return NULL;
	if ((res=(char *) malloc(((strlen(str)+3)/4)*3)) == NULL) return NULL;

	for (n=0; n<256; n++)
		b64rtable[n] = (unsigned char) 0x80;
	for (n=0; n<26; n++)
		b64rtable['A'+n] = n;
	for (n=0; n<26; n++)
		b64rtable['a'+n] = n + 26;
	for (n=0; n<10; n++)
		b64rtable['0'+n] = n + 52;
	b64rtable['+'] = 62;
	b64rtable['/'] = 63;
	b64rtable['='] = 0;
	count = 0;
	for (p=(unsigned char *)str,r=(unsigned char *)res; *p!='\0';) {
		for (n=0; n<4; n++) {
			if (*p=='\0' || b64rtable[*p]==0x80) {
				free(res);
				return NULL;
			}
			a[n] = *p;
			igroup[n] = b64rtable[*p];
			p++;
		}
		r[0] = igroup[0]<<2 | igroup[1]>>4;
		r[1] = igroup[1]<<4 | igroup[2]>>2;
		r[2] = igroup[2]<<6 | igroup[3];
		r += 3;
		num = (a[2]=='='?1:(a[3]=='='?2:3));
		count += num;
		if (num < 3) break;
	}
	*resultLen = count;

	return res;
}

char *JabberGetVersionText()
{
	char filename[MAX_PATH], *fileVersion, *res;
	DWORD unused;
	DWORD verInfoSize;
	UINT  blockSize;
	PVOID pVerInfo;

	GetModuleFileName(hInst, filename, sizeof(filename));
	verInfoSize = GetFileVersionInfoSize(filename, &unused);
	if ((pVerInfo=malloc(verInfoSize)) != NULL) {
		GetFileVersionInfo(filename, 0, verInfoSize, pVerInfo);
		VerQueryValue(pVerInfo, "\\StringFileInfo\\040904b0\\FileVersion", ( LPVOID* )&fileVersion, &blockSize);
		if (strstr(fileVersion, "cvs")) {
			res = (char *) malloc(strlen(fileVersion) + strlen(__DATE__) + 2);
			sprintf(res, "%s %s", fileVersion, __DATE__);
		}
		else {
			res = _strdup(fileVersion);
		}
		free(pVerInfo);
		return res;
	}
	return NULL;
}

time_t JabberIsoToUnixTime(char *stamp)
{
	struct tm timestamp;
	char date[9];
	char *p;
	int i, y;
	time_t t;

	if (stamp == NULL) return (time_t) 0;

	p = stamp;

	// Get the date part
	for (i=0; *p!='\0' && i<8 && isdigit(*p); p++,i++)
		date[i] = *p;

	// Parse year
	if (i == 6) {
		// 2-digit year (1970-2069)
		y = (date[0]-'0')*10 + (date[1]-'0');
		if (y < 70) y += 100;
	}
	else if (i == 8) {
		// 4-digit year
		y = (date[0]-'0')*1000 + (date[1]-'0')*100 + (date[2]-'0')*10 + date[3]-'0';
		y -= 1900;
	}
	else
		return (time_t) 0;
	timestamp.tm_year = y;
	// Parse month
	timestamp.tm_mon = (date[i-4]-'0')*10 + date[i-3]-'0' - 1;
	// Parse date
	timestamp.tm_mday = (date[i-2]-'0')*10 + date[i-1]-'0';

	// Skip any date/time delimiter
	for (; *p!='\0' && !isdigit(*p); p++);

	// Parse time
	if (sscanf(p, "%d:%d:%d", &(timestamp.tm_hour), &(timestamp.tm_min), &(timestamp.tm_sec)) != 3)
		return (time_t) 0;

	timestamp.tm_isdst = 0;	// DST is already present in _timezone below
	t = mktime(&timestamp);

	_tzset();
	t -= _timezone;

	JabberLog("%s is %s", stamp, ctime(&t));

	if (t >= 0)
		return t;
	else
		return (time_t) 0;
}

int JabberCountryNameToId(char *ctry)
{
	int ctryCount, i;
	struct CountryListEntry *ctryList;
	static struct CountryListEntry extraCtry[] = {
		{ 1,		"United States" },
		{ 1,		"United States of America" },
		{ 1,		"US" },
		{ 44,		"England" }
	};

	// Check for some common strings not present in the country list
	ctryCount = sizeof(extraCtry)/sizeof(extraCtry[0]);
	for (i=0; i<ctryCount && stricmp(extraCtry[i].szName, ctry); i++);
	if (i < ctryCount)
		return extraCtry[i].id;

	// Check Miranda country list
	CallService(MS_UTILS_GETCOUNTRYLIST, (WPARAM) &ctryCount, (LPARAM) &ctryList);
	for (i=0; i<ctryCount && stricmp(ctryList[i].szName, ctry); i++);
	if (i < ctryCount)
		return ctryList[i].id;
	else
		return 0xffff; // Unknown
}

void JabberSendPresenceTo(int status, char *to, char *extra)
{
	char priorityStr[32];
	char toStr[512];

	if (!jabberOnline) return;

	// Send <presence/> update for status (we won't handle ID_STATUS_OFFLINE here)
	// Note: jabberModeMsg is already encoded using JabberTextEncode()
	EnterCriticalSection(&modeMsgMutex);

	_snprintf(priorityStr, sizeof(priorityStr), "<priority>%d</priority>", DBGetContactSettingWord(NULL, jabberProtoName, "Priority", 0));

	if (to != NULL)
		_snprintf(toStr, sizeof(toStr), " to='%s'", to);
	else
		toStr[0] = '\0';

	switch (status) {
	case ID_STATUS_ONLINE:
		if (modeMsgs.szOnline)
			JabberSend(jabberThreadInfo->s, "<presence%s><status>%s</status>%s%s</presence>", toStr, modeMsgs.szOnline, priorityStr, (extra!=NULL)?extra:"");
		else
			JabberSend(jabberThreadInfo->s, "<presence%s>%s%s</presence>", toStr, priorityStr, (extra!=NULL)?extra:"");
		break;
	case ID_STATUS_INVISIBLE:
		JabberSend(jabberThreadInfo->s, "<presence type='invisible'%s>%s%s</presence>", toStr, priorityStr, (extra!=NULL)?extra:"");
		break;
	case ID_STATUS_AWAY:
	case ID_STATUS_ONTHEPHONE:
	case ID_STATUS_OUTTOLUNCH:
		if (modeMsgs.szAway)
			JabberSend(jabberThreadInfo->s, "<presence%s><show>away</show><status>%s</status>%s%s</presence>", toStr, modeMsgs.szAway, priorityStr, (extra!=NULL)?extra:"");
		else
			JabberSend(jabberThreadInfo->s, "<presence%s><show>away</show>%s%s</presence>", toStr, priorityStr, (extra!=NULL)?extra:"");
		break;
	case ID_STATUS_NA:
		if (modeMsgs.szNa)
			JabberSend(jabberThreadInfo->s, "<presence%s><show>xa</show><status>%s</status>%s%s</presence>", toStr, modeMsgs.szNa, priorityStr, (extra!=NULL)?extra:"");
		else
			JabberSend(jabberThreadInfo->s, "<presence%s><show>xa</show>%s%s</presence>", toStr, priorityStr, (extra!=NULL)?extra:"");
		break;
	case ID_STATUS_DND:
	case ID_STATUS_OCCUPIED:
		if (modeMsgs.szDnd)
			JabberSend(jabberThreadInfo->s, "<presence%s><show>dnd</show><status>%s</status>%s%s</presence>", toStr, modeMsgs.szDnd, priorityStr, (extra!=NULL)?extra:"");
		else
			JabberSend(jabberThreadInfo->s, "<presence%s><show>dnd</show>%s%s</presence>", toStr, priorityStr, (extra!=NULL)?extra:"");
		break;
	case ID_STATUS_FREECHAT:
		if (modeMsgs.szFreechat)
			JabberSend(jabberThreadInfo->s, "<presence%s><show>chat</show><status>%s</status>%s%s</presence>", toStr, modeMsgs.szFreechat, priorityStr, (extra!=NULL)?extra:"");
		else
			JabberSend(jabberThreadInfo->s, "<presence%s><show>chat</show>%s%s</presence>", toStr, priorityStr, (extra!=NULL)?extra:"");
		break;
	default:
		// Should not reach here
		break;
	}
	LeaveCriticalSection(&modeMsgMutex);
}

void JabberSendPresence(int status)
{
	JABBER_LIST_ITEM *item;
	int i;

	JabberSendPresenceTo(status, NULL, NULL);

	if (status == ID_STATUS_INVISIBLE)
		JabberSendVisiblePresence();
	else
		JabberSendInvisiblePresence();

	if (status == ID_STATUS_INVISIBLE) {
		i = 0;
		while ((i=JabberListFindNext(LIST_CHATROOM, i)) >= 0) {
			if ((item=JabberListGetItemPtrFromIndex(i)) != NULL) {
				// Quit all chatrooms when change to invisible
				PostMessage(item->hwndGcDlg, WM_JABBER_GC_FORCE_QUIT, 0, 0);
			}
			i++;
		}
	}
	else {
		i = 0;
		while ((i=JabberListFindNext(LIST_CHATROOM, i)) >= 0) {
			if ((item=JabberListGetItemPtrFromIndex(i)) != NULL) {
				// Also update status in all chatrooms
				JabberSendPresenceTo(status, item->jid, NULL);
			}
			i++;
		}
	}
}

char *JabberRtfEscape(char *str)
{
	char *escapedStr;
	int size;
	char *p, *q;

	if (str == NULL)
		return NULL;

	for (p=str,size=0; *p!='\0'; p++) {
		if (*p=='\\' || *p=='{' || *p=='}')
			size += 2;
		else if (*p=='\n' || *p=='\t')
			size += 5;
		else
			size++;
	}

	if ((escapedStr=(char *)malloc(size+1)) == NULL)
		return NULL;

	for (p=str,q=escapedStr; *p!='\0'; p++) {
		if (strchr("\\{}", *p) != NULL) {
			*q++ = '\\';
			*q++ = *p;
		}
		else if (*p == '\n') {
			strcpy(q, "\\par ");
			q += 5;
		}
		else if (*p == '\t') {
			strcpy(q, "\\tab ");
			q += 5;
		}
		else {
			*q++ = *p;
		}
	}
	*q = '\0';

	return escapedStr;
}

void JabberStringAppend(char **str, int *sizeAlloced, const char *fmt, ...)
{
	va_list vararg;
	char *p;
	int size, len;

	if (str == NULL) return;

	if (*str==NULL || *sizeAlloced<=0) {
		*sizeAlloced = size = 2048;
		*str = (char *) malloc(size);
		len = 0;
	}
	else {
		len = strlen(*str);
		size = *sizeAlloced - strlen(*str);
	}

	p = *str + len;
	va_start(vararg, fmt);
	while (_vsnprintf(p, size, fmt, vararg) == -1) {
		size += 2048;
		(*sizeAlloced) += 2048;
		*str = (char *) realloc(*str, *sizeAlloced);
		p = *str + len;
	}
	va_end(vararg);
}

static char clientJID[3072];

char *JabberGetClientJID(char *jid)
{
	char *resource, *p;

	if (jid == NULL) return NULL;
	strncpy(clientJID, jid, sizeof(clientJID));
	clientJID[sizeof(clientJID)-1] = '\0';
	if ((p=strchr(clientJID, '/')) == NULL) {
		p = clientJID + strlen(clientJID);
		if ((resource=JabberListGetBestResourceNamePtr(jid)) != NULL)
			_snprintf(p, sizeof(clientJID)-strlen(clientJID)-1, "/%s", resource);
	}

	return clientJID;
}