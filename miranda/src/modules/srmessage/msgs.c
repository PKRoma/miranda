/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
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
#include "../../core/commonheaders.h"
#include "msgs.h"

static void InitREOleCallback(void);

HCURSOR hCurSplitNS,hCurSplitWE,hCurHyperlinkHand;
HANDLE hMessageWindowList,hMessageSendList;
static HANDLE hEventDbEventAdded,hEventDbSettingChange;

static int ReadMessageCommand(WPARAM wParam,LPARAM lParam)
{
	struct NewMessageWindowLParam newData={0};
	HWND hwndExisting;
	int usingReadNext=DBGetContactSettingByte(NULL,"SRMsg","ShowReadNext",SRMSGDEFSET_SHOWREADNEXT);

	hwndExisting=WindowList_Find(hMessageWindowList,((CLISTEVENT*)lParam)->hContact);
	if(usingReadNext && hwndExisting!=NULL) {
		ShowWindow(hwndExisting,SW_SHOWNORMAL);
		SetForegroundWindow(hwndExisting);
		SetFocus(hwndExisting);
		SendMessage(hwndExisting,WM_COMMAND,MAKEWPARAM(IDC_READNEXT,BN_CLICKED),(LPARAM)GetDlgItem(hwndExisting,IDC_READNEXT));
	}
	else {
		newData.hContact=((CLISTEVENT*)lParam)->hContact;
		newData.isSend=0;
		CreateDialogParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_MSGSPLIT),NULL,DlgProcMessage,(LPARAM)&newData);
	}
	return 0;
}

static int MessageEventAdded(WPARAM wParam,LPARAM lParam)
{
	CLISTEVENT cle;
	DBEVENTINFO dbei;
	char *contactName;
	char toolTip[256];
	HWND hwnd;
	int usingReadNext=DBGetContactSettingByte(NULL,"SRMsg","ShowReadNext",SRMSGDEFSET_SHOWREADNEXT);
	int split=DBGetContactSettingByte(NULL,"SRMsg","Split",SRMSGDEFSET_SPLIT);

	ZeroMemory(&dbei,sizeof(dbei));
	dbei.cbSize=sizeof(dbei);
	dbei.cbBlob=0;
	CallService(MS_DB_EVENT_GET,lParam,(LPARAM)&dbei);

	if(dbei.flags&(DBEF_SENT|DBEF_READ) || dbei.eventType!=EVENTTYPE_MESSAGE) return 0;

	/* does a window for the contact exist? */	
	hwnd=WindowList_Find(hMessageWindowList,(HANDLE)wParam);
	if (hwnd) SkinPlaySound("AlertMsg");
	/* yeah, using split/read next? then it'll notice itself */
	if((usingReadNext||split)&&hwnd!=NULL) return 0;  //Msg window will have noticed this event arriving itself
	/* new message */
	SkinPlaySound("RecvMsg");

	if(DBGetContactSettingByte(NULL,"SRMsg","AutoPopup",SRMSGDEFSET_AUTOPOPUP)) {
		struct NewMessageWindowLParam newData={0};
		newData.hContact=(HANDLE)wParam;
		newData.isSend=0;
		CreateDialogParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_MSGSPLIT),NULL,DlgProcMessage,(LPARAM)&newData);
		return 0;
	}

	ZeroMemory(&cle,sizeof(cle));
	cle.cbSize=sizeof(cle);
	cle.hContact=(HANDLE)wParam;
	cle.hDbEvent=(HANDLE)lParam;
	cle.hIcon=LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
	cle.pszService="SRMsg/ReadMessage";
	contactName=(char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,wParam,0);
	_snprintf(toolTip,sizeof(toolTip),Translate("Message from %s"),contactName);
	cle.pszTooltip=toolTip;
	CallService(MS_CLIST_ADDEVENT,0,(LPARAM)&cle);
	return 0;
}

