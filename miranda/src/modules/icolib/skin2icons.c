// ---------------------------------------------------------------------------80
//         Icons Library Manager plugin for Miranda Instant Messenger
//         __________________________________________________________
//
// Copyright © 2005 Denis Stanishevskiy // StDenis
// Copyright © 2006 Joe Kucera, Bio
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
//
// -----------------------------------------------------------------------------

#define _WIN32_IE 0x0560
#define _WIN32_WINNT 0x0501

#include "commonheaders.h"
#include "m_icolib.h"

#include "IcoLib.h"


static HANDLE hIcons2ChangedEvent;
static HICON hIconBlank = NULL;

HANDLE hIcoLib_AddNewIcon, hIcoLib_RemoveIcon, hIcoLib_GetIcon, hIcoLib_ReleaseIcon;

static HIMAGELIST hDefaultIconList;
static IconItem *iconList;
static int iconCount = 0;
static int iconEventActive = 0;

CRITICAL_SECTION csIconList;

static SectionItem *sectionList;
static int sectionCount = 0;

struct IcoLibOptsData {
  HWND hwndIndex;
};


static void __fastcall MySetCursor(TCHAR* nCursor);

static void __fastcall SAFE_FREE(void** p);

static void __fastcall SafeDestroyIcon(HICON* icon);

static size_t __fastcall strlennull(const TCHAR *string);
static int __fastcall strcmpnull(const TCHAR *str1, const TCHAR *str2);
static int __fastcall strcmpnullA(const char *str1, const char *str2);
static TCHAR* __fastcall null_strdup(const TCHAR *string);
static char* __fastcall null_strdupA(const char *string);
static int null_snprintf(TCHAR *buffer, size_t count, const TCHAR* fmt, ...);

static wchar_t* __fastcall AnsiToUnicode(const char* src);
static wchar_t* __fastcall AnsiListToUnicode(const char* srclist);
static char* __fastcall UnicodeToAnsi(const wchar_t* src);


static DWORD CalcNameHashT(const TCHAR *szStr);
static DWORD CalcNameHashW(const wchar_t *szStr);
static DWORD CalcNameHash(const char *szStr);



//
//  Services
//

static HTREEITEM FindNamedTreeItemAt(HWND hwndTree, HTREEITEM hItem, const TCHAR *name)
{
  TVITEM tvi = {0};
  TCHAR str[MAX_PATH];

  if (hItem)
    tvi.hItem = TreeView_GetChild(hwndTree, hItem);
  else
    tvi.hItem = TreeView_GetRoot(hwndTree);

  if (!name)
    return tvi.hItem;

  tvi.mask = TVIF_TEXT;
  tvi.pszText = str;
  tvi.cchTextMax = MAX_PATH;

  while (tvi.hItem)
  {
    TreeView_GetItem(hwndTree, &tvi);
    
    if (!strcmpnull(tvi.pszText, name))
      return tvi.hItem;

    tvi.hItem = TreeView_GetNextSibling(hwndTree, tvi.hItem);
  }
  return NULL;
}


static HICON ExtractIconFromPath(const TCHAR *path, int cxIcon, int cyIcon)
{
  TCHAR *comma;
  TCHAR file[MAX_PATH],fileFull[MAX_PATH];
  int n;
  HICON hIcon;

  if (!path) return (HICON)NULL;

  lstrcpyn(file, path, sizeof(file));
  comma = _tcsrchr(file,',');
  if (!comma)
    n=0;
  else 
  {  
    n=_ttoi(comma+1);
    *comma=0;
  }
  CallService(MS_UTILS_PATHTOABSOLUTET, (WPARAM)file, (LPARAM)fileFull);
  hIcon = NULL;

  //SHOULD BE REPLACED WITH GOOD ENOUGH FUNCTION
  _ExtractIconEx(fileFull, n, cxIcon, cyIcon, &hIcon, LR_COLOR);
  
  return hIcon;
}


static int IcoLib_AddSection(TCHAR *sectionName, BOOL create_new)
{
  int indx;
  DWORD dwSectionNameHash = CalcNameHashT(sectionName);

  for (indx = 0; indx < sectionCount; indx++)
  {
    SectionItem *section = &sectionList[indx];

    if (dwSectionNameHash == section->name_hash)
      if (!strcmpnull(section->name, sectionName))
        return indx;
  }
  if (!create_new)
    return -1;

  sectionCount++;
  sectionList = (SectionItem*)realloc(sectionList, sizeof(SectionItem)*sectionCount);
  sectionList[indx].name = null_strdup(sectionName);
  sectionList[indx].name_hash = dwSectionNameHash;
  sectionList[indx].flags = 0;

  return indx;
}


static IconItem* IcoLib_FindIcon(const char* pszIconName)
{
  int indx;
  DWORD dwIconNameHash = CalcNameHash(pszIconName);
  
  for (indx = 0; indx < iconCount; indx++)
  {
    IconItem *item = &iconList[indx];

    if (dwIconNameHash == item->name_hash)
    {
      if (!strcmpnullA(item->name, pszIconName))
      {
        return item;
      }
    }
  }
  return NULL;
}


static void IcoLib_FreeIcon(IconItem* icon)
{
  SAFE_FREE(&icon->name);
  SAFE_FREE(&icon->description);
  SAFE_FREE(&icon->default_file);
  SAFE_FREE(&icon->temp_file);
  if (icon->icon != icon->default_icon)
    SafeDestroyIcon(&icon->icon);
  SafeDestroyIcon(&icon->default_icon);
  SafeDestroyIcon(&icon->temp_icon);
}


