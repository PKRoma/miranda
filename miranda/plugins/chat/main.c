/*
Chat module plugin for Miranda IM

Copyright (C) 2003 Jörgen Persson

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

#include "chat.h"

//globals
HINSTANCE		g_hInst;
PLUGINLINK		*pluginLink;
HANDLE			g_hWindowList;
HMENU			g_hMenu = NULL;

FONTINFO		aFonts[OPTIONS_FONTCOUNT];
HICON			hIcons[OPTIONS_ICONCOUNT];
BOOL			SmileyAddInstalled = FALSE;
BOOL			PopUpInstalled = FALSE;
HBRUSH			hEditBkgBrush = NULL;

HIMAGELIST		hImageList = NULL;


char*			pszActiveWndID = 0;
char*			pszActiveWndModule = 0;

static void InitREOleCallback(void);

PLUGININFO pluginInfo = {
	sizeof(PLUGININFO),
	#ifdef _UNICODE
		"Chat (Unicode)",
	#else
		"Chat",
	#endif
	PLUGIN_MAKE_VERSION(0,2,1,1),
	"Provides chat rooms for protocols supporting it",
	"MatriX ' m3x",
	"i_am_matrix@users.sourceforge.net",
	"© 2004 Jörgen Persson",
	"http://miranda-im.org/",
	0,
	0		
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	g_hInst = hinstDLL;
	return TRUE;
}


__declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion)
{
	if(mirandaVersion < PLUGIN_MAKE_VERSION(0,4,0,0)) return NULL;
	return &pluginInfo;
}

int __declspec(dllexport) Load(PLUGINLINK *link)
{
	BOOL bFlag = FALSE;
	HINSTANCE hDll;

	#ifndef NDEBUG //mem leak detector :-) Thanks Tornado!
	int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
	flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
	_CrtSetDbgFlag(flag); // Set flag to the new value
	#endif

	pluginLink = link;

	hDll = LoadLibraryA("riched20.dll");

	if(hDll)
	{
		CHAR modulePath[MAX_PATH];
		if (GetModuleFileName(hDll, modulePath, sizeof(modulePath)))
		{
			DWORD dummy;
			VS_FIXEDFILEINFO* vsInfo;
			UINT vsInfoSize;
			DWORD size = GetFileVersionInfoSize(modulePath, &dummy);
			BYTE* buffer = (BYTE*) malloc(size);

			GetFileVersionInfo(modulePath, 0, size, buffer);
			VerQueryValue(buffer, "\\", (LPVOID*) &vsInfo, &vsInfoSize);
			if(LOWORD(vsInfo->dwFileVersionMS) != 0)
				bFlag= TRUE;

			free(buffer);
		}

	}

	if (!bFlag)
	{
		if(IDYES == MessageBoxA(0,Translate("Miranda could not load the Chat plugin because Microsoft Rich Edit v 3 is missing.\nIf you are using Windows 95/98/NT or WINE please upgrade your Rich Edit control.\n\nDo you want to download an update now?."),Translate("Information"),MB_YESNO|MB_ICONINFORMATION))
			CallService(MS_UTILS_OPENURL, 1, (LPARAM) "http://members.chello.se/matrix/re3/richupd.exe");
		FreeLibrary(GetModuleHandleA("riched20.dll"));
		return 1;
	}

	LoadIcons();
	g_hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_MENU));
    OleInitialize(NULL);
    InitREOleCallback();
	HookEvents();
	CreateServiceFunctions();
	CreateHookableEvents();
	OptionsInit();

	/*
	{ // check sizes of structures in m_chat.h
		int iSize;
		char szTemp[10];

		iSize = sizeof(GCREGISTER);
		_snprintf(szTemp, sizeof(szTemp), "%u", iSize);
		MessageBox(NULL, szTemp, "GCREGISTER", MB_OK);

		iSize = sizeof(GCWINDOW);
		_snprintf(szTemp, sizeof(szTemp), "%u", iSize);
		MessageBox(NULL, szTemp, "GCWINDOW", MB_OK);

		iSize = sizeof(GCEVENT);
		_snprintf(szTemp, sizeof(szTemp), "%u", iSize);
		MessageBox(NULL, szTemp, "GCEVENT", MB_OK);

	}
	*/
	return 0;
}


