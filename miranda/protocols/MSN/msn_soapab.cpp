/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2007-2009 Boris Krasnovskiy.

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

static const char abReqHdr[] = 
	"SOAPAction: http://www.msn.com/webservices/AddressBook/%s\r\n";


ezxml_t CMsnProto::abSoapHdr(const char* service, const char* scenario, ezxml_t& tbdy, char*& httphdr)
{
	ezxml_t xmlp = ezxml_new("soap:Envelope");
	ezxml_set_attr(xmlp, "xmlns:soap", "http://schemas.xmlsoap.org/soap/envelope/");
	ezxml_set_attr(xmlp, "xmlns:xsi",  "http://www.w3.org/2001/XMLSchema-instance");
	ezxml_set_attr(xmlp, "xmlns:xsd",  "http://www.w3.org/2001/XMLSchema");
	ezxml_set_attr(xmlp, "xmlns:soapenc", "http://schemas.xmlsoap.org/soap/encoding/");
	
	ezxml_t hdr = ezxml_add_child(xmlp, "soap:Header", 0);
	ezxml_t apphdr = ezxml_add_child(hdr, "ABApplicationHeader", 0);
	ezxml_set_attr(apphdr, "xmlns", "http://www.msn.com/webservices/AddressBook");
	ezxml_t node = ezxml_add_child(apphdr, "ApplicationId", 0);
	ezxml_set_txt(node, "CFE80F9D-180F-4399-82AB-413F33A1FA11");
	node = ezxml_add_child(apphdr, "IsMigration", 0);
	ezxml_set_txt(node, ((abchMigrated == NULL || *abchMigrated == '1') ? "false" : "true"));
	node = ezxml_add_child(apphdr, "PartnerScenario", 0);
	ezxml_set_txt(node, scenario);

	ezxml_t authhdr = ezxml_add_child(hdr, "ABAuthHeader", 0);
	ezxml_set_attr(authhdr, "xmlns", "http://www.msn.com/webservices/AddressBook");
	node = ezxml_add_child(authhdr, "ManagedGroupRequest", 0);
	ezxml_set_txt(node, "false");
	node = ezxml_add_child(authhdr, "TicketToken", 0);
	if (authContactToken) ezxml_set_txt(node, authContactToken);

	ezxml_t bdy = ezxml_add_child(xmlp, "soap:Body", 0);
	
	tbdy = ezxml_add_child(bdy, service, 0);
	ezxml_set_attr(tbdy, "xmlns", "http://www.msn.com/webservices/AddressBook");

	if (strstr(service, "Member") == NULL && strcmp(service, "ABAdd") != 0)
	{
		ezxml_t node = ezxml_add_child(tbdy, "abId", 0);
		ezxml_set_txt(node, "00000000-0000-0000-0000-000000000000");
	}

	size_t hdrsz = strlen(service) + sizeof(abReqHdr) + 20;
	httphdr = (char*)mir_alloc(hdrsz);

	mir_snprintf(httphdr, hdrsz, abReqHdr, service);

	return xmlp;
}


ezxml_t CMsnProto::getSoapResponse(ezxml_t bdy, const char* service)
{
    char resp[40], res[40];
    mir_snprintf(resp, sizeof(resp), "%sResponse", service);
    mir_snprintf(res, sizeof(res), "%sResult", service);

	return ezxml_get(bdy, "soap:Body", 0, resp, 0, res, -1); 
}

ezxml_t CMsnProto::getSoapFault (ezxml_t bdy, bool err)
{
	ezxml_t flt = ezxml_get(bdy, "soap:Body", 0, "soap:Fault", -1);
    return err ? ezxml_get(flt, "detail", 0, "errorcode", -1) : flt;
}

void CMsnProto::UpdateABHost(const char* service, const char* url)
{
	char hostname[128];
	mir_snprintf(hostname, sizeof(hostname), "ABHost-%s", service); 

	setString(NULL, hostname, url);
}

char* CMsnProto::GetABHost(const char* service, bool isSharing)
{
	char hostname[128];
	mir_snprintf(hostname, sizeof(hostname), "ABHost-%s", service); 

	char* host = (char*)mir_alloc(256);
	if (getStaticString(NULL, hostname, host, 256))
	{
		mir_snprintf(host, 256, "https://byrdr.omega.contacts.msn.com/abservice/%s.asmx", 
			isSharing ? "SharingService" : "abservice");
	}

	return host;
}


bool CMsnProto::MSN_ABAdd(void)
{
	char* reqHdr;
	ezxml_t tbdy, node;
	ezxml_t xmlp = abSoapHdr("ABAdd", "Timer", tbdy, reqHdr);

	ezxml_t abinf = ezxml_add_child(tbdy, "abInfo", 0);
	ezxml_add_child(abinf, "name", 0);
	node = ezxml_add_child(abinf, "ownerPuid", 0);
	ezxml_set_txt(node, "0");
	node = ezxml_add_child(abinf, "ownerEmail", 0);
	ezxml_set_txt(node, MyOptions.szEmail);
	node = ezxml_add_child(abinf, "fDefault", 0);
	ezxml_set_txt(node, "true");

	char* szData = ezxml_toxml(xmlp, true);
	ezxml_free(xmlp);

	unsigned status;

	char* abUrl = GetABHost("ABAdd", false);
	char* tResult = getSslResult(&abUrl, szData, reqHdr, status);

	mir_free(reqHdr);
	free(szData);

    if (tResult != NULL) 
    {
        UpdateABHost("ABAdd", abUrl);

		ezxml_t xmlm = ezxml_parse_str(tResult, strlen(tResult));
        if (status == 500)
        {
			const char* szErr = ezxml_txt(getSoapFault(xmlm, true));
            if (strcmp(szErr, "PassportAuthFail") == 0)
            {
                MSN_GetPassportAuth();
                status = MSN_ABAdd() ? 200 : 500;
            }
        }
        ezxml_free(xmlm);
    }

	mir_free(tResult);
	mir_free(abUrl);

	return status == 200;
}