static int IcoLib_AddNewIcon(WPARAM wParam,LPARAM lParam)
{
  SKINICONDESC *sid = (SKINICONDESC*)lParam;
  int indx;
  int utf = 0;
  IconItem *item;

  if (!sid->cbSize) return 0x10000000;

  if (sid->cbSize < SKINICONDESC_SIZE_V1)
    return 1;

  if (sid->cbSize >= SKINICONDESC_SIZE)
    utf = sid->flags & SIDF_UNICODE ? 1 : 0;

  EnterCriticalSection(&csIconList);

  item = IcoLib_FindIcon(sid->pszName);

  if (!item)
  {
    iconList = (IconItem*)realloc(iconList,sizeof(IconItem)*(iconCount+1));
    indx = iconCount++;
    item = &iconList[indx];
  }
  else
    IcoLib_FreeIcon(item);

  ZeroMemory(item, sizeof(iconList[indx]));
  item->name = null_strdupA(sid->pszName);
  item->name_hash = CalcNameHash(sid->pszName);
  if (utf)
  {
#ifdef _UNICODE
    item->description = null_strdup(sid->ptszDescription);
    item->section = IcoLib_AddSection(sid->pwszSection, TRUE);
#else
    char *pszSection = UnicodeToAnsi(sid->pwszSection);
    
    item->description = UnicodeToAnsi(sid->pwszDescription);
    item->section = IcoLib_AddSection(pszSection, TRUE);
    SAFE_FREE(&pszSection);
#endif
  }
  else
  {
#ifdef _UNICODE
    WCHAR *pwszSection = AnsiToUnicode(sid->pszSection);

    item->description = AnsiToUnicode(sid->pszDescription);
    item->section = IcoLib_AddSection(pwszSection, TRUE);
    SAFE_FREE(&pwszSection);
#else
    item->description = null_strdupA(sid->pszDescription);
    item->section = IcoLib_AddSection(sid->pszSection, TRUE);
#endif
  }
  if (sid->pszDefaultFile)
  {
    char fileFull[MAX_PATH];

    CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)sid->pszDefaultFile, (LPARAM)fileFull);
#ifdef _UNICODE
    item->default_file = AnsiToUnicode(fileFull);
#else
    item->default_file = null_strdupA(fileFull);
#endif
  }
  item->default_indx = sid->iDefaultIndex;
  if (sid->cbSize >= SKINICONDESC_SIZE_V2 && sid->hDefaultIcon)
    item->default_icon = DuplicateIcon(NULL, sid->hDefaultIcon);

  if (sid->cbSize >= SKINICONDESC_SIZE_V3)
  {
    item->cx = sid->cx;
    item->cy = sid->cy;
  }
  else
  {
    item->cx = GetSystemMetrics(SM_CXSMICON);
    item->cy = GetSystemMetrics(SM_CYSMICON);
  }
  if (item->cx == GetSystemMetrics(SM_CXSMICON) && item->cy == GetSystemMetrics(SM_CYSMICON) && item->default_icon)
  {
    item->default_icon_index = ImageList_AddIcon(hDefaultIconList, item->default_icon);
    if (item->default_icon_index != -1)
      SafeDestroyIcon(&item->default_icon);
  }
  else
    item->default_icon_index = -1;

  if (sid->cbSize >= SKINICONDESC_SIZE)
  {
    sectionList[item->section].flags = sid->flags & SIDF_SORTED;
  }

  LeaveCriticalSection(&csIconList);

  return 0;
}


static int IcoLib_RemoveIcon(WPARAM wParam, LPARAM lParam)
{
  if (lParam)
  {
    int indx;
    DWORD dwIconNameHash = CalcNameHash((char*)lParam);

    EnterCriticalSection(&csIconList);

    for (indx = 0; indx < iconCount; indx++)
    {
      IconItem *item = &iconList[indx];

      if (dwIconNameHash == item->name_hash)
      {
        if (!strcmpnullA(item->name, (char*)lParam))
        {
          IcoLib_FreeIcon(item);
          iconCount--;
          memcpy(item, &iconList[indx+1], sizeof(IconItem)*(iconCount-indx));

          return 0; // Success
        }
      }
    }
    LeaveCriticalSection(&csIconList);
  }
  return 1; // Failed
}


HICON IconItem_GetIcon(IconItem *item)
{
  DBVARIANT dbv = {0};
  HICON hIcon = NULL;

  if (item->icon)
    return item->icon;

  if (!DBGetContactSettingTString(NULL, "SkinIcons", item->name, &dbv))
  {
    hIcon = ExtractIconFromPath(dbv.ptszVal, item->cx, item->cy);
    DBFreeVariant(&dbv);
  }

  if (!hIcon)
  {
    hIcon = item->default_icon;

    if (!hIcon && item->default_icon_index != -1)
      hIcon = ImageList_GetIcon(hDefaultIconList, item->default_icon_index, ILD_NORMAL);

    if (!hIcon && item->default_file)
      _ExtractIconEx(item->default_file,item->default_indx,item->cx,item->cy,&hIcon,LR_COLOR);

    if (!hIcon)
      return hIconBlank;
  }
  item->icon = hIcon;
  return hIcon;
}


// lParam: pszIconName
// wParam: PLOADIMAGEPARAM or NULL.
// if wParam == NULL, default is used:
//     uType = IMAGE_ICON
//     cx/cyDesired = GetSystemMetrics(SM_CX/CYSMICON)
//     fuLoad = 0
static int IcoLib_GetIcon(WPARAM wParam, LPARAM lParam)
{
  char* pszIconName = (char *)lParam;
  int result = 0;

  if (pszIconName)
  {
    IconItem *item;
  
    EnterCriticalSection(&csIconList);

    item = IcoLib_FindIcon(pszIconName);
    if (item)
    {
      result = (int)IconItem_GetIcon(item);
      item->ref_count++;
    }
    LeaveCriticalSection(&csIconList);
  }
  else
    result = (int)hIconBlank;

  return result;
}


// lParam: pszIconName or NULL
// wParam: HICON or NULL
static int IcoLib_ReleaseIcon(WPARAM wParam, LPARAM lParam)
{
  IconItem *item = NULL;
  
  EnterCriticalSection(&csIconList);
  
  if (lParam)
    item = IcoLib_FindIcon((char*)lParam);
  if (!item && wParam)
  { // find by HICON
    int indx;
    
    for (indx = 0; indx < iconCount; indx++)
    {
      if (iconList[indx].icon == (HICON)wParam)
      {
        item = &iconList[indx];
        break;
      }
    }
  }
  if (item && item->ref_count)
  {
    item->ref_count--;
    if (!iconEventActive && !item->ref_count && item->icon != item->default_icon)
      SafeDestroyIcon(&item->icon);
  }
  LeaveCriticalSection(&csIconList);
  
  return 0;
}


