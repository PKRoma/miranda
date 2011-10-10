/*
Plugin of Miranda IM for communicating with users of the AIM protocol.
Copyright (c) 2008-2011 Boris Krasnovskiy
Copyright (C) 2005-2006 Aaron Myles Landwehr

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "aim.h"

INT_PTR CAimProto::GetMyAwayMsg(WPARAM wParam,LPARAM lParam)
{
	char** msgptr = get_status_msg_loc(wParam ? wParam : m_iStatus);
	if (msgptr == NULL)	return 0;

	return (lParam & SGMA_UNICODE) ? (INT_PTR)mir_utf8decodeW(*msgptr) : (INT_PTR)mir_utf8decodeA(*msgptr);
}

int CAimProto::OnIdleChanged(WPARAM /*wParam*/, LPARAM lParam)
{
	if (state != 1) 
	{
		idle=0;
		return 0;
	}

	if (instantidle) //ignore- we are instant idling at the moment
		return 0;

	bool bIdle = (lParam & IDF_ISIDLE) != 0;
	bool bPrivacy = (lParam & IDF_PRIVACY) != 0;

	if (bPrivacy && idle) 
	{
		aim_set_idle(hServerConn,seqno,0);
		return 0;
	}

	if (bPrivacy)
		return 0;

	if (bIdle)  //don't want to change idle time if we are already idle
	{
		MIRANDA_IDLE_INFO mii = {0};
		mii.cbSize = sizeof(mii);
		CallService(MS_IDLE_GETIDLEINFO, 0, (LPARAM) & mii);

		idle = 1;
		aim_set_idle(hServerConn,seqno,mii.idleTime * 60);
	}
	else aim_set_idle(hServerConn,seqno,0);

	return 0;
}

int CAimProto::OnWindowEvent(WPARAM wParam, LPARAM lParam)
{
	MessageWindowEventData* msgEvData  = (MessageWindowEventData*)lParam;

	if (msgEvData->uType == MSG_WINDOW_EVT_CLOSE) 
	{
		if (state != 1 || !is_my_contact(msgEvData->hContact)) 
			return 0;

		if (getWord(msgEvData->hContact, AIM_KEY_ST, ID_STATUS_OFFLINE) == ID_STATUS_ONTHEPHONE)
			return 0;

		DBVARIANT dbv;
		if (!getBool(msgEvData->hContact, AIM_KEY_BLS, false) && !getString(msgEvData->hContact, AIM_KEY_SN, &dbv)) 
		{
			if (_stricmp(dbv.pszVal, SYSTEM_BUDDY))
				aim_typing_notification(hServerConn, seqno, dbv.pszVal, 0x000f);
			DBFreeVariant(&dbv);
		}
	}
	return 0;
}

