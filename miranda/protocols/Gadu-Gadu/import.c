////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2003-2006 Adam Strzelecki <ono+miranda@java.pl>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////////////////////////////////////////////////////

#include <sys/stat.h>
#include "gg.h"

int ggListRemove = FALSE;

////////////////////////////////////////////////////////////////////////////////
// Checks if a group already exists in Miranda with
// the specified name.
// Returns 1 if a group with the name exists, returns 0 otherwise.
int GroupNameExists(const char *name)
{
	char idstr[33];
	DBVARIANT dbv;
	int i;

	for (i = 0; ; i++) {
		itoa(i, idstr, 10);
		if (DBGetContactSetting(NULL, "CListGroups", idstr, &dbv)) break;
		if (!strcmp(dbv.pszVal + 1, name)) {
			DBFreeVariant(&dbv);
			return 1;
		}
		DBFreeVariant(&dbv);
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Creates a group with a specified name in the
// Miranda contact list.
// Returns proper group name
char *CreateGroup(char *groupName)
{
	int groupId;
	char groupIdStr[11];
	char groupName2[127];
	char *p;
	DBVARIANT dbv;

	// Cleanup group name from weird characters

	// Skip first break
	while(*groupName && *groupName == '\\') groupName++;

	p = strrchr(groupName, '\\');
	// Cleanup end
	while(p && !(*(p + 1)))
	{
		*p = 0;
		p = strrchr(groupName, '\\');
	}
	// Create upper groups
	if(p)
	{
		*p = 0;
		CreateGroup(groupName);
		*p = '\\';
	}

	// Is this a duplicate?
	if (!GroupNameExists(groupName))
	{
		lstrcpyn(groupName2 + 1, groupName, strlen(groupName) + 1);

		// Find an unused id
		for (groupId = 0; ; groupId++) {
				itoa(groupId, groupIdStr,10);
				if (DBGetContactSetting(NULL, "CListGroups", groupIdStr, &dbv))
						break;
				DBFreeVariant(&dbv);
		}

		groupName2[0] = 1|GROUPF_EXPANDED;	// 1 is required so we never get '\0'
		DBWriteContactSettingString(NULL, "CListGroups", groupIdStr, groupName2);
	}
	return groupName;
}

char *gg_makecontacts(int cr)
{
	string_t s = string_init(NULL);
	char *contacts;

	// Readup contacts
	HANDLE hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		char *szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
		if (szProto != NULL && !strcmp(szProto, GG_PROTO))
		{
			DBVARIANT dbv;

			// Readup FirstName
			if (!DBGetContactSetting(hContact, GG_PROTO, "FirstName", &dbv))
			{
				string_append(s, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			string_append_c(s, ';');
			// Readup LastName
			if (!DBGetContactSetting(hContact, GG_PROTO, "LastName", &dbv))
			{
				string_append(s, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			string_append_c(s, ';');

			// Readup Nick
			if (!DBGetContactSetting(hContact, "CList", "MyHandle", &dbv) || !DBGetContactSetting(hContact, GG_PROTO, "Nick", &dbv))
			{
				DBVARIANT dbv2;
				if (!DBGetContactSetting(hContact, GG_PROTO, "NickName", &dbv2))
				{
					string_append(s, dbv2.pszVal);
					DBFreeVariant(&dbv2);
				}
				else
					string_append(s, dbv.pszVal);
				string_append_c(s, ';');
				string_append(s, dbv.pszVal);
			}
			else
				string_append_c(s, ';');
			string_append_c(s, ';');

			// Readup Phone (fixed: uses stored editable phones)
			if (!DBGetContactSetting(hContact, "UserInfo", "MyPhone0", &dbv))
			{
				// Remove SMS postfix
				char *sms = strstr(dbv.pszVal, " SMS");
				if(sms) *sms = 0;

				string_append(s, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			string_append_c(s, ';');
			// Readup Group
			if (!DBGetContactSetting(hContact, "CList", "Group", &dbv))
			{
				string_append(s, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			string_append_c(s, ';');
			// Readup Uin
			string_append(s, ditoa(DBGetContactSettingDword(hContact, GG_PROTO, GG_KEY_UIN, 0)));
			string_append_c(s, ';');
			// Readup Mail (fixed: uses stored editable mails)
			if (!DBGetContactSetting(hContact, "UserInfo", "Mye-mail0", &dbv))
			{
				string_append(s, dbv.pszVal);
				DBFreeVariant(&dbv);
			}
			if(cr)
				string_append(s, ";0;;0;\r\n");
			else
				string_append(s, ";0;;0;\n");
		}
		hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
	}

	contacts = string_free(s, 0);

#ifdef DEBUGMODE
	gg_netlog("gg_makecontacts(): \n%s", contacts);
#endif

	return contacts;
}

char *strndup(char *str, int c)
{
	char *ret = malloc(c + 1);
	ret[c] = 0;
	strncpy(ret, str, c);
	return ret;
}

void gg_parsecontacts(char *contacts)
{
	char *p = strchr(contacts, ':'), *n;
	char *strFirstName, *strLastName, *strNickname, *strNick, *strPhone, *strGroup, *strUin, *strMail;
	uin_t uin;

	// Skip to proper data
	if(p && p < strchr(contacts, ';')) p++;
	else p = contacts;

	while(p)
	{
		// Processing line
		strFirstName = strLastName = strNickname = strNick = strPhone = strGroup = strUin = strMail = NULL;
		uin = 0;

		// FirstName
		if(p)
		{
			n = strchr(p, ';');
			if(n && n != p) strFirstName = strndup(p, (n - p));
			p = (n + 1);
		}
		// LastName
		if(n && p)
		{
			n = strchr(p, ';');
			if(n && n != p) strLastName = strndup(p, (n - p));
			p = (n + 1);
		}
		// Nickname
		if(n && p)
		{
			n = strchr(p, ';');
			if(n && n != p) strNickname = strndup(p, (n - p));
			p = (n + 1);
		}
		// Nick
		if(n && p)
		{
			n = strchr(p, ';');
			if(n && n != p) strNick = strndup(p, (n - p));
			p = (n + 1);
		}
		// Phone
		if(n && p)
		{
			n = strchr(p, ';');
			if(n && n != p)
			{
				strPhone = malloc((n - p) + 5);
				strncpy(strPhone, p, (n - p));
				strcpy((strPhone + (n - p)), " SMS"); // Add SMS postfix
			}
			p = (n + 1);
		}
		// Group
		if(n && p)
		{
			n = strchr(p, ';');
			if(n && n != p) strGroup = strndup(p, (n - p));
			p = (n + 1);
		}
		// Uin
		if(n && p)
		{
			n = strchr(p, ';');
			if(n && n != p)
			{
				strUin = strndup(p, (n - p));
				uin = atoi(strUin);
			}
			p = (n + 1);
		}
		// Mail
		if(n && p)
		{
			n = strchr(p, ';');
			if(n && n != p) strMail = strndup(p, (n - p));
			n = strchr(p, '\n');
			p = (n + 1);
		}
		if(!n) p = NULL;

		// Loadup contact
		if(uin && strNick)
		{
			HANDLE hContact = gg_getcontact(uin, 1, 1, strNick);
#ifdef DEBUGMODE
			gg_netlog("gg_parsecontacts(): Found contact %d with nickname \"%s\".", uin, strNick);
#endif

			// Write group
			if(hContact && strGroup)
				DBWriteContactSettingString(hContact, "CList", "Group", CreateGroup(strGroup));

			// Write misc data
			if(hContact && strFirstName) DBWriteContactSettingString(hContact, GG_PROTO, "FirstName", strFirstName);
			if(hContact && strLastName) DBWriteContactSettingString(hContact, GG_PROTO, "LastName", strLastName);
			if(hContact && strPhone) DBWriteContactSettingString(hContact, "UserInfo", "MyPhone0", strPhone); // Store now in User Info
			if(hContact && strMail) DBWriteContactSettingString(hContact, "UserInfo", "Mye-mail0", strMail); // Store now in User Info
		}

		// Release stuff
		if(strFirstName) free(strFirstName);
		if(strLastName) free(strLastName);
		if(strNickname) free(strNickname);
		if(strNick) free(strNick);
		if(strPhone) free(strPhone);
		if(strGroup) free(strGroup);
		if(strUin) free(strUin);
		if(strMail) free(strMail);
	}
}

//////////////////////////////////////////////////////////
// import from server
static int gg_import_server(WPARAM wParam, LPARAM lParam)
{
	char *password;
	uin_t uin;
	DBVARIANT dbv;

	// Check if connected
	if (!gg_isonline())
	{
		MessageBox(NULL,
			Translate("You have to be connected before you can import/export contacts from/to server."),
			GG_PROTOERROR, MB_OK | MB_ICONSTOP
		);
		return 0;
	}

	// Readup password
	if (!DBGetContactSetting(NULL, GG_PROTO, GG_KEY_PASSWORD, &dbv))
	{
		CallService(MS_DB_CRYPT_DECODESTRING, strlen(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
		password = strdup(dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	else return 0;

	if (!(uin = DBGetContactSettingDword(NULL, GG_PROTO, GG_KEY_UIN, 0)))
		return 0;

	// Making contacts list
	if (gg_userlist_request(ggThread->sess, GG_USERLIST_GET, NULL) == -1)
	{
		char error[128];
		mir_snprintf(error, sizeof(error), Translate("List cannot be imported because of error:\n\t%s"), strerror(errno));
		MessageBox(
			NULL,
			error,
			GG_PROTOERROR,
			MB_OK | MB_ICONSTOP
		);
#ifdef DEBUGMODE
		gg_netlog("gg_import_serverthread(): Cannot import list because of \"%s\".", strerror(errno));
#endif
	}
	free(password);

	return 0;
}

//////////////////////////////////////////////////////////
// remove from server
static int gg_remove_server(WPARAM wParam, LPARAM lParam)
{
	char *password;
	uin_t uin;
	DBVARIANT dbv;

	// Check if connected
	if (!ggThread->sess)
	{
		MessageBox(NULL,
			Translate("You have to be connected before you can import/export contacts from/to server."),
			GG_PROTOERROR, MB_OK | MB_ICONSTOP
		);
		return 0;
	}

	// Readup password
	if (!DBGetContactSetting(NULL, GG_PROTO, GG_KEY_PASSWORD, &dbv))
	{
		CallService(MS_DB_CRYPT_DECODESTRING, strlen(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
		password = strdup(dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	else return 0;

	if (!(uin = DBGetContactSettingDword(NULL, GG_PROTO, GG_KEY_UIN, 0)))
		return 0;

	// Making contacts list
	if (gg_userlist_request(ggThread->sess, GG_USERLIST_PUT, NULL) == -1)
	{
		char error[128];
		mir_snprintf(error, sizeof(error), Translate("List cannot be removeed because of error:\n\t%s"), strerror(errno));
		MessageBox(
			NULL,
			error,
			GG_PROTOERROR,
			MB_OK | MB_ICONSTOP
		);

#ifdef DEBUGMODE
		gg_netlog("gg_remove_serverthread(): Cannot remove list because of \"%s\".", strerror(errno));
#endif
	}

	// Set list removal
	ggListRemove = TRUE;
	free(password);

	return 0;
}


#if (_WIN32_WINNT >= 0x0500) && !defined(OPENFILENAME_SIZE_VERSION_400)
#ifndef CDSIZEOF_STRUCT
#define CDSIZEOF_STRUCT(structname, member)  (((int)((LPBYTE)(&((structname*)0)->member) - ((LPBYTE)((structname*)0)))) + sizeof(((structname*)0)->member))
#endif
#define OPENFILENAME_SIZE_VERSION_400A	CDSIZEOF_STRUCT(OPENFILENAMEA,lpTemplateName)
#define OPENFILENAME_SIZE_VERSION_400W	CDSIZEOF_STRUCT(OPENFILENAMEW,lpTemplateName)
#ifdef UNICODE
#define OPENFILENAME_SIZE_VERSION_400  OPENFILENAME_SIZE_VERSION_400W
#else
#define OPENFILENAME_SIZE_VERSION_400  OPENFILENAME_SIZE_VERSION_400A
#endif // !UNICODE
#endif // (_WIN32_WINNT >= 0x0500) && !defined(OPENFILENAME_SIZE_VERSION_400)

static int gg_import_text(WPARAM wParam, LPARAM lParam)
{
	char str[MAX_PATH] = "\0";
	OPENFILENAME ofn;
	char filter[512], *pfilter;
	struct stat st;
	FILE *f;

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner = NULL;
	ofn.hInstance = NULL;
	strncpy(filter, Translate("Text files"), sizeof(filter));
	strncat(filter, " (*.txt)", sizeof(filter) - strlen(filter));
	pfilter = filter + strlen(filter) + 1;
	if(pfilter >= filter + sizeof(filter)) return 0;
	strncpy(pfilter, "*.TXT", sizeof(filter) - (pfilter - filter));
	pfilter = pfilter + strlen(pfilter) + 1;
	if(pfilter >= filter + sizeof(filter)) return 0;
	strncpy(pfilter, Translate("All Files"), sizeof(filter) - (pfilter - filter));
	strncat(pfilter, " (*)", sizeof(filter) - (pfilter - filter) - strlen(pfilter));
	pfilter = pfilter + strlen(pfilter) + 1;
	if(pfilter >= filter + sizeof(filter)) return 0;
	strncpy(pfilter, "*", sizeof(filter) - (pfilter - filter));
	pfilter = pfilter + strlen(pfilter) + 1;
	if(pfilter >= filter + sizeof(filter)) return 0;
	*pfilter = '\0';
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = str;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.nMaxFile = sizeof(str);
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrDefExt = "txt";

#ifdef DEBUGMODE
		gg_netlog("gg_import_text()");
#endif
	if(!GetOpenFileName(&ofn)) return 0;

	f = fopen(str, "r");
	stat(str, &st);

	if(f && st.st_size)
	{
		char *contacts = malloc(st.st_size * sizeof(char));
		fread(contacts, sizeof(char), st.st_size, f);
		fclose(f);
		gg_parsecontacts(contacts);
		free(contacts);

		MessageBox(
			NULL,
			Translate("List import successful."),
			GG_PROTONAME,
			MB_OK | MB_ICONINFORMATION
		);
	}
	else
	{
		char error[128];
		mir_snprintf(error, sizeof(error), Translate("List cannot be exported to file \"%s\" because of error:\n\t%s"), str, strerror(errno));
		MessageBox(
			NULL,
			error,
			GG_PROTOERROR,
			MB_OK | MB_ICONSTOP
		);
	}

	return 0;
}

static int gg_export_text(WPARAM wParam, LPARAM lParam)
{
	char str[MAX_PATH];
	OPENFILENAME ofn;
	char filter[512], *pfilter;
	FILE *f;

	strncpy(str, Translate("contacts"), sizeof(str));
	strncat(str, ".txt", sizeof(str) - strlen(str));

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner = NULL;
	ofn.hInstance = NULL;
	strncpy(filter, Translate("Text files"), sizeof(filter));
	strncat(filter, " (*.txt)", sizeof(filter) - strlen(filter));
	pfilter = filter + strlen(filter) + 1;
	if(pfilter >= filter + sizeof(filter)) return 0;
	strncpy(pfilter, "*.TXT", sizeof(filter) - (pfilter - filter));
	pfilter = pfilter + strlen(pfilter) + 1;
	if(pfilter >= filter + sizeof(filter)) return 0;
	strncpy(pfilter, Translate("All Files"), sizeof(filter) - (pfilter - filter));
	strncat(pfilter, " (*)", sizeof(filter) - (pfilter - filter) - strlen(pfilter));
	pfilter = pfilter + strlen(pfilter) + 1;
	if(pfilter >= filter + sizeof(filter)) return 0;
	strncpy(pfilter, "*", sizeof(filter) - (pfilter - filter));
	pfilter = pfilter + strlen(pfilter) + 1;
	if(pfilter >= filter + sizeof(filter)) return 0;
	*pfilter = '\0';
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = str;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	ofn.nMaxFile = sizeof(str);
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrDefExt = "txt";

#ifdef DEBUGMODE
	gg_netlog("gg_export_text(%s).", str);
#endif
	if(!GetSaveFileName(&ofn)) return 0;

	if(f = fopen(str, "w"))
	{
		char *contacts = gg_makecontacts(0);
		fwrite(contacts, sizeof(char), strlen(contacts), f);
		fclose(f);
		free(contacts);

		MessageBox(
			NULL,
			Translate("List export successful."),
			GG_PROTONAME,
			MB_OK | MB_ICONINFORMATION
		);
	}
	else
	{
		char error[128];
		mir_snprintf(error, sizeof(error), Translate("List cannot be exported to file \"%s\" because of error:\n\t%s"), str, strerror(errno));
		MessageBox(
			NULL,
			error,
			GG_PROTOERROR,
			MB_OK | MB_ICONSTOP
		);
	}

	return 0;
}

//////////////////////////////////////////////////////////
// export to server
static int gg_export_server(WPARAM wParam, LPARAM lParam)
{
	char *password, *contacts;
	uin_t uin;
	DBVARIANT dbv;

	// Check if connected
	if (!ggThread->sess)
	{
		MessageBox(NULL,
			Translate("You have to be connected before you can import/export contacts from/to server."),
			GG_PROTOERROR, MB_OK | MB_ICONSTOP
		);
		return 0;
	}

	// Readup password
	if (!DBGetContactSetting(NULL, GG_PROTO, GG_KEY_PASSWORD, &dbv))
	{
		CallService(MS_DB_CRYPT_DECODESTRING, strlen(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
		password = strdup(dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	else return 0;

	if (!(uin = DBGetContactSettingDword(NULL, GG_PROTO, GG_KEY_UIN, 0)))
		return 0;

	// Making contacts list
	contacts = gg_makecontacts(1);

#ifdef DEBUGMODE
		gg_netlog("gg_userlist_request(%s).", contacts);
#endif

	if (gg_userlist_request(ggThread->sess, GG_USERLIST_PUT, contacts) == -1)
	{
		char error[128];
		mir_snprintf(error, sizeof(error), Translate("List cannot be exported because of error:\n\t%s"), strerror(errno));
		MessageBox(
			NULL,
			error,
			GG_PROTOERROR,
			MB_OK | MB_ICONSTOP
		);
#ifdef DEBUGMODE
		gg_netlog("gg_export_serverthread(): Cannot export list because of \"%s\".", strerror(errno));
#endif
	}

	// Set list removal
	ggListRemove = FALSE;
	free(contacts);
	free(password);

	return 0;
}

//////////////////////////////////////////////////////////
// Import menus and stuff
void gg_import_init()
{
	CLISTMENUITEM mi;
	char service[64];

	ZeroMemory(&mi, sizeof(mi));
	mi.cbSize = sizeof(mi);

	// Import from server item
	mir_snprintf(service, sizeof(service), GGS_IMPORT_SERVER, GG_PROTO);
	CreateServiceFunction(service, gg_import_server);
	mi.pszPopupName = GG_PROTONAME;
	mi.popupPosition = 500090000;
	mi.position = 600090000;
	mi.hIcon = LoadIconEx(IDI_IMPORT_SERVER);
	mi.pszName = LPGEN("Import List From &Server");
	mi.pszService = service;
	CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM) &mi);

	// Import from textfile
	mir_snprintf(service, sizeof(service), GGS_IMPORT_TEXT, GG_PROTO);
	CreateServiceFunction(service, gg_import_text);
	mi.pszPopupName = GG_PROTONAME;
	mi.popupPosition = 500090000;
	mi.position = 600090000;
	mi.hIcon = LoadIconEx(IDI_IMPORT_TEXT);
	mi.pszName = LPGEN("Import List From &Text File...");
	mi.pszService = service;
	CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM) &mi);


	// Remove from server
	mir_snprintf(service, sizeof(service), GGS_REMOVE_SERVER, GG_PROTO);
	CreateServiceFunction(service, gg_remove_server);
	mi.pszPopupName = GG_PROTONAME;
	mi.popupPosition = 500090000;
	mi.position = 600090000;
	mi.hIcon = LoadIconEx(IDI_REMOVE_SERVER);
	mi.pszName = LPGEN("&Remove List From Server");
	mi.pszService = service;
	CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM) &mi);

	// Export to server
	mir_snprintf(service, sizeof(service), GGS_EXPORT_SERVER, GG_PROTO);
	CreateServiceFunction(service, gg_export_server);
	mi.pszPopupName = GG_PROTONAME;
	mi.popupPosition = 500090000;
	mi.position = 700090000;
	mi.hIcon = LoadIconEx(IDI_EXPORT_SERVER);
	mi.pszName = LPGEN("Export List To &Server");
	mi.pszService = service;
	CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM) &mi);

	// Export to textfile
	mir_snprintf(service, sizeof(service), GGS_EXPORT_TEXT, GG_PROTO);
	CreateServiceFunction(service, gg_export_text);
	mi.pszPopupName = GG_PROTONAME;
	mi.popupPosition = 500090000;
	mi.position = 700090000;
	mi.hIcon = LoadIconEx(IDI_EXPORT_TEXT);
	mi.pszName = LPGEN("Export List To &Text File...");
	mi.pszService = service;
	CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM) &mi);
}
