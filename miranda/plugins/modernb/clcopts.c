/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2006 Miranda ICQ/IM project, 
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
#include "m_clc.h"
#include "clc.h"
#include "commonprototypes.h"


#define DBFONTF_BOLD       1
#define DBFONTF_ITALIC     2
#define DBFONTF_UNDERLINE  4

#define SAMEASF_FACE   1
#define SAMEASF_SIZE   2
#define SAMEASF_STYLE  4
#define SAMEASF_COLOUR 8
#define SAMEASF_EFFECT 128

static WORD fontSameAsDefault[]={0x00FF,0x8B00,0x8F00,0x8700,0x8B00,0x8104,0x8D00,0x8B02,0x8900,0x8D00,
0x8F00,0x8F00,0x8F00,0x8F00,0x8F00,0x8F00,0x8F00,0x8F00,0x8F00,0x8104,0x8D00,0x00FF,0x8F00};
static TCHAR *fontSizes[]={_T("6"),_T("7"),_T("8"),_T("9"),_T("10"),_T("12"),_T("14"),_T("16"),_T("18"),_T("20"),_T("24"),_T("28")};
static int fontListOrder[]={FONTID_CONTACTS,FONTID_INVIS,FONTID_OFFLINE,FONTID_OFFINVIS,FONTID_NOTONLIST,FONTID_OPENGROUPS,FONTID_OPENGROUPCOUNTS,FONTID_CLOSEDGROUPS,FONTID_CLOSEDGROUPCOUNTS,FONTID_DIVIDERS,FONTID_SECONDLINE,FONTID_THIRDLINE,
FONTID_AWAY,FONTID_DND,FONTID_NA,FONTID_OCCUPIED,FONTID_CHAT,FONTID_INVISIBLE,FONTID_PHONE,FONTID_LUNCH,FONTID_CONTACT_TIME,FONTID_STATUSBAR_PROTONAME,FONTID_EVENTAREA};

static BOOL CALLBACK DlgProcClcMainOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcClcMetaOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcClcBkgOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcClcTextOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcStatusBarBkgOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
//extern void OnStatusBarBackgroundChange();
extern int CLUIServices_ProtocolStatusChanged(WPARAM,LPARAM);
//extern int ImageList_AddIcon_FixAlpha(HIMAGELIST,HICON);

DWORD GetDefaultExStyle(void)
{
  BOOL param;
  DWORD ret=CLCDEFAULT_EXSTYLE;
  if(SystemParametersInfo(SPI_GETLISTBOXSMOOTHSCROLLING,0,&param,FALSE) && !param)
    ret|=CLS_EX_NOSMOOTHSCROLLING;
  if(SystemParametersInfo(SPI_GETHOTTRACKING,0,&param,FALSE) && !param)
    ret&=~CLS_EX_TRACKSELECT;
  return ret;
}

static void GetDefaultFontSetting(int i,LOGFONTA *lf,COLORREF *colour)
{
  SystemParametersInfoA(SPI_GETICONTITLELOGFONT,sizeof(LOGFONTA),lf,FALSE);
  *colour=GetSysColor(COLOR_WINDOWTEXT);
  lf->lfHeight = 8;
  switch(i) {
case FONTID_OPENGROUPS:
case FONTID_CLOSEDGROUPS:
  lf->lfWeight=FW_BOLD;
  break;
case FONTID_OPENGROUPCOUNTS:
case FONTID_CLOSEDGROUPCOUNTS:
  lf->lfHeight=(int)(lf->lfHeight*.875);
  *colour=GetSysColor(COLOR_3DSHADOW);
  break;
case FONTID_OFFINVIS:
case FONTID_INVIS:
  lf->lfItalic=!lf->lfItalic;
  break;
case FONTID_DIVIDERS:
  lf->lfHeight=(int)(lf->lfHeight*.875);
  break;
case FONTID_NOTONLIST:
  *colour=GetSysColor(COLOR_3DSHADOW);
  break;
case FONTID_SECONDLINE:
  lf->lfItalic=!lf->lfItalic;
  lf->lfHeight=(int)(lf->lfHeight*.875);
  *colour=GetSysColor(COLOR_3DSHADOW);
  break;
case FONTID_THIRDLINE:
  lf->lfHeight=(int)(lf->lfHeight*.875);
  *colour=GetSysColor(COLOR_3DSHADOW);
  break;
  }
}
int SameAsAntiCycle=0;
void CheckSameAs(int fontId,LOGFONTA *dest_lf,COLORREF *dest_colour,BYTE *effect, COLORREF *eColour1,COLORREF *eColour2)
{
  char idstr[32];
  WORD sameAsComp;
  BYTE sameAsFlags,sameAs;

  if (SameAsAntiCycle>5)
    return;
  SameAsAntiCycle++;
  mir_snprintf(idstr,sizeof(idstr),"Font%dAs",fontId);
  sameAsComp=DBGetContactSettingWord(NULL,"CLC",idstr,fontSameAsDefault[fontId]);
  sameAsFlags=HIBYTE(sameAsComp);
  sameAs=LOBYTE(sameAsComp);
  if (sameAsFlags && sameAs!=255)
  {
    LOGFONTA lf2={0};
    COLORREF color2=0;
    BYTE ef=0;
    COLORREF c1=0, c2=0;
    //Get font setting for SameAs font
    GetFontSetting(sameAs,&lf2,&color2,&ef,&c1,&c2);
    //copy same as data:
    if(sameAsFlags&SAMEASF_FACE)
      strncpy(dest_lf->lfFaceName,lf2.lfFaceName,sizeof(dest_lf->lfFaceName));
    if(sameAsFlags&SAMEASF_SIZE)
      dest_lf->lfHeight=lf2.lfHeight;
    if(sameAsFlags&SAMEASF_STYLE)
    {
      dest_lf->lfWeight=lf2.lfWeight;
      dest_lf->lfItalic=lf2.lfItalic;
      dest_lf->lfUnderline=lf2.lfUnderline;
    }	
    if(sameAsFlags&SAMEASF_COLOUR)
    {
      if (dest_colour)
        *dest_colour=color2;
    }
    if(sameAsFlags&SAMEASF_EFFECT)
    {
      if (effect)
      {
        *effect=ef;
        *eColour1=c1;
        *eColour2=c2;
      }
    }
    
  }
  SameAsAntiCycle--;
}

void GetFontSetting(int i,LOGFONTA *lf,COLORREF *colour,BYTE *effect, COLORREF *eColour1,COLORREF *eColour2)
{
  DBVARIANT dbv={0};
  char idstr[32];
  BYTE style;

  GetDefaultFontSetting(i,lf,colour);
  mir_snprintf(idstr,sizeof(idstr),"Font%dName",i);
  if(!DBGetContactSetting(NULL,"CLC",idstr,&dbv)) {
    strcpy(lf->lfFaceName,dbv.pszVal);
    //mir_free(dbv.pszVal);
	DBFreeVariant(&dbv);
  }
  mir_snprintf(idstr,sizeof(idstr),"Font%dCol",i);
  *colour=DBGetContactSettingDword(NULL,"CLC",idstr,*colour);
  if (effect)
  {
    mir_snprintf(idstr,sizeof(idstr),"Font%dEffect",i);
    *effect=DBGetContactSettingByte(NULL,"CLC",idstr,0);
    mir_snprintf(idstr,sizeof(idstr),"Font%dEffectCol1",i);
    *eColour1=DBGetContactSettingDword(NULL,"CLC",idstr,0);
    mir_snprintf(idstr,sizeof(idstr),"Font%dEffectCol2",i);
    *eColour2=DBGetContactSettingDword(NULL,"CLC",idstr,0);
  }
  mir_snprintf(idstr,sizeof(idstr),"Font%dSize",i);
  lf->lfHeight=(signed short)DBGetContactSettingByte(NULL,"CLC",idstr,lf->lfHeight);
  mir_snprintf(idstr,sizeof(idstr),"Font%dSty",i);
  style=(BYTE)DBGetContactSettingByte(NULL,"CLC",idstr,(lf->lfWeight==FW_NORMAL?0:DBFONTF_BOLD)|(lf->lfItalic?DBFONTF_ITALIC:0)|(lf->lfUnderline?DBFONTF_UNDERLINE:0));
  lf->lfWidth=lf->lfEscapement=lf->lfOrientation=0;
  lf->lfWeight=style&DBFONTF_BOLD?FW_BOLD:FW_NORMAL;
  lf->lfItalic=(style&DBFONTF_ITALIC)!=0;
  lf->lfUnderline=(style&DBFONTF_UNDERLINE)!=0;
  lf->lfStrikeOut=0;
  mir_snprintf(idstr,sizeof(idstr),"Font%dSet",i);
  lf->lfCharSet=DBGetContactSettingByte(NULL,"CLC",idstr,lf->lfCharSet);
  lf->lfOutPrecision=OUT_DEFAULT_PRECIS;
  lf->lfClipPrecision=CLIP_DEFAULT_PRECIS;
  lf->lfQuality=DEFAULT_QUALITY;
  lf->lfPitchAndFamily=DEFAULT_PITCH|FF_DONTCARE; 
  CheckSameAs(i,lf,colour,effect, eColour1,eColour2);
}


int BgMenuChange(WPARAM wParam,LPARAM lParam)
{
  if (MirandaExiting()) return 0;
  ClcOptionsChanged();
  return 0;
}

int BgClcChange(WPARAM wParam,LPARAM lParam)
{
  // ClcOptionsChanged();
  return 0;
}

int BgStatusBarChange(WPARAM wParam,LPARAM lParam)
{
  if (MirandaExiting()) return 0;
  ClcOptionsChanged();
  //OnStatusBarBackgroundChange();
  return 0;
}

BOOL CALLBACK DlgProcGenOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcCluiOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcCluiOpts2(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcSBarOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ProtocolOrderOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);


static TabItemOptionConf clist_opt_items[] = { 
	{ _T("General"), IDD_OPT_CLIST, DlgProcGenOpts},
	{ _T("List"), IDD_OPT_CLC, DlgProcClcMainOpts },
	{ _T("Window"), IDD_OPT_CLUI, DlgProcCluiOpts },
	{ _T("Behaviour"), IDD_OPT_CLUI_2, DlgProcCluiOpts2 },
	{ _T("Status Bar"), IDD_OPT_SBAR, DlgProcSBarOpts},	
	{ _T("Additional stuffs"), IDD_OPT_META_CLC, DlgProcClcMetaOpts }
};
BOOL CALLBACK DlgProcClcTabbedOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int res=0;
	if (msg==WM_INITDIALOG)
	{
      wParam=SIZEOF(clist_opt_items);
      lParam=(LPARAM)(void**)(&clist_opt_items);
	}
	res=DlgProcTabbedOpts(hwndDlg,msg,wParam,lParam);
	return res;
}

int ClcOptInit(WPARAM wParam,LPARAM lParam)
{
   OPTIONSDIALOGPAGE odp;
   if (MirandaExiting()) return 0;
   ZeroMemory(&odp,sizeof(odp));
   odp.cbSize=sizeof(odp);
   odp.position=0;
   odp.hInstance=g_hInst;
   odp.pszGroup="";//Translate("Contact List");
   odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_CLC);
   odp.pszTitle=Translate("List");
   odp.pfnDlgProc=DlgProcClcMainOpts;
   odp.flags=ODPF_BOLDGROUPS|ODPF_EXPERTONLY;
   //CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
   CreateTabPage(NULL,Translate("Contact List"), wParam, DlgProcClcTabbedOpts);
 
   odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_META_CLC);
   odp.pszTitle=Translate("Additional stuffs");
   odp.pfnDlgProc=DlgProcClcMetaOpts;
  // CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
 
   odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_CLCTEXT);
   odp.pszGroup=Translate("Customize");
   odp.pszTitle=Translate("List Text");
   odp.pfnDlgProc=DlgProcClcTextOpts;
   CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

  return 0;
}

