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

#include "commonheaders.h"

static int LastModifiedItem = -1;
static int last_selcount=0;
static int last_indizes[64];

static StatusItems_t StatusItems[] = {
	{"Offline", "EXBK_Offline", ID_STATUS_OFFLINE, 
	CLCDEFAULT_GRADIENT, CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	},
	{"Online", "EXBK_Online", ID_STATUS_ONLINE, 
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	},
	{"Away", "EXBK_Away", ID_STATUS_AWAY, 
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	},
	{"DND", "EXBK_Dnd", ID_STATUS_DND, 
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	},
	{"NA", "EXBK_NA", ID_STATUS_NA, 
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	},
	{"Occupied", "EXBK_Occupied", ID_STATUS_OCCUPIED, 
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	},
	{"Free for chat", "EXBK_FFC", ID_STATUS_FREECHAT, 
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	},
	{"Invisible", "EXBK_Invisible", ID_STATUS_INVISIBLE, 
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	},
	{"On the phone", "EXBK_OTP", ID_STATUS_ONTHEPHONE, 
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	},
	{"Out to lunch", "EXBK_OTL", ID_STATUS_OUTTOLUNCH, 
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	},	
	// following are special NON-Status cases	
	{"-------------------------------------","",ID_EXTBKSEPARATOR},
	{"Expanded Group", "EXBK_EXPANDEDGROUPS", ID_EXTBKEXPANDEDGROUP,
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	},
	{"Collapsed Group", "EXBK_COLLAPSEDGROUP", ID_EXTBKCOLLAPSEDDGROUP,
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
	},
	{"Empty Group", "EXBK_EMPTYGROUPS", ID_EXTBKEMPTYGROUPS,
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
	},	
	{"-------------------------------------","",ID_EXTBKSEPARATOR},
	{"First contact of a group", "EXBK_FIRSTITEM", ID_EXTBKFIRSTITEM,
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, 0, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
	},
	{"Single item in group", "EXBK_SINGLEITEM", ID_EXTBKSINGLEITEM,
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, 0, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
	},
	{"Last contact of a group", "EXBK_LASTITEM", ID_EXTBKLASTITEM,
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, 0, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
	},
	{"-------------------------------------","",ID_EXTBKSEPARATOR},
	{"First contact of NON-group", "EXBK_FIRSTITEM_NG", ID_EXTBKFIRSTITEM_NG,
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, 0, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
	},
	{"Single item in NON-group", "EXBK_SINGLEITEM_NG", ID_EXTBKSINGLEITEM_NG,
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, 0, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
	},
	{"Last contact of NON-group", "EXBK_LASTITEM_NG", ID_EXTBKLASTITEM_NG,
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, 0, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
	},
	{"-------------------------------------","",ID_EXTBKSEPARATOR},
	{"Even Contact Positions", "EXBK_EVEN_CNTC_POS", ID_EXTBKEVEN_CNTCTPOS,
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
	},
	{"Odd Contact Positions", "EXBK_ODD_CNTC_POS", ID_EXTBKODD_CNTCTPOS,
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, 1
	},
	{"-------------------------------------","",ID_EXTBKSEPARATOR},
	{"Selection", "EXBK_SELECTION", ID_EXTBKSELECTION,
	CLCDEFAULT_GRADIENT,CLCDEFAULT_CORNER,
	CLCDEFAULT_COLOR, CLCDEFAULT_COLOR2, CLCDEFAULT_COLOR2_TRANSPARENT, CLCDEFAULT_TEXTCOLOR, CLCDEFAULT_ALPHA, CLCDEFAULT_MRGN_LEFT, 
	CLCDEFAULT_MRGN_TOP, CLCDEFAULT_MRGN_RIGHT, CLCDEFAULT_MRGN_BOTTOM, CLCDEFAULT_IGNORE
	}
};

ChangedSItems_t ChangedSItems = {0};

BOOL GetItemByStatus(int status, StatusItems_t* retitem)
{
	int n;
	for (n=0;n<sizeof(StatusItems)/sizeof(StatusItems[0]);n++)
	{
		if (status==StatusItems[n].statusID)
		{
			*retitem = StatusItems[n];
			return TRUE;
		}			
	}
	return FALSE;
}