bool CMsnProto::MSN_SharingFindMembership(void)
{
	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = abSoapHdr("FindMembership", "Initial", tbdy, reqHdr);

	ezxml_t svcflt = ezxml_add_child(tbdy, "serviceFilter", 0);
	ezxml_t tps = ezxml_add_child(svcflt, "Types", 0);
	ezxml_t node = ezxml_add_child(tps, "ServiceType", 0);
	ezxml_set_txt(node, "Messenger");
//	node = ezxml_add_child(tps, "ServiceType", 0);
//	ezxml_set_txt(node, "Invitation");
//	node = ezxml_add_child(tps, "ServiceType", 0);
//	ezxml_set_txt(node, "SocialNetwork");
//	node = ezxml_add_child(tps, "ServiceType", 0);
//	ezxml_set_txt(node, "Space");
//	node = ezxml_add_child(tps, "ServiceType", 0);
//	ezxml_set_txt(node, "Profile");

//	node = ezxml_add_child(tbdy, "View", 0);
//	ezxml_set_txt(node, "Full");
//	node = ezxml_add_child(tbdy, "deltasOnly", 0);
//	ezxml_set_txt(node, "false");
//	node = ezxml_add_child(tbdy, "lastChange", 0);
//	ezxml_set_txt(node, "0001-01-01T00:00:00.0000000-08:00");

	char* szData = ezxml_toxml(xmlp, true);

	ezxml_free(xmlp);

	unsigned status;

	char* abUrl = GetABHost("FindMembership", true);
	char* tResult = getSslResult(&abUrl, szData, reqHdr, status);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL)
	{
		UpdateABHost("FindMembership", abUrl);
		ezxml_t xmlm = ezxml_parse_str(tResult, strlen(tResult));

		if (status == 200)
		{
            ezxml_t body = getSoapResponse(xmlm, "FindMembership");
			ezxml_t svcs = ezxml_get(body, "Services", 0, "Service", -1); 
			
			while (svcs != NULL)
			{
				const char* szType = ezxml_txt(ezxml_get(svcs, "Info", 0, "Handle", 0, "Type", -1));
				if (_stricmp(szType, "Messenger") == 0) break;
				svcs = ezxml_next(svcs);
			}

			ezxml_t mems = ezxml_get(svcs, "Memberships", 0, "Membership", -1);
			
			while (mems != NULL)
			{
				const char* szRole = ezxml_txt(ezxml_child(mems, "MemberRole"));
				
				int lstId = 0;
				if (strcmp(szRole, "Allow") == 0)			lstId = LIST_AL;
				else if (strcmp(szRole, "Block") == 0)		lstId = LIST_BL;
				else if (strcmp(szRole, "Reverse") == 0)	lstId = LIST_RL;
				else if (strcmp(szRole, "Pending") == 0)	lstId = LIST_PL;

				ezxml_t memb = ezxml_get(mems, "Members", 0, "Member", -1);
				while (memb != NULL)
				{
					const char* szType = ezxml_txt(ezxml_child(memb, "Type"));
					if (strcmp(szType, "Passport") == 0)
					{
						const char* szEmail = ezxml_txt(ezxml_child(memb, "PassportName"));
						Lists_Add(lstId, 1, szEmail);
					}
					else if (strcmp(szType, "Phone") == 0)
					{
						const char* szEmail = ezxml_txt(ezxml_child(memb, "PhoneNumber"));
						char email[128];
						mir_snprintf(email, sizeof(email), "tel:%s", szEmail);
						Lists_Add(lstId, 4, email);
					}
					else if (strcmp(szType, "Email") == 0)
					{
						const char* szEmail = ezxml_txt(ezxml_child(memb, "Email"));
						int netId = strstr(szEmail, "@yahoo.com") ? NETID_YAHOO : NETID_LCS;
						ezxml_t anot = ezxml_get(memb, "Annotations", 0, "Annotation", -1);
						while (anot != NULL)
						{
							if (strcmp(ezxml_txt(ezxml_child(anot, "Name")), "MSN.IM.BuddyType") == 0)
							{
								netId = atol(ezxml_txt(ezxml_child(anot, "Value")));
								break;
							}
							anot = ezxml_next(anot);
						}
						Lists_Add(lstId, netId, szEmail);
					}
					memb = ezxml_next(memb);
				}
				mems = ezxml_next(mems);
			}
		}
		else if (status == 500)
		{
			const char* szErr = ezxml_txt(getSoapFault(xmlm, true));
			if (strcmp(szErr, "ABDoesNotExist") == 0)
			{
				MSN_ABAdd();
				status = 200;
			}
            else if (strcmp(szErr, "PassportAuthFail") == 0)
            {
                MSN_GetPassportAuth();
                status = MSN_SharingFindMembership() ? 200 : 500;
            }
		}
		ezxml_free(xmlm);
	}
	mir_free(tResult);
	mir_free(abUrl);

	return status == 200;
}

