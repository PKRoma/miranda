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

extern HANDLE hMessageWindowList,hMessageSendList;

#define FONTF_BOLD   1
#define FONTF_ITALIC 2
struct FontOptionsList {
	char *szDescr;
	COLORREF defColour;
	char *szDefFace;
	BYTE defCharset,defStyle;
	char defSize;
	COLORREF colour;
	char szFace[LF_FACESIZE];
	BYTE charset,style;
	char size;
} static fontOptionsList[]={
	{"My messages",   RGB(240,0,0),     "Arial", DEFAULT_CHARSET, 0, -14},
	{"My URLs",       RGB(0,0,0),		"Arial", DEFAULT_CHARSET, 0, -14},
	{"My files",      RGB(0,0,0),		"Arial", DEFAULT_CHARSET, 0, -14},
	{"Your messages", RGB(64,0,128),	"Arial", DEFAULT_CHARSET, 0, -14},
	{"Your URLs",     RGB(0,0,0),		"Arial", DEFAULT_CHARSET, 0, -14},
	{"Your files",    RGB(0,0,0),		"Arial", DEFAULT_CHARSET, 0, -14},
	{"My name",       RGB(0,0,0),		"Arial", DEFAULT_CHARSET, 0, -14},
	{"My time",       RGB(0,0,0),		"Arial", DEFAULT_CHARSET, 0, -14},
	{"My colon",      RGB(0,0,0),		"Arial", DEFAULT_CHARSET, 0, -14},
	{"Your name",     RGB(80,80,80),	"Arial", DEFAULT_CHARSET, 0, -14},
	{"Your time",     RGB(80,80,80),	"Arial", DEFAULT_CHARSET, 0, -14},
	{"Your colon",    RGB(80,80,80),	"Arial", DEFAULT_CHARSET, 0, -14},
};
const int msgDlgFontCount=sizeof(fontOptionsList)/sizeof(fontOptionsList[0]);

static HWND hwndMainOptsDlg,hwndLogOptsDlg;
#define M_UPDATEPRESETBUTTONS   (WM_USER+0x100)

struct PresetInfo_t {
	UINT id;
	int set;
} static const splitPreset[]={
	{IDC_SHOWREADNEXT,0},
	{IDC_SPLIT,1},
	{IDC_AUTOCLOSE,0},
	{IDC_SHOWSENT,1},
};

void LoadMsgDlgFont(int i,LOGFONT *lf,COLORREF *colour)
{
	char str[32];
	int style;
	DBVARIANT dbv;

	if(colour) {
		wsprintf(str,"Font%dCol",i);
		*colour=DBGetContactSettingDword(NULL,"SRMsg",str,fontOptionsList[i].defColour);
	}
	if(lf) {
		wsprintf(str,"Font%dSize",i);
		lf->lfHeight=(char)DBGetContactSettingByte(NULL,"SRMsg",str,fontOptionsList[i].defSize);
		lf->lfWidth=0;
		lf->lfEscapement=0;
		lf->lfOrientation=0;
		wsprintf(str,"Font%dSty",i);
		style=DBGetContactSettingByte(NULL,"SRMsg",str,fontOptionsList[i].defStyle);
		lf->lfWeight=style&FONTF_BOLD?FW_BOLD:FW_NORMAL;
		lf->lfItalic=style&FONTF_ITALIC?1:0;
		lf->lfUnderline=0;
		lf->lfStrikeOut=0;
		wsprintf(str,"Font%dSet",i);
		lf->lfCharSet=DBGetContactSettingByte(NULL,"SRMsg",str,fontOptionsList[i].defCharset);
		lf->lfOutPrecision=OUT_DEFAULT_PRECIS;
		lf->lfClipPrecision=CLIP_DEFAULT_PRECIS;
		lf->lfQuality=DEFAULT_QUALITY;
		lf->lfPitchAndFamily=DEFAULT_PITCH|FF_DONTCARE;
		wsprintf(str,"Font%d",i);
		if(DBGetContactSetting(NULL,"SRMsg",str,&dbv))
			lstrcpy(lf->lfFaceName,fontOptionsList[i].szDefFace);
		else {
			lstrcpyn(lf->lfFaceName,dbv.pszVal,sizeof(lf->lfFaceName));
			DBFreeVariant(&dbv);
		}
	}
}

