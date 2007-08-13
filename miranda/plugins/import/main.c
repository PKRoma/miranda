/*

Import plugin for Miranda IM

Copyright (C) 2001-2005 Martin Öberg, Richard Hughes, Roland Rabien & Tristan Van de Vreede

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



#include <windows.h>
#include <string.h>
#include "resource.h"
#include "import.h"
#include <m_database.h>
#include <m_clist.h>
#include <m_protomod.h>

int GroupNameExists(const char *name);
int CreateGroup(HWND hdlgProgress, const char *groupName);
BOOL IsDuplicateEvent(HANDLE hContact, DBEVENTINFO dbei);
void Utf8ToAnsi(const char *szIn, char *szOut, int cchOut);

static int ModulesLoaded(WPARAM wParam, LPARAM lParam);
static int ImportCommand(WPARAM wParam,LPARAM lParam);

#define IMPORT_CONTACTS 0
#define IMPORT_ALL      1
#define IMPORT_CUSTOM   2

int nImportOption;
int nCustomOptions;


static HANDLE hHookModulesLoaded;


BOOL CALLBACK WizardDlgProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam);

HINSTANCE hInst;
PLUGINLINK *pluginLink;
static HWND hwndWizard = NULL;
char str[512];

PLUGININFOEX pluginInfo = {
	sizeof(PLUGININFOEX),
		"Import contacts and messages",
		PLUGIN_MAKE_VERSION(0,9,2,2),
		"Imports contacts and messages from Mirabilis ICQ and Miranda IM.",
		"Martin Öberg",
		"info at miranda-im.org",
		"© 2000-2005 Martin Öberg, Richard Hughes",
		"http://www.miranda-im.org",
		0,
		0,
        {0x2d77a746, 0xa6, 0x4343, { 0xbf, 0xc5, 0xf8, 0x8, 0xcd, 0xd7, 0x72, 0xea }} //{2D77A746-00A6-4343-BFC5-F808CDD772EA}
};


BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst=hinstDLL;
	return TRUE;
}


void GetMirandaPath(char *szPath,int cbPath)
{
	char *str2;

	GetModuleFileName(GetModuleHandle(NULL),szPath,cbPath);
	str2=strrchr(szPath,'\\');
	if(str2!=NULL) *str2=0;
}



// szIn,   pointer to source string
// szOut,  pointer to destination buffer
// cchOut, size of destination buffer
void Utf8ToAnsi(const char *szIn, char *szOut, int cchOut)
{
	if (GetVersion() != 4 || !(GetVersion() & 0x80000000))
	{
		WCHAR *wszTemp;
		int inlen;

		inlen = strlen(szIn);
		wszTemp = (WCHAR *)malloc(sizeof(WCHAR) * (inlen + 1));
		MultiByteToWideChar(CP_UTF8, 0, szIn, -1, wszTemp, inlen + 1);
		WideCharToMultiByte(CP_ACP, 0, wszTemp, -1, szOut, cchOut, NULL, NULL);
		free(wszTemp);
	}
	else   /* this hand-rolled version isn't as good because it doesn't do DBCS */
	{
		for (; *szIn && cchOut > 1; szIn++)
		{
			if (*szIn >= 0)
			{
				*szOut++ = *szIn;
				cchOut--;
			}
			else
			{
				unsigned short wideChar;

				if ((unsigned char)*szIn >= 0xe0)
				{
					wideChar = ((szIn[0]&0x0f) << 12)
					         | ((szIn[1]&0x3f) << 6)
							  | (szIn[2]&0x3f);
					szIn += 2;
				}
				else
				{
					wideChar = ((szIn[0]&0x1f) << 6)
							  | (szIn[1]&0x3f);
					szIn++;
				}
				if (wideChar >= 0x100) *szOut++ = '?';
				else *szOut++ = (char)wideChar;
				cchOut--;
			}
		}
		*szOut = '\0';
	}
}



