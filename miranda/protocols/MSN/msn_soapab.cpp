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

extern char *authContactToken;

static const char abReqHdr[] = 
	"SOAPAction: http://www.msn.com/webservices/AddressBook/%s\r\n";

static ezxml_t abSoapHdr(const char* service, const char* scenario, ezxml_t& tbdy, char*& httphdr)
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
	ezxml_set_txt(node, "996CDE1E-AA53-4477-B943-2BE802EA6166");
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

	if (strncmp(service,"AB", 2) == 0)
	{
		ezxml_t node = ezxml_add_child(tbdy, "abId", 0);
		ezxml_set_txt(node, "00000000-0000-0000-0000-000000000000");
	}

	size_t hdrsz = strlen(service) + sizeof(abReqHdr) + 20;
	httphdr = (char*)mir_alloc(hdrsz);

	mir_snprintf(httphdr, hdrsz, abReqHdr, service);

	return xmlp;
}

static void UpdateABHost(ezxml_t xmlp, char* orgurl)
{
	const char* newhost = ezxml_txt(ezxml_get(xmlp, "soap:Header", 0, "ServiceHeader", 0, 
		"PreferredHostName", -1));
	
	if (newhost[0])
	{
		char schstr[128];
		size_t sz = mir_snprintf(schstr, sizeof(schstr), "https://%s/", newhost);

		if (strncmp(orgurl, schstr, sz) != 0)
			MSN_SetString(NULL, "MsnABHost", newhost);
	}
}

static char* GetABHost(bool isSharing)
{
	char host[128];
	if (MSN_GetStaticString("MsnABHost", NULL, host, sizeof(host)))
		strcpy(host, "contacts.msn.com");

	char* fullhost = (char*)mir_alloc(256);;
	mir_snprintf(fullhost, 256, "https://%s/abservice/%s.asmx", host, 
		isSharing ? "SharingService" : "abservice");

	return fullhost;
}

void MSN_SharingFindMembership(void)
{
	SSLAgent mAgent;

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

	char* szData = ezxml_toxml(xmlp, true);

	ezxml_free(xmlp);

	unsigned status;
	char* htmlbody;

	char* abUrl = GetABHost(true);
	char* tResult = mAgent.getSslResult(abUrl, szData, reqHdr, status, htmlbody);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL && status == 200)
	{
		ezxml_t xmlm = ezxml_parse_str(htmlbody, strlen(htmlbody));
		ezxml_t mems = ezxml_get(xmlm, "soap:Body", 0, "FindMembershipResponse", 0, 
			"FindMembershipResult", 0, "Services", 0, "Service", 0, 
			"Memberships", 0, "Membership", -1);
		
		UpdateABHost(xmlm, abUrl);

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
					int netId = strstr(szEmail, "@yahoo.com") ? 32 : 2;
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
		ezxml_free(xmlm);
	}
	mir_free(tResult);
	mir_free(abUrl);

//	return status == 200;
}

// AddMember, DeleteMember
bool MSN_SharingAddDelMember(const char* szEmail, const char* szRole, const char* szMethod)
{
	SSLAgent mAgent;

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

	int netId = Lists_GetNetId(szEmail);
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
	if ((netId == 2 || netId == 32) && strcmp(szMethod, "DeleteMember") != 0)
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
	char* htmlbody;

	char* abUrl = GetABHost(true);
	char* tResult = mAgent.getSslResult(abUrl, szData, reqHdr, status, htmlbody);

	mir_free(reqHdr);
	free(szData);

	mir_free(tResult);
	mir_free(abUrl);

	return status == 200;
}