static BOOL CALLBACK DlgProcOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
		case WM_INITDIALOG:
			hwndMainOptsDlg=hwndDlg;
			TranslateDialogDefault(hwndDlg);
			CheckDlgButton(hwndDlg,IDC_SPLIT,DBGetContactSettingByte(NULL,"SRMsg","Split",SRMSGDEFSET_SPLIT));
			CheckDlgButton(hwndDlg,IDC_SHOWBUTTONLINE,DBGetContactSettingByte(NULL,"SRMsg","ShowButtonLine",SRMSGDEFSET_SHOWBUTTONLINE));
			CheckDlgButton(hwndDlg,IDC_SHOWINFOLINE,DBGetContactSettingByte(NULL,"SRMsg","ShowInfoLine",SRMSGDEFSET_SHOWINFOLINE));
			CheckDlgButton(hwndDlg,IDC_SHOWQUOTE,DBGetContactSettingByte(NULL,"SRMsg","ShowQuote",SRMSGDEFSET_SHOWQUOTE));
			CheckDlgButton(hwndDlg,IDC_AUTOPOPUP,DBGetContactSettingByte(NULL,"SRMsg","AutoPopup",SRMSGDEFSET_AUTOPOPUP));
			//CheckDlgButton(hwndDlg,IDC_POPUPMIN,DBGetContactSettingByte(NULL,"SRMsg","PopupMin",SRMSGDEFSET_POPUPMIN));
			//EnableWindow(GetDlgItem(hwndDlg,IDC_POPUPMIN),IsDlgButtonChecked(hwndDlg,IDC_AUTOPOPUP));
			CheckDlgButton(hwndDlg,IDC_AUTOMIN,DBGetContactSettingByte(NULL,"SRMsg","AutoMin",SRMSGDEFSET_AUTOMIN));
			CheckDlgButton(hwndDlg,IDC_AUTOCLOSE,DBGetContactSettingByte(NULL,"SRMsg","AutoClose",SRMSGDEFSET_AUTOCLOSE));
			CheckDlgButton(hwndDlg,IDC_CLOSEONREPLY,DBGetContactSettingByte(NULL,"SRMsg","CloseOnReply",SRMSGDEFSET_CLOSEONREPLY));
			CheckDlgButton(hwndDlg,IDC_SAVEPERCONTACT,DBGetContactSettingByte(NULL,"SRMsg","SavePerContact",SRMSGDEFSET_SAVEPERCONTACT));
			CheckDlgButton(hwndDlg,IDC_CASCADE,DBGetContactSettingByte(NULL,"SRMsg","Cascade",SRMSGDEFSET_CASCADE));
			CheckDlgButton(hwndDlg,IDC_SENDONENTER,DBGetContactSettingByte(NULL,"SRMsg","SendOnEnter",SRMSGDEFSET_SENDONENTER));
			CheckDlgButton(hwndDlg,IDC_SENDONDBLENTER,DBGetContactSettingByte(NULL,"SRMsg","SendOnDblEnter",SRMSGDEFSET_SENDONDBLENTER));
			EnableWindow(GetDlgItem(hwndDlg,IDC_CLOSEONREPLY),!IsDlgButtonChecked(hwndDlg,IDC_SPLIT));
			EnableWindow(GetDlgItem(hwndDlg,IDC_CASCADE),!IsDlgButtonChecked(hwndDlg,IDC_SAVEPERCONTACT));

			CheckDlgButton(hwndDlg,IDC_SHOWREADNEXT,DBGetContactSettingByte(NULL,"SRMsg","ShowReadNext",SRMSGDEFSET_SHOWREADNEXT));
			CheckDlgButton(hwndDlg,IDC_SHOWSENT,DBGetContactSettingByte(NULL,"SRMsg","ShowSent",SRMSGDEFSET_SHOWSENT));
			SendMessage(hwndDlg,M_UPDATEPRESETBUTTONS,0,0);
			return TRUE;
		case M_UPDATEPRESETBUTTONS:
			if(hwndLogOptsDlg) {
				CheckDlgButton(hwndDlg,IDC_SHOWREADNEXT,IsDlgButtonChecked(hwndLogOptsDlg,IDC_SHOWREADNEXT));
				CheckDlgButton(hwndDlg,IDC_SHOWSENT,IsDlgButtonChecked(hwndLogOptsDlg,IDC_SHOWSENT));
			}
			{	int i;
				int alltrue=1,allfalse=1;
				for(i=0;i<sizeof(splitPreset)/sizeof(splitPreset[0]);i++) {
					if((IsDlgButtonChecked(hwndDlg,splitPreset[i].id)!=0)==(splitPreset[i].set!=0)) allfalse=0;
					else alltrue=0;
				}
				CheckDlgButton(hwndDlg,IDC_PRESETSPLIT,alltrue);
				CheckDlgButton(hwndDlg,IDC_PRESETSINGLE,allfalse);
			}
			break;
		case WM_COMMAND:
			{	int i;
				for(i=0;i<sizeof(splitPreset)/sizeof(splitPreset[0]);i++)
					if(LOWORD(wParam)==splitPreset[i].id)
						SendMessage(hwndDlg,M_UPDATEPRESETBUTTONS,0,0);
			}
			switch(LOWORD(wParam)) {
				case IDC_PRESETSPLIT:
				case IDC_PRESETSINGLE:
				{	int i;
					for(i=0;i<sizeof(splitPreset)/sizeof(splitPreset[0]);i++)
						CheckDlgButton(hwndDlg,splitPreset[i].id,LOWORD(wParam)==IDC_PRESETSPLIT?splitPreset[i].set:!splitPreset[i].set);
					EnableWindow(GetDlgItem(hwndDlg,IDC_CLOSEONREPLY),!IsDlgButtonChecked(hwndDlg,IDC_SPLIT));
					if(hwndLogOptsDlg) {
						CheckDlgButton(hwndLogOptsDlg,IDC_SHOWREADNEXT,IsDlgButtonChecked(hwndDlg,IDC_SHOWREADNEXT));
						CheckDlgButton(hwndLogOptsDlg,IDC_SHOWSENT,IsDlgButtonChecked(hwndDlg,IDC_SHOWSENT));
					}
					break;
				}
				case IDC_AUTOMIN:
					CheckDlgButton(hwndDlg,IDC_AUTOCLOSE,BST_UNCHECKED);
					break;
				case IDC_AUTOCLOSE:
					CheckDlgButton(hwndDlg,IDC_AUTOMIN,BST_UNCHECKED);
					break;
				case IDC_SENDONENTER:
					CheckDlgButton(hwndDlg,IDC_SENDONDBLENTER,BST_UNCHECKED);
					break;
				case IDC_SENDONDBLENTER:
					CheckDlgButton(hwndDlg,IDC_SENDONENTER,BST_UNCHECKED);
					break;
				case IDC_SPLIT:
					EnableWindow(GetDlgItem(hwndDlg,IDC_CLOSEONREPLY),!IsDlgButtonChecked(hwndDlg,IDC_SPLIT));
					break;
				case IDC_SAVEPERCONTACT:
					EnableWindow(GetDlgItem(hwndDlg,IDC_CASCADE),!IsDlgButtonChecked(hwndDlg,IDC_SAVEPERCONTACT));
					break;
				//case IDC_AUTOPOPUP:
				//	EnableWindow(GetDlgItem(hwndDlg,IDC_POPUPMIN),IsDlgButtonChecked(hwndDlg,IDC_AUTOPOPUP));
				//	break;
			}
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->idFrom) {
				case 0:
					switch (((LPNMHDR)lParam)->code)
					{
						case PSN_APPLY:
							DBWriteContactSettingByte(NULL,"SRMsg","Split",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SPLIT));
							DBWriteContactSettingByte(NULL,"SRMsg","ShowButtonLine",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWBUTTONLINE));
							DBWriteContactSettingByte(NULL,"SRMsg","ShowInfoLine",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWINFOLINE));
							DBWriteContactSettingByte(NULL,"SRMsg","ShowQuote",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWQUOTE));
							DBWriteContactSettingByte(NULL,"SRMsg","AutoPopup",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOPOPUP));
							//DBWriteContactSettingByte(NULL,"SRMsg","PopupMin",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_POPUPMIN));
							DBWriteContactSettingByte(NULL,"SRMsg","AutoMin",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOMIN));
							DBWriteContactSettingByte(NULL,"SRMsg","AutoClose",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOCLOSE));
							DBWriteContactSettingByte(NULL,"SRMsg","CloseOnReply",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_CLOSEONREPLY));
							DBWriteContactSettingByte(NULL,"SRMsg","SavePerContact",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SAVEPERCONTACT));
							DBWriteContactSettingByte(NULL,"SRMsg","Cascade",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_CASCADE));
							DBWriteContactSettingByte(NULL,"SRMsg","SendOnEnter",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SENDONENTER));
							DBWriteContactSettingByte(NULL,"SRMsg","SendOnDblEnter",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SENDONDBLENTER));
                           
							DBWriteContactSettingByte(NULL,"SRMsg","ShowReadNext",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWREADNEXT));
							DBWriteContactSettingByte(NULL,"SRMsg","ShowSent",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWSENT));

							WindowList_Broadcast(hMessageWindowList,DM_OPTIONSAPPLIED,0,0);
							WindowList_Broadcast(hMessageSendList,DM_OPTIONSAPPLIED,0,0);
							return TRUE;
					}
					break;
			}
			break;
		case WM_DESTROY:
			hwndMainOptsDlg=NULL;
			break;
	}
	return FALSE;
}

