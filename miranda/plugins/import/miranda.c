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


// ==============
// == INCLUDES ==
// ==============

#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <stddef.h> // offsetof()
#include "resource.h"
#include "import.h"
#include <m_database.h>
#include <m_protocols.h>
#include <m_protosvc.h>
#include <m_utils.h>
#include "mirandadb0700.h"


// ======================
// == GLOBAL FUNCTIONS ==
// ======================

HANDLE HContactFromNumericID(char* pszProtoName, char* pszSetting, DWORD dwID);
HANDLE HContactFromID(char* pszProtoName, char* pszSetting, char* pszID);

HANDLE AddNumericContact(HWND hdlgProgress, char* pszProtoName, char* pszUniqueSetting, DWORD dwID, char* pzsNick, char* pzsGroupName);
HANDLE AddContact(HWND hdlgProgress, char* pszProtoName, char* pszUniqueSetting, char* pszID, char* pzsNick, char* pzsGroupName);

BOOL IsProtocolLoaded(char* pszProtocolName);
BOOL IsDuplicateEvent(HANDLE hContact, DBEVENTINFO dbei);
int CreateGroup(HWND hdlgProgress, const char *groupName);
void GetMirandaPath(char *szPath,int cbPath);

BOOL CALLBACK ImportTypePageProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK FinishedPageProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK ProgressPageProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK MirandaOptionsPageProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam);


// =====================
// == LOCAL FUNCTIONS ==
// =====================


void MirandaImport(HWND hdlgProgress);
int CheckFileFormat(HANDLE hFile);
static HANDLE ImportContact(HANDLE hDbFile, struct DBContact Contact);
static void ImportHistory(HANDLE hDbFile, struct DBContact Contact);
static int ImportGroups(HANDLE hDbFile, struct DBHeader *pdbHeader);

// Comment: The Find* functions only return a file offset.
//          The Get* functions actually reads the requested
//          data from the file and gives you a pointer to a structure
//          containing the data.

DWORD FindFirstContact(struct DBHeader* pDbHeader);
DWORD FindNextContact(struct DBContact* pDbContact);
DWORD FindNextEvent(HANDLE hDbFile, DWORD dwOffset);
DWORD FindOwnerContact(struct DBHeader* pDbHeader);

int GetContactCount(struct DBHeader* pDbHeader);
BOOL GetContact(HANDLE hDbFile, DWORD dwOffset, struct DBContact* pDbContact);
BOOL GetSetting(HANDLE hDbFile, struct DBContact* pDbContact, char* pszModuleName, char* pszSettingName, void** pValue, BYTE* pType);
char* GetNextSetting(char* pDbSetting);
BOOL GetSettings(HANDLE hDbFile, DWORD dwOffset, struct DBContactSettings** pDbSettings);
struct DBContactSettings* GetSettingsGroupByModuleName(HANDLE hdbFile, struct DBContact* pDbContact, char* pszName);
DWORD GetBlobSize(struct DBContactSettings* pDbSettings);
void* GetSettingByName(struct DBContactSettings* pDbSettings, char* pszSettingName);
BYTE GetSettingTypeByName(struct DBContactSettings* pDbSettings, char* pszSettingName);
void* GetSettingValue(char* pBlob);

BOOL GetEvent(HANDLE hDbFile, DWORD dwOffset, DBEVENTINFO* pDBEI);
char* GetName(HANDLE hDbFile, DWORD dwOffset);


// ======================
// == GLOBAL VARIABLES ==
// ======================

extern void (*DoImport)(HWND);
extern int nImportOption;
extern int nCustomOptions;


// =====================
// == LOCAL VARIABLES ==
// =====================

char importFile[MAX_PATH];
HWND hdlgProgress;
char str[512];
int importOptions = 0;
DWORD dwFileSize;

int nDupes;
int nContactsCount;
int nMessagesCount;
int nGroupsCount;
int nSkippedEvents;

// =============
// == DEFINES ==
// =============


// Supported database versions
#define DB_INVALID 0x00000000  // Unknown or corrupted DAT
#define DB_000700  0x00000700  // Miranda 0.1.0.0 - 0.1.2.2+

// DAT file signature
struct DBSignature {
  char name[15];
  BYTE eof;
};
static struct DBSignature dbSignature={"Miranda ICQ DB",0x1A};


// ====================
// ====================
// == IMPLEMENTATION ==
// ====================
// ====================

static void SearchForLists(HWND hdlg,const char *mirandaPath,const char *pattern,const char *type)
{

	HANDLE hFind;
	WIN32_FIND_DATA fd;
	char szSearchPath[MAX_PATH];
	char szRootName[MAX_PATH];
	char* str2;
	int i;


	_snprintf(szSearchPath, sizeof(szSearchPath), "%s\\%s", mirandaPath, pattern);
	hFind = FindFirstFile(szSearchPath, &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			lstrcpy(szRootName, fd.cFileName);
			str2 = strrchr(szRootName, '.');
			if (str2 != NULL)
				*str2 = 0;
			lstrcat(szRootName, type);
			i = SendDlgItemMessage(hdlg, IDC_LIST, LB_ADDSTRING, 0, (LPARAM)szRootName);
			str2 = (char*)malloc(lstrlen(mirandaPath) + 2 + lstrlen(fd.cFileName));
			wsprintf(str2, "%s\\%s", mirandaPath, fd.cFileName);
			SendDlgItemMessage(hdlg, IDC_LIST, LB_SETITEMDATA, i, (LPARAM)str2);
		} 
			while(FindNextFile(hFind,&fd));

		FindClose(hFind);
}	}