int __declspec(dllexport) Unload(void)
{
	DBWriteContactSettingWord(NULL, "Chat", "SplitterX", (WORD)g_LogOptions.iSplitterX);
	DBWriteContactSettingWord(NULL, "Chat", "SplitterY", (WORD)g_LogOptions.iSplitterY);
	DBWriteContactSettingDword(NULL, "Chat", "roomx", g_LogOptions.iX);
	DBWriteContactSettingDword(NULL, "Chat", "roomy", g_LogOptions.iY);
	DBWriteContactSettingDword(NULL, "Chat", "roomwidth" , g_LogOptions.iWidth);
	DBWriteContactSettingDword(NULL, "Chat", "roomheight", g_LogOptions.iHeight);

	CList_SetAllOffline(TRUE);

	if(pszActiveWndID)
		free(pszActiveWndID);
	if(pszActiveWndModule)
		free(pszActiveWndModule);

	DestroyMenu(g_hMenu);
	FreeIcons();
	OptionsUnInit();
    FreeLibrary(GetModuleHandleA("riched20.dll"));
    OleUninitialize();
	UnhookEvents();
	return 0;
}
int __declspec(dllexport) UninstallEx(PLUGINUNINSTALLPARAMS* ppup)
{
	if (ppup && ppup->bDoDeleteSettings) 
	{
		PUIRemoveDbModule("Chat");
		PUIRemoveDbModule("ChatFonts");

		PUIRemoveSkinSound("ChatMessage");
		PUIRemoveSkinSound("ChatHighlight"); 
		PUIRemoveSkinSound("ChatAction"); 
		PUIRemoveSkinSound((WPARAM) "ChatJoin"); 
		PUIRemoveSkinSound((WPARAM) "ChatKick"); 
		PUIRemoveSkinSound((WPARAM) "ChatMode"); 
		PUIRemoveSkinSound((WPARAM) "ChatNick"); 
		PUIRemoveSkinSound((WPARAM) "ChatNotice"); 
		PUIRemoveSkinSound((WPARAM) "ChatPart"); 
		PUIRemoveSkinSound((WPARAM) "ChatQuit"); 
		PUIRemoveSkinSound((WPARAM) "ChatTopic"); 
	}
	return 0; 
}

void LoadIcons(void)
{
	int i;

	for(i = 0; i < OPTIONS_ICONCOUNT; i++)
		hIcons[i] = LoadImage(g_hInst,MAKEINTRESOURCE(120+i),IMAGE_ICON,0,0,0);
	LoadMsgLogBitmaps();

	hImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32|ILC_MASK,0,3);
	ImageList_AddIcon(hImageList,LoadImage(g_hInst,MAKEINTRESOURCE(IDI_BLANK),IMAGE_ICON,0,0,0));
	ImageList_AddIcon(hImageList,LoadImage(g_hInst,MAKEINTRESOURCE(IDI_BLANK),IMAGE_ICON,0,0,0));
	g_LogOptions.TagIcon1 = LoadImage(g_hInst,MAKEINTRESOURCE(IDI_TAG1),IMAGE_ICON,0,0,0);
	g_LogOptions.TagIcon2 = LoadImage(g_hInst,MAKEINTRESOURCE(IDI_TAG2),IMAGE_ICON,0,0,0);
	return ;
}

void FreeIcons(void)
{
	int i;
	for(i = 0; i < OPTIONS_ICONCOUNT; i++)
		DeleteObject(hIcons[i]);
	FreeMsgLogBitmaps();
	ImageList_Destroy(hImageList);
	DeleteObject(g_LogOptions.TagIcon1);
	DeleteObject(g_LogOptions.TagIcon2);
	return;
}

static IRichEditOleCallbackVtbl reOleCallbackVtbl;
struct CREOleCallback reOleCallback;