// fills the struct with the settings in the database
void LoadExtBkSettingsFromDB()
{
	DWORD ret;
	int n;
	char buffer[255];
	
	for (n=0;n<sizeof(StatusItems)/sizeof(StatusItems[0]);n++)
	{
		if (StatusItems[n].statusID!=ID_EXTBKSEPARATOR)
		{		
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_IGNORE");
			ret = DBGetContactSettingByte(NULL,"CLCExt",buffer,StatusItems[n].IGNORED);
			StatusItems[n].IGNORED= ret;

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_GRADIENT");
			ret = DBGetContactSettingDword(NULL,"CLCExt",buffer,StatusItems[n].GRADIENT);			
			StatusItems[n].GRADIENT = ret;

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_CORNER");
			ret = DBGetContactSettingDword(NULL,"CLCExt",buffer,StatusItems[n].CORNER);
			StatusItems[n].CORNER = ret;

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_COLOR");
			ret = DBGetContactSettingDword(NULL,"CLCExt",buffer,StatusItems[n].COLOR);
			StatusItems[n].COLOR = ret;

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_COLOR2");
			ret = DBGetContactSettingDword(NULL,"CLCExt",buffer,StatusItems[n].COLOR2);
			StatusItems[n].COLOR2 = ret;			

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_COLOR2_TRANSPARENT");
			ret = DBGetContactSettingByte(NULL,"CLCExt",buffer,StatusItems[n].COLOR2_TRANSPARENT);
			StatusItems[n].COLOR2_TRANSPARENT = ret;

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_TEXTCOLOR");
			ret = DBGetContactSettingDword(NULL,"CLCExt",buffer,StatusItems[n].TEXTCOLOR);
			StatusItems[n].TEXTCOLOR = ret;

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_ALPHA");
			ret = DBGetContactSettingByte(NULL,"CLCExt",buffer,StatusItems[n].ALPHA);
			StatusItems[n].ALPHA = ret;

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_MRGN_LEFT");
			ret = DBGetContactSettingByte(NULL,"CLCExt",buffer,StatusItems[n].MARGIN_LEFT);
			StatusItems[n].MARGIN_LEFT = ret;

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_MRGN_TOP");
			ret = DBGetContactSettingByte(NULL,"CLCExt",buffer,StatusItems[n].MARGIN_TOP);
			StatusItems[n].MARGIN_TOP = ret;

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_MRGN_RIGHT");
			ret = DBGetContactSettingByte(NULL,"CLCExt",buffer,StatusItems[n].MARGIN_RIGHT);
			StatusItems[n].MARGIN_RIGHT = ret;

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_MRGN_BOTTOM");
			ret = DBGetContactSettingByte(NULL,"CLCExt",buffer,StatusItems[n].MARGIN_BOTTOM);
			StatusItems[n].MARGIN_BOTTOM = ret;
		}
	}
}

// writes whole struct to the database
void SaveCompleteStructToDB()
{
	int n;
	char buffer[255];

	for (n=0;n<sizeof(StatusItems)/sizeof(StatusItems[0]);n++)
	{
		if (StatusItems[n].statusID!=ID_EXTBKSEPARATOR)
		{		
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_IGNORE");
			DBWriteContactSettingByte(NULL,"CLCExt",buffer,StatusItems[n].IGNORED);

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_GRADIENT");
			DBWriteContactSettingDword(NULL,"CLCExt",buffer,StatusItems[n].GRADIENT);			

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_CORNER");
			DBWriteContactSettingDword(NULL,"CLCExt",buffer,StatusItems[n].CORNER);

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_COLOR");
			DBWriteContactSettingDword(NULL,"CLCExt",buffer,StatusItems[n].COLOR);

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_COLOR2");
			DBWriteContactSettingDword(NULL,"CLCExt",buffer,StatusItems[n].COLOR2);

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_COLOR2_TRANSPARENT");
			DBWriteContactSettingByte(NULL,"CLCExt",buffer,StatusItems[n].COLOR2_TRANSPARENT);
			
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_TEXTCOLOR");
			DBWriteContactSettingDword(NULL,"CLCExt",buffer,StatusItems[n].TEXTCOLOR);


			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_ALPHA");
			DBWriteContactSettingByte(NULL,"CLCExt",buffer,StatusItems[n].ALPHA);

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_MRGN_LEFT");
			DBWriteContactSettingByte(NULL,"CLCExt",buffer,StatusItems[n].MARGIN_LEFT);

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_MRGN_TOP");
			DBWriteContactSettingByte(NULL,"CLCExt",buffer,StatusItems[n].MARGIN_TOP);

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_MRGN_RIGHT");
			DBWriteContactSettingByte(NULL,"CLCExt",buffer,StatusItems[n].MARGIN_RIGHT);

			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_MRGN_BOTTOM");
			DBWriteContactSettingByte(NULL,"CLCExt",buffer,StatusItems[n].MARGIN_BOTTOM);
		}
	}
}