void MSN_ABGetFull(void)
{
	SSLAgent mAgent;

	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = abSoapHdr("ABFindAll", "Initial", tbdy, reqHdr);

	ezxml_t node = ezxml_add_child(tbdy, "abView", 0);
	ezxml_set_txt(node, "Full");
	node = ezxml_add_child(tbdy, "deltasOnly", 0);
	ezxml_set_txt(node, "false");
	node = ezxml_add_child(tbdy, "dynamicItemView", 0);
	ezxml_set_txt(node, "Gleam");

	char* szData = ezxml_toxml(xmlp, true);
	ezxml_free(xmlp);

	unsigned status;
	char* htmlbody;

	char* abUrl = GetABHost(false);
	char* tResult = mAgent.getSslResult(abUrl, szData, reqHdr, status, htmlbody);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL && status == 200)
	{
		ezxml_t xmlm = ezxml_parse_str(htmlbody, strlen(htmlbody));
		ezxml_t abook = ezxml_get(xmlm, "soap:Body", 0, "ABFindAllResponse", 0, 
			"ABFindAllResult", -1);
		
		UpdateABHost(xmlm, abUrl);

		if (MyOptions.ManageServer)
		{
			ezxml_t grp = ezxml_get(abook, "groups", 0, "Group", -1); 
			while (grp != NULL)
			{
				const char* szGrpId = ezxml_txt(ezxml_child(grp, "groupId"));
				const char* szGrpName = ezxml_txt(ezxml_get(grp, "groupInfo", 0, "name", -1));
				MSN_AddGroup(szGrpName, szGrpId, true); 

				grp = ezxml_next(grp);
			}
		}

		ezxml_t cont = ezxml_get(abook, "contacts", 0, "Contact", -1); 
		while (cont != NULL)
		{
			const char* szContId = ezxml_txt(ezxml_child(cont, "contactId"));
			
			ezxml_t contInf = ezxml_child(cont, "contactInfo");
			const char* szType = ezxml_txt(ezxml_child(contInf, "contactType"));

			if (strcmp(szType, "Me") != 0)
			{
				char email[128];
				int typeId = 0;

				const char* szEmail = ezxml_txt(ezxml_child(contInf, "passportName"));
				const char* szMsgUsr = ezxml_txt(ezxml_child(contInf, "isMessengerUser"));
				if (*szEmail == '\0')
				{
					ezxml_t phn = ezxml_get(contInf, "phones", 0, "ContactPhone", -1);
					if (phn != NULL)
					{
						szMsgUsr = ezxml_txt(ezxml_child(phn, "isMessengerEnabled"));
						szEmail = ezxml_txt(ezxml_child(phn, "number"));
						mir_snprintf(email, sizeof(email), "tel:%s", szEmail);
						szEmail = email;
						typeId = 4;
					}
					else 
					{
						ezxml_t eml = ezxml_get(contInf, "emails", 0, "ContactEmail", -1);
						if (eml != NULL)
						{
							szMsgUsr = ezxml_txt(ezxml_child(eml, "isMessengerEnabled"));
							szEmail = ezxml_txt(ezxml_child(eml, "email"));
							const char* szCntType = ezxml_txt(ezxml_child(eml, "contactEmailType"));
							if (strcmp(szCntType, "Messenger2") == 0)
								typeId = 32;
							else if (strcmp(szCntType, "Messenger3") == 0)
								typeId = 2;

						}
					}
				}

				const int lstFlg = strcmp(szMsgUsr, "true") ? 0 : LIST_FL; 
				if (Lists_IsInList(-1, szEmail) || lstFlg != 0)
				{
					const char* szMBE   = ezxml_txt(ezxml_child(contInf, "isMobileIMEnabled"));
					const char* szMOB   = ezxml_txt(ezxml_child(contInf, "IsNotMobileVisible"));

					const char* szNick  = ezxml_txt(ezxml_child(contInf, "displayName"));
					if (*szNick == '\0') szNick = szEmail;
					HANDLE hContact = MSN_HContactFromEmail(szEmail, szNick, 1, 0);
					MSN_SetStringUtf(hContact, "Nick", (char*)szNick);

					ezxml_t anot = ezxml_get(contInf, "annotations", 0, "Annotation", -1);
					while (anot != NULL)
					{
						if (strcmp(ezxml_txt(ezxml_child(anot, "Name")), "AB.NickName") == 0)
						{
							DBWriteContactSettingStringUtf(hContact, "CList", "MyHandle", 
								ezxml_txt(ezxml_child(anot, "Value")));
							break;
						}
						anot = ezxml_next(anot);
					}
					if (anot == NULL)
						DBDeleteContactSetting(hContact, "CList", "MyHandle");

					Lists_Add(lstFlg, typeId, szEmail);

					DBWriteContactSettingByte(hContact, msnProtocolName, "MobileAllowed", strcmp(szMOB, "true") != 0);
					DBWriteContactSettingByte(hContact, msnProtocolName, "MobileEnabled", strcmp(szMBE, "true") == 0);

					ezxml_t cgrp = ezxml_get(contInf, "groupIds", 0, "guid", -1);
					MSN_SyncContactToServerGroup( hContact, szContId, cgrp );
				}
			}
			else
			{
				if (!MSN_GetByte( "NeverUpdateNickname", 0 ))
				{
					const char* szNick  = ezxml_txt(ezxml_child(contInf, "displayName"));
					MSN_SetStringUtf(NULL, "Nick", (char*)szNick);
				}
				const char* szMBE   = ezxml_txt(ezxml_child(contInf, "isMobileIMEnabled"));
				MSN_SetByte( "MobileEnabled", strcmp(szMBE, "true") == 0);
				const char* szMOB   = ezxml_txt(ezxml_child(contInf, "IsNotMobileVisible"));
				MSN_SetByte( "MobileAllowed", strcmp(szMOB, "true") != 0);

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
		ezxml_free(xmlm);
	}
	mir_free(tResult);
	mir_free(abUrl);
}

//		"ABGroupContactAdd" : "ABGroupContactDelete", "ABGroupDelete", "ABContactDelete"
bool MSN_ABAddDelContactGroup(const char* szCntId, const char* szGrpId, const char* szMethod)
{
	SSLAgent mAgent;

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
	char* htmlbody;

	char* abUrl = GetABHost(false);
	char* tResult = mAgent.getSslResult(abUrl, szData, reqHdr, status, htmlbody);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL && status == 200)
	{
		ezxml_t xmlm = ezxml_parse_str(htmlbody, strlen(htmlbody));
		UpdateABHost(xmlm, abUrl);
		ezxml_free(xmlm);
	}
	mir_free(tResult);
	mir_free(abUrl);

	return status == 200;
}

