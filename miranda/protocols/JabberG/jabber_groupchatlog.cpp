/*

Jabber Protocol Plugin for Miranda IM
Copyright (C) 2002-2004  Santithorn Bunchua

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

#include "jabber.h"
#include "jabber_list.h"
#include "jabber_iq.h"
#include "resource.h"
#include <richedit.h>
#include <win2k.h>
#include "sdk/m_smileyadd.h"

#define TIMERID_FLASHWND	0
#define TIMEOUT_FLASHWND	1000

extern UINT jabberCodePage;

static HCURSOR hCursorSizeNS;
static HCURSOR hCursorSizeWE;
static JABBER_GCLOG_FONT gcLogFont[JABBER_GCLOG_NUM_FONT];

typedef struct {
	HANDLE hEvent;
	char *jid;
} JABBER_GCLOG_INITDIALOG;

void JabberGcLogLoadFont(JABBER_GCLOG_FONT *fontInfo)
{
	DBVARIANT dbv;
	char key[32];
	int i;
	static COLORREF defColor[JABBER_GCLOG_NUM_FONT] = { RGB(0,0,0), RGB(0,0,0), RGB(128,128,128), RGB(128,0,0), RGB(0,128,0), RGB(0,0,128) };

	for (i=0; i<JABBER_GCLOG_NUM_FONT; i++) {
		sprintf(key, "GcLogFont%dFace", i);
		if (!DBGetContactSetting(NULL, jabberProtoName, key, &dbv)) {
			strncpy(fontInfo[i].face, dbv.pszVal, sizeof(fontInfo[i].face));
			DBFreeVariant(&dbv);
		}
		else
			fontInfo[i].face[0] = '\0';
		sprintf(key, "GcLogFont%dSize", i);
		fontInfo[i].size = (char) DBGetContactSettingByte(NULL, jabberProtoName, key, 10);
		sprintf(key, "GcLogFont%dStyle", i);
		fontInfo[i].style = DBGetContactSettingByte(NULL, jabberProtoName, key, 0);
		sprintf(key, "GcLogFont%dCharset", i);
		fontInfo[i].charset = DBGetContactSettingByte(NULL, jabberProtoName, key, ANSI_CHARSET);
		sprintf(key, "GcLogFont%dColor", i);
		fontInfo[i].color = DBGetContactSettingDword(NULL, jabberProtoName, key, defColor[i]);
	}
}

static BOOL CALLBACK JabberGcLogEditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	JABBER_GCLOG_INFO *gcLogInfo;

	switch (msg) {
	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS; //DLGC_WANTARROWS|DLGC_WANTCHARS|DLGC_HASSETSEL|DLGC_WANTALLKEYS;
	case WM_CHAR:
		if (wParam=='\r' || wParam=='\n') {
			if (((GetKeyState(VK_CONTROL)&0x8000)==0) == DBGetContactSettingByte(NULL, jabberProtoName, "GcLogSendOnEnter", TRUE)) {
				PostMessage(GetParent(hwnd), WM_COMMAND, IDOK, 0);
				return 0;
			}
		}
		break;
	}
	gcLogInfo = (JABBER_GCLOG_INFO *) GetWindowLong(GetParent(hwnd), GWL_USERDATA);
	return CallWindowProc(gcLogInfo->oldEditWndProc, hwnd, msg, wParam, lParam);
}

static BOOL CALLBACK JabberGcLogHSplitterWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	JABBER_GCLOG_INFO *gcLogInfo;

	switch (msg) {
	case WM_NCHITTEST:
		return HTCLIENT;
	case WM_SETCURSOR:
		SetCursor(hCursorSizeNS);
		return TRUE;
	case WM_LBUTTONDOWN:
		SetCapture(hwnd);
		return 0;
	case WM_MOUSEMOVE:
		if (GetCapture() == hwnd) {
			HWND hParent;
			RECT rc;
			POINT pt;
			
			hParent = GetParent(hwnd);
			gcLogInfo = (JABBER_GCLOG_INFO *) GetWindowLong(hParent, GWL_USERDATA);
			pt.y = HIWORD(GetMessagePos());
			GetClientRect(hParent, &rc);
			ScreenToClient(hParent, &pt);
			if (pt.y < gcLogInfo->hSplitterMinAbove)
				pt.y = gcLogInfo->hSplitterMinAbove;
			if (rc.bottom-pt.y < gcLogInfo->hSplitterMinBelow)
				pt.y = rc.bottom-gcLogInfo->hSplitterMinBelow;
			gcLogInfo->hSplitterPos = rc.bottom-pt.y;
			SendMessage(hParent, WM_SIZE, SIZE_RESTORED, (rc.bottom<<16)+rc.right);
		}
		return 0;
	case WM_LBUTTONUP:
		ReleaseCapture();
		return 0;
	}
	gcLogInfo = (JABBER_GCLOG_INFO *) GetWindowLong(GetParent(hwnd), GWL_USERDATA);
	return CallWindowProc(gcLogInfo->oldHSplitterWndProc, hwnd, msg, wParam, lParam);
}

static BOOL CALLBACK JabberGcLogVSplitterWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	JABBER_GCLOG_INFO *gcLogInfo;

	switch (msg) {
	case WM_NCHITTEST:
		return HTCLIENT;
	case WM_SETCURSOR:
		SetCursor(hCursorSizeWE);
		return TRUE;
	case WM_LBUTTONDOWN:
		SetCapture(hwnd);
		return 0;
	case WM_MOUSEMOVE:
		if (GetCapture() == hwnd) {
			HWND hParent;
			RECT rc;
			POINT pt;
			
			hParent = GetParent(hwnd);
			gcLogInfo = (JABBER_GCLOG_INFO *) GetWindowLong(hParent, GWL_USERDATA);
			pt.x = LOWORD(GetMessagePos());
			GetClientRect(hParent, &rc);
			ScreenToClient(hParent, &pt);
			if (pt.x < gcLogInfo->vSplitterMinLeft)
				pt.x = gcLogInfo->vSplitterMinLeft;
			if (rc.right-pt.x < gcLogInfo->vSplitterMinRight)
				pt.x = rc.right-gcLogInfo->vSplitterMinRight;
			gcLogInfo->vSplitterPos = rc.right-pt.x;
			SendMessage(hParent, WM_SIZE, SIZE_RESTORED, (rc.bottom<<16)+rc.right);
		}
		return 0;
	case WM_LBUTTONUP:
		ReleaseCapture();
		return 0;
	}
	gcLogInfo = (JABBER_GCLOG_INFO *) GetWindowLong(GetParent(hwnd), GWL_USERDATA);
	return CallWindowProc(gcLogInfo->oldHSplitterWndProc, hwnd, msg, wParam, lParam);
}

BOOL CALLBACK JabberGcLogInputDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	JABBER_GCLOG_INPUT_INFO *gcLogInputInfo;

	switch (msg) {
	case WM_INITDIALOG:
		// lParam is (JABBER_GCLOG_INPUT_INFO *)
		{
			JABBER_GCLOG_INFO *gcLogInfo;
			char text[1024];
			char *localRoomJid, *localNick;

			TranslateDialogDefault(hwndDlg);
			gcLogInputInfo = (JABBER_GCLOG_INPUT_INFO *) lParam;
			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) gcLogInputInfo);
			switch (gcLogInputInfo->type) {
			case MUC_SETTOPIC:
				gcLogInfo = gcLogInputInfo->gcLogInfo;
				localRoomJid = JabberTextDecode(gcLogInfo->roomJid);
				_snprintf(text, sizeof(text), "%s %s", Translate("Set topic for"), localRoomJid);
				SetWindowText(hwndDlg, text);
				free(localRoomJid);
				GetDlgItemText(GetParent(hwndDlg), IDC_TOPIC, text, sizeof(text));
				SetDlgItemText(hwndDlg, IDC_TOPIC, text);
				break;
			case MUC_CHANGENICK:
				gcLogInfo = gcLogInputInfo->gcLogInfo;
				localRoomJid = JabberTextDecode(gcLogInfo->roomJid);
				_snprintf(text, sizeof(text), "%s %s", Translate("Change nickname in"), localRoomJid);
				SetWindowText(hwndDlg, text);
				free(localRoomJid);
				SetDlgItemText(hwndDlg, IDC_TOPIC, gcLogInputInfo->nick);
				SetDlgItemText(hwndDlg, IDOK, Translate("Change"));
				break;
			case MUC_KICKREASON:
				localNick = JabberTextDecode(gcLogInputInfo->nick);
				_snprintf(text, sizeof(text), "%s %s", Translate("Reason to kick"), localNick);
				SetWindowText(hwndDlg, text);
				free(localNick);
				SetDlgItemText(hwndDlg, IDOK, Translate("Kick"));
				break;
			case MUC_BANREASON:
				localNick = JabberTextDecode(gcLogInputInfo->nick);
				_snprintf(text, sizeof(text), "%s %s", Translate("Reason to ban"), localNick);
				SetWindowText(hwndDlg, text);
				free(localNick);
				SetDlgItemText(hwndDlg, IDOK, Translate("Ban"));
				break;
			case MUC_DESTROYREASON:
				gcLogInfo = gcLogInputInfo->gcLogInfo;
				localRoomJid = JabberTextDecode(gcLogInfo->roomJid);
				_snprintf(text, sizeof(text), "%s %s", Translate("Reason to destroy"), localRoomJid);
				SetWindowText(hwndDlg, text);
				free(localRoomJid);
				SetDlgItemText(hwndDlg, IDOK, Translate("Destroy"));
				break;
			case MUC_ADDJID:
				localRoomJid = JabberTextDecode(gcLogInputInfo->jidListInfo->roomJid);
				_snprintf(text, sizeof(text), "%s %s (%s)",
					Translate("Add to"),
					(gcLogInputInfo->jidListInfo->type==MUC_VOICELIST) ? Translate("Voice List") :
					(gcLogInputInfo->jidListInfo->type==MUC_MEMBERLIST) ? Translate("Member List") :
					(gcLogInputInfo->jidListInfo->type==MUC_MODERATORLIST) ? Translate("Moderator List") :
					(gcLogInputInfo->jidListInfo->type==MUC_BANLIST) ? Translate("Ban List") :
					(gcLogInputInfo->jidListInfo->type==MUC_ADMINLIST) ? Translate("Admin List") :
					(gcLogInputInfo->jidListInfo->type==MUC_OWNERLIST) ? Translate("Owner List") :
					Translate("JID List"),
					localRoomJid);
				SetWindowText(hwndDlg, text);
				SetDlgItemText(hwndDlg, IDOK, Translate("Add"));
				free(localRoomJid);
				break;
			}
		}
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			if (jabberOnline) {
				JABBER_GCLOG_INFO *gcLogInfo;
				char text[1024];
				char *str;
				JABBER_LIST_ITEM *item;

				gcLogInputInfo = (JABBER_GCLOG_INPUT_INFO *) GetWindowLong(hwndDlg, GWL_USERDATA);
				gcLogInfo = gcLogInputInfo->gcLogInfo;
				GetDlgItemText(hwndDlg, IDC_TOPIC, text, sizeof(text));
				text[sizeof(text)-1] = '\0';
				switch (gcLogInputInfo->type) {
				case MUC_SETTOPIC:
					if ((str=JabberTextEncode(text)) != NULL) {
						JabberSend(jabberThreadInfo->s, "<message to='%s' type='groupchat'><subject>%s</subject><body>/me has set the topic to: %s</body></message>", gcLogInfo->roomJid, str, str);
						free(str);
					}
					break;
				case MUC_CHANGENICK:
					if ((item=JabberListGetItemPtr(LIST_CHATROOM, gcLogInfo->roomJid)) != NULL) {
						if ((str=JabberTextEncode(text)) != NULL) {
							_snprintf(text, sizeof(text), "%s/%s", gcLogInfo->roomJid, str);
							JabberSendPresenceTo(jabberStatus, text, NULL);
							if (item->newNick != NULL)
								free(item->newNick);
							item->newNick = str;	// so, no need to free(str) here
						}
					}
					break;
				case MUC_KICKREASON:
					if ((str=JabberTextEncode(text)) != NULL) {
						JabberSend(jabberThreadInfo->s, "<iq type='set' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item nick='%s' role='none'><reason>%s</reason></item></query></iq>", gcLogInfo->roomJid, gcLogInputInfo->nick, str);
						free(str);
					}
					break;
				case MUC_BANREASON:
					if ((str=JabberTextEncode(text)) != NULL) {
						JabberSend(jabberThreadInfo->s, "<iq type='set' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item nick='%s' affiliation='outcast'><reason>%s</reason></item></query></iq>", gcLogInfo->roomJid, gcLogInputInfo->nick, str);
						free(str);
					}
					break;
				case MUC_DESTROYREASON:
					if ((str=JabberTextEncode(text)) != NULL) {
						JabberSend(jabberThreadInfo->s, "<iq type='set' to='%s'><query xmlns='http://jabber.org/protocol/muc#owner'><destroy><reason>%s</reason></destroy></query></iq>", gcLogInfo->roomJid, str);
						free(str);
					}
					break;
				case MUC_ADDJID:
					{
						char *p, *q;
						int iqId;

						// Trim leading and trailing whitespaces
						p = text;
						for (p=text; *p!='\0' && isspace(*p); p++);
						for (q=p; *q!='\0' && !isspace(*q); q++);
						if (*q != '\0') *q = '\0';
						if (*p != '\0') {
							if ((str=JabberTextEncode(p)) != NULL) {
								switch (gcLogInputInfo->jidListInfo->type) {
								case MUC_VOICELIST:
									JabberSend(jabberThreadInfo->s, "<iq type='set' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item %s='%s' role='participant'/></query></iq>", gcLogInputInfo->jidListInfo->roomJid, strchr(str,'@')?"jid":"nick", str);
									iqId = JabberSerialNext();
									JabberIqAdd(iqId, IQ_PROC_NONE, JabberIqResultMucGetVoiceList);
									JabberSend(jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item role='participant'/></query></iq>", iqId, gcLogInputInfo->jidListInfo->roomJid);
									break;
								case MUC_MEMBERLIST:
									JabberSend(jabberThreadInfo->s, "<iq type='set' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item %s='%s' affiliation='member'/></query></iq>", gcLogInputInfo->jidListInfo->roomJid, strchr(str,'@')?"jid":"nick", str);
									iqId = JabberSerialNext();
									JabberIqAdd(iqId, IQ_PROC_NONE, JabberIqResultMucGetMemberList);
									JabberSend(jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item affiliation='member'/></query></iq>", iqId, gcLogInputInfo->jidListInfo->roomJid);
									break;
								case MUC_MODERATORLIST:
									JabberSend(jabberThreadInfo->s, "<iq type='set' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item %s='%s' role='moderator'/></query></iq>", gcLogInputInfo->jidListInfo->roomJid, strchr(str,'@')?"jid":"nick", str);
									iqId = JabberSerialNext();
									JabberIqAdd(iqId, IQ_PROC_NONE, JabberIqResultMucGetModeratorList);
									JabberSend(jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item role='moderator'/></query></iq>", iqId, gcLogInputInfo->jidListInfo->roomJid);
									break;
								case MUC_BANLIST:
									JabberSend(jabberThreadInfo->s, "<iq type='set' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item %s='%s' affiliation='outcast'/></query></iq>", gcLogInputInfo->jidListInfo->roomJid, strchr(str,'@')?"jid":"nick", str);
									iqId = JabberSerialNext();
									JabberIqAdd(iqId, IQ_PROC_NONE, JabberIqResultMucGetBanList);
									JabberSend(jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item affiliation='outcast'/></query></iq>", iqId, gcLogInputInfo->jidListInfo->roomJid);
									break;
								case MUC_ADMINLIST:
									JabberSend(jabberThreadInfo->s, "<iq type='set' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item %s='%s' affiliation='admin'/></query></iq>", gcLogInputInfo->jidListInfo->roomJid, strchr(str,'@')?"jid":"nick", str);
									iqId = JabberSerialNext();
									JabberIqAdd(iqId, IQ_PROC_NONE, JabberIqResultMucGetAdminList);
									JabberSend(jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='http://jabber.org/protocol/muc#owner'><item affiliation='admin'/></query></iq>", iqId, gcLogInputInfo->jidListInfo->roomJid);
									break;
								case MUC_OWNERLIST:
									JabberSend(jabberThreadInfo->s, "<iq type='set' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item %s='%s' affiliation='owner'/></query></iq>", gcLogInputInfo->jidListInfo->roomJid, strchr(str,'@')?"jid":"nick", str);
									iqId = JabberSerialNext();
									JabberIqAdd(iqId, IQ_PROC_NONE, JabberIqResultMucGetOwnerList);
									JabberSend(jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='http://jabber.org/protocol/muc#owner'><item affiliation='owner'/></query></iq>", iqId, gcLogInputInfo->jidListInfo->roomJid);
									break;
								}
								free(str);
							}
						}
					}
					break;
				}
			}
			// fall through
		case IDCANCEL:
			gcLogInputInfo = (JABBER_GCLOG_INPUT_INFO *) GetWindowLong(hwndDlg, GWL_USERDATA);
			switch (gcLogInputInfo->type) {
			case MUC_SETTOPIC:
				gcLogInputInfo->gcLogInfo->hwndSetTopic = NULL;
				break;
			case MUC_CHANGENICK:
				gcLogInputInfo->gcLogInfo->hwndChangeNick = NULL;
				break;
			case MUC_KICKREASON:
				gcLogInputInfo->gcLogInfo->hwndKickReason = NULL;
				break;
			case MUC_BANREASON:
				gcLogInputInfo->gcLogInfo->hwndBanReason = NULL;
				break;
			case MUC_DESTROYREASON:
				gcLogInputInfo->gcLogInfo->hwndDestroyReason = NULL;
				break;
			}
			DestroyWindow(hwndDlg);
			break;
		}
		break;
	case WM_DESTROY:
		gcLogInputInfo = (JABBER_GCLOG_INPUT_INFO *) GetWindowLong(hwndDlg, GWL_USERDATA);
		if (gcLogInputInfo->nick) free(gcLogInputInfo->nick);
		free(gcLogInputInfo);
		break;
	}
	return FALSE;
}

static BOOL CALLBACK JabberGcLogInviteDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		{
			int index;
			HWND hwndComboBox;
			JABBER_LIST_ITEM *item;
			char *str;
			int n;

			TranslateDialogDefault(hwndDlg);
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInst, MAKEINTRESOURCE(IDI_GROUP)));
			if ((str=JabberTextDecode((char *) lParam)) != NULL) {
				SetDlgItemText(hwndDlg, IDC_ROOM, str);
			}
			hwndComboBox = GetDlgItem(hwndDlg, IDC_USER);
			index = 0;
			while ((index=JabberListFindNext(LIST_ROSTER, index)) >= 0) {
				item = JabberListGetItemPtrFromIndex(index);
				if (item->status != ID_STATUS_OFFLINE) {
					// Add every non-offline users to the combobox
					str = JabberTextDecode(item->jid);
					n = SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM) str);
					SendMessage(hwndComboBox, CB_SETITEMDATA, n, (LPARAM) item->jid);
					free(str);
				}
				index++;
			}
			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) _strdup((char *) lParam));
		}
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_INVITE:
			{
				char text[256];
				char *room, *user, *reason;
				HWND hwndComboBox;
				int iqId, n;

				hwndComboBox = GetDlgItem(hwndDlg, IDC_USER);
				if ((room=(char *) GetWindowLong(hwndDlg, GWL_USERDATA)) != NULL) {
					n = SendMessage(hwndComboBox, CB_GETCURSEL, 0, 0);
					if (n < 0) {
						// Use text box string because no selection is made
						GetWindowText(hwndComboBox, text, sizeof(text));
						user = JabberTextEncode(text);
					}
					else {
						user = (char *) SendMessage(hwndComboBox, CB_GETITEMDATA, n, 0);
					}
					if (user != NULL) {
						GetDlgItemText(hwndDlg, IDC_REASON, text, sizeof(text));
						iqId = JabberSerialNext();
						if (text[0]!='\0' && (reason=JabberTextEncode(text))!=NULL) {
							JabberSend(jabberThreadInfo->s, "<message id='"JABBER_IQID"%d' to='%s'><x xmlns='http://jabber.org/protocol/muc#user'><invite to='%s'><reason>%s</reason></invite></x></message>", iqId, room, user, reason);
							free(reason);
						}
						else {
							JabberSend(jabberThreadInfo->s, "<message id='"JABBER_IQID"%d' to='%s'><x xmlns='http://jabber.org/protocol/muc#user'><invite to='%s'></invite></x></message>", iqId, room, user);
						}
						if (n < 0) free(user);
					}
				}
			}
			// Fall through
		case IDCANCEL:
		case IDCLOSE:
			DestroyWindow(hwndDlg);
			return TRUE;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		break;
	case WM_DESTROY:
		{
			char *str;
			
			if ((str=(char *) GetWindowLong(hwndDlg, GWL_USERDATA)) != NULL)
				free(str);
		}
		break;
	}

	return FALSE;
}

static VOID CALLBACK JabberGcLogSendMessageApcProc(DWORD param)
{
	CallService(MS_MSG_SENDMESSAGE, (WPARAM) param, (LPARAM) NULL);
}

static BOOL CALLBACK JabberGcLogDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	JABBER_GCLOG_INFO *gcLogInfo;
	JABBER_GCLOG_INITDIALOG *gcInit;
	static HFONT hFont;

	gcLogInfo = (JABBER_GCLOG_INFO *) GetWindowLong(hwndDlg, GWL_USERDATA);

	switch (msg) {
	case WM_INITDIALOG:
		{
			RECT rect;
			JABBER_LIST_ITEM *item;
			char *localJid;

			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hInst, MAKEINTRESOURCE(IDI_GROUP)));
			TranslateDialogDefault(hwndDlg);
			gcLogInfo = (JABBER_GCLOG_INFO *) malloc(sizeof(JABBER_GCLOG_INFO));
			gcLogInfo->roomJid = NULL;
			gcLogInfo->vSplitterPos = DBGetContactSettingDword(NULL, jabberProtoName, "GcLogVSplit", 100);
			gcLogInfo->hSplitterPos = DBGetContactSettingDword(NULL, jabberProtoName, "GcLogHSplit", 40);
			gcLogInfo->hSplitterMinAbove = 70;
			gcLogInfo->hSplitterMinBelow = 40;
			gcLogInfo->vSplitterMinLeft = 70;
			gcLogInfo->vSplitterMinRight = 70;
			gcLogInfo->oldEditWndProc = (WNDPROC) GetWindowLong(GetDlgItem(hwndDlg, IDC_EDIT), GWL_WNDPROC);
			gcLogInfo->oldHSplitterWndProc = (WNDPROC) GetWindowLong(GetDlgItem(hwndDlg, IDC_HSPLIT), GWL_WNDPROC);
			gcLogInfo->oldVSplitterWndProc = (WNDPROC) GetWindowLong(GetDlgItem(hwndDlg, IDC_VSPLIT), GWL_WNDPROC);
			gcLogInfo->hwndSetTopic = NULL;
			gcLogInfo->hwndChangeNick = NULL;
			gcLogInfo->hwndKickReason = NULL;
			gcLogInfo->hwndBanReason = NULL;
			gcLogInfo->hwndDestroyReason = NULL;
			gcLogInfo->nFlash = 0;
			SetWindowLong(GetDlgItem(hwndDlg, IDC_EDIT), GWL_WNDPROC, (LONG) JabberGcLogEditWndProc);
			SetWindowLong(GetDlgItem(hwndDlg, IDC_HSPLIT), GWL_WNDPROC, (LONG) JabberGcLogHSplitterWndProc);
			SetWindowLong(GetDlgItem(hwndDlg, IDC_VSPLIT), GWL_WNDPROC, (LONG) JabberGcLogVSplitterWndProc);
			SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) gcLogInfo);
			GetClientRect(hwndDlg, &rect);
			Utils_RestoreWindowPosition(hwndDlg, NULL, jabberProtoName, "GcLog");

			hFont = NULL;
			PostMessage(hwndDlg, WM_JABBER_SET_FONT, 0, 0);

			gcInit = (JABBER_GCLOG_INITDIALOG *) lParam;

			if (gcInit!=NULL && (item=JabberListGetItemPtr(LIST_CHATROOM, gcInit->jid))!=NULL) {
				if ((localJid=JabberTextDecode(gcInit->jid)) != NULL) {
					SetWindowText(hwndDlg, localJid);
					free(localJid);
				}
				item->hwndGcDlg = hwndDlg;
				gcLogInfo->roomJid = strdup(item->jid);
				JabberLog("roomJid is %s", gcLogInfo->roomJid);
				SetEvent(gcInit->hEvent);
			}
			else {
				EnableWindow(GetDlgItem(hwndDlg, IDC_LOG), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_LIST), FALSE);
			}
			WindowList_Add(hWndListGcLog, hwndDlg, NULL);

			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS);
			SendDlgItemMessage(hwndDlg, IDC_LOG, EM_EXLIMITTEXT, 0, -1);
		}
		return TRUE;
	case WM_SIZE:
		if (wParam==SIZE_RESTORED || wParam==SIZE_MAXIMIZED) {
			int dlgWidth, dlgHeight;
			HDWP hdwp;

			dlgWidth = LOWORD(lParam);
			dlgHeight = HIWORD(lParam);
			JabberLog("size set to %d,%d", dlgWidth, dlgHeight);
			hdwp = BeginDeferWindowPos(6);
			hdwp = DeferWindowPos(hdwp, GetDlgItem(hwndDlg, IDC_TOPIC), 0, 70, 8, dlgWidth-10-70, 20, SWP_NOZORDER);
			hdwp = DeferWindowPos(hdwp, GetDlgItem(hwndDlg, IDC_VSPLIT), 0, dlgWidth-(gcLogInfo->vSplitterPos)-5, 30, 10, dlgHeight-(gcLogInfo->hSplitterPos)-35, SWP_NOZORDER);
			hdwp = DeferWindowPos(hdwp, GetDlgItem(hwndDlg, IDC_HSPLIT), 0, 10, dlgHeight-(gcLogInfo->hSplitterPos)-5, dlgWidth-20, 10, SWP_NOZORDER);
			hdwp = DeferWindowPos(hdwp, GetDlgItem(hwndDlg, IDC_LOG), 0, 10, 30, dlgWidth-(gcLogInfo->vSplitterPos)-15, dlgHeight-(gcLogInfo->hSplitterPos)-35, SWP_NOZORDER);
			hdwp = DeferWindowPos(hdwp, GetDlgItem(hwndDlg, IDC_LIST), 0, dlgWidth-(gcLogInfo->vSplitterPos)+5, 30, (gcLogInfo->vSplitterPos)-15, dlgHeight-(gcLogInfo->hSplitterPos)-35, SWP_NOZORDER);
			hdwp = DeferWindowPos(hdwp, GetDlgItem(hwndDlg, IDC_EDIT), 0, 10, dlgHeight-(gcLogInfo->hSplitterPos)+5, dlgWidth-20, (gcLogInfo->hSplitterPos)-15, SWP_NOZORDER);
			EndDeferWindowPos(hdwp);
		}
		break;
	case WM_CLOSE:
		if (jabberOnline) {
			JABBER_LIST_ITEM *item;

			if ((item=JabberListGetItemPtr(LIST_CHATROOM, gcLogInfo->roomJid))!=NULL && item->hwndGcDlg==hwndDlg) {
				JabberSend(jabberThreadInfo->s, "<presence to='%s' type='unavailable'/>", gcLogInfo->roomJid);
				JabberListRemove(LIST_CHATROOM, gcLogInfo->roomJid);
			}
		}
		WindowList_Remove(hWndListGcLog, hwndDlg);
		EndDialog(hwndDlg, 0);
		break;
	case WM_JABBER_SHUTDOWN:
		EndDialog(hwndDlg, 0);
		break;
	case WM_DESTROY:
		SetWindowLong(GetDlgItem(hwndDlg, IDC_EDIT), GWL_WNDPROC, (LONG) gcLogInfo->oldEditWndProc);
		SetWindowLong(GetDlgItem(hwndDlg, IDC_HSPLIT), GWL_WNDPROC, (LONG) gcLogInfo->oldHSplitterWndProc);
		SetWindowLong(GetDlgItem(hwndDlg, IDC_VSPLIT), GWL_WNDPROC, (LONG) gcLogInfo->oldVSplitterWndProc);
		DBWriteContactSettingDword(NULL, jabberProtoName, "GcLogHSplit", gcLogInfo->hSplitterPos);
		DBWriteContactSettingDword(NULL, jabberProtoName, "GcLogVSplit", gcLogInfo->vSplitterPos);
		Utils_SaveWindowPosition(hwndDlg, NULL, jabberProtoName, "GcLog");
		if (gcLogInfo->roomJid != NULL) free(gcLogInfo->roomJid);
		free(gcLogInfo);
		if (hFont != NULL) DeleteObject(hFont);
		break;
	case WM_JABBER_SET_FONT:
		{
			LOGFONT lf = {0};
			HFONT hFontNew;

			JabberGcLogLoadFont(gcLogFont);
			if (gcLogFont[0].face[0] != '\0') {
				strncpy(lf.lfFaceName, gcLogFont[0].face, sizeof(lf.lfFaceName));
				lf.lfHeight = -MulDiv(gcLogFont[0].size, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
				lf.lfCharSet = gcLogFont[0].charset;
				lf.lfWeight = (gcLogFont[0].style & JABBER_FONT_BOLD) ? FW_BOLD : FW_NORMAL;
				lf.lfItalic = (gcLogFont[0].style & JABBER_FONT_ITALIC) ? TRUE : FALSE;
				hFontNew = CreateFontIndirect(&lf);
				//SendDlgItemMessage(hwndDlg, IDC_LOG, WM_SETFONT, (WPARAM) hFontNew, TRUE);
				SendDlgItemMessage(hwndDlg, IDC_EDIT, WM_SETFONT, (WPARAM) hFontNew, TRUE);
				if (hFont != NULL) {
					DeleteObject(hFont);
					hFont = hFontNew;
				}
			}
		}
		break;
	case WM_JABBER_GC_MEMBER_ADD:
		if (lParam != (LPARAM) NULL)
			SendDlgItemMessage(hwndDlg, IDC_LIST, LB_ADDSTRING, 0, lParam);
		break;
	case WM_JABBER_SMILEY:
		if (ServiceExists(MS_SMILEYADD_REPLACESMILEYS)) {
			SMADD_RICHEDIT2 smre;

			smre.cbSize = sizeof(SMADD_RICHEDIT2);
			smre.hwndRichEditControl = GetDlgItem(hwndDlg, IDC_LOG);
			smre.Protocolname = jabberProtoName;
			smre.rangeToReplace = NULL;
			smre.useSounds = FALSE;
			smre.disableRedraw = FALSE;
			// Must be called from the gclog thread and must call
			// OleInitialize() at the beginning of the thread (in GcLogCreatThread)
			CallService(MS_SMILEYADD_REPLACESMILEYS, 0, (LPARAM) &smre);
		}
		break;
	case WM_JABBER_FLASHWND:
		if ((GetActiveWindow()!=hwndDlg || GetForegroundWindow()!=hwndDlg)
			&& DBGetContactSettingByte(NULL, jabberProtoName, "GcLogFlash", TRUE)) {

			SetTimer(hwndDlg, TIMERID_FLASHWND, TIMEOUT_FLASHWND, NULL);
		}
		break;
	case WM_TIMER:
		if (wParam == TIMERID_FLASHWND) {
			FlashWindow(hwndDlg, TRUE);
			gcLogInfo->nFlash++;
			if (gcLogInfo->nFlash > 5) {
				KillTimer(hwndDlg, TIMERID_FLASHWND);
				FlashWindow(hwndDlg, FALSE);
				gcLogInfo->nFlash = 0;
			} 
		}
		break;
	case WM_ACTIVATE:
		if (LOWORD(wParam)==WA_ACTIVE || LOWORD(wParam)==WA_CLICKACTIVE) {
			if (KillTimer(hwndDlg, TIMERID_FLASHWND))
				FlashWindow(hwndDlg, FALSE);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				char text[2048];
				char *str;

				GetDlgItemText(hwndDlg, IDC_EDIT, text, sizeof(text));
				SetDlgItemText(hwndDlg, IDC_EDIT, "");
				if (jabberOnline) {
					if ((str=JabberTextEncode(text)) != NULL) {
						JabberSend(jabberThreadInfo->s, "<message to='%s' type='groupchat'><body>%s</body></message>", gcLogInfo->roomJid, str);
						free(str);
					}
				}
			}
			break;
		case IDC_SET:
			if (gcLogInfo->hwndSetTopic!=NULL && IsWindow(gcLogInfo->hwndSetTopic))
				SetForegroundWindow(gcLogInfo->hwndSetTopic);
			else {
				JABBER_GCLOG_INPUT_INFO *gcLogInputInfo;

				gcLogInputInfo = (JABBER_GCLOG_INPUT_INFO *) malloc(sizeof(JABBER_GCLOG_INPUT_INFO));
				gcLogInputInfo->type = MUC_SETTOPIC;
				gcLogInputInfo->gcLogInfo = gcLogInfo;
				gcLogInputInfo->nick = NULL;
				gcLogInfo->hwndSetTopic = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_GROUPCHAT_INPUT), hwndDlg, JabberGcLogInputDlgProc, (LPARAM) gcLogInputInfo);
			}
			break;
		}
		break;
	case WM_MEASUREITEM:
		if (wParam == IDC_LIST) {
			MEASUREITEMSTRUCT *lpMis = (MEASUREITEMSTRUCT *) lParam;

			lpMis->itemHeight = GetSystemMetrics(SM_CYSMICON);
			return TRUE;
		}
		break;
	case WM_DRAWITEM:
		if (wParam == IDC_LIST) {

			DRAWITEMSTRUCT *lpDis = (DRAWITEMSTRUCT *) lParam;
			JABBER_LIST_ITEM *item;
			char text[256], *localNick;
			int i;

			if (lpDis->itemID == -1)
				break;

			SendMessage(lpDis->hwndItem, LB_GETTEXT, lpDis->itemID, (LPARAM) text);
			if ((item=JabberListGetItemPtr(LIST_CHATROOM, gcLogInfo->roomJid)) != NULL) {
				for (i=0; i<item->resourceCount && strcmp(text, item->resource[i].resourceName); i++);
				if (i < item->resourceCount) {
					switch (lpDis->itemAction) {
					case ODA_SELECT:
					case ODA_DRAWENTIRE:
						if (lpDis->itemState & ODS_SELECTED) {
							FillRect(lpDis->hDC, &(lpDis->rcItem), (HBRUSH) (COLOR_HIGHLIGHT+1));
							SetTextColor(lpDis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
							SetBkMode(lpDis->hDC, TRANSPARENT);
						}
						else {
							FillRect(lpDis->hDC, &(lpDis->rcItem), (HBRUSH) (COLOR_WINDOW+1));
							SetTextColor(lpDis->hDC, GetSysColor(COLOR_WINDOWTEXT));
						}
						DrawIconEx(lpDis->hDC, lpDis->rcItem.left, lpDis->rcItem.top, LoadSkinnedProtoIcon(jabberProtoName, item->resource[i].status), GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0, NULL, DI_NORMAL);
						if (item->resource[i].affiliation == AFFILIATION_OWNER)
							DrawIconEx(lpDis->hDC, lpDis->rcItem.left+GetSystemMetrics(SM_CXSMICON), lpDis->rcItem.top, jabberIcon[JABBER_IDI_GCOWNER], GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0, NULL, DI_NORMAL);
						else if (item->resource[i].affiliation == AFFILIATION_ADMIN)
							DrawIconEx(lpDis->hDC, lpDis->rcItem.left+GetSystemMetrics(SM_CXSMICON), lpDis->rcItem.top, jabberIcon[JABBER_IDI_GCADMIN], GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0, NULL, DI_NORMAL);
						if (item->resource[i].role == ROLE_MODERATOR)
							DrawIconEx(lpDis->hDC, lpDis->rcItem.left+GetSystemMetrics(SM_CXSMICON), lpDis->rcItem.top, jabberIcon[JABBER_IDI_GCMODERATOR], GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0, NULL, DI_NORMAL);
						else if (item->resource[i].role == ROLE_PARTICIPANT)
							DrawIconEx(lpDis->hDC, lpDis->rcItem.left+GetSystemMetrics(SM_CXSMICON), lpDis->rcItem.top, jabberIcon[JABBER_IDI_GCVOICE], GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0, NULL, DI_NORMAL);
						localNick = JabberTextDecode(text);
						TextOut(lpDis->hDC, lpDis->rcItem.left+GetSystemMetrics(SM_CXSMICON)+12, lpDis->rcItem.top, localNick, strlen(localNick)); 
						free(localNick);
						break;
					}
				}
			}
		}
		break;
	case WM_JABBER_CHECK_ONLINE:
		if (!jabberOnline) {
			EnableWindow(GetDlgItem(hwndDlg, IDC_SET), FALSE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), FALSE);
			SendDlgItemMessage(hwndDlg, IDC_LIST, LB_RESETCONTENT, 0, 0);
		}
		break;
	case WM_JABBER_GC_FORCE_QUIT:
		// wParam=301(banned) or 307(kicked)
		// lParam=<item><actor/><reason/></item> if not NULL
		JabberListRemove(LIST_CHATROOM, gcLogInfo->roomJid);
		EnableWindow(GetDlgItem(hwndDlg, IDC_SET), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), FALSE);
		SendDlgItemMessage(hwndDlg, IDC_LIST, LB_RESETCONTENT, 0, 0);
		if (lParam != (LPARAM)(NULL))
			JabberXmlFreeNode((XmlNode *) lParam);
		break;
	case WM_CONTEXTMENU:
		if ((HWND)wParam == GetDlgItem(hwndDlg, IDC_LIST)) {
			JABBER_LIST_ITEM *item;
			int xPos, yPos;
			RECT rect;
			int height, count, topIndex, i, j;

			xPos = (short) LOWORD(lParam);
			yPos = (short) HIWORD(lParam);
			height = (int) SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETITEMHEIGHT, 0, 0);
			count = (int) SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETCOUNT, 0, 0);
			topIndex = (int) SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETTOPINDEX, 0, 0);
			GetWindowRect(GetDlgItem(hwndDlg, IDC_LIST), &rect);
			i = ((yPos - rect.top) / height) + topIndex;
			if (i >= 0 && i < count && (item=JabberListGetItemPtr(LIST_CHATROOM, gcLogInfo->roomJid))!=NULL) {
				HMENU hMenu, hSubMenu;
				HANDLE hContact;
				char nick[256], jid[256];
				char *localNick;
				JABBER_GCLOG_INPUT_INFO *gcLogInputInfo;

				SendDlgItemMessage(hwndDlg, IDC_LIST, LB_SETCURSEL, i, 0);
				SendDlgItemMessage(hwndDlg, IDC_LIST, LB_GETTEXT, i, (LPARAM) nick);
				JabberLog("Selected nick = %s", nick);

				// i is my nick
				for (i=0; i<item->resourceCount && strcmp(item->resource[i].resourceName, item->nick); i++);
				// j is clicked nick
				for (j=0; j<item->resourceCount && strcmp(item->resource[j].resourceName, nick); j++);

				if (i<item->resourceCount && j<item->resourceCount) {
					hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT));
					hSubMenu = GetSubMenu(hMenu, 0);
					if (!strcmp(item->nick, nick)) {	// nick is already in UTF-8
						EnableMenuItem(hSubMenu, IDM_MESSAGE, MF_BYCOMMAND | MF_GRAYED);
					}
					else {
						if (item->resource[i].role == ROLE_MODERATOR) {
							if (item->resource[j].affiliation!=AFFILIATION_ADMIN && item->resource[j].affiliation!=AFFILIATION_OWNER) {
								EnableMenuItem(hSubMenu, IDM_VOICE, MF_BYCOMMAND | MF_ENABLED);
								EnableMenuItem(hSubMenu, IDM_KICK, MF_BYCOMMAND | MF_ENABLED);
							}
						}

						if (item->resource[i].affiliation == AFFILIATION_ADMIN) {
							if (item->resource[j].affiliation!=AFFILIATION_ADMIN && item->resource[j].affiliation!=AFFILIATION_OWNER) {
								EnableMenuItem(hSubMenu, IDM_BAN, MF_BYCOMMAND | MF_ENABLED);
								EnableMenuItem(hSubMenu, IDM_MODERATOR, MF_BYCOMMAND | MF_ENABLED);
							}
						}
						else if (item->resource[i].affiliation == AFFILIATION_OWNER) {
							EnableMenuItem(hSubMenu, IDM_BAN, MF_BYCOMMAND | MF_ENABLED);
							EnableMenuItem(hSubMenu, IDM_MODERATOR, MF_BYCOMMAND | MF_ENABLED);
							EnableMenuItem(hSubMenu, IDM_ADMIN, MF_BYCOMMAND | MF_ENABLED);
							EnableMenuItem(hSubMenu, IDM_OWNER, MF_BYCOMMAND | MF_ENABLED);
						}
					}

					CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) hSubMenu, 0);
					switch (TrackPopupMenu(hSubMenu, TPM_RETURNCMD|TPM_RIGHTBUTTON, xPos, yPos, 0, hwndDlg, NULL)) {
					case IDM_MESSAGE:
						localNick = JabberTextDecode(nick);
						_snprintf(jid, sizeof(jid), "%s/%s", gcLogInfo->roomJid, nick);
						hContact = JabberDBCreateContact(jid, localNick, TRUE, FALSE);
						//CallService(MS_MSG_SENDMESSAGE, (WPARAM) hContact, (LPARAM) NULL);
						QueueUserAPC(JabberGcLogSendMessageApcProc, hMainThread, (DWORD) hContact);
						free(localNick);
						break;
					case IDM_KICK:
						if (gcLogInfo->hwndKickReason!=NULL && IsWindow(gcLogInfo->hwndKickReason))
							SetForegroundWindow(gcLogInfo->hwndKickReason);
						else {
							gcLogInputInfo = (JABBER_GCLOG_INPUT_INFO *) malloc(sizeof(JABBER_GCLOG_INPUT_INFO));
							gcLogInputInfo->type = MUC_KICKREASON;
							gcLogInputInfo->gcLogInfo = gcLogInfo;
							gcLogInputInfo->nick = _strdup(nick);
							gcLogInfo->hwndKickReason = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_GROUPCHAT_INPUT), hwndDlg, JabberGcLogInputDlgProc, (LPARAM) gcLogInputInfo);
						}
						break;
					case IDM_BAN:
						if (gcLogInfo->hwndBanReason!=NULL && IsWindow(gcLogInfo->hwndBanReason))
							SetForegroundWindow(gcLogInfo->hwndBanReason);
						else {
							gcLogInputInfo = (JABBER_GCLOG_INPUT_INFO *) malloc(sizeof(JABBER_GCLOG_INPUT_INFO));
							gcLogInputInfo->type = MUC_BANREASON;
							gcLogInputInfo->gcLogInfo = gcLogInfo;
							gcLogInputInfo->nick = _strdup(nick);
							gcLogInfo->hwndBanReason = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_GROUPCHAT_INPUT), hwndDlg, JabberGcLogInputDlgProc, (LPARAM) gcLogInputInfo);
						}
						break;
					case IDM_VOICE:
						JabberSend(jabberThreadInfo->s, "<iq type='set' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item nick='%s' role='%s'/></query></iq>", gcLogInfo->roomJid, nick, (item->resource[j].role==ROLE_PARTICIPANT)?"visitor":"participant");
						break;
					case IDM_MODERATOR:
						JabberSend(jabberThreadInfo->s, "<iq type='set' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item nick='%s' role='%s'/></query></iq>", gcLogInfo->roomJid, nick, (item->resource[j].affiliation==ROLE_MODERATOR)?"participant":"moderator");
						break;
					case IDM_ADMIN:
						JabberSend(jabberThreadInfo->s, "<iq type='set' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item nick='%s' affiliation='%s'/></query></iq>", gcLogInfo->roomJid, nick, (item->resource[j].affiliation==AFFILIATION_ADMIN)?"member":"admin");
						break;
					case IDM_OWNER:
						JabberSend(jabberThreadInfo->s, "<iq type='set' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item nick='%s' affiliation='%s'/></query></iq>", gcLogInfo->roomJid, nick, (item->resource[j].affiliation==AFFILIATION_OWNER)?"admin":"owner");
						break;
			        }
					DestroyMenu(hMenu);
				}
            }
		}
		break;
	case WM_NOTIFY:
		switch (((NMHDR *) lParam)->idFrom) {
		case IDC_LOG:
			switch (((NMHDR *) lParam)->code) {
			case EN_MSGFILTER:
				switch (((MSGFILTER *) lParam)->msg) {
				case WM_RBUTTONUP:
					{
						JABBER_LIST_ITEM *item;

						if ((item=JabberListGetItemPtr(LIST_CHATROOM, gcLogInfo->roomJid)) != NULL) {
							HMENU hMenu, hSubMenu;
							POINT pt;
							int iqId, i;
							JABBER_GCLOG_INPUT_INFO *gcLogInputInfo;

							hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT));
							hSubMenu = GetSubMenu(hMenu, 1);
							for (i=0; i<item->resourceCount && strcmp(item->resource[i].resourceName, item->nick); i++);
							if (i < item->resourceCount) {
								JabberLog("%s : affiliation=%d role=%d", item->nick, item->resource[i].affiliation, item->resource[i].role);
								if (item->resource[i].role == ROLE_MODERATOR) {
									EnableMenuItem(hSubMenu, IDM_VOICE, MF_BYCOMMAND | MF_ENABLED);
								}
								if (item->resource[i].affiliation == AFFILIATION_ADMIN) {
									EnableMenuItem(hSubMenu, IDM_BAN, MF_BYCOMMAND | MF_ENABLED);
									EnableMenuItem(hSubMenu, IDM_MEMBER, MF_BYCOMMAND | MF_ENABLED);
								}
								else if (item->resource[i].affiliation == AFFILIATION_OWNER) {
									EnableMenuItem(hSubMenu, IDM_BAN, MF_BYCOMMAND | MF_ENABLED);
									EnableMenuItem(hSubMenu, IDM_MEMBER, MF_BYCOMMAND | MF_ENABLED);
									EnableMenuItem(hSubMenu, IDM_MODERATOR, MF_BYCOMMAND | MF_ENABLED);
									EnableMenuItem(hSubMenu, IDM_ADMIN, MF_BYCOMMAND | MF_ENABLED);
									EnableMenuItem(hSubMenu, IDM_OWNER, MF_BYCOMMAND | MF_ENABLED);
									EnableMenuItem(hSubMenu, IDM_CONFIG, MF_BYCOMMAND | MF_ENABLED);
									EnableMenuItem(hSubMenu, IDM_DESTROY, MF_BYCOMMAND | MF_ENABLED);
								}
								CallService(MS_LANGPACK_TRANSLATEMENU, (WPARAM) hSubMenu, 0);
								pt.x = (short) LOWORD(((ENLINK *) lParam)->lParam);
								pt.y = (short) HIWORD(((ENLINK *) lParam)->lParam);
								ClientToScreen(((NMHDR *) lParam)->hwndFrom, &pt);
								switch (TrackPopupMenu(hSubMenu, TPM_RETURNCMD|TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwndDlg, NULL)) {
								case IDM_CLEAR:
									SetDlgItemText(hwndDlg, IDC_LOG, "");
									break;
								case IDM_VOICE:
									iqId = JabberSerialNext();
									JabberIqAdd(iqId, IQ_PROC_NONE, JabberIqResultMucGetVoiceList);
									JabberSend(jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item role='participant'/></query></iq>", iqId, gcLogInfo->roomJid);
									break;
								case IDM_MEMBER:
									iqId = JabberSerialNext();
									JabberIqAdd(iqId, IQ_PROC_NONE, JabberIqResultMucGetMemberList);
									JabberSend(jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item affiliation='member'/></query></iq>", iqId, gcLogInfo->roomJid);
									break;
								case IDM_MODERATOR:
									iqId = JabberSerialNext();
									JabberIqAdd(iqId, IQ_PROC_NONE, JabberIqResultMucGetModeratorList);
									JabberSend(jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item role='moderator'/></query></iq>", iqId, gcLogInfo->roomJid);
									break;
								case IDM_BAN:
									iqId = JabberSerialNext();
									JabberIqAdd(iqId, IQ_PROC_NONE, JabberIqResultMucGetBanList);
									JabberSend(jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='http://jabber.org/protocol/muc#admin'><item affiliation='outcast'/></query></iq>", iqId, gcLogInfo->roomJid);
									break;
								case IDM_ADMIN:
									iqId = JabberSerialNext();
									JabberIqAdd(iqId, IQ_PROC_NONE, JabberIqResultMucGetAdminList);
									JabberSend(jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='http://jabber.org/protocol/muc#owner'><item affiliation='admin'/></query></iq>", iqId, gcLogInfo->roomJid);
									break;
								case IDM_OWNER:
									iqId = JabberSerialNext();
									JabberIqAdd(iqId, IQ_PROC_NONE, JabberIqResultMucGetOwnerList);
									JabberSend(jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='http://jabber.org/protocol/muc#owner'><item affiliation='owner'/></query></iq>", iqId, gcLogInfo->roomJid);
									break;
								case IDM_NICK:
									if (gcLogInfo->hwndChangeNick!=NULL && IsWindow(gcLogInfo->hwndChangeNick))
										SetForegroundWindow(gcLogInfo->hwndChangeNick);
									else {
										gcLogInputInfo = (JABBER_GCLOG_INPUT_INFO *) malloc(sizeof(JABBER_GCLOG_INPUT_INFO));
										gcLogInputInfo->type = MUC_CHANGENICK;
										gcLogInputInfo->gcLogInfo = gcLogInfo;
										gcLogInputInfo->nick = _strdup(item->nick);
										gcLogInfo->hwndChangeNick = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_GROUPCHAT_INPUT), hwndDlg, JabberGcLogInputDlgProc, (LPARAM) gcLogInputInfo);
									}
									break;
								case IDM_INVITE:
									CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_GROUPCHAT_INVITE), hwndDlg, JabberGcLogInviteDlgProc, (LPARAM) gcLogInfo->roomJid);
									break;
								case IDM_CONFIG:
									iqId = JabberSerialNext();
									JabberIqAdd(iqId, IQ_PROC_NONE, JabberIqResultGetMuc);
									JabberSend(jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='http://jabber.org/protocol/muc#owner'/></iq>", iqId, gcLogInfo->roomJid);
									break;
								case IDM_DESTROY:
									if (gcLogInfo->hwndDestroyReason!=NULL && IsWindow(gcLogInfo->hwndDestroyReason))
										SetForegroundWindow(gcLogInfo->hwndDestroyReason);
									else {
										gcLogInputInfo = (JABBER_GCLOG_INPUT_INFO *) malloc(sizeof(JABBER_GCLOG_INPUT_INFO));
										gcLogInputInfo->type = MUC_DESTROYREASON;
										gcLogInputInfo->gcLogInfo = gcLogInfo;
										gcLogInputInfo->nick = NULL;
										gcLogInfo->hwndDestroyReason = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_GROUPCHAT_INPUT), hwndDlg, JabberGcLogInputDlgProc, (LPARAM) gcLogInputInfo);
									}
									break;
								}
							}
							DestroyMenu(hMenu);
						}
					}
					break;
				}
				break;
			}
			break;
		}
		break;
	}
	
	return FALSE;
}

static void __cdecl JabberGcLogCreateThread(JABBER_GCLOG_INITDIALOG *gcInit)
{
	JABBER_LIST_ITEM *item;

	if ((item=JabberListGetItemPtr(LIST_CHATROOM, gcInit->jid)) != NULL) {
		OleInitialize(NULL);
		DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_GROUPCHAT_LOG), NULL, JabberGcLogDlgProc, (LPARAM) gcInit);
		OleUninitialize();
	}
}

void JabberGcLogCreate(char *jid)
{
	JABBER_LIST_ITEM *item;
	JABBER_GCLOG_INITDIALOG gcInit;
	char *roomJid, *p;

	if ((item=JabberListGetItemPtr(LIST_CHATROOM, jid)) != NULL) {
		if (item->hwndGcDlg == NULL) {
			roomJid = strdup(jid);
			if ((p=strchr(roomJid, '/')) != NULL)
				*p = '\0';
			// wait for event to make sure item->hwndGcDlg is set
			// (it will be set in WM_INITDIALOG)
			gcInit.jid = roomJid;
			gcInit.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
			JabberForkThread(( JABBER_THREAD_FUNC )JabberGcLogCreateThread, 0, (void *) &gcInit);
			WaitForSingleObject(gcInit.hEvent, INFINITE);
			free(roomJid);
		}
		else {
			SetFocus(item->hwndGcDlg);
		}
	}
}

void JabberGcLogUpdateMemberStatus(char *jid)
{
	char *nick;
	JABBER_LIST_ITEM *item;
	RECT rect;
	int nItem, i;

	if ((nick=strchr(jid, '/'))!=NULL && nick[1]!='\0') {
		nick++;
		if ((item=JabberListGetItemPtr(LIST_CHATROOM, jid)) != NULL) {
			if ((nItem=SendDlgItemMessage(item->hwndGcDlg, IDC_LIST, LB_FINDSTRINGEXACT, -1, (LPARAM) nick)) == LB_ERR) {
				nItem = SendDlgItemMessage(item->hwndGcDlg, IDC_LIST, LB_ADDSTRING, 0, (LPARAM) nick);
			}
			for (i=0; i<item->resourceCount && strcmp(nick, item->resource[i].resourceName); i++);
			if (i < item->resourceCount) {
				SendDlgItemMessage(item->hwndGcDlg, IDC_LIST, LB_GETITEMRECT, nItem, (LPARAM) &rect);
				InvalidateRect(GetDlgItem(item->hwndGcDlg, IDC_LIST), &rect, TRUE);
			}
			else {
				SendDlgItemMessage(item->hwndGcDlg, IDC_LIST, LB_DELETESTRING, nItem, 0);
			}
		}
	}
}

static VOID CALLBACK JabberGcLogSmileyApcProc(DWORD param)
{
	SMADD_RICHEDIT smre;

	//sel.cpMax = GetWindowTextLength(hwndLog);
	smre.cbSize = sizeof(SMADD_RICHEDIT);
	smre.hwndRichEditControl = (HWND) param;
	smre.Protocolname = jabberProtoName;
	smre.rangeToReplace = NULL; //&sel;
	CallService(MS_SMILEYADD_REPLACESMILEYS, 0, (LPARAM) &smre);
}

void JabberGcLogAppend(char *jid, time_t timestamp, char *str)
{
	JABBER_LIST_ITEM *item;
	HWND hwndLog;
	CHARRANGE sel;
	SETTEXTEX stt;
	char *nick;
	char *msg, *escapedStr;
	int nMin, nMax;
	int msgSize, i;
	struct tm *ltime;
	char c;

	if (str != NULL) {
		if ((item=JabberListGetItemPtr(LIST_CHATROOM, jid)) != NULL) {

			if ((escapedStr=JabberRtfEscape(str)) != NULL) {

				JabberUrlDecode(escapedStr);

				msg = NULL;
				JabberStringAppend(&msg, &msgSize, "{\\rtf{\\fonttbl");
				for (i=0; i<JABBER_GCLOG_NUM_FONT; i++) {
					if (gcLogFont[i].face[0])
						JabberStringAppend(&msg, &msgSize, "{\\f%d\\fnil %s;}", i, gcLogFont[i].face);
				}
				JabberStringAppend(&msg, &msgSize, "}{\\colortbl");
				for (i=0; i<JABBER_GCLOG_NUM_FONT; i++)
					JabberStringAppend(&msg, &msgSize, "\\red%d\\green%d\\blue%d;", GetRValue(gcLogFont[i].color), GetGValue(gcLogFont[i].color), GetBValue(gcLogFont[i].color));
				JabberStringAppend(&msg, &msgSize, "}");

				if ((nick=strchr(jid, '/'))!=NULL && nick[1]!='\0') {
					nick++;
					if (strlen(escapedStr)>=4 && !strncmp(escapedStr, "/me ", 4)) {
						if (gcLogFont[5].face[0])
							JabberStringAppend(&msg, &msgSize, "\\f5");
						JabberStringAppend(&msg, &msgSize, "\\cf5\\fs%d\\b%d\\i%d * %s %s\\par}",
							2*gcLogFont[5].size,
							(gcLogFont[5].style & JABBER_FONT_BOLD)?1:0,
							(gcLogFont[5].style & JABBER_FONT_ITALIC)?1:0,
							nick, escapedStr+4);
					}
					else {
						if (DBGetContactSettingByte(NULL, jabberProtoName, "GcLogTime", TRUE)==TRUE && (ltime=localtime(&timestamp))!=NULL) {
							if (gcLogFont[2].face[0])
								JabberStringAppend(&msg, &msgSize, "\\f2");
							JabberStringAppend(&msg, &msgSize, "\\cf2\\fs%d\\b%d\\i%d ",
								2*gcLogFont[2].size,
								(gcLogFont[2].style & JABBER_FONT_BOLD)?1:0,
								(gcLogFont[2].style & JABBER_FONT_ITALIC)?1:0);
							if (DBGetContactSettingByte(NULL, jabberProtoName, "GcLogDate", FALSE) == TRUE) {
								JabberStringAppend(&msg, &msgSize, "%d/%d/%02d ", ltime->tm_mon+1, ltime->tm_mday, ltime->tm_year%100);
							}
							if (ltime->tm_hour >= 12) {
								ltime->tm_hour -= 12;
								c = 'P';
							}
							else
								c = 'A';
							if (ltime->tm_hour == 0)
								ltime->tm_hour = 12;
							JabberStringAppend(&msg, &msgSize, "%02d:%02d %cM ", ltime->tm_hour, ltime->tm_min, c);
						}
						if (gcLogFont[3].face[0])
							JabberStringAppend(&msg, &msgSize, "\\f3");
						JabberStringAppend(&msg, &msgSize, "\\cf3\\fs%d\\b%d\\i%d <%s>",
							2*gcLogFont[3].size,
							(gcLogFont[3].style & JABBER_FONT_BOLD)?1:0,
							(gcLogFont[3].style & JABBER_FONT_ITALIC)?1:0,
							nick);
						if (gcLogFont[1].face[0])
							JabberStringAppend(&msg, &msgSize, "\\f1");
						JabberStringAppend(&msg, &msgSize, "\\cf1\\fs%d\\b%d\\i%d  %s\\par}",
							2*gcLogFont[1].size,
							(gcLogFont[1].style & JABBER_FONT_BOLD)?1:0,
							(gcLogFont[1].style & JABBER_FONT_ITALIC)?1:0,
							escapedStr);
					}
				}
				else {
					if (gcLogFont[4].face[0])
						JabberStringAppend(&msg, &msgSize, "\\f4");
					JabberStringAppend(&msg, &msgSize, "\\cf4\\fs%d\\b%d\\i%d %s\\par}",
						2*gcLogFont[4].size,
						(gcLogFont[4].style & JABBER_FONT_BOLD)?1:0,
						(gcLogFont[4].style & JABBER_FONT_ITALIC)?1:0,
						escapedStr);
				}

				hwndLog = GetDlgItem(item->hwndGcDlg, IDC_LOG);

				sel.cpMin = sel.cpMax = GetWindowTextLength(hwndLog);
				SendMessage(hwndLog, EM_EXSETSEL, 0, (LPARAM) &sel);
				stt.flags = ST_SELECTION;
				stt.codepage = CP_UTF8;
				SendMessage(hwndLog, EM_SETTEXTEX, (WPARAM) &stt, (LPARAM) msg);

				if (ServiceExists(MS_SMILEYADD_REPLACESMILEYS))
					PostMessage(item->hwndGcDlg, WM_JABBER_SMILEY, 0, 0);

				// scroll to bottom of the log
				GetScrollRange(hwndLog, SB_VERT, &nMin, &nMax);
				SetScrollPos(hwndLog, SB_VERT, nMax, TRUE);
	            PostMessage(hwndLog, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, nMax), (LPARAM) NULL);

				free(msg);
				free(escapedStr);

				SendMessage(item->hwndGcDlg, WM_JABBER_FLASHWND, 0, 0);
			}
		}
	}
}

void JabberGcLogSetTopic(char *jid, char *str)
{
	JABBER_LIST_ITEM *item;
	char *localTopic;

	if ((item=JabberListGetItemPtr(LIST_CHATROOM, jid))!=NULL && item->hwndGcDlg!=NULL) {
		if (str != NULL) {
			if ((localTopic=JabberTextDecode(str)) != NULL) {
				SetDlgItemText(item->hwndGcDlg, IDC_TOPIC, localTopic);
				free(localTopic);
			}
		}
		else {
			SetDlgItemText(item->hwndGcDlg, IDC_TOPIC, "");
		}
	}
}

void JabberGcLogModuleLoaded()
{
	JabberGcLogLoadFont(gcLogFont);
}

void JabberGcLogInit()
{
	LoadLibrary("riched20.dll");
	hCursorSizeNS = LoadCursor(NULL, IDC_SIZENS);
	hCursorSizeWE = LoadCursor(NULL, IDC_SIZEWE);
}

void JabberGcLogUninit()
{
	DestroyCursor(hCursorSizeNS);
	DestroyCursor(hCursorSizeWE);
	FreeLibrary(GetModuleHandle("riched20.dll"));
}