static int SendMessageCommand(WPARAM wParam,LPARAM lParam)
{
	HWND hwnd;
	struct NewMessageWindowLParam newData={0};
	int isSplit=DBGetContactSettingByte(NULL,"SRMsg","Split",SRMSGDEFSET_SPLIT);

	{
		/* does the HCONTACT's protocol support IM messages? */
		char *szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,wParam,0);
		if (szProto) {
			if (!CallProtoService(szProto,PS_GETCAPS,PFLAGNUM_1,0)&PF1_IMSEND) return 0;
		} else {
			/* unknown contact */
			return 0;
		}//if
	}

	if(hwnd=WindowList_Find(isSplit?hMessageWindowList:hMessageSendList,(HANDLE)wParam)) {
		if (lParam) {
			HWND hEdit;
			hEdit=GetDlgItem(hwnd,IDC_MESSAGE);
			SendMessage(hEdit,EM_SETSEL,-1,SendMessage(hEdit,WM_GETTEXTLENGTH,0,0));		
			SendMessage(hEdit,EM_REPLACESEL,FALSE,(LPARAM)(char*)lParam);
		} 
		ShowWindow(hwnd,SW_SHOWNORMAL);
		SetForegroundWindow(hwnd);
		SetFocus(hwnd);
	}
	else {
		newData.hContact=(HANDLE)wParam;
		newData.isSend=!isSplit;
		newData.szInitialText=(const char*)lParam;
		CreateDialogParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_MSGSPLIT),NULL,DlgProcMessage,(LPARAM)&newData);
	}
	return 0;
}

static int ForwardMessage(WPARAM wParam,LPARAM lParam)
{
	struct NewMessageWindowLParam newData={0};
	newData.hContact=NULL;
	newData.isSend=!DBGetContactSettingByte(NULL,"SRMsg","Split",SRMSGDEFSET_SPLIT);
	newData.szInitialText=(const char*)lParam;
	CreateDialogParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_MSGSPLIT),NULL,DlgProcMessage,(LPARAM)&newData);
	return 0;
}

static int MessageSettingChanged(WPARAM wParam,LPARAM lParam)
{
	DBCONTACTWRITESETTING *cws=(DBCONTACTWRITESETTING*)lParam;
	char *szProto;

	szProto=(char*)CallService(MS_PROTO_GETCONTACTBASEPROTO,wParam,0);
	if(lstrcmp(cws->szModule,"CList") && (szProto==NULL || lstrcmp(cws->szModule,szProto))) return 0;
	WindowList_Broadcast(hMessageWindowList,DM_UPDATETITLE,(WPARAM)cws,0);
	WindowList_Broadcast(hMessageSendList,DM_UPDATETITLE,(WPARAM)cws,0);
	return 0;
}

static void RestoreUnreadMessageAlerts(void)
{
	CLISTEVENT cle={0};
	DBEVENTINFO dbei={0};
	char toolTip[256];
	int windowAlreadyExists;
	int usingReadNext=DBGetContactSettingByte(NULL,"SRMsg","ShowReadNext",SRMSGDEFSET_SHOWREADNEXT);
	int autoPopup=DBGetContactSettingByte(NULL,"SRMsg","AutoPopup",SRMSGDEFSET_AUTOPOPUP);
	HANDLE hDbEvent,hContact;

	dbei.cbSize=sizeof(dbei);
	cle.cbSize=sizeof(cle);
	cle.hIcon=LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
	cle.pszService="SRMsg/ReadMessage";

	hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST,0,0);
	while(hContact) {
		hDbEvent=(HANDLE)CallService(MS_DB_EVENT_FINDFIRSTUNREAD,(WPARAM)hContact,0);
		while(hDbEvent) {
			dbei.cbBlob=0;
			CallService(MS_DB_EVENT_GET,(WPARAM)hDbEvent,(LPARAM)&dbei);
			if(!(dbei.flags&(DBEF_SENT|DBEF_READ)) && dbei.eventType==EVENTTYPE_MESSAGE) {
				windowAlreadyExists=WindowList_Find(hMessageWindowList,hContact)!=NULL;
				if(!usingReadNext && windowAlreadyExists) continue;

				if(autoPopup && !windowAlreadyExists) {
					struct NewMessageWindowLParam newData={0};
					newData.hContact=hContact;
					newData.isSend=0;
					CreateDialogParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_MSGSPLIT),NULL,DlgProcMessage,(LPARAM)&newData);
				}
				else {
					cle.hContact=hContact;
					cle.hDbEvent=hDbEvent;
					_snprintf(toolTip,sizeof(toolTip),Translate("Message from %s"),(char*)CallService(MS_CLIST_GETCONTACTDISPLAYNAME,(WPARAM)hContact,0));
					cle.pszTooltip=toolTip;
					CallService(MS_CLIST_ADDEVENT,0,(LPARAM)&cle);
				}
			}
			hDbEvent=(HANDLE)CallService(MS_DB_EVENT_FINDNEXT,(WPARAM)hDbEvent,0);
		}
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT,(WPARAM)hContact,0);
	}
}

