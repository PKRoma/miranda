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
#include "resource.h"
#include "jabber_list.h"
#include "jabber_iq.h"

extern char *streamId;
extern char *jabberVcardPhotoFileName;
extern char *jabberVcardPhotoType;

void JabberIqResultGetAuth(XmlNode *iqNode, void *userdata)
{
	struct ThreadData *info = (struct ThreadData *) userdata;
	XmlNode *queryNode;
	char *type;
	char *str;
	char text[256];
	int iqId;

	// RECVED: result of the request for authentication method
	// ACTION: send account authentication information to log in
	JabberLog("<iq/> iqIdGetAuth");
	if ((type=JabberXmlGetAttrValue(iqNode, "type")) == NULL) return;
	if ((queryNode=JabberXmlGetChild(iqNode, "query")) == NULL) return;

	if (!strcmp(type, "result")) {
		if (JabberXmlGetChild(queryNode, "digest")!=NULL && streamId) {
			if ((str=JabberUtf8Encode(info->password)) != NULL) {
				wsprintf(text, "%s%s", streamId, str);
				free(str);
				if ((str=JabberSha1(text)) != NULL) {
					_snprintf(text, sizeof(text), "<digest>%s</digest>", str);
					free(str);
				}
			}
		}
		else if (JabberXmlGetChild(queryNode, "password") != NULL) {
			if ((str=JabberTextEncode(info->password)) != NULL) {
				_snprintf(text, sizeof(text), "<password>%s</password>", str);
				free(str);
			}
		}
		else {
			JabberLog("No known authentication mechanism accepted by the server.");
			JabberSend(info->s, "</stream:stream>");
			return;
		}
		if (JabberXmlGetChild(queryNode, "resource") != NULL) {
			if ((str=JabberTextEncode(info->resource)) != NULL) {
				_snprintf(text+strlen(text), sizeof(text)-strlen(text), "<resource>%s</resource>", str);
				free(str);
			}
		}
		if ((str=JabberTextEncode(info->username)) != NULL) {
			iqId = JabberSerialNext();
			JabberIqAdd(iqId, IQ_PROC_NONE, JabberIqResultSetAuth);
			JabberSend(info->s, "<iq type='set' id='"JABBER_IQID"%d'><query xmlns='jabber:iq:auth'><username>%s</username>%s</query></iq>", iqId, str, text);
			free(str);
		}
	}
	else if (!strcmp(type, "error")) {
		JabberSend(info->s, "</stream:stream>");
	}
}

void JabberIqResultSetAuth(XmlNode *iqNode, void *userdata)
{
	struct ThreadData *info = (struct ThreadData *) userdata;
	char *type;
	int iqId;

	// RECVED: authentication result
	// ACTION: if successfully logged in, continue by requesting roster list and set my initial status
	JabberLog("<iq/> iqIdSetAuth");
	if ((type=JabberXmlGetAttrValue(iqNode, "type")) == NULL) return;

	if (!strcmp(type, "result")) {
		DBVARIANT dbv;

		if (DBGetContactSetting(NULL, jabberProtoName, "Nick", &dbv))
			DBWriteContactSettingString(NULL, jabberProtoName, "Nick", info->username);
		else
			DBFreeVariant(&dbv);
		iqId = JabberSerialNext();
		JabberIqAdd(iqId, IQ_PROC_NONE, JabberIqResultGetRoster);
		JabberSend(info->s, "<iq type='get' id='"JABBER_IQID"%d'><query xmlns='jabber:iq:roster'/></iq>", iqId);
		if (hwndJabberAgents) {
			// Retrieve agent information
			iqId = JabberSerialNext();
			JabberIqAdd(iqId, IQ_PROC_GETAGENTS, JabberIqResultGetAgents);
			JabberSend(info->s, "<iq type='get' id='"JABBER_IQID"%d'><query xmlns='jabber:iq:agents'/></iq>", iqId);
		}
	}
	// What to do if password error? etc...
	else if (!strcmp(type, "error")) {
		char text[128];

		JabberSend(info->s, "</stream:stream>");
		_snprintf(text, sizeof(text), "%s %s@%s.", Translate("Authentication failed for"), info->username, info->server);
		MessageBox(NULL, text, Translate("Jabber Authentication"), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
		ProtoBroadcastAck(jabberProtoName, NULL, ACKTYPE_LOGIN, ACKRESULT_FAILED, NULL, LOGINERR_WRONGPASSWORD);
		jabberThreadInfo = NULL;	// To disallow auto reconnect
	}
}

void JabberIqResultGetRoster(XmlNode *iqNode, void *userdata)
{
	struct ThreadData *info = (struct ThreadData *) userdata;
	XmlNode *queryNode;
	char *type;
	char *str;

	// RECVED: roster information
	// ACTION: populate LIST_ROSTER and create contact for any new rosters
	JabberLog("<iq/> iqIdGetRoster");
	if ((type=JabberXmlGetAttrValue(iqNode, "type")) == NULL) return;
	if ((queryNode=JabberXmlGetChild(iqNode, "query")) == NULL) return;

	if (!strcmp(type, "result")) {
		str = JabberXmlGetAttrValue(queryNode, "xmlns");
		if (str!=NULL && !strcmp(str, "jabber:iq:roster")) {
			DBVARIANT dbv;
			XmlNode *itemNode, *groupNode;
			JABBER_SUBSCRIPTION sub;
			JABBER_LIST_ITEM *item;
			HANDLE hContact;
			char *jid, *name, *nick;
			int i, oldStatus;
			CLISTMENUITEM clmi;

			for (i=0; i<queryNode->numChild; i++) {
				itemNode = queryNode->child[i];
				if (!strcmp(itemNode->name, "item")) {
					str = JabberXmlGetAttrValue(itemNode, "subscription");
					if (str==NULL) sub = SUB_NONE;
					else if (!strcmp(str, "both")) sub = SUB_BOTH;
					else if (!strcmp(str, "to")) sub = SUB_TO;
					else if (!strcmp(str, "from")) sub = SUB_FROM;
					else sub = SUB_NONE;
					//if (str!=NULL && (!strcmp(str, "to") || !strcmp(str, "both"))) {
					if ((jid=JabberXmlGetAttrValue(itemNode, "jid")) != NULL) {
						if ((name=JabberXmlGetAttrValue(itemNode, "name")) != NULL)
							nick = JabberTextDecode(name);
						else {
							str = JabberNickFromJID(jid);
							nick = JabberTextDecode(str);
							free(str);
						}
						if (nick != NULL) {
							item = JabberListAdd(LIST_ROSTER, jid);
							if (item->nick) free(item->nick);
							item->nick = nick;
							item->subscription = sub;
							if ((hContact=JabberHContactFromJID(jid)) == NULL) {
								// Received roster has a new JID.
								// Add the jid (with empty resource) to Miranda contact list.
								hContact = JabberDBCreateContact(jid, nick, FALSE, TRUE);
							}
							DBWriteContactSettingString(hContact, "CList", "MyHandle", nick);
							if (item->group) free(item->group);
							if ((groupNode=JabberXmlGetChild(itemNode, "group"))!=NULL && groupNode->text!=NULL) {
								item->group = JabberTextDecode(groupNode->text);
								JabberContactListCreateGroup(item->group);
								// Don't set group again if already correct, or Miranda may show wrong group count in some case
								if (!DBGetContactSetting(hContact, "CList", "Group", &dbv)) {
									if (strcmp(dbv.pszVal, item->group))
										DBWriteContactSettingString(hContact, "CList", "Group", item->group);
									DBFreeVariant(&dbv);
								}
								else
									DBWriteContactSettingString(hContact, "CList", "Group", item->group);
							}
							else {
								item->group = NULL;
								DBDeleteContactSetting(hContact, "CList", "Group");
							}
						}
					}
				}
			}
			// Delete orphaned contacts (if roster sync is enabled)
			if (DBGetContactSettingByte(NULL, jabberProtoName, "RosterSync", FALSE) == TRUE) {
				HANDLE *list;
				int listSize, listAllocSize;

				listSize = listAllocSize = 0;
				list = NULL;
				hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
				while (hContact != NULL) {
					str = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) hContact, 0);
					if(str!=NULL && !strcmp(str, jabberProtoName)) {
						if (!DBGetContactSetting(hContact, jabberProtoName, "jid", &dbv)) {
							if (!JabberListExist(LIST_ROSTER, dbv.pszVal)) {
								JabberLog("Syncing roster: preparing to delete %s (hContact=0x%x)", dbv.pszVal, hContact);
								if (listSize >= listAllocSize) {
									listAllocSize = listSize + 100;
									if ((list=(HANDLE *) realloc(list, listAllocSize)) == NULL) {
										listSize = 0;
										break;
									}
								}
								list[listSize++] = hContact;
							}
							DBFreeVariant(&dbv);
						}
					}
					hContact = (HANDLE) CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM) hContact, 0);
				}
				for (i=0; i<listSize; i++) {
					JabberLog("Syncing roster: deleting 0x%x", list[i]);
					CallService(MS_DB_CONTACT_DELETE, (WPARAM) list[i], 0);
				}
				if (list != NULL)
					free(list);
			}
			///////////////////////////////////////
			jabberOnline = TRUE;
			memset(&clmi, 0, sizeof(CLISTMENUITEM));
			clmi.cbSize = sizeof(CLISTMENUITEM);
			clmi.flags = CMIM_FLAGS;
			CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hMenuAgent, (LPARAM) &clmi);
			CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hMenuChangePassword, (LPARAM) &clmi);
			CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM) hMenuGroupchat, (LPARAM) &clmi);
			if (hwndJabberGroupchat)
				SendMessage(hwndJabberGroupchat, WM_JABBER_CHECK_ONLINE, 0, 0);
			if (hwndJabberJoinGroupchat)
				SendMessage(hwndJabberJoinGroupchat, WM_JABBER_CHECK_ONLINE, 0, 0);
