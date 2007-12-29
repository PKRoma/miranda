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

extern char *authStorageToken;
extern char mycid[], mypuid[];

static char proresid[64];

static const char storeReqHdr[] = 
	"SOAPAction: http://www.msn.com/webservices/storage/w10/%s\r\n";

static ezxml_t storeSoapHdr(const char* service, const char* scenario, ezxml_t& tbdy, char*& httphdr)
{
	ezxml_t xmlp = ezxml_new("soap:Envelope");
	ezxml_set_attr(xmlp, "xmlns:soap", "http://schemas.xmlsoap.org/soap/envelope/");
	ezxml_set_attr(xmlp, "xmlns:xsi",  "http://www.w3.org/2001/XMLSchema-instance");
	ezxml_set_attr(xmlp, "xmlns:xsd",  "http://www.w3.org/2001/XMLSchema");
	ezxml_set_attr(xmlp, "xmlns:soapenc", "http://schemas.xmlsoap.org/soap/encoding/");
	
	ezxml_t hdr = ezxml_add_child(xmlp, "soap:Header", 0);

//	ezxml_t cachehdr = ezxml_add_child(hdr, "AffinityCacheHeader", 0);
//	ezxml_set_attr(cachehdr, "xmlns", "http://www.msn.com/webservices/storage/w10");
//	ezxml_t node = ezxml_add_child(cachehdr, "CacheKey", 0);
//	ezxml_set_txt(node, cachekey);

	ezxml_t apphdr = ezxml_add_child(hdr, "StorageApplicationHeader", 0);
	ezxml_set_attr(apphdr, "xmlns", "http://www.msn.com/webservices/storage/w10");
	ezxml_t node = ezxml_add_child(apphdr, "ApplicationID", 0);
	ezxml_set_txt(node, "Messenger Client 8.5");
	node = ezxml_add_child(apphdr, "Scenario", 0);
	ezxml_set_txt(node, scenario);

	ezxml_t authhdr = ezxml_add_child(hdr, "StorageUserHeader", 0);
	ezxml_set_attr(authhdr, "xmlns", "http://www.msn.com/webservices/storage/w10");
	node = ezxml_add_child(authhdr, "Puid", 0);
	ezxml_set_txt(node, mypuid);
	node = ezxml_add_child(authhdr, "TicketToken", 0);
	if (authStorageToken) ezxml_set_txt(node, authStorageToken);

	ezxml_t bdy = ezxml_add_child(xmlp, "soap:Body", 0);
	
	tbdy = ezxml_add_child(bdy, service, 0);
	ezxml_set_attr(tbdy, "xmlns", "http://www.msn.com/webservices/storage/w10");

	size_t hdrsz = strlen(service) + sizeof(storeReqHdr) + 20;
	httphdr = (char*)mir_alloc(hdrsz);

	mir_snprintf(httphdr, hdrsz, storeReqHdr, service);

	return xmlp;
}

 static char* GetStoreHost(void)
{
	char host[128];
	if (MSN_GetStaticString("MsnStoreHost", NULL, host, sizeof(host)))
		strcpy(host, "storage.msn.com");
//		strcpy(host, "tkrdr.storage.msn.com");

	char* fullhost = (char*)mir_alloc(256);
	mir_snprintf(fullhost, 256, "https://%s/storageservice/SchematizedStore.asmx", host);

	return fullhost;
}