// updates the struct with the changed dlg item
void UpdateStatusStructSettingsFromOptDlg(HWND hwndDlg, int index)
{
	char buf[15];

	if (ChangedSItems.bIGNORED)
		StatusItems[index].IGNORED = IsDlgButtonChecked(hwndDlg,IDC_IGNORE);

	if (ChangedSItems.bGRADIENT) {
		StatusItems[index].GRADIENT = GRADIENT_NONE;
		if (IsDlgButtonChecked(hwndDlg,IDC_GRADIENT))
			StatusItems[index].GRADIENT |= GRADIENT_ACTIVE;
		if (IsDlgButtonChecked(hwndDlg,IDC_GRADIENT_LR))
			StatusItems[index].GRADIENT |= GRADIENT_LR;
		if (IsDlgButtonChecked(hwndDlg,IDC_GRADIENT_RL))
			StatusItems[index].GRADIENT |= GRADIENT_RL;
		if (IsDlgButtonChecked(hwndDlg,IDC_GRADIENT_TB))
			StatusItems[index].GRADIENT |= GRADIENT_TB;
		if (IsDlgButtonChecked(hwndDlg,IDC_GRADIENT_BT))
			StatusItems[index].GRADIENT |= GRADIENT_BT;
	}
	if (ChangedSItems.bCORNER) {

		StatusItems[index].CORNER = CORNER_NONE;
		if (IsDlgButtonChecked(hwndDlg,IDC_CORNER))
			StatusItems[index].CORNER |= CORNER_ACTIVE ;
		if (IsDlgButtonChecked(hwndDlg,IDC_CORNER_TL))
			StatusItems[index].CORNER |= CORNER_TL ;
		if (IsDlgButtonChecked(hwndDlg,IDC_CORNER_TR))
			StatusItems[index].CORNER |= CORNER_TR;
		if (IsDlgButtonChecked(hwndDlg,IDC_CORNER_BR))
			StatusItems[index].CORNER |= CORNER_BR;
		if (IsDlgButtonChecked(hwndDlg,IDC_CORNER_BL))
			StatusItems[index].CORNER |= CORNER_BL;
	}

	if (ChangedSItems.bCOLOR)
		StatusItems[index].COLOR = SendDlgItemMessage(hwndDlg,IDC_BASECOLOUR,CPM_GETCOLOUR,0,0);	

	if (ChangedSItems.bCOLOR2)
		StatusItems[index].COLOR2 = SendDlgItemMessage(hwndDlg,IDC_BASECOLOUR2,CPM_GETCOLOUR,0,0);

	if (ChangedSItems.bCOLOR2_TRANSPARENT)
		StatusItems[index].COLOR2_TRANSPARENT = IsDlgButtonChecked(hwndDlg,IDC_COLOR2_TRANSPARENT);	

	if (ChangedSItems.bTEXTCOLOR)
		StatusItems[index].TEXTCOLOR = SendDlgItemMessage(hwndDlg,IDC_TEXTCOLOUR,CPM_GETCOLOUR,0,0);

	if (ChangedSItems.bALPHA)
	{
		GetWindowText(GetDlgItem(hwndDlg,IDC_ALPHA), buf ,10);		// can be removed now
		if (lstrlen(buf)>0) 
			StatusItems[index].ALPHA = (BYTE)SendDlgItemMessage(hwndDlg,IDC_ALPHASPIN,UDM_GETPOS,0,0);
	}

	if (ChangedSItems.bMARGIN_LEFT)
	{
		GetWindowText(GetDlgItem(hwndDlg,IDC_MRGN_LEFT), buf ,10);		
		if (lstrlen(buf)>0) 
			StatusItems[index].MARGIN_LEFT = (BYTE)SendDlgItemMessage(hwndDlg,IDC_MRGN_LEFT_SPIN,UDM_GETPOS,0,0);
	}

	if (ChangedSItems.bMARGIN_TOP)
	{
		GetWindowText(GetDlgItem(hwndDlg,IDC_MRGN_TOP), buf ,10);		
		if (lstrlen(buf)>0) 
			StatusItems[index].MARGIN_TOP = (BYTE)SendDlgItemMessage(hwndDlg,IDC_MRGN_TOP_SPIN,UDM_GETPOS,0,0); 
	}

	if (ChangedSItems.bMARGIN_RIGHT)
	{
		GetWindowText(GetDlgItem(hwndDlg,IDC_MRGN_RIGHT), buf ,10);		
		if (lstrlen(buf)>0) 
			StatusItems[index].MARGIN_RIGHT = (BYTE)SendDlgItemMessage(hwndDlg,IDC_MRGN_RIGHT_SPIN,UDM_GETPOS,0,0);
	}

	if (ChangedSItems.bMARGIN_BOTTOM)
	{
		GetWindowText(GetDlgItem(hwndDlg,IDC_MRGN_BOTTOM), buf ,10);		
		if (lstrlen(buf)>0) 
			StatusItems[index].MARGIN_BOTTOM = (BYTE)SendDlgItemMessage(hwndDlg,IDC_MRGN_BOTTOM_SPIN,UDM_GETPOS,0,0);
	}
}

void SaveLatestChanges(HWND hwndDlg)
{
	int n;
	// process old selection
	if (last_selcount>0)
	{
		for (n=0;n<last_selcount;n++)
		{
			if (StatusItems[last_indizes[n]].statusID != ID_EXTBKSEPARATOR)	
			{	
				UpdateStatusStructSettingsFromOptDlg(hwndDlg,last_indizes[n]);
			} 
		}
	}

	// reset bChange
	ChangedSItems.bALPHA = FALSE;
	ChangedSItems.bGRADIENT = FALSE;
	ChangedSItems.bCORNER = FALSE;
	ChangedSItems.bCOLOR = FALSE;
	ChangedSItems.bCOLOR2 = FALSE;
	ChangedSItems.bCOLOR2_TRANSPARENT = FALSE;
	ChangedSItems.bTEXTCOLOR = FALSE;
	ChangedSItems.bALPHA = FALSE;
	ChangedSItems.bMARGIN_LEFT = FALSE;
	ChangedSItems.bMARGIN_TOP = FALSE;
	ChangedSItems.bMARGIN_RIGHT = FALSE;
	ChangedSItems.bMARGIN_BOTTOM = FALSE;
	ChangedSItems.bIGNORED = FALSE;

}