// AddMember, DeleteMember
bool CMsnProto::MSN_SharingAddDelMember(const char* szEmail, const int listId, const int netId, const char* szMethod)
{
	const char* szRole;
	if (listId & LIST_AL) szRole = "Allow";
	else if (listId & LIST_BL) szRole = "Block";
	else if (listId & LIST_PL) szRole = "Pending";
	else if (listId & LIST_RL) szRole = "Reverse";
	else return false;

	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = abSoapHdr(szMethod, "BlockUnblock", tbdy, reqHdr);

	ezxml_t svchnd = ezxml_add_child(tbdy, "serviceHandle", 0);
	ezxml_t node = ezxml_add_child(svchnd, "Id", 0);
	ezxml_set_txt(node, "0");
	node = ezxml_add_child(svchnd, "Type", 0);
	ezxml_set_txt(node, "Messenger");
	node = ezxml_add_child(svchnd, "ForeignId", 0);
//	ezxml_set_txt(node, "");

	const char* szMemberName = "";
	const char* szTypeName = "";
	const char* szAccIdName = "";

	switch (netId)
	{
		case 1: 
			szMemberName = "PassportMember";
			szTypeName = "Passport";
			szAccIdName = "PassportName";
			break;

		case 4: 
			szMemberName = "PhoneMember";
			szTypeName = "Phone";
			szAccIdName = "PhoneNumber";
			szEmail = strchr(szEmail, ':') + 1; 
			break;

		case 2: 
		case 32: 
			szMemberName = "EmailMember";
			szTypeName = "Email";
			szAccIdName = "Email";
			break;
	}

	ezxml_t memb = ezxml_add_child(tbdy, "memberships", 0);
	memb = ezxml_add_child(memb, "Membership", 0);
	node = ezxml_add_child(memb, "MemberRole", 0);
	ezxml_set_txt(node, szRole);
	memb = ezxml_add_child(memb, "Members", 0);
	memb = ezxml_add_child(memb, "Member", 0);
	ezxml_set_attr(memb, "xsi:type",  szMemberName);
	ezxml_set_attr(memb, "xmlns:xsi",  "http://www.w3.org/2001/XMLSchema-instance");
	node = ezxml_add_child(memb, "Type", 0);
	ezxml_set_txt(node, szTypeName);
	node = ezxml_add_child(memb, "State", 0);
	ezxml_set_txt(node, "Accepted");
	node = ezxml_add_child(memb, szAccIdName, 0);
	ezxml_set_txt(node, szEmail);
	
	char buf[64];
	if ((netId == NETID_LCS || netId == NETID_YAHOO) && strcmp(szMethod, "DeleteMember") != 0)
	{
		node = ezxml_add_child(memb, "Annotations", 0);
		ezxml_t anot = ezxml_add_child(node, "Annotation", 0);
		node = ezxml_add_child(anot, "Name", 0);
		ezxml_set_txt(node, "MSN.IM.BuddyType");
		node = ezxml_add_child(anot, "Value", 0);

		mir_snprintf(buf, sizeof(buf), "%02d:", netId);
		ezxml_set_txt(node, buf);
	}

	char* szData = ezxml_toxml(xmlp, true);

	ezxml_free(xmlp);

	unsigned status;

	char* abUrl = GetABHost(szMethod, true);
	char* tResult = getSslResult(&abUrl, szData, reqHdr, status);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL) 
    {
        UpdateABHost(szMethod, abUrl);
		ezxml_t xmlm = ezxml_parse_str(tResult, strlen(tResult));
		if (status == 500)
		{
			const char* szErr = ezxml_txt(getSoapFault(xmlm, true));
            if (strcmp(szErr, "PassportAuthFail") == 0)
            {
                MSN_GetPassportAuth();
                status = MSN_SharingAddDelMember(szEmail, listId, netId, szMethod) ? 200 : 500;
            }
		}
		ezxml_free(xmlm);
    }

	mir_free(tResult);
	mir_free(abUrl);

	return status == 200;
}

void CMsnProto::SetAbParam(HANDLE hContact, const char *name, const char *par)
{
	if (*par) setStringUtf(hContact, name, (char*)par);
//	else deleteSetting(hContact, "FirstName");
}