static int SplitmsgModulesLoaded(WPARAM wParam,LPARAM lParam)
{
	CLISTMENUITEM mi;
	PROTOCOLDESCRIPTOR **protocol;
	int protoCount,i;

	LoadMsgLogIcons();
	ZeroMemory(&mi,sizeof(mi));
	mi.cbSize=sizeof(mi);
	mi.position=-2000090000;
	mi.flags=0;
	mi.hIcon=LoadSkinnedIcon(SKINICON_EVENT_MESSAGE);
	mi.pszName=Translate("&Message");
	mi.pszService=MS_MSG_SENDMESSAGE;
	CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&protoCount,(LPARAM)&protocol);
	for(i=0;i<protoCount;i++) {
		if(protocol[i]->type!=PROTOTYPE_PROTOCOL) continue;
		if(CallProtoService(protocol[i]->szName,PS_GETCAPS,PFLAGNUM_1,0)&PF1_IMSEND) {
			mi.pszContactOwner=protocol[i]->szName;
			CallService(MS_CLIST_ADDCONTACTMENUITEM,0,(LPARAM)&mi);
		}
	}
	/*temp: Til the session manager is finished and Rob has the new splitmode plugin */
	HookEvent(ME_CLIST_DOUBLECLICKED,SendMessageCommand);
	RestoreUnreadMessageAlerts();
	return 0;
}

static int SplitmsgShutdown(WPARAM wParam,LPARAM lParam)
{
	DestroyCursor(hCurSplitNS);
	DestroyCursor(hCurHyperlinkHand);
	DestroyCursor(hCurSplitWE);
	UnhookEvent(hEventDbEventAdded);
	UnhookEvent(hEventDbSettingChange);
	FreeMsgLogIcons();
	FreeLibrary(GetModuleHandle("riched20"));
	OleUninitialize();
	return 0;
}

static int IconsChanged(WPARAM wParam,LPARAM lParam)
{
	FreeMsgLogIcons();
	LoadMsgLogIcons();
	WindowList_Broadcast(hMessageWindowList,DM_REMAKELOG,0,0);
	// change all the icons
	WindowList_Broadcast(hMessageWindowList, WM_SETICON, ICON_SMALL, (LPARAM)LoadSkinnedIcon(SKINICON_EVENT_MESSAGE));
	WindowList_Broadcast(hMessageSendList, WM_SETICON, ICON_SMALL, (LPARAM)LoadSkinnedIcon(SKINICON_EVENT_MESSAGE));
	return 0;
}

int LoadSendRecvMessageModule(void)
{
	if(LoadLibrary("riched20.dll")==NULL) {
		if (IDYES!=MessageBox(0,Translate("Miranda could not load the built-in message module, riched20.dll is missing. If you are using Windows 95 or WINE please make sure you have riched20.dll installed. Press 'Yes' to continue loading Miranda."),Translate("Information"),MB_YESNO|MB_ICONINFORMATION)) return 1;
		return 0;
	}
	OleInitialize(NULL);
	InitREOleCallback();
	hMessageWindowList=(HANDLE)CallService(MS_UTILS_ALLOCWINDOWLIST,0,0);
	hMessageSendList=(HANDLE)CallService(MS_UTILS_ALLOCWINDOWLIST,0,0);
	InitOptions();
	hEventDbEventAdded=HookEvent(ME_DB_EVENT_ADDED,MessageEventAdded);
	hEventDbSettingChange=HookEvent(ME_DB_CONTACT_SETTINGCHANGED,MessageSettingChanged);
	HookEvent(ME_SYSTEM_SHUTDOWN,SplitmsgShutdown);
	HookEvent(ME_SYSTEM_MODULESLOADED,SplitmsgModulesLoaded);
	HookEvent(ME_SKIN_ICONSCHANGED,IconsChanged);
	CreateServiceFunction(MS_MSG_SENDMESSAGE,SendMessageCommand);
	CreateServiceFunction(MS_MSG_FORWARDMESSAGE,ForwardMessage);
	CreateServiceFunction("SRMsg/ReadMessage",ReadMessageCommand);
	SkinAddNewSound("RecvMsg",Translate("Message: Queued Incoming"),"message.wav");
	SkinAddNewSound("AlertMsg",Translate("Message: Incoming"),"messagealert.wav");
	SkinAddNewSound("SendMsg",Translate("Message: Outgoing"),"outgoing.wav");
	hCurSplitNS=LoadCursor(NULL,IDC_SIZENS);
	hCurSplitWE=LoadCursor(NULL,IDC_SIZEWE);
	hCurHyperlinkHand=LoadCursor(NULL,IDC_HAND);
	if(hCurHyperlinkHand==NULL) hCurHyperlinkHand=LoadCursor(GetModuleHandle(NULL),MAKEINTRESOURCE(IDC_HYPERLINKHAND));
	return 0;
}

