/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project, 
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

#include "m_fontservice.h"

#define FONTF_NORMAL 0
#define FONTF_BOLD   1
#define FONTF_ITALIC 2
#define FONTF_UNDERLINE  4

struct FontOptionsList
{
	int		 fontID;
	TCHAR*   szGroup;
	TCHAR*   szDescr;
	COLORREF defColour;
	TCHAR*   szDefFace;
	BYTE     defCharset, defStyle;
	char     defSize;
	COLORREF colour;
	TCHAR    szFace[LF_FACESIZE];
	BYTE     charset, style;
	char     size;
};

#define CLCGROUP			LPGENT("Contact List")
#define DEFAULT_COLOUR		RGB(0, 0, 0)
#define DEFAULT_FAMILY		_T("Arial")
#define DEFAULT_FONT_STYLE	FONTF_NORMAL
#define DEFAULT_SIZE		-8
#define DEFAULT_SMALLSIZE	-6

static struct FontOptionsList fontOptionsList[] = {
		{ FONTID_CONTACTS, CLCGROUP, LPGENT( "Standard contacts"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_AWAY, CLCGROUP, LPGENT( "Away contacts"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_DND, CLCGROUP, LPGENT( "DND contacts"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_NA, CLCGROUP, LPGENT( "NA contacts"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_OCCUPIED, CLCGROUP, LPGENT( "Occupied contacts"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_CHAT, CLCGROUP, LPGENT( "Free for chat contacts"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_INVISIBLE, CLCGROUP, LPGENT( "Invisible contacts"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_PHONE, CLCGROUP, LPGENT( "On the phone contacts"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_LUNCH, CLCGROUP, LPGENT( "Out to lunch contacts"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_OFFLINE, CLCGROUP, LPGENT( "Offline contacts"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_INVIS, CLCGROUP, LPGENT( "Online contacts to whom you have a different visibility"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },		
		{ FONTID_OFFINVIS, CLCGROUP, LPGENT( "Offline contacts to whom you have a different visibility"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_NOTONLIST, CLCGROUP, LPGENT( "Contacts who are 'not on list'"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_OPENGROUPS, CLCGROUP, LPGENT( "Open groups"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_OPENGROUPCOUNTS, CLCGROUP, LPGENT( "Open group member counts"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_CLOSEDGROUPS, CLCGROUP, LPGENT( "Closed groups"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_CLOSEDGROUPCOUNTS, CLCGROUP, LPGENT( "Closed group member counts"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_DIVIDERS, CLCGROUP, LPGENT( "Dividers"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_SECONDLINE, CLCGROUP, LPGENT( "Second line"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_THIRDLINE, CLCGROUP, LPGENT( "Third line"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_CONTACT_TIME, CLCGROUP, LPGENT( "Contact time"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_STATUSBAR_PROTONAME, CLCGROUP, LPGENT( "Status bar text"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_EVENTAREA, CLCGROUP, LPGENT( "Event area text"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
		{ FONTID_VIEMODES, CLCGROUP, LPGENT( "Current view mode text"), DEFAULT_COLOUR, DEFAULT_FAMILY, DEFAULT_CHARSET, DEFAULT_FONT_STYLE, DEFAULT_SIZE },
};

const int msgDlgFontCount = SIZEOF(fontOptionsList);
/*
void LoadMsgDlgFont(int i, LOGFONT* lf, COLORREF * colour)
{
	char str[32];
	int style;
	DBVARIANT dbv;

	if ( colour ) {
		wsprintfA(str, "SRMFont%dCol", i);
		*colour = DBGetContactSettingDword(NULL, SRMMMOD, str, fontOptionsList[i].defColour);
	}
	if ( lf ) {
		wsprintfA(str, "SRMFont%dSize", i);
		lf->lfHeight = (char) DBGetContactSettingByte(NULL, SRMMMOD, str, fontOptionsList[i].defSize);
		lf->lfWidth = 0;
		lf->lfEscapement = 0;
		lf->lfOrientation = 0;
		wsprintfA(str, "SRMFont%dSty", i);
		style = DBGetContactSettingByte(NULL, SRMMMOD, str, fontOptionsList[i].defStyle);
		lf->lfWeight = style & FONTF_BOLD ? FW_BOLD : FW_NORMAL;
		lf->lfItalic = style & FONTF_ITALIC ? 1 : 0;
		lf->lfUnderline = 0;
		lf->lfStrikeOut = 0;
		wsprintfA(str, "SRMFont%dSet", i);
		lf->lfCharSet = DBGetContactSettingByte(NULL, SRMMMOD, str, fontOptionsList[i].defCharset);
		lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
		lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
		lf->lfQuality = DEFAULT_QUALITY;
		lf->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
		wsprintfA(str, "SRMFont%d", i);
		if (DBGetContactSettingTString(NULL, SRMMMOD, str, &dbv))
			lstrcpy(lf->lfFaceName, fontOptionsList[i].szDefFace);
		else {
			lstrcpyn(lf->lfFaceName, dbv.ptszVal, SIZEOF(lf->lfFaceName));
			DBFreeVariant(&dbv);
}	}	}

*/
void RegisterCLUIFonts( void )
{
	FontIDT fontid = {0};
	ColourIDT colourid = {0};
	char idstr[10];
	int i, index = 0;

	fontid.cbSize = FontID_SIZEOF_V2;
	fontid.flags =  FIDF_DEFAULTVALID | FIDF_APPENDNAME | FIDF_SAVEPOINTSIZE | FIDF_ALLOWEFFECTS;
	for ( i = 0; i < msgDlgFontCount; i++, index++ ) 
	{
		strncpy(fontid.dbSettingsGroup, "_CList", sizeof(fontid.dbSettingsGroup));
		_tcsncpy(fontid.group, fontOptionsList[i].szGroup, SIZEOF(fontid.group));
		_tcsncpy(fontid.name, fontOptionsList[i].szDescr, SIZEOF(fontid.name));
		sprintf(idstr, "Font%d", fontOptionsList[i].fontID);
		strncpy(fontid.prefix, idstr, SIZEOF(fontid.prefix));
		fontid.order = i+1;

		fontid.deffontsettings.charset = fontOptionsList[i].defCharset;
		fontid.deffontsettings.colour = fontOptionsList[i].defColour;
		fontid.deffontsettings.size = fontOptionsList[i].defSize;
		fontid.deffontsettings.style = fontOptionsList[i].defStyle;
		_tcsncpy(fontid.deffontsettings.szFace, fontOptionsList[i].szDefFace, SIZEOF(fontid.deffontsettings.szFace));
		CallService(MS_FONT_REGISTERT, (WPARAM)&fontid, 0);
	}
}




static WORD fontSameAsDefault[]={0x00FF,0x8B00,0x8F00,0x8700,0x8B00,0x8104,0x8D00,0x8B02,0x8900,0x8D00,
0x8F00,0x8F00,0x8F00,0x8F00,0x8F00,0x8F00,0x8F00,0x8F00,0x8F00,0x8104,0x8D00,0x00FF,0x8F00,0x8F00};
static TCHAR *fontSizes[]={_T("6"),_T("7"),_T("8"),_T("9"),_T("10"),_T("12"),_T("14"),_T("16"),_T("18"),_T("20"),_T("24"),_T("28")};
static int fontListOrder[]={FONTID_CONTACTS,FONTID_INVIS,FONTID_OFFLINE,FONTID_OFFINVIS,FONTID_NOTONLIST,FONTID_OPENGROUPS,FONTID_OPENGROUPCOUNTS,FONTID_CLOSEDGROUPS,FONTID_CLOSEDGROUPCOUNTS,FONTID_DIVIDERS,FONTID_SECONDLINE,FONTID_THIRDLINE,
FONTID_AWAY,FONTID_DND,FONTID_NA,FONTID_OCCUPIED,FONTID_CHAT,FONTID_INVISIBLE,FONTID_PHONE,FONTID_LUNCH,FONTID_CONTACT_TIME,FONTID_STATUSBAR_PROTONAME,FONTID_EVENTAREA, FONTID_VIEMODES};

static BOOL CALLBACK DlgProcClcMainOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcClcMetaOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcClcBkgOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcClcTextOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcStatusBarBkgOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);



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
	lf->lfWeight=FW_NORMAL;
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
static int SameAsAntiCycle=0;
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
		//mir_free_and_nill(dbv.pszVal);
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





static BOOL CALLBACK DlgProcGenOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcSBarOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcCluiOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK DlgProcCluiOpts2(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

static struct TabItemOptionConf
{
	TCHAR *name;			// Tab name
	int id;					// Dialog id
	DLGPROC wnd_proc;		// Dialog function
	DWORD flag;				// Expertonly
} clist_opt_items[] = 
{ 
	{ _T("General"), IDD_OPT_CLIST, DlgProcGenOpts, 0},
	{ _T("List"), IDD_OPT_CLC, DlgProcClcMainOpts, 0 },
	{ _T("Window"), IDD_OPT_CLUI, DlgProcCluiOpts, 0 },
	{ _T("Behaviour"), IDD_OPT_CLUI_2, DlgProcCluiOpts2, 0 },
	{ _T("Status Bar"), IDD_OPT_SBAR, DlgProcSBarOpts, 0},	
	{ _T("Additional stuff"), IDD_OPT_META_CLC, DlgProcClcMetaOpts, 0 },
	{ 0 }
};


int ClcOptInit(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp;
	if (MirandaExiting()) return 0;
	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize=sizeof(odp);
	odp.position=0;
	odp.hInstance=g_hInst;
	//odp.ptszGroup=TranslateT("Contact List");
	odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_CLC);
	odp.ptszTitle=LPGENT("Contact List");
	odp.pfnDlgProc=DlgProcClcMainOpts;
	odp.flags=ODPF_BOLDGROUPS|ODPF_TCHAR;
	//CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);  
	{
		int i;	
		for (i=0; clist_opt_items[i].id!=0; i++)
		{
			odp.pszTemplate=MAKEINTRESOURCEA(clist_opt_items[i].id);
			odp.ptszTab=TranslateTS(clist_opt_items[i].name);
			odp.pfnDlgProc=clist_opt_items[i].wnd_proc;
			odp.flags=ODPF_BOLDGROUPS|ODPF_TCHAR|clist_opt_items[i].flag;
			CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);
		}
	}

	odp.pszTemplate=MAKEINTRESOURCEA(IDD_OPT_CLCTEXT);
	odp.ptszGroup=TranslateT("Customize");
	odp.ptszTitle=TranslateT("List Text");
	odp.pfnDlgProc=DlgProcClcTextOpts;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	if (g_CluiData.fDisableSkinEngine)
	{
		odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_CLCBKG);
		odp.ptszGroup = TranslateT("Customize");
		odp.ptszTitle = TranslateT("Contact list skin");
		odp.ptszTab  = TranslateT("List Background");
		odp.pfnDlgProc = DlgProcClcBkgOpts;
		odp.flags = ODPF_BOLDGROUPS|ODPF_TCHAR;
		CallService(MS_OPT_ADDPAGE, wParam, (LPARAM) & odp);
	}
	//RegisterCLUIFonts();
	return 0;
}

struct CheckBoxToStyleEx_t 
{
	int id;
	DWORD flag;
	int not;
} static const checkBoxToStyleEx[]=
{
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
	{IDC_NOTNOSMOOTHSCROLLING,CLS_EX_NOSMOOTHSCROLLING,1}
};

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
	{PF2_ONTHEPHONE,_T("On the phone")}
};
static const struct CheckBoxValues_t offlineValues[]=
{
	{MODEF_OFFLINE,_T("Offline")},
	{PF2_ONLINE,_T("Online")},
	{PF2_SHORTAWAY,_T("Away")},
	{PF2_LONGAWAY,_T("NA")},
	{PF2_LIGHTDND,_T("Occupied")},
	{PF2_HEAVYDND,_T("DND")},
	{PF2_FREECHAT,_T("Free for chat")},
	{PF2_INVISIBLE,_T("Invisible")},
	{PF2_OUTTOLUNCH,_T("Out to lunch")},
	{PF2_ONTHEPHONE,_T("On the phone")}
};

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
		CheckDlgButton(hwndDlg, IDC_META, DBGetContactSettingByte(NULL,"CLC","Meta",SETTING_USEMETAICON_DEFAULT) ? BST_CHECKED : BST_UNCHECKED); /// by FYR
		CheckDlgButton(hwndDlg, IDC_METADBLCLK, DBGetContactSettingByte(NULL,"CLC","MetaDoubleClick",SETTING_METAAVOIDDBLCLICK_DEFAULT) ? BST_CHECKED : BST_UNCHECKED); /// by FYR
		CheckDlgButton(hwndDlg, IDC_METASUBEXTRA, DBGetContactSettingByte(NULL,"CLC","MetaHideExtra",SETTING_METAHIDEEXTRA_DEFAULT) ? BST_CHECKED : BST_UNCHECKED); /// by FYR
		CheckDlgButton(hwndDlg, IDC_METASUBEXTRA_IGN, DBGetContactSettingByte(NULL,"CLC","MetaIgnoreEmptyExtra",SETTING_METAAVOIDDBLCLICK_DEFAULT) ? BST_CHECKED : BST_UNCHECKED); /// by FYR
		CheckDlgButton(hwndDlg, IDC_METASUB_HIDEOFFLINE, DBGetContactSettingByte(NULL,"CLC","MetaHideOfflineSub",SETTING_METAHIDEOFFLINESUB_DEFAULT) ? BST_CHECKED : BST_UNCHECKED); /// by FYR
		CheckDlgButton(hwndDlg, IDC_METAEXPAND, DBGetContactSettingByte(NULL,"CLC","MetaExpanding",SETTING_METAEXPANDING_DEFAULT) ? BST_CHECKED : BST_UNCHECKED); /// by FYR
		CheckDlgButton(hwndDlg, IDC_DISCOVER_AWAYMSG, DBGetContactSettingByte(NULL,"ModernData","InternalAwayMsgDiscovery",SETTING_INTERNALAWAYMSGREQUEST_DEFAULT) ? BST_CHECKED : BST_UNCHECKED); /// by FYR
		CheckDlgButton(hwndDlg, IDC_REMOVE_OFFLINE_AWAYMSG, DBGetContactSettingByte(NULL,"ModernData","RemoveAwayMessageForOffline",SETTING_REMOVEAWAYMSGFOROFFLINE_DEFAULT) ? BST_CHECKED : BST_UNCHECKED); /// by FYR

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

		SendDlgItemMessage(hwndDlg,IDC_GROUPINDENTSPIN,UDM_SETRANGE,0,MAKELONG(50,0));
		SendDlgItemMessage(hwndDlg,IDC_GROUPINDENTSPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingByte(NULL,"CLC","GroupIndent",CLCDEFAULT_GROUPINDENT),0));
		CheckDlgButton(hwndDlg,IDC_GREYOUT,DBGetContactSettingDword(NULL,"CLC","GreyoutFlags",CLCDEFAULT_GREYOUTFLAGS)?BST_CHECKED:BST_UNCHECKED);

		EnableWindow(GetDlgItem(hwndDlg,IDC_SMOOTHTIME),IsDlgButtonChecked(hwndDlg,IDC_NOTNOSMOOTHSCROLLING));
		EnableWindow(GetDlgItem(hwndDlg,IDC_GREYOUTOPTS),IsDlgButtonChecked(hwndDlg,IDC_GREYOUT));
		FillCheckBoxTree(GetDlgItem(hwndDlg,IDC_GREYOUTOPTS),greyoutValues,sizeof(greyoutValues)/sizeof(greyoutValues[0]),DBGetContactSettingDword(NULL,"CLC","FullGreyoutFlags",CLCDEFAULT_FULLGREYOUTFLAGS));
		FillCheckBoxTree(GetDlgItem(hwndDlg,IDC_HIDEOFFLINEOPTS),offlineValues,sizeof(offlineValues)/sizeof(offlineValues[0]),DBGetContactSettingDword(NULL,"CLC","OfflineModes",CLCDEFAULT_OFFLINEMODES));
		CheckDlgButton(hwndDlg,IDC_NOSCROLLBAR,DBGetContactSettingByte(NULL,"CLC","NoVScrollBar",CLCDEFAULT_NOVSCROLL)?BST_CHECKED:BST_UNCHECKED);
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
				//mir_free_and_nill(dbv.pszVal);
				DBFreeVariant(&dbv);
		}
		}

		CheckDlgButton(hwndDlg,IDC_HILIGHTMODE,DBGetContactSettingByte(NULL,"StatusBar","HiLightMode",SETTING_SBHILIGHTMODE_DEFAULT)==0?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hwndDlg,IDC_HILIGHTMODE1,DBGetContactSettingByte(NULL,"StatusBar","HiLightMode",SETTING_SBHILIGHTMODE_DEFAULT)==1?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hwndDlg,IDC_HILIGHTMODE2,DBGetContactSettingByte(NULL,"StatusBar","HiLightMode",SETTING_SBHILIGHTMODE_DEFAULT)==2?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hwndDlg,IDC_HILIGHTMODE3,DBGetContactSettingByte(NULL,"StatusBar","HiLightMode",SETTING_SBHILIGHTMODE_DEFAULT)==3?BST_CHECKED:BST_UNCHECKED);



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


//static BOOL CALLBACK DlgProcClcBkgOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
//{
//	switch (msg)
//	{
//	case WM_INITDIALOG:
//		TranslateDialogDefault(hwndDlg);
//		CheckDlgButton(hwndDlg,IDC_BITMAP,DBGetContactSettingByte(NULL,"CLC","UseBitmap",CLCDEFAULT_USEBITMAP)?BST_CHECKED:BST_UNCHECKED);
//		SendMessage(hwndDlg,WM_USER+10,0,0);
//		SendDlgItemMessage(hwndDlg,IDC_BKGCOLOUR,CPM_SETDEFAULTCOLOUR,0,CLCDEFAULT_BKCOLOUR);
//		SendDlgItemMessage(hwndDlg,IDC_BKGCOLOUR,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"CLC","BkColour",CLCDEFAULT_BKCOLOUR));
//		SendDlgItemMessage(hwndDlg,IDC_SELCOLOUR,CPM_SETDEFAULTCOLOUR,0,CLCDEFAULT_SELBKCOLOUR);
//		SendDlgItemMessage(hwndDlg,IDC_SELCOLOUR,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"CLC","SelBkColour",CLCDEFAULT_SELBKCOLOUR));
//		{	DBVARIANT dbv={0};
//		if(!DBGetContactSetting(NULL,"CLC","BkBitmap",&dbv)) {
//			SetDlgItemTextA(hwndDlg,IDC_FILENAME,dbv.pszVal);
//			if (ServiceExists(MS_UTILS_PATHTOABSOLUTE)) {
//				char szPath[MAX_PATH];
//
//				if (CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)szPath))
//					SetDlgItemTextA(hwndDlg,IDC_FILENAME,szPath);
//			}
//			else 
//				//mir_free_and_nill(dbv.pszVal);
//				DBFreeVariant(&dbv);
//		}
//		}
//
//		/*			CheckDlgButton(hwndDlg,IDC_HILIGHTMODE,DBGetContactSettingByte(NULL,"CLC","HiLightMode",SETTING_HILIGHTMODE_DEFAULT)==0?BST_CHECKED:BST_UNCHECKED);
//		CheckDlgButton(hwndDlg,IDC_HILIGHTMODE1,DBGetContactSettingByte(NULL,"CLC","HiLightMode",SETTING_HILIGHTMODE_DEFAULT)==1?BST_CHECKED:BST_UNCHECKED);
//		CheckDlgButton(hwndDlg,IDC_HILIGHTMODE2,DBGetContactSettingByte(NULL,"CLC","HiLightMode",SETTING_HILIGHTMODE_DEFAULT)==2?BST_CHECKED:BST_UNCHECKED);
//		CheckDlgButton(hwndDlg,IDC_HILIGHTMODE3,DBGetContactSettingByte(NULL,"CLC","HiLightMode",SETTING_HILIGHTMODE_DEFAULT)==3?BST_CHECKED:BST_UNCHECKED);
//
//		*/			
//
//		{	WORD bmpUse=DBGetContactSettingWord(NULL,"CLC","BkBmpUse",CLCDEFAULT_BKBMPUSE);
//		CheckDlgButton(hwndDlg,IDC_STRETCHH,bmpUse&CLB_STRETCHH?BST_CHECKED:BST_UNCHECKED);
//		CheckDlgButton(hwndDlg,IDC_STRETCHV,bmpUse&CLB_STRETCHV?BST_CHECKED:BST_UNCHECKED);
//		CheckDlgButton(hwndDlg,IDC_TILEH,bmpUse&CLBF_TILEH?BST_CHECKED:BST_UNCHECKED);
//		CheckDlgButton(hwndDlg,IDC_TILEV,bmpUse&CLBF_TILEV?BST_CHECKED:BST_UNCHECKED);
//		CheckDlgButton(hwndDlg,IDC_SCROLL,bmpUse&CLBF_SCROLL?BST_CHECKED:BST_UNCHECKED);
//		CheckDlgButton(hwndDlg,IDC_PROPORTIONAL,bmpUse&CLBF_PROPORTIONAL?BST_CHECKED:BST_UNCHECKED);
//		CheckDlgButton(hwndDlg,IDC_TILEVROWH,bmpUse&CLBF_TILEVTOROWHEIGHT?BST_CHECKED:BST_UNCHECKED);
//
//		}
//		{	HRESULT (STDAPICALLTYPE *MySHAutoComplete)(HWND,DWORD);
//		MySHAutoComplete=(HRESULT (STDAPICALLTYPE*)(HWND,DWORD))GetProcAddress(GetModuleHandle(TEXT("shlwapi")),"SHAutoComplete");
//		if(MySHAutoComplete) MySHAutoComplete(GetDlgItem(hwndDlg,IDC_FILENAME),1);
//		}
//		return TRUE;
//	case WM_USER+10:
//		EnableWindow(GetDlgItem(hwndDlg,IDC_FILENAME),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
//		EnableWindow(GetDlgItem(hwndDlg,IDC_BROWSE),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
//		EnableWindow(GetDlgItem(hwndDlg,IDC_STRETCHH),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
//		EnableWindow(GetDlgItem(hwndDlg,IDC_STRETCHV),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
//		EnableWindow(GetDlgItem(hwndDlg,IDC_TILEH),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
//		EnableWindow(GetDlgItem(hwndDlg,IDC_TILEV),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
//		EnableWindow(GetDlgItem(hwndDlg,IDC_SCROLL),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
//		EnableWindow(GetDlgItem(hwndDlg,IDC_PROPORTIONAL),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
//		EnableWindow(GetDlgItem(hwndDlg,IDC_TILEVROWH),IsDlgButtonChecked(hwndDlg,IDC_BITMAP));
//		break;
//	case WM_USER + 11:
//		{
//			BOOL b = IsDlgButtonChecked(hwndDlg, IDC_WINCOLOUR);
//			EnableWindow(GetDlgItem(hwndDlg, IDC_BKGCOLOUR), !b);
//			EnableWindow(GetDlgItem(hwndDlg, IDC_SELCOLOUR), !b);
//			break;
//		}
//	case WM_COMMAND:
//		if(LOWORD(wParam)==IDC_BROWSE) {
//			char str[MAX_PATH];
//			OPENFILENAMEA ofn={0};
//			char filter[512];
//
//			GetDlgItemTextA(hwndDlg,IDC_FILENAME,str,sizeof(str));
//			ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
//			ofn.hwndOwner = hwndDlg;
//			ofn.hInstance = NULL;
//			CallService(MS_UTILS_GETBITMAPFILTERSTRINGS,sizeof(filter),(LPARAM)filter);
//			ofn.lpstrFilter = filter;
//			ofn.lpstrFile = str;
//			ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
//			ofn.nMaxFile = sizeof(str);
//			ofn.nMaxFileTitle = MAX_PATH;
//			ofn.lpstrDefExt = "bmp";
//			if(!GetOpenFileNameA(&ofn)) break;
//			SetDlgItemTextA(hwndDlg,IDC_FILENAME,str);
//		}
//		else if(LOWORD(wParam)==IDC_FILENAME && HIWORD(wParam)!=EN_CHANGE) break;
//		if(LOWORD(wParam)==IDC_BITMAP)			SendMessage(hwndDlg,WM_USER+10,0,0);
//		if (LOWORD(wParam) == IDC_WINCOLOUR)	SendMessage(hwndDlg, WM_USER + 11, 0, 0);
//		if(LOWORD(wParam)==IDC_FILENAME && (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus())) return 0;
//		SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
//		break;
//	case WM_NOTIFY:
//		switch(((LPNMHDR)lParam)->idFrom) {
//	case 0:
//		switch (((LPNMHDR)lParam)->code)
//		{
//		case PSN_APPLY:
//
//			DBWriteContactSettingByte(NULL, "CLC", "UseBitmap", (BYTE) IsDlgButtonChecked(hwndDlg, IDC_BITMAP));
//			{
//				COLORREF col;
//				col = SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_GETCOLOUR, 0, 0);
//				if (col == CLCDEFAULT_BKCOLOUR)
//					DBDeleteContactSetting(NULL, "CLC", "BkColour");
//				else
//					DBWriteContactSettingDword(NULL, "CLC", "BkColour", col);
//				col = SendDlgItemMessage(hwndDlg, IDC_SELCOLOUR, CPM_GETCOLOUR, 0, 0);
//				if (col == CLCDEFAULT_SELBKCOLOUR)
//					DBDeleteContactSetting(NULL, "CLC", "SelBkColour");
//				else
//					DBWriteContactSettingDword(NULL, "CLC", "SelBkColour", col);
//				DBWriteContactSettingByte(NULL, "CLC", "UseWinColours", (BYTE)(IsDlgButtonChecked(hwndDlg, IDC_WINCOLOUR)));
//			}
//			{	
//				char str[MAX_PATH],strrel[MAX_PATH];
//				GetDlgItemTextA(hwndDlg,IDC_FILENAME,str,sizeof(str));
//				if (ServiceExists(MS_UTILS_PATHTORELATIVE)) {
//					if (CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)str, (LPARAM)strrel))
//						DBWriteContactSettingString(NULL,"CLC","BkBitmap",strrel);
//					else DBWriteContactSettingString(NULL,"CLC","BkBitmap",str);
//				}
//				else DBWriteContactSettingString(NULL,"CLC","BkBitmap",str);
//
//			}
//			{	WORD flags=0;
//			if(IsDlgButtonChecked(hwndDlg,IDC_STRETCHH)) flags|=CLB_STRETCHH;
//			if(IsDlgButtonChecked(hwndDlg,IDC_STRETCHV)) flags|=CLB_STRETCHV;
//			if(IsDlgButtonChecked(hwndDlg,IDC_TILEH)) flags|=CLBF_TILEH;
//			if(IsDlgButtonChecked(hwndDlg,IDC_TILEV)) flags|=CLBF_TILEV;
//			if(IsDlgButtonChecked(hwndDlg,IDC_SCROLL)) flags|=CLBF_SCROLL;
//			if(IsDlgButtonChecked(hwndDlg,IDC_PROPORTIONAL)) flags|=CLBF_PROPORTIONAL;
//			if(IsDlgButtonChecked(hwndDlg,IDC_TILEVROWH)) flags|=CLBF_TILEVTOROWHEIGHT;
//
//			DBWriteContactSettingWord(NULL,"CLC","BkBmpUse",flags);
//			}
//			/*							{
//			int hil=0;
//			if (IsDlgButtonChecked(hwndDlg,IDC_HILIGHTMODE1))  hil=1;
//			if (IsDlgButtonChecked(hwndDlg,IDC_HILIGHTMODE2))  hil=2;
//			if (IsDlgButtonChecked(hwndDlg,IDC_HILIGHTMODE3))  hil=3;
//
//			DBWriteContactSettingByte(NULL,"CLC","HiLightMode",(BYTE)hil);
//
//			}
//			*/
//			ClcOptionsChanged();
//			CLUIServices_ProtocolStatusChanged(0,0);
//			if (IsWindowVisible(pcli->hwndContactList))
//			{
//				CLUI_ShowWindowMod(pcli->hwndContactList,SW_HIDE);
//				CLUI_ShowWindowMod(pcli->hwndContactList,SW_SHOW);
//			}
//
//			return TRUE;
//		}
//		break;
//		}
//		break;
//	}
//	return FALSE;
//}

static const TCHAR *szFontIdDescr[FONTID_MODERN_MAX+1]=
{
	_T("Standard contacts"),
		_T("Online contacts to whom you have a different visibility"),
		_T("Offline contacts"),
		_T("Contacts who are 'not on list'"),
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
		_T("Event area text"),
		_T("Current view mode text")
};

#include <pshpack1.h>
struct _fontSettings {
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
	g_dwFillFontListThreadID=0;
	return;
}

static int CALLBACK EnumFontScriptsProc(ENUMLOGFONTEX *lpelfe,NEWTEXTMETRICEX *lpntme,int FontType,LPARAM lParam)
{
	if(SendMessage((HWND)lParam,CB_FINDSTRINGEXACT,-1,(LPARAM)lpelfe->elfScript)==CB_ERR) {
		int i=SendMessage((HWND)lParam,CB_ADDSTRING,0,(LPARAM)lpelfe->elfScript);
		SendMessage((HWND)lParam,CB_SETITEMDATA,i,lpelfe->elfLogFont.lfCharSet);
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
		g_dwFillFontListThreadID=(DWORD)mir_forkthread(FillFontListThread,hwndDlg);				
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


		CheckDlgButton(hwndDlg,IDC_HILIGHTMODE,DBGetContactSettingByte(NULL,"CLC","HiLightMode",SETTING_HILIGHTMODE_DEFAULT)==0?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hwndDlg,IDC_HILIGHTMODE1,DBGetContactSettingByte(NULL,"CLC","HiLightMode",SETTING_HILIGHTMODE_DEFAULT)==1?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hwndDlg,IDC_HILIGHTMODE2,DBGetContactSettingByte(NULL,"CLC","HiLightMode",SETTING_HILIGHTMODE_DEFAULT)==2?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hwndDlg,IDC_HILIGHTMODE3,DBGetContactSettingByte(NULL,"CLC","HiLightMode",SETTING_HILIGHTMODE_DEFAULT)==3?BST_CHECKED:BST_UNCHECKED);

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
		{	
			LOGFONT lf={0};
			int i;
			HDC hdc=GetDC(hwndDlg);
			lf.lfCharSet=DEFAULT_CHARSET;
			GetDlgItemText(hwndDlg,IDC_TYPEFACE,lf.lfFaceName,sizeof(lf.lfFaceName));
			lf.lfPitchAndFamily=0;
			SendDlgItemMessage(hwndDlg,IDC_SCRIPT,CB_RESETCONTENT,0,0);
			EnumFontFamiliesEx(hdc,&lf,(FONTENUMPROC)EnumFontScriptsProc,(LPARAM)GetDlgItem(hwndDlg,IDC_SCRIPT),0);
			ReleaseDC(hwndDlg,hdc);
			for(i=SendDlgItemMessage(hwndDlg,IDC_SCRIPT,CB_GETCOUNT,0,0)-1;i>=0;i--) 
			{
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
			HBITMAP hbmp=ske_CreateDIB32(dis->rcItem.right-dis->rcItem.left,dis->rcItem.bottom-dis->rcItem.top);
			HBITMAP obmp=SelectObject(hdc,hbmp);
			HFONT oldFnt=SelectObject(hdc,hFontSample);
			RECT rc={0};
			rc.right=dis->rcItem.right-dis->rcItem.left;
			rc.bottom=dis->rcItem.bottom-dis->rcItem.top;
			FillRect(hdc,&rc,hBrush);
			ske_SetRectOpaque(hdc,&rc);
			SetTextColor(hdc,ColorSample);
			ske_SelectTextEffect(hdc,EffectSample-1,Color1Sample,Color2Sample);
			ske_DrawText(hdc,TranslateT("Sample"),lstrlen(TranslateT("Sample")),&rc,DT_CENTER|DT_VCENTER);
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
TCHAR *sortby[]={_T("Name"), _T("Name (use locale settings)") , _T("Status"), _T("Last message time"), _T("Protocol"),_T("Rate"), _T("-Nothing-")};
int sortbyValue[]={ SORTBY_NAME, SORTBY_NAME_LOCALE, SORTBY_STATUS, SORTBY_LASTMSG, SORTBY_PROTO ,SORTBY_RATE , SORTBY_NOTHING };
static BOOL 
CALLBACK DlgProcGenOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_USER+1:
		{
			HANDLE hContact=(HANDLE)wParam;
			DBCONTACTWRITESETTING * ws = (DBCONTACTWRITESETTING *)lParam;
			if ( hContact == NULL && ws != NULL && ws->szModule != NULL && ws->szSetting != NULL
				&& _strcmpi(ws->szModule,"CList")==0 && _strcmpi(ws->szSetting,"UseGroups")==0
				&& IsWindowVisible(hwndDlg) ) {
					CheckDlgButton(hwndDlg,IDC_DISABLEGROUPS,ws->value.bVal == 0);
				}
				break;
		}
	case WM_DESTROY: 
		{
			UnhookEvent( (HANDLE)GetWindowLong(hwndDlg,GWL_USERDATA) );
			break;
		}

	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		SetWindowLong(hwndDlg, GWL_USERDATA, (long)HookEventMessage(ME_DB_CONTACT_SETTINGCHANGED,hwndDlg,WM_USER+1));

		CheckDlgButton(hwndDlg, IDC_ONTOP, DBGetContactSettingByte(NULL,"CList","OnTop",SETTING_ONTOP_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_HIDEOFFLINE, DBGetContactSettingByte(NULL,"CList","HideOffline",SETTING_HIDEOFFLINE_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_HIDEEMPTYGROUPS, DBGetContactSettingByte(NULL,"CList","HideEmptyGroups",SETTING_HIDEEMPTYGROUPS_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_DISABLEGROUPS, DBGetContactSettingByte(NULL,"CList","UseGroups",SETTING_USEGROUPS_DEFAULT) ? BST_UNCHECKED : BST_CHECKED);
		{
			int i, item;
			int s1, s2, s3;
			for (i=0; i<sizeof(sortby)/sizeof(char*); i++) 
			{
				item=SendDlgItemMessage(hwndDlg,IDC_CLSORT1,CB_ADDSTRING,0,(LPARAM)TranslateTS(sortby[i]));
				SendDlgItemMessage(hwndDlg,IDC_CLSORT1,CB_SETITEMDATA,item,(LPARAM)0);
				item=SendDlgItemMessage(hwndDlg,IDC_CLSORT2,CB_ADDSTRING,0,(LPARAM)TranslateTS(sortby[i]));
				SendDlgItemMessage(hwndDlg,IDC_CLSORT2,CB_SETITEMDATA,item,(LPARAM)0);
				item=SendDlgItemMessage(hwndDlg,IDC_CLSORT3,CB_ADDSTRING,0,(LPARAM)TranslateTS(sortby[i]));
				SendDlgItemMessage(hwndDlg,IDC_CLSORT3,CB_SETITEMDATA,item,(LPARAM)0);

			}
			s1=DBGetContactSettingByte(NULL,"CList","SortBy1",SETTING_SORTBY1_DEFAULT);
			s2=DBGetContactSettingByte(NULL,"CList","SortBy2",SETTING_SORTBY2_DEFAULT);
			s3=DBGetContactSettingByte(NULL,"CList","SortBy3",SETTING_SORTBY3_DEFAULT);

			for (i=0; i<sizeof(sortby)/sizeof(char*); i++) 
			{
				if (s1==sortbyValue[i])
					SendDlgItemMessage(hwndDlg,IDC_CLSORT1,CB_SETCURSEL,i,0);
				if (s2==sortbyValue[i])
					SendDlgItemMessage(hwndDlg,IDC_CLSORT2,CB_SETCURSEL,i,0);
				if (s3==sortbyValue[i])
					SendDlgItemMessage(hwndDlg,IDC_CLSORT3,CB_SETCURSEL,i,0);		
			}

		}
		CheckDlgButton(hwndDlg, IDC_NOOFFLINEMOVE, DBGetContactSettingByte(NULL,"CList","NoOfflineBottom",SETTING_NOOFFLINEBOTTOM_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_OFFLINETOROOT, DBGetContactSettingByte(NULL,"CList","PlaceOfflineToRoot",SETTING_PLACEOFFLINETOROOT_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);


		CheckDlgButton(hwndDlg, IDC_CONFIRMDELETE, DBGetContactSettingByte(NULL,"CList","ConfirmDelete",SETTING_CONFIRMDELETE_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_AUTOHIDE, DBGetContactSettingByte(NULL,"CList","AutoHide",SETTING_AUTOHIDE_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIME),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
		EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
		{
			DWORD caps=CallService(MS_CLUI_GETCAPS,CLUICAPS_FLAGS1,0);
			caps=CLUIF_HIDEEMPTYGROUPS|CLUIF_DISABLEGROUPS|CLUIF_HASONTOPOPTION|CLUIF_HASAUTOHIDEOPTION;
			if(!(caps&CLUIF_HIDEEMPTYGROUPS)) CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_HIDEEMPTYGROUPS),SW_HIDE);
			if(!(caps&CLUIF_DISABLEGROUPS)) CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_DISABLEGROUPS),SW_HIDE);
			if(caps&CLUIF_HASONTOPOPTION) CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_ONTOP),SW_HIDE);
			if(caps&CLUIF_HASAUTOHIDEOPTION) {
				CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_AUTOHIDE),SW_HIDE);
				CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_HIDETIME),SW_HIDE);
				CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN),SW_HIDE);
				CLUI_ShowWindowMod(GetDlgItem(hwndDlg,IDC_STAUTOHIDESECS),SW_HIDE);
			}
		}
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN,UDM_SETRANGE,0,MAKELONG(900,1));
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","HideTime",SETTING_HIDETIME_DEFAULT),0));
		CheckDlgButton(hwndDlg, IDC_ONECLK, DBGetContactSettingByte(NULL,"CList","Tray1Click",SETTING_TRAY1CLICK_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		{
			BYTE trayOption=DBGetContactSettingByte(NULL,"CLUI","XStatusTray",SETTING_TRAYOPTION_DEFAULT);
			CheckDlgButton(hwndDlg, IDC_SHOWXSTATUS, (trayOption&3) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SHOWNORMAL,  (trayOption&2) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_TRANSPARENTOVERLAY, (trayOption&4) ? BST_CHECKED : BST_UNCHECKED);

			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWNORMAL),IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS));
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSPARENTOVERLAY),IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS)&&IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL));

		}
		CheckDlgButton(hwndDlg, IDC_ALWAYSSTATUS, DBGetContactSettingByte(NULL,"CList","AlwaysStatus",SETTING_ALWAYSSTATUS_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);

		CheckDlgButton(hwndDlg, IDC_ALWAYSPRIMARY, !DBGetContactSettingByte(NULL,"CList","AlwaysPrimary",SETTING_ALWAYSPRIMARY_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);

		CheckDlgButton(hwndDlg, IDC_ALWAYSMULTI, !DBGetContactSettingByte(NULL,"CList","AlwaysMulti",SETTING_ALWAYSMULTI_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_DONTCYCLE, DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)==SETTING_TRAYICON_SINGLE ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CYCLE, DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)==SETTING_TRAYICON_CYCLE ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_MULTITRAY, DBGetContactSettingByte(NULL,"CList","TrayIcon",SETTING_TRAYICON_DEFAULT)==SETTING_TRAYICON_MULTI ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_DISABLEBLINK, DBGetContactSettingByte(NULL,"CList","DisableTrayFlash",SETTING_DISABLETRAYFLASH_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED);
		//			CheckDlgButton(hwndDlg, IDC_ICONBLINK, DBGetContactSettingByte(NULL,"CList","NoIconBlink",SETTING_NOICONBLINF_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton(hwndDlg, IDC_SHOW_AVATARS, DBGetContactSettingByte(NULL,"CList","AvatarsShow",SETTINGS_SHOWAVATARS_DEFAULT) == 1 ? BST_CHECKED : BST_UNCHECKED );
		CheckDlgButton(hwndDlg, IDC_SHOW_ANIAVATARS, DBGetContactSettingByte(NULL,"CList","AvatarsAnimated",(ServiceExists(MS_AV_GETAVATARBITMAP)&&!g_CluiData.fGDIPlusFail)) == 1 ? BST_CHECKED : BST_UNCHECKED );

		if(IsDlgButtonChecked(hwndDlg,IDC_DONTCYCLE)) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_CYCLETIMESPIN),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_CYCLETIME),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_ALWAYSMULTI),FALSE);
		}
		if(IsDlgButtonChecked(hwndDlg,IDC_CYCLE)) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_PRIMARYSTATUS),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_ALWAYSMULTI),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_ALWAYSPRIMARY),FALSE);
		}
		if(IsDlgButtonChecked(hwndDlg,IDC_MULTITRAY)) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_CYCLETIMESPIN),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_CYCLETIME),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_PRIMARYSTATUS),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_ALWAYSPRIMARY),FALSE);
		}
		SendDlgItemMessage(hwndDlg,IDC_CYCLETIMESPIN,UDM_SETRANGE,0,MAKELONG(120,1));
		SendDlgItemMessage(hwndDlg,IDC_CYCLETIMESPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","CycleTime",SETTING_CYCLETIME_DEFAULT),0));
		{	int i,count,item;
		PROTOCOLDESCRIPTOR **protos;
		char szName[64];
		DBVARIANT dbv={DBVT_DELETED};
		DBGetContactSetting(NULL,"CList","PrimaryStatus",&dbv);
		CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&count,(LPARAM)&protos);
		item=SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_ADDSTRING,0,(LPARAM)TranslateT("Global"));
		SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_SETITEMDATA,item,(LPARAM)0);
		for(i=0;i<count;i++) {
			if(protos[i]->type!=PROTOTYPE_PROTOCOL || CallProtoService(protos[i]->szName,PS_GETCAPS,PFLAGNUM_2,0)==0) continue;
			CallProtoService(protos[i]->szName,PS_GETNAME,sizeof(szName),(LPARAM)szName);
			item=SendDlgItemMessageA(hwndDlg,IDC_PRIMARYSTATUS,CB_ADDSTRING,0,(LPARAM)szName);
			SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_SETITEMDATA,item,(LPARAM)protos[i]);
			if((dbv.type==DBVT_ASCIIZ || dbv.type==DBVT_UTF8)&& !strcmp(dbv.pszVal,protos[i]->szName))
				SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_SETCURSEL,item,0);
		}
		DBFreeVariant(&dbv);
		}
		if(-1==(int)SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_GETCURSEL,0,0))
			SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_SETCURSEL,0,0);
		SendDlgItemMessage(hwndDlg,IDC_BLINKSPIN,UDM_SETBUDDY,(WPARAM)GetDlgItem(hwndDlg,IDC_BLINKTIME),0);		// set buddy			
		SendDlgItemMessage(hwndDlg,IDC_BLINKSPIN,UDM_SETRANGE,0,MAKELONG(0x3FFF,250));
		SendDlgItemMessage(hwndDlg,IDC_BLINKSPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","IconFlashTime",SETTING_ICONFLASHTIME_DEFAULT),0));
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam)==IDC_SHOWXSTATUS||LOWORD(wParam)==IDC_SHOWNORMAL)
		{
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWNORMAL),IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS));
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSPARENTOVERLAY),IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS)&&IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL));
		}
		if(LOWORD(wParam)==IDC_AUTOHIDE) 
		{
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIME),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
		}
		if(LOWORD(wParam)==IDC_DONTCYCLE || LOWORD(wParam)==IDC_CYCLE || LOWORD(wParam)==IDC_MULTITRAY) 
		{
			EnableWindow(GetDlgItem(hwndDlg,IDC_PRIMARYSTATUS),IsDlgButtonChecked(hwndDlg,IDC_DONTCYCLE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_CYCLETIME),IsDlgButtonChecked(hwndDlg,IDC_CYCLE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_CYCLETIMESPIN),IsDlgButtonChecked(hwndDlg,IDC_CYCLE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_ALWAYSMULTI),IsDlgButtonChecked(hwndDlg,IDC_MULTITRAY));
			EnableWindow(GetDlgItem(hwndDlg,IDC_ALWAYSPRIMARY),IsDlgButtonChecked(hwndDlg,IDC_DONTCYCLE));
		}
		if((LOWORD(wParam)==IDC_HIDETIME || LOWORD(wParam)==IDC_CYCLETIME) && HIWORD(wParam)!=EN_CHANGE) break;
		if(LOWORD(wParam)==IDC_PRIMARYSTATUS && HIWORD(wParam)!=CBN_SELCHANGE) break;
		if((LOWORD(wParam)==IDC_HIDETIME || LOWORD(wParam)==IDC_CYCLETIME) && (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus())) return 0;
		if (LOWORD(wParam)==IDC_BLINKTIME && (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus())) return 0; // dont make apply enabled during buddy set crap
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->idFrom) 
		{
		case 0:
			switch (((LPNMHDR)lParam)->code)
			{
			case PSN_APPLY:
				DBWriteContactSettingByte(NULL,"CList","OnDesktop",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ONDESKTOP));
				DBWriteContactSettingByte(NULL,"CList","HideOffline",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_HIDEOFFLINE));
				{	DWORD caps=CallService(MS_CLUI_GETCAPS,CLUICAPS_FLAGS1,0);
				if(caps&CLUIF_HIDEEMPTYGROUPS) DBWriteContactSettingByte(NULL,"CList","HideEmptyGroups",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_HIDEEMPTYGROUPS));
				if(caps&CLUIF_DISABLEGROUPS) DBWriteContactSettingByte(NULL,"CList","UseGroups",(BYTE)!IsDlgButtonChecked(hwndDlg,IDC_DISABLEGROUPS));
				if(!(caps&CLUIF_HASONTOPOPTION)) {
					DBWriteContactSettingByte(NULL,"CList","OnTop",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ONTOP));
					SetWindowPos(pcli->hwndContactList, IsDlgButtonChecked(hwndDlg,IDC_ONTOP)?HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
				}
				if(!(caps&CLUIF_HASAUTOHIDEOPTION)) {
					DBWriteContactSettingByte(NULL,"CList","AutoHide",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
					DBWriteContactSettingWord(NULL,"CList","HideTime",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN,UDM_GETPOS,0,0));
				}
				}
				{
					int s1=SendDlgItemMessage(hwndDlg,IDC_CLSORT1,CB_GETCURSEL,0,0);
					int s2=SendDlgItemMessage(hwndDlg,IDC_CLSORT2,CB_GETCURSEL,0,0);
					int s3=SendDlgItemMessage(hwndDlg,IDC_CLSORT3,CB_GETCURSEL,0,0);
					if (s1>=0) s1=sortbyValue[s1];
					if (s2>=0) s2=sortbyValue[s2];
					if (s3>=0) s3=sortbyValue[s3];
					DBWriteContactSettingByte(NULL,"CList","SortBy1",(BYTE)s1);
					DBWriteContactSettingByte(NULL,"CList","SortBy2",(BYTE)s2);
					DBWriteContactSettingByte(NULL,"CList","SortBy3",(BYTE)s3);
				}
				DBWriteContactSettingByte(NULL,"CList","NoOfflineBottom",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_NOOFFLINEMOVE));
				DBWriteContactSettingByte(NULL,"CList","PlaceOfflineToRoot",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_OFFLINETOROOT));

				DBWriteContactSettingByte(NULL,"CList","ConfirmDelete",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_CONFIRMDELETE));
				DBWriteContactSettingByte(NULL,"CList","Tray1Click",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ONECLK));
				DBWriteContactSettingByte(NULL,"CList","AlwaysStatus",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ALWAYSSTATUS));
				DBWriteContactSettingByte(NULL,"CList","AlwaysMulti",(BYTE)!IsDlgButtonChecked(hwndDlg,IDC_ALWAYSMULTI));
				DBWriteContactSettingByte(NULL,"CList","AlwaysPrimary",(BYTE)!IsDlgButtonChecked(hwndDlg,IDC_ALWAYSPRIMARY));
				DBWriteContactSettingByte(NULL,"CList","TrayIcon",(BYTE)(IsDlgButtonChecked(hwndDlg,IDC_DONTCYCLE)?SETTING_TRAYICON_SINGLE:(IsDlgButtonChecked(hwndDlg,IDC_CYCLE)?SETTING_TRAYICON_CYCLE:SETTING_TRAYICON_MULTI)));
				DBWriteContactSettingWord(NULL,"CList","CycleTime",(WORD)SendDlgItemMessage(hwndDlg,IDC_CYCLETIMESPIN,UDM_GETPOS,0,0));
				DBWriteContactSettingWord(NULL,"CList","IconFlashTime",(WORD)SendDlgItemMessage(hwndDlg,IDC_BLINKSPIN,UDM_GETPOS,0,0));
				DBWriteContactSettingByte(NULL,"CList","DisableTrayFlash",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_DISABLEBLINK));

				{
					BYTE xOptions=0;
					xOptions=IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS)?1:0;
					xOptions|=(xOptions && IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL))?2:0;
					xOptions|=(xOptions && IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENTOVERLAY))?4:0;
					DBWriteContactSettingByte(NULL,"CLUI","XStatusTray",xOptions);				
				}
				if (!SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_GETCURSEL,0,0),0)) 
					DBDeleteContactSetting(NULL, "CList","PrimaryStatus");
				else DBWriteContactSettingString(NULL,"CList","PrimaryStatus",((PROTOCOLDESCRIPTOR*)SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_PRIMARYSTATUS,CB_GETCURSEL,0,0),0))->szName);
				pcli->pfnTrayIconIconsChanged();
				pcli->pfnLoadContactTree(); /* this won't do job properly since it only really works when changes happen */
				SendMessage(pcli->hwndContactTree,CLM_AUTOREBUILD,0,0); /* force reshuffle */
				ClcOptionsChanged(); // Used to force loading avatar an list height related options
				return TRUE;
			}
			break;
		}
		break;
	}
	return FALSE;
}



