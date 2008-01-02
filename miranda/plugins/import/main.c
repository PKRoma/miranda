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

#include "import.h"

#include "resource.h"

void FreeVariant( DBVARIANT* dbv );
void WriteVariant( HANDLE hContact, const char* module, const char* var, DBVARIANT* dbv );

int GroupNameExists(const char* name);
int CreateGroup(HWND hdlgProgress, BYTE type, const char* name);
BOOL IsDuplicateEvent(HANDLE hContact, DBEVENTINFO dbei);
void Utf8ToAnsi(const char *szIn, char *szOut, int cchOut);

static int ModulesLoaded(WPARAM wParam, LPARAM lParam);
static int ImportCommand(WPARAM wParam,LPARAM lParam);

#define IMPORT_CONTACTS 0
#define IMPORT_ALL      1
#define IMPORT_CUSTOM   2

struct MM_INTERFACE mmi;

int nImportOption;
int nCustomOptions;

static DWORD dwPreviousTimeStamp = -1;
static HANDLE hPreviousContact = INVALID_HANDLE_VALUE;
static HANDLE hPreviousDbEvent = NULL;

static HANDLE hHookModulesLoaded;

BOOL UnicodeDB = FALSE;

BOOL CALLBACK WizardDlgProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam);

HINSTANCE hInst;
PLUGINLINK *pluginLink;
static HWND hwndWizard = NULL;
char str[512];

PLUGININFOEX pluginInfo = {
	sizeof(PLUGININFOEX),
	"Import contacts and messages",
	PLUGIN_MAKE_VERSION(0,8,0,0),
	"Imports contacts and messages from Mirabilis ICQ and Miranda IM.",
	"Miranda team",
	"info@miranda-im.org",
	"© 2000-2008 Martin Öberg, Richard Hughes, Dmitry Kuzkin, George Hazan",
	"http://www.miranda-im.org",
	UNICODE_AWARE,
	0,	//{2D77A746-00A6-4343-BFC5-F808CDD772EA}
      {0x2d77a746, 0xa6, 0x4343, { 0xbf, 0xc5, 0xf8, 0x8, 0xcd, 0xd7, 0x72, 0xea }}
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst = hinstDLL;
	return TRUE;
}

void GetMirandaPath(char *szPath,int cbPath)
{
	char *str2;

	GetModuleFileNameA( GetModuleHandle(NULL),szPath,cbPath);
	str2=strrchr(szPath,'\\');
	if(str2!=NULL) *str2=0;
}

// szIn,   pointer to source string
// szOut,  pointer to destination buffer
// cchOut, size of destination buffer

void Utf8ToAnsi(const char *szIn, char *szOut, int cchOut)
{
	if (GetVersion() != 4 || !(GetVersion() & 0x80000000)) {
		WCHAR *wszTemp;
		int inlen;

		inlen = strlen(szIn);
		wszTemp = (WCHAR *)malloc(sizeof(WCHAR) * (inlen + 1));
		MultiByteToWideChar(CP_UTF8, 0, szIn, -1, wszTemp, inlen + 1);
		WideCharToMultiByte(CP_ACP, 0, wszTemp, -1, szOut, cchOut, NULL, NULL);
		free(wszTemp);
	}
	else {  /* this hand-rolled version isn't as good because it doesn't do DBCS */
		for (; *szIn && cchOut > 1; szIn++) {
			if (*szIn >= 0) {
				*szOut++ = *szIn;
				cchOut--;
			}
			else {
				unsigned short wideChar;

				if ((unsigned char)*szIn >= 0xe0) {
					wideChar = ((szIn[0]&0x0f) << 12)
					         | ((szIn[1]&0x3f) << 6)
							  | (szIn[2]&0x3f);
					szIn += 2;
				}
				else {
					wideChar = ((szIn[0]&0x1f) << 6)
							  | (szIn[1]&0x3f);
					szIn++;
				}
				if (wideChar >= 0x100) *szOut++ = '?';
				else *szOut++ = (char)wideChar;
				cchOut--;
		}	}

		*szOut = '\0';
}	}

// Returns TRUE if the event already exist in the database