bool MSN_StoreGetProfile(void)
{
	SSLAgent mAgent;

	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = storeSoapHdr("GetProfile", "Initial", tbdy, reqHdr);

	ezxml_t prohndl = ezxml_add_child(tbdy, "profileHandle", 0);

	ezxml_t alias = ezxml_add_child(prohndl, "Alias", 0);
	ezxml_t node = ezxml_add_child(alias, "Name", 0);
	ezxml_set_txt(node, mycid);
	node = ezxml_add_child(alias, "NameSpace", 0);
	ezxml_set_txt(node, "MyCidStuff");

	node = ezxml_add_child(prohndl, "RelationshipName", 0);
	ezxml_set_txt(node, "MyProfile");

	ezxml_t proattr = ezxml_add_child(tbdy, "profileAttributes", 0);
	node = ezxml_add_child(proattr, "ResourceID", 0);
	ezxml_set_txt(node, "true");
	node = ezxml_add_child(proattr, "DateModified", 0);
	ezxml_set_txt(node, "true");

	ezxml_t exproattr = ezxml_add_child(proattr, "ExpressionProfileAttributes", 0);
	node = ezxml_add_child(exproattr, "ResourceID", 0);
	ezxml_set_txt(node, "true");
	node = ezxml_add_child(exproattr, "DateModified", 0);
	ezxml_set_txt(node, "true");
	node = ezxml_add_child(exproattr, "DisplayName", 0);
	ezxml_set_txt(node, "true");
	node = ezxml_add_child(exproattr, "DisplayNameLastModified", 0);
	ezxml_set_txt(node, "true");
	node = ezxml_add_child(exproattr, "PersonalStatus", 0);
	ezxml_set_txt(node, "true");
	node = ezxml_add_child(exproattr, "PersonalStatusLastModified", 0);
	ezxml_set_txt(node, "true");
	node = ezxml_add_child(exproattr, "StaticUserTilePublicURL", 0);
	ezxml_set_txt(node, "true");
	node = ezxml_add_child(exproattr, "Photo", 0);
	ezxml_set_txt(node, "true");
	node = ezxml_add_child(exproattr, "Flags", 0);
	ezxml_set_txt(node, "true");

	char* szData = ezxml_toxml(xmlp, true);

	ezxml_free(xmlp);

	unsigned status;
	char* htmlbody;

	char* storeUrl = GetStoreHost();
	char* tResult = mAgent.getSslResult(storeUrl, szData, reqHdr, status, htmlbody);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL && status == 200)
	{
		ezxml_t xmlm = ezxml_parse_str(htmlbody, strlen(htmlbody));
		ezxml_t resxml = ezxml_get(xmlm, "soap:Body", 0, "GetProfileResponse", 0, 
			"GetProfileResult", -1);
		
		mir_snprintf(proresid, sizeof(proresid), "%s", ezxml_txt(ezxml_child(resxml, "ResourceID")));

		ezxml_t expr = ezxml_child(resxml, "ExpressionProfile");

		if (!MSN_GetByte( "NeverUpdateNickname", 0 ))
		{
			const char* szNick = ezxml_txt(ezxml_child(expr, "DisplayName"));
			MSN_SetStringUtf(NULL, "Nick", (char*)szNick);
		}

		ezxml_free(xmlm);
	}
	mir_free(tResult);
	mir_free(storeUrl);

	return status == 200;
}

bool MSN_StoreUpdateNick(const char* szNick)
{
	SSLAgent mAgent;

	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = storeSoapHdr("UpdateProfile", "RoamingIdentityChanged", tbdy, reqHdr);

	ezxml_t pro = ezxml_add_child(tbdy, "profile", 0);
	ezxml_t node = ezxml_add_child(pro, "ResourceID", 0);
	ezxml_set_txt(node, proresid);

	ezxml_t expro = ezxml_add_child(pro, "ExpressionProfile", 0);
	node = ezxml_add_child(expro, "FreeText", 0);
	ezxml_set_txt(node, "Update");
	node = ezxml_add_child(expro, "DisplayName", 0);
	ezxml_set_txt(node, szNick);
	node = ezxml_add_child(expro, "Flags", 0);
	ezxml_set_txt(node, "0");

	char* szData = ezxml_toxml(xmlp, true);

	ezxml_free(xmlp);

	unsigned status;
	char* htmlbody;

	char* storeUrl = GetStoreHost();
	char* tResult = mAgent.getSslResult(storeUrl, szData, reqHdr, status, htmlbody);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL && status == 200)
	{
		MSN_ABUpdateDynamicItem();
	}
	mir_free(tResult);
	mir_free(storeUrl);

	return status == 200;
}