// wenn die listbox geändert wurde
void OnListItemsChange(HWND hwndDlg)
{	
	SaveLatestChanges(hwndDlg);

	// set new selection
	last_selcount = SendMessage(GetDlgItem(hwndDlg,IDC_ITEMS),LB_GETSELCOUNT,0,0);	
	if (last_selcount>0) 
	{
		int n;
		StatusItems_t DialogSettingForMultiSel;

		// get selected indizes
		SendMessage(GetDlgItem(hwndDlg,IDC_ITEMS),LB_GETSELITEMS,64,(LPARAM)last_indizes);
		
		// initialize with first items value
		DialogSettingForMultiSel = StatusItems[last_indizes[0]];

		for (n=0;n<last_selcount;n++)
		{
			if (StatusItems[last_indizes[n]].statusID != ID_EXTBKSEPARATOR)	
			{
				if (StatusItems[last_indizes[n]].ALPHA != StatusItems[last_indizes[0]].ALPHA)
					DialogSettingForMultiSel.ALPHA = -1;
				if (StatusItems[last_indizes[n]].COLOR != StatusItems[last_indizes[0]].COLOR)
					DialogSettingForMultiSel.COLOR = CLCDEFAULT_COLOR;
				if (StatusItems[last_indizes[n]].COLOR2 != StatusItems[last_indizes[0]].COLOR2)
					DialogSettingForMultiSel.COLOR2 = CLCDEFAULT_COLOR2;
				if (StatusItems[last_indizes[n]].COLOR2_TRANSPARENT != StatusItems[last_indizes[0]].COLOR2_TRANSPARENT)
					DialogSettingForMultiSel.COLOR2_TRANSPARENT = CLCDEFAULT_COLOR2_TRANSPARENT;
				if (StatusItems[last_indizes[n]].TEXTCOLOR != StatusItems[last_indizes[0]].TEXTCOLOR)
					DialogSettingForMultiSel.TEXTCOLOR = CLCDEFAULT_TEXTCOLOR;
				if (StatusItems[last_indizes[n]].CORNER != StatusItems[last_indizes[0]].CORNER)
					DialogSettingForMultiSel.CORNER = CLCDEFAULT_CORNER;
				if (StatusItems[last_indizes[n]].GRADIENT != StatusItems[last_indizes[0]].GRADIENT)
					DialogSettingForMultiSel.GRADIENT = CLCDEFAULT_GRADIENT;
				if (StatusItems[last_indizes[n]].IGNORED != StatusItems[last_indizes[0]].IGNORED)
					DialogSettingForMultiSel.IGNORED = CLCDEFAULT_IGNORE;
				if (StatusItems[last_indizes[n]].MARGIN_BOTTOM != StatusItems[last_indizes[0]].MARGIN_BOTTOM)
					DialogSettingForMultiSel.MARGIN_BOTTOM = -1;
				if (StatusItems[last_indizes[n]].MARGIN_LEFT != StatusItems[last_indizes[0]].MARGIN_LEFT)
					DialogSettingForMultiSel.MARGIN_LEFT = -1;
				if (StatusItems[last_indizes[n]].MARGIN_RIGHT != StatusItems[last_indizes[0]].MARGIN_RIGHT)
					DialogSettingForMultiSel.MARGIN_RIGHT = -1;
				if (StatusItems[last_indizes[n]].MARGIN_TOP != StatusItems[last_indizes[0]].MARGIN_TOP)
					DialogSettingForMultiSel.MARGIN_TOP = -1;
			} 
		}

		if (last_selcount==1 && StatusItems[last_indizes[0]].statusID == ID_EXTBKSEPARATOR)
		{
			ChangeControlItems(hwndDlg,0,0);
			last_selcount = 0;
		} else
			ChangeControlItems(hwndDlg,1,0);
			FillOptionDialogByStatusItem(hwndDlg, &DialogSettingForMultiSel);
	}
}

// Save Non-StatusItems Settings
void SaveNonStatusItemsSettings(HWND hwndDlg)
{	
	DBWriteContactSettingByte(NULL,"CLCExt","EXBK_EqualSelection", IsDlgButtonChecked(hwndDlg,IDC_EQUALSELECTION));
	DBWriteContactSettingByte(NULL,"CLCExt","EXBK_SelBlend", IsDlgButtonChecked(hwndDlg,IDC_SELBLEND));	
	DBWriteContactSettingByte(NULL,"CLCExt","EXBK_FillWallpaper", IsDlgButtonChecked(hwndDlg,IDC_FILLWALLPAPER));
	DBWriteContactSettingByte(NULL,"CLCExt","EXBK_CenterGroupnames", IsDlgButtonChecked(hwndDlg,IDC_CENTERGROUPNAMES));
}

// fills the combobox of the options dlg for the first time
void FillItemList(HWND hwndDlg)
{
	int n;
	for (n=0;n<sizeof(StatusItems)/sizeof(StatusItems[0]);n++)
		SendDlgItemMessage(hwndDlg,IDC_ITEMS,LB_ADDSTRING,0,(LPARAM)StatusItems[n].szName);
}