// Returns TRUE if the event already exist in the database
BOOL IsDuplicateEvent(HANDLE hContact, DBEVENTINFO dbei)
{
	
	HANDLE hExistingDbEvent;
	DBEVENTINFO dbeiExisting;

	if (!(hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDFIRST, (WPARAM)hContact, 0)))
		return FALSE;
	ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
	dbeiExisting.cbSize = sizeof(dbeiExisting);
	dbeiExisting.cbBlob = 0;
	CallService(MS_DB_EVENT_GET, (WPARAM)hExistingDbEvent, (LPARAM)&dbeiExisting);
	if (dbei.timestamp < dbeiExisting.timestamp)
		return FALSE;

	if (!(hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDLAST, (WPARAM)hContact, 0)))
		return FALSE;
	ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
	dbeiExisting.cbSize = sizeof(dbeiExisting);
	dbeiExisting.cbBlob = 0;
	CallService(MS_DB_EVENT_GET, (WPARAM)hExistingDbEvent, (LPARAM)&dbeiExisting);
	if (dbei.timestamp > dbeiExisting.timestamp)
		return FALSE;

	while (hExistingDbEvent != NULL) {
		ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
		dbeiExisting.cbSize = sizeof(dbeiExisting);
		dbeiExisting.cbBlob = 0;
		CallService(MS_DB_EVENT_GET, (WPARAM)hExistingDbEvent, (LPARAM)&dbeiExisting);
		
		if (dbei.timestamp > dbeiExisting.timestamp)
			return FALSE;

			// Compare event with import candidate
		if ((dbei.timestamp == dbeiExisting.timestamp) &&
			(dbei.eventType == dbeiExisting.eventType) &&
			(dbei.cbBlob == dbeiExisting.cbBlob)) {
			return TRUE;
			break;
		}
		
		// Get next event in chain
		hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDPREV, (WPARAM)hExistingDbEvent, 0);
	}

	return FALSE;

}



HANDLE HContactFromNumericID(char* pszProtoName, char* pszSetting, DWORD dwID)
{

	HANDLE hContact;
	char* szProto;


	// Owner?
	if (DBGetContactSettingDword(NULL, pszProtoName, pszSetting, 0) == dwID)
		return NULL;

	hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL)
	{
		szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if (szProto != NULL && !strcmp(szProto, pszProtoName))
		{
			if (DBGetContactSettingDword(hContact, pszProtoName, pszSetting, 0) == dwID)
				return hContact;
		}
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}

	return INVALID_HANDLE_VALUE;
}



HANDLE HContactFromID(char* pszProtoName, char* pszSetting, char* pszID)
{

	HANDLE hContact;
	char* szProto;
	DBVARIANT dbv;


	// Owner?
//	if (DBGetContactSettingDword(NULL, pszProtoName, pszSetting, 0) == dwID)
//		return NULL;

	hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL)
	{
		szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if ((szProto != NULL) && (!strcmp(szProto, pszProtoName)))
		{
			if (DBGetContactSetting(hContact, pszProtoName, pszSetting, &dbv) == 0)
			{
				if (dbv.type==DBVT_ASCIIZ && strcmp(pszID, dbv.pszVal) == 0)
				{
					DBFreeVariant(&dbv);
					return hContact;
				}
				DBFreeVariant(&dbv);
			}
		}
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}

	return INVALID_HANDLE_VALUE;
}



HANDLE AddNumericContact(HWND hdlgProgress, char* pszProtoName, char* pszUniqueSetting, DWORD dwID, char* pzsNick, char* pzsGroupName)
{

	HANDLE hContact = INVALID_HANDLE_VALUE;

	
	hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
	if (CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)pszProtoName) != 0)
	{
		CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);
		wsprintf(str, "Failed to add %s contact %u", pszProtoName, dwID);
		AddMessage(str);
		return INVALID_HANDLE_VALUE;
	}
	
	// Write ID
	DBWriteContactSettingDword(hContact, pszProtoName, pszUniqueSetting, dwID);

	// Write group
	if (pzsGroupName)
	{
		if (!GroupNameExists(pzsGroupName))
			CreateGroup(hdlgProgress, pzsGroupName);
		DBWriteContactSettingString(hContact, "CList", "Group", pzsGroupName);
	}

	// Write nick
	if (pzsNick && pzsNick[0])
	{
		DBWriteContactSettingString(hContact, "CList", "MyHandle", pzsNick);
		wsprintf(str, "Added %s contact %u, '%s'", pszProtoName, dwID, pzsNick);
	}
	else
	{
		wsprintf(str, "Added %s contact %u", pszProtoName, dwID);
	}
	
	AddMessage(str);

		
	return hContact;

}



HANDLE AddContact(HWND hdlgProgress, char* pszProtoName, char* pszUniqueSetting, char* pszID, char* pzsNick, char* pzsGroupName)
{

	HANDLE hContact = INVALID_HANDLE_VALUE;


	hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
	if (CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)pszProtoName) != 0)
	{
		CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);
		wsprintf(str, "Failed to add %s contact %s", pszProtoName, pszID);
		AddMessage(str);
		return INVALID_HANDLE_VALUE;
	}
	
	DBWriteContactSettingString(hContact, pszProtoName, pszUniqueSetting, pszID);

	if (pzsGroupName)
	{
		if (!GroupNameExists(pzsGroupName))
			CreateGroup(hdlgProgress, pzsGroupName);
		DBWriteContactSettingString(hContact, "CList", "Group", pzsGroupName);
	}

	if (pzsNick && pzsNick[0])
	{
		DBWriteContactSettingString(hContact, "CList", "MyHandle", pzsNick);
		wsprintf(str,"Added %s contact %s, '%s'", pszProtoName, pszID, pzsNick);
	}
	else
		wsprintf(str,"Added %s contact %s", pszProtoName, pszID);
	
	AddMessage(str);


	return hContact;

}