//		"ABFindAll"
bool CMsnProto::MSN_ABFind(const char* szMethod, const char* szGuid)
{
	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = abSoapHdr(szMethod, "Initial", tbdy, reqHdr);

	ezxml_t node = ezxml_add_child(tbdy, "abView", 0);
	ezxml_set_txt(node, "Full");
	node = ezxml_add_child(tbdy, "deltasOnly", 0);
	ezxml_set_txt(node, "false");
	node = ezxml_add_child(tbdy, "dynamicItemView", 0);
	ezxml_set_txt(node, "Gleam");
    
    if (szGuid)
    {
	    node = ezxml_add_child(tbdy, "contactIds", 0);
	    node = ezxml_add_child(node, "guid", 0);
    	ezxml_set_txt(node, szGuid);
    }

	char* szData = ezxml_toxml(xmlp, true);
	ezxml_free(xmlp);

	unsigned status;

	char* abUrl = GetABHost(szMethod, false);
	char* tResult = getSslResult(&abUrl, szData, reqHdr, status);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL && status == 200)
	{
		UpdateABHost(szMethod, abUrl);
		ezxml_t xmlm = ezxml_parse_str(tResult, strlen(tResult));

        if (status == 200)
        {
            ezxml_t body = getSoapResponse(xmlm, szMethod);

		    ezxml_t abinf = ezxml_get(body, "ab", 0, "abInfo", -1);
		    mir_snprintf(mycid,  sizeof(mycid), "%s", ezxml_txt(ezxml_child(abinf, "OwnerCID")));
		    mir_snprintf(mypuid, sizeof(mycid), "%s", ezxml_txt(ezxml_child(abinf, "ownerPuid")));

		    if (MyOptions.ManageServer)
		    {
			    ezxml_t grp = ezxml_get(body, "groups", 0, "Group", -1); 
			    while (grp != NULL)
			    {
				    const char* szGrpId = ezxml_txt(ezxml_child(grp, "groupId"));
				    const char* szGrpName = ezxml_txt(ezxml_get(grp, "groupInfo", 0, "name", -1));
				    MSN_AddGroup(szGrpName, szGrpId, true); 

				    grp = ezxml_next(grp);
			    }
		    }

		    ezxml_t cont = ezxml_get(body, "contacts", 0, "Contact", -1); 
		    while (cont != NULL)
		    {
			    const char* szContId = ezxml_txt(ezxml_child(cont, "contactId"));
    			
			    ezxml_t contInf = ezxml_child(cont, "contactInfo");
			    const char* szType = ezxml_txt(ezxml_child(contInf, "contactType"));

			    if (strcmp(szType, "Me") != 0)
			    {
				    char email[128];

				    const char* szEmail = ezxml_txt(ezxml_child(contInf, "passportName"));
				    const char* szMsgUsr = ezxml_txt(ezxml_child(contInf, "isMessengerUser"));
    				
				    int netId = strcmp(szMsgUsr, "true") ? NETID_EMAIL : NETID_MSN;

				    if (*szEmail == '\0')
				    {
					    ezxml_t phn = ezxml_get(contInf, "phones", 0, "ContactPhone", -1);
					    if (phn != NULL)
					    {
						    szMsgUsr = ezxml_txt(ezxml_child(phn, "isMessengerEnabled"));
						    szEmail = ezxml_txt(ezxml_child(phn, "number"));
						    mir_snprintf(email, sizeof(email), "tel:%s", szEmail);
						    szEmail = email;
						    netId = NETID_MOB;
					    }
					    else 
					    {
						    ezxml_t eml = ezxml_get(contInf, "emails", 0, "ContactEmail", -1);
						    while (eml != NULL)
						    {
							    szMsgUsr = ezxml_txt(ezxml_child(eml, "isMessengerEnabled"));
							    szEmail = ezxml_txt(ezxml_child(eml, "email"));
							    const char* szCntType = ezxml_txt(ezxml_child(eml, "contactEmailType"));
							    if (strcmp(szCntType, "Messenger2") == 0)
								    netId = NETID_YAHOO;
							    else if (strcmp(szCntType, "Messenger3") == 0)
								    netId = NETID_LCS;
							    if (strcmp(szMsgUsr, "true") == 0) break;
							    eml = ezxml_next(eml);
						    }
					    }
				    }

				    if (szEmail[0] == 0) 
					    szEmail  = ezxml_txt(ezxml_child(contInf, "quickName"));

				    Lists_Add(LIST_FL, netId, szEmail);
				    const char *szTmp;

    //				Depricated in WLM 8.1
    //				const char* szNick  = ezxml_txt(ezxml_child(contInf, "displayName"));
    //				if (*szNick == '\0') szNick = szEmail;
				    HANDLE hContact = MSN_HContactFromEmail(szEmail, szEmail, true, false);
    //				setStringUtf(hContact, "Nick", (char*)szNick);
    				
				    const char* szNick = NULL;
				    ezxml_t anot = ezxml_get(contInf, "annotations", 0, "Annotation", -1);
				    while (anot != NULL)
				    {
					    if (strcmp(ezxml_txt(ezxml_child(anot, "Name")), "AB.NickName") == 0)
					    {
						    szNick = ezxml_txt(ezxml_child(anot, "Value"));
						    DBWriteContactSettingStringUtf(hContact, "CList", "MyHandle", szNick);
					    }
					    if (strcmp(ezxml_txt(ezxml_child(anot, "Name")), "AB.JobTitle") == 0)
					    {
						    szTmp = ezxml_txt(ezxml_child(anot, "Value"));
						    SetAbParam(hContact, "CompanyPosition", szTmp);
					    }
					    anot = ezxml_next(anot);
				    }
				    if (szNick == NULL)
					    DBDeleteContactSetting(hContact, "CList", "MyHandle");

				    setString( hContact, "ID", szContId );

				    switch (netId)
				    {
				    case NETID_YAHOO:
					    setString( hContact, "Transport", "YAHOO" );
					    break;

				    case NETID_LCS:
					    setString( hContact, "Transport", "LCS" );
					    break;

				    default:
					    deleteSetting( hContact, "Transport" );
				    }

                    if (netId != NETID_EMAIL)
                    {
				        ezxml_t cgrp = ezxml_get(contInf, "groupIds", 0, "guid", -1);
				        MSN_SyncContactToServerGroup( hContact, szContId, cgrp );
                    }

				    szTmp = ezxml_txt(ezxml_child(contInf, "CID"));
				    SetAbParam(hContact, "CID", szTmp);

                    szTmp  = ezxml_txt(ezxml_child(contInf, "IsNotMobileVisible"));
				    setByte(hContact, "MobileAllowed", strcmp(szTmp, "true") != 0);

				    szTmp = ezxml_txt(ezxml_child(contInf, "isMobileIMEnabled"));
				    setByte(hContact, "MobileEnabled", strcmp(szTmp, "true") == 0);

				    szTmp = ezxml_txt(ezxml_child(contInf, "firstName"));
				    SetAbParam(hContact, "FirstName", szTmp);

				    szTmp = ezxml_txt(ezxml_child(contInf, "lastName"));
				    SetAbParam(hContact, "LastName", szTmp);

				    szTmp = ezxml_txt(ezxml_child(contInf, "birthdate"));
				    char *szPtr;
				    if (strtol(szTmp, &szPtr, 10) > 1)
				    {
					    setWord(hContact, "BirthYear", (WORD)strtol(szTmp, &szPtr, 10));
					    setByte(hContact, "BirthMonth", (BYTE)strtol(szPtr+1, &szPtr, 10));
					    setByte(hContact, "BirthDay", (BYTE)strtol(szPtr+1, &szPtr, 10));
				    }
				    else
				    {
    //					deleteSetting(hContact, "BirthYear");
    //					deleteSetting(hContact, "BirthMonth");
    //					deleteSetting(hContact, "BirthDay");
				    }

				    szTmp = ezxml_txt(ezxml_child(contInf, "comment"));
				    if (*szTmp) DBWriteContactSettingString(hContact, "UserInfo", "MyNotes", szTmp);
    //				else DBDeleteContactSetting(hContact, "UserInfo", "MyNotes");

				    ezxml_t loc = ezxml_get(contInf, "locations", 0, "ContactLocation", -1);
				    while (loc != NULL)
				    {
					    const char* szCntType = ezxml_txt(ezxml_child(loc, "contactLocationType"));
    					
					    int locid = -1;
					    if (strcmp(szCntType, "ContactLocationPersonal") == 0)
						    locid = 0;
					    else if (strcmp(szCntType, "ContactLocationBusiness") == 0)
						    locid = 1;

					    if (locid >= 0)
					    {
						    szTmp = ezxml_txt(ezxml_child(loc, "name"));
						    SetAbParam(hContact, "Company", szTmp);
						    szTmp = ezxml_txt(ezxml_child(loc, "street"));
						    SetAbParam(hContact, locid ? "CompanyStreet" : "Street", szTmp);
						    szTmp = ezxml_txt(ezxml_child(loc, "city"));
						    SetAbParam(hContact, locid ? "CompanyCity" : "City", szTmp);
						    szTmp = ezxml_txt(ezxml_child(loc, "state"));
						    SetAbParam(hContact, locid ? "CompanyState" : "State", szTmp);
						    szTmp = ezxml_txt(ezxml_child(loc, "country"));
						    SetAbParam(hContact, locid ? "CompanyCountry" : "Country", szTmp);
						    szTmp = ezxml_txt(ezxml_child(loc, "postalCode"));
						    SetAbParam(hContact, locid ? "CompanyZIP" : "ZIP", szTmp);
					    }
					    loc = ezxml_next(loc);
				    }

				    ezxml_t web = ezxml_get(contInf, "webSites", 0, "ContactWebSite", -1);
				    while (web != NULL)
				    {
					    const char* szCntType = ezxml_txt(ezxml_child(web, "contactWebSiteType"));
					    if (strcmp(szCntType, "ContactWebSiteBusiness") == 0)
					    {
						    szTmp = ezxml_txt(ezxml_child(web, "webURL"));
						    SetAbParam(hContact, "CompanyHomepage", szTmp);
					    }
					    web = ezxml_next(web);
				    }
			    }
			    else
			    {
    //              This depricated in WLM 8.1
    //				if (!getByte( "NeverUpdateNickname", 0 ))
    //				{
    //					const char* szNick  = ezxml_txt(ezxml_child(contInf, "displayName"));
    //					setStringUtf(NULL, "Nick", (char*)szNick);
    //				}
				    const char *szTmp;

				    szTmp = ezxml_txt(ezxml_child(contInf, "isMobileIMEnabled"));
				    setByte( "MobileEnabled", strcmp(szTmp, "true") == 0);
				    szTmp = ezxml_txt(ezxml_child(contInf, "IsNotMobileVisible"));
				    setByte( "MobileAllowed", strcmp(szTmp, "true") != 0);

				    ezxml_t anot = ezxml_get(contInf, "annotations", 0, "Annotation", -1);
				    while (anot != NULL)
				    {
					    if (strcmp(ezxml_txt(ezxml_child(anot, "Name")), "MSN.IM.BLP") == 0)
						    msnOtherContactsBlocked = !atol(ezxml_txt(ezxml_child(anot, "Value")));

					    anot = ezxml_next(anot);
				    }
			    }
			    cont = ezxml_next(cont);
		    }
        }
        else if (status == 500)
        {
			const char* szErr = ezxml_txt(getSoapFault(xmlm, true));
            if (strcmp(szErr, "PassportAuthFail") == 0)
            {
                MSN_GetPassportAuth();
                status = MSN_ABFind(szMethod, szGuid) ? 200 : 500;
            }
        }
		ezxml_free(xmlm);
	}
	mir_free(tResult);
	mir_free(abUrl);

	return status == 200;
}