BOOL CALLBACK MirandaPageProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch(message) {
	case WM_INITDIALOG:
		{
			char szMirandaPath[MAX_PATH];
			TranslateDialogDefault(hdlg);
			GetMirandaPath(szMirandaPath,sizeof(szMirandaPath));
			SearchForLists(hdlg,szMirandaPath,"*.dat"," (Miranda IM v0.x)");
			return TRUE;
		}

	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDC_BACK:
			PostMessage(GetParent(hdlg),WIZM_GOTOPAGE,IDD_IMPORTTYPE,(LPARAM)ImportTypePageProc);
			break;
		case IDOK:
			{
				char filename[MAX_PATH];

				GetDlgItemText(hdlg, IDC_FILENAME, filename, sizeof(filename));
				if (_access(filename, 4)) {
					MessageBox(hdlg, Translate("The given file does not exist. Please check that you have entered the name correctly."), Translate("Miranda Import"), MB_OK);
					break;
				}
				lstrcpy(importFile, filename);
				PostMessage(GetParent(hdlg),WIZM_GOTOPAGE,IDD_OPTIONS,(LPARAM)MirandaOptionsPageProc);
				break;
			}
		case IDCANCEL:
			PostMessage(GetParent(hdlg),WM_CLOSE,0,0);
			break;
		case IDC_LIST:
			if(HIWORD(wParam)==LBN_SELCHANGE) {
				int sel = SendDlgItemMessage(hdlg, IDC_LIST, LB_GETCURSEL, 0, 0);
				if (sel == LB_ERR) break;
				SetDlgItemText(hdlg, IDC_FILENAME, (char*)SendDlgItemMessage(hdlg, IDC_LIST, LB_GETITEMDATA, sel, 0));
			}
			break;
		case IDC_OTHER:
			{
				OPENFILENAME ofn;
				char str[MAX_PATH];

				GetDlgItemText(hdlg,IDC_FILENAME,str,sizeof(str));
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = hdlg;
				ofn.hInstance = NULL;
				ofn.lpstrFilter = Translate("Miranda IM database (*.dat)\0*.DAT\0All Files (*)\0*\0");
				ofn.lpstrDefExt = "dat";
				ofn.lpstrFile = str;
				ofn.Flags = OFN_FILEMUSTEXIST;
				ofn.nMaxFile = sizeof(str);
				ofn.nMaxFileTitle = MAX_PATH;
				if (GetOpenFileName(&ofn))
					SetDlgItemText(hdlg,IDC_FILENAME,str);
				break;
			}
		}
		break;
		case WM_DESTROY:
			{
				int i;

				for(i=SendDlgItemMessage(hdlg,IDC_LIST,LB_GETCOUNT,0,0)-1;i>=0;i--)
					free((char*)SendDlgItemMessage(hdlg,IDC_LIST,LB_GETITEMDATA,i,0));
				break;
			}
	}

	return FALSE;
}


BOOL CALLBACK MirandaOptionsPageProc(HWND hdlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	switch(message) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hdlg);
		EnableWindow(GetDlgItem(hdlg,IDC_RADIO_ALL), TRUE);
		EnableWindow(GetDlgItem(hdlg,IDC_STATIC_ALL), TRUE);
		EnableWindow(GetDlgItem(hdlg,IDC_RADIO_CONTACTS), TRUE);
		EnableWindow(GetDlgItem(hdlg,IDC_STATIC_CONTACTS), TRUE);
		CheckDlgButton(hdlg,IDC_RADIO_ALL,BST_CHECKED);
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDC_BACK:
			PostMessage(GetParent(hdlg),WIZM_GOTOPAGE,IDD_MIRANDADB,(LPARAM)MirandaPageProc);
			break;
		case IDOK:
			if(IsDlgButtonChecked(hdlg,IDC_RADIO_ALL)) {
				nImportOption = IMPORT_ALL;
				nCustomOptions = IOPT_MSGSENT|IOPT_MSGRECV|IOPT_URLSENT|IOPT_URLRECV;
				DoImport = MirandaImport;
				PostMessage(GetParent(hdlg),WIZM_GOTOPAGE,IDD_PROGRESS,(LPARAM)ProgressPageProc);
				break;
			}
			if(IsDlgButtonChecked(hdlg,IDC_RADIO_CONTACTS)) {
				nImportOption = IMPORT_CONTACTS;
				nCustomOptions = 0;
				DoImport = MirandaImport;
				PostMessage(GetParent(hdlg),WIZM_GOTOPAGE,IDD_PROGRESS,(LPARAM)ProgressPageProc);
				break;
			}
			break;
		case IDCANCEL:
			PostMessage(GetParent(hdlg), WM_CLOSE, 0, 0);
			break;
		}
		break;
	}
	return FALSE;
}


#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif


// Read header from file, returns null on failure
struct DBHeader* GetHeader(HANDLE hDbFile)
{
	struct DBHeader* pdbHeader;
	DWORD dwBytesRead;

	if (!(pdbHeader = calloc(1, sizeof(struct DBHeader))))
		return NULL;