void FillOptionDialogByStatusItem(HWND hwndDlg, StatusItems_t *item)
{
	char itoabuf[15];
	DWORD ret;

	CheckDlgButton(hwndDlg,IDC_IGNORE,(item->IGNORED)?BST_CHECKED:BST_UNCHECKED);

	CheckDlgButton(hwndDlg,IDC_GRADIENT,(item->GRADIENT&GRADIENT_ACTIVE)?BST_CHECKED:BST_UNCHECKED);
	EnableWindow(GetDlgItem(hwndDlg,IDC_GRADIENT_LR),item->GRADIENT&GRADIENT_ACTIVE);
	EnableWindow(GetDlgItem(hwndDlg,IDC_GRADIENT_RL),item->GRADIENT&GRADIENT_ACTIVE);
	EnableWindow(GetDlgItem(hwndDlg,IDC_GRADIENT_TB),item->GRADIENT&GRADIENT_ACTIVE);
	EnableWindow(GetDlgItem(hwndDlg,IDC_GRADIENT_BT),item->GRADIENT&GRADIENT_ACTIVE);
	CheckDlgButton(hwndDlg,IDC_GRADIENT_LR,(item->GRADIENT&GRADIENT_LR)?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(hwndDlg,IDC_GRADIENT_RL,(item->GRADIENT&GRADIENT_RL)?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(hwndDlg,IDC_GRADIENT_TB,(item->GRADIENT&GRADIENT_TB)?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(hwndDlg,IDC_GRADIENT_BT,(item->GRADIENT&GRADIENT_BT)?BST_CHECKED:BST_UNCHECKED);

	CheckDlgButton(hwndDlg,IDC_CORNER,(item->CORNER&CORNER_ACTIVE)?BST_CHECKED:BST_UNCHECKED);
	EnableWindow(GetDlgItem(hwndDlg,IDC_CORNER_TL),item->CORNER&CORNER_ACTIVE);
	EnableWindow(GetDlgItem(hwndDlg,IDC_CORNER_TR),item->CORNER&CORNER_ACTIVE);
	EnableWindow(GetDlgItem(hwndDlg,IDC_CORNER_BR),item->CORNER&CORNER_ACTIVE);
	EnableWindow(GetDlgItem(hwndDlg,IDC_CORNER_BL),item->CORNER&CORNER_ACTIVE);

	CheckDlgButton(hwndDlg,IDC_CORNER_TL,(item->CORNER&CORNER_TL)?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(hwndDlg,IDC_CORNER_TR,(item->CORNER&CORNER_TR)?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(hwndDlg,IDC_CORNER_BR,(item->CORNER&CORNER_BR)?BST_CHECKED:BST_UNCHECKED);
	CheckDlgButton(hwndDlg,IDC_CORNER_BL,(item->CORNER&CORNER_BL)?BST_CHECKED:BST_UNCHECKED);

	ret = item->COLOR;
	SendDlgItemMessage(hwndDlg,IDC_BASECOLOUR,CPM_SETDEFAULTCOLOUR,0,CLCDEFAULT_COLOR);
	SendDlgItemMessage(hwndDlg,IDC_BASECOLOUR,CPM_SETCOLOUR,0,ret);

	ret = item->COLOR2;
	SendDlgItemMessage(hwndDlg,IDC_BASECOLOUR2,CPM_SETDEFAULTCOLOUR,0,CLCDEFAULT_COLOR2);
	SendDlgItemMessage(hwndDlg,IDC_BASECOLOUR2,CPM_SETCOLOUR,0,ret);

	CheckDlgButton(hwndDlg,IDC_COLOR2_TRANSPARENT,(item->COLOR2_TRANSPARENT)?BST_CHECKED:BST_UNCHECKED);

	ret = item->TEXTCOLOR;
	SendDlgItemMessage(hwndDlg,IDC_TEXTCOLOUR,CPM_SETDEFAULTCOLOUR,0,CLCDEFAULT_TEXTCOLOR);
	SendDlgItemMessage(hwndDlg,IDC_TEXTCOLOUR,CPM_SETCOLOUR,0,ret);


	//	TODO: I suppose we don't need to use _itoa here. 
	//	we could probably just set the integer value of the buddy spinner control:

	if (item->ALPHA==-1)
	{
		SetDlgItemText(hwndDlg,IDC_ALPHA,"");
	}
	else {	
		ret = item->ALPHA;
		_itoa(ret, itoabuf,10);	
		SetDlgItemText(hwndDlg,IDC_ALPHA,itoabuf);
	}

	if (item->MARGIN_LEFT==-1)
		SetDlgItemText(hwndDlg,IDC_MRGN_LEFT,"");
	else {
		ret = item->MARGIN_LEFT;
		_itoa(ret,itoabuf,10);
		SetDlgItemText(hwndDlg,IDC_MRGN_LEFT,itoabuf);
	}

	if (item->MARGIN_TOP==-1)
		SetDlgItemText(hwndDlg,IDC_MRGN_TOP,"");
	else {
		ret = item->MARGIN_TOP;
		_itoa(ret,itoabuf,10);
		SetDlgItemText(hwndDlg,IDC_MRGN_TOP,itoabuf);
	}

	if (item->MARGIN_RIGHT==-1)
		SetDlgItemText(hwndDlg,IDC_MRGN_RIGHT,"");
	else {
		ret = item->MARGIN_RIGHT;
		_itoa(ret,itoabuf,10);
		SetDlgItemText(hwndDlg,IDC_MRGN_RIGHT,itoabuf);
	}

	if (item->MARGIN_BOTTOM==-1)
		SetDlgItemText(hwndDlg,IDC_MRGN_BOTTOM,"");
	else {
		ret = item->MARGIN_BOTTOM;
		_itoa(ret,itoabuf,10);
		SetDlgItemText(hwndDlg,IDC_MRGN_BOTTOM,itoabuf);
	}

	ReActiveCombo(hwndDlg);
}
// update dlg with selected item
void FillOptionDialogByCurrentSel(HWND hwndDlg)
{	
	int index = SendDlgItemMessage(hwndDlg,IDC_ITEMS,LB_GETCURSEL,0,0);

	LastModifiedItem = index;

	if (CheckItem(index,hwndDlg))
	{
		FillOptionDialogByStatusItem(hwndDlg, &StatusItems[index]);
	}
}

void ReActiveCombo(HWND hwndDlg)
{		
	if (IsDlgButtonChecked(hwndDlg,IDC_IGNORE))
	{
		EnableWindow(GetDlgItem(hwndDlg,IDC_GRADIENT_LR),IsDlgButtonChecked(hwndDlg,IDC_GRADIENT));
		EnableWindow(GetDlgItem(hwndDlg,IDC_GRADIENT_RL),IsDlgButtonChecked(hwndDlg,IDC_GRADIENT));
		EnableWindow(GetDlgItem(hwndDlg,IDC_GRADIENT_TB),IsDlgButtonChecked(hwndDlg,IDC_GRADIENT));
		EnableWindow(GetDlgItem(hwndDlg,IDC_GRADIENT_BT),IsDlgButtonChecked(hwndDlg,IDC_GRADIENT));
		
		EnableWindow(GetDlgItem(hwndDlg,IDC_BASECOLOUR2),!IsDlgButtonChecked(hwndDlg,IDC_COLOR2_TRANSPARENT));
		EnableWindow(GetDlgItem(hwndDlg,IDC_COLOR2LABLE),!IsDlgButtonChecked(hwndDlg,IDC_COLOR2_TRANSPARENT));

		EnableWindow(GetDlgItem(hwndDlg,IDC_CORNER_TL),IsDlgButtonChecked(hwndDlg,IDC_CORNER));
		EnableWindow(GetDlgItem(hwndDlg,IDC_CORNER_TR),IsDlgButtonChecked(hwndDlg,IDC_CORNER));
		EnableWindow(GetDlgItem(hwndDlg,IDC_CORNER_BR),IsDlgButtonChecked(hwndDlg,IDC_CORNER));
		EnableWindow(GetDlgItem(hwndDlg,IDC_CORNER_BL),IsDlgButtonChecked(hwndDlg,IDC_CORNER));		
		ChangeControlItems(hwndDlg, !IsDlgButtonChecked(hwndDlg,IDC_IGNORE), IDC_IGNORE);		
	}else
	{		
		ChangeControlItems(hwndDlg, !IsDlgButtonChecked(hwndDlg,IDC_IGNORE), IDC_IGNORE);
		EnableWindow(GetDlgItem(hwndDlg,IDC_GRADIENT_LR),IsDlgButtonChecked(hwndDlg,IDC_GRADIENT));
		EnableWindow(GetDlgItem(hwndDlg,IDC_GRADIENT_RL),IsDlgButtonChecked(hwndDlg,IDC_GRADIENT));
		EnableWindow(GetDlgItem(hwndDlg,IDC_GRADIENT_TB),IsDlgButtonChecked(hwndDlg,IDC_GRADIENT));
		EnableWindow(GetDlgItem(hwndDlg,IDC_GRADIENT_BT),IsDlgButtonChecked(hwndDlg,IDC_GRADIENT));

		EnableWindow(GetDlgItem(hwndDlg,IDC_BASECOLOUR2),!IsDlgButtonChecked(hwndDlg,IDC_COLOR2_TRANSPARENT));
		EnableWindow(GetDlgItem(hwndDlg,IDC_COLOR2LABLE),!IsDlgButtonChecked(hwndDlg,IDC_COLOR2_TRANSPARENT));

		EnableWindow(GetDlgItem(hwndDlg,IDC_CORNER_TL),IsDlgButtonChecked(hwndDlg,IDC_CORNER));
		EnableWindow(GetDlgItem(hwndDlg,IDC_CORNER_TR),IsDlgButtonChecked(hwndDlg,IDC_CORNER));
		EnableWindow(GetDlgItem(hwndDlg,IDC_CORNER_BR),IsDlgButtonChecked(hwndDlg,IDC_CORNER));
		EnableWindow(GetDlgItem(hwndDlg,IDC_CORNER_BL),IsDlgButtonChecked(hwndDlg,IDC_CORNER));				
	}
}

// enabled or disabled the whole status controlitems group (with exceptional control)
void ChangeControlItems(HWND hwndDlg, int status, int except)
{
	if (except!=IDC_GRADIENT)	EnableWindow(GetDlgItem(hwndDlg,IDC_GRADIENT),status);
	if (except!=IDC_GRADIENT_LR)	EnableWindow(GetDlgItem(hwndDlg,IDC_GRADIENT_LR),status);
	if (except!=IDC_GRADIENT_RL)	EnableWindow(GetDlgItem(hwndDlg,IDC_GRADIENT_RL),status);
	if (except!=IDC_GRADIENT_TB)	EnableWindow(GetDlgItem(hwndDlg,IDC_GRADIENT_TB),status);
	if (except!=IDC_GRADIENT_BT)	EnableWindow(GetDlgItem(hwndDlg,IDC_GRADIENT_BT),status);
	if (except!=IDC_CORNER)	EnableWindow(GetDlgItem(hwndDlg,IDC_CORNER),status);
	if (except!=IDC_CORNER_TL)	EnableWindow(GetDlgItem(hwndDlg,IDC_CORNER_TL),status);
	if (except!=IDC_CORNER_TR)	EnableWindow(GetDlgItem(hwndDlg,IDC_CORNER_TR),status);
	if (except!=IDC_CORNER_BR)	EnableWindow(GetDlgItem(hwndDlg,IDC_CORNER_BR),status);
	if (except!=IDC_CORNER_BL)	EnableWindow(GetDlgItem(hwndDlg,IDC_CORNER_BL),status);
	if (except!=IDC_CORNER_TL)	EnableWindow(GetDlgItem(hwndDlg,IDC_CORNER_TL),status);
	if (except!=IDC_MARGINLABLE)	EnableWindow(GetDlgItem(hwndDlg,IDC_MARGINLABLE),status);
	if (except!=IDC_MRGN_TOP)	EnableWindow(GetDlgItem(hwndDlg,IDC_MRGN_TOP),status);
	if (except!=IDC_MRGN_RIGHT)	EnableWindow(GetDlgItem(hwndDlg,IDC_MRGN_RIGHT),status);
	if (except!=IDC_MRGN_BOTTOM)	EnableWindow(GetDlgItem(hwndDlg,IDC_MRGN_BOTTOM),status);
	if (except!=IDC_MRGN_LEFT)	EnableWindow(GetDlgItem(hwndDlg,IDC_MRGN_LEFT),status);
	if (except!=IDC_MRGN_TOP_SPIN)	EnableWindow(GetDlgItem(hwndDlg,IDC_MRGN_TOP_SPIN),status);
	if (except!=IDC_MRGN_RIGHT_SPIN)	EnableWindow(GetDlgItem(hwndDlg,IDC_MRGN_RIGHT_SPIN),status);
	if (except!=IDC_MRGN_BOTTOM_SPIN)	EnableWindow(GetDlgItem(hwndDlg,IDC_MRGN_BOTTOM_SPIN),status);
	if (except!=IDC_MRGN_LEFT_SPIN)	EnableWindow(GetDlgItem(hwndDlg,IDC_MRGN_LEFT_SPIN),status);
	if (except!=IDC_BASECOLOUR)	EnableWindow(GetDlgItem(hwndDlg,IDC_BASECOLOUR),status);	
	if (except!=IDC_COLORLABLE)	EnableWindow(GetDlgItem(hwndDlg,IDC_COLORLABLE),status);
	if (except!=IDC_BASECOLOUR2)	EnableWindow(GetDlgItem(hwndDlg,IDC_BASECOLOUR2),status);
	if (except!=IDC_COLOR2LABLE)	EnableWindow(GetDlgItem(hwndDlg,IDC_COLOR2LABLE),status);
	if (except!=IDC_COLOR2_TRANSPARENT)	EnableWindow(GetDlgItem(hwndDlg,IDC_COLOR2_TRANSPARENT),status);
	if (except!=IDC_TEXTCOLOUR)	EnableWindow(GetDlgItem(hwndDlg,IDC_TEXTCOLOUR),status);
	if (except!=IDC_TEXTCOLOURLABLE)	EnableWindow(GetDlgItem(hwndDlg,IDC_TEXTCOLOURLABLE),status);

	if (except!=IDC_ALPHA)	EnableWindow(GetDlgItem(hwndDlg,IDC_ALPHA),status);
	if (except!=IDC_ALPHASPIN)	EnableWindow(GetDlgItem(hwndDlg,IDC_ALPHASPIN),status);
	if (except!=IDC_ALPHALABLE)	EnableWindow(GetDlgItem(hwndDlg,IDC_ALPHALABLE),status);
	if (except!=IDC_IGNORE)	EnableWindow(GetDlgItem(hwndDlg,IDC_IGNORE),status);	
}

// enabled all status controls if the selected item is a separator
BOOL CheckItem(int item, HWND hwndDlg)
{
	if (StatusItems[item].statusID==ID_EXTBKSEPARATOR)
	{
		ChangeControlItems(hwndDlg,0,0);
		return FALSE;
	}
	else
	{
		ChangeControlItems(hwndDlg,1,0);
		return TRUE;
	}
}

void SetChangedStatusItemFlag(WPARAM wParam, HWND hwndDlg)
{
	if (	
			// not non status item controls
			LOWORD(wParam)!=IDC_EXPORT
		&&	LOWORD(wParam)!=IDC_IMPORT
		&&	LOWORD(wParam)!=IDC_EQUALSELECTION
		&&	LOWORD(wParam)!=IDC_SELBLEND
		&&	LOWORD(wParam)!=IDC_FILLWALLPAPER
		&&	LOWORD(wParam)!=IDC_CENTERGROUPNAMES
		&&	LOWORD(wParam)!=IDC_ITEMS
			// focussed
		&&  (
				GetDlgItem(hwndDlg,LOWORD(wParam))==GetFocus()
				||	HIWORD(wParam)==CPN_COLOURCHANGED
			)
			// change message
		&&	( 
				HIWORD(wParam)==BN_CLICKED
			||	HIWORD(wParam)==EN_CHANGE
			||	HIWORD(wParam)==CPN_COLOURCHANGED)
		)
	{
		switch (LOWORD(wParam))
		{
		case IDC_IGNORE: ChangedSItems.bIGNORED = TRUE; break;
		case IDC_GRADIENT: ChangedSItems.bGRADIENT = TRUE; break;
		case IDC_GRADIENT_LR: ChangedSItems.bGRADIENT = TRUE;break;
		case IDC_GRADIENT_RL: ChangedSItems.bGRADIENT = TRUE; break;
		case IDC_GRADIENT_BT: ChangedSItems.bGRADIENT = TRUE; break;
		case IDC_GRADIENT_TB: ChangedSItems.bGRADIENT = TRUE; break;

		case IDC_CORNER: ChangedSItems.bCORNER = TRUE; break;
		case IDC_CORNER_TL: ChangedSItems.bCORNER = TRUE; break;
		case IDC_CORNER_TR: ChangedSItems.bCORNER = TRUE; break;
		case IDC_CORNER_BR: ChangedSItems.bCORNER = TRUE; break;
		case IDC_CORNER_BL: ChangedSItems.bCORNER = TRUE; break;

		case IDC_BASECOLOUR: ChangedSItems.bCOLOR = TRUE; break;		
		case IDC_BASECOLOUR2: ChangedSItems.bCOLOR2 = TRUE; break;
		case IDC_COLOR2_TRANSPARENT: ChangedSItems.bCOLOR2_TRANSPARENT = TRUE; break;
		case IDC_TEXTCOLOUR: ChangedSItems.bTEXTCOLOR = TRUE; break;

		case IDC_ALPHA: ChangedSItems.bALPHA = TRUE; break;
		case IDC_ALPHASPIN: ChangedSItems.bALPHA = TRUE; break;

		case IDC_MRGN_LEFT: ChangedSItems.bMARGIN_LEFT = TRUE; break;
		case IDC_MRGN_LEFT_SPIN: ChangedSItems.bMARGIN_LEFT = TRUE; break;

		case IDC_MRGN_TOP: ChangedSItems.bMARGIN_TOP = TRUE; break;
		case IDC_MRGN_TOP_SPIN: ChangedSItems.bMARGIN_TOP = TRUE; break;

		case IDC_MRGN_RIGHT: ChangedSItems.bMARGIN_RIGHT = TRUE; break;
		case IDC_MRGN_RIGHT_SPIN: ChangedSItems.bMARGIN_RIGHT = TRUE; break;

		case IDC_MRGN_BOTTOM: ChangedSItems.bMARGIN_BOTTOM = TRUE; break;
		case IDC_MRGN_BOTTOM_SPIN: ChangedSItems.bMARGIN_BOTTOM = TRUE; break;
		}

	}
}

BOOL isValidItem(void)
{
	if (StatusItems[LastModifiedItem].statusID==ID_EXTBKSEPARATOR)
		return FALSE;

	return TRUE;
}

void export(char *file)
{
	int n;
	char buffer[255];	
	for (n=0;n<sizeof(StatusItems)/sizeof(StatusItems[0]);n++)
	{	
		if (StatusItems[n].statusID!=ID_EXTBKSEPARATOR)
		{		
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_ALPHA");
			WritePrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].ALPHA),sizeof(StatusItems[n].ALPHA),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_COLOR");
			WritePrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].COLOR),sizeof(StatusItems[n].COLOR),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_COLOR2");
			WritePrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].COLOR2),sizeof(StatusItems[n].COLOR2),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_COLOR2_TRANSPARENT");
			WritePrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].COLOR2_TRANSPARENT),sizeof(StatusItems[n].COLOR2_TRANSPARENT),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_TEXTCOLOR");
			WritePrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].TEXTCOLOR),sizeof(StatusItems[n].TEXTCOLOR),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_CORNER");
			WritePrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].CORNER),sizeof(StatusItems[n].CORNER),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_GRADIENT");
			WritePrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].GRADIENT),sizeof(StatusItems[n].GRADIENT),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_IGNORED");
			WritePrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].IGNORED),sizeof(StatusItems[n].IGNORED),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_MARGIN_BOTTOM");
			WritePrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].MARGIN_BOTTOM),sizeof(StatusItems[n].MARGIN_BOTTOM),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_MARGIN_LEFT");
			WritePrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].MARGIN_LEFT),sizeof(StatusItems[n].MARGIN_LEFT),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_MARGIN_RIGHT");
			WritePrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].MARGIN_RIGHT),sizeof(StatusItems[n].MARGIN_RIGHT),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_MARGIN_TOP");
			WritePrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].MARGIN_TOP),sizeof(StatusItems[n].MARGIN_TOP),file);	
		}
	}	
}