void MSN_ABAddGroup(const char* szGrpName)
{
	SSLAgent mAgent;

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
	char* htmlbody;

	char* abUrl = GetABHost(false);
	char* tResult = mAgent.getSslResult(abUrl, szData, reqHdr, status, htmlbody);

	free(szData);
	mir_free(reqHdr);

	if (tResult != NULL && status == 200)
	{
		ezxml_t xmlm = ezxml_parse_str(htmlbody, strlen(htmlbody));
		
		const char* szGrpId = ezxml_txt(ezxml_get(xmlm, "soap:Body", 0, "ABGroupAddResponse", 0, 
			"ABGroupAddResult", 0, "guid", -1));

		UpdateABHost(xmlm, abUrl);
		MSN_AddGroup(szGrpName, szGrpId, false); 

		ezxml_free(xmlm);
	}
	mir_free(tResult);
	mir_free(abUrl);
}


void MSN_ABRenameGroup(const char* szGrpName, const char* szGrpId)
{
	SSLAgent mAgent;

	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = abSoapHdr("ABGroupUpdate", "Timer", tbdy, reqHdr);

	ezxml_t node = ezxml_add_child(tbdy, "groups", 0);
	ezxml_t grp = ezxml_add_child(node, "Group", 0);

	node = ezxml_add_child(grp, "groupId", 0);
	ezxml_set_txt(node, szGrpId);
	ezxml_t grpi = ezxml_add_child(grp, "GroupInfo", 0);
	node = ezxml_add_child(grpi, "name", 0);
	ezxml_set_txt(node, szGrpName);
	node = ezxml_add_child(grp, "propertiesChanged", 0);
	ezxml_set_txt(node, "GroupName");

	char* szData = ezxml_toxml(xmlp, true);
	ezxml_free(xmlp);

	unsigned status;
	char* htmlbody;

	char* abUrl = GetABHost(false);
	char* tResult = mAgent.getSslResult(abUrl, szData, reqHdr, status, htmlbody);

	free(szData);
	mir_free(reqHdr);

	if (tResult != NULL && status == 200)
	{
		ezxml_t xmlm = ezxml_parse_str(htmlbody, strlen(htmlbody));
		UpdateABHost(xmlm, abUrl);
		ezxml_free(xmlm);
	}
	mir_free(tResult);
	mir_free(abUrl);
}


