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
			"<ps:BinaryVersion>3</ps:BinaryVersion>"
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
						"<wsa:Address>messenger.msn.com</wsa:Address>"
					"</wsa:EndpointReference>"
				"</wsp:AppliesTo>"
				"<wsse:PolicyReference URI=\"?%s\"></wsse:PolicyReference>"
//				"<wsse:PolicyReference URI=\"?id=507&amp;tw=40&amp;fs=1&amp;kpp=1&amp;kv=7&amp;ver=2.1.6000.1&amp;rn=2ymvt7yu\"></wsse:PolicyReference>"
			"</wst:RequestSecurityToken>"
			"<wst:RequestSecurityToken Id=\"RST2\">"
				"<wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>"
				"<wsp:AppliesTo>"
					"<wsa:EndpointReference>"
						"<wsa:Address>contacts.msn.com</wsa:Address>"
					"</wsa:EndpointReference>"
				"</wsp:AppliesTo>"
//				"<wsse:PolicyReference URI=\"?%s\"></wsse:PolicyReference>"
				"<wsse:PolicyReference URI=\"?fs=1&amp;id=24000&amp;kv=7&amp;rn=93S9SWWw&amp;tw=0&amp;ver=2.1.6000.1\"></wsse:PolicyReference>"
			"</wst:RequestSecurityToken>"
//			"<wst:RequestSecurityToken Id=\"RST3\">"
//				"<wst:RequestType>http://schemas.xmlsoap.org/ws/2004/04/security/trust/Issue</wst:RequestType>"
//				"<wsp:AppliesTo>"
//					"<wsa:EndpointReference>"
//						"<wsa:Address>voice.messenger.msn.com</wsa:Address>"
//					"</wsa:EndpointReference>"
//				"</wsp:AppliesTo>"
//				"<wsse:PolicyReference URI=\"?id=69264\"></wsse:PolicyReference>"
//			"</wst:RequestSecurityToken>"
		"</ps:RequestMultipleSecurityTokens>"
	"</Body>"
"</Envelope>";

char pAuthToken[256], tAuthToken[256]; 
char pContAuthToken[256], tContAuthToken[256]; 


/////////////////////////////////////////////////////////////////////////////////////////
// Performs the MSN Passport login via TLS

int MSN_GetPassportAuth( char* authChallengeInfo )
{
	int retVal = -1;
	SSLAgent mAgent;

	char szPassword[ 100 ];
	MSN_GetStaticString( "Password", NULL, szPassword, sizeof( szPassword ));
	MSN_CallService( MS_DB_CRYPT_DECODESTRING, strlen( szPassword )+1, ( LPARAM )szPassword );
	szPassword[ 16 ] = 0;
	char* szEncPassword = HtmlEncode(szPassword);

	// Replace ',' with '&' 
	char *p = authChallengeInfo;
	while( *p != 0 ) {
		if ( *p == ',' ) *p = '&';
		++p;
	}
	char* szEncAuthInfo = HtmlEncode(authChallengeInfo);

	const size_t len = sizeof(authPacket) + 2048;
	char* szAuthInfo = ( char* )alloca( len );
	mir_snprintf( szAuthInfo, len, authPacket, MyOptions.szEmail, szEncPassword, szEncAuthInfo, szEncAuthInfo );

	mir_free( szEncPassword );
	mir_free( szEncAuthInfo );

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
						const char* parResult = ezxml_txt(toks);
						
						char *pRef = NULL, *tRef = NULL;
						if (strcmp(id, "PPToken1")==0)
						{
							pRef = pAuthToken;
							tRef = tAuthToken;
						}
						else if (strcmp(id, "PPToken2")==0)
						{
							pRef = pContAuthToken;
							tRef = tContAuthToken;
						}
						if (pRef != NULL)
						{
							txtParseParam(parResult, NULL, "t=", "&p=", tRef, sizeof(tAuthToken));
							txtParseParam(parResult, NULL, "&p=", NULL, pRef, sizeof(pAuthToken));
							retVal = 0;
						}
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