int InitSkin2Icons(void)
{
  hIconBlank = LoadIcon(NULL, MAKEINTRESOURCE(IDI_BLANK));

  hDefaultIconList = ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32|ILC_MASK,15,15);

  InitializeCriticalSection(&csIconList);
  hIcoLib_AddNewIcon = CreateServiceFunction(MS_SKIN2_ADDICON, IcoLib_AddNewIcon);
  hIcoLib_RemoveIcon = CreateServiceFunction(MS_SKIN2_REMOVEICON, IcoLib_RemoveIcon);
  hIcoLib_GetIcon = CreateServiceFunction(MS_SKIN2_GETICON, IcoLib_GetIcon);
  hIcoLib_ReleaseIcon = CreateServiceFunction(MS_SKIN2_RELEASEICON, IcoLib_ReleaseIcon);

  hIcons2ChangedEvent = CreateHookableEvent(ME_SKIN2_ICONSCHANGED);

  return 0;
}


void UninitSkin2Icons(void)
{
  int indx;
  
  DestroyHookableEvent(hIcons2ChangedEvent);
  
  DestroyServiceFunction(hIcoLib_AddNewIcon);
  DestroyServiceFunction(hIcoLib_RemoveIcon);
  DestroyServiceFunction(hIcoLib_GetIcon);
  DestroyServiceFunction(hIcoLib_ReleaseIcon);
  DeleteCriticalSection(&csIconList);

  for (indx = 0; indx < iconCount; indx++)
    IcoLib_FreeIcon(&iconList[indx]);
  SAFE_FREE(&iconList);

  for (indx = 0; indx < sectionCount; indx++)
    SAFE_FREE(&sectionList[indx].name);
  SAFE_FREE(&sectionList);

  ImageList_Destroy(hDefaultIconList);

  SafeDestroyIcon(&hIconBlank);
}


static void LoadSectionIcons(TCHAR *filename, int sectionActive)
{
  TCHAR path[MAX_PATH];
  int suffIndx;
  HICON hIcon;
  int indx;

  null_snprintf(path, SIZEOF(path), _T("%s,"), filename);
  suffIndx = strlennull(path);

  EnterCriticalSection(&csIconList);

  for (indx = 0; indx < iconCount; indx++)
  {
    IconItem *item = &iconList[indx];

    if (item->default_file && item->section == sectionActive)
    {
      _itot(item->default_indx, path + suffIndx, 10);
      hIcon = ExtractIconFromPath(path, item->cx, item->cy);
      if (hIcon)
      {
        SAFE_FREE(&item->temp_file);
        SafeDestroyIcon(&item->temp_icon);

        item->temp_file = null_strdup(path);
        item->temp_icon = hIcon;
      }
    }
  }
  LeaveCriticalSection(&csIconList);
}


void LoadSubIcons(HWND htv, TCHAR *filename, HTREEITEM hItem)
{
  TVITEM tvi;
  int sectionActive;

  tvi.mask = TVIF_HANDLE|TVIF_PARAM;
  tvi.hItem = hItem;

  TreeView_GetItem(htv, &tvi);
  sectionActive = tvi.lParam;
  if (sectionActive != -1)
    LoadSectionIcons(filename, sectionActive);
  else
  {
    tvi.hItem = TreeView_GetChild(htv, tvi.hItem);
    while (tvi.hItem)
    {
      LoadSubIcons(htv, filename, tvi.hItem);

      tvi.hItem = TreeView_GetNextSibling(htv, tvi.hItem);
    }
  }
}


static void UndoChanges(int iconIndx, int cmd)
{
  IconItem *item = &iconList[iconIndx];

  SAFE_FREE(&item->temp_file);
  SafeDestroyIcon(&item->temp_icon);

  if (cmd == ID_RESET)
  {
    DBDeleteContactSetting(NULL, "SkinIcons", item->name);
    if (item->icon != item->default_icon)
      SafeDestroyIcon(&item->icon);
    else
      item->icon = NULL;
  }
}


void UndoSubItemChanges(HWND htv, HTREEITEM hItem, int cmd)
{
  TVITEM tvi = {0};
  int indx;

  tvi.mask = TVIF_HANDLE|TVIF_PARAM;
  tvi.hItem = hItem;
  TreeView_GetItem(htv, &tvi); // tvi.lParam == active section

  if (tvi.lParam != -1)
  {
    EnterCriticalSection(&csIconList);

    for (indx = 0; indx < iconCount; indx++)
      if (iconList[indx].section == tvi.lParam)
        UndoChanges(indx, cmd);

    LeaveCriticalSection(&csIconList);
  }

  tvi.hItem = TreeView_GetChild(htv, tvi.hItem);
  while (tvi.hItem)
  {
    UndoSubItemChanges(htv, tvi.hItem, cmd);

    tvi.hItem = TreeView_GetNextSibling(htv, tvi.hItem);
  }
}


static void OpenIconsPage()
{
  CallService(MS_UTILS_OPENURL, 1, (LPARAM)"http://addons.miranda-im.org/index.php?action=display&id=35");
}


static int OpenPopupMenu(HWND hwndDlg)
{
  HMENU hMenu, hPopup;
  POINT pt;
  int cmd;

  GetCursorPos(&pt);
  hMenu = LoadMenu(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_ICOLIB_CONTEXT));
  hPopup = GetSubMenu(hMenu, 0);
  CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM)hPopup, 0);
  cmd = TrackPopupMenu(hPopup, TPM_RIGHTBUTTON|TPM_RETURNCMD, pt.x,  pt.y, 0, hwndDlg, NULL);
  DestroyMenu(hMenu);

  return cmd;
}


static TCHAR* OpenFileDlg(HWND hParent, const TCHAR* szFile, BOOL bAll)
{
  OPENFILENAME ofn = {0};
  TCHAR filter[512],*pfilter,file[MAX_PATH*2];

  ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
  ofn.hwndOwner = hParent;

  lstrcpy(filter,TranslateT("Icon Sets"));
  if (bAll)
    lstrcat(filter,_T(" (*.dll;*.icl;*.exe;*.ico)"));
  else
    lstrcat(filter,_T(" (*.dll)"));
  pfilter=filter+lstrlen(filter)+1;
  if (bAll)
    lstrcpy(pfilter,_T("*.DLL;*.ICL;*.EXE;*.ICO"));
  else
    lstrcpy(pfilter,_T("*.DLL"));
  pfilter += lstrlen(pfilter) + 1;
  lstrcpy(pfilter, TranslateT("All Files"));
  lstrcat(pfilter,_T(" (*)"));
  pfilter += lstrlen(pfilter) + 1;
  lstrcpy(pfilter,_T("*"));
  pfilter += lstrlen(pfilter) + 1;
  *pfilter='\0';

  ofn.lpstrFilter = filter;
  ofn.lpstrDefExt = _T("dll");
  lstrcpyn(file, szFile, SIZEOF(file));
  ofn.lpstrFile = file;
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
  ofn.nMaxFile = MAX_PATH*2;

  if (!GetOpenFileName(&ofn)) return NULL;
  
  return null_strdup(file);
}