HWND g_hCLUIOptionsWnd=NULL;

static BOOL CALLBACK DlgProcSBarOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static LOGFONTA lf;
	switch (msg)
	{
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		CheckDlgButton(hwndDlg, IDC_SHOWSBAR, DBGetContactSettingByte(NULL,"CLUI","ShowSBar",SETTING_SHOWSBAR_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_USECONNECTINGICON, DBGetContactSettingByte(NULL,"CLUI","UseConnectingIcon",SETTING_USECONNECTINGICON_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SHOWXSTATUSNAME, ((DBGetContactSettingByte(NULL,"CLUI","ShowXStatus",SETTING_SHOWXSTATUS_DEFAULT)&8)>0) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SHOWXSTATUS, ((DBGetContactSettingByte(NULL,"CLUI","ShowXStatus",SETTING_SHOWXSTATUS_DEFAULT)&3)>0) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SHOWNORMAL, ((DBGetContactSettingByte(NULL,"CLUI","ShowXStatus",SETTING_SHOWXSTATUS_DEFAULT)&3)==2) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SHOWBOTH, ((DBGetContactSettingByte(NULL,"CLUI","ShowXStatus",SETTING_SHOWXSTATUS_DEFAULT)&3)==3) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SHOWUNREADEMAIL, (DBGetContactSettingByte(NULL,"CLUI","ShowUnreadEmails",SETTING_SHOWUNREADEMAILS_DEFAULT)==1) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_TRANSPARENTOVERLAY, ((DBGetContactSettingByte(NULL,"CLUI","ShowXStatus",SETTING_SHOWXSTATUS_DEFAULT)&4)) ? BST_CHECKED : BST_UNCHECKED);
		{
			BYTE showOpts=DBGetContactSettingByte(NULL,"CLUI","SBarShow",SETTING_SBARSHOW_DEFAULT);
			CheckDlgButton(hwndDlg, IDC_SHOWICON, showOpts&1 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SHOWPROTO, showOpts&2 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SHOWSTATUS, showOpts&4 ? BST_CHECKED : BST_UNCHECKED);
		}
		CheckDlgButton(hwndDlg, IDC_RIGHTSTATUS, DBGetContactSettingByte(NULL,"CLUI","SBarRightClk",SETTING_SBARRIGHTCLK_DEFAULT) ? BST_UNCHECKED : BST_CHECKED);
		CheckDlgButton(hwndDlg, IDC_RIGHTMIRANDA, !IsDlgButtonChecked(hwndDlg,IDC_RIGHTSTATUS) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_EQUALSECTIONS, DBGetContactSettingByte(NULL,"CLUI","EqualSections",SETTING_EQUALSECTIONS_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);

		SendDlgItemMessage(hwndDlg,IDC_MULTI_SPIN,UDM_SETRANGE,0,MAKELONG(50,0));
		SendDlgItemMessage(hwndDlg,IDC_MULTI_SPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingByte(NULL,"Protocols","ProtosPerLine",SETTING_PROTOSPERLINE_DEFAULT),0));

		SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN,UDM_SETRANGE,0,MAKELONG(50,0));
		SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingDword(NULL,"CLUI","LeftOffset",SETTING_LEFTOFFSET_DEFAULT),0));

		SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN2,UDM_SETRANGE,0,MAKELONG(50,0));
		SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN2,UDM_SETPOS,0,MAKELONG(DBGetContactSettingDword(NULL,"CLUI","RightOffset",SETTING_RIGHTOFFSET_DEFAULT),0));

		SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN3,UDM_SETRANGE,0,MAKELONG(50,0));
		SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN3,UDM_SETPOS,0,MAKELONG(DBGetContactSettingDword(NULL,"CLUI","SpaceBetween",SETTING_SPACEBETWEEN_DEFAULT),2));

		{
			int i, item;
			TCHAR *align[]={_T("Left"), _T("Center"), _T("Right")};
			for (i=0; i<sizeof(align)/sizeof(char*); i++) {
				item=SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_ADDSTRING,0,(LPARAM)TranslateTS(align[i]));
				SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_SETCURSEL,DBGetContactSettingByte(NULL,"CLUI","Align",SETTING_ALIGN_DEFAULT),0);
			}
		}
		{
			int en=IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWICON),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWPROTO),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWSTATUS),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_RIGHTSTATUS),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_RIGHTMIRANDA),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_EQUALSECTIONS),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_USECONNECTINGICON),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_USEOWNERDRAW),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETSPIN),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETICON),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETSPIN2),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETICON2),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETSPIN3),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETICON3),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON1),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_COMBO2),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWXSTATUSNAME),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWXSTATUS),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWBOTH),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS) && !IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL));
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWNORMAL),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS)&& !IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH));
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSPARENTOVERLAY),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS) && IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL)&& !IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH));
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWUNREADEMAIL),en);

			EnableWindow(GetDlgItem(hwndDlg,IDC_MULTI),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_MULTI_2),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_MULTI_COUNT),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_MULTI_SPIN),en);
		}
		return TRUE;
	case WM_COMMAND:
		if(LOWORD(wParam)==IDC_BUTTON1)  
		{ 
			if (HIWORD(wParam)==BN_CLICKED)
			{
				CHOOSEFONTA fnt;
				memset(&fnt,0,sizeof(CHOOSEFONTA));
				fnt.lStructSize=sizeof(CHOOSEFONTA);
				fnt.hwndOwner=hwndDlg;
				fnt.Flags=CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT;
				fnt.lpLogFont=&lf;
				ChooseFontA(&fnt);
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
				return 0;
			} 
		}
		else if (LOWORD(wParam)==IDC_COLOUR ||(LOWORD(wParam)==IDC_COMBO2 && HIWORD(wParam)==CBN_SELCHANGE)) SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
		else if (LOWORD(wParam)==IDC_SHOWSBAR) {
			int en=IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWICON),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWPROTO),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWSTATUS),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_RIGHTSTATUS),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_RIGHTMIRANDA),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_EQUALSECTIONS),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_USECONNECTINGICON),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_USEOWNERDRAW),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETSPIN),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETICON),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETSPIN2),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETICON2),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETSPIN3),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_OFFSETICON3),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_COMBO2),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON1),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWXSTATUSNAME),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWXSTATUS),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWUNREADEMAIL),en);

			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWBOTH),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS) && !IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL));
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWNORMAL),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS)&& !IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH));
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSPARENTOVERLAY),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS) && IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL)&& !IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH));

			EnableWindow(GetDlgItem(hwndDlg,IDC_MULTI),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_MULTI_2),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_MULTI_COUNT),en);
			EnableWindow(GetDlgItem(hwndDlg,IDC_MULTI_SPIN),en);

			SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);	  
		}
		else if (LOWORD(wParam)==IDC_SHOWXSTATUS)	
		{
			int en=IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWBOTH),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS) && !IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL));
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWNORMAL),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS)&& !IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH));
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSPARENTOVERLAY),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS) && IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL)&& !IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH));
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);	
		}
		else if (LOWORD(wParam)==IDC_SHOWBOTH)
		{
			int en=IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR);
			if (IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH))	CheckDlgButton(hwndDlg,IDC_SHOWNORMAL,FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWNORMAL),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS)&& !IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH));
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSPARENTOVERLAY),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS) && IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL)&& !IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH));
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);	
		}
		else if (LOWORD(wParam)==IDC_SHOWNORMAL)	
		{
			int en=IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR);
			if (IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL))	CheckDlgButton(hwndDlg,IDC_SHOWBOTH,FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWBOTH),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS) && !IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL));       
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSPARENTOVERLAY),en && IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS) && IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL)&& !IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH));

			SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);	
		}
		else if ( (LOWORD(wParam)==IDC_MULTI_COUNT||LOWORD(wParam)==IDC_OFFSETICON||LOWORD(wParam)==IDC_OFFSETICON2||LOWORD(wParam)==IDC_OFFSETICON3) 
			&& HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()) return 0; // dont make apply enabled during buddy set crap 
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case PSN_APPLY:
			{             
				//DBWriteContactSettingByte(NULL,"CLUI","ShowSBar",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR));
				DBWriteContactSettingByte(NULL,"CLUI","SBarShow",(BYTE)((IsDlgButtonChecked(hwndDlg,IDC_SHOWICON)?1:0)|(IsDlgButtonChecked(hwndDlg,IDC_SHOWPROTO)?2:0)|(IsDlgButtonChecked(hwndDlg,IDC_SHOWSTATUS)?4:0)));
				DBWriteContactSettingByte(NULL,"CLUI","SBarRightClk",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_RIGHTMIRANDA));
				DBWriteContactSettingByte(NULL,"CLUI","EqualSections",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_EQUALSECTIONS));
				DBWriteContactSettingByte(NULL,"CLUI","UseConnectingIcon",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_USECONNECTINGICON));
				DBWriteContactSettingDword(NULL,"CLUI","LeftOffset",(DWORD)SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN,UDM_GETPOS,0,0));
				DBWriteContactSettingDword(NULL,"CLUI","RightOffset",(DWORD)SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN2,UDM_GETPOS,0,0));
				DBWriteContactSettingDword(NULL,"CLUI","SpaceBetween",(DWORD)SendDlgItemMessage(hwndDlg,IDC_OFFSETSPIN3,UDM_GETPOS,0,0));
				DBWriteContactSettingByte(NULL,"CLUI","Align",(BYTE)SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_GETCURSEL,0,0));
				DBWriteContactSettingByte(NULL,"CLUI","ShowUnreadEmails",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWUNREADEMAIL));
				DBWriteContactSettingByte(NULL,"Protocols","ProtosPerLine",(BYTE)SendDlgItemMessage(hwndDlg,IDC_MULTI_SPIN,UDM_GETPOS,0,0));
				{
					BYTE val=0;
					if (IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUS))
					{
						if (IsDlgButtonChecked(hwndDlg,IDC_SHOWBOTH)) val=3;
						else if (IsDlgButtonChecked(hwndDlg,IDC_SHOWNORMAL)) val=2;
						else val=1;
						val+=IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENTOVERLAY)?4:0;
					}
					val+=IsDlgButtonChecked(hwndDlg,IDC_SHOWXSTATUSNAME)?8:0;
					DBWriteContactSettingByte(NULL,"CLUI","ShowXStatus",val);
				}	
				DBWriteContactSettingDword(NULL,"ModernData","StatusBarFontCol",SendDlgItemMessage(hwndDlg,IDC_COLOUR,CPM_GETCOLOUR,0,0));
				DBWriteContactSettingByte(NULL,"CLUI","ShowSBar",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWSBAR));

				LoadStatusBarData();
				CLUIServices_ProtocolStatusChanged(0,0);	
				return TRUE;
			}
		}
		break;
	}
	return FALSE;
}