BOOL IsDuplicateEvent(HANDLE hContact, DBEVENTINFO dbei)
{
	HANDLE hExistingDbEvent;
	DBEVENTINFO dbeiExisting;
	DWORD dwFirstEventTimeStamp;
	DWORD dwLastEventTimeStamp;
	BOOL BadDb = FALSE;

	if (!(hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDFIRST, (WPARAM)hContact, 0)))
		return FALSE;

	ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
	dbeiExisting.cbSize = sizeof(dbeiExisting);
	dbeiExisting.cbBlob = 0;
	CallService(MS_DB_EVENT_GET, (WPARAM)hExistingDbEvent, (LPARAM)&dbeiExisting);
	dwFirstEventTimeStamp = dbeiExisting.timestamp;

	if (!(hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDLAST, (WPARAM)hContact, 0)))
		return FALSE;

	ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
	dbeiExisting.cbSize = sizeof(dbeiExisting);
	dbeiExisting.cbBlob = 0;
	CallService(MS_DB_EVENT_GET, (WPARAM)hExistingDbEvent, (LPARAM)&dbeiExisting);
	dwLastEventTimeStamp = dbeiExisting.timestamp;

	// check for DB consistency
	if (dbeiExisting.timestamp < dwFirstEventTimeStamp) {
		dwLastEventTimeStamp = dwFirstEventTimeStamp;
		dwFirstEventTimeStamp = dbeiExisting.timestamp;
		BadDb = TRUE;
	}

	// compare with first timestamp
	if (dbei.timestamp < dwFirstEventTimeStamp)
		return FALSE;

	// compare with last timestamp
	if (dbei.timestamp > dwLastEventTimeStamp)
		return FALSE;

	if (hContact != hPreviousContact || BadDb) {
		hPreviousContact = hContact;
		// remember last event
		dwPreviousTimeStamp = dwLastEventTimeStamp;
		hPreviousDbEvent = hExistingDbEvent;
	}
	else {
		// fix for equal timestamps
		if (dbei.timestamp == dwPreviousTimeStamp) {
			// use last history msg
			//	dwPreviousTimeStamp = dwLastEventTimeStamp;
			//	hPreviousDbEvent = hExistingDbEvent;

			// last history msg timestamp & handle
			HANDLE hLastDbEvent = hExistingDbEvent;

			// find event with another timestamp
			hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDPREV, (WPARAM)hPreviousDbEvent, 0);
			while (hExistingDbEvent != NULL) {
				ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
				dbeiExisting.cbSize = sizeof(dbeiExisting);
				dbeiExisting.cbBlob = 0;
				CallService(MS_DB_EVENT_GET, (WPARAM)hExistingDbEvent, (LPARAM)&dbeiExisting);

				if (dbeiExisting.timestamp != dwPreviousTimeStamp)
					break;

				hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDPREV, (WPARAM)hExistingDbEvent, 0);
			}

			if (hExistingDbEvent != NULL) 			{
				// use found msg
				dwPreviousTimeStamp = dbeiExisting.timestamp;
				hPreviousDbEvent = hExistingDbEvent;
			}
			else {
				// use last msg
				dwPreviousTimeStamp = dwLastEventTimeStamp;
				hPreviousDbEvent = hLastDbEvent;
				hExistingDbEvent = hLastDbEvent;
			}
		}
		else // use previous saved
			hExistingDbEvent = hPreviousDbEvent;
	}

	if (dbei.timestamp <= dwPreviousTimeStamp) { 	// look back
		while (hExistingDbEvent != NULL) {
			ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
			dbeiExisting.cbSize = sizeof(dbeiExisting);
			dbeiExisting.cbBlob = 0;
			CallService(MS_DB_EVENT_GET, (WPARAM)hExistingDbEvent, (LPARAM)&dbeiExisting);

			if (dbei.timestamp > dbeiExisting.timestamp) {
				hPreviousDbEvent = hExistingDbEvent;
				dwPreviousTimeStamp = dbeiExisting.timestamp;
				return FALSE;
			}

			// Compare event with import candidate
			if ( dbei.timestamp == dbeiExisting.timestamp && 
				  dbei.eventType == dbeiExisting.eventType && 
				  dbei.cbBlob == dbeiExisting.cbBlob ) {
				hPreviousDbEvent = hExistingDbEvent;
				dwPreviousTimeStamp = dbeiExisting.timestamp;
				return TRUE;
			}

			// Get previous event in chain
			hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDPREV, (WPARAM)hExistingDbEvent, 0);
		}
	}
	else { 	// look forward
		while (hExistingDbEvent != NULL) {
			ZeroMemory(&dbeiExisting, sizeof(dbeiExisting));
			dbeiExisting.cbSize = sizeof(dbeiExisting);
			dbeiExisting.cbBlob = 0;
			CallService(MS_DB_EVENT_GET, (WPARAM)hExistingDbEvent, (LPARAM)&dbeiExisting);

			if (dbei.timestamp < dbeiExisting.timestamp) {
				hPreviousDbEvent = hExistingDbEvent;
				dwPreviousTimeStamp = dbeiExisting.timestamp;
				return FALSE;
			}

			// Compare event with import candidate
			if ( dbei.timestamp == dbeiExisting.timestamp && 
				  dbei.eventType == dbeiExisting.eventType && 
				  dbei.cbBlob == dbeiExisting.cbBlob ) {
				hPreviousDbEvent = hExistingDbEvent;
				dwPreviousTimeStamp = dbeiExisting.timestamp;
				return TRUE;
			}

			// Get next event in chain
			hExistingDbEvent = (HANDLE)CallService(MS_DB_EVENT_FINDNEXT, (WPARAM)hExistingDbEvent, 0);
	}	}

	return FALSE;
}

