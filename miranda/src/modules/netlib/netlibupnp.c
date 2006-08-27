/*
UPnP plugin for Miranda IM
Copyright (C) 2006 borkra

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

/* Main file for the Weather Protocol, includes loading, unloading,
   upgrading, support for plugin uninsaller, and anything that doesn't
   belong to any other file.
*/

#include "commonheaders.h"
#include "netlib.h"

static char search_request_msg[] = 
	"M-SEARCH * HTTP/1.1\r\n"
	"MX: 2\r\n"
	"HOST: 239.255.255.250:1900\r\n"
	"MAN: \"ssdp:discover\"\r\n"
	"ST: urn:schemas-upnp-org:service:%s\r\n"
	"\r\n";

static char xml_get_hdr[] =
"GET %s HTTP/1.1\r\n"
	"Connection: close\r\n"
	"Host: %s:%s\r\n\r\n";

static char soap_post_hdr[] =
	"POST %s HTTP/1.1\r\n"
	"HOST: %s:%s\r\n"
	"CONTENT-LENGTH: %d\r\n"
	"CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n"
	"SOAPACTION: \"%s#%s\"\r\n\r\n"
	"%s";

static char search_device[] = 
	"<serviceType>%s</serviceType>";

static char soap_action[] =
	"<?xml version=\"1.0\"?>\r\n"
	"<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
		"s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
	  "<s:Body>\r\n"
	    "<u:%s xmlns:u=\"%s\">\r\n"
	      "%s"
	    "</u:%s>\r\n"
	  "</s:Body>\r\n"
	"</s:Envelope>\r\n";

static char add_port_mapping[] =
	"<NewRemoteHost></NewRemoteHost>\r\n"
	"<NewExternalPort>%i</NewExternalPort>\r\n"
	"<NewProtocol>%s</NewProtocol>\r\n"
	"<NewInternalPort>%i</NewInternalPort>\r\n"
	"<NewInternalClient>%s</NewInternalClient>\r\n"
	"<NewEnabled>1</NewEnabled>\r\n"
	"<NewPortMappingDescription>Miranda</NewPortMappingDescription>\r\n"
	"<NewLeaseDuration>0</NewLeaseDuration>\r\n";

static char delete_port_mapping[] =
	"<NewRemoteHost></NewRemoteHost>\r\n"
	"<NewExternalPort>%i</NewExternalPort>\r\n"
	"<NewProtocol>%s</NewProtocol>\r\n";

static char default_http_port[] = "80";

static char http_ok[] = "200 OK";
static char http_dupport[] = "HTTP/1.1 718";

static BOOL gatewayFound = FALSE;
static SOCKADDR_IN locIP;
static time_t lastDiscTime = 0;
static int expireTime = 120;

static char szCtlUrl[256], szDev[256];


static BOOL txtParseParam(char* szData, char* presearch, 
						  char* start, char* finish, char* param, int size)
{
	char *cp, *cp1;
	int len;
	
	*param = 0;

	if (presearch != NULL)
	{
		cp1 = strstr(szData, presearch);
		if (cp1 == NULL) return FALSE;
	}
	else
		cp1 = szData;

	cp = strstr(cp1, start);
	if (cp == NULL) return FALSE;
	cp += strlen(start);
	while (*cp == ' ') ++cp;

	cp1 = strstr(cp, finish);
	if (cp1 == NULL) return FALSE;
	while (*(cp1-1) == ' ' && cp1 > cp) --cp1;

	len = min(cp1 - cp, size);
	strncpy(param, cp, len);
	param[len] = 0;

	return TRUE;
}

static LongLog(char* szData)
{
	char* buf = szData;
	int sz = strlen(szData);

	while ( sz > 1000)
	{
		char* nbuf = buf + 1000;
		char t = *nbuf;
		*nbuf = 0;
		Netlib_Logf(NULL, buf);
		*nbuf = t;
		buf = nbuf;
		sz -= 1000;
	}
	Netlib_Logf(NULL, buf);
}