static BOOL CALLBACK DlgProcCluiOpts2(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:

		TranslateDialogDefault(hwndDlg);
		CheckDlgButton(hwndDlg, IDC_CLIENTDRAG, DBGetContactSettingByte(NULL,"CLUI","ClientAreaDrag",SETTING_CLIENTDRAG_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_DRAGTOSCROLL, (DBGetContactSettingByte(NULL,"CLUI","DragToScroll",SETTING_DRAGTOSCROLL_DEFAULT)&&!DBGetContactSettingByte(NULL,"CLUI","ClientAreaDrag",SETTING_CLIENTDRAG_DEFAULT)) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_AUTOSIZE, g_CluiData.fAutoSize ? BST_CHECKED : BST_UNCHECKED);			
		CheckDlgButton(hwndDlg, IDC_LOCKSIZING, DBGetContactSettingByte(NULL,"CLUI","LockSize",SETTING_LOCKSIZE_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);			   
		CheckDlgButton(hwndDlg, IDC_BRINGTOFRONT, DBGetContactSettingByte(NULL,"CList","BringToFront",SETTING_BRINGTOFRONT_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);			   		


		SendDlgItemMessage(hwndDlg,IDC_MAXSIZESPIN,UDM_SETRANGE,0,MAKELONG(100,0));
		SendDlgItemMessage(hwndDlg,IDC_MAXSIZESPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","MaxSizeHeight",SETTING_MAXSIZEHEIGHT_DEFAULT));
		CheckDlgButton(hwndDlg, IDC_AUTOSIZEUPWARD, DBGetContactSettingByte(NULL,"CLUI","AutoSizeUpward",SETTING_AUTOSIZEUPWARD_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SNAPTOEDGES, DBGetContactSettingByte(NULL,"CLUI","SnapToEdges",SETTING_SNAPTOEDGES_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);

		CheckDlgButton(hwndDlg, IDC_EVENTAREA_NONE, DBGetContactSettingByte(NULL,"CLUI","EventArea",SETTING_EVENTAREAMODE_DEFAULT)==0 ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_EVENTAREA, DBGetContactSettingByte(NULL,"CLUI","EventArea",SETTING_EVENTAREAMODE_DEFAULT)==1 ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_EVENTAREA_ALWAYS, DBGetContactSettingByte(NULL,"CLUI","EventArea",SETTING_EVENTAREAMODE_DEFAULT)==2 ? BST_CHECKED : BST_UNCHECKED);

		CheckDlgButton(hwndDlg, IDC_AUTOHIDE, DBGetContactSettingByte(NULL,"CList","AutoHide",SETTING_AUTOHIDE_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN,UDM_SETRANGE,0,MAKELONG(900,1));
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"CList","HideTime",SETTING_HIDETIME_DEFAULT),0));
		EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIME),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
		EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
		EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC01),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
		{
			int i, item;
			TCHAR *hidemode[]={TranslateT("Hide to tray"), TranslateT("Behind left edge"), TranslateT("Behind right edge")};
			for (i=0; i<sizeof(hidemode)/sizeof(char*); i++) {
				item=SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_ADDSTRING,0,(LPARAM)(hidemode[i]));
				SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_SETITEMDATA,item,(LPARAM)0);
				SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_SETCURSEL,DBGetContactSettingByte(NULL,"ModernData","HideBehind",SETTING_HIDEBEHIND_DEFAULT),0);
			}
		}
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN2,UDM_SETRANGE,0,MAKELONG(600,0));
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN2,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"ModernData","ShowDelay",SETTING_SHOWDELAY_DEFAULT),0));
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN3,UDM_SETRANGE,0,MAKELONG(600,0));
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN3,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"ModernData","HideDelay",SETTING_HIDEDELAY_DEFAULT),0));
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN4,UDM_SETRANGE,0,MAKELONG(50,1));
		SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN4,UDM_SETPOS,0,MAKELONG(DBGetContactSettingWord(NULL,"ModernData","HideBehindBorderSize",SETTING_HIDEBEHINDBORDERSIZE_DEFAULT),0));
		{
			int mode=SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_GETCURSEL,0,0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWDELAY),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDEDELAY),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN2),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN3),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN4),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDEDELAY2),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC5),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC6),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC7),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC8),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC10),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC13),mode!=0);
		}

		if(!IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE)) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC21),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC22),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_MAXSIZEHEIGHT),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_MAXSIZESPIN),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_AUTOSIZEUPWARD),FALSE);
		}
		return TRUE;
	case WM_COMMAND:
		if(LOWORD(wParam)==IDC_AUTOHIDE) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIME),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC01),IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
		}
		else if(LOWORD(wParam)==IDC_DRAGTOSCROLL && IsDlgButtonChecked(hwndDlg,IDC_CLIENTDRAG)) {
			CheckDlgButton(hwndDlg,IDC_CLIENTDRAG,FALSE);
		}
		else if(LOWORD(wParam)==IDC_CLIENTDRAG && IsDlgButtonChecked(hwndDlg,IDC_DRAGTOSCROLL)) {
			CheckDlgButton(hwndDlg,IDC_DRAGTOSCROLL,FALSE);
		}
		else if(LOWORD(wParam)==IDC_AUTOSIZE) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC21),IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC22),IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_MAXSIZEHEIGHT),IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_MAXSIZESPIN),IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
			EnableWindow(GetDlgItem(hwndDlg,IDC_AUTOSIZEUPWARD),IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
		}
		else if (LOWORD(wParam)==IDC_HIDEMETHOD)
		{
			int mode=SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_GETCURSEL,0,0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWDELAY),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDEDELAY),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN2),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN3),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDETIMESPIN4),mode!=0);     
			EnableWindow(GetDlgItem(hwndDlg,IDC_HIDEDELAY2),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC5),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC6),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC7),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC8),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC10),mode!=0);
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC13),mode!=0);
		}
		if ((LOWORD(wParam)==IDC_HIDETIME || LOWORD(wParam)==IDC_HIDEDELAY2 ||LOWORD(wParam)==IDC_HIDEDELAY ||LOWORD(wParam)==IDC_SHOWDELAY || LOWORD(wParam)==IDC_MAXSIZEHEIGHT) &&
			(HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))
			return 0;
		// Enable apply button
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
		break;	
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case PSN_APPLY:
			//
			//DBWriteContactSettingByte(NULL,"CLUI","LeftClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_GETPOS,0,0));
			//DBWriteContactSettingByte(NULL,"CLUI","RightClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_RIGHTMARGINSPIN,UDM_GETPOS,0,0));
			//DBWriteContactSettingByte(NULL,"CLUI","TopClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_TOPMARGINSPIN,UDM_GETPOS,0,0));
			//DBWriteContactSettingByte(NULL,"CLUI","BottomClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_BOTTOMMARGINSPIN,UDM_GETPOS,0,0));
			//if (g_proc_UpdateLayeredWindow!=NULL && IsDlgButtonChecked(hwndDlg,IDC_LAYERENGINE)) 
			//	DBWriteContactSettingByte(NULL,"ModernData","EnableLayering",0);
			//else 
			//	DBDeleteContactSetting(NULL,"ModernData","EnableLayering");	
			DBWriteContactSettingByte(NULL,"ModernData","HideBehind",(BYTE)SendDlgItemMessage(hwndDlg,IDC_HIDEMETHOD,CB_GETCURSEL,0,0));  
			DBWriteContactSettingWord(NULL,"ModernData","ShowDelay",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN2,UDM_GETPOS,0,0));
			DBWriteContactSettingWord(NULL,"ModernData","HideDelay",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN3,UDM_GETPOS,0,0));
			DBWriteContactSettingWord(NULL,"ModernData","HideBehindBorderSize",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN4,UDM_GETPOS,0,0));

			DBWriteContactSettingByte(NULL,"CLUI","DragToScroll",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_DRAGTOSCROLL));
			DBWriteContactSettingByte(NULL,"CList","BringToFront",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_BRINGTOFRONT));
			g_mutex_bChangingMode=TRUE;
			DBWriteContactSettingByte(NULL,"CLUI","ClientAreaDrag",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_CLIENTDRAG));
			DBWriteContactSettingByte(NULL,"CLUI","AutoSize",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
			DBWriteContactSettingByte(NULL,"CLUI","LockSize",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_LOCKSIZING));
			DBWriteContactSettingByte(NULL,"CLUI","MaxSizeHeight",(BYTE)GetDlgItemInt(hwndDlg,IDC_MAXSIZEHEIGHT,NULL,FALSE));               
			DBWriteContactSettingByte(NULL,"CLUI","AutoSizeUpward",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZEUPWARD));
			DBWriteContactSettingByte(NULL,"CLUI","SnapToEdges",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SNAPTOEDGES));

			DBWriteContactSettingByte(NULL,"CLUI","EventArea",
				(BYTE)(IsDlgButtonChecked(hwndDlg,IDC_EVENTAREA_ALWAYS)?2:(BYTE)IsDlgButtonChecked(hwndDlg,IDC_EVENTAREA)?1:0));

			DBWriteContactSettingByte(NULL,"CList","AutoHide",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
			DBWriteContactSettingWord(NULL,"CList","HideTime",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN,UDM_GETPOS,0,0));			
			CLUI_ChangeWindowMode(); 
			SendMessage(pcli->hwndContactTree,WM_SIZE,0,0);	//forces it to send a cln_listsizechanged
			CLUI_ReloadCLUIOptions();
			EventArea_ConfigureEventArea();
			cliShowHide(0,1);
			g_mutex_bChangingMode=FALSE;
			return TRUE;
		}
		break;
	}
	return FALSE;
}

