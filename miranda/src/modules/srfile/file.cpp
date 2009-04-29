/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

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
#include "commonheaders.h"
#include "file.h"

static HANDLE hSRFileMenuItem;

static INT_PTR SendFileCommand(WPARAM wParam, LPARAM)
{
	struct FileSendData fsd;
	fsd.hContact=(HANDLE)wParam;
	fsd.ppFiles=NULL;
	CreateDialogParam(hMirandaInst,MAKEINTRESOURCE(IDD_FILESEND),NULL,DlgProcSendFile,(LPARAM)&fsd);
	return 0;
}

static INT_PTR SendSpecificFiles(WPARAM wParam,LPARAM lParam)
{
	struct FileSendData fsd;
	fsd.hContact=(HANDLE)wParam;
	fsd.ppFiles=(const char**)lParam;
	CreateDialogParam(hMirandaInst,MAKEINTRESOURCE(IDD_FILESEND),NULL,DlgProcSendFile,(LPARAM)&fsd);
	return 0;
}

static INT_PTR GetReceivedFilesFolder(WPARAM wParam,LPARAM lParam)
{
	GetContactReceivedFilesDir((HANDLE)wParam,(char*)lParam,MAX_PATH,TRUE);
	return 0;
}

static INT_PTR RecvFileCommand(WPARAM, LPARAM lParam)
{
	CreateDialogParam(hMirandaInst,MAKEINTRESOURCE(IDD_FILERECV),NULL,DlgProcRecvFile,lParam);
	return 0;
}

void PushFileEvent( HANDLE hContact, HANDLE hdbe, LPARAM lParam )
{
	CLISTEVENT cle={0};
	cle.cbSize = sizeof(cle);
	cle.hContact = hContact;
	cle.hDbEvent = hdbe;
	cle.lParam = lParam;
	if ( DBGetContactSettingByte(NULL,"SRFile","AutoAccept",0) && !DBGetContactSettingByte(hContact,"CList","NotOnList",0)) {
		CreateDialogParam(hMirandaInst,MAKEINTRESOURCE(IDD_FILERECV),NULL,DlgProcRecvFile,(LPARAM)&cle);
	}
	else {
		SkinPlaySound("RecvFile");

		TCHAR szTooltip[256];
		mir_sntprintf(szTooltip,SIZEOF(szTooltip),TranslateT("File from %s"), cli.pfnGetContactDisplayName( hContact, 0 ));
		cle.ptszTooltip = szTooltip;

		cle.flags |= CLEF_TCHAR;
		cle.hIcon = LoadSkinIcon( SKINICON_EVENT_FILE );
		cle.pszService = "SRFile/RecvFile";
		CallService(MS_CLIST_ADDEVENT,0,(LPARAM)&cle);
}	}

static int FileEventAdded(WPARAM wParam,LPARAM lParam)
{
	DWORD dwSignature;

	DBEVENTINFO dbei={0};
	dbei.cbSize = sizeof(dbei);
	dbei.cbBlob = sizeof( DWORD );
	dbei.pBlob = ( PBYTE )&dwSignature;
	CallService( MS_DB_EVENT_GET, lParam, ( LPARAM )&dbei );
	if ( dbei.flags&(DBEF_SENT|DBEF_READ) || dbei.eventType != EVENTTYPE_FILE || dwSignature == 0 )
		return 0;

	PushFileEvent(( HANDLE )wParam, ( HANDLE )lParam, 0 );
	return 0;
}

int SRFile_GetRegValue(HKEY hKeyBase,const char *szSubKey,const char *szValue,char *szOutput,int cbOutput)
{
	HKEY hKey;
	DWORD cbOut=cbOutput;

	if(RegOpenKeyExA(hKeyBase,szSubKey,0,KEY_QUERY_VALUE,&hKey)!=ERROR_SUCCESS) return 0;
	if(RegQueryValueExA(hKey,szValue,NULL,NULL,(PBYTE)szOutput,&cbOut)!=ERROR_SUCCESS) {RegCloseKey(hKey); return 0;}
	RegCloseKey(hKey);
	return 1;
}

void GetSensiblyFormattedSize(DWORD size,TCHAR *szOut,int cchOut,int unitsOverride,int appendUnits,int *unitsUsed)
{
	if(!unitsOverride) {
		if(size<1000) unitsOverride=UNITS_BYTES;
		else if(size<100*1024) unitsOverride=UNITS_KBPOINT1;
		else if(size<1024*1024) unitsOverride=UNITS_KBPOINT0;
		else unitsOverride=UNITS_MBPOINT2;
	}
	if(unitsUsed) *unitsUsed=unitsOverride;
	switch(unitsOverride) {
		case UNITS_BYTES: mir_sntprintf(szOut,cchOut,_T("%u%s%s"),size,appendUnits?_T(" "):_T(""),appendUnits?TranslateT("bytes"):_T("")); break;
		case UNITS_KBPOINT1: mir_sntprintf(szOut,cchOut,_T("%.1lf%s"),size/1024.0,appendUnits?_T(" KB"):_T("")); break;
		case UNITS_KBPOINT0: mir_sntprintf(szOut,cchOut,_T("%u%s"),size/1024,appendUnits?_T(" KB"):_T("")); break;
		default: mir_sntprintf(szOut,cchOut,_T("%.2lf%s"),size/1048576.0,appendUnits?_T(" MB"):_T("")); break;
	}
}

