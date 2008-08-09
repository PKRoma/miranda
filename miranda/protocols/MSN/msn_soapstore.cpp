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
#include "msn_proto.h"

static const char storeReqHdr[] = 
	"SOAPAction: http://www.msn.com/webservices/storage/w10/%s\r\n";

ezxml_t CMsnProto::storeSoapHdr(const char* service, const char* scenario, ezxml_t& tbdy, char*& httphdr)
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

char* CMsnProto::GetStoreHost(const char* service)
{
	char hostname[128];
	mir_snprintf(hostname, sizeof(hostname), "StoreHost-%s", service); 

	char* host = (char*)mir_alloc(256);
	if (getStaticString(NULL, hostname, host, 256))
		strcpy(host, "https://tkrdr.storage.msn.com/storageservice/SchematizedStore.asmx");

	return host;
}

void CMsnProto::UpdateStoreHost(const char* service, const char* url)
{
	char hostname[128];
	mir_snprintf(hostname, sizeof(hostname), "StoreHost-%s", service); 

	setString(NULL, hostname, url);
}

bool CMsnProto::MSN_StoreCreateProfile(void)
{
	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = storeSoapHdr("CreateProfile", "RoamingIdentityChanged", tbdy, reqHdr);

	ezxml_t pro = ezxml_add_child(tbdy, "profile", 0);
	ezxml_t node;

	pro = ezxml_add_child(pro, "ExpressionProfile", 0);
	ezxml_add_child(pro, "PersonalStatus", 0);
	node = ezxml_add_child(pro, "RoleDefinitionName", 0);
	ezxml_set_txt(node, "ExpressionProfileDefault");

	char* szData = ezxml_toxml(xmlp, true);

	ezxml_free(xmlp);

	unsigned status;

	char* storeUrl = GetStoreHost("CreateProfile");
	char* tResult = getSslResult(&storeUrl, szData, reqHdr, status);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL) UpdateStoreHost("CreateProfile", storeUrl);

	mir_free(tResult);
	mir_free(storeUrl);

	return status == 200;
}

bool CMsnProto::MSN_StoreGetProfile(void)
{
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

	char* storeUrl = GetStoreHost("GetProfile");
	char* tResult = getSslResult(&storeUrl, szData, reqHdr, status);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL) UpdateStoreHost("GetProfile", storeUrl);

	if (tResult != NULL && status == 200)
	{
		ezxml_t xmlm = ezxml_parse_str(tResult, strlen(tResult));
		ezxml_t resxml = ezxml_get(xmlm, "soap:Body", 0, "GetProfileResponse", 0, 
			"GetProfileResult", -1);
		
		mir_snprintf(proresid, sizeof(proresid), "%s", ezxml_txt(ezxml_child(resxml, "ResourceID")));

		ezxml_t expr = ezxml_child(resxml, "ExpressionProfile");
		if (expr == NULL) 
			MSN_StoreCreateProfile();
		else
		{
			if (!getByte( "NeverUpdateNickname", 0 ))
			{
				const char* szNick = ezxml_txt(ezxml_child(expr, "DisplayName"));
				setStringUtf(NULL, "Nick", (char*)szNick);
			}
		}
		ezxml_free(xmlm);
	}
	else if (tResult != NULL && status == 500)
	{
		MSN_StoreCreateProfile();
	}

	mir_free(tResult);
	mir_free(storeUrl);

	return status == 200;
}

bool CMsnProto::MSN_StoreUpdateNick(const char* szNick)
{
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

	char* storeUrl = GetStoreHost("UpdateProfile");
	char* tResult = getSslResult(&storeUrl, szData, reqHdr, status);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL) UpdateStoreHost("UpdateProfile", storeUrl);

	if (tResult != NULL && status == 200)
	{
		MSN_ABUpdateDynamicItem();
	}
	mir_free(tResult);
	mir_free(storeUrl);

	return status == 200;
}


bool CMsnProto::MSN_StoreCreateRelationships(const char *szSrcId, const char *szTgtId)
{
	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = storeSoapHdr("CreateRelationships", "RoamingIdentityChanged", tbdy, reqHdr);

	ezxml_t rels = ezxml_add_child(tbdy, "relationships", 0);
	ezxml_t rel = ezxml_add_child(rels, "Relationship", 0);
	ezxml_t node = ezxml_add_child(rel, "SourceID", 0);
	ezxml_set_txt(node, szSrcId);
	node = ezxml_add_child(rel, "SourceType", 0);
	ezxml_set_txt(node, "SubProfile");
	node = ezxml_add_child(rel, "TargetID", 0);
	ezxml_set_txt(node, szTgtId);
	node = ezxml_add_child(rel, "TargetType", 0);
	ezxml_set_txt(node, "Photo");
	node = ezxml_add_child(rel, "RelationshipName", 0);
	ezxml_set_txt(node, "ProfilePhoto");

	char* szData = ezxml_toxml(xmlp, true);

	ezxml_free(xmlp);

	unsigned status;

	char* storeUrl = GetStoreHost("CreateRelationships");
	char* tResult = getSslResult(&storeUrl, szData, reqHdr, status);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL) UpdateStoreHost("CreateRelationships", storeUrl);

	mir_free(tResult);
	mir_free(storeUrl);

	return status == 200;
}