static BOOL CALLBACK DlgProcCluiOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL fEnabled=FALSE;
	switch (msg)
	{
	case WM_INITDIALOG:		
		
		TranslateDialogDefault(hwndDlg);
		g_hCLUIOptionsWnd=hwndDlg;
		CheckDlgButton(hwndDlg, IDC_ONTOP, DBGetContactSettingByte(NULL,"CList","OnTop",SETTING_ONTOP_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);		
		{	// ====== Activate/Deactivate Non-Layered items =======
			fEnabled=!g_CluiData.fLayered || g_CluiData.fDisableSkinEngine;
			EnableWindow(GetDlgItem(hwndDlg,IDC_TOOLWND),fEnabled);
			EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),fEnabled);
			EnableWindow(GetDlgItem(hwndDlg,IDC_BORDER),fEnabled);
			EnableWindow(GetDlgItem(hwndDlg,IDC_NOBORDERWND),fEnabled);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWCAPTION),fEnabled);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWMAINMENU),fEnabled);
			EnableWindow(GetDlgItem(hwndDlg,IDC_TITLETEXT),fEnabled);
			EnableWindow(GetDlgItem(hwndDlg,IDC_DROPSHADOW),fEnabled);
			EnableWindow(GetDlgItem(hwndDlg,IDC_TITLEBAR_STATIC),fEnabled);
			EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR_KEY),!fEnabled);
			EnableWindow(GetDlgItem(hwndDlg,IDC_CHECKKEYCOLOR),!fEnabled);
			EnableWindow(GetDlgItem(hwndDlg,IDC_ROUNDCORNERS),fEnabled);
		}
		{   // ====== Non-Layered Mode =====
			CheckDlgButton(hwndDlg, IDC_TOOLWND, DBGetContactSettingByte(NULL,"CList","ToolWindow",SETTING_TOOLWINDOW_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_MIN2TRAY, DBGetContactSettingByte(NULL,"CList","Min2Tray",SETTING_MIN2TRAY_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_BORDER, DBGetContactSettingByte(NULL,"CList","ThinBorder",SETTING_THINBORDER_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_NOBORDERWND, DBGetContactSettingByte(NULL,"CList","NoBorder",SETTING_NOBORDER_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			if(IsDlgButtonChecked(hwndDlg,IDC_TOOLWND))
				EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),FALSE);
			CheckDlgButton(hwndDlg, IDC_SHOWCAPTION, DBGetContactSettingByte(NULL,"CLUI","ShowCaption",SETTING_SHOWCAPTION_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SHOWMAINMENU, DBGetContactSettingByte(NULL,"CLUI","ShowMainMenu",SETTING_SHOWMAINMENU_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			//EnableWindow(GetDlgItem(hwndDlg,IDC_CLIENTDRAG),!IsDlgButtonChecked(hwndDlg,IDC_DRAGTOSCROLL));
			if(!IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION)) 
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TOOLWND),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TITLETEXT),FALSE);
			}
			if (IsDlgButtonChecked(hwndDlg,IDC_BORDER) || IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND))
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TOOLWND),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_TITLETEXT),FALSE);
				EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWCAPTION),FALSE);				
			}
			CheckDlgButton(hwndDlg, IDC_DROPSHADOW, DBGetContactSettingByte(NULL,"CList","WindowShadow",SETTING_WINDOWSHADOW_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_ROUNDCORNERS, DBGetContactSettingByte(NULL,"CLC","RoundCorners",SETTING_ROUNDCORNERS_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);	  
		}   // ====== End of Non-Layered Mode =====

		CheckDlgButton(hwndDlg, IDC_FADEINOUT, DBGetContactSettingByte(NULL,"CLUI","FadeInOut",SETTING_FADEIN_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_ONDESKTOP, DBGetContactSettingByte(NULL,"CList","OnDesktop", SETTING_ONDESKTOP_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		SendDlgItemMessage(hwndDlg,IDC_FRAMESSPIN,UDM_SETRANGE,0,MAKELONG(10,0));
		SendDlgItemMessage(hwndDlg,IDC_CAPTIONSSPIN,UDM_SETRANGE,0,MAKELONG(10,0));
		SendDlgItemMessage(hwndDlg,IDC_FRAMESSPIN,UDM_SETPOS,0,DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenFrames",SETTING_GAPFRAMES_DEFAULT));
		SendDlgItemMessage(hwndDlg,IDC_CAPTIONSSPIN,UDM_SETPOS,0,DBGetContactSettingDword(NULL,"CLUIFrames","GapBetweenTitleBar",SETTING_GAPTITLEBAR_DEFAULT));
		SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_SETRANGE,0,MAKELONG(250,0));
		SendDlgItemMessage(hwndDlg,IDC_RIGHTMARGINSPIN,UDM_SETRANGE,0,MAKELONG(250,0));
		SendDlgItemMessage(hwndDlg,IDC_TOPMARGINSPIN,UDM_SETRANGE,0,MAKELONG(250,0));
		SendDlgItemMessage(hwndDlg,IDC_BOTTOMMARGINSPIN,UDM_SETRANGE,0,MAKELONG(250,0));
		SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","LeftClientMargin",SETTING_LEFTCLIENTMARIGN_DEFAULT));
		SendDlgItemMessage(hwndDlg,IDC_RIGHTMARGINSPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","RightClientMargin",SETTING_RIGHTCLIENTMARIGN_DEFAULT));
		SendDlgItemMessage(hwndDlg,IDC_TOPMARGINSPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","TopClientMargin",SETTING_TOPCLIENTMARIGN_DEFAULT));
		SendDlgItemMessage(hwndDlg,IDC_BOTTOMMARGINSPIN,UDM_SETPOS,0,DBGetContactSettingByte(NULL,"CLUI","BottomClientMargin",SETTING_BOTTOMCLIENTMARIGN_DEFAULT));
		
		CheckDlgButton(hwndDlg, IDC_DISABLEENGINE, DBGetContactSettingByte(NULL,"ModernData","DisableEngine", SETTING_DISABLESKIN_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);		
		
		EnableWindow(GetDlgItem(hwndDlg,IDC_LAYERENGINE),(g_proc_UpdateLayeredWindow!=NULL && !DBGetContactSettingByte(NULL,"ModernData","DisableEngine", SETTING_DISABLESKIN_DEFAULT))?TRUE:FALSE);
		CheckDlgButton(hwndDlg, IDC_LAYERENGINE, ((DBGetContactSettingByte(NULL,"ModernData","EnableLayering",SETTING_ENABLELAYERING_DEFAULT)&&g_proc_UpdateLayeredWindow!=NULL) && !DBGetContactSettingByte(NULL,"ModernData","DisableEngine", SETTING_DISABLESKIN_DEFAULT)) ? BST_UNCHECKED:BST_CHECKED);   

		CheckDlgButton(hwndDlg, IDC_CHECKKEYCOLOR, DBGetContactSettingByte(NULL,"ModernSettings","UseKeyColor",SETTING_USEKEYCOLOR_DEFAULT) ? BST_CHECKED : BST_UNCHECKED);
		EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR_KEY),!fEnabled&&IsDlgButtonChecked(hwndDlg,IDC_CHECKKEYCOLOR));
		SendDlgItemMessage(hwndDlg,IDC_COLOUR_KEY,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"ModernSettings","KeyColor",(DWORD)SETTING_KEYCOLOR_DEFAULT));	
		{
			DBVARIANT dbv={0};
			TCHAR *s=NULL;
			char szUin[20];
			if(!DBGetContactSettingTString(NULL,"CList","TitleText",&dbv))
				s=mir_tstrdup(dbv.ptszVal);
			else
				s=mir_tstrdup(_T(MIRANDANAME));
			//dbv.pszVal=s;
			SetDlgItemText(hwndDlg,IDC_TITLETEXT,s);
			if (s) mir_free_and_nill(s);
			DBFreeVariant(&dbv);
			//if (s) mir_free_and_nill(s);
			SendDlgItemMessage(hwndDlg,IDC_TITLETEXT,CB_ADDSTRING,0,(LPARAM)MIRANDANAME);
			sprintf(szUin,"%u",DBGetContactSettingDword(NULL,"ICQ","UIN",0));
			SendDlgItemMessage(hwndDlg,IDC_TITLETEXT,CB_ADDSTRING,0,(LPARAM)szUin);

			if(!DBGetContactSetting(NULL,"ICQ","Nick",&dbv)) {
				SendDlgItemMessage(hwndDlg,IDC_TITLETEXT,CB_ADDSTRING,0,(LPARAM)dbv.pszVal);
				//mir_free_and_nill(dbv.pszVal);
				DBFreeVariant(&dbv);
				dbv.pszVal=NULL;
			}
			if(!DBGetContactSetting(NULL,"ICQ","FirstName",&dbv)) {
				SendDlgItemMessage(hwndDlg,IDC_TITLETEXT,CB_ADDSTRING,0,(LPARAM)dbv.pszVal);
				//mir_free_and_nill(dbv.pszVal);
				DBFreeVariant(&dbv);
				dbv.pszVal=NULL;
			}
			if(!DBGetContactSetting(NULL,"ICQ","e-mail",&dbv)) {
				SendDlgItemMessage(hwndDlg,IDC_TITLETEXT,CB_ADDSTRING,0,(LPARAM)dbv.pszVal);
				//mir_free_and_nill(dbv.pszVal);
				DBFreeVariant(&dbv);
				dbv.pszVal=NULL;
			}
		}
		if(!IsWinVer2000Plus()) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_FADEINOUT),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSPARENT),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_DROPSHADOW),FALSE);
		}
		else CheckDlgButton(hwndDlg,IDC_TRANSPARENT,DBGetContactSettingByte(NULL,"CList","Transparent",SETTING_TRANSPARENT_DEFAULT)?BST_CHECKED:BST_UNCHECKED);
		if(!IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT)) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC11),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC12),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSACTIVE),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSINACTIVE),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_ACTIVEPERC),FALSE);
			EnableWindow(GetDlgItem(hwndDlg,IDC_INACTIVEPERC),FALSE);
		}
		SendDlgItemMessage(hwndDlg,IDC_TRANSACTIVE,TBM_SETRANGE,FALSE,MAKELONG(1,255));
		SendDlgItemMessage(hwndDlg,IDC_TRANSINACTIVE,TBM_SETRANGE,FALSE,MAKELONG(1,255));
		SendDlgItemMessage(hwndDlg,IDC_TRANSACTIVE,TBM_SETPOS,TRUE,DBGetContactSettingByte(NULL,"CList","Alpha",SETTING_ALPHA_DEFAULT));
		SendDlgItemMessage(hwndDlg,IDC_TRANSINACTIVE,TBM_SETPOS,TRUE,DBGetContactSettingByte(NULL,"CList","AutoAlpha",SETTING_AUTOALPHA_DEFAULT));
		SendMessage(hwndDlg,WM_HSCROLL,0x12345678,0);
		return TRUE;

	case WM_COMMAND:
		if(LOWORD(wParam)==IDC_TRANSPARENT) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC11),IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
			EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC12),IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSACTIVE),IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
			EnableWindow(GetDlgItem(hwndDlg,IDC_TRANSINACTIVE),IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
			EnableWindow(GetDlgItem(hwndDlg,IDC_ACTIVEPERC),IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
			EnableWindow(GetDlgItem(hwndDlg,IDC_INACTIVEPERC),IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
		}
		else if(LOWORD(wParam)==IDC_LAYERENGINE || LOWORD(wParam)==IDC_DISABLEENGINE)
		{	// ====== Activate/Deactivate Non-Layered items =======
			fEnabled=!(IsWindowEnabled(GetDlgItem(hwndDlg,IDC_LAYERENGINE)) && !IsDlgButtonChecked(hwndDlg,IDC_LAYERENGINE) && !IsDlgButtonChecked(hwndDlg,IDC_DISABLEENGINE));

			EnableWindow(GetDlgItem(hwndDlg,IDC_TOOLWND),fEnabled&&(IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION))&&!(IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND)||IsDlgButtonChecked(hwndDlg,IDC_BORDER)));  
			EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),fEnabled&&(IsDlgButtonChecked(hwndDlg,IDC_TOOLWND) && IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION)) && !(IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND)||IsDlgButtonChecked(hwndDlg,IDC_BORDER)));  
			EnableWindow(GetDlgItem(hwndDlg,IDC_TITLETEXT),fEnabled&&(IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION))&&!(IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND)||IsDlgButtonChecked(hwndDlg,IDC_BORDER)));  
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWCAPTION),fEnabled&&!(IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND)||IsDlgButtonChecked(hwndDlg,IDC_BORDER)));             
			EnableWindow(GetDlgItem(hwndDlg,IDC_BORDER),fEnabled);
			EnableWindow(GetDlgItem(hwndDlg,IDC_NOBORDERWND),fEnabled);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWMAINMENU),fEnabled);
			EnableWindow(GetDlgItem(hwndDlg,IDC_DROPSHADOW),fEnabled);
			EnableWindow(GetDlgItem(hwndDlg,IDC_TITLEBAR_STATIC),fEnabled);
			EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR_KEY),!fEnabled);
			EnableWindow(GetDlgItem(hwndDlg,IDC_CHECKKEYCOLOR),!fEnabled);
			EnableWindow(GetDlgItem(hwndDlg,IDC_ROUNDCORNERS),fEnabled);
			if (LOWORD(wParam)==IDC_DISABLEENGINE)
			{
				EnableWindow(GetDlgItem(hwndDlg,IDC_LAYERENGINE),!IsDlgButtonChecked(hwndDlg,IDC_DISABLEENGINE) && g_proc_UpdateLayeredWindow!=NULL);
				if (IsDlgButtonChecked(hwndDlg,IDC_DISABLEENGINE))
					CheckDlgButton(hwndDlg,IDC_LAYERENGINE,BST_CHECKED);
			}

		}
		else if(LOWORD(wParam)==IDC_ONDESKTOP && IsDlgButtonChecked(hwndDlg,IDC_ONDESKTOP)) {
			CheckDlgButton(hwndDlg, IDC_ONTOP, BST_UNCHECKED);    
		}
		else if(LOWORD(wParam)==IDC_ONTOP && IsDlgButtonChecked(hwndDlg,IDC_ONTOP)) {
			CheckDlgButton(hwndDlg, IDC_ONDESKTOP, BST_UNCHECKED);    
		}
		else if(LOWORD(wParam)==IDC_TOOLWND) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),!IsDlgButtonChecked(hwndDlg,IDC_TOOLWND));
		}
		else if(LOWORD(wParam)==IDC_SHOWCAPTION) {
			EnableWindow(GetDlgItem(hwndDlg,IDC_TOOLWND),IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION));
			EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),!IsDlgButtonChecked(hwndDlg,IDC_TOOLWND) && IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION));
			EnableWindow(GetDlgItem(hwndDlg,IDC_TITLETEXT),IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION));
		}
		else if(LOWORD(wParam)==IDC_NOBORDERWND ||LOWORD(wParam)==IDC_BORDER)
		{
			EnableWindow(GetDlgItem(hwndDlg,IDC_TOOLWND),(IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION))&&!(IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND)||IsDlgButtonChecked(hwndDlg,IDC_BORDER)));  
			EnableWindow(GetDlgItem(hwndDlg,IDC_MIN2TRAY),(IsDlgButtonChecked(hwndDlg,IDC_TOOLWND) && IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION)) && !(IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND)||IsDlgButtonChecked(hwndDlg,IDC_BORDER)));  
			EnableWindow(GetDlgItem(hwndDlg,IDC_TITLETEXT),(IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION))&&!(IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND)||IsDlgButtonChecked(hwndDlg,IDC_BORDER)));  
			EnableWindow(GetDlgItem(hwndDlg,IDC_SHOWCAPTION),!(IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND)||IsDlgButtonChecked(hwndDlg,IDC_BORDER)));             
			if (LOWORD(wParam)==IDC_BORDER) CheckDlgButton(hwndDlg, IDC_NOBORDERWND,BST_UNCHECKED);
			else CheckDlgButton(hwndDlg, IDC_BORDER,BST_UNCHECKED); 

		}
		else if (LOWORD(wParam)==IDC_CHECKKEYCOLOR) 
		{
			EnableWindow(GetDlgItem(hwndDlg,IDC_COLOUR_KEY),IsDlgButtonChecked(hwndDlg,IDC_CHECKKEYCOLOR));
			SendDlgItemMessage(hwndDlg,IDC_COLOUR_KEY,CPM_SETCOLOUR,0,DBGetContactSettingDword(NULL,"ModernSettings","KeyColor",(DWORD)SETTING_KEYCOLOR_DEFAULT));
		}
		if ((LOWORD(wParam)==IDC_TITLETEXT || LOWORD(wParam)==IDC_MAXSIZEHEIGHT || LOWORD(wParam)==IDC_FRAMESGAP || LOWORD(wParam)==IDC_CAPTIONSGAP ||
			LOWORD(wParam)==IDC_LEFTMARGIN || LOWORD(wParam)==IDC_RIGHTMARGIN|| LOWORD(wParam)==IDC_TOPMARGIN || LOWORD(wParam)==IDC_BOTTOMMARGIN) 
			&&(HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))
			return 0;
		// Enable apply button
		SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
		break;

	case WM_HSCROLL:
		{	char str[10];
		sprintf(str,"%d%%",100*SendDlgItemMessage(hwndDlg,IDC_TRANSINACTIVE,TBM_GETPOS,0,0)/255);
		SetDlgItemTextA(hwndDlg,IDC_INACTIVEPERC,str);
		sprintf(str,"%d%%",100*SendDlgItemMessage(hwndDlg,IDC_TRANSACTIVE,TBM_GETPOS,0,0)/255);
		SetDlgItemTextA(hwndDlg,IDC_ACTIVEPERC,str);
		}
		if(wParam!=0x12345678) SendMessage(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case PSN_APPLY:
			//
			DBWriteContactSettingByte(NULL,"CLUI","LeftClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_LEFTMARGINSPIN,UDM_GETPOS,0,0));
			DBWriteContactSettingByte(NULL,"CLUI","RightClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_RIGHTMARGINSPIN,UDM_GETPOS,0,0));
			DBWriteContactSettingByte(NULL,"CLUI","TopClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_TOPMARGINSPIN,UDM_GETPOS,0,0));
			DBWriteContactSettingByte(NULL,"CLUI","BottomClientMargin",(BYTE)SendDlgItemMessage(hwndDlg,IDC_BOTTOMMARGINSPIN,UDM_GETPOS,0,0));
			DBWriteContactSettingByte(NULL,"ModernData","DisableEngine",IsDlgButtonChecked(hwndDlg,IDC_DISABLEENGINE));
			if (!IsDlgButtonChecked(hwndDlg,IDC_DISABLEENGINE))
			{
				if (g_proc_UpdateLayeredWindow!=NULL && IsDlgButtonChecked(hwndDlg,IDC_LAYERENGINE)) 
					DBWriteContactSettingByte(NULL,"ModernData","EnableLayering",0);
				else 
					DBDeleteContactSetting(NULL,"ModernData","EnableLayering");	
			}
			DBWriteContactSettingByte(NULL,"ModernSettings","UseKeyColor",IsDlgButtonChecked(hwndDlg,IDC_CHECKKEYCOLOR)?1:0);
			DBWriteContactSettingDword(NULL,"ModernSettings","KeyColor",SendDlgItemMessage(hwndDlg,IDC_COLOUR_KEY,CPM_GETCOLOUR,0,0));
			g_CluiData.fUseKeyColor=(BOOL)DBGetContactSettingByte(NULL,"ModernSettings","UseKeyColor",SETTING_USEKEYCOLOR_DEFAULT);
			g_CluiData.dwKeyColor=DBGetContactSettingDword(NULL,"ModernSettings","KeyColor",(DWORD)SETTING_KEYCOLOR_DEFAULT);
			DBWriteContactSettingByte(NULL,"CList","OnDesktop",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ONDESKTOP));
			DBWriteContactSettingByte(NULL,"CList","OnTop",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ONTOP));
			SetWindowPos(pcli->hwndContactList, IsDlgButtonChecked(hwndDlg,IDC_ONTOP)?HWND_TOPMOST:HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |SWP_NOACTIVATE);
			DBWriteContactSettingByte(NULL,"CLUI","DragToScroll",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_DRAGTOSCROLL));

			{ //====== Non-Layered Mode ======
				DBWriteContactSettingByte(NULL,"CList","ToolWindow",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_TOOLWND));
				DBWriteContactSettingByte(NULL,"CLUI","ShowCaption",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWCAPTION));
				DBWriteContactSettingByte(NULL,"CLUI","ShowMainMenu",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SHOWMAINMENU));
				DBWriteContactSettingByte(NULL,"CList","ThinBorder",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_BORDER));
				DBWriteContactSettingByte(NULL,"CList","NoBorder",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_NOBORDERWND));
				{	
					TCHAR title[256];
					GetDlgItemText(hwndDlg,IDC_TITLETEXT,title,SIZEOF(title));
					DBWriteContactSettingTString(NULL,"CList","TitleText",title);
					//			SetWindowText(pcli->hwndContactList,title);
				}
				DBWriteContactSettingByte(NULL,"CList","Min2Tray",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_MIN2TRAY));
				DBWriteContactSettingByte(NULL,"CList","WindowShadow",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_DROPSHADOW));
				DBWriteContactSettingByte(NULL,"CLC","RoundCorners",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ROUNDCORNERS));		
			} //====== End of Non-Layered Mode ======
			g_mutex_bChangingMode=TRUE;

			if (IsDlgButtonChecked(hwndDlg,IDC_ONDESKTOP)) 
			{
				HWND hProgMan=FindWindow(TEXT("Progman"),NULL);
				if (IsWindow(hProgMan)) 
				{
					SetParent(pcli->hwndContactList,hProgMan);
					callProxied_CLUIFrames_SetParentForContainers(hProgMan);
					g_CluiData.fOnDesktop=1;
				}
			} 
			else 
			{
				if (GetParent(pcli->hwndContactList))
				{
					SetParent(pcli->hwndContactList,NULL);
					callProxied_CLUIFrames_SetParentForContainers(NULL);
				}
				g_CluiData.fOnDesktop=0;
			}
			AniAva_UpdateParent();


			//DBWriteContactSettingByte(NULL,"CLUI","ClientAreaDrag",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_CLIENTDRAG));
			DBWriteContactSettingByte(NULL,"CLUI","FadeInOut",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_FADEINOUT));
			g_CluiData.fSmoothAnimation=IsWinVer2000Plus()&&(BYTE)IsDlgButtonChecked(hwndDlg,IDC_FADEINOUT);

			//DBWriteContactSettingByte(NULL,"CLUI","AutoSize",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZE));
			//DBWriteContactSettingByte(NULL,"CLUI","LockSize",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_LOCKSIZING));
			//DBWriteContactSettingByte(NULL,"CLUI","MaxSizeHeight",(BYTE)GetDlgItemInt(hwndDlg,IDC_MAXSIZEHEIGHT,NULL,FALSE));               
			{
				int i1;
				int i2;
				i1=SendDlgItemMessage(hwndDlg,IDC_FRAMESSPIN,UDM_GETPOS,0,0);
				i2=SendDlgItemMessage(hwndDlg,IDC_CAPTIONSSPIN,UDM_GETPOS,0,0);   

				DBWriteContactSettingDword(NULL,"CLUIFrames","GapBetweenFrames",(DWORD)i1);
				DBWriteContactSettingDword(NULL,"CLUIFrames","GapBetweenTitleBar",(DWORD)i2);
				callProxied_CLUIFramesOnClistResize((WPARAM)pcli->hwndContactList,(LPARAM)0);
			}
			//DBWriteContactSettingByte(NULL,"CLUI","AutoSizeUpward",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOSIZEUPWARD));
			//DBWriteContactSettingByte(NULL,"CLUI","SnapToEdges",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_SNAPTOEDGES));
			//DBWriteContactSettingByte(NULL,"CList","AutoHide",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_AUTOHIDE));
			//DBWriteContactSettingWord(NULL,"CList","HideTime",(WORD)SendDlgItemMessage(hwndDlg,IDC_HIDETIMESPIN,UDM_GETPOS,0,0));

			DBWriteContactSettingByte(NULL,"CList","Transparent",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT));
			DBWriteContactSettingByte(NULL,"CList","Alpha",(BYTE)SendDlgItemMessage(hwndDlg,IDC_TRANSACTIVE,TBM_GETPOS,0,0));
			DBWriteContactSettingByte(NULL,"CList","AutoAlpha",(BYTE)SendDlgItemMessage(hwndDlg,IDC_TRANSINACTIVE,TBM_GETPOS,0,0));
			DBWriteContactSettingByte(NULL,"CList","OnDesktop",(BYTE)IsDlgButtonChecked(hwndDlg,IDC_ONDESKTOP));

			/*
			if(IsDlgButtonChecked(hwndDlg,IDC_TRANSPARENT))	{
				SetWindowLong(pcli->hwndContactList, GWL_EXSTYLE, GetWindowLong(pcli->hwndContactList, GWL_EXSTYLE) | WS_EX_LAYERED);
				if(g_proc_SetLayeredWindowAttributesNew) g_proc_SetLayeredWindowAttributesNew(pcli->hwndContactList, RGB(0,0,0), (BYTE)DBGetContactSettingByte(NULL,"CList","AutoAlpha",SETTING_AUTOALPHA_DEFAULT), LWA_ALPHA);
			}
			else {
				SetWindowLong(pcli->hwndContactList, GWL_EXSTYLE, GetWindowLong(pcli->hwndContactList, GWL_EXSTYLE) & ~WS_EX_LAYERED);
			}
			*/
			ske_LoadSkinFromDB();
			CLUI_UpdateLayeredMode();	
			CLUI_ChangeWindowMode(); 
			SendMessage(pcli->hwndContactTree,WM_SIZE,0,0);	//forces it to send a cln_listsizechanged
			CLUI_ReloadCLUIOptions();
			cliShowHide(0,1);			
			g_mutex_bChangingMode=FALSE;
			return TRUE;
		}
		break;
	}
	return FALSE;
}