void MSN_ABUpdateNick(const char* szNick, const char* szCntId)
{
	SSLAgent mAgent;

	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = abSoapHdr("ABContactUpdate", "Timer", tbdy, reqHdr);

	ezxml_t node = ezxml_add_child(tbdy, "contacts", 0);
	ezxml_t cont = ezxml_add_child(node, "Contact", 0);
	ezxml_set_attr(cont, "xmlns", "http://www.msn.com/webservices/AddressBook");
	if (szCntId != NULL)
	{
		node = ezxml_add_child(cont, "contactId", 0);
		ezxml_set_txt(node, szCntId);
		node = ezxml_add_child(cont, "contactInfo", 0);
		node = ezxml_add_child(node, "annotations", 0);
		ezxml_t anot = ezxml_add_child(node, "Annotation", 0);
		node = ezxml_add_child(anot, "Name", 0);
		ezxml_set_txt(node, "AB.NickName");
 		node = ezxml_add_child(anot, "Value", 0);
		if (szNick) ezxml_set_txt(node, szNick);
		node = ezxml_add_child(cont, "propertiesChanged", 0);
		ezxml_set_txt(node, "Annotation");
	}
	else
	{
		ezxml_t conti = ezxml_add_child(cont, "contactInfo", 0);
		node = ezxml_add_child(conti, "contactType", 0);
		ezxml_set_txt(node, "Me");
		node = ezxml_add_child(conti, "displayName", 0);
		ezxml_set_txt(node, szNick);
		node = ezxml_add_child(cont, "propertiesChanged", 0);
		ezxml_set_txt(node, "DisplayName");
	}

	char* szData = ezxml_toxml(xmlp, true);
	ezxml_free(xmlp);

	unsigned status;
	char* htmlbody;

	char* abUrl = GetABHost(false);
	char* tResult = mAgent.getSslResult(abUrl, szData, reqHdr, status, htmlbody);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL && status == 200)
	{
		ezxml_t xmlm = ezxml_parse_str(htmlbody, strlen(htmlbody));
		UpdateABHost(xmlm, abUrl);
		ezxml_free(xmlm);
	}
	mir_free(tResult);
	mir_free(abUrl);
}