//		"ABGroupContactAdd" : "ABGroupContactDelete", "ABGroupDelete", "ABContactDelete"
bool CMsnProto::MSN_ABAddDelContactGroup(const char* szCntId, const char* szGrpId, const char* szMethod)
{
	char* reqHdr;
	ezxml_t tbdy, node;
	ezxml_t xmlp = abSoapHdr(szMethod, "Timer", tbdy, reqHdr);

	if (szGrpId != NULL)
	{
		node = ezxml_add_child(tbdy, "groupFilter", 0);
		node = ezxml_add_child(node, "groupIds", 0);
		node = ezxml_add_child(node, "guid", 0);
		ezxml_set_txt(node, szGrpId);
	}

	if (szCntId != NULL)
	{
		node = ezxml_add_child(tbdy, "contacts", 0);
		node = ezxml_add_child(node, "Contact", 0);
		node = ezxml_add_child(node, "contactId", 0);
		ezxml_set_txt(node, szCntId);
	}

	char* szData = ezxml_toxml(xmlp, true);
	ezxml_free(xmlp);

	unsigned status;

	char* abUrl = GetABHost(szMethod, false);
	char* tResult = getSslResult(&abUrl, szData, reqHdr, status);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL)
	{
		UpdateABHost(szMethod, abUrl);
		ezxml_t xmlm = ezxml_parse_str(tResult, strlen(tResult));
		if (status == 500)
		{
			const char* szErr = ezxml_txt(getSoapFault(xmlm, true));
            if (strcmp(szErr, "PassportAuthFail") == 0)
            {
                MSN_GetPassportAuth();
                status = MSN_ABAddDelContactGroup(szCntId, szGrpId, szMethod) ? 200 : 500;
            }
		}
		ezxml_free(xmlm);
	}
	mir_free(tResult);
	mir_free(abUrl);

	return status == 200;
}