//
//  User interface
//
#define DM_REBUILDICONSPREVIEW   (WM_USER+10)
#define DM_CHANGEICON            (WM_USER+11)
#define DM_CHANGESPECIFICICON    (WM_USER+12)
#define DM_UPDATEICONSPREVIEW    (WM_USER+13)
#define DM_REBUILD_CTREE         (WM_USER+14)
BOOL CALLBACK DlgProcIconImport(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

void DoOptionsChanged(HWND hwndDlg)
{
  SendMessage(hwndDlg, DM_UPDATEICONSPREVIEW, 0, 0);
  SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
}


void DoIconsChanged(HWND hwndDlg)
{
  int indx;

  SendMessage(hwndDlg, DM_UPDATEICONSPREVIEW, 0, 0);

  iconEventActive = 1; // Disable icon destroying - performance boost
  NotifyEventHooks(hIcons2ChangedEvent, 0, 0);
  iconEventActive = 0;

  EnterCriticalSection(&csIconList); // Destroy unused icons
  for (indx = 0; indx < iconCount; indx++)
  {
    IconItem *item = &iconList[indx];

    if (!item->ref_count && item->icon != item->default_icon)
      SafeDestroyIcon(&item->icon);
  }
  LeaveCriticalSection(&csIconList);
}


static int CALLBACK DoSortIconsFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  return lstrcmpi(iconList[lParam1].description, iconList[lParam2].description);
}