void MSN_ABUpdateAttr(const char* szAttr, const int value)
{
	SSLAgent mAgent;

	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = abSoapHdr("ABContactUpdate", "Timer", tbdy, reqHdr);

	ezxml_t node = ezxml_add_child(tbdy, "contacts", 0);
	ezxml_t cont = ezxml_add_child(node, "Contact", 0);
	ezxml_set_attr(cont, "xmlns", "http://www.msn.com/webservices/AddressBook");
	ezxml_t conti = ezxml_add_child(cont, "contactInfo", 0);
	node = ezxml_add_child(conti, "contactType", 0);
	ezxml_set_txt(node, "Me");
	node = ezxml_add_child(conti, "annotations", 0);
	ezxml_t anot = ezxml_add_child(node, "Annotation", 0);
	node = ezxml_add_child(anot, "Name", 0);
	ezxml_set_txt(node, szAttr);
	node = ezxml_add_child(anot, "Value", 0);

	char buf[64];
	mir_snprintf(buf, sizeof(buf), "%d", value);
	ezxml_set_txt(node, buf);

	node = ezxml_add_child(cont, "propertiesChanged", 0);
	ezxml_set_txt(node, "Annotation");

	char* szData = ezxml_toxml(xmlp, true);
	ezxml_free(xmlp);

	unsigned status;
	char* htmlbody;

	char* abUrl = GetABHost(false);
	char* tResult = mAgent.getSslResult(abUrl, szData, reqHdr, status, htmlbody);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL && status == 200)
	{
		ezxml_t xmlm = ezxml_parse_str(htmlbody, strlen(htmlbody));
		UpdateABHost(xmlm, abUrl);
		ezxml_free(xmlm);
	}
	mir_free(tResult);
	mir_free(abUrl);
}


unsigned MSN_ABContactAdd(const char* szEmail, const char* szNick, int typeId, const bool search)
{
	SSLAgent mAgent;

	char* reqHdr;
	ezxml_t tbdy;
	ezxml_t xmlp = abSoapHdr("ABContactAdd", "ContactSave", tbdy, reqHdr);

	ezxml_t conts = ezxml_add_child(tbdy, "contacts", 0);
	ezxml_t node = ezxml_add_child(conts, "Contact", 0);
	ezxml_set_attr(node, "xmlns", "http://www.msn.com/webservices/AddressBook");
	ezxml_t conti = ezxml_add_child(node, "contactInfo", 0);
	ezxml_t contp;

	const char* szEmailNP = strchr(szEmail, ':');
	if (szEmailNP != NULL) typeId = 4;

	switch (typeId)
	{
	case 1:
		node = ezxml_add_child(conti, "contactType", 0);
		ezxml_set_txt(node, "LivePending");
		node = ezxml_add_child(conti, "passportName", 0);
		ezxml_set_txt(node, szEmail);
		node = ezxml_add_child(conti, "isMessengerUser", 0);
		ezxml_set_txt(node, "true");
		break;

	case 4:
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

	case 2:
	case 32:
		node = ezxml_add_child(conti, "emails", 0);
		contp = ezxml_add_child(node, "ContactEmail", 0);
		node = ezxml_add_child(contp, "contactEmailType", 0);
		ezxml_set_txt(node, typeId == 32 ? "Messenger2" : "Messenger3");
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
	char* htmlbody;

	char* abUrl = GetABHost(false);
	char* tResult = mAgent.getSslResult(abUrl, szData, reqHdr, status, htmlbody);

	mir_free(reqHdr);
	free(szData);

	if (tResult != NULL)
	{
		ezxml_t xmlm = ezxml_parse_str(htmlbody, strlen(htmlbody));
		if (status == 200)
		{
		
			const char* szContId = ezxml_txt(ezxml_get(xmlm, "soap:Body", 0, "ABContactAddResponse", 0, 
				"ABContactAddResult", 0, "guid", -1));

			UpdateABHost(xmlm, abUrl);

			if (search) 
				MSN_ABAddDelContactGroup(szContId , NULL, "ABContactDelete");
			else
			{
				HANDLE hContact = MSN_HContactFromEmail( szEmail, szNick ? szNick : szEmail, 1, 0 );
				MSN_SetString(hContact, "ID", szContId);
			}
			status = 0;
		}
		else 
		{
			const char* szErr = ezxml_txt(ezxml_get(xmlm, "soap:Body", 0, "soap:Fault", 0, 
				"detail", 0, "errorcode", -1));
			status = strcmp(szErr, "EmailDomainIsFederated") ? 1 : 2;
		}
		ezxml_free(xmlm);
	}
	mir_free(tResult);
	mir_free(abUrl);

	return status;
}