static void discoverUPnP(char* szUrl, int sizeUrl)
{
	char* buf;
	int buflen;
	unsigned i, j, nip = 0;
	char* szData = NULL;
	unsigned* ips = NULL;

	static const unsigned any = INADDR_ANY;
	fd_set readfd;
	TIMEVAL tv = { 1, 0 };

	char hostname[256];
	PHOSTENT he;

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	SOCKADDR_IN enetaddr;
	enetaddr.sin_family = AF_INET;
	enetaddr.sin_port = htons(1900);
	enetaddr.sin_addr.s_addr = inet_addr("239.255.255.250");

	FD_ZERO(&readfd);
	FD_SET(sock, &readfd);

	szUrl[0] = 0;

	gethostname( hostname, sizeof( hostname ));
	he = gethostbyname( hostname );

	if (he)
	{
		while(he->h_addr_list[nip]) ++nip;

		ips = mir_alloc(nip * sizeof(unsigned));

		for (j=0; j<nip; ++j)
			ips[j] = *(unsigned*)he->h_addr_list[j];
	}

	buf = mir_alloc(1500);

	for(i = 3;  --i && szUrl[0] == 0;) 
	{
		for (j=0; j<nip; ++j)
		{
			if (ips)
				setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&ips[j], sizeof(unsigned));

			buflen = mir_snprintf(buf, 1500, search_request_msg, "WANIPConnection:1");
			sendto(sock, buf, buflen, 0, (SOCKADDR*)&enetaddr, sizeof(enetaddr)); 
			LongLog(buf);

			buflen = mir_snprintf(buf, 1500, search_request_msg, "WANPPPConnection:1");
			sendto(sock, buf, buflen, 0, (SOCKADDR*)&enetaddr, sizeof(enetaddr)); 
			LongLog(buf);
		}

		while (select(0, &readfd, NULL, NULL, &tv) == 1) 
		{
			buflen = recv(sock, buf, 1500, 0);
			if (buflen != SOCKET_ERROR) 
			{
				buf[buflen] = 0;
				LongLog(buf);

				if (txtParseParam(buf, NULL, "LOCATION:", "\r", szUrl, sizeUrl))
				{
					char age[30];
					txtParseParam(szUrl, NULL, "http://", "/", szCtlUrl, sizeof(szCtlUrl));
					txtParseParam(buf, NULL, "ST:", "\r", szDev, sizeof(szDev));
					txtParseParam(buf, "max-age", "=", "\r", age, sizeof(age));
					expireTime = atoi(age);
					break;
				}
			}
		}
	}

	mir_free(buf);
	mir_free(ips);
	setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&any, sizeof(unsigned));
	closesocket(sock);
}


static void httpTransact (char* szUrl, char* szResult, int resSize, char* szActionName)
{
	// Parse URL
	char *ppath, *phost, *pport, *szHost, *szPort;
	int sz;

	char* szData = mir_alloc(4096);

	phost = strstr(szUrl,"://");
	if (phost == NULL) phost = szUrl;
	else phost += 3;
	
	ppath = strchr(phost,'/');
	if (ppath == NULL) ppath = phost + strlen(phost);
	
	pport = strchr(phost,':');
	if (pport == NULL) pport = ppath;

	sz = pport - phost + 1;
	szHost = _alloca(sz);
	strncpy(szHost, phost, sz);
	szHost[sz-1] = 0;

	sz = ppath - pport;
	if (sz > 1)
	{
		szPort = _alloca(sz);
		strncpy(szPort, pport+1, sz);
		szPort[sz-1] = 0;
	}
	else
		szPort = default_http_port;

	if (szActionName == NULL) 
		sz = mir_snprintf (szData, 4096,
			xml_get_hdr, ppath, szHost, szPort);
	else
	{
		char szData1[1024];
		
		sz = mir_snprintf (szData1, sizeof(szData1),
			soap_action, szActionName, szDev, szResult, szActionName);

		sz = mir_snprintf (szData, 4096,
			soap_post_hdr, ppath, szHost, szPort, 
			sz, szDev, szActionName, szData1);
	}

	szResult[0] = 0;

	{
		static TIMEVAL tv = { 3, 0 };
		fd_set readfd;

		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		SOCKADDR_IN enetaddr;
		enetaddr.sin_family = AF_INET;
		enetaddr.sin_port = htons((unsigned short)atol(szPort));
		enetaddr.sin_addr.s_addr = inet_addr(szHost);

		Netlib_Logf(NULL, "UPnP HTTP connection Host: %s Port: %s\n", szHost, szPort); 

		FD_ZERO(&readfd);
		FD_SET(sock, &readfd);

		if (connect(sock, (SOCKADDR*)&enetaddr, sizeof(enetaddr)) == 0)
		{
			if (send( sock, szData, sz, 0 ) != SOCKET_ERROR)
			{
				LongLog(szData);
				sz = 0;
				for(;;) 
				{
					int bytesRecv;

					if (select(0, &readfd, NULL, NULL, &tv) != 1) break;

					bytesRecv = recv( sock, &szResult[sz], resSize-sz, 0 );
					if ( bytesRecv == 0 || bytesRecv == WSAECONNRESET ||
							bytesRecv == SOCKET_ERROR) 
						break;
					else
					{
						sz += bytesRecv;
					}
					if (sz >= (resSize-1)) 
					{
						szResult[resSize-1] = 0;
						break;
					}
					else
						szResult[sz] = 0;
				}
				LongLog(szResult);
			}
			else
				Netlib_Logf(NULL, "UPnP send failed %d", WSAGetLastError()); 
		}
		else
			Netlib_Logf(NULL, "UPnP connect failed %d", WSAGetLastError()); 

		if (szActionName == NULL) 
		{
			int len = sizeof(locIP);
			getsockname(sock, (SOCKADDR*)&locIP, &len);
		}

		closesocket(sock);
	}

	mir_free(szData);
}