bool CMsnProto::MSN_StoreDeleteRelationships(const char *szResId)
{
	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = storeSoapHdr("DeleteRelationships", "RoamingIdentityChanged", tbdy, reqHdr);

	ezxml_t srch = ezxml_add_child(tbdy, "sourceHandle", 0);
	ezxml_t node = ezxml_add_child(srch, "RelationshipName", 0);
	ezxml_set_txt(node, "/UserTiles");

	ezxml_t alias = ezxml_add_child(srch, "Alias", 0);
	node = ezxml_add_child(alias, "Name", 0);
	ezxml_set_txt(node, mycid);
	node = ezxml_add_child(alias, "NameSpace", 0);
	ezxml_set_txt(node, "MyCidStuff");

	node = ezxml_add_child(tbdy, "targetHandles", 0);
	node = ezxml_add_child(node, "ObjectHandle", 0);
	node = ezxml_add_child(node, "ResourceID", 0);
	ezxml_set_txt(node, szResId);

	char* szData = ezxml_toxml(xmlp, true);

	ezxml_free(xmlp);

	unsigned status;

	char* storeUrl = GetStoreHost("DeleteRelationships");
	char* tResult = getSslResult(&storeUrl, szData, reqHdr, status);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL) UpdateStoreHost("DeleteRelationships", storeUrl);

	mir_free(tResult);
	mir_free(storeUrl);

	return status == 200;
}


bool CMsnProto::MSN_StoreCreateDocument(const char *szName, const char *szMimeType, const char *szPicData)
{
	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = storeSoapHdr("CreateDocument", "RoamingIdentityChanged", tbdy, reqHdr);

	ezxml_t hndl = ezxml_add_child(tbdy, "parentHandle", 0);
	ezxml_t node = ezxml_add_child(hndl, "RelationshipName", 0);
	ezxml_set_txt(node, "/UserTiles");

	ezxml_t alias = ezxml_add_child(hndl, "Alias", 0);
	node = ezxml_add_child(alias, "Name", 0);
	ezxml_set_txt(node, mycid);
	node = ezxml_add_child(alias, "NameSpace", 0);
	ezxml_set_txt(node, "MyCidStuff");

	ezxml_t doc = ezxml_add_child(tbdy, "document", 0);
	ezxml_set_attr(doc, "xsi:type", "Photo");
	node = ezxml_add_child(doc, "Name", 0);
	ezxml_set_txt(node, szName);

	doc = ezxml_add_child(tbdy, "DocumentStreams", 0);
	doc = ezxml_add_child(tbdy, "DocumentStream", 0);
	ezxml_set_attr(doc, "xsi:type", "PhotoStream");
	node = ezxml_add_child(doc, "DocumentStreamType", 0);

	ezxml_set_txt(node, "UserTileStatic");
	node = ezxml_add_child(doc, "MimeType", 0);
	ezxml_set_txt(node, szMimeType);
	node = ezxml_add_child(doc, "Data", 0);
	ezxml_set_txt(node, szPicData);
	node = ezxml_add_child(doc, "DataSize", 0);
	ezxml_set_txt(node, "0");

	node = ezxml_add_child(tbdy, "relationshipName", 0);
	ezxml_set_txt(node, "Messenger User Tile");

	char* szData = ezxml_toxml(xmlp, true);

	ezxml_free(xmlp);

	unsigned status;

	char* storeUrl = GetStoreHost("CreateDocument");
	char* tResult = getSslResult(&storeUrl, szData, reqHdr, status);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL) UpdateStoreHost("CreateDocument", storeUrl);

	mir_free(tResult);
	mir_free(storeUrl);

	return status == 200;
}


bool CMsnProto::MSN_StoreFindDocuments(void)
{
	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = storeSoapHdr("FindDocuments", "RoamingIdentityChanged", tbdy, reqHdr);

	ezxml_t srch = ezxml_add_child(tbdy, "objectHandle", 0);
	ezxml_t node = ezxml_add_child(srch, "RelationshipName", 0);
	ezxml_set_txt(node, "/UserTiles");

	ezxml_t alias = ezxml_add_child(srch, "Alias", 0);
	node = ezxml_add_child(alias, "Name", 0);
	ezxml_set_txt(node, mycid);
	node = ezxml_add_child(alias, "NameSpace", 0);
	ezxml_set_txt(node, "MyCidStuff");

	ezxml_t doc = ezxml_add_child(tbdy, "documentAttributes", 0);
	node = ezxml_add_child(doc, "ResourceID", 0);
	ezxml_set_txt(node, "true");
	node = ezxml_add_child(doc, "Name", 0);
	ezxml_set_txt(node, "true");

	doc = ezxml_add_child(tbdy, "documentFilter", 0);
	node = ezxml_add_child(doc, "FilterAttributes", 0);
	ezxml_set_txt(node, "None");

	doc = ezxml_add_child(tbdy, "documentSort", 0);
	node = ezxml_add_child(doc, "SortBy", 0);
	ezxml_set_txt(node, "DateModified");

	doc = ezxml_add_child(tbdy, "findContext", 0);
	node = ezxml_add_child(doc, "FindMethod", 0);
	ezxml_set_txt(node, "Default");
	node = ezxml_add_child(doc, "ChunkSize", 0);
	ezxml_set_txt(node, "25");

	char* szData = ezxml_toxml(xmlp, true);

	ezxml_free(xmlp);

	unsigned status;

	char* storeUrl = GetStoreHost("FindDocuments");
	char* tResult = getSslResult(&storeUrl, szData, reqHdr, status);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL) UpdateStoreHost("FindDocuments", storeUrl);

	mir_free(tResult);
	mir_free(storeUrl);

	return status == 200;
}
