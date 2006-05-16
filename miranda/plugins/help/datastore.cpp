/*
Miranda IM Help Plugin
Copyright (C) 2002 Richard Hughes

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
#include <stdio.h>
#include <newpluginapi.h>
#include <m_netlib.h>
#include <m_langpack.h>
#include "help.h"

#include <process.h>
#include <time.h>

#define DIALOGCACHEEXPIRY    10*60    //delete from cache after n seconds

extern HWND hwndHelpDlg;
extern HANDLE hNetlibUser;

struct DlgControlData {
	int id;
	int type;
	char *szTitle;
	char *szText;
};

struct DialogData {
	char *szId;
	char *szModule;
	struct DlgControlData *control;
	int controlCount;
	time_t timeDownloaded,timeLastUsed;
	int changes;
};

static struct DialogData *dialogCache=NULL;
static int dialogCacheCount=0;
static CRITICAL_SECTION csDialogCache;

void InitDialogCache(void)
{
	InitializeCriticalSection(&csDialogCache);
}

static void FreeDialogCacheEntry(struct DialogData *entry)
{
	int i;
	for(i=0;i<entry->controlCount;i++) {
		if(entry->control[i].szText) free(entry->control[i].szText);
		if(entry->control[i].szTitle) free(entry->control[i].szTitle);
	}
	if(entry->controlCount) free(entry->control);
	if(entry->szId) free(entry->szId);
	if(entry->szModule) free(entry->szModule);
}

void FreeDialogCache(void)
{
	int i;
	DeleteCriticalSection(&csDialogCache);
	for(i=0;i<dialogCacheCount;i++)
		FreeDialogCacheEntry(&dialogCache[i]);
	dialogCacheCount=0;
	if(dialogCache) free(dialogCache);
	dialogCache=NULL;
}

/**************************** DOWNLOAD HELP ***************************/

struct DownloaderThreadStartParams {
	char *szDlgId;
	char *szModule;
	int ctrlId;
};