static BOOL CALLBACK DlgProcLogOptions(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HBRUSH hBkgColourBrush;

	switch(msg) {
		case WM_INITDIALOG:
			hwndLogOptsDlg=hwndDlg;
			TranslateDialogDefault(hwndDlg);
			switch(DBGetContactSettingByte(NULL,"SRMsg","LoadHistory",SRMSGDEFSET_LOADHISTORY)) {
				case LOADHISTORY_UNREAD:
					CheckDlgButton(hwndDlg,IDC_LOADUNREAD,BST_CHECKED);
					break;
				case LOADHISTORY_COUNT:
					CheckDlgButton(hwndDlg,IDC_LOADCOUNT,BST_CHECKED);
					EnableWindow(GetDlgItem(hwndDlg,IDC_LOADCOUNTN),TRUE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_LOADCOUNTSPIN),TRUE);
					break;
				case LOADHISTORY_TIME:
					CheckDlgButton(hwndDlg,IDC_LOADTIME,BST_CHECKED);
					EnableWindow(GetDlgItem(hwndDlg,IDC_LOADTIMEN),TRUE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_LOADTIMESPIN),TRUE);
					EnableWindow(GetDlgItem(hwndDlg,IDC_STMINSOLD),TRUE);
					break;
			}
			SendDlgItemMessage(hwndDlg,IDC_LOADCOUNTSPIN,UDM_SETRANGE,0,MAKELONG(100,0));
			SendDlgItemMessage(hwndDlg,IDC_LOADCOUNTSPIN,UDM_SETPOS,0,DBGetContactSettingWord(NULL,"SRMsg","LoadCount",SRMSGDEFSET_LOADCOUNT));
			SendDlgItemMessage(hwndDlg,IDC_LOADTIMESPIN,UDM_SETRANGE,0,MAKELONG(12*60,0));
			SendDlgItemMessage(hwndDlg,IDC_LOADTIMESPIN,UDM_SETPOS,0,DBGetContactSettingWord(NULL,"SRMsg","LoadTime",SRMSGDEFSET_LOADTIME));
                          
			CheckDlgButton(hwndDlg,IDC_SHOWLOGICONS,DBGetContactSettingByte(NULL,"SRMsg","ShowLogIcons",SRMSGDEFSET_SHOWLOGICONS));
			CheckDlgButton(hwndDlg,IDC_SHOWNAMES,!DBGetContactSettingByte(NULL,"SRMsg","HideNames",SRMSGDEFSET_HIDENAMES));
			CheckDlgButton(hwndDlg,IDC_SHOWTIMES,DBGetContactSettingByte(NULL,"SRMsg","ShowTime",SRMSGDEFSET_SHOWTIME));
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWDATES),IsDlgButtonChecked(hwndDlg,IDC_SHOWTIMES));
			CheckDlgButton(hwndDlg,IDC_SHOWDATES,DBGetContactSettingByte(NULL,"SRMsg","ShowDate",SRMSGDEFSET_SHOWDATE));
			if(hwndMainOptsDlg) {
				CheckDlgButton(hwndDlg,IDC_SHOWREADNEXT,IsDlgButtonChecked(hwndMainOptsDlg,IDC_SHOWREADNEXT));
				CheckDlgButton(hwndDlg,IDC_SHOWSENT,IsDlgButtonChecked(hwndMainOptsDlg,IDC_SHOWSENT));
			}
			else {
				CheckDlgButton(hwndDlg,IDC_SHOWREADNEXT,DBGetContactSettingByte(NULL,"SRMsg","ShowReadNext",SRMSGDEFSET_SHOWREADNEXT));
				CheckDlgButton(hwndDlg,IDC_SHOWSENT,DBGetContactSettingByte(NULL,"SRMsg","ShowSent",SRMSGDEFSET_SHOWSENT));
			}
			CheckDlgButton(hwndDlg,IDC_SHOWURLS,DBGetContactSettingByte(NULL,"SRMsg","ShowURLs",SRMSGDEFSET_SHOWURLS));
			CheckDlgButton(hwndDlg,IDC_SHOWFILES,DBGetContactSettingByte(NULL,"SRMsg","ShowFiles",SRMSGDEFSET_SHOWFILES));
			SendDlgItemMessage(hwndDlg,IDC_BKGCOLOUR,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"SRMsg","BkgColour",SRMSGDEFSET_BKGCOLOUR));
			SendDlgItemMessage(hwndDlg,IDC_BKGCOLOUR,CPM_SETDEFAULTCOLOUR,0,SRMSGDEFSET_BKGCOLOUR);

			hBkgColourBrush=CreateSolidBrush(SendDlgItemMessage(hwndDlg,IDC_BKGCOLOUR,CPM_GETCOLOUR,0,0));

			{	int i;
				LOGFONT lf;
				for(i=0;i<sizeof(fontOptionsList)/sizeof(fontOptionsList[0]);i++) {
					LoadMsgDlgFont(i,&lf,&fontOptionsList[i].colour);
					lstrcpy(fontOptionsList[i].szFace,lf.lfFaceName);
					fontOptionsList[i].size=(char)lf.lfHeight;
					fontOptionsList[i].style=(lf.lfWeight>=FW_BOLD?FONTF_BOLD:0)|(lf.lfItalic?FONTF_ITALIC:0);
					fontOptionsList[i].charset=lf.lfCharSet;
					//I *think* some OSs will fail LB_ADDSTRING if lParam==0
					SendDlgItemMessage(hwndDlg,IDC_FONTLIST,LB_ADDSTRING,0,i+1);
				}
				SendDlgItemMessage(hwndDlg,IDC_FONTLIST,LB_SETSEL,TRUE,0);
				SendDlgItemMessage(hwndDlg,IDC_FONTCOLOUR,CPM_SETCOLOUR,0,fontOptionsList[0].colour);
				SendDlgItemMessage(hwndDlg,IDC_FONTCOLOUR,CPM_SETDEFAULTCOLOUR,0,fontOptionsList[0].defColour);
			}
			return TRUE;
		case WM_CTLCOLORLISTBOX:
			SetBkColor((HDC)wParam,SendDlgItemMessage(hwndDlg,IDC_BKGCOLOUR,CPM_GETCOLOUR,0,0));
			return (BOOL)hBkgColourBrush;
		case WM_MEASUREITEM:
		{	MEASUREITEMSTRUCT *mis=(MEASUREITEMSTRUCT *)lParam;
			HFONT hFont,hoFont;
			HDC hdc;
			SIZE fontSize;
			int iItem=mis->itemData-1;
			hFont=CreateFont(fontOptionsList[iItem].size,0,0,0,
							 fontOptionsList[iItem].style&FONTF_BOLD?FW_BOLD:FW_NORMAL,
							 fontOptionsList[iItem].style&FONTF_ITALIC?1:0,0,0,
							 fontOptionsList[iItem].charset,
							 OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_DONTCARE,
							 fontOptionsList[iItem].szFace);
			hdc=GetDC(GetDlgItem(hwndDlg,mis->CtlID));
			hoFont=(HFONT)SelectObject(hdc,hFont);
			GetTextExtentPoint32(hdc,fontOptionsList[iItem].szDescr,lstrlen(fontOptionsList[iItem].szDescr),&fontSize);
			SelectObject(hdc,hoFont);
			ReleaseDC(GetDlgItem(hwndDlg,mis->CtlID),hdc);
			DeleteObject(hFont);
			mis->itemWidth=fontSize.cx;
			mis->itemHeight=fontSize.cy;
			return TRUE;
		}
		case WM_DRAWITEM:
		{	DRAWITEMSTRUCT *dis=(DRAWITEMSTRUCT *)lParam;
			HFONT hFont,hoFont;
			char *pszText;
			int iItem=dis->itemData-1;
			hFont=CreateFont(fontOptionsList[iItem].size,0,0,0,
							 fontOptionsList[iItem].style&FONTF_BOLD?FW_BOLD:FW_NORMAL,
							 fontOptionsList[iItem].style&FONTF_ITALIC?1:0,0,0,
							 fontOptionsList[iItem].charset,
							 OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_DONTCARE,
							 fontOptionsList[iItem].szFace);
			hoFont=(HFONT)SelectObject(dis->hDC,hFont);
			SetBkMode(dis->hDC,TRANSPARENT);
			FillRect(dis->hDC,&dis->rcItem,hBkgColourBrush);
			if(dis->itemState&ODS_SELECTED)
				FrameRect(dis->hDC,&dis->rcItem,GetSysColorBrush(COLOR_HIGHLIGHT));
			SetTextColor(dis->hDC,fontOptionsList[iItem].colour);
			pszText=Translate(fontOptionsList[iItem].szDescr);
			TextOut(dis->hDC,dis->rcItem.left,dis->rcItem.top,pszText,lstrlen(pszText));
			SelectObject(dis->hDC,hoFont);
			DeleteObject(hFont);
			return TRUE;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDC_LOADUNREAD:
				case IDC_LOADCOUNT:
				case IDC_LOADTIME:
					EnableWindow(GetDlgItem(hwndDlg,IDC_LOADCOUNTN),IsDlgButtonChecked(hwndDlg,IDC_LOADCOUNT));
					EnableWindow(GetDlgItem(hwndDlg,IDC_LOADCOUNTSPIN),IsDlgButtonChecked(hwndDlg,IDC_LOADCOUNT));
					EnableWindow(GetDlgItem(hwndDlg,IDC_LOADTIMEN),IsDlgButtonChecked(hwndDlg,IDC_LOADTIME));
					EnableWindow(GetDlgItem(hwndDlg,IDC_LOADTIMESPIN),IsDlgButtonChecked(hwndDlg,IDC_LOADTIME));
					EnableWindow(GetDlgItem(hwndDlg,IDC_STMINSOLD),IsDlgButtonChecked(hwndDlg,IDC_LOADTIME));
					break;
				case IDC_SHOWTIMES:
					EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWDATES),IsDlgButtonChecked(hwndDlg,IDC_SHOWTIMES));
					break;
				case IDC_SHOWREADNEXT:
				case IDC_SHOWSENT:
					if(hwndMainOptsDlg)
						SendMessage(hwndMainOptsDlg,M_UPDATEPRESETBUTTONS,0,0);
					break;
				case IDC_BKGCOLOUR:
					DeleteObject(hBkgColourBrush);
					hBkgColourBrush=CreateSolidBrush(SendDlgItemMessage(hwndDlg,IDC_BKGCOLOUR,CPM_GETCOLOUR,0,0));
					InvalidateRect(GetDlgItem(hwndDlg,IDC_FONTLIST),NULL,TRUE);
					break;
				case IDC_FONTLIST:
					if(HIWORD(wParam)==LBN_SELCHANGE) {
						if(SendDlgItemMessage(hwndDlg,IDC_FONTLIST,LB_GETSELCOUNT,0,0)>1) {
							SendDlgItemMessage(hwndDlg,IDC_FONTCOLOUR,CPM_SETCOLOUR,0,GetSysColor(COLOR_3DFACE));
							SendDlgItemMessage(hwndDlg,IDC_FONTCOLOUR,CPM_SETDEFAULTCOLOUR,0,GetSysColor(COLOR_WINDOWTEXT));
						}
						else {
							int i=SendDlgItemMessage(hwndDlg,IDC_FONTLIST,LB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_FONTLIST,LB_GETCURSEL,0,0),0)-1;
							SendDlgItemMessage(hwndDlg,IDC_FONTCOLOUR,CPM_SETCOLOUR,0,fontOptionsList[i].colour);
							SendDlgItemMessage(hwndDlg,IDC_FONTCOLOUR,CPM_SETDEFAULTCOLOUR,0,fontOptionsList[i].defColour);
						}
					}
					if(HIWORD(wParam)!=LBN_DBLCLK) return TRUE;
					//fall through
				case IDC_CHOOSEFONT:
				{	CHOOSEFONT cf={0};
					LOGFONT lf={0};
					int i=SendDlgItemMessage(hwndDlg,IDC_FONTLIST,LB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_FONTLIST,LB_GETCURSEL,0,0),0)-1;
					lf.lfHeight=fontOptionsList[i].size;
					lf.lfWeight=fontOptionsList[i].style&FONTF_BOLD?FW_BOLD:FW_NORMAL;
					lf.lfItalic=fontOptionsList[i].style&FONTF_ITALIC?1:0;
					lf.lfCharSet=fontOptionsList[i].charset;
					lf.lfOutPrecision=OUT_DEFAULT_PRECIS;
					lf.lfClipPrecision=CLIP_DEFAULT_PRECIS;
					lf.lfQuality=DEFAULT_QUALITY;
					lf.lfPitchAndFamily=DEFAULT_PITCH|FF_DONTCARE;
					lstrcpy(lf.lfFaceName,fontOptionsList[i].szFace);
					cf.lStructSize=sizeof(cf);
					cf.hwndOwner=hwndDlg;
					cf.lpLogFont=&lf;
					cf.Flags=CF_FORCEFONTEXIST|CF_INITTOLOGFONTSTRUCT|CF_SCREENFONTS;
					if(ChooseFont(&cf)) {
						int selItems[sizeof(fontOptionsList)/sizeof(fontOptionsList[0])];
						int sel,selCount;

						selCount=SendDlgItemMessage(hwndDlg,IDC_FONTLIST,LB_GETSELITEMS,sizeof(fontOptionsList)/sizeof(fontOptionsList[0]),(LPARAM)selItems);
						for(sel=0;sel<selCount;sel++) {
							i=SendDlgItemMessage(hwndDlg,IDC_FONTLIST,LB_GETITEMDATA,selItems[sel],0)-1;
							fontOptionsList[i].size=(char)lf.lfHeight;
							fontOptionsList[i].style=(lf.lfWeight>=FW_BOLD?FONTF_BOLD:0)|(lf.lfItalic?FONTF_ITALIC:0);
							fontOptionsList[i].charset=lf.lfCharSet;
							lstrcpy(fontOptionsList[i].szFace,lf.lfFaceName);
							{	MEASUREITEMSTRUCT mis={0};
								mis.CtlID=IDC_FONTLIST;
								mis.itemData=i+1;
								SendMessage(hwndDlg,WM_MEASUREITEM,0,(LPARAM)&mis);
								SendDlgItemMessage(hwndDlg,IDC_FONTLIST,LB_SETITEMHEIGHT,selItems[sel],mis.itemHeight);
							}
						}
						InvalidateRect(GetDlgItem(hwndDlg,IDC_FONTLIST),NULL,TRUE);
						break;
					}
					return TRUE;
				}
				case IDC_FONTCOLOUR:
				{ 	int selItems[sizeof(fontOptionsList)/sizeof(fontOptionsList[0])];
					int sel,selCount,i;

					selCount=SendDlgItemMessage(hwndDlg,IDC_FONTLIST,LB_GETSELITEMS,sizeof(fontOptionsList)/sizeof(fontOptionsList[0]),(LPARAM)selItems);
					for(sel=0;sel<selCount;sel++) {
						i=SendDlgItemMessage(hwndDlg,IDC_FONTLIST,LB_GETITEMDATA,selItems[sel],0)-1;
						fontOptionsList[i].colour=SendDlgItemMessage(hwndDlg,IDC_FONTCOLOUR,CPM_GETCOLOUR,0,0);
					}
					InvalidateRect(GetDlgItem(hwndDlg,IDC_FONTLIST),NULL,FALSE);
					break;
				}
				case IDC_LOADCOUNTN:
				case IDC_LOADTIMEN:
					if(HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()) return TRUE;
					break;
			}
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->idFrom) {
				case 0:
					switch (((LPNMHDR)lParam)->code)
					{
						case PSN_APPLY:
							if(IsDlgButtonChecked(hwndDlg,IDC_LOADCOUNT))
								DBWriteContactSettingByte(NULL,"SRMsg","LoadHistory",LOADHISTORY_COUNT);
							else if(IsDlgButtonChecked(hwndDlg,IDC_LOADTIME))
								DBWriteContactSettingByte(NULL,"SRMsg","LoadHistory",LOADHISTORY_TIME);
							else
								DBWriteContactSettingByte(NULL,"SRMsg","LoadHistory",LOADHISTORY_UNREAD);
							DBWriteContactSettingWord(NULL,"SRMsg","LoadCount",(WORD)SendDlgItemMessage(hwndDlg,IDC_LOADCOUNTSPIN,UDM_GETPOS,0,0));
							DBWriteContactSettingWord(NULL,"SRMsg","LoadTime",(WORD)SendDlgItemMessage(hwndDlg,IDC_LOADTIMESPIN,UDM_GETPOS,0,0));
                           
							DBWriteContactSettingByte(NULL,"SRMsg","ShowLogIcons",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWLOGICONS));
							DBWriteContactSettingByte(NULL,"SRMsg","HideNames",(BYTE)!IsDlgButtonChecked(hwndDlg,IDC_SHOWNAMES));
							DBWriteContactSettingByte(NULL,"SRMsg","ShowTime",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWTIMES));
							DBWriteContactSettingByte(NULL,"SRMsg","ShowDate",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWDATES));
							DBWriteContactSettingByte(NULL,"SRMsg","ShowReadNext",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWREADNEXT));
							DBWriteContactSettingByte(NULL,"SRMsg","ShowURLs",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWURLS));
							DBWriteContactSettingByte(NULL,"SRMsg","ShowFiles",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWFILES));
							DBWriteContactSettingByte(NULL,"SRMsg","ShowSent",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWSENT));
							DBWriteContactSettingDword(NULL,"SRMsg","BkgColour",SendDlgItemMessage(hwndDlg,IDC_BKGCOLOUR,CPM_GETCOLOUR,0,0));

							{	int i;
								char str[32];
								for(i=0;i<sizeof(fontOptionsList)/sizeof(fontOptionsList[0]);i++) {
									wsprintf(str,"Font%d",i);
									DBWriteContactSettingString(NULL,"SRMsg",str,fontOptionsList[i].szFace);
									wsprintf(str,"Font%dSize",i);
									DBWriteContactSettingByte(NULL,"SRMsg",str,fontOptionsList[i].size);
									wsprintf(str,"Font%dSty",i);
									DBWriteContactSettingByte(NULL,"SRMsg",str,fontOptionsList[i].style);
									wsprintf(str,"Font%dSet",i);
									DBWriteContactSettingByte(NULL,"SRMsg",str,fontOptionsList[i].charset);
									wsprintf(str,"Font%dCol",i);
									DBWriteContactSettingDword(NULL,"SRMsg",str,fontOptionsList[i].colour);
								}
							}

							FreeMsgLogIcons();
							LoadMsgLogIcons();
							WindowList_Broadcast(hMessageWindowList,DM_OPTIONSAPPLIED,0,0);
							WindowList_Broadcast(hMessageSendList,DM_OPTIONSAPPLIED,0,0);
							return TRUE;
					}
					break;
			}
			break;
		case WM_DESTROY:
			hwndLogOptsDlg=NULL;
			DeleteObject(hBkgColourBrush);
			break;
	}
	return FALSE;
}

static int OptInitialise(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp={0};

	odp.cbSize=sizeof(odp);
	odp.position=910000000;
	odp.hInstance=GetModuleHandle(NULL);
	odp.pszTemplate=MAKEINTRESOURCE(IDD_OPT_MSGDLG);
	odp.pszTitle=Translate("Messaging");
	odp.pszGroup=Translate("Events");
	odp.pfnDlgProc=DlgProcOptions;
	odp.flags=ODPF_BOLDGROUPS;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	odp.pszTemplate=MAKEINTRESOURCE(IDD_OPT_MSGLOG);
	odp.pszTitle=Translate("Messaging Log");
	odp.pfnDlgProc=DlgProcLogOptions;
	odp.nIDBottomSimpleControl=IDC_STMSGLOGGROUP;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
	return 0;
}

int InitOptions(void)
{
	HookEvent(ME_OPT_INITIALISE,OptInitialise);
	return 0;
}