static STDMETHODIMP_(ULONG) CREOleCallback_QueryInterface(struct CREOleCallback *lpThis, REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, &IID_IRichEditOleCallback)) {
        *ppvObj = lpThis;
        lpThis->lpVtbl->AddRef((IRichEditOleCallback *) lpThis);
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

static STDMETHODIMP_(ULONG) CREOleCallback_AddRef(struct CREOleCallback *lpThis)
{
    if (lpThis->refCount == 0) {
        if (S_OK != StgCreateDocfile(NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_DELETEONRELEASE, 0, &lpThis->pictStg))
            lpThis->pictStg = NULL;
        lpThis->nextStgId = 0;
    }
    return ++lpThis->refCount;
}

static STDMETHODIMP_(ULONG) CREOleCallback_Release(struct CREOleCallback *lpThis)
{
    if (--lpThis->refCount == 0) {
        if (lpThis->pictStg)
            lpThis->pictStg->lpVtbl->Release(lpThis->pictStg);
    }
    return lpThis->refCount;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_ContextSensitiveHelp(struct CREOleCallback *lpThis, BOOL fEnterMode)
{
    return S_OK;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_DeleteObject(struct CREOleCallback *lpThis, LPOLEOBJECT lpoleobj)
{
    return S_OK;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_GetClipboardData(struct CREOleCallback *lpThis, CHARRANGE * lpchrg, DWORD reco, LPDATAOBJECT * lplpdataobj)
{
    return E_NOTIMPL;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_GetContextMenu(struct CREOleCallback *lpThis, WORD seltype, LPOLEOBJECT lpoleobj, CHARRANGE * lpchrg, HMENU * lphmenu)
{
    return E_INVALIDARG;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_GetDragDropEffect(struct CREOleCallback *lpThis, BOOL fDrag, DWORD grfKeyState, LPDWORD pdwEffect)
{
    return S_OK;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_GetInPlaceContext(struct CREOleCallback *lpThis, LPOLEINPLACEFRAME * lplpFrame, LPOLEINPLACEUIWINDOW * lplpDoc, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    return E_INVALIDARG;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_GetNewStorage(struct CREOleCallback *lpThis, LPSTORAGE * lplpstg)
{
    WCHAR szwName[64];
    char szName[64];
    wsprintfA(szName, "s%u", lpThis->nextStgId);
    MultiByteToWideChar(CP_ACP, 0, szName, -1, szwName, sizeof(szwName) / sizeof(szwName[0]));
    if (lpThis->pictStg == NULL)
        return STG_E_MEDIUMFULL;
    return lpThis->pictStg->lpVtbl->CreateStorage(lpThis->pictStg, szwName, STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, 0, lplpstg);
}

static STDMETHODIMP_(HRESULT) CREOleCallback_QueryAcceptData(struct CREOleCallback *lpThis, LPDATAOBJECT lpdataobj, CLIPFORMAT * lpcfFormat, DWORD reco, BOOL fReally, HGLOBAL hMetaPict)
{
    return S_OK;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_QueryInsertObject(struct CREOleCallback *lpThis, LPCLSID lpclsid, LPSTORAGE lpstg, LONG cp)
{
    return S_OK;
}

static STDMETHODIMP_(HRESULT) CREOleCallback_ShowContainerUI(struct CREOleCallback *lpThis, BOOL fShow)
{
    return S_OK;
}

static void InitREOleCallback(void)
{
    reOleCallback.lpVtbl = &reOleCallbackVtbl;
    reOleCallback.lpVtbl->AddRef = (ULONG(__stdcall *) (IRichEditOleCallback *)) CREOleCallback_AddRef;
    reOleCallback.lpVtbl->Release = (ULONG(__stdcall *) (IRichEditOleCallback *)) CREOleCallback_Release;
    reOleCallback.lpVtbl->QueryInterface = (ULONG(__stdcall *) (IRichEditOleCallback *, REFIID, PVOID *)) CREOleCallback_QueryInterface;
    reOleCallback.lpVtbl->ContextSensitiveHelp = (HRESULT(__stdcall *) (IRichEditOleCallback *, BOOL)) CREOleCallback_ContextSensitiveHelp;
    reOleCallback.lpVtbl->DeleteObject = (HRESULT(__stdcall *) (IRichEditOleCallback *, LPOLEOBJECT)) CREOleCallback_DeleteObject;
    reOleCallback.lpVtbl->GetClipboardData = (HRESULT(__stdcall *) (IRichEditOleCallback *, CHARRANGE *, DWORD, LPDATAOBJECT *)) CREOleCallback_GetClipboardData;
    reOleCallback.lpVtbl->GetContextMenu = (HRESULT(__stdcall *) (IRichEditOleCallback *, WORD, LPOLEOBJECT, CHARRANGE *, HMENU *)) CREOleCallback_GetContextMenu;
    reOleCallback.lpVtbl->GetDragDropEffect = (HRESULT(__stdcall *) (IRichEditOleCallback *, BOOL, DWORD, LPDWORD)) CREOleCallback_GetDragDropEffect;
    reOleCallback.lpVtbl->GetInPlaceContext = (HRESULT(__stdcall *) (IRichEditOleCallback *, LPOLEINPLACEFRAME *, LPOLEINPLACEUIWINDOW *, LPOLEINPLACEFRAMEINFO))
        CREOleCallback_GetInPlaceContext;
    reOleCallback.lpVtbl->GetNewStorage = (HRESULT(__stdcall *) (IRichEditOleCallback *, LPSTORAGE *)) CREOleCallback_GetNewStorage;
    reOleCallback.lpVtbl->QueryAcceptData = (HRESULT(__stdcall *) (IRichEditOleCallback *, LPDATAOBJECT, CLIPFORMAT *, DWORD, BOOL, HGLOBAL)) CREOleCallback_QueryAcceptData;
    reOleCallback.lpVtbl->QueryInsertObject = (HRESULT(__stdcall *) (IRichEditOleCallback *, LPCLSID, LPSTORAGE, LONG)) CREOleCallback_QueryInsertObject;
    reOleCallback.lpVtbl->ShowContainerUI = (HRESULT(__stdcall *) (IRichEditOleCallback *, BOOL)) CREOleCallback_ShowContainerUI;
    reOleCallback.refCount = 0;
}
