// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
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
//
// File name      : $Source$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"



extern char gpszICQProtoName[MAX_PATH];
extern HANDLE hInst;
extern HANDLE ghServerNetlibUser;

static HWND hwndUploadContacts=NULL;
static const UINT settingsControls[]={IDC_STATIC12,IDC_GROUPS,IDC_ALLGROUPS,IDC_VISIBILITY,IDC_IGNORE,IDOK};



// Selects the "All contacts" checkbox if all other list entries
// are selected, deselects it if not.
static void UpdateAllContactsCheckmark(HWND hwndList, HANDLE hItemAll)
{

	int check = 1;
	HANDLE hContact;
	HANDLE hItem;

	
	hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);

	while (hContact)
	{
		hItem = (HANDLE)SendMessage(hwndList, CLM_FINDCONTACT, (WPARAM)hContact, 0);
		if (hItem && !SendMessage(hwndList, CLM_GETCHECKMARK, (WPARAM)hItem, 0))
		{ 
			check = 0;
			break;
		}
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}

	SendMessage(hwndList, CLM_SETCHECKMARK, (WPARAM)hItemAll, check);
	
}



// Loop over all contacts and update the checkmark
// that indicates wether or not they are already uploaded
static void UpdateCheckmarks(HWND hwndDlg, HANDLE hItemAll)
{
	
	HANDLE hContact;
	HANDLE hItem;
	char* szProto;
	
	
	hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while (hContact)
	{
		hItem = (HANDLE)SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM)hContact, 0);
		if (hItem)
		{
			szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
			if (szProto && !lstrcmp(szProto, gpszICQProtoName))
				SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, (WPARAM)hItem,
				DBGetContactSettingWord(hContact, gpszICQProtoName, "ServerId", 0) != 0);
		}
		hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
	
	// Update the "All contacts" checkmark
	UpdateAllContactsCheckmark(GetDlgItem(hwndDlg,IDC_CLIST),hItemAll);
	
}



static void AppendToUploadLog(HWND hwndDlg, const char *fmt, ...)
{

	va_list va;
	char szText[1024];
	int iItem;


	va_start(va, fmt);
	_vsnprintf(szText, sizeof(szText), fmt, va);
	va_end(va);

	iItem = SendDlgItemMessage(hwndDlg, IDC_LOG, LB_ADDSTRING, 0, (LPARAM)szText);
	SendDlgItemMessage(hwndDlg, IDC_LOG, LB_SETTOPINDEX, iItem, 0);

}



static void DeleteLastUploadLogLine(HWND hwndDlg)
{

	SendDlgItemMessage(hwndDlg, IDC_LOG, LB_DELETESTRING, SendDlgItemMessage(hwndDlg, IDC_LOG, LB_GETCOUNT, 0, 0)-1, 0);

}



static void GetLastUploadLogLine(HWND hwndDlg, char *szBuf)
{

	SendDlgItemMessage(hwndDlg, IDC_LOG, LB_GETTEXT, SendDlgItemMessage(hwndDlg, IDC_LOG, LB_GETCOUNT, 0, 0)-1, (LPARAM)szBuf);

}