	// Goto start of file
	if (SetFilePointer(hDbFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return FALSE;

	// Read header
	if ((!ReadFile(hDbFile, pdbHeader, sizeof(struct DBHeader), &dwBytesRead, NULL)) ||
		(dwBytesRead != sizeof(struct DBHeader)))
		return NULL;

	// Return pointer to header
	return pdbHeader;
};

int CheckFileFormat(HANDLE hDbFile)
{
	struct DBHeader* pdbHeader;

	// Read header
	if (!(pdbHeader = GetHeader(hDbFile)))
		return DB_INVALID;

	// Check header signature
	if (memcmp(pdbHeader->signature, &dbSignature, sizeof(pdbHeader->signature)))
	{
		AddMessage("Signature mismatch");
		free(pdbHeader);
		return DB_INVALID;
	}

	// Determine Miranda version
	switch (pdbHeader->version) {
	case DB_000700:
		AddMessage("This looks like a Miranda database, version 0.1.0.0 or above.");
		free(pdbHeader);
		return DB_000700;

	default:
		AddMessage("Version mismatch");
		free(pdbHeader);
		return DB_INVALID;
}	}

// High level Miranda DB access functions

// Return true if pValue points to the requested value
//		if (!FindSetting(&Contact, ICQOSCPROTONAME, "uin", &UIN))
BOOL GetSetting(HANDLE hDbFile, struct DBContact* pDbContact, char* pszModuleName, char* pszSettingName, void** pValue, BYTE* bType)
{
	struct DBContactSettings* pDbSettings = NULL;

	if (pDbSettings = GetSettingsGroupByModuleName(hDbFile, pDbContact, pszModuleName))
	{
		if (*pValue = GetSettingByName(pDbSettings, pszSettingName))
		{
		  *bType = GetSettingTypeByName(pDbSettings, pszSettingName);

			free(pDbSettings);
			return TRUE;
		}
#ifdef _DEBUG
		else
		{
			_snprintf(str, sizeof(str), "Failed to find setting %s", pszSettingName);
			AddMessage(str);
		}
#endif
		free(pDbSettings);
	}
#ifdef _DEBUG
	else
	{
		_snprintf(str, sizeof(str), "Failed to find module %s", pszModuleName);
		AddMessage(str);
	}
#endif

	// Search failed
	*pValue = NULL;
	*bType = DBVT_DELETED;

	return FALSE;

}

// **
// ** CONTACT CHAIN
// **

// Return offset to first contact
DWORD FindFirstContact(struct DBHeader* pDbHeader)
{
	if (!pDbHeader)
		return 0;

	return pDbHeader->ofsFirstContact;
}

DWORD FindOwnerContact(struct DBHeader* pDbHeader)
{
	if (!pDbHeader)
		return 0;

	return pDbHeader->ofsUser;
}

// Return offset to next contact
DWORD FindNextContact(struct DBContact* pDbContact)
{
	if (!pDbContact)
		return 0;

	if (pDbContact->signature != DBCONTACT_SIGNATURE)
		return 0;

	return pDbContact->ofsNext;
}

// Read the contact at offset 'dwOffset'
// Returns true if successful and pDbContact points to the contact struct
// pDbContact must point to allocated struct
BOOL GetContact(HANDLE hDbFile, DWORD dwOffset, struct DBContact* pDbContact)
{
	DWORD dwBytesRead;

	// Early reject
	if (dwOffset == 0 || dwOffset >= dwFileSize)
		return FALSE;

	// ** Read and verify the struct

	if (SetFilePointer(hDbFile, (LONG)dwOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return FALSE;

	if ((!ReadFile(hDbFile, pDbContact, sizeof(struct DBContact), &dwBytesRead, NULL)) ||
		(dwBytesRead != sizeof(struct DBContact)))
		return FALSE;

	if ((pDbContact->signature != DBCONTACT_SIGNATURE) ||
		(pDbContact->ofsNext >= dwFileSize))
		return FALSE; // Contact corrupted

	return TRUE;
}

// Return ptr to next setting in settings struct
char* GetNextSetting(char* pDbSetting)
{
	// Get next setting
	pDbSetting = pDbSetting + *pDbSetting+1; // Skip name

	switch(*(BYTE*)pDbSetting) {
	case DBVT_BYTE:
		pDbSetting = pDbSetting+1+1;
		break;

	case DBVT_WORD:
		pDbSetting = pDbSetting+1+2;
		break;

	case DBVT_DWORD:
		pDbSetting = pDbSetting+1+4;
		break;

	case DBVT_ASCIIZ:
	case DBVT_BLOB:
	case DBVT_UTF8:
	case DBVT_WCHAR:
	case DBVTF_VARIABLELENGTH:
		pDbSetting = pDbSetting + 3 + *(WORD*)(pDbSetting+1);
		break;

	case DBVT_DELETED:
		AddMessage("DEBUG: Deleted setting treated as 0-length setting");
		pDbSetting = pDbSetting+1;
		break;

	default:
		// Unknown datatype assert
		AddMessage("ERROR: Faulty settings chain");
		return NULL;

	}

	return pDbSetting;
}


// **
// ** SETTINGS CHAIN
// **


// Return the settings at offset 'dwOffset'
BOOL GetSettingsGroup(HANDLE hDbFile, DWORD dwOffset, struct DBContactSettings** pDbSettings)
{
	DWORD dwBytesRead, dwBlobSize;
	struct DBContactSettings pSettings;

	// Early reject
	if (dwOffset == 0 || dwOffset >= dwFileSize)
		return FALSE;

	// ** Read and verify the struct

	if (SetFilePointer(hDbFile, dwOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return FALSE;

	if ((!ReadFile(hDbFile, &pSettings, sizeof(struct DBContactSettings), &dwBytesRead, NULL)) ||
		(dwBytesRead != sizeof(struct DBContactSettings)))
		return FALSE;

	if (pSettings.signature != DBCONTACTSETTINGS_SIGNATURE)
		return FALSE; // Setttings corrupted

	// ** Read the struct and the following blob

	dwBlobSize = pSettings.cbBlob;
	if (SetFilePointer(hDbFile, dwOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return FALSE;

	if (!(*pDbSettings = calloc(1, sizeof(struct DBContactSettings) + dwBlobSize)))
		return FALSE;

	if ((!ReadFile(hDbFile, *pDbSettings, sizeof(struct DBContactSettings) + dwBlobSize, &dwBytesRead, NULL)) ||
		(dwBytesRead != sizeof(struct DBContactSettings) + dwBlobSize))
	{
		free(*pDbSettings);
		return FALSE;
	}

	return TRUE;
}


// pDbContact is a ptr to a struct DBContact
// Returns pointer to a struct DBContactSettings or NULL
struct DBContactSettings* GetSettingsGroupByModuleName(HANDLE hDbFile, struct DBContact* pDbContact, char* pszName)
{
	char* pszGroupName;
	struct DBContactSettings* pSettingsGroup;
	DWORD dwGroupOfs;

	// Get ptr to first settings group
	if (!(dwGroupOfs = pDbContact->ofsFirstSettings))
		return NULL; // No settings exists in this contact

	// Loop over all settings groups
	while (dwGroupOfs && dwGroupOfs < dwFileSize)
	{
		pSettingsGroup = NULL;

		// Read and verify the struct
		if (!GetSettingsGroup(hDbFile, dwGroupOfs, &pSettingsGroup))
			return NULL; // Bad struct

		// Struct OK, now get the name
		if ((pszGroupName = GetName(hDbFile, pSettingsGroup->ofsModuleName)))
		{
			// Is it the right one?
			if (strcmp(pszGroupName, pszName) == 0)
			{
#ifdef _DEBUG
				_snprintf(str, sizeof(str), "Found module: %s", pszGroupName);
				AddMessage(str);
#endif
				free(pszGroupName);
				return pSettingsGroup;
			}
#ifdef _DEBUG
			else {
				_snprintf(str, sizeof(str), "Ignoring module: %s", pszGroupName);
				AddMessage(str);
			}
#endif
			if (pszGroupName)
				free(pszGroupName);
		}
		else {
			AddMessage("Warning: Found module with no name");
		}

		dwGroupOfs = pSettingsGroup->ofsNext;

		if (pSettingsGroup)
			free(pSettingsGroup);
	}

	// Search failed
	return NULL;
}

// pDbSettings must point to a complete DBContactSettings struct in memory
void* GetSettingByName(struct DBContactSettings* pDbSettings, char* pszSettingName)
{
	char* pDbSetting;
	char* pszName;

	// We need at least one setting to start with
	if (!(pDbSetting = pDbSettings->blob))
		return NULL;

	// ** pDbSettings now points to the first setting in this module

	// Loop over all settings
	while (pDbSetting && *pDbSetting)
	{
		pszName = calloc(*pDbSetting+1, 1);
		memcpy(pszName, pDbSetting+1, *pDbSetting);

		// Is this the right one?
		if (strcmp(pszSettingName, pszName) == 0)
		{
			free(pszName);
			return GetSettingValue(pDbSetting);
		}
		else
		{
			free(pszName);
#ifdef _DEBUG
			pszName = calloc(*pDbSetting+1, 1);
			memcpy(pszName, pDbSetting+1, *pDbSetting);
			_snprintf(str, sizeof(str), "Ignoring setting: %s", pszName);
			AddMessage(str);
			free(pszName);
#endif
			pDbSetting = GetNextSetting(pDbSetting);
	}	}

	// Search failed
	return NULL;
}

// pDbSettings must point to a complete DBContactSettings struct in memory
BYTE GetSettingTypeByName(struct DBContactSettings* pDbSettings, char* pszSettingName)
{
	char* pDbSetting;
	char* pszName;

	// We need at least one setting to start with
	if (!(pDbSetting = pDbSettings->blob))
		return DBVT_DELETED;

	// ** pDbSettings now points to the first setting in this module

	// Loop over all settings
	while (pDbSetting && *pDbSetting)
	{
		pszName = calloc(*pDbSetting+1, 1);
		memcpy(pszName, pDbSetting+1, *pDbSetting);

		// Is this the right one?
		if (lstrcmp(pszSettingName, pszName) == 0)
		{
			free(pszName);
			return (BYTE)*(pDbSetting + (*pDbSetting) + 1);
		}
		else
		{
			free(pszName);
#ifdef _DEBUG
			pszName = calloc(*pDbSetting+1, 1);
			memcpy(pszName, pDbSetting+1, *pDbSetting);
			mir_snprintf(str, sizeof(str), "Ignoring setting: %s", pszName);
			AddMessage(str);
			free(pszName);
#endif
			pDbSetting = GetNextSetting(pDbSetting);
	}	}

	// Search failed
	return DBVT_DELETED;
}

// dwSettingpointer points to a valid DBSettings struct
void* GetSettingValue(char* pBlob)
{
	void* pValue;

#ifdef _DEBUG
{
	char* pszName = calloc((*pBlob)+1, 1);
	memcpy(pszName, pBlob+1, *pBlob);
	_snprintf(str, sizeof(str), "Getting type %u value for setting: %s", (BYTE)*(pBlob+(*pBlob)+1), pszName);
	AddMessage(str);
	free(pszName);
}
#endif

	// Skip name
	pBlob = pBlob + (*pBlob)+1;

	// Check what type it is
	switch((BYTE)*pBlob) {
	case DBVT_BYTE:
		pValue = calloc(1,sizeof(byte));
		*(byte*)pValue = pBlob[1];
		return pValue;

	case DBVT_WORD:
		pValue = calloc(1, sizeof(WORD));
		*(WORD*)pValue = *(WORD*)(pBlob+1);
		return pValue;

	case DBVT_DWORD:
		pValue = calloc(1, sizeof(DWORD));
		*(DWORD*)pValue = *(DWORD*)(pBlob+1);
		return pValue;

	case DBVT_ASCIIZ:
		pValue = calloc((*(WORD*)(pBlob+1))+1, sizeof(char));
		memcpy(pValue, pBlob+3, *(WORD*)(pBlob+1));
		return pValue;

	case DBVT_UTF8:
	case DBVT_WCHAR:
		pValue = calloc((*(WORD*)(pBlob+1))+2, sizeof(char));
		memcpy(pValue, pBlob+3, *(WORD*)(pBlob+1));
	  return pValue;

	case DBVTF_VARIABLELENGTH:
	case DBVT_BLOB:
		pValue = calloc(*(WORD*)(pBlob+1), sizeof(char));
		memcpy(pValue, pBlob+3, *(WORD*)(pBlob+1));
		return pValue;

	case DBVT_DELETED:
		AddMessage("DEBUG: Deleted setting treated as 0-length setting");
		// Fall through

	default:
		return NULL;

	}

	return NULL;
}

// Returns true if pDBEI has been filled in with nice values
// Don't forget to free those pointers!
BOOL GetEvent(HANDLE hDbFile, DWORD dwOffset, DBEVENTINFO* pDBEI)
{
	DWORD dwBytesRead;
	struct DBEvent pEvent;
	char* pBlob;


	// Early reject
	if (dwOffset == 0 || dwOffset >= dwFileSize)
		return FALSE;


	// ** Read and verify the struct

	if (SetFilePointer(hDbFile, dwOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return FALSE;

	if (!ReadFile(hDbFile, &pEvent, sizeof(struct DBEvent), &dwBytesRead, NULL))
		return FALSE;

	if (pEvent.signature != DBEVENT_SIGNATURE)
		return FALSE; // Event corrupted

	// ** Read the blob

	if (SetFilePointer(hDbFile, dwOffset+offsetof(struct DBEvent, blob), NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return FALSE;

	if (!(pBlob = calloc(pEvent.cbBlob, 1)))
		return FALSE;

	if ((!ReadFile(hDbFile, pBlob, pEvent.cbBlob, &dwBytesRead, NULL)) ||
		(dwBytesRead != pEvent.cbBlob))
	{
		free(pBlob);
		return FALSE;
	}

	// ** Copy the static part to the event info struct

	pDBEI->timestamp = pEvent.timestamp;
	pDBEI->flags = pEvent.flags&DBEF_SENT ? DBEF_SENT : DBEF_READ; // Imported events are always marked READ
	pDBEI->eventType = pEvent.eventType;
	pDBEI->cbSize = sizeof(DBEVENTINFO);
	pDBEI->cbBlob = pEvent.cbBlob;
	pDBEI->pBlob = pBlob;
	if (!(pDBEI->szModule = GetName(hDbFile, pEvent.ofsModuleName)))
	{
		free(pBlob);
		return FALSE;
	}

	return TRUE;
}

// Returns a pointer to a string with the name
// from a DBModuleName struct if given a file offset
// Returns NULL on failure
char* GetName(HANDLE hDbFile, DWORD dwOffset)
{
	DWORD dwBytesRead;
	char* pszName = 0;
	struct DBModuleName pModule;

	// Early reject
	if (dwOffset == 0 || dwOffset >= dwFileSize)
		return FALSE;

	// ** Read and verify the name struct

	if (SetFilePointer(hDbFile, dwOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return NULL;

	if ((!ReadFile(hDbFile, &pModule, sizeof(struct DBModuleName), &dwBytesRead, NULL)) ||
		(dwBytesRead != sizeof(struct DBModuleName)))
		return NULL;

	if (pModule.signature != DBMODULENAME_SIGNATURE)
	{
		AddMessage("Modulename corrupted");
		return NULL; // ModuleName corrupted
	}

	// ** Name struct OK, now get the actual name

	if (!(pszName = calloc(pModule.cbName+1, sizeof(char))))
		return NULL;

	if (SetFilePointer(hDbFile, dwOffset + offsetof(struct DBModuleName, name), NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		free(pszName);
		return NULL;
	}

	// Read name into string buffer
	if ((!ReadFile(hDbFile, pszName, pModule.cbName, &dwBytesRead, NULL)) ||
		(dwBytesRead != pModule.cbName))
	{
		free(pszName);
		return NULL;
	}

	return pszName;
}

DWORD FindNextEvent(HANDLE hDbFile, DWORD dwOffset)
{
	DWORD dwBytesRead;
	struct DBEvent pEvent;

	// Early reject
	if (dwOffset == 0 || dwOffset >= dwFileSize)
		return FALSE;

	// ** Read and verify the struct

	if (SetFilePointer(hDbFile, dwOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		return FALSE;

	if ((!ReadFile(hDbFile, &pEvent, sizeof(struct DBEvent), &dwBytesRead, NULL)) ||
		(dwBytesRead != sizeof(struct DBEvent)))
		return FALSE;

	if ((pEvent.signature != DBEVENT_SIGNATURE) ||
		(pEvent.ofsNext > dwFileSize))
		return FALSE; // Event corrupted

	return pEvent.ofsNext;
}

int ImportGroups(HANDLE hDbFile, struct DBHeader* pdbHeader)
{
	struct DBContactSettings* pDbSettings;
	struct DBContact DbContact;
	char* pSetting;
	char* pszGroupName;
	DWORD dwOffset;
	int nGroups = 0;

	// Find owner data
	dwOffset = pdbHeader->ofsUser;
	if (!GetContact(hDbFile, dwOffset, &DbContact))
	{
		AddMessage("No owner found.");
		return -1;
	}

	// Find the module with the groups, and import them all
	if (pDbSettings = GetSettingsGroupByModuleName(hDbFile, &DbContact, "CListGroups"))
	{
		pSetting = pDbSettings->blob;
		while (pSetting && *pSetting)
		{
			if (pszGroupName = GetSettingValue(pSetting))
			{
				if (CreateGroup(hdlgProgress, pszGroupName+1))
					nGroups++;
				free(pszGroupName);
			}
			pSetting = GetNextSetting(pSetting);
		}
		free(pDbSettings);
	}

	return nGroups;
}

HANDLE ImportContact(HANDLE hDbFile, struct DBContact Contact)
{
	HANDLE hContact = INVALID_HANDLE_VALUE;
	char* pszProtoName = NULL;
	void* pSettingValue = NULL;
	BYTE bSettingType = DBVT_DELETED;
	BYTE* pbHidden = NULL;
	DWORD* pdwIgnoreMask = NULL;
	char* pzsGroupName = NULL;
	char* pzsNickName = NULL;

	// Check what protocol this contact belongs to
	if (GetSetting(hDbFile, &Contact, "Protocol", "p", &pszProtoName, &bSettingType))
	{
		if (!IsProtocolLoaded(pszProtoName))
		{
			_snprintf(str, sizeof(str), "Skipping contact, %s not installed.", pszProtoName);
			AddMessage(str);
		}
		else
		{
			DWORD dwCaps1;
			char* pszUniqueSetting = NULL;

			dwCaps1 = (DWORD)CallProtoService(pszProtoName, PS_GETCAPS, PFLAGNUM_1, 0);
			pszUniqueSetting = (char*)CallProtoService(pszProtoName, PS_GETCAPS, PFLAG_UNIQUEIDSETTING, 0);

			// Skip protocols with no unique id setting (some non IM protocols return NULL)

			if (!pszUniqueSetting)
			{
				_snprintf(str, sizeof(str), "Skipping non-IM contact (%s)", pszProtoName);
				AddMessage(str);
			}

			// Import numeric user

			else if (GetSetting(hDbFile, &Contact, pszProtoName, pszUniqueSetting, &pSettingValue, &bSettingType))
			{
				if (bSettingType == DBVT_DWORD)
				{
					DWORD* pdwUniqueID = (DWORD*)pSettingValue;//NULL;

					// Does the contact already exist?
					if (HContactFromNumericID(pszProtoName, pszUniqueSetting, *pdwUniqueID) == INVALID_HANDLE_VALUE)
					{
						// No, add contact and copy some important settings
						GetSetting(hDbFile, &Contact, "CList", "Group", &pzsGroupName, &bSettingType);

						if (!GetSetting(hDbFile, &Contact, "CList", "MyHandle", &pzsNickName, &bSettingType))
							GetSetting(hDbFile, &Contact, pszProtoName, "Nick", &pzsNickName, &bSettingType);

						hContact = AddNumericContact(hdlgProgress, pszProtoName, pszUniqueSetting, *pdwUniqueID, pzsNickName, pzsGroupName);

						if (hContact != INVALID_HANDLE_VALUE)
						{
							WORD* pwOrd = NULL;
							char* pszString = NULL;

							// Hidden?
							if (GetSetting(hDbFile, &Contact, "CList", "Hidden", &pbHidden, &bSettingType))
								DBWriteContactSettingByte(hContact,"CList","Hidden", *pbHidden);

							// Ignore settings
							if (GetSetting(hDbFile, &Contact, "Ignore", "Mask1", &pdwIgnoreMask, &bSettingType))
								DBWriteContactSettingDword(hContact,"Ignore","Mask1", *pdwIgnoreMask);

							// Apparent mode
							if (GetSetting(hDbFile, &Contact, pszProtoName, "ApparentMode", &pwOrd, &bSettingType))
							{
								DBWriteContactSettingWord(hContact, pszProtoName,"ApparentMode", *pwOrd);
								free(pwOrd);
							}

							// Nick // TODO this requires Unicode fix !!!!!
							if (GetSetting(hDbFile, &Contact, pszProtoName, "Nick", &pszString, &bSettingType))
							{
								DBWriteContactSettingString(hContact, pszProtoName,"Nick", pszString);
								free(pszString);
							}
							// Myhandle // TODO this requires Unicode fix !!!!!
							if (GetSetting(hDbFile, &Contact, pszProtoName, "MyHandle", &pszString, &bSettingType))
							{
								DBWriteContactSettingString(hContact, pszProtoName,"MyHandle", pszString);
								free(pszString);
							}
							// First name
							if (GetSetting(hDbFile, &Contact, pszProtoName, "FirstName", &pszString, &bSettingType))
							{
								DBWriteContactSettingString(hContact, pszProtoName,"FirstName", pszString);
								free(pszString);
							}
							// Last name
							if (GetSetting(hDbFile, &Contact, pszProtoName, "LastName", &pszString, &bSettingType))
							{
								DBWriteContactSettingString(hContact, pszProtoName,"LastName", pszString);
								free(pszString);
							}
							// About
							if (GetSetting(hDbFile, &Contact, pszProtoName, "About", &pszString, &bSettingType))
							{
								DBWriteContactSettingString(hContact, pszProtoName,"About", pszString);
								free(pszString);
							}
						}
						else
						{
							_snprintf(str, sizeof(str), "Unknown error while adding %s contact %u", pszProtoName, *pdwUniqueID);
							AddMessage(str);
						}
					}
					else
					{
						_snprintf(str, sizeof(str), "Skipping duplicate %s contact %u", pszProtoName, *pdwUniqueID);
						AddMessage(str);
					}
				}

				// Import string user

				else if (bSettingType == DBVT_ASCIIZ)
				{
					char* pszUniqueID = (char*)pSettingValue;

					// Does the contact already exist?
					if (HContactFromID(pszProtoName, pszUniqueSetting, pszUniqueID) == INVALID_HANDLE_VALUE)
					{
						// No, add contact and copy some important settings
						GetSetting(hDbFile, &Contact, "CList", "Group", &pzsGroupName, &bSettingType);

						if (!GetSetting(hDbFile, &Contact, "CList", "MyHandle", &pzsNickName, &bSettingType))
							GetSetting(hDbFile, &Contact, pszProtoName, "Nick", &pzsNickName, &bSettingType);

						hContact = AddContact(hdlgProgress, pszProtoName, pszUniqueSetting, pszUniqueID, pzsNickName, pzsGroupName);

						if (hContact != INVALID_HANDLE_VALUE)
						{

							WORD* pwOrd = NULL;
							char* pszString = NULL;

							// Ignore settings
							if (GetSetting(hDbFile, &Contact, "Ignore", "Mask1", &pdwIgnoreMask, &bSettingType))
								DBWriteContactSettingDword(hContact,"Ignore","Mask1", *pdwIgnoreMask);

							// Apparent mode
							if (GetSetting(hDbFile, &Contact, pszProtoName, "ApparentMode", &pwOrd, &bSettingType))
							{
								DBWriteContactSettingWord(hContact, pszProtoName,"ApparentMode", *pwOrd);
								free(pwOrd);
							}

							// Nick
							if (GetSetting(hDbFile, &Contact, pszProtoName, "Nick", &pszString, &bSettingType))
							{
								DBWriteContactSettingString(hContact, pszProtoName,"Nick", pszString);
								free(pszString);
							}
							// Myhandle
							if (GetSetting(hDbFile, &Contact, pszProtoName, "MyHandle", &pszString, &bSettingType))
							{
								DBWriteContactSettingString(hContact, pszProtoName,"MyHandle", pszString);
								free(pszString);
							}
							// First name
							if (GetSetting(hDbFile, &Contact, pszProtoName, "FirstName", &pszString, &bSettingType))
							{
								DBWriteContactSettingString(hContact, pszProtoName,"FirstName", pszString);
								free(pszString);
							}
							// Last name
							if (GetSetting(hDbFile, &Contact, pszProtoName, "LastName", &pszString, &bSettingType))
							{
								DBWriteContactSettingString(hContact, pszProtoName,"LastName", pszString);
								free(pszString);
							}
							// About
							if (GetSetting(hDbFile, &Contact, pszProtoName, "About", &pszString, &bSettingType))
							{
								DBWriteContactSettingString(hContact, pszProtoName,"About", pszString);
								free(pszString);
							}
						}
						else
						{
							_snprintf(str, sizeof(str), "Unknown error while adding %s contact %s", pszProtoName, pszUniqueID);
							AddMessage(str);
						}
					}
					else
					{
						_snprintf(str, sizeof(str), "Skipping duplicate %s contact, %s", pszProtoName, pszUniqueID);
						AddMessage(str);
					}
				}
				if (pSettingValue)
					free(pSettingValue);
			}
			else
			{
				_snprintf(str, sizeof(str), "Skipping %s contact, ID not found", pszProtoName);
				AddMessage(str);
			}
		}
	}
	else
	{
		AddMessage("Skipping contact with no protocol");
	}

	// Clean up and exit
	if (pbHidden) free(pbHidden);
	if (pdwIgnoreMask) free(pdwIgnoreMask);
	if (pszProtoName) free(pszProtoName);
	if (pzsGroupName) free(pzsGroupName);
	if (pzsNickName) free(pzsNickName);

	return hContact;
}

// This function should always be called after contact import. That is
// why there are no messages for errors related to contacts. Those
// would only be a repetition of the messages printed during contact
// import.
static void ImportHistory(HANDLE hDbFile, struct DBContact Contact)
{
	DBEVENTINFO dbei;
	HANDLE hContact = INVALID_HANDLE_VALUE;
	DWORD dwOffset;
	MSG msg;
	char* pszProtoName = NULL;
	BYTE bType;

	// Check what protocol this contact belongs to
	if (GetSetting(hDbFile, &Contact, "Protocol", "p", &pszProtoName, &bType))
	{
		// Protocol installed?
		if (IsProtocolLoaded(pszProtoName))
		{
			// Is contact in database?
			DWORD dwCaps1;
			char* pszUniqueSetting = NULL;

			dwCaps1 = (DWORD)CallProtoService(pszProtoName, PS_GETCAPS, PFLAGNUM_1, 0);
			pszUniqueSetting = (char*)CallProtoService(pszProtoName, PS_GETCAPS, PFLAG_UNIQUEIDSETTING, 0);

			// Skip protocols with no unique id setting (some non IM protocols return NULL)
			if (pszUniqueSetting)
			{
				void* pSetting;

				//if (dwCaps1&PF1_NUMERICUSERID)
				if (GetSetting(hDbFile, &Contact, pszProtoName, pszUniqueSetting, &pSetting, &bType))
				{
					if (bType==DBVT_DWORD)
					{
						DWORD* pdwUniqueID = (DWORD*)pSetting;//NULL;
						hContact = HContactFromNumericID(pszProtoName, pszUniqueSetting, *pdwUniqueID);
					}

					// Lookup string user

					else
					{
						char* pszUniqueID = (char*)pSetting;//NULL;
						hContact = HContactFromID(pszProtoName, pszUniqueSetting, pszUniqueID);
					}
					if (pSetting)
						free(pSetting);
				}
			}
		}
	}

	// OK to import this chain?
	if (hContact != INVALID_HANDLE_VALUE)
	{
		int i = 0;

		// Get the start of the event chain
		dwOffset = Contact.ofsFirstEvent;
		while (dwOffset) {

			// Copy the event and import it
			ZeroMemory(&dbei, sizeof(dbei));
			if (GetEvent(hDbFile, dwOffset, &dbei)) {
				// Check for duplicate entries
				if (!IsDuplicateEvent(hContact, dbei)) {
					// Add dbevent
					if (CallService(MS_DB_EVENT_ADD, (WPARAM)hContact, (LPARAM)&dbei))
						nMessagesCount++;
					else
						AddMessage("Failed to add message");
				}
				else
					nDupes++;
				free(dbei.pBlob);
				free(dbei.szModule);
			}

			if (!(i%10)) {
				if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
			}	}

			// Get next event
			dwOffset = FindNextEvent(hDbFile, dwOffset);
			i++;
	}	}

	// Clean up and exit
	if (pszProtoName)
		free(pszProtoName);

	return;
}

static void MirandaImport(HWND hdlg)
{
	int nDBVersion;
	int i;
	int nNumberOfContacts = 0;
	MSG msg;
	DWORD dwTimer;
	DWORD dwOffset;
	HANDLE hFile;
	char str[256];
	char* pszModuleName = NULL;
	struct DBHeader* pdbHeader = NULL;
	struct DBContact Contact;

	// Just to keep the macros happy
	hdlgProgress = hdlg;

	// Reset statistics
	nDupes = 0;
	nContactsCount = 0;
	nMessagesCount = 0;
	nGroupsCount = 0;

	SetProgress(0);

	// Open database
	hFile = CreateFile(importFile,
		GENERIC_READ,                 // open for reading
		0,                            // do not share
		NULL,                         // no security
		OPEN_EXISTING,                // existing file only
		FILE_ATTRIBUTE_NORMAL,        // normal file
		NULL);                        // no attr. template

	// Read error
	if (hFile == INVALID_HANDLE_VALUE)
	{
		AddMessage("Could not open file.");
		SetProgress(100);
		return;
	}

	// Check filesize
	dwFileSize = GetFileSize(hFile, NULL) ;
	if ((dwFileSize == INVALID_FILE_SIZE) || (dwFileSize < sizeof(struct DBHeader)))
	{
		AddMessage("This is not a valid Miranda IM database.");
		SetProgress(100);
		CloseHandle(hFile);
		return;
	}

	// Check header and database version
	nDBVersion = CheckFileFormat(hFile);
	if (nDBVersion == DB_INVALID)
	{
		AddMessage("This is not a valid Miranda IM database.");
		SetProgress(100);
		CloseHandle(hFile);
		return;
	}

	// Load database header
	if (!(pdbHeader = GetHeader(hFile)))
	{
		AddMessage("Read failure.");
		SetProgress(100);
		CloseHandle(hFile);
		return;
	}

	// Get number of contacts
	nNumberOfContacts = pdbHeader->contactCount;
	_snprintf(str, sizeof(str), "Number of contacts in database: %d", nNumberOfContacts);
	AddMessage(str);
	AddMessage("");


	// Configure database for fast writing
	CallService(MS_DB_SETSAFETYMODE, FALSE, 0);

	// Start benchmark timer
	dwTimer = time(NULL);


	// Import Groups
	AddMessage("Importing groups.");
	nGroupsCount = ImportGroups(hFile, pdbHeader);
	if (nGroupsCount == -1)
	{
		AddMessage("Group import failed.");
	}
	AddMessage("");
	// End of Import Groups

	// Import Contacts
	AddMessage("Importing contacts.");
	i = 1;
	dwOffset = FindFirstContact(pdbHeader);
	while (dwOffset && (dwOffset < dwFileSize))
	{
		if (!GetContact(hFile, dwOffset, &Contact))
		{
			_snprintf(str, sizeof(str), "ERROR: Chain broken, no valid contact at %d", dwOffset);
			AddMessage(str);
			SetProgress(100);
			break;
		}

		if (ImportContact(hFile, Contact) != INVALID_HANDLE_VALUE)
			nContactsCount++;

		// Update progress bar
		SetProgress(100 * i / nNumberOfContacts);
		i++;

		// Process queued messages
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Get next contact in chain
		dwOffset = FindNextContact(&Contact);
	}
	AddMessage("");
	// End of Import Contacts

	// Import history
	if (nImportOption != IMPORT_CONTACTS)
	{
		AddMessage("Importing history");

		// Import NULL contact message chain
		dwOffset = FindOwnerContact(pdbHeader);
		if (!GetContact(hFile, dwOffset, &Contact))
		{
			_snprintf(str, sizeof(str), "ERROR: Chain broken, no valid contact at %d", dwOffset);
			AddMessage(str);
			SetProgress(100);
		}
		else
			ImportHistory(hFile, Contact);

		// Import other contact messages
		dwOffset = FindFirstContact(pdbHeader);
		for(i=1; i <= nNumberOfContacts; i++)
		{
			if (!GetContact(hFile, dwOffset, &Contact))
			{
				_snprintf(str, sizeof(str), "ERROR: Chain broken, no valid contact at %d", dwOffset);
				AddMessage(str);
				SetProgress(100);
				break;
			}

			ImportHistory(hFile, Contact);

			SetProgress(100 * i / nNumberOfContacts);
			dwOffset = FindNextContact(&Contact);
		}
		AddMessage("");
	}
	// End of Import History

	// Restore database writing mode
	CallService(MS_DB_SETSAFETYMODE, TRUE, 0);

	// Clean up before exit
	CloseHandle(hFile);
	free(pdbHeader);

	// Stop timer
	dwTimer = time(NULL) - dwTimer;

	// Print statistics
	_snprintf(str, sizeof(str), "Import completed in %d seconds.", dwTimer);
	AddMessage(str);
	SetProgress(100);
	_snprintf(str, sizeof(str), "Added %d contacts and %d groups.", nContactsCount, nGroupsCount);
	AddMessage(str);
	if ((nImportOption == IMPORT_ALL)||(nImportOption == IMPORT_CUSTOM))
	{
		_snprintf(str, sizeof(str), "Added %d events and skipped %d duplicates.", nMessagesCount, nDupes);
		AddMessage(str);
	}

	return;
}