BOOL CALLBACK DlgProcIcoLibOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  struct IcoLibOptsData *dat;
  static HTREEITEM prevItem = 0;
  HWND hPreview = GetDlgItem(hwndDlg, IDC_PREVIEW);

  dat = (struct IcoLibOptsData*)GetWindowLong(hwndDlg, GWL_USERDATA);
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      int indx;
      
      TranslateDialogDefault(hwndDlg);

      dat = (struct IcoLibOptsData*)malloc(sizeof(struct IcoLibOptsData));
      dat->hwndIndex = NULL;
      SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)dat);
      //
      //  Reset temporary data & upload sections list
      //
      EnterCriticalSection(&csIconList);
      for (indx = 0; indx < iconCount; indx++)
      {
        iconList[indx].temp_file = NULL;
        iconList[indx].temp_icon = NULL;
      }
      LeaveCriticalSection(&csIconList);
      //
      //  Setup preview listview
      //
      ListView_SetImageList(hPreview, ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32|ILC_MASK,0,30), LVSIL_NORMAL);
      ListView_SetIconSpacing(hPreview, 56, 67);

      SendMessage(hwndDlg, DM_REBUILD_CTREE, 0, 0);
      return TRUE;
    }

    case DM_REBUILD_CTREE:
    {
      HWND hwndTree = GetDlgItem(hwndDlg, IDC_CATEGORYLIST);
      TVINSERTSTRUCT tvis = {0};
      int indx;
      TCHAR itemName[1024];
      HTREEITEM hSection;

      TreeView_SelectItem(hwndTree, NULL);
      TreeView_DeleteAllItems(hwndTree);

      tvis.hInsertAfter = TVI_LAST;//TVI_SORT;
      tvis.item.mask = TVIF_TEXT|TVIF_STATE|TVIF_PARAM;
      tvis.item.state = tvis.item.stateMask = TVIS_EXPANDED;
      for (indx = 0; indx < sectionCount; indx++)
      {
        TCHAR* sectionName;

        hSection = NULL;
        lstrcpy(itemName, sectionList[indx].name);
        sectionName = itemName;

        while (sectionName)
        { // allow multi-level tree
          TCHAR* pItemName = sectionName;
          HTREEITEM hItem;

          if (sectionName = _tcschr(sectionName, '/'))
          { // one level deeper
            *sectionName = 0;
            sectionName++;
          }

          hItem = FindNamedTreeItemAt(hwndTree, hSection, pItemName);
          if (!sectionName || !hItem)
          {
            if (!hItem)
            {
              tvis.hParent = hSection;
              tvis.item.lParam = sectionName ? -1 : indx;
              tvis.item.pszText = pItemName;
              hItem = TreeView_InsertItem(hwndTree, &tvis);
            }
            else
            {
              TVITEM tvi = {0};

              tvi.hItem = hItem;
              tvi.lParam = indx;
              tvi.mask = TVIF_PARAM | TVIF_HANDLE;
              TreeView_SetItem(hwndTree, &tvi);
            }
          }
          hSection = hItem;
        }
      }
      ShowWindow(hwndTree, SW_SHOW);

      TreeView_SelectItem(hwndTree, FindNamedTreeItemAt(hwndTree, 0, NULL));
      //SendMessage(hwndDlg, DM_REBUILDICONSPREVIEW, 0, 0);
      
      break;
    }
    //
    //  Rebuild preview to new section
    //
    case DM_REBUILDICONSPREVIEW:
    {  
      //TVITEM tvi;
      LVITEM lvi = {0};
      HIMAGELIST hIml;
      HICON hIcon;
      int sectionActive;
      int indx;

      //MySetCursor(IDC_WAIT);

      sectionActive = lParam;
      EnableWindow(hPreview, sectionActive != -1);

      ListView_DeleteAllItems(hPreview);
      hIml = ListView_GetImageList(hPreview, LVSIL_NORMAL);
      ImageList_RemoveAll(hIml);

      if (sectionActive == -1)
        break;

      lvi.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;

      EnterCriticalSection(&csIconList);

      for (indx = 0; indx < iconCount; indx++)
      {
        IconItem *item = &iconList[indx];

        if (item->section == sectionActive)
        {
          lvi.pszText = TranslateTS(item->description);
          hIcon = item->temp_icon;
          if (!hIcon)
          {
            if (!item->icon) item->temp = 1;
            hIcon = IconItem_GetIcon(item);
          }
          lvi.iImage = ImageList_AddIcon(hIml, hIcon);
          lvi.lParam = indx;
          ListView_InsertItem(hPreview, &lvi);
        }
      }
      LeaveCriticalSection(&csIconList);

      if (sectionList[sectionActive].flags & SIDF_SORTED)
        ListView_SortItems(hPreview, DoSortIconsFunc, 0);

      //MySetCursor(IDC_ARROW);
      break;
    }
    //
    //  Refresh preview to new section
    //
    case DM_UPDATEICONSPREVIEW:
    {  
      LVITEM lvi = {0};
      HIMAGELIST hIml;
      HICON hIcon;
      int indx, count;

      //MySetCursor(IDC_WAIT);

      hIml = ListView_GetImageList(hPreview, LVSIL_NORMAL);

      lvi.mask = LVIF_IMAGE|LVIF_PARAM;
      count = ListView_GetItemCount(hPreview);

      for (indx = 0; indx < count; indx++)
      {
        lvi.iItem = indx;
        ListView_GetItem(hPreview, &lvi);
        EnterCriticalSection(&csIconList);
        hIcon = iconList[lvi.lParam].temp_icon;
        if (!hIcon)
          hIcon = IconItem_GetIcon(&iconList[lvi.lParam]);
        LeaveCriticalSection(&csIconList);

        if (hIcon)
          ImageList_ReplaceIcon(hIml, lvi.iImage, hIcon);
      }
      ListView_RedrawItems(hPreview, 0, count);
      //MySetCursor(IDC_ARROW);
      break;
    }
    //
    //  Temporary change icon - only inside options dialog
    //
    case DM_CHANGEICON:
    {
      TCHAR *path=(TCHAR*)lParam;
      LVITEM lvi = {0};
      IconItem *item;

      lvi.mask = LVIF_PARAM;
      lvi.iItem = wParam;
      ListView_GetItem(hPreview, &lvi);

      EnterCriticalSection(&csIconList);
      item = &iconList[lvi.lParam];

      SAFE_FREE(&item->temp_file);
      SafeDestroyIcon(&item->temp_icon);
      item->temp_file = null_strdup(path);
      item->temp_icon = (HICON)ExtractIconFromPath(path, item->cx, item->cy);

      LeaveCriticalSection(&csIconList);

      DoOptionsChanged(hwndDlg);
      //SendMessage(hwndDlg,DM_REBUILDICONSPREVIEW,0,0);
      break;    
    }
    case WM_COMMAND:
      if (LOWORD(wParam) == IDC_IMPORT)
      {
        dat->hwndIndex = CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ICOLIB_IMPORT), GetParent(hwndDlg), DlgProcIconImport, (LPARAM)hwndDlg);
        EnableWindow((HWND)lParam, FALSE);
      }
      else if (LOWORD(wParam) == IDC_GETMORE)
      {
        OpenIconsPage();
        break;
      }
      else if (LOWORD(wParam) == IDC_LOADICONS)
      {
        TCHAR filetmp[1] = {0};
        TCHAR *file;

        if (file = OpenFileDlg(hwndDlg, filetmp, FALSE))
        {
          HWND htv = GetDlgItem(hwndDlg, IDC_CATEGORYLIST);
          TCHAR filename[MAX_PATH];

          CallService(MS_UTILS_PATHTORELATIVET, (WPARAM)file, (LPARAM)filename);
          SAFE_FREE(&file);

          MySetCursor(IDC_WAIT);

          LoadSubIcons(htv, filename, TreeView_GetSelection(htv));

          MySetCursor(IDC_ARROW);

          DoOptionsChanged(hwndDlg);
        }
      }
      break;

    case WM_CONTEXTMENU:
    {
      HWND htv = GetDlgItem(hwndDlg, IDC_CATEGORYLIST);
      //LVHITTESTINFO lvhti;
      //GetCursorPos(&lvhti.pt);
      //ScreenToClient(GetDlgItem(hwndDlg,IDC_PREVIEW),&lvhti.pt);
      //if(ListView_HitTest(GetDlgItem(hwndDlg,IDC_PREVIEW),&lvhti) != -1)
      if ((HWND)wParam == hPreview)
      {
        UINT count = ListView_GetSelectedCount(hPreview);

        if (count > 0)
        {
          int cmd = OpenPopupMenu(hwndDlg);

          switch(cmd)
          {
            case ID_CANCELCHANGE:
            case ID_RESET:
            {
              LVITEM lvi = {0};
              int itemIndx = -1;

              while ((itemIndx = ListView_GetNextItem(hPreview, itemIndx, LVNI_SELECTED)) != -1)
              {
                lvi.mask = LVIF_PARAM;
                lvi.iItem = itemIndx; //lvhti.iItem;
                ListView_GetItem(hPreview, &lvi);

                UndoChanges(lvi.lParam, cmd);
              }

              if (cmd == ID_RESET)
                DoIconsChanged(hwndDlg);
              else
                DoOptionsChanged(hwndDlg);

              break;
            }
          }
        }
      }
      else if ((HWND)wParam == htv)
      {
        int cmd = OpenPopupMenu(hwndDlg);

        switch(cmd)
        {
          case ID_CANCELCHANGE:
          case ID_RESET:
          {
            UndoSubItemChanges(htv, TreeView_GetSelection(htv), cmd);

            if (cmd == ID_RESET)
              DoIconsChanged(hwndDlg);
            else
              DoOptionsChanged(hwndDlg);

            break;
          }
        }
      }
    }
    break;
    case WM_NOTIFY:
      switch(((LPNMHDR)lParam)->idFrom)
      {
        case 0:
          switch(((LPNMHDR)lParam)->code)
          {
            case PSN_APPLY:
            {
              int indx;

              EnterCriticalSection(&csIconList);

              for (indx = 0; indx < iconCount; indx++)
              {
                IconItem *item = &iconList[indx];
                if (item->temp_file)
                {
                  DBWriteContactSettingTString(NULL, "SkinIcons", item->name, item->temp_file);
                  if (item->temp_icon)
                  {
                    if (item->icon != item->default_icon)
                      SafeDestroyIcon(&item->icon);
                    item->icon = item->temp_icon;
                    item->temp_icon = NULL;
                  }
                }
                item->ref_count = 0;
              }
              LeaveCriticalSection(&csIconList);

              DoIconsChanged(hwndDlg);

              return TRUE;
            }
            break;
          }
          break;
        case IDC_CATEGORYLIST:
          switch(((NMHDR*)lParam)->code)
          {
            case TVN_SELCHANGEDA: // !!!! This needs to be here - both !!
            case TVN_SELCHANGEDW:
            {
              NMTREEVIEW *pnmtv = (NMTREEVIEW*)lParam;
              TVITEM tvi = pnmtv->itemNew;

              SendMessage(hwndDlg, DM_REBUILDICONSPREVIEW, 0, tvi.lParam);

              break;
            }
          }
          break;
      }
      break;

    case WM_DESTROY:
    {
      int indx;

      DestroyWindow(dat->hwndIndex);

      EnterCriticalSection(&csIconList);
      for (indx = 0; indx < iconCount; indx++)
      {
        IconItem *item = &iconList[indx];

        SAFE_FREE(&item->temp_file);
        SafeDestroyIcon(&item->temp_icon);
        if (item->temp && !item->ref_count)
        {
          if (item->icon!=item->default_icon)
            SafeDestroyIcon(&item->icon);
        }
        item->temp = 0;
      }
      LeaveCriticalSection(&csIconList);

      SAFE_FREE(&dat);
      break;
    }
  }
  return FALSE;
}