void CMsnProto::MSN_ABAddGroup(const char* szGrpName)
{
	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = abSoapHdr("ABGroupAdd", "GroupSave", tbdy, reqHdr);

	ezxml_t node = ezxml_add_child(tbdy, "groupAddOptions", 0);
	node = ezxml_add_child(node, "fRenameOnMsgrConflict", 0);
	ezxml_set_txt(node, "false");

	node = ezxml_add_child(tbdy, "groupInfo", 0);
	ezxml_t grpi = ezxml_add_child(node, "GroupInfo", 0);
	node = ezxml_add_child(grpi, "name", 0);
	ezxml_set_txt(node, szGrpName);
	node = ezxml_add_child(grpi, "groupType", 0);
	ezxml_set_txt(node, "C8529CE2-6EAD-434d-881F-341E17DB3FF8");
	node = ezxml_add_child(grpi, "fMessenger", 0);
	ezxml_set_txt(node, "false");
	node = ezxml_add_child(grpi, "annotations", 0);
	ezxml_t annt = ezxml_add_child(node, "Annotation", 0);
	node = ezxml_add_child(annt, "Name", 0);
	ezxml_set_txt(node, "MSN.IM.Display");
	node = ezxml_add_child(annt, "Value", 0);
	ezxml_set_txt(node, "1");

	char* szData = ezxml_toxml(xmlp, true);
	ezxml_free(xmlp);

	unsigned status;

	char* abUrl = GetABHost("ABGroupAdd", false);
	char* tResult = getSslResult(&abUrl, szData, reqHdr, status);

	free(szData);
	mir_free(reqHdr);

	if (tResult != NULL)
	{
		UpdateABHost("ABGroupAdd", abUrl);
		ezxml_t xmlm = ezxml_parse_str(tResult, strlen(tResult));
        if (status == 200)
        {
            ezxml_t body = getSoapResponse(xmlm, "ABGroupAdd");
		    const char* szGrpId = ezxml_txt(ezxml_child(body, "guid"));
		    MSN_AddGroup(szGrpName, szGrpId, false); 
        }
        else if (status = 500)
        {
			const char* szErr = ezxml_txt(getSoapFault(xmlm, true));
            if (strcmp(szErr, "PassportAuthFail") == 0)
            {
                MSN_GetPassportAuth();
                MSN_ABAddGroup(szGrpName);
            }
        }
		ezxml_free(xmlm);
	}
	mir_free(tResult);
	mir_free(abUrl);
}


void CMsnProto::MSN_ABRenameGroup(const char* szGrpName, const char* szGrpId)
{
	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = abSoapHdr("ABGroupUpdate", "Timer", tbdy, reqHdr);

	ezxml_t node = ezxml_add_child(tbdy, "groups", 0);
	ezxml_t grp = ezxml_add_child(node, "Group", 0);

	node = ezxml_add_child(grp, "groupId", 0);
	ezxml_set_txt(node, szGrpId);
	ezxml_t grpi = ezxml_add_child(grp, "groupInfo", 0);
	node = ezxml_add_child(grpi, "name", 0);
	ezxml_set_txt(node, szGrpName);
	node = ezxml_add_child(grp, "propertiesChanged", 0);
	ezxml_set_txt(node, "GroupName");

	char* szData = ezxml_toxml(xmlp, true);
	ezxml_free(xmlp);

	unsigned status;

	char* abUrl = GetABHost("ABGroupUpdate", false);
	char* tResult = getSslResult(&abUrl, szData, reqHdr, status);

	free(szData);
	mir_free(reqHdr);

	if (tResult != NULL)
	{
		UpdateABHost("ABGroupUpdate", abUrl);
		ezxml_t xmlm = ezxml_parse_str(tResult, strlen(tResult));
		if (status == 500)
		{
			const char* szErr = ezxml_txt(getSoapFault(xmlm, true));
            if (strcmp(szErr, "PassportAuthFail") == 0)
            {
                MSN_GetPassportAuth();
                MSN_ABRenameGroup(szGrpName, szGrpId);
            }
		}
		ezxml_free(xmlm);
	}
	mir_free(tResult);
	mir_free(abUrl);
}


void CMsnProto::MSN_ABUpdateProperty(const char* szCntId, const char* propName, const char* propValue)
{
	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = abSoapHdr("ABContactUpdate", "Timer", tbdy, reqHdr);

	ezxml_t node = ezxml_add_child(tbdy, "contacts", 0);
	ezxml_t cont = ezxml_add_child(node, "Contact", 0);
	ezxml_set_attr(cont, "xmlns", "http://www.msn.com/webservices/AddressBook");

	ezxml_t conti = ezxml_add_child(cont, "contactInfo", 0);
	if (szCntId == NULL)
	{
		node = ezxml_add_child(conti, "contactType", 0);
		ezxml_set_txt(node, "Me");
	}
	else
	{
		node = ezxml_add_child(cont, "contactId", 0);
		ezxml_set_txt(node, szCntId);
	}
	node = ezxml_add_child(conti, propName, 0);
	ezxml_set_txt(node, propValue);

	node = ezxml_add_child(cont, "propertiesChanged", 0);
	char* szPrpChg = mir_strdup(propName); 
	*szPrpChg = _toupper(*szPrpChg);
	ezxml_set_txt(node, szPrpChg);

	char* szData = ezxml_toxml(xmlp, true);
	ezxml_free(xmlp);
	mir_free(szPrpChg);

	unsigned status;

	char* abUrl = GetABHost("ABContactUpdate", false);
	char* tResult = getSslResult(&abUrl, szData, reqHdr, status);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL)
	{
		UpdateABHost("ABContactUpdate", abUrl);
		ezxml_t xmlm = ezxml_parse_str(tResult, strlen(tResult));
		if (status == 500)
		{
			const char* szErr = ezxml_txt(getSoapFault(xmlm, true));
            if (strcmp(szErr, "PassportAuthFail") == 0)
            {
                MSN_GetPassportAuth();
                MSN_ABUpdateProperty(szCntId, propName, propValue);
            }
		}
		ezxml_free(xmlm);
	}
	mir_free(tResult);
	mir_free(abUrl);
}


