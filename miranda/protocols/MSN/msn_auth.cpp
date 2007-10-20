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
#include "des.h"

static const char defaultPassportUrl[] = "https://login.live.com/RST.srf";

static const char authPacket[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<Envelope xmlns=\"http://schemas.xmlsoap.org/soap/envelope/\""
		" xmlns:wsse=\"http://schemas.xmlsoap.org/ws/2003/06/secext\"" 
		" xmlns:saml=\"urn:oasis:names:tc:SAML:1.0:assertion\"" 
		" xmlns:wsp=\"http://schemas.xmlsoap.org/ws/2002/12/policy\"" 
		" xmlns:wsu=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\"" 
		" xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/03/addressing\"" 
		" xmlns:wssc=\"http://schemas.xmlsoap.org/ws/2004/04/sc\"" 
		" xmlns:wst=\"http://schemas.xmlsoap.org/ws/2004/04/trust\">"
	"<Header>"
		"<ps:AuthInfo xmlns:ps=\"http://schemas.microsoft.com/Passport/SoapServices/PPCRL\" Id=\"PPAuthInfo\">"
			"<ps:HostingApp>{7108E71A-9926-4FCB-BCC9-9A9D3F32E423}</ps:HostingApp>"
			"<ps:BinaryVersion>4</ps:BinaryVersion>"
			"<ps:UIVersion>1</ps:UIVersion>"
			"<ps:Cookies></ps:Cookies>"
			"<ps:RequestParams>AQAAAAIAAABsYwQAAAAxMDMz</ps:RequestParams>"
		"</ps:AuthInfo>"
		"<wsse:Security>"
			"<wsse:UsernameToken Id=\"user\">"
				"<wsse:Username>%s</wsse:Username>"
				"<wsse:Password>%s</wsse:Password>"
			"</wsse:UsernameToken>"
		"</wsse:Security>"
	"</Header>"
	"<Body>"
		"<ps:RequestMultipleSecurityTokens xmlns:ps=\"http://schemas.microsoft.com/Passport/SoapServices/PPCRL\" Id=\"RSTS\">"
			"<wst:RequestSecurityToken Id=\"RST0\">"
				"<wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>"
				"<wsp:AppliesTo>"
					"<wsa:EndpointReference>"
						"<wsa:Address>http://Passport.NET/tb</wsa:Address>"
					"</wsa:EndpointReference>"
				"</wsp:AppliesTo>"
			"</wst:RequestSecurityToken>"
			"<wst:RequestSecurityToken Id=\"RST1\">"
				"<wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>"
				"<wsp:AppliesTo>"
					"<wsa:EndpointReference>"
						"<wsa:Address>messengerclear.live.com</wsa:Address>"
					"</wsa:EndpointReference>"
				"</wsp:AppliesTo>"
				"<wsse:PolicyReference URI=\"MBI_KEY_OLD\"></wsse:PolicyReference>"
			"</wst:RequestSecurityToken>"
			"<wst:RequestSecurityToken Id=\"RST2\">"
				"<wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>"
				"<wsp:AppliesTo>"
					"<wsa:EndpointReference>"
						"<wsa:Address>messenger.msn.com</wsa:Address>"
					"</wsa:EndpointReference>"
				"</wsp:AppliesTo>"
				"<wsse:PolicyReference URI=\"?id=507\"></wsse:PolicyReference>"
			"</wst:RequestSecurityToken>"
			"<wst:RequestSecurityToken Id=\"RST3\">"
				"<wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>"
				"<wsp:AppliesTo>"
					"<wsa:EndpointReference>"
						"<wsa:Address>contacts.msn.com</wsa:Address>"
					"</wsa:EndpointReference>"
				"</wsp:AppliesTo>"
				"<wsse:PolicyReference URI=\"MBI\"></wsse:PolicyReference>"
			"</wst:RequestSecurityToken>"
			"<wst:RequestSecurityToken Id=\"RST4\">"
				"<wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>"
				"<wsp:AppliesTo>"
					"<wsa:EndpointReference>"
						"<wsa:Address>messengersecure.live.com</wsa:Address>"
					"</wsa:EndpointReference>"
				"</wsp:AppliesTo>"
				"<wsse:PolicyReference URI=\"MBI_SSL\"></wsse:PolicyReference>"
			"</wst:RequestSecurityToken>"
			"<wst:RequestSecurityToken Id=\"RST5\">"
				"<wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>"
				"<wsp:AppliesTo>"
					"<wsa:EndpointReference>"
						"<wsa:Address>spaces.live.com</wsa:Address>"
					"</wsa:EndpointReference>"
				"</wsp:AppliesTo>"
				"<wsse:PolicyReference URI=\"MBI\"></wsse:PolicyReference>"
			"</wst:RequestSecurityToken>"
			"<wst:RequestSecurityToken Id=\"RST6\">"
				"<wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>"
				"<wsp:AppliesTo>"
					"<wsa:EndpointReference>"
						"<wsa:Address>storage.msn.com</wsa:Address>"
					"</wsa:EndpointReference>"
				"</wsp:AppliesTo>"
				"<wsse:PolicyReference URI=\"MBI\"></wsse:PolicyReference>"
			"</wst:RequestSecurityToken>"
		"</ps:RequestMultipleSecurityTokens>"
	"</Body>"
"</Envelope>";


char *pAuthToken = NULL, *tAuthToken = NULL; 
char *oimSendToken = NULL;
char *authStrToken = NULL, *authSecretToken = NULL; 
char *authContactToken = NULL;
char *hotSecretToken = NULL, *hotAuthToken = NULL;

/////////////////////////////////////////////////////////////////////////////////////////
// Performs the MSN Passport login via TLS

int MSN_GetPassportAuth( void )
{
	int retVal = -1;
	SSLAgent mAgent;

	char szPassword[ 100 ];
	MSN_GetStaticString( "Password", NULL, szPassword, sizeof( szPassword ));
	MSN_CallService( MS_DB_CRYPT_DECODESTRING, strlen( szPassword )+1, ( LPARAM )szPassword );
	szPassword[ 16 ] = 0;
	char* szEncPassword = HtmlEncode(szPassword);

	const size_t len = sizeof(authPacket) + 2048;
	char* szAuthInfo = ( char* )alloca( len );
	mir_snprintf( szAuthInfo, len, authPacket, MyOptions.szEmail, szEncPassword );

	mir_free( szEncPassword );

	char szPassportHost[ 256 ];
	if ( MSN_GetStaticString( "MsnPassportHost", NULL, szPassportHost, sizeof( szPassportHost )) 
		|| strstr( szPassportHost, "/RST.srf" ) == NULL )
		strcpy( szPassportHost, defaultPassportUrl );

	bool defaultUrlAllow = strcmp( szPassportHost, defaultPassportUrl ) != 0;
	char *tResult = NULL;

	while (retVal == -1)
	{
		unsigned status;
		MimeHeaders httpInfo;
		char* htmlbody;

		tResult = mAgent.getSslResult( szPassportHost, szAuthInfo, NULL, status, httpInfo, htmlbody);
		if ( tResult == NULL ) {
			if ( defaultUrlAllow ) {
				strcpy( szPassportHost, defaultPassportUrl );
				defaultUrlAllow = false;
				continue;
			}
			else {
				retVal = 4;
				break;
		}	}

		switch ( status )
		{
			case 200: 
			{
				ezxml_t xml = ezxml_parse_str((char*)htmlbody, strlen(htmlbody));

				ezxml_t tokr = ezxml_get(xml, "S:Body", 0, 
					"wst:RequestSecurityTokenResponseCollection", 0,
					"wst:RequestSecurityTokenResponse", -1);
				
				while (tokr != NULL)
				{
					ezxml_t toks = ezxml_get(tokr, "wst:RequestedSecurityToken", 0, 
						"wsse:BinarySecurityToken", -1);
					if (toks != NULL) 
					{
						const char* id = ezxml_attr(toks, "Id");
						if (strcmp(id, "Compact1")==0)
						{
							ezxml_t node = ezxml_get(tokr, "wst:RequestedProofToken", 0, 
								"wst:BinarySecret", -1);
							replaceStr(authStrToken, ezxml_txt(toks));
							replaceStr(authSecretToken, ezxml_txt(node)); 
							retVal = 0;
						}
						if (strcmp(id, "PPToken2")==0)
						{
							const char* tok = ezxml_txt(toks);
							const size_t toklen = strlen(tok);
							char* ch = (char*)strchr(tok, '&');
							*ch = 0;
							replaceStr(tAuthToken, tok+2);
							replaceStr(pAuthToken, ch+3);
							*ch = '&';
						}
						else if (strcmp(id, "Compact3")==0)
						{
							replaceStr(authContactToken, ezxml_txt(toks));
						}
						else if (strcmp(id, "Compact4")==0)
						{
							replaceStr(oimSendToken, ezxml_txt(toks));
						}
					}
					else
					{
						ezxml_t node = ezxml_get(tokr, "wst:RequestedSecurityToken", 0, "EncryptedData", -1);
						hotAuthToken = ezxml_toxml(node, 0);
						
						node = ezxml_get(tokr, "wst:RequestedProofToken", 0, "wst:BinarySecret", -1);
						replaceStr(hotSecretToken, ezxml_txt(node));
					}

					tokr = ezxml_next(tokr); 
				}

				if (retVal != 0)
				{
					ezxml_t tokrdr = ezxml_get(xml, "S:Fault", 0, "psf:redirectUrl", -1);
					if (tokrdr != NULL)
					{
						strcpy(szPassportHost, ezxml_txt(tokrdr));
						MSN_DebugLog( "Redirected to '%s'", szPassportHost );
					}
					else
					{
						const char* szFault = ezxml_txt(ezxml_get(xml, "S:Fault", 0, "faultcode", -1));
						retVal = strcmp( szFault, "wsse:FailedAuthentication" ) == 0 ? 3 : 5;
						if (retVal == 5 && defaultUrlAllow)
						{
							strcpy( szPassportHost, defaultPassportUrl );
							defaultUrlAllow = false;
							retVal = -1;
						}
					}
				}

				ezxml_free(xml);
				break;
			}
			default:
				if ( defaultUrlAllow ) {
					strcpy( szPassportHost, defaultPassportUrl );
					defaultUrlAllow = false;
				}
				else 
					retVal = 6;
		}
		mir_free( tResult );
	}

	if ( retVal != 0 ) 
	{
		MSN_ShowError( retVal == 3 ? "Your username or password is incorrect" : 
			"Unable to contact MS Passport servers check proxy/firewall settings" );
		MSN_SendBroadcast( NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD );
	}
	else
		MSN_SetString(NULL, "MsnPassportHost", szPassportHost);

	MSN_DebugLog( "MSN_CheckRedirector exited with errorCode = %d", retVal );
	return retVal;
}

void hmac_sha1 (mir_sha1_byte_t *md, mir_sha1_byte_t *key, size_t keylen, mir_sha1_byte_t *text, size_t textlen)
{
	const unsigned SHA_BLOCKSIZE = 64;

	unsigned char mdkey[MIR_SHA1_HASH_SIZE];
	unsigned char k_ipad[SHA_BLOCKSIZE], k_opad[SHA_BLOCKSIZE];
	mir_sha1_ctx ctx;

	if (keylen > SHA_BLOCKSIZE) 
	{
		mir_sha1_init(&ctx);
		mir_sha1_append(&ctx, key, keylen);
		mir_sha1_finish(&ctx, mdkey);
		keylen = 20;
		key = mdkey;
	}

	memcpy(k_ipad, key, keylen);
	memcpy(k_opad, key, keylen);
	memset(k_ipad+keylen, 0x36, SHA_BLOCKSIZE - keylen);
	memset(k_opad+keylen, 0x5c, SHA_BLOCKSIZE - keylen);

	for (unsigned i = 0; i < keylen; i++) {
		k_ipad[i] ^= 0x36;
		k_opad[i] ^= 0x5c;
	}

	mir_sha1_init(&ctx);
	mir_sha1_append(&ctx, k_ipad, SHA_BLOCKSIZE);
	mir_sha1_append(&ctx, text, textlen);
	mir_sha1_finish(&ctx, md);

	mir_sha1_init(&ctx);
	mir_sha1_append(&ctx, k_opad, SHA_BLOCKSIZE);
	mir_sha1_append(&ctx, md, MIR_SHA1_HASH_SIZE);
	mir_sha1_finish(&ctx, md);
}


static void derive_key(mir_sha1_byte_t* der, unsigned char* key, size_t keylen, unsigned char* data, size_t datalen)
{
	mir_sha1_byte_t hash1[MIR_SHA1_HASH_SIZE];
	mir_sha1_byte_t hash2[MIR_SHA1_HASH_SIZE];
	mir_sha1_byte_t hash3[MIR_SHA1_HASH_SIZE];
	mir_sha1_byte_t hash4[MIR_SHA1_HASH_SIZE];

	const size_t buflen = MIR_SHA1_HASH_SIZE + datalen;
	mir_sha1_byte_t* buf = (mir_sha1_byte_t*)alloca(buflen);

	hmac_sha1(hash1, key, keylen, data, datalen);
	hmac_sha1(hash3, key, keylen, hash1, MIR_SHA1_HASH_SIZE);
	
	memcpy(buf, hash1, MIR_SHA1_HASH_SIZE);
	memcpy(buf + MIR_SHA1_HASH_SIZE, data, datalen);
	hmac_sha1(hash2, key, keylen, buf, buflen);
		
	memcpy(buf, hash3, MIR_SHA1_HASH_SIZE);
	memcpy(buf + MIR_SHA1_HASH_SIZE, data, datalen);
	hmac_sha1(hash4, key, keylen, buf, buflen);
        
	memcpy(der, hash2, MIR_SHA1_HASH_SIZE);
	memcpy(der + MIR_SHA1_HASH_SIZE, hash4, 4);
}

typedef struct tag_MsgrUsrKeyHdr
{
	unsigned size;
	unsigned cryptMode;
	unsigned cipherType;
	unsigned hashType;
	unsigned ivLen;
	unsigned hashLen;
	unsigned long cipherLen;
} MsgrUsrKeyHdr;

static const MsgrUsrKeyHdr userKeyHdr = 
{ 
	sizeof(MsgrUsrKeyHdr),
	1,			// CRYPT_MODE_CBC
	0x6603,		// CALG_3DES
	0x8004,		// CALG_SHA1
	8,			// sizeof(ivBytes)
	MIR_SHA1_HASH_SIZE,
	72			// sizeof(cipherBytes);
};


static unsigned char* PKCS5_Padding(char* in, size_t &len)
{
	const size_t nlen = ((len >> 3) + 1) << 3;
	unsigned char* res = (unsigned char*)mir_alloc(nlen);
	memcpy(res, in, len);

	const unsigned char pad =  8 - (len & 7);
	memset(res + len, pad, pad);

	len = nlen;
	return res;
}


char* GenerateLoginBlob(char* challenge) 
{
	const size_t keylen = strlen(authSecretToken);
	size_t key1len = Netlib_GetBase64DecodedBufferSize(keylen);
	unsigned char* key1 = (unsigned char*)alloca(key1len);

	NETLIBBASE64 nlb = { authSecretToken, keylen, key1, key1len };
	MSN_CallService( MS_NETLIB_BASE64DECODE, 0, LPARAM( &nlb ));
	key1len = nlb.cbDecoded; 
	
	mir_sha1_byte_t key2[MIR_SHA1_HASH_SIZE+4];
	mir_sha1_byte_t key3[MIR_SHA1_HASH_SIZE+4];

	static const unsigned char encdata1[] = "WS-SecureConversationSESSION KEY HASH";
	static const unsigned char encdata2[] = "WS-SecureConversationSESSION KEY ENCRYPTION";

	derive_key(key2, key1, key1len, (unsigned char*)encdata1, sizeof(encdata1) - 1);
	derive_key(key3, key1, key1len, (unsigned char*)encdata2, sizeof(encdata2) - 1);

	size_t chllen = strlen(challenge);

	mir_sha1_byte_t hash[MIR_SHA1_HASH_SIZE];
	hmac_sha1(hash, key2, MIR_SHA1_HASH_SIZE+4, (mir_sha1_byte_t*)challenge, chllen);

	unsigned char* newchl = PKCS5_Padding(challenge, chllen);

	const size_t pktsz = sizeof(MsgrUsrKeyHdr) + MIR_SHA1_HASH_SIZE + 8 + chllen;
	unsigned char* userKey = (unsigned char*)alloca(pktsz);

	unsigned char* p = userKey;
	memcpy(p, &userKeyHdr, sizeof(MsgrUsrKeyHdr));
	((MsgrUsrKeyHdr*)p)->cipherLen = chllen;
	p += sizeof(MsgrUsrKeyHdr);
	
	unsigned char iv[8];

	srand(time(NULL));
	for (int i=0; i<sizeof(iv)/2; ++i) ((unsigned short*)iv)[i] = rand() + rand();
	
	memcpy(p, iv, sizeof(iv));
	p += sizeof(iv);

	memcpy(p, hash, sizeof(hash));
	p += MIR_SHA1_HASH_SIZE;

	des3_context ctxd = {0};
	des3_set_3keys(&ctxd, key3);
	des3_cbc_encrypt(&ctxd, iv, newchl, p, chllen);

	mir_free(newchl);

	const size_t rlen = Netlib_GetBase64EncodedBufferSize(pktsz);
	char* buf = (char*)mir_alloc(rlen);

	NETLIBBASE64 nlb1 = { buf, rlen, userKey, pktsz };
	MSN_CallService( MS_NETLIB_BASE64ENCODE, 0, LPARAM( &nlb1 ));

	return buf;
}


char* HotmailLogin(const char* url, const char* id, bool ismail)
{
	unsigned char nonce[24];
	srand(time(NULL));
	for (int i=0; i<sizeof(nonce)/2; ++i) ((unsigned short*)nonce)[i] = rand() + rand();

	const size_t hotSecretlen = strlen(hotSecretToken);
	size_t key1len = Netlib_GetBase64DecodedBufferSize(hotSecretlen);
	unsigned char* key1 = (unsigned char*)alloca(key1len);

	NETLIBBASE64 nlb = { hotSecretToken, hotSecretlen, key1, key1len };
	MSN_CallService( MS_NETLIB_BASE64DECODE, 0, LPARAM( &nlb ));
	key1len = nlb.cbDecoded; 

	static const unsigned char encdata[] = "WS-SecureConversation";
	const size_t data1len = sizeof(nonce) + sizeof(encdata) - 1;
	
	unsigned char* data1 = (unsigned char*)alloca(data1len);
	memcpy(data1, encdata, sizeof(encdata) - 1);
	memcpy(data1 + sizeof(encdata) - 1, nonce, sizeof(nonce));

	unsigned char key2[MIR_SHA1_HASH_SIZE+4];
	derive_key(key2, key1, key1len, data1, data1len);

	static const char postdataM[] = "ct=%u&bver=4&id=%s&rru=%s&svc=mail&js=yes&pl=%%3Fid%%3D%s&da=%s&nonce=";
	static const char postdataS[] = "ct=%u&bver=4&id=%s&ru=%s&js=yes&pl=%%3Fid%%3D%s&da=%s&nonce=";
	const char *postdata = ismail ? postdataM : postdataS;
  
	const size_t xmlenclen = 2 * strlen(hotAuthToken);
	char* xmlenc = (char*)alloca(xmlenclen);
	UrlEncode(hotAuthToken, xmlenc, xmlenclen);

	char rruenc[256];
	UrlEncode(url, rruenc, sizeof(rruenc));

	const size_t fnpstlen = sizeof(postdata) + strlen(xmlenc) + strlen(rruenc) + 200;
	char* fnpst = (char*)alloca(fnpstlen);

	size_t sz = mir_snprintf(fnpst, fnpstlen, postdata, time(NULL), id, rruenc, id, xmlenc);

	size_t noncenclen = Netlib_GetBase64EncodedBufferSize(sizeof(nonce));
	char* noncenc = (char*)alloca(noncenclen);
	NETLIBBASE64 nlb1 = { noncenc, noncenclen, nonce, sizeof(nonce) };
	MSN_CallService( MS_NETLIB_BASE64ENCODE, 0, LPARAM( &nlb1 ));
	noncenclen = nlb1.cchEncoded - 1;
	
	UrlEncode(noncenc, fnpst + sz, fnpstlen - sz);
	sz = strlen(fnpst);

	mir_sha1_byte_t hash[MIR_SHA1_HASH_SIZE];
	hmac_sha1(hash, key2, sizeof(key2), (mir_sha1_byte_t*)fnpst, sz);
	
	sz += mir_snprintf(fnpst + sz, fnpstlen - sz, "&hash=");

	NETLIBBASE64 nlb2 = { fnpst + sz, fnpstlen - sz, hash, sizeof(hash) };
	MSN_CallService( MS_NETLIB_BASE64ENCODE, 0, LPARAM( &nlb2 ));

	char* res = (char*)mir_alloc(2*fnpstlen);
	sz = mir_snprintf(res, 2*fnpstlen, "token=");
	UrlEncode(fnpst, res+sz, 2*fnpstlen - sz);

	return res;
}

void FreeAuthTokens(void)
{
	mir_free(pAuthToken);
	mir_free(tAuthToken);
	mir_free(oimSendToken);
	mir_free(authStrToken);
	mir_free(authSecretToken); 
	mir_free(authContactToken);
	mir_free(hotSecretToken);
	free(hotAuthToken);
}