#include "commonheaders.h"

#define CLBF_TILEVTOROWHEIGHT        0x0100


#define DEFAULT_BKCOLOUR      GetSysColor(COLOR_3DFACE)
#define DEFAULT_USEBITMAP     0
#define DEFAULT_BKBMPUSE      CLB_STRETCH
#define DEFAULT_SELBKCOLOUR   GetSysColor(COLOR_HIGHLIGHT)



extern HINSTANCE g_hInst;
extern PLUGINLINK *pluginLink;
extern struct MM_INTERFACE mmi;

char **bkgrList = NULL;
int bkgrCount = 0;
HANDLE hEventBkgrChanged=NULL;
/*
#define mir_alloc(n) mmi.mmi_malloc(n)
#define mir_free(ptr) mmi.mmi_free(ptr)
#define mir_realloc(ptr,size) mmi.mmi_realloc(ptr,size)
*/

#define M_BKGR_UPDATE	(WM_USER+10)
#define M_BKGR_SETSTATE	(WM_USER+11)
#define M_BKGR_GETSTATE	(WM_USER+12)

#define M_BKGR_BACKCOLOR	0x01
#define M_BKGR_SELECTCOLOR	0x02
#define M_BKGR_ALLOWBITMAPS	0x04
#define M_BKGR_STRETCH		0x08
#define M_BKGR_TILE			0x10