/*
			if (jabberDesiredStatus != ID_STATUS_INVISIBLE) {
				BOOL bScreenSaverRunning = FALSE;

				// postpone sending status change to SETAWAYMSG service if the SETAWAYMSG
				// dialog will show up for user to change away message
				JabberLog("modeMsgStatusChangePending = %d", modeMsgStatusChangePending);
				SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &bScreenSaverRunning, FALSE);
				if (!bScreenSaverRunning &&
					!modeMsgStatusChangePending &&
					!DBGetContactSettingByte(NULL, "SRAway", SRAwayStatusModeToDbSetting(jabberDesiredStatus, "Ignore"), 0) &&
					!DBGetContactSettingByte(NULL, "SRAway", SRAwayStatusModeToDbSetting(jabberDesiredStatus, "NoDlg"), 1)) {
					JabberLog("Postponing change status to SETAWAYMSG");
					return;
				}
			}
*/
			JabberLog("Status changed via THREADSTART");
			modeMsgStatusChangePending = FALSE;
			oldStatus = jabberStatus;
			switch (jabberDesiredStatus) {
			case ID_STATUS_ONLINE:
			case ID_STATUS_NA:
			case ID_STATUS_FREECHAT:
			case ID_STATUS_INVISIBLE:
				jabberStatus = jabberDesiredStatus;
				break;
			case ID_STATUS_AWAY:
			case ID_STATUS_ONTHEPHONE:
			case ID_STATUS_OUTTOLUNCH:
				jabberStatus = ID_STATUS_AWAY;
				break;
			case ID_STATUS_DND:
			case ID_STATUS_OCCUPIED:
				jabberStatus = ID_STATUS_DND;
				break;
			}
			JabberSendPresence(jabberStatus);
			ProtoBroadcastAck(jabberProtoName, NULL, ACKTYPE_STATUS, ACKRESULT_SUCCESS, (HANDLE) oldStatus, jabberStatus);
			//////////////////////////////////
			if (hwndJabberAgents)
				SendMessage(hwndJabberAgents, WM_JABBER_TRANSPORT_REFRESH, 0, 0);
			if (hwndJabberVcard)
				SendMessage(hwndJabberVcard, WM_JABBER_CHECK_ONLINE, 0, 0);
		}
	}
}

void JabberIqResultGetAgents(XmlNode *iqNode, void *userdata)
{
	struct ThreadData *info = (struct ThreadData *) userdata;
	XmlNode *queryNode;
	char *type, *jid;
	char *str;

	// RECVED: agent list
	// ACTION: refresh agent list dialog
	JabberLog("<iq/> iqIdGetAgents");
	if ((type=JabberXmlGetAttrValue(iqNode, "type")) == NULL) return;
	if ((queryNode=JabberXmlGetChild(iqNode, "query")) == NULL) return;

	if (!strcmp(type, "result")) {
		str = JabberXmlGetAttrValue(queryNode, "xmlns");
		if (str!=NULL && !strcmp(str, "jabber:iq:agents")) {
			XmlNode *agentNode, *n;
			JABBER_LIST_ITEM *item;
			int i;

			JabberListRemoveList(LIST_AGENT);
			for (i=0; i<queryNode->numChild; i++) {
				agentNode = queryNode->child[i];
				if (!strcmp(agentNode->name, "agent")) {
					if ((jid=JabberXmlGetAttrValue(agentNode, "jid")) != NULL) {
						item = JabberListAdd(LIST_AGENT, jid);
						if (JabberXmlGetChild(agentNode, "register") != NULL)
							item->cap |= AGENT_CAP_REGISTER;
						if (JabberXmlGetChild(agentNode, "search") != NULL)
							item->cap |= AGENT_CAP_SEARCH;
						if (JabberXmlGetChild(agentNode, "groupchat") != NULL)
							item->cap |= AGENT_CAP_GROUPCHAT;
						// set service also???
						// most chatroom servers don't announce <grouchat/> so we will
						// also treat <service>public</service> as groupchat services
						if ((n=JabberXmlGetChild(agentNode, "service"))!=NULL && n->text!=NULL && !strcmp(n->text, "public"))
							item->cap |= AGENT_CAP_GROUPCHAT;
						if ((n=JabberXmlGetChild(agentNode, "name"))!=NULL && n->text!=NULL)
							item->name = JabberTextDecode(n->text);
					}
				}
			}
		}
		if (hwndJabberAgents != NULL) {
			if ((jid=JabberXmlGetAttrValue(iqNode, "from")) != NULL)
				SendMessage(hwndJabberAgents, WM_JABBER_AGENT_REFRESH, 0, (LPARAM) jid);
			else
				SendMessage(hwndJabberAgents, WM_JABBER_AGENT_REFRESH, 0, (LPARAM) info->server);
		}
	}
}