INT_PTR CAimProto::GetProfile(WPARAM wParam, LPARAM lParam)
{
	if (state != 1)
		return 0;

	DBVARIANT dbv;
	if (!getString((HANDLE)wParam, AIM_KEY_SN, &dbv))
	{
		request_HTML_profile = 1;
		aim_query_profile(hServerConn, seqno, dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	return 0;
}

INT_PTR CAimProto::GetHTMLAwayMsg(WPARAM wParam, LPARAM /*lParam*/)
{
	if (state != 1)
		return 0;

	DBVARIANT dbv;
	if (!getString((HANDLE)wParam, AIM_KEY_SN, &dbv))
	{
		request_away_message = 1;
		aim_query_away_message(hServerConn, seqno, dbv.pszVal);
	}
	return 0;
}

int CAimProto::OnDbSettingChanged(WPARAM wParam,LPARAM lParam)
{
	DBCONTACTWRITESETTING *cws = (DBCONTACTWRITESETTING*)lParam;

	if (strcmp(cws->szModule, MOD_KEY_CL) == 0 && state == 1 && wParam)
	{
		HANDLE hContact = (HANDLE)wParam;
		if (strcmp(cws->szSetting, AIM_KEY_NL) == 0)
		{
			if (cws->value.type == DBVT_DELETED)
			{
				DBVARIANT dbv;
				if(!DBGetContactSettingStringUtf(hContact, MOD_KEY_CL, OTH_KEY_GP, &dbv) && dbv.pszVal[0])
				{
					add_contact_to_group(hContact, dbv.pszVal);
					DBFreeVariant(&dbv);
				}
				else
					add_contact_to_group(hContact, AIM_DEFAULT_GROUP);
			}
		}
		else if (strcmp(cws->szSetting, "MyHandle") == 0)
		{
			char* name;
			switch (cws->value.type)
			{
			case DBVT_DELETED:
				set_local_nick(hContact, NULL, NULL);
				break;

			case DBVT_ASCIIZ:
				name = mir_utf8encode(cws->value.pszVal);
				set_local_nick(hContact, name, NULL);
				mir_free(name);
				break;

			case DBVT_UTF8:
				set_local_nick(hContact, cws->value.pszVal, NULL);
				break;

			case DBVT_WCHAR:
				name = mir_utf8encodeW(cws->value.pwszVal);
				set_local_nick(hContact, name, NULL);
				mir_free(name);
				break;
			}
		}
	}

	return 0;
}

int CAimProto::OnContactDeleted(WPARAM wParam,LPARAM /*lParam*/)
{
	if (state != 1) return 0;

	const HANDLE hContact = (HANDLE)wParam;

	if (DBGetContactSettingByte(hContact, MOD_KEY_CL, AIM_KEY_NL, 0))
		return 0;

	DBVARIANT dbv;
	if (!getString(hContact, AIM_KEY_SN, &dbv)) 
	{
		for(int i=1;;++i)
		{
			unsigned short item_id = getBuddyId(hContact, i);
			if (item_id == 0) break; 

			unsigned short group_id = getGroupId(hContact, i);
			if (group_id)
			{
				aim_ssi_update(hServerConn, seqno, true);
				aim_delete_contact(hServerConn, seqno, dbv.pszVal, item_id, group_id, 0);
				char* group = group_list.find_name(group_id);
				update_server_group(group, group_id);
				aim_ssi_update(hServerConn, seqno, false);
			}
		}
		DBFreeVariant(&dbv);
	}
	return 0;
}


int CAimProto::OnGroupChange(WPARAM wParam,LPARAM lParam)
{
	if (state != 1 || !getByte(AIM_KEY_MG, 1)) return 0;

	const HANDLE hContact = (HANDLE)wParam;
	const CLISTGROUPCHANGE* grpchg = (CLISTGROUPCHANGE*)lParam;

	if (hContact == NULL)
	{
		if (grpchg->pszNewName == NULL && grpchg->pszOldName != NULL)
		{
			char* szOldName = mir_utf8encodeT(grpchg->pszOldName);
			unsigned short group_id = group_list.find_id(szOldName);
			if (group_id)
			{
				aim_delete_contact(hServerConn, seqno, szOldName, 0, group_id, 1);
				group_list.remove_by_id(group_id);
				update_server_group("", 0);
			}
			mir_free(szOldName);
		}
		else if (grpchg->pszNewName != NULL && grpchg->pszOldName != NULL)
		{
			char* szOldName = mir_utf8encodeT(grpchg->pszOldName);
			unsigned short group_id = group_list.find_id(szOldName);
			if (group_id)
			{
				char* szNewName = mir_utf8encodeT(grpchg->pszNewName);
				update_server_group(szNewName, group_id);
				mir_free(szNewName);
			}
			mir_free(szOldName);
		}
	}
	else
	{
		if (is_my_contact(hContact) && getBuddyId(hContact, 1) && 
			!DBGetContactSettingByte(hContact, MOD_KEY_CL, AIM_KEY_NL, 0))
		{
			if (grpchg->pszNewName)
			{
				char* szNewName = mir_utf8encodeT(grpchg->pszNewName);
				add_contact_to_group(hContact, szNewName);
				mir_free(szNewName);
			}
			else
				add_contact_to_group(hContact, AIM_DEFAULT_GROUP);
		}
	}
	return 0;
}

INT_PTR CAimProto::AddToServerList(WPARAM wParam, LPARAM /*lParam*/)
{
	if (state != 1) return 0;

	HANDLE hContact = (HANDLE)wParam;
	DBVARIANT dbv;
	if (!DBGetContactSettingStringUtf(hContact, MOD_KEY_CL, OTH_KEY_GP, &dbv) && dbv.pszVal[0])
	{
		add_contact_to_group(hContact, dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	else 
		add_contact_to_group(hContact, AIM_DEFAULT_GROUP);
	return 0;
}

INT_PTR CAimProto::BlockBuddy(WPARAM wParam, LPARAM /*lParam*/)
{
	if (state != 1)	return 0;

	HANDLE hContact = (HANDLE)wParam;
	unsigned short item_id;
	DBVARIANT dbv;
	if (getString(hContact, AIM_KEY_SN, &dbv)) return 0;

	switch(pd_mode)
	{
	case 1:
		pd_mode = 4;
		aim_set_pd_info(hServerConn, seqno);

	case 4:
		item_id = block_list.find_id(dbv.pszVal);
		if (item_id != 0)
		{
			block_list.remove_by_id(item_id);
			aim_delete_contact(hServerConn, seqno, dbv.pszVal, item_id, 0, 3);
		}
		else
		{
			item_id = block_list.add(dbv.pszVal);
			aim_add_contact(hServerConn, seqno, dbv.pszVal, item_id, 0, 3);
		}
		break;

	case 2:
		pd_mode = 3;
		aim_set_pd_info(hServerConn, seqno);

	case 3:
		item_id = allow_list.find_id(dbv.pszVal);
		if (item_id != 0)
		{
			allow_list.remove_by_id(item_id);
			aim_delete_contact(hServerConn, seqno, dbv.pszVal, item_id, 0, 2);
		}
		else
		{
			item_id = allow_list.add(dbv.pszVal);
			aim_add_contact(hServerConn, seqno, dbv.pszVal, item_id, 0, 2);
		}
		break;
	}
	DBFreeVariant(&dbv);

	return 0;
}

 INT_PTR CAimProto::JoinChatUI(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_CHAT), NULL, join_chat_dialog, LPARAM(this));
	return 0;
}

INT_PTR CAimProto::OnJoinChat(WPARAM wParam, LPARAM /*lParam*/)
{
	if (state != 1)	return 0;

	HANDLE hContact = (HANDLE)wParam;

	DBVARIANT dbv;
	if (!getString(hContact, "ChatRoomID", &dbv))
	{
		chatnav_param* par = new chatnav_param(dbv.pszVal, getWord(hContact, "Exchange", 4));
		ForkThread(&CAimProto::chatnav_request_thread, par);
		DBFreeVariant(&dbv);
	}
	return 0;
}

INT_PTR CAimProto::OnLeaveChat(WPARAM wParam, LPARAM /*lParam*/)
{
	if (state != 1)	return 0;

	HANDLE hContact = (HANDLE)wParam;

	DBVARIANT dbv;
	if (!getString(hContact, "ChatRoomID", &dbv))
	{
		chat_leave(dbv.pszVal);
		DBFreeVariant(&dbv);
	}
	return 0;
}

INT_PTR CAimProto::InstantIdle(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_IDLE), NULL, instant_idle_dialog, LPARAM(this));
	return 0;
}