#define ARRAY_SIZE(arr)	(sizeof(arr)/sizeof(arr[0]))
static int bitmapRelatedControls[] = {
	IDC_FILENAME,IDC_BROWSE,IDC_STRETCHH,IDC_STRETCHV,IDC_TILEH,IDC_TILEV,
	IDC_SCROLL,IDC_PROPORTIONAL,IDC_TILEVROWH
};
struct BkgrItem
{
	BYTE changed;
	BYTE useBitmap;
	COLORREF bkColor, selColor;
	char filename[MAX_PATH];
	WORD flags;
	BYTE useWinColours;
};
struct BkgrData
{
	struct BkgrItem *item;
	int indx;
	int count;
};
static BOOL CALLBACK DlgProcClcBkgOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct BkgrData *dat = (struct BkgrData *)GetWindowLong(hwndDlg, GWL_USERDATA);
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			int indx;
			HWND hList = GetDlgItem(hwndDlg, IDC_BKGRLIST);
			TranslateDialogDefault(hwndDlg);

			dat=(struct BkgrData*)mir_alloc(sizeof(struct BkgrData));
			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)dat);
			dat->count = bkgrCount;
			dat->item = (struct BkgrItem*)mir_alloc(sizeof(struct BkgrItem)*dat->count);
			dat->indx = CB_ERR;
			for(indx = 0; indx < dat->count; indx++)
			{
				char *module = bkgrList[indx] + strlen(bkgrList[indx]) + 1;
				int jndx;

				dat->item[indx].changed = FALSE;
				dat->item[indx].useBitmap = DBGetContactSettingByte(NULL,module, "UseBitmap", DEFAULT_USEBITMAP);
				dat->item[indx].bkColor = DBGetContactSettingDword(NULL,module, "BkColour", DEFAULT_BKCOLOUR);
				dat->item[indx].selColor = DBGetContactSettingDword(NULL,module, "SelBkColour", DEFAULT_SELBKCOLOUR);
				dat->item[indx].useWinColours = DBGetContactSettingByte(NULL,module, "UseWinColours", CLCDEFAULT_USEWINDOWSCOLOURS);	
				{	
					DBVARIANT dbv;
					if(!DBGetContactSetting(NULL,module,"BkBitmap",&dbv))
					{
						int retval = CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)dbv.pszVal, (LPARAM)dat->item[indx].filename);
						if(!retval || retval == CALLSERVICE_NOTFOUND)
							lstrcpynA(dat->item[indx].filename, dbv.pszVal, MAX_PATH);
						mir_free(dbv.pszVal);
					}
					else
						*dat->item[indx].filename = 0;
				}
				dat->item[indx].flags = DBGetContactSettingWord(NULL,module,"BkBmpUse", DEFAULT_BKBMPUSE);
				jndx = SendMessageA(hList, CB_ADDSTRING, 0, (LPARAM)Translate(bkgrList[indx]));
				SendMessage(hList, CB_SETITEMDATA, jndx, indx);
			}
			SendMessage(hList, CB_SETCURSEL, 0, 0);
			PostMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_BKGRLIST, CBN_SELCHANGE), 0);
			{
				HRESULT (STDAPICALLTYPE *MySHAutoComplete)(HWND,DWORD);
				MySHAutoComplete=(HRESULT (STDAPICALLTYPE*)(HWND,DWORD))GetProcAddress(GetModuleHandleA("shlwapi"),"SHAutoComplete");
				if(MySHAutoComplete) MySHAutoComplete(GetDlgItem(hwndDlg,IDC_FILENAME),1);
			}
			return TRUE;
		}
		case WM_DESTROY:
			if(dat)
			{
				if(dat->item) mir_free(dat->item);
				mir_free(dat);
			}
		
			return TRUE;

		case M_BKGR_GETSTATE:
		{
			int indx = wParam;
			if(indx == CB_ERR || indx >= dat->count) break;
			indx = SendDlgItemMessage(hwndDlg, IDC_BKGRLIST, CB_GETITEMDATA, indx, 0);

			dat->item[indx].useBitmap = IsDlgButtonChecked(hwndDlg,IDC_BITMAP);
			dat->item[indx].useWinColours = IsDlgButtonChecked(hwndDlg,IDC_USEWINCOL);
			dat->item[indx].bkColor = SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_GETCOLOUR,0,0);
			dat->item[indx].selColor = SendDlgItemMessage(hwndDlg, IDC_SELCOLOUR, CPM_GETCOLOUR,0,0);
			
			GetDlgItemTextA(hwndDlg, IDC_FILENAME, dat->item[indx].filename, sizeof(dat->item[indx].filename));
			{
				WORD flags = 0;
				if(IsDlgButtonChecked(hwndDlg,IDC_STRETCHH)) flags |= CLB_STRETCHH;
				if(IsDlgButtonChecked(hwndDlg,IDC_STRETCHV)) flags |= CLB_STRETCHV;
				if(IsDlgButtonChecked(hwndDlg,IDC_TILEH)) flags |= CLBF_TILEH;
				if(IsDlgButtonChecked(hwndDlg,IDC_TILEV)) flags |= CLBF_TILEV;
				if(IsDlgButtonChecked(hwndDlg,IDC_SCROLL)) flags |= CLBF_SCROLL;
				if(IsDlgButtonChecked(hwndDlg,IDC_PROPORTIONAL)) flags |= CLBF_PROPORTIONAL;
				if(IsDlgButtonChecked(hwndDlg,IDC_TILEVROWH)) flags |= CLBF_TILEVTOROWHEIGHT;
				dat->item[indx].flags = flags;
			}	
			break;
		}
		case M_BKGR_SETSTATE:
		{
			int flags;
			int indx = wParam;
			if (indx==-1) break;
			flags = dat->item[indx].flags;
			if(indx == CB_ERR || indx >= dat->count) break;
			indx = SendDlgItemMessage(hwndDlg, IDC_BKGRLIST, CB_GETITEMDATA, indx, 0);

			CheckDlgButton(hwndDlg, IDC_BITMAP, dat->item[indx].useBitmap?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_USEWINCOL, dat->item[indx].useWinColours?BST_CHECKED:BST_UNCHECKED);

			EnableWindow(GetDlgItem(hwndDlg,IDC_BKGCOLOUR), !dat->item[indx].useWinColours);
			EnableWindow(GetDlgItem(hwndDlg,IDC_SELCOLOUR), !dat->item[indx].useWinColours);

			SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETDEFAULTCOLOUR, 0, DEFAULT_BKCOLOUR);
			SendDlgItemMessage(hwndDlg, IDC_BKGCOLOUR, CPM_SETCOLOUR, 0, dat->item[indx].bkColor);
			SendDlgItemMessage(hwndDlg, IDC_SELCOLOUR, CPM_SETDEFAULTCOLOUR, 0, DEFAULT_SELBKCOLOUR);
			SendDlgItemMessage(hwndDlg, IDC_SELCOLOUR, CPM_SETCOLOUR, 0, dat->item[indx].selColor);
			SetDlgItemTextA(hwndDlg, IDC_FILENAME, dat->item[indx].filename);	

			CheckDlgButton(hwndDlg,IDC_STRETCHH, flags&CLB_STRETCHH?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg,IDC_STRETCHV,flags&CLB_STRETCHV?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg,IDC_TILEH,flags&CLBF_TILEH?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg,IDC_TILEV,flags&CLBF_TILEV?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg,IDC_SCROLL,flags&CLBF_SCROLL?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg,IDC_PROPORTIONAL,flags&CLBF_PROPORTIONAL?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg,IDC_TILEVROWH,flags&CLBF_TILEVTOROWHEIGHT?BST_CHECKED:BST_UNCHECKED);