static void DownloaderThread(struct DownloaderThreadStartParams *dtsp)
{
	NETLIBHTTPREQUEST nlhr={0},*nlhrReply;
	struct ResizableCharBuffer url={0};
	char szServer[80],szPath[128],szLanguage[16];
	int succeeded=0;
	char *szDlgIdUrlEncoded;

	GetStringOption(OPTION_SERVER,szServer,sizeof(szServer));
	GetStringOption(OPTION_PATH,szPath,sizeof(szPath));
	GetStringOption(OPTION_LANGUAGE,szLanguage,sizeof(szLanguage));
	szDlgIdUrlEncoded=(char*)CallService(MS_NETLIB_URLENCODE,0,(LPARAM)dtsp->szDlgId);
	AppendToCharBuffer(&url,"http://%s%sdialoghelp.php?module=%s&id=%s&lang=%s",
					   szServer,szPath,dtsp->szModule,szDlgIdUrlEncoded,szLanguage);
	HeapFree(GetProcessHeap(),0,szDlgIdUrlEncoded);
	nlhr.cbSize=sizeof(nlhr);
	nlhr.requestType=REQUEST_GET;
	nlhr.flags=NLHRF_DUMPASTEXT;
	nlhr.szUrl=url.sz;
	nlhrReply=(NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION,(WPARAM)hNetlibUser,(LPARAM)&nlhr);
	free(url.sz);
	if(nlhrReply) {
		if(nlhrReply->resultCode<200 || nlhrReply->resultCode>=300) {
			MessageBoxA(NULL,Translate("The help server replied with a failure code."),nlhrReply->szResultDescr,MB_OK);
		}
		else if(nlhrReply->dataLength) {
			int i;
			struct ResizableCharBuffer content={0};
			int inDialogTag=0,inControlTag=0,inTitleTag=0,inTextTag=0;
			struct DialogData dialog={0};
			struct DlgControlData *control;
			char *pszData=nlhrReply->pData;

			for(i=0;pszData[i];) {
				if(pszData[i]=='<') {
					char *pszTagEnd;
					int iNameEnd;
					char szTagName[64];
					int processed=0;
					
					pszTagEnd=strchr(pszData+i+1,'>');
					if(pszTagEnd==NULL) break;
					for(iNameEnd=i+1;pszData[iNameEnd] && pszData[iNameEnd]!='>' && !isspace(pszData[iNameEnd]);iNameEnd++);
					lstrcpynA(szTagName,pszData+i+1,min(sizeof(szTagName),iNameEnd-i));
					if(!lstrcmpiA(szTagName,"dialog") && !inDialogTag) {
						dialog.szId=GetHtmlTagAttribute(pszData+i,"id");
						dialog.szModule=GetHtmlTagAttribute(pszData+i,"module");
						if(dialog.szId && dialog.szModule)
							inDialogTag=1;
						else {
							if(dialog.szId) free(dialog.szId);
							if(dialog.szModule) free(dialog.szModule);
						}
						processed=1;
					}
					else if(!lstrcmpiA(szTagName,"/dialog")) {
						inDialogTag=0;
						processed=1;
					}
					else if(inDialogTag) {
						if(!lstrcmpiA(szTagName,"control") && !inControlTag) {
							char *szId,*szType;
							dialog.controlCount++;
							dialog.control=(struct DlgControlData*)realloc(dialog.control,sizeof(struct DlgControlData)*dialog.controlCount);
							control=&dialog.control[dialog.controlCount-1];
							szId=GetHtmlTagAttribute(pszData+i,"id");
							szType=GetHtmlTagAttribute(pszData+i,"type");
							if(szId && szType) {
								control->id=atoi(szId);
								control->type=atoi(szType);
								control->szText=NULL;
								control->szTitle=NULL;
								inControlTag=1;
							}
							else dialog.controlCount--;
							if(szId) free(szId);
							if(szType) free(szType);
							processed=1;
						}
						else if(!lstrcmpiA(szTagName,"/control")) {
							inControlTag=0;
							processed=1;
						}
						else if(inControlTag) {
							if(!lstrcmpiA(szTagName,"title") && !inTitleTag) {
								if(content.sz) free(content.sz);
								content.sz=NULL;
								content.cbAlloced=content.iEnd=0;
								inTitleTag=1;
								processed=1;
							}
							else if(!lstrcmpiA(szTagName,"text") && !inTextTag) {
								if(content.sz) free(content.sz);
								content.sz=NULL;
								content.cbAlloced=content.iEnd=0;
								inTextTag=1;
								processed=1;
							}
							else if(!lstrcmpiA(szTagName,"/title") && inTitleTag) {
								control->szTitle=content.sz;
								content.sz=NULL;
								content.cbAlloced=content.iEnd=0;
								inTitleTag=0;
								processed=1;
							}
							else if(!lstrcmpiA(szTagName,"/text") && inTextTag) {
								control->szText=content.sz;
								content.sz=NULL;
								content.cbAlloced=content.iEnd=0;
								inTextTag=0;
								processed=1;
							}
						}
					}
					if(processed) {
						i=pszTagEnd-pszData+1;
						continue;
					}
				}
				AppendCharToCharBuffer(&content,pszData[i++]);
			}
			if(content.sz) free(content.sz);
			dialog.timeDownloaded=dialog.timeLastUsed=time(NULL);
			
			if(dialog.szId) {
				int dialogInserted=0;
				EnterCriticalSection(&csDialogCache);
				for(i=0;i<dialogCacheCount;i++) {
					if(dialogCache[i].timeLastUsed<dialog.timeDownloaded-DIALOGCACHEEXPIRY) {
						FreeDialogCacheEntry(&dialogCache[i]);
						if(dialogInserted) {
							MoveMemory(dialogCache+i,dialogCache+i+1,sizeof(struct DialogData)*(dialogCacheCount-i-1));
							dialogCache=(struct DialogData*)realloc(dialogCache,sizeof(struct DialogData)*dialogCacheCount);
							dialogCacheCount--;
						}
						else {
							dialogInserted=1;
							dialogCache[i]=dialog;
						}
					}
				}
				if(!dialogInserted) {
					dialogCacheCount++;
					dialogCache=(struct DialogData*)realloc(dialogCache,sizeof(struct DialogData)*dialogCacheCount);
					dialogCache[dialogCacheCount-1]=dialog;
				}
				LeaveCriticalSection(&csDialogCache);
				PostMessage(hwndHelpDlg,M_HELPDOWNLOADED,0,0);
				succeeded=1;
			}
		}
		CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,0,(LPARAM)nlhrReply);
	}
	else MessageBox(NULL,TranslateT("Help can't connect to the server, it may be temporarily down. Please try again later."),TranslateT("Help"),MB_OK);
	free(dtsp->szDlgId);
	free(dtsp->szModule);
	free(dtsp);
	if(!succeeded) PostMessage(hwndHelpDlg,M_HELPDOWNLOADFAILED,0,0);
}