static int IconDlg_Resize(HWND hwndDlg,LPARAM lParam, UTILRESIZECONTROL *urc)
{
  switch (urc->wId)
  {
    case IDC_ICONSET:
      return RD_ANCHORX_WIDTH | RD_ANCHORY_TOP;
      break;
    case IDC_BROWSE:
      return RD_ANCHORX_RIGHT | RD_ANCHORY_TOP;
      break;
    case IDC_PREVIEW:
      return RD_ANCHORX_WIDTH | RD_ANCHORY_HEIGHT;      
      break;
    case IDC_GETMORE:
      return RD_ANCHORX_CENTRE | RD_ANCHORY_BOTTOM;
  }
  return RD_ANCHORX_LEFT | RD_ANCHORY_TOP; // default
}


BOOL CALLBACK DlgProcIconImport(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static HWND hwndParent,hwndDragOver;
  static int dragging;
  static int dragItem,dropHiLite;

  HWND hPreview = GetDlgItem(hwndDlg, IDC_PREVIEW);

  switch (msg)
  {
    case WM_INITDIALOG:
    {
      hwndParent = (HWND)lParam;
      dragging = dragItem = 0;
      TranslateDialogDefault(hwndDlg);
      ListView_SetImageList(hPreview, ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32|ILC_MASK,0,100), LVSIL_NORMAL);
      ListView_SetIconSpacing(hPreview, 56, 67);
      {  
        RECT rcThis, rcParent;
        int cxScreen = GetSystemMetrics(SM_CXSCREEN);

        GetWindowRect(hwndDlg,&rcThis);
        GetWindowRect(hwndParent,&rcParent);
        OffsetRect(&rcThis,rcParent.right-rcThis.left,0);
        OffsetRect(&rcThis,0,rcParent.top-rcThis.top);
        GetWindowRect(GetParent(hwndParent),&rcParent);
        if (rcThis.right > cxScreen)
        {
          OffsetRect(&rcParent,cxScreen-rcThis.right,0);
          OffsetRect(&rcThis,cxScreen-rcThis.right,0);
          MoveWindow(GetParent(hwndParent),rcParent.left,rcParent.top,rcParent.right-rcParent.left,rcParent.bottom-rcParent.top,TRUE);
        }
        MoveWindow(hwndDlg,rcThis.left,rcThis.top,rcThis.right-rcThis.left,rcThis.bottom-rcThis.top,FALSE);
        GetClientRect(hwndDlg, &rcThis);
        SendMessage(hwndDlg, WM_SIZE, 0, MAKELPARAM(rcThis.right-rcThis.left, rcThis.bottom-rcThis.top));
      }
      {  
        HRESULT (STDAPICALLTYPE *MySHAutoComplete)(HWND,DWORD);

        MySHAutoComplete = (HRESULT (STDAPICALLTYPE*)(HWND,DWORD))GetProcAddress(GetModuleHandleA("shlwapi"),"SHAutoComplete");
        if (MySHAutoComplete) MySHAutoComplete(GetDlgItem(hwndDlg,IDC_ICONSET), 1);
      }
      SetDlgItemText(hwndDlg,IDC_ICONSET,_T("icons.dll"));

      return TRUE;
    }

    case DM_REBUILDICONSPREVIEW:
    {  
      LVITEM lvi;
      TCHAR filename[MAX_PATH],caption[64];
      HIMAGELIST hIml;
      int count,i;
      HICON hIcon;

      MySetCursor(IDC_WAIT);
      ListView_DeleteAllItems(hPreview);
      hIml = ListView_GetImageList(hPreview, LVSIL_NORMAL);
      ImageList_RemoveAll(hIml);
      GetDlgItemText(hwndDlg, IDC_ICONSET, filename, SIZEOF(filename));
      {  
        RECT rcPreview,rcGroup;

        GetWindowRect(hPreview, &rcPreview);
        GetWindowRect(GetDlgItem(hwndDlg,IDC_IMPORTMULTI),&rcGroup);
        //SetWindowPos(hPreview,0,0,0,rcPreview.right-rcPreview.left,rcGroup.bottom-rcPreview.top,SWP_NOZORDER|SWP_NOMOVE);
      }

      if (_taccess(filename,0) != 0)
      {
        MySetCursor(IDC_ARROW);
        break;
      }

      lvi.mask = LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM;
      lvi.iSubItem = 0;
      lvi.iItem = 0;
      count = (int)ExtractIcon(GetModuleHandle(NULL), filename, -1);
      for (i = 0; i < count; lvi.iItem++, i++)
      {
        null_snprintf(caption, 64, _T("%d"), i+1);
        lvi.pszText = caption;
        hIcon = ExtractIcon(GetModuleHandle(NULL), filename, i);
        lvi.iImage = ImageList_AddIcon(hIml, hIcon);
        DestroyIcon(hIcon);
        lvi.lParam = i;
        ListView_InsertItem(hPreview, &lvi);
      }
      MySetCursor(IDC_ARROW);
      break;
    }

    case WM_COMMAND:
      switch(LOWORD(wParam))
      {
        case IDC_BROWSE:
        { 
          TCHAR str[MAX_PATH];
          TCHAR *file;

          GetDlgItemText(hwndDlg,IDC_ICONSET,str,sizeof(str));
          if (!(file = OpenFileDlg(GetParent(hwndDlg), str, TRUE))) break;
          SetDlgItemText(hwndDlg,IDC_ICONSET,file);
          SAFE_FREE(&file);
          break;
        }
        case IDC_GETMORE:
        { 
          OpenIconsPage();
          break;
        }
        case IDC_ICONSET:
        {
          if (HIWORD(wParam) == EN_CHANGE)
            SendMessage(hwndDlg, DM_REBUILDICONSPREVIEW, 0, 0);
          break;
        }
      }
      break;

    case WM_MOUSEMOVE:
      if (dragging)
      {
        LVHITTESTINFO lvhti;
        int onItem=0;
        HWND hwndOver;
        RECT rc;
        POINT ptDrag;
        HWND hPPreview = GetDlgItem(hwndParent, IDC_PREVIEW);

        lvhti.pt.x = (short)LOWORD(lParam); lvhti.pt.y = (short)HIWORD(lParam);
        ClientToScreen(hwndDlg, &lvhti.pt);
        hwndOver = WindowFromPoint(lvhti.pt);
        GetWindowRect(hwndOver, &rc);
        ptDrag.x = lvhti.pt.x - rc.left; ptDrag.y = lvhti.pt.y - rc.top;
        if (hwndOver != hwndDragOver)
        {
          ImageList_DragLeave(hwndDragOver);
          hwndDragOver = hwndOver;
          ImageList_DragEnter(hwndDragOver, ptDrag.x, ptDrag.y);
        }
        ImageList_DragMove(ptDrag.x, ptDrag.y);
        if (hwndOver == hPPreview)
        {
          ScreenToClient(hPPreview, &lvhti.pt);

          if (ListView_HitTest(hPPreview, &lvhti) != -1)
          {
            if (lvhti.iItem != dropHiLite)
            {
              ImageList_DragLeave(hwndDragOver);
              if (dropHiLite != -1)
                ListView_SetItemState(hPPreview, dropHiLite, 0, LVIS_DROPHILITED);
              dropHiLite = lvhti.iItem;
              ListView_SetItemState(hPPreview, dropHiLite, LVIS_DROPHILITED, LVIS_DROPHILITED);
              UpdateWindow(hPPreview);
              ImageList_DragEnter(hwndDragOver, ptDrag.x, ptDrag.y);
            }
            onItem = 1;
          }
        }
        if (!onItem && dropHiLite != -1)
        {
          ImageList_DragLeave(hwndDragOver);
          ListView_SetItemState(hPPreview, dropHiLite, 0, LVIS_DROPHILITED);
          UpdateWindow(hPPreview);
          ImageList_DragEnter(hwndDragOver, ptDrag.x, ptDrag.y);
          dropHiLite = -1;
        }
        MySetCursor(onItem ? IDC_ARROW : IDC_NO);
      }
      break;

    case WM_LBUTTONUP:
      if(dragging)
      {
        ReleaseCapture();
        ImageList_EndDrag();
        dragging = 0;
        if (dropHiLite != -1)
        {
          TCHAR path[MAX_PATH],fullPath[MAX_PATH],filename[MAX_PATH];
          LVITEM lvi;

          GetDlgItemText(hwndDlg, IDC_ICONSET, fullPath, sizeof(fullPath));
          CallService(MS_UTILS_PATHTORELATIVET, (WPARAM)fullPath, (LPARAM)filename);
          lvi.mask=LVIF_PARAM;
          lvi.iItem = dragItem; lvi.iSubItem = 0;
          ListView_GetItem(hPreview, &lvi);
          null_snprintf(path, MAX_PATH, _T("%s,%d"), filename, (int)lvi.lParam);
          SendMessage(hwndParent, DM_CHANGEICON, dropHiLite, (LPARAM)path);
          ListView_SetItemState(GetDlgItem(hwndParent, IDC_PREVIEW), dropHiLite, 0, LVIS_DROPHILITED);
        }
      }
      break;

    case WM_NOTIFY:
      switch (((LPNMHDR)lParam)->idFrom) 
      {
        case IDC_PREVIEW:
          switch (((LPNMHDR)lParam)->code) 
          {
            case LVN_BEGINDRAG:
              SetCapture(hwndDlg);
              dragging = 1;
              dragItem = ((LPNMLISTVIEW)lParam)->iItem;
              dropHiLite = -1;
              ImageList_BeginDrag(ListView_GetImageList(hPreview, LVSIL_NORMAL), dragItem, GetSystemMetrics(SM_CXICON)/2, GetSystemMetrics(SM_CYICON)/2);
              {  
                POINT pt;
                RECT rc;

                GetCursorPos(&pt);
                GetWindowRect(hwndDlg, &rc);
                ImageList_DragEnter(hwndDlg, pt.x - rc.left, pt.y - rc.top);
                hwndDragOver = hwndDlg;
              }
              break;
          }
          break;
      }
      break;

    case WM_CLOSE:
      DestroyWindow(hwndDlg);
      EnableWindow(GetDlgItem(hwndParent,IDC_IMPORT),TRUE);
      break;

    case WM_SIZE:
    { // make the dlg resizeable
      UTILRESIZEDIALOG urd = {0};

      if (IsIconic(hwndDlg)) break;
      urd.cbSize = sizeof(urd);
      urd.hInstance = GetModuleHandle(NULL);
      urd.hwndDlg = hwndDlg;
      urd.lParam = 0; // user-defined
      urd.lpTemplate = MAKEINTRESOURCEA(IDD_ICOLIB_IMPORT);
      urd.pfnResizer = IconDlg_Resize;
      CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM)&urd);
      break;
    }
  }
  return FALSE;
}