HANDLE HContactFromNumericID(char* pszProtoName, char* pszSetting, DWORD dwID)
{
	HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL) {
		char* szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if (szProto != NULL && !strcmp(szProto, pszProtoName))
			if (DBGetContactSettingDword(hContact, pszProtoName, pszSetting, 0) == dwID)
				return hContact;

		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}

	return INVALID_HANDLE_VALUE;
}

HANDLE HContactFromID(char* pszProtoName, char* pszSetting, char* pszID)
{
	HANDLE hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact != NULL) {
		char* szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if ( !lstrcmpA(szProto, pszProtoName)) {
			DBVARIANT dbv;
			if (DBGetContactSettingString(hContact, pszProtoName, pszSetting, &dbv) == 0) {
				if (strcmp(pszID, dbv.pszVal) == 0) {
					mir_free(dbv.pszVal);
					return hContact;
				}
				DBFreeVariant(&dbv);
			}	
		}

		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}

	return INVALID_HANDLE_VALUE;
}

HANDLE AddContact(HWND hdlgProgress, char* pszProtoName, char* pszUniqueSetting, 
						DBVARIANT* id, DBVARIANT* nick, DBVARIANT* group)
{
	HANDLE hContact;
	char szid[ 40 ];
	char* pszUserID = ( id->type == DBVT_DWORD ) ? ltoa( id->dVal, szid, 10 ) : id->pszVal;

	hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
	if ( CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)pszProtoName) != 0) {
		CallService(MS_DB_CONTACT_DELETE, (WPARAM)hContact, 0);
		AddMessage( LPGEN("Failed to add %s contact %s"), pszProtoName, pszUserID );
		return INVALID_HANDLE_VALUE;
	}

	WriteVariant( hContact, pszProtoName, pszUniqueSetting, id );

	if ( group->type ) {
		if ( !GroupNameExists( group->pszVal ))
			CreateGroup( hdlgProgress, group->type, group->pszVal );
		WriteVariant( hContact, "CList", "Group", group );
	}

	if ( nick->type && nick->pszVal[0] ) {
		WriteVariant( hContact, "CList", "MyHandle", nick );
		AddMessage( LPGEN("Added %s contact %s, '%s'"), pszProtoName, pszUserID, nick->pszVal );
	}
	AddMessage( LPGEN("Added %s contact %s"), pszProtoName, pszUserID );

	FreeVariant( id );
	FreeVariant( nick );
	FreeVariant( group );
	return hContact;
}

HANDLE HistoryImportFindContact(HWND hdlgProgress, DWORD uin, int addUnknown)
{
	HANDLE hContact = HContactFromNumericID(ICQOSCPROTONAME, "UIN", uin);
	if (hContact == NULL) {
		AddMessage( LPGEN("Ignored event from/to self"));
		return INVALID_HANDLE_VALUE;
	}

	if (hContact != INVALID_HANDLE_VALUE)
		return hContact;

	if (!addUnknown)
		return INVALID_HANDLE_VALUE;

	hContact = (HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
	CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM)ICQOSCPROTONAME);
	DBWriteContactSettingDword(hContact, ICQOSCPROTONAME, "UIN", uin);
	AddMessage( LPGEN("Added contact %u (found in history)"), uin );
	return hContact;
}

// ------------------------------------------------
// Checks if a group already exists in Miranda with
// the specified name.
// ------------------------------------------------
// Returns 1 if a group with the name exists, returns
// 0 otherwise.