INT_PTR CAimProto::ManageAccount(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	execute_cmd("https://my.screenname.aol.com");
	return 0;
}

INT_PTR CAimProto::GetAvatarInfo(WPARAM wParam, LPARAM lParam)
{
	PROTO_AVATAR_INFORMATION* AI = (PROTO_AVATAR_INFORMATION*)lParam;
	
	INT_PTR res = GAIR_NOAVATAR;
	AI->filename[0] = 0;
	AI->format = PA_FORMAT_UNKNOWN;

	if (getByte(AIM_KEY_DA, 0)) return res;

	if (AI->hContact == NULL)
	{
		get_avatar_filename(NULL, AI->filename, sizeof(AI->filename), NULL);
		AI->format = detect_image_type(AI->filename);
		return (AI->format ? GAIR_SUCCESS : GAIR_NOAVATAR);
	}

	char* hash = getSetting(AI->hContact, AIM_KEY_AH);
	if (hash)
	{
		get_avatar_filename(AI->hContact, AI->filename, sizeof(AI->filename), NULL);
		char* hashs=getSetting(AI->hContact, AIM_KEY_ASH);
		if (hashs && strcmp(hashs, hash) == 0)
		{
			AI->format = detect_image_type(AI->filename);
			if (AI->format != PA_FORMAT_UNKNOWN) res = GAIR_SUCCESS;
		}
		mir_free(hashs);

		if ((wParam & GAIF_FORCE) != 0 && res != GAIR_SUCCESS)
		{
			WORD wStatus = getWord(AI->hContact, AIM_KEY_ST, ID_STATUS_OFFLINE);
			if (wStatus == ID_STATUS_OFFLINE) 
			{
				deleteSetting(AI->hContact, "AvatarHash");
			}
			else 
			{
				avatar_req_param *ar = new avatar_req_param(getSetting(AI->hContact, AIM_KEY_SN), mir_strdup(hash));
				LOG("Starting avatar request thread for %s)", ar->sn);
				ForkThread(&CAimProto::avatar_request_thread, ar);
				res = GAIR_WAITFOR;
			}
		}
		mir_free(hash);
	}
	return res;
}