// Tripple redirection sucks but is needed to nullify the array pointer
void FreeFilesMatrix(char ***files)
{

	char **pFile;

	if (*files == NULL)
		return;

	// Free each filename in the pointer array
	pFile = *files;
	while (*pFile != NULL)
	{
		mir_free(*pFile);
		*pFile = NULL;
		pFile++;
	}

	// Free the array itself
	mir_free(*files);
	*files = NULL;

}

void FreeProtoFileTransferStatus(PROTOFILETRANSFERSTATUS *fts)
{
	if(fts->currentFile) mir_free(fts->currentFile);
	if(fts->files) {
		int i;
		for(i=0;i<fts->totalFiles;i++) mir_free(fts->files[i]);
		mir_free(fts->files);
	}
	if(fts->workingDir) mir_free(fts->workingDir);
}

void CopyProtoFileTransferStatus(PROTOFILETRANSFERSTATUS *dest,PROTOFILETRANSFERSTATUS *src)
{
	*dest=*src;
	if(src->currentFile) dest->currentFile=mir_strdup(src->currentFile);
	if(src->files) {
		int i;
		dest->files=(char**)mir_alloc(sizeof(char*)*src->totalFiles);
		for(i=0;i<src->totalFiles;i++)
			dest->files[i]=mir_strdup(src->files[i]);
	}
	if(src->workingDir) dest->workingDir=mir_strdup(src->workingDir);
}

void UpdateProtoFileTransferStatus(PROTOFILETRANSFERSTATUS *dest,PROTOFILETRANSFERSTATUS *src)
{
  dest->hContact = src->hContact;
  dest->sending = src->sending;
  if (dest->totalFiles != src->totalFiles) {
    int i;
    for(i=0;i<dest->totalFiles;i++) mir_free(dest->files[i]);
    mir_free(dest->files);
    dest->files = NULL;
	  dest->totalFiles = src->totalFiles;
  }
  if (src->files) {
    int i;
    if (!dest->files) dest->files = (char**)mir_calloc(sizeof(char*)*src->totalFiles);
    for(i=0;i<src->totalFiles;i++)
      if(!dest->files[i] || !src->files[i] || strcmp(dest->files[i],src->files[i])) {
        mir_free(dest->files[i]);
        if(src->files[i]) dest->files[i]=mir_strdup(src->files[i]); else dest->files[i]=NULL;
      }
  } else if (dest->files) {
    int i;
    for(i=0;i<dest->totalFiles;i++) mir_free(dest->files[i]);
    mir_free(dest->files);
    dest->files = NULL;
  }
	dest->currentFileNumber = src->currentFileNumber;
	dest->totalBytes = src->totalBytes;
	dest->totalProgress = src->totalProgress;
  if (src->workingDir && (!dest->workingDir || strcmp(dest->workingDir, src->workingDir))) {
    mir_free(dest->workingDir);
    if(src->workingDir) dest->workingDir=mir_strdup(src->workingDir); else dest->workingDir = NULL;
  }
  if (!dest->currentFile || !src->currentFile || strcmp(dest->currentFile, src->currentFile)) {
    mir_free(dest->currentFile);
    if(src->currentFile) dest->currentFile=mir_strdup(src->currentFile); else dest->currentFile = NULL;
  }
	dest->currentFileSize = src->currentFileSize;
	dest->currentFileProgress = src->currentFileProgress;
	dest->currentFileTime = src->currentFileTime;
}

static void RemoveUnreadFileEvents(void)
{
	DBEVENTINFO dbei={0};
	HANDLE hDbEvent,hContact;

	dbei.cbSize=sizeof(dbei);
	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	while(hContact) {
		hDbEvent=(HANDLE)CallService(MS_DB_EVENT_FINDFIRSTUNREAD,(WPARAM)hContact,0);
		while(hDbEvent) {
			dbei.cbBlob=0;
			CallService(MS_DB_EVENT_GET,(WPARAM)hDbEvent,(LPARAM)&dbei);
			if(!(dbei.flags&(DBEF_SENT|DBEF_READ)) && dbei.eventType==EVENTTYPE_FILE)
				CallService(MS_DB_EVENT_MARKREAD,(WPARAM)hContact,(LPARAM)hDbEvent);
			hDbEvent=(HANDLE)CallService(MS_DB_EVENT_FINDNEXT,(WPARAM)hDbEvent,0);
		}
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
	}
}