int GroupNameExists(const char* name)
{
	char idstr[33];
	DBVARIANT dbv;
	int i;

	for (i = 0; ; i++) {
		itoa(i, idstr, 10);
		if (DBGetContactSettingString(NULL, "CListGroups", idstr, &dbv))
			break;

		if ( !lstrcmpA( dbv.pszVal + 1, name )) {
			DBFreeVariant( &dbv );
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

int CreateGroup(HWND hdlgProgress, BYTE type, const char* name)
{
	// Is this a duplicate?
	if ( !GroupNameExists( name )) {
		DBVARIANT dbv;
		// Find an unused id
		int groupId;
		char groupIdStr[11];
		for (groupId = 0; ; groupId++) {
			itoa(groupId, groupIdStr,10);
			if (DBGetContactSettingString(NULL, "CListGroups", groupIdStr, &dbv))
				break;
			DBFreeVariant(&dbv);
		}

      dbv.type = type;
		dbv.pszVal = ( char* )alloca( strlen( name )+2 );
		dbv.pszVal[ 0 ] = 1;
		strcpy( dbv.pszVal+1, name );      
		WriteVariant( NULL, "CListGroups", groupIdStr, &dbv );
		return 1;
	}

	AddMessage( LPGEN("Skipping duplicate group %s."), name );
	return 0;
}

BOOL IsProtocolLoaded(char* pszProtocolName)
{
	return CallService(MS_PROTO_ISPROTOCOLLOADED, 0, (LPARAM)pszProtocolName) ? TRUE : FALSE;
}

static int ImportCommand(WPARAM wParam,LPARAM lParam)
{
	if (IsWindow(hwndWizard)) {
		SetForegroundWindow(hwndWizard);
		SetFocus(hwndWizard);
	}
	else hwndWizard = CreateDialog(hInst, MAKEINTRESOURCE(IDD_WIZARD), NULL, WizardDlgProc);

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MirandaPluginInfoEx - returns an information about a plugin

__declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 7, 0, 0))
		return NULL;

	return &pluginInfo;
}

/////////////////////////////////////////////////////////////////////////////////////////
// MirandaPluginInterfaces - returns the protocol interface to the core

static const MUUID interfaces[] = {MIID_IMPORT, MIID_LAST};

__declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Performs a primary set of actions upon plugin loading

static int ModulesLoaded(WPARAM wParam, LPARAM lParam)
{
	int nProtocols = 0;
	int n;
	PROTOCOLDESCRIPTOR **ppProtos = NULL;

	if (DBGetContactSettingByte(NULL, IMPORT_MODULE, IMP_KEY_FR, 0))
		return 0;

	// Only autorun import wizard if at least one protocol is installed
	CallService(MS_PROTO_ENUMPROTOCOLS, (WPARAM)&nProtocols, (LPARAM)&ppProtos);
	for (n=0; n < nProtocols; n++) {
		if (ppProtos[n]->type == PROTOTYPE_PROTOCOL) {
			CallService(IMPORT_SERVICE, 0, 0);
			DBWriteContactSettingByte(NULL, IMPORT_MODULE, IMP_KEY_FR, 1);
			break;
	}	}

	if (hHookModulesLoaded)
		UnhookEvent(hHookModulesLoaded);

	UnicodeDB = ServiceExists(MS_DB_CONTACT_GETSETTING_STR);
	return 0;
}

int __declspec(dllexport) Load(PLUGINLINK *link)
{
	pluginLink = link;
	mir_getMMI( &mmi );

	CreateServiceFunction(IMPORT_SERVICE, ImportCommand);
	{
		CLISTMENUITEM mi;
		ZeroMemory(&mi, sizeof(mi));
		mi.cbSize = sizeof(mi);
		mi.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_IMPORT));
		mi.pszName = Translate("&Import...");
		mi.position = 500050000;
		mi.pszService = IMPORT_SERVICE;
		CallService(MS_CLIST_ADDMAINMENUITEM, 0, (LPARAM)&mi);
	}
	HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
	{
		INITCOMMONCONTROLSEX icex;
		icex.dwSize = sizeof(icex);
		icex.dwICC = ICC_DATE_CLASSES;
		InitCommonControlsEx(&icex);
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Unload a plugin

int __declspec(dllexport) Unload(void)
{
	if (hHookModulesLoaded)
		UnhookEvent(hHookModulesLoaded);

	return 0;
}