/*
			{
				WORD visibility;
				int cy = 55;
				char *sz = bkgrList[indx] + strlen(bkgrList[indx]) + 1;
				sz += strlen(sz) + 1;
				visibility = (WORD)~(*(DWORD*)(sz));
//M_BKGR_BACKCOLOR,M_BKGR_SELECTCOLOR,M_BKGR_ALLOWBITMAPS,M_BKGR_STRETCH,M_BKGR_TILE}
				if(visibility & M_BKGR_BACKCOLOR)
				{
					SetWindowPos(GetDlgItem(hwndDlg, IDC_BC_STATIC), 0,
						20, cy,
						0, 0,
						SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
					SetWindowPos(GetDlgItem(hwndDlg, IDC_BKGCOLOUR), 0,
						130, cy,
						0, 0,
						SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
					cy += 25;					
				}
				if(visibility & M_BKGR_SELECTCOLOR)
				{
					SetWindowPos(GetDlgItem(hwndDlg, IDC_SC_STATIC), 0,
						20, cy,
						0, 0,
						SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
					SetWindowPos(GetDlgItem(hwndDlg, IDC_SELCOLOUR), 0,
						130, cy,
						0, 0,
						SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
					cy += 25;					
				}
				ShowWindow(GetDlgItem(hwndDlg,IDC_STRETCHH),     visibility&CLB_STRETCHH?SW_SHOW:SW_HIDE);
				ShowWindow(GetDlgItem(hwndDlg,IDC_STRETCHV),     visibility&CLB_STRETCHV?SW_SHOW:SW_HIDE);
				ShowWindow(GetDlgItem(hwndDlg,IDC_TILEH),        visibility&CLBF_TILEH?SW_SHOW:SW_HIDE);
				ShowWindow(GetDlgItem(hwndDlg,IDC_TILEV),        visibility&CLBF_TILEV?SW_SHOW:SW_HIDE);
				ShowWindow(GetDlgItem(hwndDlg,IDC_SCROLL),       visibility&CLBF_SCROLL?SW_SHOW:SW_HIDE);
				ShowWindow(GetDlgItem(hwndDlg,IDC_PROPORTIONAL), visibility&CLBF_PROPORTIONAL?SW_SHOW:SW_HIDE);
				ShowWindow(GetDlgItem(hwndDlg,IDC_TILEVROWH),    visibility&CLBF_TILEVTOROWHEIGHT?SW_SHOW:SW_HIDE);
			}
*/

			SendMessage(hwndDlg, M_BKGR_UPDATE, 0,0);
			break;
		}
		case M_BKGR_UPDATE:
		{
			int isChecked = IsDlgButtonChecked(hwndDlg,IDC_BITMAP);
			int indx;
			for(indx = 0; indx < ARRAY_SIZE(bitmapRelatedControls); indx++)
				EnableWindow(GetDlgItem(hwndDlg, bitmapRelatedControls[indx]),isChecked);
			break;
		}
		case WM_COMMAND:
			if(LOWORD(wParam) == IDC_BROWSE)
			{
				char str[MAX_PATH];
				OPENFILENAMEA ofn={0};
				char filter[512];

				GetDlgItemTextA(hwndDlg,IDC_FILENAME, str, sizeof(str));
				ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
				ofn.hwndOwner = hwndDlg;
				ofn.hInstance = NULL;
				CallService(MS_UTILS_GETBITMAPFILTERSTRINGS, sizeof(filter), (LPARAM)filter);
				ofn.lpstrFilter = filter;
				ofn.lpstrFile = str;
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
				ofn.nMaxFile = sizeof(str);
				ofn.nMaxFileTitle = MAX_PATH;
				ofn.lpstrDefExt = "bmp";
				if(!GetOpenFileNameA(&ofn)) break;
				SetDlgItemTextA(hwndDlg, IDC_FILENAME, str);
			}
			else
				if(LOWORD(wParam) == IDC_FILENAME && HIWORD(wParam) != EN_CHANGE) break;
			if(LOWORD(wParam) == IDC_BITMAP)
				SendMessage(hwndDlg, M_BKGR_UPDATE, 0,0);
			if(LOWORD(wParam) == IDC_FILENAME && (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != GetFocus()))
				return 0;
			if(LOWORD(wParam) == IDC_BKGRLIST)
			{
				if(HIWORD(wParam) == CBN_SELCHANGE)
				{
					SendMessage(hwndDlg, M_BKGR_GETSTATE, dat->indx, 0);
					SendMessage(hwndDlg, M_BKGR_SETSTATE, dat->indx = SendDlgItemMessage(hwndDlg, IDC_BKGRLIST, CB_GETCURSEL, 0,0), 0);
				}
				return 0;
			}
			{
				int indx = SendDlgItemMessage(hwndDlg, IDC_BKGRLIST, CB_GETCURSEL, 0,0);
				if(indx != CB_ERR && indx < dat->count)
				{
					indx = SendDlgItemMessage(hwndDlg, IDC_BKGRLIST, CB_GETITEMDATA, indx, 0);					
					dat->item[indx].changed = TRUE;
				
				}	
				{
					BOOL EnableColours=!IsDlgButtonChecked(hwndDlg,IDC_USEWINCOL);
					EnableWindow(GetDlgItem(hwndDlg,IDC_BKGCOLOUR), EnableColours);
					EnableWindow(GetDlgItem(hwndDlg,IDC_SELCOLOUR), EnableColours);
				}
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0,0);
			}
			break;
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->idFrom)
			{
				case 0:
					switch (((LPNMHDR)lParam)->code)
					{
						case PSN_APPLY:
						{
							int indx;
							SendMessage(hwndDlg, M_BKGR_GETSTATE, SendDlgItemMessage(hwndDlg, IDC_BKGRLIST, CB_GETCURSEL, 0,0), 0);
							for(indx = 0; indx < dat->count; indx++)
							if(dat->item[indx].changed)
							{
								char *module = bkgrList[indx] + strlen(bkgrList[indx]) + 1;
								DBWriteContactSettingByte(NULL, module, "UseBitmap", (BYTE)dat->item[indx].useBitmap);
								{	
									COLORREF col;

									if((col = dat->item[indx].bkColor) == DEFAULT_BKCOLOUR)																		
										DBDeleteContactSetting(NULL, module, "BkColour");
									else
										DBWriteContactSettingDword(NULL, module, "BkColour", col);

									if((col = dat->item[indx].selColor) == DEFAULT_SELBKCOLOUR)
										DBDeleteContactSetting(NULL, module, "SelBkColour");
									else
										DBWriteContactSettingDword(NULL, module, "SelBkColour", col);
								}
								DBWriteContactSettingByte(NULL, module, "UseWinColours", (BYTE)dat->item[indx].useWinColours);
								
								{
									char str[MAX_PATH];
									int retval = CallService(MS_UTILS_PATHTOABSOLUTE,
										(WPARAM)dat->item[indx].filename,
										(LPARAM)str);
									if(!retval || retval == CALLSERVICE_NOTFOUND)
										DBWriteContactSettingString(NULL, module, "BkBitmap", dat->item[indx].filename);
									else
										DBWriteContactSettingString(NULL, module, "BkBitmap", str);
								}
								DBWriteContactSettingWord(NULL, module, "BkBmpUse", dat->item[indx].flags);
								dat->item[indx].changed = FALSE;
								NotifyEventHooks(hEventBkgrChanged, (WPARAM)module, 0);
							}
							return TRUE;
						}
					}
					break;
			}
			break;
	}
	return FALSE;
}