static int SRFilePreBuildMenu(WPARAM wParam, LPARAM)
{
	CLISTMENUITEM mi = { 0 };
	mi.cbSize = sizeof(mi);
	mi.flags = CMIM_FLAGS | CMIF_HIDDEN;

	char *szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, wParam, 0);
	if (szProto != NULL) {
		if ( CallProtoService(szProto, PS_GETCAPS,PFLAGNUM_1, 0 ) & PF1_FILESEND) {
			if ( CallProtoService(szProto, PS_GETCAPS,PFLAGNUM_4, 0 ) & PF4_OFFLINEFILES )
				mi.flags = CMIM_FLAGS;
			else if ( DBGetContactSettingWord(( HANDLE )wParam, szProto, "Status", ID_STATUS_OFFLINE ) != ID_STATUS_OFFLINE )
				mi.flags = CMIM_FLAGS;
	}	}

	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hSRFileMenuItem, (LPARAM)&mi);
	return 0;
}

static int SRFileModulesLoaded(WPARAM, LPARAM)
{
	CLISTMENUITEM mi = { 0 };
	mi.cbSize = sizeof(mi);
	mi.position = -2000020000;
	mi.icolibItem = GetSkinIconHandle( SKINICON_EVENT_FILE );
	mi.pszName = LPGEN("&File");
	mi.pszService = MS_FILE_SENDFILE;
	mi.flags = CMIF_ICONFROMICOLIB;
	hSRFileMenuItem = ( HANDLE )CallService(MS_CLIST_ADDCONTACTMENUITEM,0,(LPARAM)&mi);

	RemoveUnreadFileEvents();
	return 0;
}

INT_PTR FtMgrShowCommand(WPARAM, LPARAM)
{
	FtMgr_Show(true, true);
	return 0;
}

INT_PTR openContRecDir(WPARAM wparam, LPARAM)
{
	char szContRecDir[MAX_PATH];
	HANDLE hContact = (HANDLE)wparam;
	GetContactReceivedFilesDir(hContact, szContRecDir, SIZEOF(szContRecDir),TRUE);
	ShellExecuteA(0, "open", szContRecDir, 0, 0, SW_SHOW);
	return 0;
}

INT_PTR openRecDir(WPARAM, LPARAM)
{
	char szContRecDir[MAX_PATH];
	GetReceivedFilesDir(szContRecDir, SIZEOF(szContRecDir));
	ShellExecuteA(0, "open", szContRecDir, 0, 0, SW_SHOW);
	return 0;
}

int LoadSendRecvFileModule(void)
{
	CreateServiceFunction("FtMgr/Show", FtMgrShowCommand);

	CLISTMENUITEM mi = { 0 };
	mi.cbSize = sizeof(mi);
	mi.flags = CMIF_ICONFROMICOLIB;
	mi.icolibItem = GetSkinIconHandle( SKINICON_EVENT_FILE );
	mi.position = 1900000000;
	mi.pszName = LPGEN("File &Transfers...");
	mi.pszService = "FtMgr/Show"; //MS_PROTO_SHOWFTMGR;
	CallService( MS_CLIST_ADDMAINMENUITEM, 0, ( LPARAM )&mi );

	HookEvent(ME_SYSTEM_MODULESLOADED,SRFileModulesLoaded);
	HookEvent(ME_DB_EVENT_ADDED,FileEventAdded);
	HookEvent(ME_OPT_INITIALISE,FileOptInitialise);
	HookEvent(ME_CLIST_PREBUILDCONTACTMENU, SRFilePreBuildMenu);

	CreateServiceFunction(MS_FILE_SENDFILE,SendFileCommand);
	CreateServiceFunction(MS_FILE_SENDSPECIFICFILES,SendSpecificFiles);
	CreateServiceFunction(MS_FILE_GETRECEIVEDFILESFOLDER,GetReceivedFilesFolder);
	CreateServiceFunction("SRFile/RecvFile",RecvFileCommand);

	CreateServiceFunction("SRFile/OpenContRecDir",openContRecDir);
	CreateServiceFunction("SRFile/OpenRecDir",openRecDir);

	SkinAddNewSoundEx("RecvFile",Translate("File"),Translate("Incoming"));
	SkinAddNewSoundEx("FileDone",Translate("File"),Translate("Complete"));
	SkinAddNewSoundEx("FileFailed",Translate("File"),Translate("Error"));
	SkinAddNewSoundEx("FileDenied",Translate("File"),Translate("Denied"));
	return 0;
}