void import(char *file, HWND hwndDlg)
{
	int n;
	char buffer[255];	
	for (n=0;n<sizeof(StatusItems)/sizeof(StatusItems[0]);n++)
	{	
		if (StatusItems[n].statusID!=ID_EXTBKSEPARATOR)
		{		
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_ALPHA");
			GetPrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].ALPHA),sizeof(StatusItems[n].ALPHA),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_COLOR");
			GetPrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].COLOR),sizeof(StatusItems[n].COLOR),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_COLOR2");
			GetPrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].COLOR2),sizeof(StatusItems[n].COLOR2),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_COLOR2_TRANSPARENT");
			GetPrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].COLOR2_TRANSPARENT),sizeof(StatusItems[n].COLOR2_TRANSPARENT),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_TEXTCOLOR");
			GetPrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].TEXTCOLOR),sizeof(StatusItems[n].TEXTCOLOR),file);
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_CORNER");
			GetPrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].CORNER),sizeof(StatusItems[n].CORNER),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_GRADIENT");
			GetPrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].GRADIENT),sizeof(StatusItems[n].GRADIENT),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_IGNORED");
			GetPrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].IGNORED),sizeof(StatusItems[n].IGNORED),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_MARGIN_BOTTOM");
			GetPrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].MARGIN_BOTTOM),sizeof(StatusItems[n].MARGIN_BOTTOM),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_MARGIN_LEFT");
			GetPrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].MARGIN_LEFT),sizeof(StatusItems[n].MARGIN_LEFT),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_MARGIN_RIGHT");
			GetPrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].MARGIN_RIGHT),sizeof(StatusItems[n].MARGIN_RIGHT),file);	
			lstrcpy(buffer,StatusItems[n].szDBname); lstrcat(buffer,"_MARGIN_TOP");
			GetPrivateProfileStruct("ExtBKSettings",buffer,&(StatusItems[n].MARGIN_TOP),sizeof(StatusItems[n].MARGIN_TOP),file);
		}
	}

	// refresh
	FillOptionDialogByCurrentSel(hwndDlg);
	ClcOptionsChanged();
}