static int BkgrCfg_Register(WPARAM wParam,LPARAM lParam)
{
	char *szSetting = (char *)wParam;
	char *value, *tok;
	int len = strlen(szSetting) + 1;

	value = (char *)mir_alloc(len + 4); // add room for flags (DWORD)
	memcpy(value, szSetting, len);
	tok = strchr(value, '/');
	if(tok == NULL)
	{
		mir_free(value);
		return 1;
	}
	*tok = 0;
	*(DWORD*)(value + len) = lParam;

	bkgrList = (char **)mir_realloc(bkgrList, sizeof(char*)*(bkgrCount+1));
	bkgrList[bkgrCount] = value;
	bkgrCount++;
	
	return 0;
}


int BGModuleLoad()
{	
	CreateServiceFunction(MS_BACKGROUNDCONFIG_REGISTER, BkgrCfg_Register);

	hEventBkgrChanged = CreateHookableEvent(ME_BACKGROUNDCONFIG_CHANGED);
	return 0;
}

int BGModuleUnload(void)
{
	if(bkgrList != NULL)
	{
		int indx;
		for(indx = 0; indx < bkgrCount; indx++)
			if(bkgrList[indx] != NULL)
				mir_free(bkgrList[indx]);
		mir_free(bkgrList);
	}
	DestroyHookableEvent(hEventBkgrChanged);

	return 0;
}