INT_PTR CAimProto::GetAvatarCaps(WPARAM wParam, LPARAM lParam)
{
	int res = 0;

	switch (wParam)
	{
	case AF_MAXSIZE:
		((POINT*)lParam)->x = 64;
		((POINT*)lParam)->y = 64;
		break;

	case AF_MAXFILESIZE:
		res = 7168;
		break;

	case AF_PROPORTION:
		res = PIP_SQUARE;
		break;

	case AF_FORMATSUPPORTED:
		res = (lParam == PA_FORMAT_JPEG || lParam == PA_FORMAT_GIF || lParam == PA_FORMAT_BMP);
		break;

	case AF_ENABLED:
	case AF_DONTNEEDDELAYS:
	case AF_FETCHALWAYS:
		res = 1;
		break;
	}

	return res;
}

INT_PTR CAimProto::GetAvatar(WPARAM wParam, LPARAM lParam)
{
	char* buf = (char*)wParam;
	int  size = (int)lParam;

	if (buf == NULL || size <= 0)
		return -1;

	get_avatar_filename(NULL, buf, size, NULL);
	return _access(buf, 0);
}

INT_PTR CAimProto::SetAvatar(WPARAM wParam, LPARAM lParam)
{
	char* szFileName = (char*)lParam;

	char tFileName[MAX_PATH];
	get_avatar_filename(NULL, tFileName, sizeof(tFileName), NULL);
	remove(tFileName);

	if (szFileName == NULL)
	{
		aim_set_avatar_hash(hServerConn, seqno, 0, 1, 5, "\x02\x01\xd2\x04\x72");
	}
	else
	{
		char hash[16], *data;
		unsigned short size;
		if (!get_avatar_hash(szFileName, hash, &data, size))
			return 1;

		char* tFileName = (char*)mir_alloc(MAX_PATH);
		char *ext = strrchr(szFileName, '.');
		get_avatar_filename(NULL, tFileName, MAX_PATH, ext);
		int fileId = _open(tFileName, _O_CREAT | _O_TRUNC | _O_WRONLY | O_BINARY, _S_IREAD | _S_IWRITE);
		if (fileId < 0)
		{
//			ShowError("Cannot set avatar. File '%s' could not be created/overwritten", tFileName);
			mir_free(tFileName);         
			return 1; 
		}
		_write(fileId, data, size);
		_close(fileId);
	
		ForkThread(&CAimProto::avatar_upload_thread, tFileName);
	}
	return 0;
}