static void findUPnPGateway(void)
{
	time_t curTime = time(NULL);

	if ((curTime - lastDiscTime) >= expireTime)
	{
		char szUrl[256];
		char* szData = mir_alloc(8192);

		lastDiscTime = curTime;

		discoverUPnP(szUrl, sizeof(szUrl));
		if (szUrl[0] != 0)
		{
			httpTransact(szUrl, szData, 8192, NULL);
			gatewayFound = strstr(szData, http_ok) != NULL;
		}
		else
			gatewayFound = FALSE;

		if (gatewayFound)
		{
			char szTemp[256];
			size_t ctlLen;

			txtParseParam(szData, NULL, "<URLBase>", "</URLBase>", szTemp, sizeof(szTemp));
			if (szTemp[0] != 0) strcpy(szCtlUrl, szTemp);
			ctlLen = strlen(szCtlUrl);
			if (ctlLen > 0 && szCtlUrl[ctlLen-1] == '/')
				szCtlUrl[--ctlLen] = 0;

			mir_snprintf(szTemp, sizeof(szTemp), search_device, szDev);
			txtParseParam(szData, szTemp, "<controlURL>", "</controlURL>", szUrl, sizeof(szUrl));
			switch (szUrl[0])
			{
				case 0:
					gatewayFound = FALSE;
					break;

				case '/': 
					strncat(szCtlUrl, szUrl, sizeof(szCtlUrl) - ctlLen);
					szCtlUrl[sizeof(szCtlUrl)-1] = 0;
					break;

				default: 
					strncpy(szCtlUrl, szUrl, sizeof(szCtlUrl));
					szCtlUrl[sizeof(szCtlUrl)-1] = 0;
					break;
			}
		}
		Netlib_Logf(NULL, "UPnP Gateway detected %d, Control URL: %s\n", gatewayFound, szCtlUrl); 
		mir_free(szData);
	}
}


BOOL NetlibUPnPAddPortMapping(WORD intport, char *proto, 
							  WORD *extport, DWORD *extip, BOOL search)
{
	BOOL res = FALSE;

	findUPnPGateway();

	if (gatewayFound)
	{
		char* szData = mir_alloc(4096);
		char szExtIP[30];
		*extport = intport - 1;

		do {
			++*extport;
			mir_snprintf(szData, 4096, add_port_mapping, 
				*extport, proto, intport, inet_ntoa(locIP.sin_addr));
			httpTransact(szCtlUrl, szData, 4096, "AddPortMapping");
		} while (search && strstr(szData, http_dupport) != NULL);
		
		res = strstr(szData, http_ok) != NULL;

		if (res)
		{
			szData[0] = 0;
			httpTransact(szCtlUrl, szData, 4096, "GetExternalIPAddress");
			txtParseParam(szData, "<NewExternalIPAddress", ">", "<", szExtIP, sizeof(szExtIP));

			res &= strstr(szData, http_ok) != NULL;
			*extip = ntohl(res ? inet_addr(szExtIP) : locIP.sin_addr.S_un.S_addr);
		}

		mir_free(szData);
	}

	return res;
}


void NetlibUPnPDeletePortMapping(WORD extport, char* proto)
{
	if (extport != 0)
	{
		findUPnPGateway();

		if (gatewayFound)
		{
			char* szData = mir_alloc(4096);
			
			mir_snprintf(szData, 4096, delete_port_mapping, 
				extport, proto);
			httpTransact(szCtlUrl, szData, 4096, "DeletePortMapping");

			mir_free(szData);
		}
	}
}