/* Utility functions */

static void __fastcall MySetCursor(TCHAR* nCursor)
{
  SetCursor(LoadCursor(NULL, nCursor));
}



static void __fastcall SAFE_FREE(void** p)
{
  if (*p)
  {
    free(*p);
    *p = NULL;
  }
}


static void __fastcall SafeDestroyIcon(HICON* icon)
{
  if (*icon)
  {
    DestroyIcon(*icon);
    *icon = NULL;
  }
}


/* a strlen() that likes NULL */
static size_t __fastcall strlennull(const TCHAR *string)
{
  if (string)
    return lstrlen(string);

  return 0;
}

/* a strlen() that likes NULL */
static size_t __fastcall strlennullW(const WCHAR *string)
{
  if (string)
    return wcslen(string);

  return 0;
}

/* a strlen() that likes NULL */
static size_t __fastcall strlennullA(const char *string)
{
  if (string)
    return lstrlenA(string);

  return 0;
}

/* a strcmp() that likes NULL */
static int __fastcall strcmpnull(const TCHAR *str1, const TCHAR *str2)
{
  if (str1 && str2)
    return lstrcmp(str1, str2);

  return 1;
}

static int __fastcall strcmpnullA(const char *str1, const char *str2)
{
  if (str1 && str2)
    return strcmp(str1, str2);

  return 1;
}