int GetControlHelp(const char *pszDlgId,const char *pszModule,int ctrlId,char **ppszTitle,char **ppszText,int *pType,DWORD flags)
{
	int i,j;
	struct DownloaderThreadStartParams *dtsp;

	EnterCriticalSection(&csDialogCache);
	for(i=0;i<dialogCacheCount;i++) {
		if(lstrcmpA(pszDlgId,dialogCache[i].szId)) continue;
		for(j=0;j<dialogCache[i].controlCount;j++) {
			if(ctrlId!=dialogCache[i].control[j].id) continue;
			if(ppszTitle) *ppszTitle=dialogCache[i].control[j].szTitle;
			if(ppszText) *ppszText=dialogCache[i].control[j].szText;
			if(pType) *pType=dialogCache[i].control[j].type;
			dialogCache[i].timeLastUsed=time(NULL);
			LeaveCriticalSection(&csDialogCache);
			return 0;
		}
		break;
	}
	if(!(flags&GCHF_DONTDOWNLOAD)) {
		dtsp=(struct DownloaderThreadStartParams*)malloc(sizeof(struct DownloaderThreadStartParams));
		dtsp->szDlgId=_strdup(pszDlgId);
		dtsp->szModule=_strdup(pszModule);
		dtsp->ctrlId=ctrlId;
		_beginthread((void (*)(void*))DownloaderThread,0,dtsp);
	}
	LeaveCriticalSection(&csDialogCache);
	return 1;
}

/**************************** UPLOAD ***************************/

#ifdef EDITOR
void SetControlHelp(const char *pszDlgId,const char *pszModule,int ctrlId,char *pszTitle,char *pszText,int type)
{
	int i,j;
	int found=0;

	EnterCriticalSection(&csDialogCache);
	for(i=0;i<dialogCacheCount;i++) {
		if(lstrcmp(pszDlgId,dialogCache[i].szId)) continue;
		for(j=0;j<dialogCache[i].controlCount;j++) {
			if(ctrlId==dialogCache[i].control[j].id) {
				if(dialogCache[i].control[j].szTitle) free(dialogCache[i].control[j].szTitle);
				if(dialogCache[i].control[j].szText) free(dialogCache[i].control[j].szText);
				found=1;
				break;
			}
		}
		if(!found) {
			j=dialogCache[i].controlCount++;
			dialogCache[i].control=(struct DlgControlData*)realloc(dialogCache[i].control,sizeof(struct DlgControlData)*dialogCache[i].controlCount);
			found=1;
		}
		break;
	}
	if(!found) {
		i=dialogCacheCount++;
		dialogCache=(struct DialogData*)realloc(dialogCache,sizeof(struct DialogData)*dialogCacheCount);
		dialogCache[i].control=(struct DlgControlData*)malloc(sizeof(struct DlgControlData));
		dialogCache[i].controlCount=1;
		j=0;
		dialogCache[i].szId=_strdup(pszDlgId);
		dialogCache[i].szModule=_strdup(pszModule);
		dialogCache[i].timeDownloaded=0;
	}
	if(pszTitle) dialogCache[i].control[j].szTitle=_strdup(pszTitle);
	else dialogCache[i].control[j].szTitle=NULL;
	if(pszText) dialogCache[i].control[j].szText=_strdup(pszText);
	else dialogCache[i].control[j].szText=NULL;
	dialogCache[i].control[j].type=type;
	dialogCache[i].control[j].id=ctrlId;
	dialogCache[i].timeLastUsed=time(NULL);
	dialogCache[i].changes=1;
	LeaveCriticalSection(&csDialogCache);
}