void CMsnProto::MSN_ABUpdateAttr(const char* szCntId, const char* szAttr, const char* szValue)
{
	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = abSoapHdr("ABContactUpdate", "Timer", tbdy, reqHdr);

	ezxml_t node = ezxml_add_child(tbdy, "contacts", 0);
	ezxml_t cont = ezxml_add_child(node, "Contact", 0);
	ezxml_set_attr(cont, "xmlns", "http://www.msn.com/webservices/AddressBook");
	ezxml_t conti = ezxml_add_child(cont, "contactInfo", 0);
	if (szCntId == NULL)
	{
		node = ezxml_add_child(conti, "contactType", 0);
		ezxml_set_txt(node, "Me");
	}
	else
	{
		node = ezxml_add_child(cont, "contactId", 0);
		ezxml_set_txt(node, szCntId);
	}
	node = ezxml_add_child(conti, "annotations", 0);
	ezxml_t anot = ezxml_add_child(node, "Annotation", 0);
	node = ezxml_add_child(anot, "Name", 0);
	ezxml_set_txt(node, szAttr);
	node = ezxml_add_child(anot, "Value", 0);
	if (szValue != NULL) ezxml_set_txt(node, szValue);

	node = ezxml_add_child(cont, "propertiesChanged", 0);
	ezxml_set_txt(node, "Annotation");

	char* szData = ezxml_toxml(xmlp, true);
	ezxml_free(xmlp);

	unsigned status;

	char* abUrl = GetABHost("ABContactUpdate", false);
	char* tResult = getSslResult(&abUrl, szData, reqHdr, status);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL)
	{
		UpdateABHost("ABContactUpdate", abUrl);
		ezxml_t xmlm = ezxml_parse_str(tResult, strlen(tResult));
		if (status == 500)
		{
			const char* szErr = ezxml_txt(getSoapFault(xmlm, true));
            if (strcmp(szErr, "PassportAuthFail") == 0)
            {
                MSN_GetPassportAuth();
                MSN_ABUpdateAttr(szCntId, szAttr, szValue);
            }
		}
		ezxml_free(xmlm);
	}
	mir_free(tResult);
	mir_free(abUrl);
}


void CMsnProto::MSN_ABUpdateNick(const char* szNick, const char* szCntId)
{
	if (szCntId != NULL)
		MSN_ABUpdateAttr(szCntId, "AB.NickName", szNick);
	else
		MSN_ABUpdateProperty(szCntId, "displayName", szNick);
}


unsigned CMsnProto::MSN_ABContactAdd(const char* szEmail, const char* szNick, int netId, const bool search, const bool retry)
{
	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = abSoapHdr("ABContactAdd", "ContactSave", tbdy, reqHdr);

	ezxml_t conts = ezxml_add_child(tbdy, "contacts", 0);
	ezxml_t node = ezxml_add_child(conts, "Contact", 0);
	ezxml_set_attr(node, "xmlns", "http://www.msn.com/webservices/AddressBook");
	ezxml_t conti = ezxml_add_child(node, "contactInfo", 0);
	ezxml_t contp;

	const char* szEmailNP = strchr(szEmail, ':');
	if (szEmailNP != NULL) netId = NETID_MSN;

	switch (netId)
	{
	case NETID_MSN:
		node = ezxml_add_child(conti, "contactType", 0);
		ezxml_set_txt(node, "LivePending");
		node = ezxml_add_child(conti, "passportName", 0);
		ezxml_set_txt(node, szEmail);
		node = ezxml_add_child(conti, "isMessengerUser", 0);
		ezxml_set_txt(node, "true");
		break;

	case NETID_MOB:
		++szEmailNP;
		if (szNick == NULL) szNick = szEmailNP;
		node = ezxml_add_child(conti, "phones", 0);
		contp = ezxml_add_child(node, "ContactPhone", 0);
		node = ezxml_add_child(contp, "contactPhoneType", 0);
		ezxml_set_txt(node, "ContactPhoneMobile");
		node = ezxml_add_child(contp, "number", 0);
		ezxml_set_txt(node, szEmailNP);
		node = ezxml_add_child(contp, "isMessengerEnabled", 0);
		ezxml_set_txt(node, "true");
		node = ezxml_add_child(contp, "propertiesChanged", 0);
		ezxml_set_txt(node, "Number IsMessengerEnabled");
		break;

	case NETID_LCS:
	case NETID_YAHOO:
		node = ezxml_add_child(conti, "emails", 0);
		contp = ezxml_add_child(node, "ContactEmail", 0);
		node = ezxml_add_child(contp, "contactEmailType", 0);
		ezxml_set_txt(node, netId == NETID_YAHOO ? "Messenger2" : "Messenger3");
		node = ezxml_add_child(contp, "email", 0);
		ezxml_set_txt(node, szEmail);
		node = ezxml_add_child(contp, "isMessengerEnabled", 0);
		ezxml_set_txt(node, "true");
		node = ezxml_add_child(contp, "propertiesChanged", 0);
		ezxml_set_txt(node, "Email IsMessengerEnabled");
		break;
	}

	if (szNick != NULL)
	{
		node = ezxml_add_child(conti, "annotations", 0);
		ezxml_t annt = ezxml_add_child(node, "Annotation", 0);
		node = ezxml_add_child(annt, "Name", 0);
		ezxml_set_txt(node, "MSN.IM.Display");
		node = ezxml_add_child(annt, "Value", 0);
		ezxml_set_txt(node, szNick);
	}

	node = ezxml_add_child(conts, "options", 0);
	node = ezxml_add_child(node, "EnableAllowListManagement", 0);
	ezxml_set_txt(node, "true");

	char* szData = ezxml_toxml(xmlp, true);
	ezxml_free(xmlp);

	unsigned status;

	char* abUrl = GetABHost("ABContactAdd", false);
	char* tResult = getSslResult(&abUrl, szData, reqHdr, status);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL)
	{
		UpdateABHost("ABContactAdd", abUrl);
		ezxml_t xmlm = ezxml_parse_str(tResult, strlen(tResult));
		if (status == 200)
		{
            ezxml_t body = getSoapResponse(xmlm, "ABContactAdd");
		
			const char* szContId = ezxml_txt(ezxml_child(body, "guid"));

			if (search) 
				MSN_ABAddDelContactGroup(szContId , NULL, "ABContactDelete");
			else
			{
				HANDLE hContact = MSN_HContactFromEmail( szEmail, szNick ? szNick : szEmail, true, false );
				setString(hContact, "ID", szContId);
			}
			status = 0;
		}
		else if (status == 500)
		{
			const char* szErr = ezxml_txt(getSoapFault(xmlm, true));

            if (strcmp(szErr, "InvalidPassportUser") == 0)
                status = 1;
            else if (strcmp(szErr, "EmailDomainIsFederated") == 0)
                status = 2;
            else if (strcmp(szErr, "ContactAlreadyExists") == 0) 
            {
                status = 3;

                ezxml_t node = getSoapFault(xmlm, false);
                node = ezxml_get(node, "detail", 0, "additionalDetails", 0, "conflictObjectId", -1);
			    const char* szContId = ezxml_txt(node);

                if (search) 
                {
                    if (retry)
                    {
				        MSN_ABAddDelContactGroup(szContId , NULL, "ABContactDelete");
			            status = 0;
                    }
                }
			    else
			    {
				    HANDLE hContact = MSN_HContactFromEmail( szEmail, szNick ? szNick : szEmail, true, false );
				    setString(hContact, "ID", szContId);
			    }
            }
            else if (strcmp(szErr, "PassportAuthFail") == 0)
            {
                MSN_GetPassportAuth();
                status = MSN_ABContactAdd(szEmail, szNick, netId, search, retry);
            }
            else
            {
                status = MSN_ABContactAdd(szEmail, szNick, netId, search, true);
            }
		}
		ezxml_free(xmlm);
	}
	mir_free(tResult);
	mir_free(abUrl);

	return status;
}