static int null_snprintf(TCHAR *buffer, size_t count, const TCHAR* fmt, ...)
{
  va_list va;
  int len;

  ZeroMemory(buffer, count);
  va_start(va, fmt);
  len = _vsntprintf(buffer, count-1, fmt, va);
  va_end(va);
  return len;
}

static TCHAR* __fastcall null_strdup(const TCHAR *string)
{
  if (string)
    return _tcsdup(string);

  return NULL;
}

static char* __fastcall null_strdupA(const char *string)
{
  if (string)
    return strdup(string);

  return NULL;
}

static UINT GetLangCodepage()
{
  return CallService(MS_LANGPACK_GETCODEPAGE, 0, 0);
}

static wchar_t* __fastcall AnsiToUnicode(const char* src)
{
  int wchars;
  wchar_t* szRes;
  UINT cp;

  if (!strlennullA(src)) return NULL;

  cp = GetLangCodepage();
  wchars = MultiByteToWideChar(cp, MB_PRECOMPOSED, src, -1, NULL, 0);
  szRes = calloc(wchars + 1, sizeof(unsigned short));
  MultiByteToWideChar(cp, MB_PRECOMPOSED, src, -1, szRes, wchars);

  return szRes;
}

static wchar_t* __fastcall AnsiListToUnicode(const char* srclist)
{
  wchar_t *res = NULL;
  int reslen = 0;
  char *work = (char*)srclist;

  while (*work)
  {
    wchar_t *tmp = AnsiToUnicode(work);
    int newlen;

    work += strlennullA(work) + 1;
    newlen = reslen + (wcslen(tmp) + 1)*sizeof(wchar_t);
    res = (wchar_t*)realloc(res, newlen);
    memcpy((char*)res + reslen, tmp, newlen - reslen);
    reslen = newlen;
    SAFE_FREE(&tmp);
  }
  res = (wchar_t*)realloc(res, reslen + 2);
  res[reslen/2] = '\0';
  return res;
}

static char* __fastcall UnicodeToAnsi(const wchar_t* src)
{
  int cbLen;
  char* szRes;
  UINT cp;

  if (!strlennullW(src)) return NULL;

  cp = GetLangCodepage();
	cbLen = WideCharToMultiByte(cp, 0, src, -1, NULL, 0, NULL, NULL);
	szRes = (char*)calloc(cbLen + 1, sizeof(char));
	WideCharToMultiByte(cp, 0, src, -1, szRes, cbLen, NULL, NULL);

  return szRes;
}


#if __GNUC__
#define NOINLINEASM
#endif

static DWORD CalcNameHashW(const wchar_t *szStr)
{
  if (*szStr == 0) return 0;
#if defined _M_IX86 && !defined _NUMEGA_BC_FINALCHECK && !defined NOINLINEASM
  __asm {       //this breaks if szStr is empty
    xor  edx,edx
    xor  eax,eax
    mov  esi,szStr
    mov  ax,[esi]
    xor  cl,cl
  lph_top:   //only 4 of 9 instructions in here don't use AL, so optimal pipe use is impossible
    xor  edx,eax
    inc  esi
    inc  esi
    xor  eax,eax
    and  cl,31
    mov  ax,[esi]
    add  cl,5
    test ax,ax
    rol  eax,cl     //rol is u-pipe only, but pairable
                    //rol doesn't touch z-flag
    jnz  lph_top  //5 clock tick loop. not bad.

    xor  eax,edx
  }
#else
  DWORD hash=0;
  int i;
  int shift=0;
  for(i=0;szStr[i];i++) {
    hash^=szStr[i]<<shift;
    if(shift>24) hash^=(szStr[i]>>(32-shift))&0x7F;
    shift=(shift+5)&0x1F;
  }
  return hash;
#endif
}

static DWORD CalcNameHash(const char *szStr)
{
  if (*szStr == 0) return 0;
#if defined _M_IX86 && !defined _NUMEGA_BC_FINALCHECK && !defined NOINLINEASM
  __asm {       //this breaks if szStr is empty
    xor  edx,edx
    xor  eax,eax
    mov  esi,szStr
    mov  al,[esi]
    xor  cl,cl
  lph_top:   //only 4 of 9 instructions in here don't use AL, so optimal pipe use is impossible
    xor  edx,eax
    inc  esi
    xor  eax,eax
    and  cl,31
    mov  al,[esi]
    add  cl,5
    test al,al
    rol  eax,cl     //rol is u-pipe only, but pairable
                     //rol doesn't touch z-flag
    jnz  lph_top  //5 clock tick loop. not bad.

    xor  eax,edx
  }
#else
  DWORD hash=0;
  int i;
  int shift=0;
  for(i=0;szStr[i];i++) {
    hash^=szStr[i]<<shift;
    if(shift>24) hash^=(szStr[i]>>(32-shift))&0x7F;
    shift=(shift+5)&0x1F;
  }
  return hash;
#endif
}

static DWORD CalcNameHashT(const TCHAR *szStr)
{
#ifdef _UNICODE
  return CalcNameHashW(szStr);
#else
  return CalcNameHash(szStr);
#endif
}