#define ACTION_ADDWITHOUTAUTH   0
#define ACTION_ADDWITHAUTH      1
#define ACTION_REMOVE           2
#define M_PROTOACK    (WM_USER+100)
#define M_UPLOADMORE  (WM_USER+101)
static BOOL CALLBACK DlgProcUploadList(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam)
{

	static int working;
	static HANDLE hItemAll;
	static HANDLE hProtoAckHook;
	static int currentSequence;
	static int currentAction;
	static HANDLE hCurrentContact;
	static WORD wNewContactId;
	static WORD wNewGroupId;


	switch(message)
	{
		
	case WM_INITDIALOG:
		{
			
			CLCINFOITEM cii = {0};
			
			
			TranslateDialogDefault(hwndDlg);
			working = 0;
			hProtoAckHook = NULL;
			
			// Add the "All contacts" item
			cii.cbSize = sizeof(cii);
			cii.flags = CLCIIF_GROUPFONT | CLCIIF_CHECKBOX;
			cii.pszText = Translate("** All contacts **");
			hItemAll = (HANDLE)SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_ADDINFOITEM, 0, (LPARAM)&cii);
			
			UpdateCheckmarks(hwndDlg, hItemAll);
		}
		
		return TRUE;
		
		
		// The M_PROTOACK message is received when the
		// server has responded to our last update packet
	case M_PROTOACK:
		{	

			ACKDATA *ack = (ACKDATA*)lParam;
			char szLastLogLine[256];

			
			// Is this an ack we are waiting for?
			if (lstrcmp(ack->szModule, gpszICQProtoName))
				break;

			if (ack->type != ICQACKTYPE_SERVERCLIST)
				break;

			if ((int)ack->hProcess != currentSequence)
				break;
			

			switch (currentAction)
			{

			case ACTION_ADDWITHOUTAUTH:
				if (ack->result == ACKRESULT_SUCCESS)
				{

					DBWriteContactSettingByte(hCurrentContact, gpszICQProtoName, "Auth", 0);
					DBWriteContactSettingWord(hCurrentContact, gpszICQProtoName, "ServerId", wNewContactId);
					DBWriteContactSettingWord(hCurrentContact, gpszICQProtoName, "SrvGroupId", wNewGroupId);
					break;

				}
				else
				{
					// If the server refused to add the contact without authorization,
					// we try again _with_ authorization TLV

					char* pszNick = NULL;
					DWORD dwUin;
					DBVARIANT dbv;

					
					// Only upload custom nicks
					if (!DBGetContactSetting(hCurrentContact, "CList", "MyHandle", &dbv)) {
						if (dbv.pszVal && strlen(dbv.pszVal) > 0)
							pszNick = _strdup(dbv.pszVal);
						DBFreeVariant(&dbv);
					}
					
					// Update the Auth setting
					DBWriteContactSettingByte(hCurrentContact, gpszICQProtoName, "Auth", 1);
					currentAction = ACTION_ADDWITHAUTH;

					// TODO: Ansi->UTF8
					dwUin = DBGetContactSettingDword(hCurrentContact, gpszICQProtoName, UNIQUEIDSETTING, 0);
					currentSequence = icq_sendUploadContactServ(dwUin, wNewGroupId, wNewContactId, pszNick, 1, SSI_ITEM_BUDDY);

					SAFE_FREE(pszNick);

					return FALSE;

				}
				
			case ACTION_ADDWITHAUTH:
				if (ack->result == ACKRESULT_SUCCESS)
				{
					DBWriteContactSettingWord(hCurrentContact, gpszICQProtoName, "ServerId", wNewContactId);
					DBWriteContactSettingWord(hCurrentContact, gpszICQProtoName, "SrvGroupId", wNewGroupId);
				}
				break;

			case ACTION_REMOVE:
				if (ack->result == ACKRESULT_SUCCESS)
				{
					DBWriteContactSettingWord(hCurrentContact, gpszICQProtoName, "ServerId", 0);
					DBWriteContactSettingWord(hCurrentContact, gpszICQProtoName, "SrvGroupId", 0);
				}
				break;

			}
			
			// Update the log window
			GetLastUploadLogLine(hwndDlg, szLastLogLine);
			DeleteLastUploadLogLine(hwndDlg);
			AppendToUploadLog(hwndDlg, "%s%s", szLastLogLine,
				Translate(ack->result == ACKRESULT_SUCCESS ? "OK" : "FAILED"));
			SendMessage(hwndDlg, M_UPLOADMORE, 0, 0);

		}
		break;
		
		// The M_UPLOADMORE window message is received when the user presses 'Update'
		// and every time an ack from the server has been taken care of.
	case M_UPLOADMORE:
		{

			HANDLE hContact;
			HANDLE hItem;
			DWORD dwUin;
			char* pszNick;
			int isChecked;
			int isOnServer;

			
			// Iterate over all contacts until one is found that
			// needs to be updated on the server
			hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
			
			while (hContact)
			{
				
				hItem = (HANDLE)SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM)hContact, 0);
				if (hItem)
				{
					isChecked = SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_GETCHECKMARK, (WPARAM)hItem, 0) != 0;
					isOnServer = DBGetContactSettingWord(hContact, gpszICQProtoName, "ServerId", 0) != 0;
					dwUin = DBGetContactSettingDword(hContact, gpszICQProtoName, UNIQUEIDSETTING, 0);

					// Is this one out of sync?
					if (dwUin && (isChecked != isOnServer))
					{

						DBVARIANT dbv;

						
						hCurrentContact = hContact;
						
						// Only upload custom nicks
						pszNick = NULL;
						if (!DBGetContactSetting(hContact, "CList", "MyHandle", &dbv))
						{
							if (dbv.pszVal && strlen(dbv.pszVal) > 0)
								pszNick = _strdup(dbv.pszVal);
							DBFreeVariant(&dbv);
						}
						
						if (isChecked)
						{

							// Queue for uploading
							if (pszNick)
								AppendToUploadLog(hwndDlg, Translate("Uploading %s..."), pszNick);
							else 
								AppendToUploadLog(hwndDlg, Translate("Uploading %u..."), dwUin);
							currentAction = ACTION_ADDWITHOUTAUTH;
							
							// Only upload contact if we have a default group ID
							if ((wNewGroupId = (WORD)DBGetContactSettingWord(NULL, gpszICQProtoName, "SrvDefGroupId", 0)) != 0)
							{
								wNewContactId = GenerateServerId();
								currentSequence = icq_sendUploadContactServ(dwUin, wNewGroupId, wNewContactId, pszNick, 0, SSI_ITEM_BUDDY);
								return FALSE;
							}
							else
							{
								char szLastLogLine[256];
								// Update the log window with the failure and continue with next contact
								GetLastUploadLogLine(hwndDlg, szLastLogLine);
								DeleteLastUploadLogLine(hwndDlg);
								AppendToUploadLog(hwndDlg, "%s%s", szLastLogLine, Translate("FAILED"));
								AppendToUploadLog(hwndDlg, Translate("No upload group available"));
								Netlib_Logf(ghServerNetlibUser, "Upload failed, no default group");
							}
						}
						else
						{
							// Queue for deletion
							if (pszNick)
								AppendToUploadLog(hwndDlg, Translate("Deleting %s..."), pszNick);
							else 
								AppendToUploadLog(hwndDlg, Translate("Deleting %u..."), dwUin);
							currentAction = ACTION_REMOVE;
							currentSequence = icq_sendDeleteServerContactServ(dwUin,
								(WORD)DBGetContactSettingWord(hContact, gpszICQProtoName, "SrvGroupId", 0),
								(WORD)DBGetContactSettingWord(hContact, gpszICQProtoName, "ServerId", 0),
								SSI_ITEM_BUDDY);
						}
						
						SAFE_FREE(pszNick);
						
						break;
					}
				}
				
				hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
				
			}
			
			if (hContact == NULL)
			{
				// All contacts are in sync
				AppendToUploadLog(hwndDlg, Translate("All operations complete"));
				SetDlgItemText(hwndDlg, IDCANCEL, Translate("Close"));
				sendAddEnd();
				working = 0;
				SendDlgItemMessage(hwndDlg,IDC_CLIST,CLM_SETGREYOUTFLAGS,0,0);
				UpdateCheckmarks(hwndDlg, hItemAll);
				if (hProtoAckHook)
					UnhookEvent(hProtoAckHook);
			}
			
			break;
		}
		
		
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				
			case IDOK:
				working = 1;
				icq_ShowMultipleControls(hwndDlg, settingsControls, sizeof(settingsControls)/sizeof(settingsControls[0]), SW_HIDE);
				ShowWindow(GetDlgItem(hwndDlg, IDC_LOG), SW_SHOW);
				SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETGREYOUTFLAGS, 0xFFFFFFFF, 0);
				InvalidateRect(GetDlgItem(hwndDlg, IDC_CLIST), NULL, FALSE);
				hProtoAckHook = HookEventMessage(ME_PROTO_ACK, hwndDlg, M_PROTOACK);
				sendAddStart();
				PostMessage(hwndDlg, M_UPLOADMORE, 0, 0);
				break;
				
			case IDCANCEL:
				DestroyWindow(hwndDlg);
				break;
				
			}
		}
		break;
		
	case WM_NOTIFY:
		{
			switch(((NMHDR*)lParam)->idFrom)
			{
				
			case IDC_CLIST:
				{
					switch(((NMHDR*)lParam)->code)
					{
						
					case CLN_OPTIONSCHANGED:
						{
							int i;
							
							SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETLEFTMARGIN, 2, 0);
							SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETBKBITMAP, 0, (LPARAM)(HBITMAP)NULL);
							SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETBKCOLOR, GetSysColor(COLOR_WINDOW), 0);
							SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETGREYOUTFLAGS, working?0xFFFFFFFF:0, 0);
							for (i=0; i<=FONTID_MAX; i++)
								SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETTEXTCOLOR, i, GetSysColor(COLOR_WINDOWTEXT));
						}
						break;
						
					case CLN_LISTREBUILT:
						{
							HANDLE hContact;
							HANDLE hItem;
							char* szProto;
							
							// Delete non-icq contacts
							hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
							while (hContact)
							{
								hItem = (HANDLE)SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM)hContact, 0);
								if (hItem)
								{
									szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
									if (szProto == NULL || lstrcmp(szProto, gpszICQProtoName))
										SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_DELETEITEM, (WPARAM)hItem, 0);
								}
								hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
							}
						}
						break;
						
					case CLN_CHECKCHANGED:
						{
							NMCLISTCONTROL *nm = (NMCLISTCONTROL*)lParam;
							HANDLE hContact;
							HANDLE hItem;
							
							if (nm->flags&CLNF_ISINFO)
							{
								
								int check;
								
								
								check = SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_GETCHECKMARK, (WPARAM)hItemAll, 0);
								
								hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
								while (hContact)
								{
									hItem = (HANDLE)SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_FINDCONTACT, (WPARAM)hContact, 0);
									if (hItem)
										SendDlgItemMessage(hwndDlg, IDC_CLIST, CLM_SETCHECKMARK, (WPARAM)hItem, check);
									hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
								}
							}
							else
							{
								UpdateAllContactsCheckmark(GetDlgItem(hwndDlg, IDC_CLIST), hItemAll);
							}
						}
						break;
						
					}
				}
				break;

			}
		}
		break;
		
	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		break;
		
	case WM_DESTROY:
		if (hProtoAckHook)
			UnhookEvent(hProtoAckHook);
		hwndUploadContacts = NULL;
		working = 0;
		break;
		
	}
	
	
	return FALSE;
	
}



void ShowUploadContactsDialog(void)
{

	if (hwndUploadContacts == NULL)
		hwndUploadContacts = CreateDialog(hInst, MAKEINTRESOURCE(IDD_ICQUPLOADLIST), NULL, DlgProcUploadList);

	SetForegroundWindow(hwndUploadContacts);

}