void CMsnProto::MSN_ABUpdateDynamicItem(void)
{
	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = abSoapHdr("UpdateDynamicItem", "RoamingIdentityChanged", tbdy, reqHdr);

	ezxml_t dynitms = ezxml_add_child(tbdy, "dynamicItems", 0);
	ezxml_t dynitm = ezxml_add_child(dynitms, "DynamicItem", 0);
	
	ezxml_set_attr(dynitm, "xsi:type", "PassportDynamicItem");
	ezxml_t node = ezxml_add_child(dynitm, "Type", 0);
	ezxml_set_txt(node, "Passport");
	node = ezxml_add_child(dynitm, "PassportName", 0);
	ezxml_set_txt(node, MyOptions.szEmail);

	ezxml_t nots = ezxml_add_child(dynitm, "Notifications", 0);
	ezxml_t notd = ezxml_add_child(nots, "NotificationData", 0);
	ezxml_t strsvc = ezxml_add_child(notd, "StoreService", 0);
	ezxml_t info = ezxml_add_child(strsvc, "Info", 0);

	ezxml_t hnd = ezxml_add_child(info, "Handle", 0);
	node = ezxml_add_child(hnd, "Id", 0);
	ezxml_set_txt(node, "0");
	node = ezxml_add_child(hnd, "Type", 0);
	ezxml_set_txt(node, "Profile");
	node = ezxml_add_child(hnd, "ForeignId", 0);
	ezxml_set_txt(node, "MyProfile");
	
	node = ezxml_add_child(info, "InverseRequired", 0);
	ezxml_set_txt(node, "false");
	node = ezxml_add_child(info, "IsBot", 0);
	ezxml_set_txt(node, "false");

	node = ezxml_add_child(strsvc, "Changes", 0);
	node = ezxml_add_child(strsvc, "LastChange", 0);
	ezxml_set_txt(node, "0001-01-01T00:00:00");
	node = ezxml_add_child(strsvc, "Deleted", 0);
	ezxml_set_txt(node, "false");

	node = ezxml_add_child(notd, "Status", 0);
	ezxml_set_txt(node, "Exist Access");
	node = ezxml_add_child(notd, "LastChanged", 0);
	
	time_t timer;
	time(&timer);
	tm *tmst = gmtime(&timer);

	char tmstr[32];
	mir_snprintf(tmstr, sizeof(tmstr), "%04u-%02u-%02uT%02u:%02u:%02uZ", 
		tmst->tm_year + 1900, tmst->tm_mon+1, tmst->tm_mday, 
		tmst->tm_hour, tmst->tm_min, tmst->tm_sec);

	ezxml_set_txt(node, tmstr);
	node = ezxml_add_child(notd, "Gleam", 0);
	ezxml_set_txt(node, "false");
	node = ezxml_add_child(notd, "InstanceId", 0);
	ezxml_set_txt(node, "0");

	node = ezxml_add_child(dynitm, "Changes", 0);
	ezxml_set_txt(node, "Notifications");

	char* szData = ezxml_toxml(xmlp, true);
	ezxml_free(xmlp);

	unsigned status;

	char* abUrl = GetABHost("UpdateDynamicItem", false);
	char* tResult = getSslResult(&abUrl, szData, reqHdr, status);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL)
	{
		UpdateABHost("UpdateDynamicItem", abUrl);
		ezxml_t xmlm = ezxml_parse_str(tResult, strlen(tResult));
		if (status == 500)
		{
			const char* szErr = ezxml_txt(getSoapFault(xmlm, true));
            if (strcmp(szErr, "PassportAuthFail") == 0)
            {
                MSN_GetPassportAuth();
                MSN_ABUpdateDynamicItem();
            }
		}
		ezxml_free(xmlm);
	}
	mir_free(tResult);
	mir_free(abUrl);
}