void JabberIqResultGetRegister(XmlNode *iqNode, void *userdata)
{
	struct ThreadData *info = (struct ThreadData *) userdata;
	XmlNode *queryNode, *errorNode, *n;
	char *type;
	char *str;

	// RECVED: result of the request for (agent) registration mechanism
	// ACTION: activate (agent) registration input dialog
	JabberLog("<iq/> iqIdGetRegister");
	if ((type=JabberXmlGetAttrValue(iqNode, "type")) == NULL) return;
	if ((queryNode=JabberXmlGetChild(iqNode, "query")) == NULL) return;

	if (!strcmp(type, "result")) {
		if (hwndAgentRegInput) {
			if ((n=JabberXmlCopyNode(iqNode)) != NULL) {
				SendMessage(hwndAgentRegInput, WM_JABBER_REGINPUT_ACTIVATE, 1 /*success*/, (LPARAM) n);
			}
		}
	}
	else if (!strcmp(type, "error")) {
		if (hwndAgentRegInput) {
			errorNode = JabberXmlGetChild(iqNode, "error");
			str = JabberErrorMsg(errorNode);
			SendMessage(hwndAgentRegInput, WM_JABBER_REGINPUT_ACTIVATE, 0 /*error*/, (LPARAM) str);
			free(str);
		}
	}
}

void JabberIqResultSetRegister(XmlNode *iqNode, void *userdata)
{
	XmlNode *errorNode;
	char *type;
	char *str;

	// RECVED: result of registration process
	// ACTION: notify of successful agent registration
	JabberLog("<iq/> iqIdSetRegister");
	if ((type=JabberXmlGetAttrValue(iqNode, "type")) == NULL) return;

	if (!strcmp(type, "result")) {
		if (hwndRegProgress)
			SendMessage(hwndRegProgress, WM_JABBER_REGDLG_UPDATE, 100, (LPARAM) Translate("Registration successful"));
	}
	else if (!strcmp(type, "error")) {
		if (hwndRegProgress) {
			errorNode = JabberXmlGetChild(iqNode, "error");
			str = JabberErrorMsg(errorNode);
			SendMessage(hwndRegProgress, WM_JABBER_REGDLG_UPDATE, 100, (LPARAM) str);
			free(str);
		}
	}
}