static void DialogCacheUploaderThread(void *unused)
{
	int i,j;
	char szServer[80],szPath[128],szLanguage[16],szAuth[128];
	NETLIBHTTPREQUEST nlhr={0},*nlhrReply;
	NETLIBHTTPHEADER headers[1];
	struct ResizableCharBuffer url={0};
	struct ResizableCharBuffer data={0},dlgId={0};
	char *szDataUrlEncoded,*szDlgIdUrlEncoded;

	GetStringOption(OPTION_SERVER,szServer,sizeof(szServer));
	GetStringOption(OPTION_PATH,szPath,sizeof(szPath));
	GetStringOption(OPTION_LANGUAGE,szLanguage,sizeof(szLanguage));
	GetStringOption(OPTION_AUTH,szAuth,sizeof(szAuth));

	for(i=0;i<dialogCacheCount;i++) {
		if(!dialogCache[i].changes) continue;

		AppendToCharBuffer(&data,"t <dialog id=\"%s\" module=\"%s\">\r\n",dialogCache[i].szId,dialogCache[i].szModule);
		for(j=0;j<dialogCache[i].controlCount;j++) {
			AppendToCharBuffer(&data,"<control id=%d type=%d>\r\n",dialogCache[i].control[j].id,dialogCache[i].control[j].type);
			if(dialogCache[i].control[j].szTitle) AppendToCharBuffer(&data,"<title>%s</title>\r\n",dialogCache[i].control[j].szTitle);
			if(dialogCache[i].control[j].szText) AppendToCharBuffer(&data,"<text>%s</text>\r\n",dialogCache[i].control[j].szText);
			AppendToCharBuffer(&data,"</control>\r\n");
		}
		AppendToCharBuffer(&data,"</dialog>");
		szDataUrlEncoded=(char*)CallService(MS_NETLIB_URLENCODE,0,(LPARAM)data.sz);
		szDataUrlEncoded[1]='=';
		free(data.sz);

		szDlgIdUrlEncoded=(char*)CallService(MS_NETLIB_URLENCODE,0,(LPARAM)dialogCache[i].szId);
		AppendToCharBuffer(&url,"http://%s%sdialoghelp.php?module=%s&id=%s&lang=%s&auth=%s&upload=1",
							szServer,szPath,dialogCache[i].szModule,szDlgIdUrlEncoded,szLanguage,szAuth);
		HeapFree(GetProcessHeap(),0,szDlgIdUrlEncoded);
		nlhr.cbSize=sizeof(nlhr);
		nlhr.requestType=REQUEST_POST;
		nlhr.flags=NLHRF_DUMPASTEXT;
		nlhr.szUrl=url.sz;
		nlhr.headersCount=1;
		nlhr.headers=headers;
		headers[0].szName="Content-Type";
		headers[0].szValue="application/x-www-form-urlencoded";
		nlhr.pData=szDataUrlEncoded;
		nlhr.dataLength=lstrlen(szDataUrlEncoded);
		nlhrReply=(NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION,(WPARAM)hNetlibUser,(LPARAM)&nlhr);
		HeapFree(GetProcessHeap(),0,szDataUrlEncoded);
		free(url.sz);
		if(nlhrReply==NULL) {
			MessageBox(NULL,"",Translate("Help Upload failed"),MB_OK);
		}
		else {
			if(nlhrReply->resultCode<200 || nlhrReply->resultCode>=300 || nlhrReply->dataLength) {
				MessageBox(NULL,nlhrReply->szResultDescr,Translate("Help Upload failed"),MB_OK);
			}
			else {
				dialogCache[i].changes=0;
				dialogCache[i].timeDownloaded=time(NULL);
			}
			CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,0,(LPARAM)nlhrReply);
		}
	}
	MessageBox(NULL,Translate("Upload complete"),Translate("Help Upload"),MB_OK);
}

void UploadDialogCache(void)
{
	_beginthread(DialogCacheUploaderThread,0,NULL);
}
#endif  //defined EDITOR

/************************ DOWNLOAD LANGUAGE LIST *********************/

static void GetLangListThread(HWND hwndReply)
{
	NETLIBHTTPREQUEST nlhr={0},*nlhrReply;
	struct ResizableCharBuffer url={0};
	char szServer[80],szPath[128];
	int succeeded=0;

	GetStringOption(OPTION_SERVER,szServer,sizeof(szServer));
	GetStringOption(OPTION_PATH,szPath,sizeof(szPath));
	AppendToCharBuffer(&url,"http://%s%slanglist.php",szServer,szPath);
	nlhr.cbSize=sizeof(nlhr);
	nlhr.requestType=REQUEST_GET;
	nlhr.flags=NLHRF_DUMPASTEXT;
	nlhr.szUrl=url.sz;
	nlhrReply=(NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION,(WPARAM)hNetlibUser,(LPARAM)&nlhr);
	free(url.sz);
	if(nlhrReply) {
		if(nlhrReply->resultCode>=200 && nlhrReply->resultCode<300 && nlhrReply->dataLength) {
			int langId;
			char *pszEndOfNumber;
			int *langList=NULL;
			int langCount=0;
			char *pszData=nlhrReply->pData;
			int i;

			for(i=0;pszData[i];) {
				langId=strtol(pszData+i,&pszEndOfNumber,0x10);
				if(pszEndOfNumber==pszData+i) i++;
				else {
					i=pszEndOfNumber-pszData;
					if(langId) {
						langCount++;
						langList=(int*)realloc(langList,sizeof(int)*langCount);
						langList[langCount-1]=langId;
					}
				}
			}
			CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,0,(LPARAM)nlhrReply);
			PostMessage(hwndReply,M_LANGLIST,langCount,(LPARAM)langList);
			return;
		}
		CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,0,(LPARAM)nlhrReply);
	}
	PostMessage(hwndReply,M_LANGLISTFAILED,0,0);
}

int GetLanguageList(HWND hwndReply)
{
	_beginthread((void (*)(void*))GetLangListThread,0,hwndReply);
	return 0;
}