HANDLE HistoryImportFindContact(HWND hdlgProgress, DWORD uin, int addUnknown)
{

	HANDLE hContact;
	char str[64];

	hContact = HContactFromNumericID(ICQOSCPROTONAME, "UIN", uin);

	if (hContact == NULL) {
		AddMessage("Ignored event from/to self");
		return INVALID_HANDLE_VALUE;
	}
	if (hContact != INVALID_HANDLE_VALUE) return hContact;
	if (!addUnknown) return INVALID_HANDLE_VALUE;

	hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
	CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)ICQOSCPROTONAME);
	DBWriteContactSettingDword(hContact, ICQOSCPROTONAME, "UIN", uin);
	wsprintf(str, "Added contact %u (found in history)", uin);
	AddMessage(str);
	return hContact;

}


// ------------------------------------------------
// Checks if a group already exists in Miranda with
// the specified name.
// ------------------------------------------------
// Returns 1 if a group with the name exists, returns
// 0 otherwise.
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


// ------------------------------------------------
// Creates a group with a specified name in the 
// Miranda contact list.
// ------------------------------------------------
// Returns 1 if successful and 0 when it fails.
int CreateGroup(HWND hdlgProgress, const char *groupName)
{
	int groupId;
	char groupIdStr[11];
	DBVARIANT dbv;
	char groupName2[127];
	char str[256];
	
	// Is this a duplicate?
	if (!GroupNameExists(groupName)) {
		lstrcpyn(groupName2 + 1, groupName, strlen(groupName) + 1);
		
		// Find an unused id
		for (groupId = 0; ; groupId++) {
			itoa(groupId, groupIdStr,10);
			if (DBGetContactSetting(NULL, "CListGroups", groupIdStr, &dbv))
				break;
			DBFreeVariant(&dbv);
		}

		wsprintf(str, "Added group %s.", groupName2 + 1);
		AddMessage(str);
		
		groupName2[0] = 1|GROUPF_EXPANDED;  // 1 is required so we never get '\0'
		DBWriteContactSettingString(NULL, "CListGroups", groupIdStr, groupName2);
		return 1;
	}
	else {
		wsprintf(str,"Skipping duplicate group %s.", groupName);
		AddMessage(str);
	}

	return 0;
}



BOOL IsProtocolLoaded(char* pszProtocolName)
{
	
	if (CallService(MS_PROTO_ISPROTOCOLLOADED, 0, (LPARAM)pszProtocolName))
		return TRUE;
	else
		return FALSE;
	
}



static int ImportCommand(WPARAM wParam,LPARAM lParam)
{

	if (IsWindow(hwndWizard))
	{
		SetForegroundWindow(hwndWizard);
		SetFocus(hwndWizard);
	}
	else
	{
		hwndWizard = CreateDialog(hInst, MAKEINTRESOURCE(IDD_WIZARD), NULL, WizardDlgProc);
	}

	return 0;

}


__declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{

	if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 4, 0, 0))
		return NULL;
	
	return &pluginInfo;
	
}

static const MUUID interfaces[] = {MIID_IMPORT, MIID_LAST};
__declspec(dllexport) const MUUID * MirandaPluginInterfaces(void)
{
    
	return interfaces;
    
}

int __declspec(dllexport) Load(PLUGINLINK *link)
{

	CLISTMENUITEM mi;

	pluginLink = link;

	CreateServiceFunction(IMPORT_SERVICE, ImportCommand);

	ZeroMemory(&mi, sizeof(mi));
	mi.cbSize = sizeof(mi);
	mi.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_IMPORT));
	mi.pszName = LPGEN("&Import...");
	mi.position = 500050000;
	mi.pszService = IMPORT_SERVICE;
	CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi);

	HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);

	return 0;

}


int __declspec(dllexport) Unload(void)
{

	if (hHookModulesLoaded)
		UnhookEvent(hHookModulesLoaded);

	return 0;
}


static int ModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	
	int nProtocols = 0;
	int n;
	PROTOCOLDESCRIPTOR **ppProtos = NULL;


    if (DBGetContactSettingByte(NULL, IMPORT_MODULE, IMP_KEY_FR, 0))
        return 0;

	// Only autorun import wizard if at least one protocol is installed
	CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&nProtocols, (LPARAM)&ppProtos);
	for (n=0; n < nProtocols; n++)
	{
		if (ppProtos[n]->type == PROTOTYPE_PROTOCOL)
		{
			CallService(IMPORT_SERVICE, 0, 0);
			DBWriteContactSettingByte(NULL, IMPORT_MODULE, IMP_KEY_FR, 1);
			break;
		}
	}

	if (hHookModulesLoaded)
		UnhookEvent(hHookModulesLoaded);

	return 0;

}