void JabberIqResultGetVcard(XmlNode *iqNode, void *userdata)
{
	XmlNode *vCardNode, *m, *n, *o;
	char *type, *jid, *localJID;
	HANDLE hContact;
	char text[128];
	int len;
	DBVARIANT dbv;
	WORD nFlag;

	JabberLog("<iq/> iqIdGetVcard");
	if ((type=JabberXmlGetAttrValue(iqNode, "type")) == NULL) return;
	if ((jid=JabberXmlGetAttrValue(iqNode, "from")) == NULL) return;

	localJID = JabberTextDecode(jid);
	len = strlen(jabberJID);
	if (!strncmp(localJID, jabberJID, len) && (localJID[len]=='/' || localJID[len]=='\0')) {
		free(localJID);
		hContact = NULL;
		JabberLog("Vcard for myself");
	}
	else {
		free(localJID);
		hContact = JabberHContactFromJID(jid);
		if (hContact == NULL) return;
		JabberLog("Other user's vcard");
	}

	if (!strcmp(type, "result")) {
		BOOL hasFn, hasNick, hasGiven, hasFamily, hasMiddle, hasBday, hasGender;
		BOOL hasPhone, hasFax, hasCell, hasUrl;
		BOOL hasHome, hasHomeStreet, hasHomeStreet2, hasHomeLocality, hasHomeRegion, hasHomePcode, hasHomeCtry;
		BOOL hasWork, hasWorkStreet, hasWorkStreet2, hasWorkLocality, hasWorkRegion, hasWorkPcode, hasWorkCtry;
		BOOL hasOrgname, hasOrgunit, hasRole, hasTitle;
		BOOL hasDesc, hasPhoto;
		int nEmail, nPhone, nYear, nMonth, nDay;
		int i;
		char *nText, *mText, *oText;

		hasFn = hasNick = hasGiven = hasFamily = hasMiddle = hasBday = hasGender = FALSE;
		hasPhone = hasFax = hasCell = hasUrl = FALSE;
		hasHome = hasHomeStreet = hasHomeStreet2 = hasHomeLocality = hasHomeRegion = hasHomePcode = hasHomeCtry = FALSE;
		hasWork = hasWorkStreet = hasWorkStreet2 = hasWorkLocality = hasWorkRegion = hasWorkPcode = hasWorkCtry = FALSE;
		hasOrgname = hasOrgunit = hasRole = hasTitle = FALSE;
		hasDesc = hasPhoto = FALSE;
		nEmail = nPhone = 0;

		if ((vCardNode=JabberXmlGetChild(iqNode, "vCard")) != NULL) {
			for (i=0; i<vCardNode->numChild; i++) {
				n = vCardNode->child[i];
				if (n==NULL || n->name==NULL) continue;
				if (!strcmp(n->name, "FN")) {
					if (n->text != NULL) {
						hasFn = TRUE;
						nText = JabberTextDecode(n->text);
						DBWriteContactSettingString(hContact, jabberProtoName, "FullName", nText);
						free(nText);
					}
				}
				else if (!strcmp(n->name, "NICKNAME")) {
					if (n->text != NULL) {
						hasNick = TRUE;
						nText = JabberTextDecode(n->text);
						DBWriteContactSettingString(hContact, jabberProtoName, "Nick", nText);
						free(nText);
					}
				}
				else if (!strcmp(n->name, "N")) {
					// First/Last name
					if (!hasGiven && !hasFamily && !hasMiddle) {
						if ((m=JabberXmlGetChild(n, "GIVEN"))!=NULL && m->text!=NULL) {
							hasGiven = TRUE;
							mText = JabberTextDecode(m->text);
							DBWriteContactSettingString(hContact, jabberProtoName, "FirstName", mText);
							free(mText);
						}
						if ((m=JabberXmlGetChild(n, "FAMILY"))!=NULL && m->text!=NULL) {
							hasFamily = TRUE;
							mText = JabberTextDecode(m->text);
							DBWriteContactSettingString(hContact, jabberProtoName, "LastName", mText);
							free(mText);
						}
						if ((m=JabberXmlGetChild(n, "MIDDLE"))!=NULL && m->text!=NULL) {
							hasMiddle = TRUE;
							mText = JabberTextDecode(m->text);
							DBWriteContactSettingString(hContact, jabberProtoName, "MiddleName", mText);
							free(mText);
						}
					}
				}
				else if (!strcmp(n->name, "EMAIL")) {
					// E-mail address(es)
					if ((m=JabberXmlGetChild(n, "USERID")) == NULL)	// Some bad client put e-mail directly in <EMAIL/> instead of <USERID/>
						m = n;
					if (m->text != NULL) {
						if (hContact != NULL) {
							if (nEmail == 0)
								strcpy(text, "e-mail");
							else
								sprintf(text, "e-mail%d", nEmail-1);
						}
						else
							sprintf(text, "e-mail%d", nEmail);
						mText = JabberTextDecode(m->text);
						DBWriteContactSettingString(hContact, jabberProtoName, text, mText);
						free(mText);
						if (hContact == NULL) {
							sprintf(text, "e-mailFlag%d", nEmail);
							nFlag = 0;
							if (JabberXmlGetChild(n, "HOME") != NULL) nFlag |= JABBER_VCEMAIL_HOME;
							if (JabberXmlGetChild(n, "WORK") != NULL) nFlag |= JABBER_VCEMAIL_WORK;
							if (JabberXmlGetChild(n, "INTERNET") != NULL) nFlag |= JABBER_VCEMAIL_INTERNET;
							if (JabberXmlGetChild(n, "X400") != NULL) nFlag |= JABBER_VCEMAIL_X400;
							DBWriteContactSettingWord(NULL, jabberProtoName, text, nFlag);
						}
						nEmail++;
					}
				}
				else if (!strcmp(n->name, "BDAY")) {
					// Birthday
					if (!hasBday && n->text!=NULL) {
						if (hContact != NULL) {
							if (sscanf(n->text, "%d-%d-%d", &nYear, &nMonth, &nDay) == 3) {
								hasBday = TRUE;
								DBWriteContactSettingWord(hContact, jabberProtoName, "BirthYear", (WORD) nYear);
								DBWriteContactSettingByte(hContact, jabberProtoName, "BirthMonth", (BYTE) nMonth);
								DBWriteContactSettingByte(hContact, jabberProtoName, "BirthDay", (BYTE) nDay);
							}
						}
						else {
							hasBday = TRUE;
							nText = JabberTextDecode(n->text);
							DBWriteContactSettingString(NULL, jabberProtoName, "BirthDate", nText);
							free(nText);
						}
					}
				}
				else if (!strcmp(n->name, "GENDER")) {
					// Gender
					if (!hasGender && n->text!=NULL) {
						if (hContact != NULL) {
							if (n->text[0] && strchr("mMfF", n->text[0])!=NULL) {
								hasGender = TRUE;
								DBWriteContactSettingByte(hContact, jabberProtoName, "Gender", (BYTE) toupper(n->text[0]));
							}
						}
						else {
							hasGender = TRUE;
							nText = JabberTextDecode(n->text);
							DBWriteContactSettingString(NULL, jabberProtoName, "GenderString", nText);
							free(nText);
						}
					}
				}
				else if (!strcmp(n->name, "ADR")) {
					if (!hasHome && JabberXmlGetChild(n, "HOME")!=NULL) {
						// Home address
						hasHome = TRUE;
						if ((m=JabberXmlGetChild(n, "STREET"))!=NULL && m->text!=NULL) {
							hasHomeStreet = TRUE;
							mText = JabberTextDecode(m->text);
							if (hContact != NULL) {
								if ((o=JabberXmlGetChild(n, "EXTADR"))!=NULL && o->text!=NULL) {
									oText = JabberTextDecode(o->text);
									_snprintf(text, sizeof(text), "%s\r\n%s", mText, oText);
									free(oText);
								}
								else if ((o=JabberXmlGetChild(n, "EXTADD"))!=NULL && o->text!=NULL) {
									// Some bad client use <EXTADD/> instead of <EXTADR/>
									oText = JabberTextDecode(o->text);
									_snprintf(text, sizeof(text), "%s\r\n%s", mText, oText);
									free(oText);
								}
								else
									strncpy(text, mText, sizeof(text));
								text[sizeof(text)-1] = '\0';
								DBWriteContactSettingString(hContact, jabberProtoName, "Street", text);
							}
							else {
								DBWriteContactSettingString(hContact, jabberProtoName, "Street", mText);
								if ((m=JabberXmlGetChild(n, "EXTADR")) == NULL)
									m = JabberXmlGetChild(n, "EXTADD");
								if (m!=NULL && m->text!=NULL) {
									hasHomeStreet2 = TRUE;
									free(mText);
									mText = JabberTextDecode(m->text);
									DBWriteContactSettingString(hContact, jabberProtoName, "Street2", mText);
								}
							}
							free(mText);
						}
						if ((m=JabberXmlGetChild(n, "LOCALITY"))!=NULL && m->text!=NULL) {
							hasHomeLocality = TRUE;
							mText = JabberTextDecode(m->text);
							DBWriteContactSettingString(hContact, jabberProtoName, "City", mText);
							free(mText);
						}
						if ((m=JabberXmlGetChild(n, "REGION"))!=NULL && m->text!=NULL) {
							hasHomeRegion = TRUE;
							mText = JabberTextDecode(m->text);
							DBWriteContactSettingString(hContact, jabberProtoName, "State", mText);
							free(mText);
						}
						if ((m=JabberXmlGetChild(n, "PCODE"))!=NULL && m->text!=NULL) {
							hasHomePcode = TRUE;
							mText = JabberTextDecode(m->text);
							DBWriteContactSettingString(hContact, jabberProtoName, "ZIP", mText);
							free(mText);
						}
						if ((m=JabberXmlGetChild(n, "CTRY"))==NULL || m->text==NULL)	// Some bad client use <COUNTRY/> instead of <CTRY/>
							m = JabberXmlGetChild(n, "COUNTRY");
						if (m!=NULL && m->text!=NULL) {
							hasHomeCtry = TRUE;
							mText = JabberTextDecode(m->text);
							if (hContact != NULL)
								DBWriteContactSettingWord(hContact, jabberProtoName, "Country", (WORD) JabberCountryNameToId(mText));
							else
								DBWriteContactSettingString(hContact, jabberProtoName, "CountryName", mText);
							free(mText);
						}
					}
					if (!hasWork && JabberXmlGetChild(n, "WORK")!=NULL) {
						// Work address
						hasWork = TRUE;
						if ((m=JabberXmlGetChild(n, "STREET"))!=NULL && m->text!=NULL) {
							hasWorkStreet = TRUE;
							mText = JabberTextDecode(m->text);
							if (hContact != NULL) {
								if ((o=JabberXmlGetChild(n, "EXTADR"))!=NULL && o->text!=NULL) {
									oText = JabberTextDecode(o->text);
									_snprintf(text, sizeof(text), "%s\r\n%s", mText, oText);
									free(oText);
								}
								else if ((o=JabberXmlGetChild(n, "EXTADD"))!=NULL && o->text!=NULL) {
									// Some bad client use <EXTADD/> instead of <EXTADR/>
									oText = JabberTextDecode(o->text);
									_snprintf(text, sizeof(text), "%s\r\n%s", mText, oText);
									free(oText);
								}
								else
									strncpy(text, mText, sizeof(text));
								text[sizeof(text)-1] = '\0';
								DBWriteContactSettingString(hContact, jabberProtoName, "CompanyStreet", text);
							}
							else {
								DBWriteContactSettingString(hContact, jabberProtoName, "CompanyStreet", mText);
								if ((m=JabberXmlGetChild(n, "EXTADR")) == NULL)
									m = JabberXmlGetChild(n, "EXTADD");
								if (m!=NULL && m->text!=NULL) {
									hasWorkStreet2 = TRUE;
									free(mText);
									mText = JabberTextDecode(m->text);
									DBWriteContactSettingString(hContact, jabberProtoName, "CompanyStreet2", mText);
								}
							}
							free(mText);
						}
						if ((m=JabberXmlGetChild(n, "LOCALITY"))!=NULL && m->text!=NULL) {
							hasWorkLocality = TRUE;
							mText = JabberTextDecode(m->text);
							DBWriteContactSettingString(hContact, jabberProtoName, "CompanyCity", mText);
							free(mText);
						}
						if ((m=JabberXmlGetChild(n, "REGION"))!=NULL && m->text!=NULL) {
							hasWorkRegion = TRUE;
							mText = JabberTextDecode(m->text);
							DBWriteContactSettingString(hContact, jabberProtoName, "CompanyState", mText);
							free(mText);
						}
						if ((m=JabberXmlGetChild(n, "PCODE"))!=NULL && m->text!=NULL) {
							hasWorkPcode = TRUE;
							mText = JabberTextDecode(m->text);
							DBWriteContactSettingString(hContact, jabberProtoName, "CompanyZIP", mText);
							free(mText);
						}
						if ((m=JabberXmlGetChild(n, "CTRY"))==NULL || m->text==NULL)	// Some bad client use <COUNTRY/> instead of <CTRY/>
							m = JabberXmlGetChild(n, "COUNTRY");
						if (m!=NULL && m->text!=NULL) {
							hasWorkCtry = TRUE;
							mText = JabberTextDecode(m->text);
							if (hContact != NULL)
								DBWriteContactSettingWord(hContact, jabberProtoName, "CompanyCountry", (WORD) JabberCountryNameToId(mText));
							else
								DBWriteContactSettingString(hContact, jabberProtoName, "CompanyCountryName", mText);
							free(mText);
						}
					}
				}
				else if (!strcmp(n->name, "TEL")) {
					// Telephone/Fax/Cellular
					if ((m=JabberXmlGetChild(n, "NUMBER"))!=NULL && m->text!=NULL) {
						mText = JabberTextDecode(m->text);
						if (hContact != NULL) {
							if (!hasFax && JabberXmlGetChild(n, "FAX")!=NULL) {
								hasFax = TRUE;
								DBWriteContactSettingString(hContact, jabberProtoName, "Fax", mText);
							}
							if (!hasCell && JabberXmlGetChild(n, "CELL")!=NULL) {
								hasCell = TRUE;
								DBWriteContactSettingString(hContact, jabberProtoName, "Cellular", mText);
							}
							if (!hasPhone &&
								(JabberXmlGetChild(n, "HOME")!=NULL ||
								 JabberXmlGetChild(n, "WORK")!=NULL ||
								 JabberXmlGetChild(n, "VOICE")!=NULL ||
								 (JabberXmlGetChild(n, "FAX")==NULL &&
								  JabberXmlGetChild(n, "PAGER")==NULL &&
								  JabberXmlGetChild(n, "MSG")==NULL &&
								  JabberXmlGetChild(n, "CELL")==NULL &&
								  JabberXmlGetChild(n, "VIDEO")==NULL &&
								  JabberXmlGetChild(n, "BBS")==NULL &&
								  JabberXmlGetChild(n, "MODEM")==NULL &&
								  JabberXmlGetChild(n, "ISDN")==NULL &&
								  JabberXmlGetChild(n, "PCS")==NULL))) {
								hasPhone = TRUE;
								DBWriteContactSettingString(hContact, jabberProtoName, "Phone", mText);
							}
						}
						else {
							sprintf(text, "Phone%d", nPhone);

							DBWriteContactSettingString(NULL, jabberProtoName, text, mText);
							sprintf(text, "PhoneFlag%d", nPhone);
							nFlag = 0;
							if (JabberXmlGetChild(n, "HOME") != NULL) nFlag |= JABBER_VCTEL_HOME;
							if (JabberXmlGetChild(n, "WORK") != NULL) nFlag |= JABBER_VCTEL_WORK;
							if (JabberXmlGetChild(n, "VOICE") != NULL) nFlag |= JABBER_VCTEL_VOICE;
							if (JabberXmlGetChild(n, "FAX") != NULL) nFlag |= JABBER_VCTEL_FAX;
							if (JabberXmlGetChild(n, "PAGER") != NULL) nFlag |= JABBER_VCTEL_PAGER;
							if (JabberXmlGetChild(n, "MSG") != NULL) nFlag |= JABBER_VCTEL_MSG;
							if (JabberXmlGetChild(n, "CELL") != NULL) nFlag |= JABBER_VCTEL_CELL;
							if (JabberXmlGetChild(n, "VIDEO") != NULL) nFlag |= JABBER_VCTEL_VIDEO;
							if (JabberXmlGetChild(n, "BBS") != NULL) nFlag |= JABBER_VCTEL_BBS;
							if (JabberXmlGetChild(n, "MODEM") != NULL) nFlag |= JABBER_VCTEL_MODEM;
							if (JabberXmlGetChild(n, "ISDN") != NULL) nFlag |= JABBER_VCTEL_ISDN;
							if (JabberXmlGetChild(n, "PCS") != NULL) nFlag |= JABBER_VCTEL_PCS;
							DBWriteContactSettingWord(NULL, jabberProtoName, text, nFlag);
							nPhone++;
						}
						free(mText);
					}
				}
				else if (!strcmp(n->name, "URL")) {
					// Homepage
					if (!hasUrl && n->text!=NULL) {
						hasUrl = TRUE;
						nText = JabberTextDecode(n->text);
						DBWriteContactSettingString(hContact, jabberProtoName, "Homepage", nText);
						free(nText);
					}
				}
				else if (!strcmp(n->name, "ORG")) {
					if (!hasOrgname && !hasOrgunit) {
						if ((m=JabberXmlGetChild(n, "ORGNAME"))!=NULL && m->text!=NULL) {
							hasOrgname = TRUE;
							mText = JabberTextDecode(m->text);
							DBWriteContactSettingString(hContact, jabberProtoName, "Company", mText);
							free(mText);
						}
						if ((m=JabberXmlGetChild(n, "ORGUNIT"))!=NULL && m->text!=NULL) {	// The real vCard can have multiple <ORGUNIT/> but we will only display the first one
							hasOrgunit = TRUE;
							mText = JabberTextDecode(m->text);
							DBWriteContactSettingString(hContact, jabberProtoName, "CompanyDepartment", mText);
							free(mText);
						}
					}
				}
				else if (!strcmp(n->name, "ROLE")) {
					if (!hasRole && n->text!=NULL) {
						hasRole = TRUE;
						nText = JabberTextDecode(n->text);
						DBWriteContactSettingString(hContact, jabberProtoName, "Role", nText);
						free(nText);
					}
				}
				else if (!strcmp(n->name, "TITLE")) {
					if (!hasTitle && n->text!=NULL) {
						hasTitle = TRUE;
						nText = JabberTextDecode(n->text);
						DBWriteContactSettingString(hContact, jabberProtoName, "CompanyPosition", nText);
						free(nText);
					}
				}
				else if (!strcmp(n->name, "DESC")) {
					if (!hasDesc && n->text!=NULL) {
						hasDesc = TRUE;
						nText = JabberTextDecode(n->text);
						DBWriteContactSettingString(hContact, jabberProtoName, "About", nText);
						free(nText);
					}
				}
				else if (!strcmp(n->name, "PHOTO")) {
					if (/*hContact==NULL &&*/ !hasPhoto && (m=JabberXmlGetChild(n, "TYPE"))!=NULL && m->text!=NULL && (!strcmp(m->text, "image/jpeg") || !strcmp(m->text, "image/gif") || !strcmp(m->text, "image/bmp"))) {
						if ((o=JabberXmlGetChild(n, "BINVAL"))!=NULL && o->text) {
							char *buffer;
							int bufferLen;
							DWORD nWritten;
							char szTempPath[MAX_PATH], szTempFileName[MAX_PATH];
							HANDLE hFile;
							JABBER_LIST_ITEM *item;

							if (GetTempPath(sizeof(szTempPath), szTempPath) <= 0)
								lstrcpy(szTempPath, ".\\");
							if (GetTempFileName(szTempPath, "jab", 0, szTempFileName) > 0) {
								JabberLog("Picture file name set to %s", szTempFileName);
								if ((buffer=JabberBase64Decode(o->text, &bufferLen)) != NULL) {
									if ((hFile=CreateFile(szTempFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE) {
										JabberLog("Writing %d bytes", bufferLen);
										if (WriteFile(hFile, buffer, bufferLen, &nWritten, NULL)) {
											JabberLog("%d bytes written", nWritten);
											if (hContact == NULL) {
												hasPhoto = TRUE;
												if (jabberVcardPhotoFileName) {
													DeleteFile(jabberVcardPhotoFileName);
													free(jabberVcardPhotoFileName);
												}
												jabberVcardPhotoFileName = _strdup(szTempFileName);
												if (jabberVcardPhotoType) free(jabberVcardPhotoType);
												jabberVcardPhotoType = _strdup(m->text);
												JabberLog("My picture saved to %s", szTempFileName);
											}
											else {
												if (!DBGetContactSetting(hContact, jabberProtoName, "jid", &dbv)) {
													if ((item=JabberListGetItemPtr(LIST_ROSTER, jid)) != NULL) {
														hasPhoto = TRUE;
														if (item->photoFileName)
															DeleteFile(item->photoFileName);
														item->photoFileName = _strdup(szTempFileName);
														JabberLog("Contact's picture saved to %s", szTempFileName);
													}
												}
											}
										}
										CloseHandle(hFile);
									}
									free(buffer);
								}
								if (!hasPhoto)
									DeleteFile(szTempFileName);
							}
						}
					}
				}
			}
		}
		if (!hasFn)
			DBDeleteContactSetting(hContact, jabberProtoName, "FullName");
		// We are not deleting "Nick"
//		if (!hasNick)
//			DBDeleteContactSetting(hContact, jabberProtoName, "Nick");
		if (!hasGiven)
			DBDeleteContactSetting(hContact, jabberProtoName, "FirstName");
		if (!hasFamily)
			DBDeleteContactSetting(hContact, jabberProtoName, "LastName");
		if (!hasMiddle)
			DBDeleteContactSetting(hContact, jabberProtoName, "MiddleName");
		if (hContact != NULL) {
			while (1) {
				if (nEmail <= 0)
					DBDeleteContactSetting(hContact, jabberProtoName, "e-mail");
				else {
					sprintf(text, "e-mail%d", nEmail-1);
					if (DBGetContactSetting(hContact, jabberProtoName, text, &dbv)) break;
					DBFreeVariant(&dbv);
					DBDeleteContactSetting(hContact, jabberProtoName, text);
				}
				nEmail++;
			}
		}
		else {
			while (1) {
				sprintf(text, "e-mail%d", nEmail);
				if (DBGetContactSetting(NULL, jabberProtoName, text, &dbv)) break;
				DBFreeVariant(&dbv);
				DBDeleteContactSetting(NULL, jabberProtoName, text);
				sprintf(text, "e-mailFlag%d", nEmail);
				DBDeleteContactSetting(NULL, jabberProtoName, text);
				nEmail++;
			}
		}
		if (!hasBday) {
			DBDeleteContactSetting(hContact, jabberProtoName, "BirthYear");
			DBDeleteContactSetting(hContact, jabberProtoName, "BirthMonth");
			DBDeleteContactSetting(hContact, jabberProtoName, "BirthDay");
			DBDeleteContactSetting(hContact, jabberProtoName, "BirthDate");
		}
		if (!hasGender) {
			if (hContact != NULL)
				DBDeleteContactSetting(hContact, jabberProtoName, "Gender");
			else
				DBDeleteContactSetting(NULL, jabberProtoName, "GenderString");
		}
		if (hContact != NULL) {
			if (!hasPhone)
				DBDeleteContactSetting(hContact, jabberProtoName, "Phone");
			if (!hasFax)
				DBDeleteContactSetting(hContact, jabberProtoName, "Fax");
			if (!hasCell)
				DBDeleteContactSetting(hContact, jabberProtoName, "Cellular");
		}
		else {
			while (1) {
				sprintf(text, "Phone%d", nPhone);
				if (DBGetContactSetting(NULL, jabberProtoName, text, &dbv)) break;
				DBFreeVariant(&dbv);
				DBDeleteContactSetting(NULL, jabberProtoName, text);
				sprintf(text, "PhoneFlag%d", nPhone);
				DBDeleteContactSetting(NULL, jabberProtoName, text);
				nPhone++;
			}
		}
		if (!hasHomeStreet)
			DBDeleteContactSetting(hContact, jabberProtoName, "Street");
		if (!hasHomeStreet2 && hContact==NULL)
			DBDeleteContactSetting(hContact, jabberProtoName, "Street2");
		if (!hasHomeLocality)
			DBDeleteContactSetting(hContact, jabberProtoName, "City");
		if (!hasHomeRegion)
			DBDeleteContactSetting(hContact, jabberProtoName, "State");
		if (!hasHomePcode)
			DBDeleteContactSetting(hContact, jabberProtoName, "ZIP");
		if (!hasHomeCtry) {
			if (hContact != NULL)
				DBDeleteContactSetting(hContact, jabberProtoName, "Country");
			else
				DBDeleteContactSetting(hContact, jabberProtoName, "CountryName");
		}
		if (!hasWorkStreet)
			DBDeleteContactSetting(hContact, jabberProtoName, "CompanyStreet");
		if (!hasWorkStreet2 && hContact==NULL)
			DBDeleteContactSetting(hContact, jabberProtoName, "CompanyStreet2");
		if (!hasWorkLocality)
			DBDeleteContactSetting(hContact, jabberProtoName, "CompanyCity");
		if (!hasWorkRegion)
			DBDeleteContactSetting(hContact, jabberProtoName, "CompanyState");
		if (!hasWorkPcode)
			DBDeleteContactSetting(hContact, jabberProtoName, "CompanyZIP");
		if (!hasWorkCtry) {
			if (hContact != NULL)
				DBDeleteContactSetting(hContact, jabberProtoName, "CompanyCountry");
			else
				DBDeleteContactSetting(hContact, jabberProtoName, "CompanyCountryName");
		}
		if (!hasUrl)
			DBDeleteContactSetting(hContact, jabberProtoName, "Homepage");
		if (!hasOrgname)
			DBDeleteContactSetting(hContact, jabberProtoName, "Company");
		if (!hasOrgunit)
			DBDeleteContactSetting(hContact, jabberProtoName, "CompanyDepartment");
		if (!hasRole)
			DBDeleteContactSetting(hContact, jabberProtoName, "Role");
		if (!hasTitle)
			DBDeleteContactSetting(hContact, jabberProtoName, "CompanyPosition");
		if (!hasDesc)
			DBDeleteContactSetting(hContact, jabberProtoName, "About");
		if (!hasPhoto && jabberVcardPhotoFileName!=NULL) {
			DeleteFile(jabberVcardPhotoFileName);
			jabberVcardPhotoFileName = NULL;
		}

		if (hContact != NULL)
			ProtoBroadcastAck(jabberProtoName, hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
		else if (hwndJabberVcard)
			SendMessage(hwndJabberVcard, WM_JABBER_REFRESH, 0, 0);
	}
	else if (!strcmp(type, "error")) {
		if (hContact != NULL)
			ProtoBroadcastAck(jabberProtoName, hContact, ACKTYPE_GETINFO, ACKRESULT_FAILED, (HANDLE) 1, 0);
	}
}

void JabberIqResultSetVcard(XmlNode *iqNode, void *userdata)
{
	char *type;

	JabberLog("<iq/> iqIdSetVcard");
	if ((type=JabberXmlGetAttrValue(iqNode, "type")) == NULL) return;

	if (hwndJabberVcard)
		SendMessage(hwndJabberVcard, WM_JABBER_REFRESH, 0, 0);
}

void JabberIqResultSetSearch(XmlNode *iqNode, void *userdata)
{
	XmlNode *queryNode, *itemNode, *n;
	char *type, *jid, *str;
	int id, i;
	JABBER_SEARCH_RESULT jsr;

	JabberLog("<iq/> iqIdGetSearch");
	if ((type=JabberXmlGetAttrValue(iqNode, "type")) == NULL) return;
	if ((str=JabberXmlGetAttrValue(iqNode, "id")) == NULL) return;
	id = atoi(str+strlen(JABBER_IQID));

	if (!strcmp(type, "result")) {
		if ((queryNode=JabberXmlGetChild(iqNode, "query")) == NULL) return;
		jsr.hdr.cbSize = sizeof(JABBER_SEARCH_RESULT);
		for (i=0; i<queryNode->numChild; i++) {
			itemNode = queryNode->child[i];
			if (!strcmp(itemNode->name, "item")) {
				if ((jid=JabberXmlGetAttrValue(itemNode, "jid")) != NULL) {
					strncpy(jsr.jid, jid, sizeof(jsr.jid));
					jsr.jid[sizeof(jsr.jid)-1] = '\0';
					JabberLog("Result jid=%s", jid);
					if ((n=JabberXmlGetChild(itemNode, "nick"))!=NULL && n->text!=NULL)
						jsr.hdr.nick = JabberTextDecode(n->text);
					else
						jsr.hdr.nick = _strdup("");
					if ((n=JabberXmlGetChild(itemNode, "first"))!=NULL && n->text!=NULL)
						jsr.hdr.firstName = JabberTextDecode(n->text);
					else
						jsr.hdr.firstName = _strdup("");
					if ((n=JabberXmlGetChild(itemNode, "last"))!=NULL && n->text!=NULL)
						jsr.hdr.lastName = JabberTextDecode(n->text);
					else
						jsr.hdr.lastName = _strdup("");
					if ((n=JabberXmlGetChild(itemNode, "email"))!=NULL && n->text!=NULL)
						jsr.hdr.email = JabberTextDecode(n->text);
					else
						jsr.hdr.email = _strdup("");
					ProtoBroadcastAck(jabberProtoName, NULL, ACKTYPE_SEARCH, ACKRESULT_DATA, (HANDLE) id, (LPARAM) &jsr);
					free(jsr.hdr.nick);
					free(jsr.hdr.firstName);
					free(jsr.hdr.lastName);
					free(jsr.hdr.email);
				}
			}
		}
		ProtoBroadcastAck(jabberProtoName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) id, 0);
	}
	else if (!strcmp(type, "error")) {
		// ProtoBroadcastAck(jabberProtoName, NULL, ACKTYPE_SEARCH, ACKRESULT_FAILED, (HANDLE) id, 0);
		// There is no ACKRESULT_FAILED for ACKTYPE_SEARCH :) look at findadd.c
		// So we will just send a SUCCESS
		ProtoBroadcastAck(jabberProtoName, NULL, ACKTYPE_SEARCH, ACKRESULT_SUCCESS, (HANDLE) id, 0);
	}
}

void JabberIqResultSetPassword(XmlNode *iqNode, void *userdata)
{
	char *type;

	JabberLog("<iq/> iqIdSetPassword");
	if ((type=JabberXmlGetAttrValue(iqNode, "type")) == NULL) return;

	if (!strcmp(type, "result")) {
		strncpy(jabberThreadInfo->password, jabberThreadInfo->newPassword, sizeof(jabberThreadInfo->password));
		MessageBox(NULL, Translate("Password is successfully changed. Don't forget to update your password in the Jabber protocol option."), Translate("Change Password"), MB_OK|MB_ICONINFORMATION|MB_SETFOREGROUND);
	}
	else if (!strcmp(type, "error")) {
		MessageBox(NULL, Translate("Password cannot be changed."), Translate("Change Password"), MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
	}
}

void JabberIqResultDiscoAgentItems(XmlNode *iqNode, void *userdata)
{
	struct ThreadData *info = (struct ThreadData *) userdata;
	XmlNode *queryNode, *itemNode;
	char *type, *jid, *from;
	JABBER_LIST_ITEM *item;
	int i;
	int iqId;
	char *str;

	// RECVED: agent list
	// ACTION: refresh agent list dialog
	JabberLog("<iq/> iqIdDiscoAgentItems");
	if ((type=JabberXmlGetAttrValue(iqNode, "type")) == NULL) return;
	if ((from=JabberXmlGetAttrValue(iqNode, "from")) == NULL) return;

	if (!strcmp(type, "result")) {
		if ((queryNode=JabberXmlGetChild(iqNode, "query")) != NULL) {
			str = JabberXmlGetAttrValue(queryNode, "xmlns");
			if (str!=NULL && !strcmp(str, "http://jabber.org/protocol/disco#items")) {
				JabberListRemoveList(LIST_AGENT);
				for (i=0; i<queryNode->numChild; i++) {
					if ((itemNode=queryNode->child[i])!=NULL && itemNode->name!=NULL && !strcmp(itemNode->name, "item")) {
						if ((jid=JabberXmlGetAttrValue(itemNode, "jid")) != NULL) {
							item = JabberListAdd(LIST_AGENT, jid);
							item->name = JabberTextDecode(JabberXmlGetAttrValue(itemNode, "name"));
							item->cap = AGENT_CAP_REGISTER | AGENT_CAP_GROUPCHAT;	// default to all cap until specific info is later received
							iqId = JabberSerialNext();
							JabberIqAdd(iqId, IQ_PROC_NONE, JabberIqResultDiscoAgentInfo);
							JabberSend(jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='http://jabber.org/protocol/disco#info'/></iq>", iqId, jid);
						}
					}
				}
			}
		}
		if (hwndJabberAgents != NULL) {
			if ((jid=JabberXmlGetAttrValue(iqNode, "from")) != NULL)
				SendMessage(hwndJabberAgents, WM_JABBER_AGENT_REFRESH, 0, (LPARAM) jid);
			else
				SendMessage(hwndJabberAgents, WM_JABBER_AGENT_REFRESH, 0, (LPARAM) info->server);
		}
	}
	else if (!strcmp(type, "error")) {
		// disco is not supported, try jabber:iq:agents
		iqId = JabberSerialNext();
		JabberIqAdd(iqId, IQ_PROC_GETAGENTS, JabberIqResultGetAgents);
		JabberSend(jabberThreadInfo->s, "<iq type='get' id='"JABBER_IQID"%d' to='%s'><query xmlns='jabber:iq:agents'/></iq>", iqId, from);
	}
}

void JabberIqResultDiscoAgentInfo(XmlNode *iqNode, void *userdata)
{
	struct ThreadData *info = (struct ThreadData *) userdata;
	XmlNode *queryNode, *itemNode, *identityNode;
	char *type, *from, *var;
	JABBER_LIST_ITEM *item;
	int i;
	char *str;

	// RECVED: info for a specific agent
	// ACTION: refresh agent list dialog
	JabberLog("<iq/> iqIdDiscoAgentInfo");
	if ((type=JabberXmlGetAttrValue(iqNode, "type")) == NULL) return;
	if ((from=JabberXmlGetAttrValue(iqNode, "from")) == NULL) return;

	if (!strcmp(type, "result")) {
		if ((queryNode=JabberXmlGetChild(iqNode, "query")) != NULL) {
			str = JabberXmlGetAttrValue(queryNode, "xmlns");
			if (str!=NULL && !strcmp(str, "http://jabber.org/protocol/disco#info")) {
				if ((item=JabberListGetItemPtr(LIST_AGENT, from)) != NULL) {
					// Use the first <identity/> to set name
					if ((identityNode=JabberXmlGetChild(queryNode, "identity")) != NULL) {
						if ((str=JabberXmlGetAttrValue(identityNode, "name")) != NULL) {
							if (item->name) free(item->name);
							item->name = JabberTextDecode(str);
						}
					}
					item->cap = 0;
					for (i=0; i<queryNode->numChild; i++) {
						if ((itemNode=queryNode->child[i])!=NULL && itemNode->name!=NULL) {
							if (!strcmp(itemNode->name, "feature")) {
								if ((var=JabberXmlGetAttrValue(itemNode, "var")) != NULL) {
									if (!strcmp(var, "jabber:iq:register"))
										item->cap |= AGENT_CAP_REGISTER;
									else if (!strcmp(var, "http://jabber.org/protocol/muc"))
										item->cap |= AGENT_CAP_GROUPCHAT;
								}
							}
						}
					}
				}
			}
		}
		if (hwndJabberAgents != NULL)
			SendMessage(hwndJabberAgents, WM_JABBER_AGENT_REFRESH, 0, (LPARAM) NULL);
	}
}

void JabberIqResultDiscoClientInfo(XmlNode *iqNode, void *userdata)
{
	struct ThreadData *info = (struct ThreadData *) userdata;
	XmlNode *queryNode, *itemNode;
	char *type, *from, *var;
	JABBER_LIST_ITEM *item;
	int i;
	char *str;

	// RECVED: info for a specific client
	// ACTION: update client cap
	// ACTION: if item->ft is pending, initiate file transfer
	JabberLog("<iq/> iqIdDiscoClientInfo");
	if ((type=JabberXmlGetAttrValue(iqNode, "type")) == NULL) return;
	if ((from=JabberXmlGetAttrValue(iqNode, "from")) == NULL) return;

	if (!strcmp(type, "result")) {
		if ((item=JabberListGetItemPtr(LIST_ROSTER, from)) != NULL) {
			if ((queryNode=JabberXmlGetChild(iqNode, "query")) != NULL) {
				str = JabberXmlGetAttrValue(queryNode, "xmlns");
				if (str!=NULL && !strcmp(str, "http://jabber.org/protocol/disco#info")) {
					item->cap = CLIENT_CAP_READY;
					for (i=0; i<queryNode->numChild; i++) {
						if ((itemNode=queryNode->child[i])!=NULL && itemNode->name!=NULL) {
							if (!strcmp(itemNode->name, "feature")) {
								if ((var=JabberXmlGetAttrValue(itemNode, "var")) != NULL) {
									if (!strcmp(var, "http://jabber.org/protocol/si"))
										item->cap |= CLIENT_CAP_SI;
									else if (!strcmp(var, "http://jabber.org/protocol/si/profile/file-transfer"))
										item->cap |= CLIENT_CAP_SIFILE;
									else if (!strcmp(var, "http://jabber.org/protocol/bytestreams"))
										item->cap |= CLIENT_CAP_BYTESTREAM;
								}
							}
						}
					}
				}
			}
			// Check for pending file transfer session request
			if (item->ft != NULL) {
				JABBER_FILE_TRANSFER *ft;

				ft = item->ft;
				item->ft = NULL;
				if ((item->cap&CLIENT_CAP_FILE) && (item->cap&CLIENT_CAP_BYTESTREAM))
					JabberFtInitiate(item->jid, ft);
				else
					JabberForkThread(( JABBER_THREAD_FUNC )JabberFileServerThread, 0, ft);
			}
		}
	}
}