//remember to free() return value
char *QuoteText(char *text,int charsPerLine,int removeExistingQuotes)
{
	int inChar,outChar,lineChar;
	int justDoneLineBreak,bufSize;
	char *strout;

	bufSize=strlen(text)+23;
	strout=(char*)malloc(bufSize);
	inChar=0;
	justDoneLineBreak=1;
	for(outChar=0,lineChar=0;text[inChar];) {
		if(outChar>=bufSize-8) {
			bufSize+=20;
			strout=(char*)realloc(strout,bufSize);
		}
		if(justDoneLineBreak && text[inChar]!='\r' && text[inChar]!='\n') {
			if(removeExistingQuotes)
				if(text[inChar]=='>') {
					while(text[++inChar]!='\n');
					inChar++;
					continue;
				}
			strout[outChar++]='>'; strout[outChar++]=' ';
			lineChar=2;
		}
		if(lineChar==charsPerLine && text[inChar]!='\r' && text[inChar]!='\n') {
			int decreasedBy;
			for(decreasedBy=0;lineChar>10;lineChar--,inChar--,outChar--,decreasedBy++)
				if(strout[outChar]==' ' || strout[outChar]=='\t' || strout[outChar]=='-') break;
			if(lineChar<=10) {lineChar+=decreasedBy; inChar+=decreasedBy; outChar+=decreasedBy;}
			else inChar++;
			strout[outChar++]='\r'; strout[outChar++]='\n';
			justDoneLineBreak=1;
			continue;
		}
		strout[outChar++]=text[inChar];
		lineChar++;
		if(text[inChar]=='\n' || text[inChar]=='\r') {
			if(text[inChar]=='\r' && text[inChar+1]!='\n')
				strout[outChar++]='\n';
			justDoneLineBreak=1;
			lineChar=0;
		}
		else justDoneLineBreak=0;
		inChar++;
	}
	strout[outChar++]='\r';
	strout[outChar++]='\n';
	strout[outChar]=0;
	return strout;
}

static IRichEditOleCallbackVtbl reOleCallbackVtbl;
struct CREOleCallback reOleCallback;

static STDMETHODIMP_(ULONG) CREOleCallback_QueryInterface(struct CREOleCallback *lpThis,REFIID riid,LPVOID *ppvObj)
{
	if(IsEqualIID(riid,&IID_IRichEditOleCallback)) {
		*ppvObj=lpThis;
		lpThis->lpVtbl->AddRef((IRichEditOleCallback*)lpThis);
		return S_OK;
	}
	*ppvObj=NULL;
	return E_NOINTERFACE;
}

static STDMETHODIMP_(ULONG) CREOleCallback_AddRef(struct CREOleCallback *lpThis)
{
	if(lpThis->refCount==0) {
		if(S_OK!=StgCreateDocfile(NULL,STGM_READWRITE|STGM_SHARE_EXCLUSIVE|STGM_CREATE|STGM_DELETEONRELEASE,0,&lpThis->pictStg))
			lpThis->pictStg=NULL;
		lpThis->nextStgId=0;
	}
	return ++lpThis->refCount;
}