struct CheckBoxToStyleEx_t {
  int id;
  DWORD flag;
  int not;
} static const checkBoxToStyleEx[]={
  {IDC_DISABLEDRAGDROP,CLS_EX_DISABLEDRAGDROP,0},
  {IDC_NOTEDITLABELS,CLS_EX_EDITLABELS,1},
  {IDC_SHOWSELALWAYS,CLS_EX_SHOWSELALWAYS,0},
  {IDC_TRACKSELECT,CLS_EX_TRACKSELECT,0},
  {IDC_SHOWGROUPCOUNTS,CLS_EX_SHOWGROUPCOUNTS,0},
  {IDC_HIDECOUNTSWHENEMPTY,CLS_EX_HIDECOUNTSWHENEMPTY,0},
  {IDC_DIVIDERONOFF,CLS_EX_DIVIDERONOFF,0},
  {IDC_NOTNOTRANSLUCENTSEL,CLS_EX_NOTRANSLUCENTSEL,1},
  {IDC_LINEWITHGROUPS,CLS_EX_LINEWITHGROUPS,0},
  {IDC_QUICKSEARCHVISONLY,CLS_EX_QUICKSEARCHVISONLY,0},
  {IDC_SORTGROUPSALPHA,CLS_EX_SORTGROUPSALPHA,0},
  {IDC_NOTNOSMOOTHSCROLLING,CLS_EX_NOSMOOTHSCROLLING,1}};

  struct CheckBoxValues_t {
    DWORD style;
    TCHAR *szDescr;
  };
  static const struct CheckBoxValues_t greyoutValues[]={
    {GREYF_UNFOCUS,_T("Not focused")},
    {MODEF_OFFLINE,_T("Offline")},
    {PF2_ONLINE,_T("Online")},
    {PF2_SHORTAWAY,_T("Away")},
    {PF2_LONGAWAY,_T("NA")},
    {PF2_LIGHTDND,_T("Occupied")},
    {PF2_HEAVYDND,_T("DND")},
    {PF2_FREECHAT,_T("Free for chat")},
    {PF2_INVISIBLE,_T("Invisible")},
    {PF2_OUTTOLUNCH,_T("Out to lunch")},
    {PF2_ONTHEPHONE,_T("On the phone")}};
    static const struct CheckBoxValues_t offlineValues[]={
      {MODEF_OFFLINE,_T("Offline")},
      {PF2_ONLINE,_T("Online")},
      {PF2_SHORTAWAY,_T("Away")},
      {PF2_LONGAWAY,_T("NA")},
      {PF2_LIGHTDND,_T("Occupied")},
      {PF2_HEAVYDND,_T("DND")},
      {PF2_FREECHAT,_T("Free for chat")},
      {PF2_INVISIBLE,_T("Invisible")},
      {PF2_OUTTOLUNCH,_T("Out to lunch")},
      {PF2_ONTHEPHONE,_T("On the phone")}};

      static void FillCheckBoxTree(HWND hwndTree,const struct CheckBoxValues_t *values,int nValues,DWORD style)
      {
        TVINSERTSTRUCT tvis;
        int i;

        tvis.hParent=NULL;
        tvis.hInsertAfter=TVI_LAST;
        tvis.item.mask=TVIF_PARAM|TVIF_TEXT|TVIF_STATE|TVIF_IMAGE;
        for(i=0;i<nValues;i++) {
          tvis.item.lParam=values[i].style;
          tvis.item.pszText=TranslateTS(values[i].szDescr);
          tvis.item.stateMask=TVIS_STATEIMAGEMASK;
          tvis.item.state=INDEXTOSTATEIMAGEMASK((style&tvis.item.lParam)!=0?2:1);
          tvis.item.iImage=tvis.item.iSelectedImage=(style&tvis.item.lParam)!=0?1:0;
          TreeView_InsertItem(hwndTree,&tvis);
        }
      }

      static DWORD MakeCheckBoxTreeFlags(HWND hwndTree)
      {
        DWORD flags=0;
        TVITEMA tvi;

        tvi.mask=TVIF_HANDLE|TVIF_PARAM|TVIF_IMAGE;
        tvi.hItem=TreeView_GetRoot(hwndTree);
        while(tvi.hItem) {
          TreeView_GetItem(hwndTree,&tvi);
          if(tvi.iImage) flags|=tvi.lParam;
          tvi.hItem=TreeView_GetNextSibling(hwndTree,tvi.hItem);
        }
        return flags;
      }

      static BOOL CALLBACK DlgProcClcMetaOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
      {
        LPNMHDR t;
        t=((LPNMHDR)lParam);
        switch (msg)
        {
        case WM_INITDIALOG:

          TranslateDialogDefault(hwndDlg);
          CheckDlgButton(hwndDlg, IDC_META, DBGetContactSettingByte(NULL,"CLC","Meta",0) ? BST_CHECKED : BST_UNCHECKED); /// by FYR
          CheckDlgButton(hwndDlg, IDC_METADBLCLK, DBGetContactSettingByte(NULL,"CLC","MetaDoubleClick",0) ? BST_CHECKED : BST_UNCHECKED); /// by FYR
          CheckDlgButton(hwndDlg, IDC_METASUBEXTRA, DBGetContactSettingByte(NULL,"CLC","MetaHideExtra",0) ? BST_CHECKED : BST_UNCHECKED); /// by FYR
          CheckDlgButton(hwndDlg, IDC_METASUBEXTRA_IGN, DBGetContactSettingByte(NULL,"CLC","MetaIgnoreEmptyExtra",1) ? BST_CHECKED : BST_UNCHECKED); /// by FYR
          CheckDlgButton(hwndDlg, IDC_METASUB_HIDEOFFLINE, DBGetContactSettingByte(NULL,"CLC","MetaHideOfflineSub",1) ? BST_CHECKED : BST_UNCHECKED); /// by FYR
          CheckDlgButton(hwndDlg, IDC_METAEXPAND, DBGetContactSettingByte(NULL,"CLC","MetaExpanding",1) ? BST_CHECKED : BST_UNCHECKED); /// by FYR
          CheckDlgButton(hwndDlg, IDC_DISCOVER_AWAYMSG, DBGetContactSettingByte(NULL,"ModernData","InternalAwayMsgDiscovery",0) ? BST_CHECKED : BST_UNCHECKED); /// by FYR
          CheckDlgButton(hwndDlg, IDC_REMOVE_OFFLINE_AWAYMSG, DBGetContactSettingByte(NULL,"ModernData","RemoveAwayMessageForOffline",0) ? BST_CHECKED : BST_UNCHECKED); /// by FYR

          SendDlgItemMessage(hwndDlg,IDC_SUBINDENTSPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_SUBINDENT),0);	
          SendDlgItemMessage(hwndDlg,IDC_SUBINDENTSPIN,UDM_SETRANGE,0,MAKELONG(50,0));
          SendDlgItemMessage(hwndDlg,IDC_SUBINDENTSPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingByte(NULL,"CLC","SubIndent",CLCDEFAULT_GROUPINDENT),0));

          {
            BYTE t;
            t=IsDlgButtonChecked(hwndDlg,IDC_METAEXPAND);
            EnableWindow(GetDlgItem(hwndDlg,IDC_METADBLCLK),t);
            EnableWindow(GetDlgItem(hwndDlg,IDC_METASUBEXTRA),t);
            EnableWindow(GetDlgItem(hwndDlg,IDC_METASUB_HIDEOFFLINE),t);
            EnableWindow(GetDlgItem(hwndDlg,IDC_SUBINDENTSPIN),t);
            EnableWindow(GetDlgItem(hwndDlg,IDC_SUBINDENT),t);
          }
          {
            BYTE t;
            t=ServiceExists(MS_MC_GETMOSTONLINECONTACT);
            CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_META),t);
            CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_METADBLCLK),t);
            CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_METASUB_HIDEOFFLINE),t);
            CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_METAEXPAND),t);
            CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_METASUBEXTRA),t);
            CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_FRAME_META),t);
            CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_FRAME_META_CAPT),!t); 
            CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_SUBINDENTSPIN),t);
            CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_SUBINDENT),t);
            CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_SUBIDENTCAPT),t);
          }
          return TRUE;
        case WM_COMMAND:
          if(LOWORD(wParam)==IDC_METAEXPAND)
          {
            BYTE t;
            t=IsDlgButtonChecked(hwndDlg,IDC_METAEXPAND);
            EnableWindow(GetDlgItem(hwndDlg,IDC_METADBLCLK),t);
            EnableWindow(GetDlgItem(hwndDlg,IDC_METASUBEXTRA),t);
            EnableWindow(GetDlgItem(hwndDlg,IDC_METASUB_HIDEOFFLINE),t);
            EnableWindow(GetDlgItem(hwndDlg,IDC_SUBINDENTSPIN),t);
            EnableWindow(GetDlgItem(hwndDlg,IDC_SUBINDENT),t);
          }
          if((LOWORD(wParam)==IDC_SUBINDENT) && (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus())) return 0;
          SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
          return TRUE;
        case WM_NOTIFY:

          switch(t->idFrom) 
          {
          case 0:
            switch (t->code)
            {
            case PSN_APPLY:
              DBWriteContactSettingByte(NULL,"CLC","Meta",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_META)); // by FYR
              DBWriteContactSettingByte(NULL,"CLC","MetaDoubleClick",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_METADBLCLK)); // by FYR
              DBWriteContactSettingByte(NULL,"CLC","MetaHideExtra",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_METASUBEXTRA)); // by FYR
              DBWriteContactSettingByte(NULL,"CLC","MetaIgnoreEmptyExtra",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_METASUBEXTRA_IGN)); // by FYR
              DBWriteContactSettingByte(NULL,"CLC","MetaHideOfflineSub",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_METASUB_HIDEOFFLINE)); // by FYR					
              DBWriteContactSettingByte(NULL,"CLC","MetaExpanding",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_METAEXPAND));
              DBWriteContactSettingByte(NULL,"ModernData","InternalAwayMsgDiscovery",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_DISCOVER_AWAYMSG));
              DBWriteContactSettingByte(NULL,"ModernData","RemoveAwayMessageForOffline",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_REMOVE_OFFLINE_AWAYMSG));

              DBWriteContactSettingByte(NULL,"CLC","SubIndent",(BYTE)SendDlgItemMessage(hwndDlg,IDC_SUBINDENTSPIN,UDM_GETPOS,0,0));
              ClcOptionsChanged();
              PostMessage(pcli->hwndContactList,WM_SIZE,0,0);

              return TRUE;
            }
            break;
          }
          break;
        }
        return FALSE;




      }

      static BOOL CALLBACK DlgProcClcMainOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
      {
        switch (msg)
        {
        case WM_INITDIALOG:



          TranslateDialogDefault(hwndDlg);
          SetWindowLong(GetDlgItem(hwndDlg,IDC_GREYOUTOPTS),GWL_STYLE,GetWindowLong(GetDlgItem(hwndDlg,IDC_GREYOUTOPTS),GWL_STYLE)|TVS_NOHSCROLL);
          SetWindowLong(GetDlgItem(hwndDlg,IDC_HIDEOFFLINEOPTS),GWL_STYLE,GetWindowLong(GetDlgItem(hwndDlg,IDC_HIDEOFFLINEOPTS),GWL_STYLE)|TVS_NOHSCROLL);
          {
            HIMAGELIST himlCheckBoxes;
            himlCheckBoxes=ImageList_Create(GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),ILC_COLOR32|ILC_MASK,2,2);
            ImageList_AddIcon(himlCheckBoxes,LoadSmallIconShared(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_NOTICK)));
            ImageList_AddIcon(himlCheckBoxes,LoadSmallIconShared(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_TICK)));
            TreeView_SetImageList(GetDlgItem(hwndDlg,IDC_GREYOUTOPTS),himlCheckBoxes,TVSIL_NORMAL);
            TreeView_SetImageList(GetDlgItem(hwndDlg,IDC_HIDEOFFLINEOPTS),himlCheckBoxes,TVSIL_NORMAL);
          }			
          {	int i;
          DWORD exStyle=DBGetContactSettingDword(NULL,"CLC","ExStyle",GetDefaultExStyle());
          for(i=0;i<sizeof(checkBoxToStyleEx)/sizeof(checkBoxToStyleEx[0]);i++)
            CheckDlgButton(hwndDlg,checkBoxToStyleEx[i].id,(exStyle&checkBoxToStyleEx[i].flag)^(checkBoxToStyleEx[i].flag*checkBoxToStyleEx[i].not)?BST_CHECKED:BST_UNCHECKED);
          }
          {	UDACCEL accel[2]={{0,10},{2,50}};
          SendDlgItemMessage(hwndDlg,IDC_SMOOTHTIMESPIN,UDM_SETRANGE,0,MAKELONG(999,0));
          SendDlgItemMessage(hwndDlg,IDC_SMOOTHTIMESPIN,UDM_SETACCEL,sizeof(accel)/sizeof(accel[0]),(LPARAM)&accel);
          SendDlgItemMessage(hwndDlg,IDC_SMOOTHTIMESPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CLC","ScrollTime",CLCDEFAULT_SCROLLTIME),0));
          }
          CheckDlgButton(hwndDlg,IDC_IDLE,DBGetContactSettingByte(NULL,"CLC","ShowIdle",CLCDEFAULT_SHOWIDLE)?BST_CHECKED:BST_UNCHECKED);

          /*		CheckDlgButton(hwndDlg, IDC_META, DBGetContactSettingByte(NULL,"CLC","Meta",0) ? BST_CHECKED : BST_UNCHECKED); /// by FYR
          CheckDlgButton(hwndDlg, IDC_METADBLCLK, DBGetContactSettingByte(NULL,"CLC","MetaDoubleClick",0) ? BST_CHECKED : BST_UNCHECKED); /// by FYR
          CheckDlgButton(hwndDlg, IDC_METASUBEXTRA, DBGetContactSettingByte(NULL,"CLC","MetaHideExtra",1) ? BST_CHECKED : BST_UNCHECKED); /// by FYR
          */		
          //		SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_SETRANGE,0,MAKELONG(64,0));
          //		SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingByte(NULL,"CLC","LeftMargin",CLCDEFAULT_LEFTMARGIN),0));
          SendDlgItemMessage(hwndDlg,IDC_GROUPINDENTSPIN,UDM_SETRANGE,0,MAKELONG(50,0));
          SendDlgItemMessage(hwndDlg,IDC_GROUPINDENTSPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingByte(NULL,"CLC","GroupIndent",CLCDEFAULT_GROUPINDENT),0));
          CheckDlgButton(hwndDlg,IDC_GREYOUT,DBGetContactSettingDword(NULL,"CLC","GreyoutFlags",CLCDEFAULT_GREYOUTFLAGS)?BST_CHECKED:BST_UNCHECKED);


          EnableWindow(GetDlgItem(hwndDlg,IDC_SMOOTHTIME),IsDlgButtonChecked(hwndDlg,IDC_NOTNOSMOOTHSCROLLING));
          EnableWindow(GetDlgItem(hwndDlg,IDC_GREYOUTOPTS),IsDlgButtonChecked(hwndDlg,IDC_GREYOUT));
          FillCheckBoxTree(GetDlgItem(hwndDlg,IDC_GREYOUTOPTS),greyoutValues,sizeof(greyoutValues)/sizeof(greyoutValues[0]),DBGetContactSettingDword(NULL,"CLC","FullGreyoutFlags",CLCDEFAULT_FULLGREYOUTFLAGS));
          FillCheckBoxTree(GetDlgItem(hwndDlg,IDC_HIDEOFFLINEOPTS),offlineValues,sizeof(offlineValues)/sizeof(offlineValues[0]),DBGetContactSettingDword(NULL,"CLC","OfflineModes",CLCDEFAULT_OFFLINEMODES));
          CheckDlgButton(hwndDlg,IDC_NOSCROLLBAR,DBGetContactSettingByte(NULL,"CLC","NoVScrollBar",0)?BST_CHECKED:BST_UNCHECKED);
          return TRUE;
        case WM_VSCROLL:
          SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
          break;
        case WM_COMMAND:
          if(LOWORD(wParam)==IDC_NOTNOSMOOTHSCROLLING)
            EnableWindow(GetDlgItem(hwndDlg,IDC_SMOOTHTIME),IsDlgButtonChecked(hwndDlg,IDC_NOTNOSMOOTHSCROLLING));
          if(LOWORD(wParam)==IDC_GREYOUT)
            EnableWindow(GetDlgItem(hwndDlg,IDC_GREYOUTOPTS),IsDlgButtonChecked(hwndDlg,IDC_GREYOUT));
          if((/*LOWORD(wParam)==IDC_LEFTMARGIN ||*/ LOWORD(wParam)==IDC_SMOOTHTIME || LOWORD(wParam)==IDC_GROUPINDENT) && (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus())) return 0;
          SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
          break;
        case WM_NOTIFY:
          switch(((LPNMHDR)lParam)->idFrom) {
        case IDC_GREYOUTOPTS:
        case IDC_HIDEOFFLINEOPTS:
          if(((LPNMHDR)lParam)->code==NM_CLICK) {
            TVHITTESTINFO hti;
            hti.pt.x=(short)LOWORD(GetMessagePos());
            hti.pt.y=(short)HIWORD(GetMessagePos());
            ScreenToClient(((LPNMHDR)lParam)->hwndFrom,&hti.pt);
            if(TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom,&hti))
              if(hti.flags&TVHT_ONITEMICON) {
                TVITEMA tvi;
                tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
                tvi.hItem=hti.hItem;
                TreeView_GetItem(((LPNMHDR)lParam)->hwndFrom,&tvi);
                tvi.iImage=tvi.iSelectedImage=tvi.iImage=!tvi.iImage;
                //tvi.state=tvi.iImage?2:1;
                TreeView_SetItem(((LPNMHDR)lParam)->hwndFrom,&tvi);
                SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
              }

              /*
              case NM_CLICK:
              {

              TVHITTESTINFO hti;
              hti.pt.x=(short)LOWORD(GetMessagePos());
              hti.pt.y=(short)HIWORD(GetMessagePos());
              ScreenToClient(((LPNMHDR)lParam)->hwndFrom,&hti.pt);
              if(TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom,&hti))
              if(hti.flags&TVHT_ONITEMICON) {
              TVITEMA tvi;
              tvi.mask=TVIF_HANDLE|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
              tvi.hItem=hti.hItem;
              TreeView_GetItem(((LPNMHDR)lParam)->hwndFrom,&tvi);
              tvi.iImage=tvi.iSelectedImage=!tvi.iImage;
              ((ProtocolData *)tvi.lParam)->show=tvi.iImage;
              TreeView_SetItem(((LPNMHDR)lParam)->hwndFrom,&tvi);
              SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);

              //all changes take effect in runtime
              //CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_PROTOCOLORDERWARNING),SW_SHOW);
              }
              */
          }
          break;
        case 0:
          switch (((LPNMHDR)lParam)->code)
          {
          case PSN_APPLY:
            {	int i;
            DWORD exStyle=0;
            for(i=0;i<sizeof(checkBoxToStyleEx)/sizeof(checkBoxToStyleEx[0]);i++)
              if((IsDlgButtonChecked(hwndDlg,checkBoxToStyleEx[i].id)==0)==checkBoxToStyleEx[i].not)
                exStyle|=checkBoxToStyleEx[i].flag;
            DBWriteContactSettingDword(NULL,"CLC","ExStyle",exStyle);
            }
            {	DWORD fullGreyoutFlags=MakeCheckBoxTreeFlags(GetDlgItem(hwndDlg,IDC_GREYOUTOPTS));
            DBWriteContactSettingDword(NULL,"CLC","FullGreyoutFlags",fullGreyoutFlags);
            if(IsDlgButtonChecked(hwndDlg,IDC_GREYOUT))
              DBWriteContactSettingDword(NULL,"CLC","GreyoutFlags",fullGreyoutFlags);
            else
              DBWriteContactSettingDword(NULL,"CLC","GreyoutFlags",0);
            }
            /*						DBWriteContactSettingByte(NULL,"CLC","Meta",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_META)); // by FYR
            DBWriteContactSettingByte(NULL,"CLC","MetaDoubleClick",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_METADBLCLK)); // by FYR
            DBWriteContactSettingByte(NULL,"CLC","MetaHideExtra",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_METASUBEXTRA)); // by FYR

            */						
            DBWriteContactSettingByte(NULL,"CLC","ShowIdle",(BYTE)(IsDlgButtonChecked(hwndDlg,IDC_IDLE)?1:0));
            DBWriteContactSettingDword(NULL,"CLC","OfflineModes",MakeCheckBoxTreeFlags(GetDlgItem(hwndDlg,IDC_HIDEOFFLINEOPTS)));
            //						DBWriteContactSettingByte(NULL,"CLC","LeftMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_GETPOS,0,0));
            DBWriteContactSettingWord(NULL,"CLC","ScrollTime",(WORD)SendDlgItemMessage(hwndDlg,IDC_SMOOTHTIMESPIN,UDM_GETPOS,0,0));
            DBWriteContactSettingByte(NULL,"CLC","GroupIndent",(BYTE)SendDlgItemMessage(hwndDlg,IDC_GROUPINDENTSPIN,UDM_GETPOS,0,0));
            DBWriteContactSettingByte(NULL,"CLC","NoVScrollBar",(BYTE)(IsDlgButtonChecked(hwndDlg,IDC_NOSCROLLBAR)?1:0));


            ClcOptionsChanged();
            return TRUE;
          }
          break;
          }
          break;
        case WM_DESTROY:
          ImageList_Destroy(TreeView_GetImageList(GetDlgItem(hwndDlg,IDC_GREYOUTOPTS),TVSIL_NORMAL));
          break;
        }
        return FALSE;
      }

      static BOOL CALLBACK DlgProcStatusBarBkgOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
      {
        switch (msg)
        {
        case WM_INITDIALOG:
          TranslateDialogDefault(hwndDlg);
          CheckDlgButton(hwndDlg,IDC_BITMAP,DBGetContactSettingByte(NULL,"StatusBar","UseBitmap",CLCDEFAULT_USEBITMAP)?BST_CHECKED:BST_UNCHECKED);
          SendMessage(hwndDlg,WM_USER+10,0,0);
          SendDlgItemMessage(hwndDlg,IDC_BKGCOLOUR,CPM_SETDEFAULTCOLOUR,0,CLCDEFAULT_BKCOLOUR);
          //		SendDlgItemMessage(hwndDlg,IDC_BKGCOLOUR,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"StatusBar","BkColour",CLCDEFAULT_BKCOLOUR));
          SendDlgItemMessage(hwndDlg,IDC_SELCOLOUR,CPM_SETDEFAULTCOLOUR,0,CLCDEFAULT_SELBKCOLOUR);
          SendDlgItemMessage(hwndDlg,IDC_SELCOLOUR,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"StatusBar","SelBkColour",CLCDEFAULT_SELBKCOLOUR));
          {	DBVARIANT dbv={0};
          if(!DBGetContactSetting(NULL,"StatusBar","BkBitmap",&dbv)) {
            SetDlgItemTextA(hwndDlg,IDC_FILENAME,dbv.pszVal);
            if (ServiceExists(MS_UTILS_PATHTOABSOLUTE)) {
              char szPath[MAX_PATH];

              if (CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)szPath))
                SetDlgItemTextA(hwndDlg,IDC_FILENAME,szPath);
            }
            else 
              //mir_free(dbv.pszVal);
            DBFreeVariant(&dbv);
          }
          }

          CheckDlgButton(hwndDlg,IDC_HILIGHTMODE,DBGetContactSettingByte(NULL,"StatusBar","HiLightMode",0)==0?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_HILIGHTMODE1,DBGetContactSettingByte(NULL,"StatusBar","HiLightMode",0)==1?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_HILIGHTMODE2,DBGetContactSettingByte(NULL,"StatusBar","HiLightMode",0)==2?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_HILIGHTMODE3,DBGetContactSettingByte(NULL,"StatusBar","HiLightMode",0)==3?BST_CHECKED:BST_UNCHECKED);



          {	WORD bmpUse=DBGetContactSettingWord(NULL,"StatusBar","BkBmpUse",CLCDEFAULT_BKBMPUSE);
          CheckDlgButton(hwndDlg,IDC_STRETCHH,bmpUse&CLB_STRETCHH?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_STRETCHV,bmpUse&CLB_STRETCHV?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_TILEH,bmpUse&CLBF_TILEH?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_TILEV,bmpUse&CLBF_TILEV?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_SCROLL,bmpUse&CLBF_SCROLL?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_PROPORTIONAL,bmpUse&CLBF_PROPORTIONAL?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_TILEVROWH,bmpUse&CLBF_TILEVTOROWHEIGHT?BST_CHECKED:BST_UNCHECKED);

          }
          {	HRESULT (STDAPICALLTYPE *MySHAutoComplete)(HWND,DWORD);
          MySHAutoComplete=(HRESULT (STDAPICALLTYPE*)(HWND,DWORD))GetProcAddress(GetModuleHandle(TEXT("shlwapi")),"SHAutoComplete");
          if(MySHAutoComplete) MySHAutoComplete(GetDlgItem(hwndDlg,IDC_FILENAME),1);
          }
          return TRUE;
        case WM_USER+10:
          EnableWindow(GetDlgItem(hwndDlg,IDC_FILENAME),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
          EnableWindow(GetDlgItem(hwndDlg,IDC_BROWSE),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
          EnableWindow(GetDlgItem(hwndDlg,IDC_STRETCHH),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
          EnableWindow(GetDlgItem(hwndDlg,IDC_STRETCHV),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
          EnableWindow(GetDlgItem(hwndDlg,IDC_TILEH),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
          EnableWindow(GetDlgItem(hwndDlg,IDC_TILEV),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
          EnableWindow(GetDlgItem(hwndDlg,IDC_SCROLL),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
          EnableWindow(GetDlgItem(hwndDlg,IDC_PROPORTIONAL),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
          EnableWindow(GetDlgItem(hwndDlg,IDC_TILEVROWH),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
          break;
        case WM_COMMAND:
          if(LOWORD(wParam)==IDC_BROWSE) {
            char str[MAX_PATH];
            OPENFILENAMEA ofn={0};
            char filter[512];

            GetDlgItemTextA(hwndDlg,IDC_FILENAME,str,sizeof(str));
            ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
            ofn.hwndOwner = hwndDlg;
            ofn.hInstance = NULL;
            CallService(MS_UTILS_GETBITMAPFILTERSTRINGS,sizeof(filter),(LPARAM)filter);
            ofn.lpstrFilter = filter;
            ofn.lpstrFile = str;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
            ofn.nMaxFile = sizeof(str);
            ofn.nMaxFileTitle = MAX_PATH;
            ofn.lpstrDefExt = "bmp";
            if(!GetOpenFileNameA(&ofn)) break;
            SetDlgItemTextA(hwndDlg,IDC_FILENAME,str);
          }
          else if(LOWORD(wParam)==IDC_FILENAME && HIWORD(wParam)!=EN_CHANGE) break;
          if(LOWORD(wParam)==IDC_BITMAP) SendMessage(hwndDlg,WM_USER+10,0,0);
          if(LOWORD(wParam)==IDC_FILENAME && (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus())) return 0;
          SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
          break;
        case WM_NOTIFY:
          switch(((LPNMHDR)lParam)->idFrom) {
        case 0:
          switch (((LPNMHDR)lParam)->code)
          {
          case PSN_APPLY:



            DBWriteContactSettingByte(NULL,"StatusBar","UseBitmap",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
            {	COLORREF col;
            col=SendDlgItemMessage(hwndDlg,IDC_BKGCOLOUR,CPM_GETCOLOUR,0,0);
            if(col==CLCDEFAULT_BKCOLOUR) DBDeleteContactSetting(NULL,"StatusBar","BkColour");
            else DBWriteContactSettingDword(NULL,"StatusBar","BkColour",col);
            col=SendDlgItemMessage(hwndDlg,IDC_SELCOLOUR,CPM_GETCOLOUR,0,0);
            if(col==CLCDEFAULT_SELBKCOLOUR) DBDeleteContactSetting(NULL,"StatusBar","SelBkColour");
            else DBWriteContactSettingDword(NULL,"StatusBar","SelBkColour",col);
            }
            {	
              char str[MAX_PATH],strrel[MAX_PATH];
              GetDlgItemTextA(hwndDlg,IDC_FILENAME,str,sizeof(str));
              if (ServiceExists(MS_UTILS_PATHTORELATIVE)) {
                if (CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)str, (LPARAM)strrel))
                  DBWriteContactSettingString(NULL,"StatusBar","BkBitmap",strrel);
                else DBWriteContactSettingString(NULL,"StatusBar","BkBitmap",str);
              }
              else DBWriteContactSettingString(NULL,"StatusBar","BkBitmap",str);

            }
            {	WORD flags=0;
            if(IsDlgButtonChecked(hwndDlg,IDC_STRETCHH)) flags|=CLB_STRETCHH;
            if(IsDlgButtonChecked(hwndDlg,IDC_STRETCHV)) flags|=CLB_STRETCHV;
            if(IsDlgButtonChecked(hwndDlg,IDC_TILEH)) flags|=CLBF_TILEH;
            if(IsDlgButtonChecked(hwndDlg,IDC_TILEV)) flags|=CLBF_TILEV;
            if(IsDlgButtonChecked(hwndDlg,IDC_SCROLL)) flags|=CLBF_SCROLL;
            if(IsDlgButtonChecked(hwndDlg,IDC_PROPORTIONAL)) flags|=CLBF_PROPORTIONAL;
            if(IsDlgButtonChecked(hwndDlg,IDC_TILEVROWH)) flags|=CLBF_TILEVTOROWHEIGHT;

            DBWriteContactSettingWord(NULL,"StatusBar","BkBmpUse",flags);
            }
            {
              int hil=0;
              if (IsDlgButtonChecked(hwndDlg,IDC_HILIGHTMODE1))  hil=1;
              if (IsDlgButtonChecked(hwndDlg,IDC_HILIGHTMODE2))  hil=2;
              if (IsDlgButtonChecked(hwndDlg,IDC_HILIGHTMODE3))  hil=3;

              DBWriteContactSettingByte(NULL,"StatusBar","HiLightMode",(BYTE)hil);

            }

            ClcOptionsChanged();
            //OnStatusBarBackgroundChange();
            return TRUE;
          }
          break;
          }
          break;
        }
        return FALSE;
      }


      static BOOL CALLBACK DlgProcClcBkgOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
      {
        switch (msg)
        {
        case WM_INITDIALOG:
          TranslateDialogDefault(hwndDlg);
          CheckDlgButton(hwndDlg,IDC_BITMAP,DBGetContactSettingByte(NULL,"CLC","UseBitmap",CLCDEFAULT_USEBITMAP)?BST_CHECKED:BST_UNCHECKED);
          SendMessage(hwndDlg,WM_USER+10,0,0);
          SendDlgItemMessage(hwndDlg,IDC_BKGCOLOUR,CPM_SETDEFAULTCOLOUR,0,CLCDEFAULT_BKCOLOUR);
          SendDlgItemMessage(hwndDlg,IDC_BKGCOLOUR,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"CLC","BkColour",CLCDEFAULT_BKCOLOUR));
          SendDlgItemMessage(hwndDlg,IDC_SELCOLOUR,CPM_SETDEFAULTCOLOUR,0,CLCDEFAULT_SELBKCOLOUR);
          SendDlgItemMessage(hwndDlg,IDC_SELCOLOUR,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"CLC","SelBkColour",CLCDEFAULT_SELBKCOLOUR));
          {	DBVARIANT dbv={0};
          if(!DBGetContactSetting(NULL,"CLC","BkBitmap",&dbv)) {
            SetDlgItemTextA(hwndDlg,IDC_FILENAME,dbv.pszVal);
            if (ServiceExists(MS_UTILS_PATHTOABSOLUTE)) {
              char szPath[MAX_PATH];

              if (CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)szPath))
                SetDlgItemTextA(hwndDlg,IDC_FILENAME,szPath);
            }
            else 
              //mir_free(dbv.pszVal);
            DBFreeVariant(&dbv);
          }
          }

          /*			CheckDlgButton(hwndDlg,IDC_HILIGHTMODE,DBGetContactSettingByte(NULL,"CLC","HiLightMode",0)==0?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_HILIGHTMODE1,DBGetContactSettingByte(NULL,"CLC","HiLightMode",0)==1?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_HILIGHTMODE2,DBGetContactSettingByte(NULL,"CLC","HiLightMode",0)==2?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_HILIGHTMODE3,DBGetContactSettingByte(NULL,"CLC","HiLightMode",0)==3?BST_CHECKED:BST_UNCHECKED);

          */			

          {	WORD bmpUse=DBGetContactSettingWord(NULL,"CLC","BkBmpUse",CLCDEFAULT_BKBMPUSE);
          CheckDlgButton(hwndDlg,IDC_STRETCHH,bmpUse&CLB_STRETCHH?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_STRETCHV,bmpUse&CLB_STRETCHV?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_TILEH,bmpUse&CLBF_TILEH?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_TILEV,bmpUse&CLBF_TILEV?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_SCROLL,bmpUse&CLBF_SCROLL?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_PROPORTIONAL,bmpUse&CLBF_PROPORTIONAL?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_TILEVROWH,bmpUse&CLBF_TILEVTOROWHEIGHT?BST_CHECKED:BST_UNCHECKED);

          }
          {	HRESULT (STDAPICALLTYPE *MySHAutoComplete)(HWND,DWORD);
          MySHAutoComplete=(HRESULT (STDAPICALLTYPE*)(HWND,DWORD))GetProcAddress(GetModuleHandle(TEXT("shlwapi")),"SHAutoComplete");
          if(MySHAutoComplete) MySHAutoComplete(GetDlgItem(hwndDlg,IDC_FILENAME),1);
          }
          return TRUE;
        case WM_USER+10:
          EnableWindow(GetDlgItem(hwndDlg,IDC_FILENAME),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
          EnableWindow(GetDlgItem(hwndDlg,IDC_BROWSE),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
          EnableWindow(GetDlgItem(hwndDlg,IDC_STRETCHH),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
          EnableWindow(GetDlgItem(hwndDlg,IDC_STRETCHV),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
          EnableWindow(GetDlgItem(hwndDlg,IDC_TILEH),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
          EnableWindow(GetDlgItem(hwndDlg,IDC_TILEV),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
          EnableWindow(GetDlgItem(hwndDlg,IDC_SCROLL),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
          EnableWindow(GetDlgItem(hwndDlg,IDC_PROPORTIONAL),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
          EnableWindow(GetDlgItem(hwndDlg,IDC_TILEVROWH),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
          break;
        case WM_COMMAND:
          if(LOWORD(wParam)==IDC_BROWSE) {
            char str[MAX_PATH];
            OPENFILENAMEA ofn={0};
            char filter[512];

            GetDlgItemTextA(hwndDlg,IDC_FILENAME,str,sizeof(str));
            ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
            ofn.hwndOwner = hwndDlg;
            ofn.hInstance = NULL;
            CallService(MS_UTILS_GETBITMAPFILTERSTRINGS,sizeof(filter),(LPARAM)filter);
            ofn.lpstrFilter = filter;
            ofn.lpstrFile = str;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
            ofn.nMaxFile = sizeof(str);
            ofn.nMaxFileTitle = MAX_PATH;
            ofn.lpstrDefExt = "bmp";
            if(!GetOpenFileNameA(&ofn)) break;
            SetDlgItemTextA(hwndDlg,IDC_FILENAME,str);
          }
          else if(LOWORD(wParam)==IDC_FILENAME && HIWORD(wParam)!=EN_CHANGE) break;
          if(LOWORD(wParam)==IDC_BITMAP) SendMessage(hwndDlg,WM_USER+10,0,0);
          if(LOWORD(wParam)==IDC_FILENAME && (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus())) return 0;
          SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
          break;
        case WM_NOTIFY:
          switch(((LPNMHDR)lParam)->idFrom) {
        case 0:
          switch (((LPNMHDR)lParam)->code)
          {
          case PSN_APPLY:



            DBWriteContactSettingByte(NULL,"CLC","UseBitmap",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
            {	COLORREF col;
            col=SendDlgItemMessage(hwndDlg,IDC_BKGCOLOUR,CPM_GETCOLOUR,0,0);
            if(col==CLCDEFAULT_BKCOLOUR) DBDeleteContactSetting(NULL,"CLC","BkColour");
            else DBWriteContactSettingDword(NULL,"CLC","BkColour",col);
            col=SendDlgItemMessage(hwndDlg,IDC_SELCOLOUR,CPM_GETCOLOUR,0,0);
            if(col==CLCDEFAULT_SELBKCOLOUR) DBDeleteContactSetting(NULL,"CLC","SelBkColour");
            else DBWriteContactSettingDword(NULL,"CLC","SelBkColour",col);
            }
            {	
              char str[MAX_PATH],strrel[MAX_PATH];
              GetDlgItemTextA(hwndDlg,IDC_FILENAME,str,sizeof(str));
              if (ServiceExists(MS_UTILS_PATHTORELATIVE)) {
                if (CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)str, (LPARAM)strrel))
                  DBWriteContactSettingString(NULL,"CLC","BkBitmap",strrel);
                else DBWriteContactSettingString(NULL,"CLC","BkBitmap",str);
              }
              else DBWriteContactSettingString(NULL,"CLC","BkBitmap",str);

            }
            {	WORD flags=0;
            if(IsDlgButtonChecked(hwndDlg,IDC_STRETCHH)) flags|=CLB_STRETCHH;
            if(IsDlgButtonChecked(hwndDlg,IDC_STRETCHV)) flags|=CLB_STRETCHV;
            if(IsDlgButtonChecked(hwndDlg,IDC_TILEH)) flags|=CLBF_TILEH;
            if(IsDlgButtonChecked(hwndDlg,IDC_TILEV)) flags|=CLBF_TILEV;
            if(IsDlgButtonChecked(hwndDlg,IDC_SCROLL)) flags|=CLBF_SCROLL;
            if(IsDlgButtonChecked(hwndDlg,IDC_PROPORTIONAL)) flags|=CLBF_PROPORTIONAL;
            if(IsDlgButtonChecked(hwndDlg,IDC_TILEVROWH)) flags|=CLBF_TILEVTOROWHEIGHT;

            DBWriteContactSettingWord(NULL,"CLC","BkBmpUse",flags);
            }
            /*							{
            int hil=0;
            if (IsDlgButtonChecked(hwndDlg,IDC_HILIGHTMODE1))  hil=1;
            if (IsDlgButtonChecked(hwndDlg,IDC_HILIGHTMODE2))  hil=2;
            if (IsDlgButtonChecked(hwndDlg,IDC_HILIGHTMODE3))  hil=3;

            DBWriteContactSettingByte(NULL,"CLC","HiLightMode",(BYTE)hil);

            }
            */
            ClcOptionsChanged();
            CLUIServices_ProtocolStatusChanged(0,0);
            if (IsWindowVisible(pcli->hwndContactList))
            {
              CLUI_ShowWindowMod(pcli->hwndContactList,SW_HIDE);
              CLUI_ShowWindowMod(pcli->hwndContactList,SW_SHOW);
            }

            return TRUE;
          }
          break;
          }
          break;
        }
        return FALSE;
      }

      static const TCHAR *szFontIdDescr[FONTID_MODERN_MAX+1]=
      {_T("Standard contacts"),
      _T("Online contacts to whom you have a different visibility"),
      _T("Offline contacts"),
      _T("Contacts which are 'not on list'"),
      _T("Open groups"),
      _T("Open group member counts"),
      _T("Dividers"),
      _T("Offline contacts to whom you have a different visibility"),
      _T("Second line"),
      _T("Third line"),
      _T("Away contacts"),
      _T("DND contacts"),
      _T("NA contacts"),
      _T("Occupied contacts"),
      _T("Free for chat contacts"),
      _T("Invisible contacts"),
      _T("On the phone contacts"),
      _T("Out to lunch contacts"),
      _T("Contact time"),
      _T("Closed groups"),
      _T("Closed group member counts"),
      _T("Status bar text"),
      _T("Event area text")
      };

#include <pshpack1.h>
      struct _fontSettings{
        BYTE sameAsFlags,sameAs;
        COLORREF colour;
        char size;
        BYTE style;
        BYTE charset;
        char szFace[LF_FACESIZE];
        BYTE Effect;
        COLORREF EffectColor1;
        COLORREF EffectColor2;
      } static fontSettings[FONTID_MODERN_MAX+1];
#include <poppack.h>

#define M_REBUILDFONTGROUP   (WM_USER+10)
#define M_REMAKESAMPLE       (WM_USER+11)
#define M_RECALCONEFONT      (WM_USER+12)
#define M_RECALCOTHERFONTS   (WM_USER+13)
#define M_SAVEFONT           (WM_USER+14)
#define M_REFRESHSAMEASBOXES (WM_USER+15)
#define M_FILLSCRIPTCOMBO    (WM_USER+16)
#define M_REDOROWHEIGHT      (WM_USER+17)
#define M_LOADFONT           (WM_USER+18)
#define M_GUESSSAMEASBOXES   (WM_USER+19)
#define M_SETSAMEASBOXES     (WM_USER+20)

      static int CALLBACK EnumFontsProc(ENUMLOGFONTEXA *lpelfe,NEWTEXTMETRICEX *lpntme,int FontType,LPARAM lParam)
      {
        if(!IsWindow((HWND)lParam)) return FALSE;
        if(SendMessageA((HWND)lParam,CB_FINDSTRINGEXACT,-1,(LPARAM)lpelfe->elfLogFont.lfFaceName)==CB_ERR)
          SendMessageA((HWND)lParam,CB_ADDSTRING,0,(LPARAM)lpelfe->elfLogFont.lfFaceName);
        return TRUE;
      }

      void FillFontListThread(HWND hwndDlg)
      {
        LOGFONTA lf={0};
        HDC hdc=GetDC(hwndDlg);
        lf.lfCharSet=DEFAULT_CHARSET;
        lf.lfFaceName[0]=0;
        lf.lfPitchAndFamily=0;
        EnumFontFamiliesExA(hdc,&lf,(FONTENUMPROCA)EnumFontsProc,(LPARAM)GetDlgItem(hwndDlg,IDC_TYPEFACE),0);
        ReleaseDC(hwndDlg,hdc);
        g_hFillFontListThreadID=0;
        return;
      }

      static int CALLBACK EnumFontScriptsProc(ENUMLOGFONTEXA *lpelfe,NEWTEXTMETRICEXA *lpntme,int FontType,LPARAM lParam)
      {
        if(SendMessageA((HWND)lParam,CB_FINDSTRINGEXACT,-1,(LPARAM)lpelfe->elfScript)==CB_ERR) {
          int i=SendMessageA((HWND)lParam,CB_ADDSTRING,0,(LPARAM)lpelfe->elfScript);
          SendMessageA((HWND)lParam,CB_SETITEMDATA,i,lpelfe->elfLogFont.lfCharSet);
        }
        return TRUE;
      }

      static int TextOptsDlgResizer(HWND hwndDlg,LPARAM lParam,UTILRESIZECONTROL *urc)
      {
        return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
      }

      static void SwitchTextDlgToMode(HWND hwndDlg,int expert)
      {
        CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_GAMMACORRECT),expert?SW_SHOW:SW_HIDE);
        CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_STSAMETEXT),expert?SW_SHOW:SW_HIDE);
        CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_SAMETYPE),expert?SW_SHOW:SW_HIDE);
        CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_SAMESIZE),expert?SW_SHOW:SW_HIDE);
        CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_SAMESTYLE),expert?SW_SHOW:SW_HIDE);
        CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_SAMECOLOUR),expert?SW_SHOW:SW_HIDE);
		CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_SAMEEFFECT),expert?SW_SHOW:SW_HIDE);
        CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_STSIZETEXT),expert?SW_HIDE:SW_SHOW);
        CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_STCOLOURTEXT),expert?SW_HIDE:SW_SHOW);
        SetDlgItemTextA(hwndDlg,IDC_STASTEXT,Translate(expert?"as:":"based on:"));
        {	UTILRESIZEDIALOG urd={0};
        urd.cbSize=sizeof(urd);
        urd.hwndDlg=hwndDlg;
        urd.hInstance=g_hInst;
        urd.lpTemplate=MAKEINTRESOURCEA(expert?IDD_OPT_CLCTEXT:IDD_OPT_CLCTEXT);
        urd.pfnResizer=TextOptsDlgResizer;
        CallService(MS_UTILS_RESIZEDIALOG,0,(LPARAM)&urd);
        }
        //resizer breaks the sizing of the edit box
        //SendDlgItemMessage(hwndDlg,IDC_ROWHEIGHTSPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_ROWHEIGHT),0);
        SendMessage(hwndDlg,M_REFRESHSAMEASBOXES,SendDlgItemMessage(hwndDlg,IDC_FONTID,CB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_FONTID,CB_GETCURSEL,0,0),0),0);
      }