static STDMETHODIMP_(ULONG) CREOleCallback_Release(struct CREOleCallback *lpThis)
{
	if(--lpThis->refCount==0) {
		if(lpThis->pictStg) lpThis->pictStg->lpVtbl->Release(lpThis->pictStg);
	}
	return lpThis->refCount;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_ContextSensitiveHelp(struct CREOleCallback *lpThis,BOOL fEnterMode)
{
	return S_OK;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_DeleteObject(struct CREOleCallback *lpThis,LPOLEOBJECT lpoleobj)
{
	return S_OK;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_GetClipboardData(struct CREOleCallback *lpThis,CHARRANGE *lpchrg,DWORD reco,LPDATAOBJECT *lplpdataobj)
{
	return E_NOTIMPL;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_GetContextMenu(struct CREOleCallback *lpThis,WORD seltype,LPOLEOBJECT lpoleobj,CHARRANGE *lpchrg,HMENU *lphmenu)
{
	return E_INVALIDARG;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_GetDragDropEffect(struct CREOleCallback *lpThis,BOOL fDrag,DWORD grfKeyState,LPDWORD pdwEffect)
{
	return S_OK;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_GetInPlaceContext(struct CREOleCallback *lpThis,LPOLEINPLACEFRAME *lplpFrame,LPOLEINPLACEUIWINDOW *lplpDoc,LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
	return E_INVALIDARG;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_GetNewStorage(struct CREOleCallback *lpThis,LPSTORAGE *lplpstg)
{
	WCHAR szwName[64];
	char szName[64];
	wsprintf(szName,"s%u",lpThis->nextStgId);
	MultiByteToWideChar(CP_ACP,0,szName,-1,szwName,sizeof(szwName)/sizeof(szwName[0]));
	if(lpThis->pictStg==NULL) return STG_E_MEDIUMFULL;
	return lpThis->pictStg->lpVtbl->CreateStorage(lpThis->pictStg,szwName,STGM_READWRITE|STGM_SHARE_EXCLUSIVE|STGM_CREATE,0,0,lplpstg);
}

static STDMETHODIMP_(HRESULT) CREOleCallback_QueryAcceptData(struct CREOleCallback *lpThis,LPDATAOBJECT lpdataobj,CLIPFORMAT *lpcfFormat,DWORD reco,BOOL fReally,HGLOBAL hMetaPict)
{
	return S_OK;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_QueryInsertObject(struct CREOleCallback *lpThis,LPCLSID lpclsid,LPSTORAGE lpstg,LONG cp)
{
	return S_OK;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_ShowContainerUI(struct CREOleCallback *lpThis,BOOL fShow)
{
	return S_OK;
}

static void InitREOleCallback(void)
{
	reOleCallback.lpVtbl=&reOleCallbackVtbl;
	reOleCallback.lpVtbl->AddRef=(ULONG(__stdcall *)(IRichEditOleCallback*))CREOleCallback_AddRef;
	reOleCallback.lpVtbl->Release=(ULONG(__stdcall *)(IRichEditOleCallback*))CREOleCallback_Release;
	reOleCallback.lpVtbl->QueryInterface=(ULONG(__stdcall *)(IRichEditOleCallback*,REFIID,PVOID*))CREOleCallback_QueryInterface;
	reOleCallback.lpVtbl->ContextSensitiveHelp=(HRESULT(__stdcall *)(IRichEditOleCallback*,BOOL))CREOleCallback_ContextSensitiveHelp;
	reOleCallback.lpVtbl->DeleteObject=(HRESULT(__stdcall *)(IRichEditOleCallback*,LPOLEOBJECT))CREOleCallback_DeleteObject;
	reOleCallback.lpVtbl->GetClipboardData=(HRESULT(__stdcall *)(IRichEditOleCallback*,CHARRANGE*,DWORD,LPDATAOBJECT*))CREOleCallback_GetClipboardData;
	reOleCallback.lpVtbl->GetContextMenu=(HRESULT(__stdcall *)(IRichEditOleCallback*,WORD,LPOLEOBJECT,CHARRANGE*,HMENU*))CREOleCallback_GetContextMenu;
	reOleCallback.lpVtbl->GetDragDropEffect=(HRESULT(__stdcall *)(IRichEditOleCallback*,BOOL,DWORD,LPDWORD))CREOleCallback_GetDragDropEffect;
	reOleCallback.lpVtbl->GetInPlaceContext=(HRESULT(__stdcall *)(IRichEditOleCallback*,LPOLEINPLACEFRAME*,LPOLEINPLACEUIWINDOW*,LPOLEINPLACEFRAMEINFO))CREOleCallback_GetInPlaceContext;
	reOleCallback.lpVtbl->GetNewStorage=(HRESULT(__stdcall *)(IRichEditOleCallback*,LPSTORAGE*))CREOleCallback_GetNewStorage;
	reOleCallback.lpVtbl->QueryAcceptData=(HRESULT(__stdcall *)(IRichEditOleCallback*,LPDATAOBJECT,CLIPFORMAT*,DWORD,BOOL,HGLOBAL))CREOleCallback_QueryAcceptData;
	reOleCallback.lpVtbl->QueryInsertObject=(HRESULT(__stdcall *)(IRichEditOleCallback*,LPCLSID,LPSTORAGE,LONG))CREOleCallback_QueryInsertObject;
	reOleCallback.lpVtbl->ShowContainerUI=(HRESULT(__stdcall *)(IRichEditOleCallback*,BOOL))CREOleCallback_ShowContainerUI;
	reOleCallback.refCount=0;
}