#include "effectenum.h"
//extern TCHAR *ModernEffectNames[];
      static BOOL CALLBACK DlgProcClcTextOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
      {
        static HFONT hFontSample;
        static BYTE EffectSample=0;
        static COLORREF ColorSample=0,Color1Sample=0,Color2Sample=0;


        switch (msg)
        {
        case WM_INITDIALOG:
          hFontSample=NULL;

          TranslateDialogDefault(hwndDlg);
          {
            TCHAR sample[50];
            _sntprintf(sample,sizeof(sample)/sizeof(TCHAR),TEXT("01:23 Sample - %s"),TranslateT("Sample"));
            SetDlgItemText(hwndDlg,IDC_SAMPLE,sample);
          }

          //CheckDlgButton(hwndDlg,IDC_NOTCHECKFONTSIZE,DBGetContactSettingByte(NULL,"CLC","DoNotCheckFontSize",0)?BST_CHECKED:BST_UNCHECKED);			

          if(!SendMessage(GetParent(hwndDlg),PSM_ISEXPERT,0,0))
            SwitchTextDlgToMode(hwndDlg,0);
          g_hFillFontListThreadID=(DWORD)forkthread(FillFontListThread,0,hwndDlg);				
          {	int i,itemId,fontId;
          LOGFONTA lf={0};
          COLORREF colour;
          BYTE effect;
          COLORREF eColour1;
          COLORREF eColour2;
          WORD sameAs;
          char str[32];

 
          for(i=0;i<=FONTID_MODERN_MAX;i++) {
            fontId=fontListOrder[i];
            GetFontSetting(fontId,&lf,&colour,&effect,&eColour1,&eColour2);

            sprintf(str,"Font%dAs",fontId);
            sameAs=DBGetContactSettingWord(NULL,"CLC",str,fontSameAsDefault[fontId]);
            fontSettings[fontId].sameAsFlags=HIBYTE(sameAs);
            fontSettings[fontId].sameAs=LOBYTE(sameAs);

            fontSettings[fontId].style=(lf.lfWeight==FW_NORMAL?0:DBFONTF_BOLD)|(lf.lfItalic?DBFONTF_ITALIC:0)|(lf.lfUnderline?DBFONTF_UNDERLINE:0);
            if (lf.lfHeight<0)
            {
              HDC hdc;
              SIZE size;
              HFONT hFont;
              hdc=GetDC(hwndDlg); 
              hFont=CreateFontIndirectA(&lf);						
              SelectObject(hdc,hFont);					
              GetTextExtentPoint32A(hdc,"_W",2,&size);
              ReleaseDC(hwndDlg,hdc);
              DeleteObject(hFont);
              fontSettings[fontId].size=(char)size.cy;
            }
            else fontSettings[fontId].size=(char)lf.lfHeight;
            fontSettings[fontId].charset=lf.lfCharSet;
            fontSettings[fontId].colour=colour;
            fontSettings[fontId].Effect=effect;
            fontSettings[fontId].EffectColor1=eColour1;
            fontSettings[fontId].EffectColor2=eColour2;
            lstrcpyA(fontSettings[fontId].szFace,lf.lfFaceName);
            itemId=SendDlgItemMessage(hwndDlg,IDC_FONTID,CB_ADDSTRING,0,(LPARAM)TranslateTS(szFontIdDescr[fontId]));
            SendDlgItemMessage(hwndDlg,IDC_FONTID,CB_SETITEMDATA,itemId,fontId);
          }

          for(i=0;i<sizeof(fontSizes)/sizeof(fontSizes[0]);i++)
            SendDlgItemMessage(hwndDlg,IDC_FONTSIZE,CB_ADDSTRING,0,(LPARAM)fontSizes[i]);
          }    
          {
            int i=0;
            int count=MAXPREDEFINEDEFFECTS;
            int itemid=SendDlgItemMessage(hwndDlg,IDC_EFFECT_NAME,CB_ADDSTRING,0,(LPARAM)TranslateTS(_T("<none>")));
            SendDlgItemMessage(hwndDlg,IDC_FONTID,CB_SETITEMDATA,itemid,0);
            for (i=0;i<count;i++)
            {
              itemid=SendDlgItemMessage(hwndDlg,IDC_EFFECT_NAME,CB_ADDSTRING,0,(LPARAM)TranslateTS(ModernEffectNames[i]));
              SendDlgItemMessage(hwndDlg,IDC_EFFECT_NAME,CB_SETITEMDATA,itemid,i+1);
              SendDlgItemMessage(hwndDlg,IDC_EFFECT_NAME,CB_SETCURSEL,0,0);
            }
          }
		  SendDlgItemMessage(hwndDlg,IDC_EFFECT_COLOUR_SPIN1,UDM_SETRANGE,0,MAKELONG(255,0));
		  SendDlgItemMessage(hwndDlg,IDC_EFFECT_COLOUR_SPIN2,UDM_SETRANGE,0,MAKELONG(255,0));
		  SendDlgItemMessage(hwndDlg,IDC_FONTID,CB_SETCURSEL,0,0);
          //SendDlgItemMessage(hwndDlg,IDC_ROWHEIGHTSPIN,UDM_SETRANGE,0,MAKELONG(255,0));
          //SendDlgItemMessage(hwndDlg,IDC_ROWHEIGHTSPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingByte(NULL,"CLC","RowHeight",CLCDEFAULT_ROWHEIGHT),0));
          SendMessage(hwndDlg,M_REBUILDFONTGROUP,0,0);
          SendMessage(hwndDlg,M_SAVEFONT,0,0);
          SendDlgItemMessage(hwndDlg,IDC_HOTCOLOUR,CPM_SETDEFAULTCOLOUR,0,CLCDEFAULT_HOTTEXTCOLOUR);
          SendDlgItemMessage(hwndDlg,IDC_HOTCOLOUR,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"CLC","HotTextColour",CLCDEFAULT_HOTTEXTCOLOUR));
          CheckDlgButton(hwndDlg,IDC_GAMMACORRECT,DBGetContactSettingByte(NULL,"CLC","GammaCorrect",CLCDEFAULT_GAMMACORRECT)?BST_CHECKED:BST_UNCHECKED);
          SendDlgItemMessage(hwndDlg,IDC_SELCOLOUR,CPM_SETDEFAULTCOLOUR,0,CLCDEFAULT_SELTEXTCOLOUR);
          SendDlgItemMessage(hwndDlg,IDC_SELCOLOUR,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"CLC","SelTextColour",CLCDEFAULT_SELTEXTCOLOUR));
          SendDlgItemMessage(hwndDlg,IDC_QUICKCOLOUR,CPM_SETDEFAULTCOLOUR,0,CLCDEFAULT_QUICKSEARCHCOLOUR);
          SendDlgItemMessage(hwndDlg,IDC_QUICKCOLOUR,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"CLC","QuickSearchColour",CLCDEFAULT_QUICKSEARCHCOLOUR));
		  
       
		  CheckDlgButton(hwndDlg,IDC_HILIGHTMODE,DBGetContactSettingByte(NULL,"CLC","HiLightMode",0)==0?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_HILIGHTMODE1,DBGetContactSettingByte(NULL,"CLC","HiLightMode",0)==1?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_HILIGHTMODE2,DBGetContactSettingByte(NULL,"CLC","HiLightMode",0)==2?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_HILIGHTMODE3,DBGetContactSettingByte(NULL,"CLC","HiLightMode",0)==3?BST_CHECKED:BST_UNCHECKED);
          
          return TRUE;
        case M_REBUILDFONTGROUP:	//remake all the needed controls when the user changes the font selector at the top
          {	int i=SendDlgItemMessage(hwndDlg,IDC_FONTID,CB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_FONTID,CB_GETCURSEL,0,0),0);
          SendMessage(hwndDlg,M_SETSAMEASBOXES,i,0);
          {	int j,id,itemId;
          TCHAR szText[256];
          SendDlgItemMessage(hwndDlg,IDC_SAMEAS,CB_RESETCONTENT,0,0);
          itemId=SendDlgItemMessage(hwndDlg,IDC_SAMEAS,CB_ADDSTRING,0,(LPARAM)TranslateT("<none>"));
          SendDlgItemMessage(hwndDlg,IDC_SAMEAS,CB_SETITEMDATA,itemId,0xFF);
          if(0xFF==fontSettings[i].sameAs)
            SendDlgItemMessage(hwndDlg,IDC_SAMEAS,CB_SETCURSEL,itemId,0);
          for(j=0;j<=FONTID_MODERN_MAX;j++) {
            SendDlgItemMessage(hwndDlg,IDC_FONTID,CB_GETLBTEXT,j,(LPARAM)szText);
            id=SendDlgItemMessage(hwndDlg,IDC_FONTID,CB_GETITEMDATA,j,0);
            if(id==i) continue;
            itemId=SendDlgItemMessage(hwndDlg,IDC_SAMEAS,CB_ADDSTRING,0,(LPARAM)szText);
            SendDlgItemMessage(hwndDlg,IDC_SAMEAS,CB_SETITEMDATA,itemId,id);
            if(id==fontSettings[i].sameAs)
              SendDlgItemMessage(hwndDlg,IDC_SAMEAS,CB_SETCURSEL,itemId,0);
          }
          }
          SendMessage(hwndDlg,M_LOADFONT,i,0);
          SendMessage(hwndDlg,M_REFRESHSAMEASBOXES,i,0);
          SendMessage(hwndDlg,M_REMAKESAMPLE,0,0);
          break;
          }
        case M_SETSAMEASBOXES:	//set the check mark in the 'same as' boxes to the right value for fontid wParam
          CheckDlgButton(hwndDlg,IDC_SAMETYPE,fontSettings[wParam].sameAsFlags&SAMEASF_FACE?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_SAMESIZE,fontSettings[wParam].sameAsFlags&SAMEASF_SIZE?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_SAMESTYLE,fontSettings[wParam].sameAsFlags&SAMEASF_STYLE?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_SAMECOLOUR,fontSettings[wParam].sameAsFlags&SAMEASF_COLOUR?BST_CHECKED:BST_UNCHECKED);
		  CheckDlgButton(hwndDlg,IDC_SAMEEFFECT,fontSettings[wParam].sameAsFlags&SAMEASF_EFFECT?BST_CHECKED:BST_UNCHECKED);
          break;
        case M_FILLSCRIPTCOMBO:		  //fill the script combo box and set the selection to the value for fontid wParam
          {	LOGFONTA lf={0};
          int i;
          HDC hdc=GetDC(hwndDlg);
          lf.lfCharSet=DEFAULT_CHARSET;
          GetDlgItemTextA(hwndDlg,IDC_TYPEFACE,lf.lfFaceName,sizeof(lf.lfFaceName));
          lf.lfPitchAndFamily=0;
          SendDlgItemMessage(hwndDlg,IDC_SCRIPT,CB_RESETCONTENT,0,0);
          EnumFontFamiliesExA(hdc,&lf,(FONTENUMPROCA)EnumFontScriptsProc,(LPARAM)GetDlgItem(hwndDlg,IDC_SCRIPT),0);
          ReleaseDC(hwndDlg,hdc);
          for(i=SendDlgItemMessage(hwndDlg,IDC_SCRIPT,CB_GETCOUNT,0,0)-1;i>=0;i--) {
            if(SendDlgItemMessage(hwndDlg,IDC_SCRIPT,CB_GETITEMDATA,i,0)==fontSettings[wParam].charset) {
              SendDlgItemMessage(hwndDlg,IDC_SCRIPT,CB_SETCURSEL,i,0);
              break;
            }
          }
          if(i<0) SendDlgItemMessage(hwndDlg,IDC_SCRIPT,CB_SETCURSEL,0,0);
          break;
          }
        case WM_CTLCOLORSTATIC:
          if((HWND)lParam==GetDlgItem(hwndDlg,IDC_SAMPLE)) {
            SetTextColor((HDC)wParam,SendDlgItemMessage(hwndDlg,IDC_COLOUR,CPM_GETCOLOUR,0,0));
            SetBkColor((HDC)wParam,GetSysColor(COLOR_3DFACE));
            return (BOOL)GetSysColorBrush(COLOR_3DFACE);
          }
          break;
        case M_REFRESHSAMEASBOXES:		  //set the disabled flag on the 'same as' checkboxes to the values for fontid wParam
          EnableWindow(GetDlgItem(hwndDlg,IDC_SAMETYPE),fontSettings[wParam].sameAs!=0xFF);
          EnableWindow(GetDlgItem(hwndDlg,IDC_SAMESIZE),fontSettings[wParam].sameAs!=0xFF);
          EnableWindow(GetDlgItem(hwndDlg,IDC_SAMESTYLE),fontSettings[wParam].sameAs!=0xFF);
          EnableWindow(GetDlgItem(hwndDlg,IDC_SAMECOLOUR),fontSettings[wParam].sameAs!=0xFF);
		  EnableWindow(GetDlgItem(hwndDlg,IDC_SAMEEFFECT),fontSettings[wParam].sameAs!=0xFF);
          if(SendMessage(GetParent(hwndDlg),PSM_ISEXPERT,0,0)) {
            EnableWindow(GetDlgItem(hwndDlg,IDC_TYPEFACE),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_FACE));
            EnableWindow(GetDlgItem(hwndDlg,IDC_SCRIPT),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_FACE));
            EnableWindow(GetDlgItem(hwndDlg,IDC_FONTSIZE),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_SIZE));
            EnableWindow(GetDlgItem(hwndDlg,IDC_BOLD),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_STYLE));
            EnableWindow(GetDlgItem(hwndDlg,IDC_ITALIC),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_STYLE));
            EnableWindow(GetDlgItem(hwndDlg,IDC_UNDERLINE),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_STYLE));
            EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR),fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_COLOUR));
			{
				BOOL Ena=fontSettings[wParam].sameAs==0xFF || !(fontSettings[wParam].sameAsFlags&SAMEASF_EFFECT);
				EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_NAME),Ena);
				EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR1),Ena);
				EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR2),Ena);
				EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_TEXT1),Ena);
				EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_TEXT2),Ena);
				EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_SPIN1),Ena);
				EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_SPIN2),Ena);
			}
          }
          else {
            EnableWindow(GetDlgItem(hwndDlg,IDC_TYPEFACE),TRUE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_SCRIPT),TRUE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_FONTSIZE),TRUE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_BOLD),TRUE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_ITALIC),TRUE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_UNDERLINE),TRUE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR),TRUE);
          }
          break;
        case M_REMAKESAMPLE:	//remake the sample edit box font based on the settings in the controls
          {	LOGFONTA lf;
          if(hFontSample) {
            SendDlgItemMessage(hwndDlg,IDC_SAMPLE,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDC_FONTID,WM_GETFONT,0,0),0);
            DeleteObject(hFontSample);
          }
          lf.lfHeight=GetDlgItemInt(hwndDlg,IDC_FONTSIZE,NULL,FALSE);			
          {
            HDC hdc=GetDC(NULL);				
            lf.lfHeight=-MulDiv(lf.lfHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);
            ReleaseDC(NULL,hdc);				
          }

          lf.lfWidth=lf.lfEscapement=lf.lfOrientation=0;
          lf.lfWeight=IsDlgButtonChecked(hwndDlg,IDC_BOLD)?FW_BOLD:FW_NORMAL;
          lf.lfItalic=IsDlgButtonChecked(hwndDlg,IDC_ITALIC);
          lf.lfUnderline=IsDlgButtonChecked(hwndDlg,IDC_UNDERLINE);
          lf.lfStrikeOut=0;
          lf.lfCharSet=(BYTE)SendDlgItemMessage(hwndDlg,IDC_SCRIPT,CB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_SCRIPT,CB_GETCURSEL,0,0),DEFAULT_CHARSET);
          lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
          lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;
          lf.lfQuality=DEFAULT_QUALITY;
          lf.lfPitchAndFamily=DEFAULT_PITCH|FF_DONTCARE;
          GetDlgItemTextA(hwndDlg,IDC_TYPEFACE,lf.lfFaceName,sizeof(lf.lfFaceName));
          hFontSample=CreateFontIndirectA(&lf);
          SendDlgItemMessageA(hwndDlg,IDC_SAMPLE,WM_SETFONT,(WPARAM)hFontSample,TRUE);
          EffectSample=(BYTE)SendDlgItemMessage(hwndDlg,IDC_EFFECT_NAME,CB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_EFFECT_NAME,CB_GETCURSEL,0,0),0);;
          ColorSample=SendDlgItemMessage(hwndDlg,IDC_COLOUR,CPM_GETCOLOUR,0,0);;
          Color1Sample=SendDlgItemMessage(hwndDlg,IDC_EFFECT_COLOUR1,CPM_GETCOLOUR,0,0)|((~(BYTE)SendDlgItemMessage(hwndDlg,IDC_EFFECT_COLOUR_SPIN1,UDM_GETPOS,0,0))<<24);
          Color2Sample=SendDlgItemMessage(hwndDlg,IDC_EFFECT_COLOUR2,CPM_GETCOLOUR,0,0)|((~(BYTE)SendDlgItemMessage(hwndDlg,IDC_EFFECT_COLOUR_SPIN2,UDM_GETPOS,0,0))<<24);;
          InvalidateRect(GetDlgItem(hwndDlg,IDC_PREVIEW),NULL,FALSE);
          break;
          }
        case M_RECALCONEFONT:	   //copy the 'same as' settings for fontid wParam from their sources
          if(fontSettings[wParam].sameAs==0xFF) break;
          if(fontSettings[wParam].sameAsFlags&SAMEASF_FACE) {
            lstrcpyA(fontSettings[wParam].szFace,fontSettings[fontSettings[wParam].sameAs].szFace);
            fontSettings[wParam].charset=fontSettings[fontSettings[wParam].sameAs].charset;
          }
          if(fontSettings[wParam].sameAsFlags&SAMEASF_SIZE)
            fontSettings[wParam].size=fontSettings[fontSettings[wParam].sameAs].size;
          if(fontSettings[wParam].sameAsFlags&SAMEASF_STYLE)
            fontSettings[wParam].style=fontSettings[fontSettings[wParam].sameAs].style;
          if(fontSettings[wParam].sameAsFlags&SAMEASF_COLOUR)
            fontSettings[wParam].colour=fontSettings[fontSettings[wParam].sameAs].colour;
		  if(fontSettings[wParam].sameAsFlags&SAMEASF_EFFECT)
		  {
            fontSettings[wParam].Effect=fontSettings[fontSettings[wParam].sameAs].Effect;
			fontSettings[wParam].EffectColor1=fontSettings[fontSettings[wParam].sameAs].EffectColor1;
			fontSettings[wParam].EffectColor2=fontSettings[fontSettings[wParam].sameAs].EffectColor2;
		  }			  
          break;
        case M_RECALCOTHERFONTS:	//recalculate the 'same as' settings for all fonts but wParam
          {	int i;
          for(i=0;i<=FONTID_MODERN_MAX;i++) {
            if(i==(int)wParam) continue;
            SendMessage(hwndDlg,M_RECALCONEFONT,i,0);
          }
          break;
          }
        case M_SAVEFONT:	//save the font settings from the controls to font wParam
          fontSettings[wParam].sameAsFlags=
			   (IsDlgButtonChecked(hwndDlg,IDC_SAMETYPE)?SAMEASF_FACE:0)
			  |(IsDlgButtonChecked(hwndDlg,IDC_SAMESIZE)?SAMEASF_SIZE:0)
			  |(IsDlgButtonChecked(hwndDlg,IDC_SAMESTYLE)?SAMEASF_STYLE:0)
			  |(IsDlgButtonChecked(hwndDlg,IDC_SAMECOLOUR)?SAMEASF_COLOUR:0)
			  |(IsDlgButtonChecked(hwndDlg,IDC_SAMEEFFECT)?SAMEASF_EFFECT:0)
			  ;
          fontSettings[wParam].sameAs=(BYTE)SendDlgItemMessage(hwndDlg,IDC_SAMEAS,CB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_SAMEAS,CB_GETCURSEL,0,0),0);
          GetDlgItemTextA(hwndDlg,IDC_TYPEFACE,fontSettings[wParam].szFace,sizeof(fontSettings[wParam].szFace));
          fontSettings[wParam].charset=(BYTE)SendDlgItemMessage(hwndDlg,IDC_SCRIPT,CB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_SCRIPT,CB_GETCURSEL,0,0),0);
          fontSettings[wParam].size=(char)GetDlgItemInt(hwndDlg,IDC_FONTSIZE,NULL,FALSE);
          fontSettings[wParam].style=(IsDlgButtonChecked(hwndDlg,IDC_BOLD)?DBFONTF_BOLD:0)|(IsDlgButtonChecked(hwndDlg,IDC_ITALIC)?DBFONTF_ITALIC:0)|(IsDlgButtonChecked(hwndDlg,IDC_UNDERLINE)?DBFONTF_UNDERLINE:0);
          fontSettings[wParam].colour=SendDlgItemMessage(hwndDlg,IDC_COLOUR,CPM_GETCOLOUR,0,0);
          fontSettings[wParam].Effect=(BYTE)SendDlgItemMessage(hwndDlg,IDC_EFFECT_NAME,CB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_EFFECT_NAME,CB_GETCURSEL,0,0),0);
          fontSettings[wParam].EffectColor1=SendDlgItemMessage(hwndDlg,IDC_EFFECT_COLOUR1,CPM_GETCOLOUR,0,0)|((~(BYTE)SendDlgItemMessage(hwndDlg,IDC_EFFECT_COLOUR_SPIN1,UDM_GETPOS,0,0))<<24);
          fontSettings[wParam].EffectColor2=SendDlgItemMessage(hwndDlg,IDC_EFFECT_COLOUR2,CPM_GETCOLOUR,0,0)|((~(BYTE)SendDlgItemMessage(hwndDlg,IDC_EFFECT_COLOUR_SPIN2,UDM_GETPOS,0,0))<<24);

          //SendMessage(hwndDlg,M_REDOROWHEIGHT,0,0);

          if (fontSettings[wParam].Effect)
          {
            int i=0;
            int cnt=SendDlgItemMessage(hwndDlg,IDC_EFFECT_NAME,CB_GETCOUNT,0,0);
            for (i=0; i<cnt; i++)
              if (SendDlgItemMessage(hwndDlg,IDC_EFFECT_NAME,CB_GETITEMDATA,i,0)==fontSettings[wParam].Effect)
              {
                SendDlgItemMessage(hwndDlg,IDC_EFFECT_NAME,CB_SETCURSEL,i,0);
                break;
              }
            EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR1),TRUE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR2),TRUE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_SPIN1),TRUE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_SPIN2),TRUE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_TEXT1),TRUE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_TEXT2),TRUE);
          }
          else
          {
            EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR1),FALSE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR2),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_SPIN1),FALSE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_SPIN2),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_TEXT1),FALSE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_TEXT2),FALSE);
          }
  
          break;
          /*		case M_REDOROWHEIGHT:	//recalculate the minimum feasible row height
          {	
          int i;
          int minHeight=1;//GetSystemMetrics(SM_CYSMICON) +1;
          int t;
          t=IsDlgButtonChecked(hwndDlg,IDC_NOTCHECKFONTSIZE);
          if (t) 
          {
          SendDlgItemMessage(hwndDlg,IDC_ROWHEIGHTSPIN,UDM_SETRANGE,0,MAKELONG(255,1));
          break;
          }
          for(i=0;i<=FONTID_MODERN_MAX;i++)
          {	
          SIZE fontSize;
          HFONT hFont, oldfnt; 
          HDC hdc=GetDC(NULL);
          LOGFONTA lf;
          lf.lfHeight=fontSettings[i].size;			
          {
          HDC hdc=GetDC(NULL);				
          lf.lfHeight=-MulDiv(lf.lfHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);
          ReleaseDC(NULL,hdc);				
          }

          lf.lfWidth=lf.lfEscapement=lf.lfOrientation=0;
          lf.lfWeight=(fontSettings[i].style&DBFONTF_BOLD)?FW_BOLD:FW_NORMAL;
          lf.lfItalic=fontSettings[i].style&DBFONTF_ITALIC;
          lf.lfUnderline=fontSettings[i].style&DBFONTF_UNDERLINE;
          lf.lfStrikeOut=0;
          lf.lfCharSet=(BYTE)fontSettings[i].charset;
          lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
          lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;
          lf.lfQuality=DEFAULT_QUALITY;
          lf.lfPitchAndFamily=DEFAULT_PITCH|FF_DONTCARE;
          strcpy(lf.lfFaceName,fontSettings[i].szFace);

          hFont=CreateFontIndirect(&lf);
          oldfnt=(HFONT)SelectObject(hdc,(HFONT)hFont);
          GetTextExtentPoint32A(hdc,"x",1,&fontSize);
          if(fontSize.cy>minHeight) minHeight=fontSize.cy;
          SelectObject(hdc,oldfnt);
          DeleteObject(hFont);
          ReleaseDC(NULL,hdc);


          }
          i=SendDlgItemMessage(hwndDlg,IDC_ROWHEIGHTSPIN,UDM_GETPOS,0,0);
          if(i<minHeight) SendDlgItemMessage(hwndDlg,IDC_ROWHEIGHTSPIN,UDM_SETPOS,0,MAKELONG(minHeight,0));

          SendDlgItemMessage(hwndDlg,IDC_ROWHEIGHTSPIN,UDM_SETRANGE,0,MAKELONG(255,minHeight));
          break;
          }
          */
        case M_LOADFONT:	//load font wParam into the controls
          SetDlgItemTextA(hwndDlg,IDC_TYPEFACE,fontSettings[wParam].szFace);
          SendMessage(hwndDlg,M_FILLSCRIPTCOMBO,wParam,0);
          SetDlgItemInt(hwndDlg,IDC_FONTSIZE,fontSettings[wParam].size,FALSE);
          CheckDlgButton(hwndDlg,IDC_BOLD,fontSettings[wParam].style&DBFONTF_BOLD?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_ITALIC,fontSettings[wParam].style&DBFONTF_ITALIC?BST_CHECKED:BST_UNCHECKED);
          CheckDlgButton(hwndDlg,IDC_UNDERLINE,fontSettings[wParam].style&DBFONTF_UNDERLINE?BST_CHECKED:BST_UNCHECKED);
          if (fontSettings[wParam].Effect)// && !(fontSettings[wParam].sameAsFlags&SAMEASF_EFFECT && fontSettings[wParam].sameAs!=0xFF))
          {
            int i=0;
            int cnt=SendDlgItemMessage(hwndDlg,IDC_EFFECT_NAME,CB_GETCOUNT,0,0);
            for (i=0; i<cnt; i++)
              if (SendDlgItemMessage(hwndDlg,IDC_EFFECT_NAME,CB_GETITEMDATA,i,0)==fontSettings[wParam].Effect)
              {
                SendDlgItemMessage(hwndDlg,IDC_EFFECT_NAME,CB_SETCURSEL,i,0);
                break;
              }
            EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR1),TRUE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR2),TRUE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_SPIN1),TRUE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_SPIN2),TRUE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_TEXT1),TRUE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_TEXT2),TRUE);
            SendDlgItemMessage(hwndDlg,IDC_EFFECT_COLOUR1,CPM_SETCOLOUR,0,fontSettings[wParam].EffectColor1&0x00FFFFFF);
            SendDlgItemMessage(hwndDlg,IDC_EFFECT_COLOUR2,CPM_SETCOLOUR,0,fontSettings[wParam].EffectColor2&0x00FFFFFF);
			SendDlgItemMessage(hwndDlg,IDC_EFFECT_COLOUR_SPIN1,UDM_SETPOS,0,MAKELONG((BYTE)~((BYTE)((fontSettings[wParam].EffectColor1&0xFF000000)>>24)),0));
			SendDlgItemMessage(hwndDlg,IDC_EFFECT_COLOUR_SPIN2,UDM_SETPOS,0,MAKELONG((BYTE)~((BYTE)((fontSettings[wParam].EffectColor2&0xFF000000)>>24)),0));
          }
          else
          {
            SendDlgItemMessage(hwndDlg,IDC_EFFECT_NAME,CB_SETCURSEL,0,0);
            EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR1),FALSE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR2),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_SPIN1),FALSE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_SPIN2),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_TEXT1),FALSE);
            EnableWindow(GetDlgItem(hwndDlg,IDC_EFFECT_COLOUR_TEXT2),FALSE);
            SendDlgItemMessage(hwndDlg,IDC_EFFECT_COLOUR1,CPM_SETCOLOUR,0,fontSettings[wParam].EffectColor1&0x00FFFFFF);
            SendDlgItemMessage(hwndDlg,IDC_EFFECT_COLOUR2,CPM_SETCOLOUR,0,fontSettings[wParam].EffectColor2&0x00FFFFFF);
			SendDlgItemMessage(hwndDlg,IDC_EFFECT_COLOUR_SPIN1,UDM_SETPOS,0,MAKELONG((BYTE)~((BYTE)((fontSettings[wParam].EffectColor1&0xFF000000)>>24)),0));
			SendDlgItemMessage(hwndDlg,IDC_EFFECT_COLOUR_SPIN2,UDM_SETPOS,0,MAKELONG((BYTE)~((BYTE)((fontSettings[wParam].EffectColor2&0xFF000000)>>24)),0));

          }
          {	LOGFONTA lf;
          COLORREF colour;
          GetDefaultFontSetting(wParam,&lf,&colour);
          SendDlgItemMessage(hwndDlg,IDC_COLOUR,CPM_SETDEFAULTCOLOUR,0,colour);
          }
          SendDlgItemMessage(hwndDlg,IDC_COLOUR,CPM_SETCOLOUR,0,fontSettings[wParam].colour);
          break;
        case M_GUESSSAMEASBOXES:   //guess suitable values for the 'same as' checkboxes for fontId wParam
          fontSettings[wParam].sameAsFlags=0;
          if(fontSettings[wParam].sameAs==0xFF) break;
          if(!lstrcmpA(fontSettings[wParam].szFace,fontSettings[fontSettings[wParam].sameAs].szFace) &&
            fontSettings[wParam].charset==fontSettings[fontSettings[wParam].sameAs].charset)
            fontSettings[wParam].sameAsFlags|=SAMEASF_FACE;
          if(fontSettings[wParam].size==fontSettings[fontSettings[wParam].sameAs].size)
            fontSettings[wParam].sameAsFlags|=SAMEASF_SIZE;
          if(fontSettings[wParam].style==fontSettings[fontSettings[wParam].sameAs].style)
            fontSettings[wParam].sameAsFlags|=SAMEASF_STYLE;
          if(fontSettings[wParam].colour==fontSettings[fontSettings[wParam].sameAs].colour)
            fontSettings[wParam].sameAsFlags|=SAMEASF_COLOUR;
		  if((fontSettings[wParam].Effect==fontSettings[fontSettings[wParam].sameAs].Effect)
			  &&(fontSettings[wParam].EffectColor1==fontSettings[fontSettings[wParam].sameAs].EffectColor1)
			  &&(fontSettings[wParam].EffectColor2==fontSettings[fontSettings[wParam].sameAs].EffectColor2) )
			  fontSettings[wParam].sameAsFlags|=SAMEASF_EFFECT;
          SendMessage(hwndDlg,M_SETSAMEASBOXES,wParam,0);
          break;
        case WM_VSCROLL:
          SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
          break;
        case WM_COMMAND:
          {	int fontId=SendDlgItemMessage(hwndDlg,IDC_FONTID,CB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_FONTID,CB_GETCURSEL,0,0),0);
          switch(LOWORD(wParam)) {
        case IDC_FONTID:
          if(HIWORD(wParam)!=CBN_SELCHANGE) return FALSE;
          SendMessage(hwndDlg,M_REBUILDFONTGROUP,0,0);
          return 0;
        case IDC_SAMETYPE:
        case IDC_SAMESIZE:
        case IDC_SAMESTYLE:
        case IDC_SAMECOLOUR:
		case IDC_SAMEEFFECT:
          SendMessage(hwndDlg,M_SAVEFONT,fontId,0);
          SendMessage(hwndDlg,M_RECALCONEFONT,fontId,0);
		  SendMessage(hwndDlg,M_REFRESHSAMEASBOXES,fontId,0);
		  SendMessage(hwndDlg,M_LOADFONT,fontId,0);
          SendMessage(hwndDlg,M_REMAKESAMPLE,0,0);          
          break;
        case IDC_SAMEAS:
          if(HIWORD(wParam)!=CBN_SELCHANGE) return FALSE;
          if(SendDlgItemMessage(hwndDlg,IDC_SAMEAS,CB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_SAMEAS,CB_GETCURSEL,0,0),0)==fontId)
            SendDlgItemMessage(hwndDlg,IDC_SAMEAS,CB_SETCURSEL,0,0);
          if(!SendMessage(GetParent(hwndDlg),PSM_ISEXPERT,0,0)) {
            //			int sameAs=SendDlgItemMessage(hwndDlg,IDC_SAMEAS,CB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_SAMEAS,CB_GETCURSEL,0,0),0);
            //			if(sameAs!=0xFF) SendMessage(hwndDlg,M_LOADFONT,sameAs,0);
            SendMessage(hwndDlg,M_SAVEFONT,fontId,0);
            SendMessage(hwndDlg,M_GUESSSAMEASBOXES,fontId,0);
          }
          else SendMessage(hwndDlg,M_SAVEFONT,fontId,0);
          SendMessage(hwndDlg,M_RECALCONEFONT,fontId,0);
          SendMessage(hwndDlg,M_LOADFONT,fontId,0);
          SendMessage(hwndDlg,M_FILLSCRIPTCOMBO,fontId,0);
          SendMessage(hwndDlg,M_REMAKESAMPLE,0,0);
          SendMessage(hwndDlg,M_REFRESHSAMEASBOXES,fontId,0);
          break;
        case IDC_TYPEFACE:
        case IDC_SCRIPT:
        case IDC_FONTSIZE:
        case IDC_EFFECT_NAME:

          if(HIWORD(wParam)!=CBN_EDITCHANGE && HIWORD(wParam)!=CBN_SELCHANGE) return FALSE;
          if(HIWORD(wParam)==CBN_SELCHANGE) {
            SendDlgItemMessage(hwndDlg,LOWORD(wParam),CB_SETCURSEL,SendDlgItemMessage(hwndDlg,LOWORD(wParam),CB_GETCURSEL,0,0),0);
          }
          if(LOWORD(wParam)==IDC_TYPEFACE)
            SendMessage(hwndDlg,M_FILLSCRIPTCOMBO,fontId,0);
          //fall through
		case IDC_EFFECT_COLOUR_TEXT1:
        case IDC_EFFECT_COLOUR_TEXT2:
			{
				if ((LOWORD(wParam)==IDC_EFFECT_COLOUR_TEXT1||LOWORD(wParam)==IDC_EFFECT_COLOUR_TEXT2)
					&&(HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))
					return 0;
			}
        case IDC_BOLD:
        case IDC_ITALIC:
        case IDC_UNDERLINE:
        case IDC_COLOUR:
        case IDC_EFFECT_COLOUR1:
        case IDC_EFFECT_COLOUR2:

          SendMessage(hwndDlg,M_SAVEFONT,fontId,0);
          if(!SendMessage(GetParent(hwndDlg),PSM_ISEXPERT,0,0)) {
            SendMessage(hwndDlg,M_GUESSSAMEASBOXES,fontId,0);
            SendMessage(hwndDlg,M_REFRESHSAMEASBOXES,fontId,0);
          }
          SendMessage(hwndDlg,M_RECALCOTHERFONTS,fontId,0);
          SendMessage(hwndDlg,M_REMAKESAMPLE,0,0);
          //SendMessage(hwndDlg,M_REDOROWHEIGHT,0,0);
          break;
          /*				case IDC_NOTCHECKFONTSIZE:
          SendMessage(hwndDlg,M_REDOROWHEIGHT,0,0);
          break;
          */
        case IDC_SAMPLE:
          return 0;
          /*				case IDC_ROWHEIGHT:
          if(HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()) return 0;
          break;
          */			}
          SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
          break;
          }
        case WM_DRAWITEM:
          if (wParam==IDC_PREVIEW)
          {
            LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;
            HBRUSH hBrush=CreateSolidBrush(GetSysColor(COLOR_3DFACE));
            HDC hdc=CreateCompatibleDC(dis->hDC);
            HBITMAP hbmp=SkinEngine_CreateDIB32(dis->rcItem.right-dis->rcItem.left,dis->rcItem.bottom-dis->rcItem.top);
            HBITMAP obmp=SelectObject(hdc,hbmp);
            HFONT oldFnt=SelectObject(hdc,hFontSample);
            RECT rc={0};
            rc.right=dis->rcItem.right-dis->rcItem.left;
            rc.bottom=dis->rcItem.bottom-dis->rcItem.top;
            FillRect(hdc,&rc,hBrush);
            SkinEngine_SetRectOpaque(hdc,&rc);
            SetTextColor(hdc,ColorSample);
              SkinEngine_SelectTextEffect(hdc,EffectSample-1,Color1Sample,Color2Sample);
              SkinEngine_DrawText(hdc,TranslateT("Sample"),lstrlen(TranslateT("Sample")),&rc,DT_CENTER|DT_VCENTER);
              BitBlt(dis->hDC,dis->rcItem.left,dis->rcItem.top,rc.right,rc.bottom,hdc,0,0,SRCCOPY);
            SelectObject(hdc,obmp);
            SelectObject(hdc,oldFnt);
            DeleteObject(hbmp);
            DeleteObject(hBrush);
            mod_DeleteDC(hdc);            
          }
          break;
        case WM_NOTIFY:
          switch(((LPNMHDR)lParam)->idFrom) {
        case 0:
          switch (((LPNMHDR)lParam)->code)
          {
          case PSN_APPLY:
            {	int i;
            char str[20];

            for(i=0;i<=FONTID_MODERN_MAX;i++) {
              mir_snprintf(str,sizeof(str),"Font%dName",i);
              DBWriteContactSettingString(NULL,"CLC",str,fontSettings[i].szFace);
              mir_snprintf(str,sizeof(str),"Font%dSet",i);
              DBWriteContactSettingByte(NULL,"CLC",str,fontSettings[i].charset);
              mir_snprintf(str,sizeof(str),"Font%dSize",i);
              DBWriteContactSettingByte(NULL,"CLC",str,fontSettings[i].size);
              mir_snprintf(str,sizeof(str),"Font%dSty",i);
              DBWriteContactSettingByte(NULL,"CLC",str,fontSettings[i].style);
              mir_snprintf(str,sizeof(str),"Font%dCol",i);
              DBWriteContactSettingDword(NULL,"CLC",str,fontSettings[i].colour);

              mir_snprintf(str,sizeof(str),"Font%dEffect",i);
              DBWriteContactSettingByte(NULL,"CLC",str,fontSettings[i].Effect);
              mir_snprintf(str,sizeof(str),"Font%dEffectCol1",i);
              DBWriteContactSettingDword(NULL,"CLC",str,fontSettings[i].EffectColor1);
              mir_snprintf(str,sizeof(str),"Font%dEffectCol2",i);
              DBWriteContactSettingDword(NULL,"CLC",str,fontSettings[i].EffectColor2);

              mir_snprintf(str,sizeof(str),"Font%dAs",i);
              DBWriteContactSettingWord(NULL,"CLC",str,(WORD)((fontSettings[i].sameAsFlags<<8)|fontSettings[i].sameAs));
            }
            }
            {	COLORREF col;
            col=SendDlgItemMessage(hwndDlg,IDC_SELCOLOUR,CPM_GETCOLOUR,0,0);
            if(col==CLCDEFAULT_SELTEXTCOLOUR) DBDeleteContactSetting(NULL,"CLC","SelTextColour");
            else DBWriteContactSettingDword(NULL,"CLC","SelTextColour",col);
            col=SendDlgItemMessage(hwndDlg,IDC_HOTCOLOUR,CPM_GETCOLOUR,0,0);
            if(col==CLCDEFAULT_HOTTEXTCOLOUR) DBDeleteContactSetting(NULL,"CLC","HotTextColour");
            else DBWriteContactSettingDword(NULL,"CLC","HotTextColour",col);
            col=SendDlgItemMessage(hwndDlg,IDC_QUICKCOLOUR,CPM_GETCOLOUR,0,0);
            if(col==CLCDEFAULT_QUICKSEARCHCOLOUR) DBDeleteContactSetting(NULL,"CLC","QuickSearchColour");
            else DBWriteContactSettingDword(NULL,"CLC","QuickSearchColour",col);
            }
            //DBWriteContactSettingByte(NULL,"CLC","RowHeight",(BYTE)SendDlgItemMessage(hwndDlg,IDC_ROWHEIGHTSPIN,UDM_GETPOS,0,0));
            DBWriteContactSettingByte(NULL,"CLC","GammaCorrect",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_GAMMACORRECT));
            //DBWriteContactSettingByte(NULL,"CLC","DoNotCheckFontSize",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_NOTCHECKFONTSIZE));	
            {
              int hil=0;
              if (IsDlgButtonChecked(hwndDlg,IDC_HILIGHTMODE1))  hil=1;
              if (IsDlgButtonChecked(hwndDlg,IDC_HILIGHTMODE2))  hil=2;
              if (IsDlgButtonChecked(hwndDlg,IDC_HILIGHTMODE3))  hil=3;

              DBWriteContactSettingByte(NULL,"CLC","HiLightMode",(BYTE)hil);

            }	

            ClcOptionsChanged();
            return TRUE;
          case PSN_EXPERTCHANGED:
            SwitchTextDlgToMode(hwndDlg,((PSHNOTIFY*)lParam)->lParam);
            break;
          }
          break;
          }
          break;
        case WM_DESTROY:
          if(hFontSample) {
            SendDlgItemMessage(hwndDlg,IDC_SAMPLE,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDC_FONTID,WM_GETFONT,0,0),0);
            DeleteObject(hFontSample);
          }
          break;
        }
        return FALSE;
      }