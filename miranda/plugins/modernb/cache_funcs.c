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

Created by Pescuma
Modified by FYR

*/

/************************************************************************/
/*             Module for working with lines text and avatars           */
/************************************************************************/

#include "commonheaders.h"
#include "cache_funcs.h"


/*
 *	Module Prototypes
 */
int CopySkipUnPrintableChars(TCHAR *to, TCHAR * buf, DWORD size);
typedef BOOL (* ExecuteOnAllContactsFuncPtr) (struct ClcContact *contact, BOOL subcontact, void *param);
BOOL ExecuteOnAllContacts(struct ClcData *dat, ExecuteOnAllContactsFuncPtr func, void *param);
BOOL ExecuteOnAllContactsOfGroup(struct ClcGroup *group, ExecuteOnAllContactsFuncPtr func, void *param);

/*
 *	Module global variables to implement away messages requests queue
 *  need to avoid disconnection from server (ICQ)
 */

#define ASKPERIOD 3000

typedef struct _AskChain {
	HANDLE ContactRequest;
	struct AskChain *Next;
}AskChain;

AskChain * FirstChain=NULL;
AskChain * LastChain=NULL;
BOOL LockChainAddition=0;
BOOL LockChainDeletion=0;
DWORD RequestTick=0;
BOOL ISTREADSTARTED=0;


/*
 *  Add contact handle to requests queue
 */
int AddHandleToChain(HANDLE hContact)
{
  AskChain * workChain;
  while (LockChainAddition) 
  {
    SleepEx(0,TRUE);
    if (Miranda_Terminated()) return 0;
  }
  LockChainDeletion=1;
  {
    //check that handle is present
     AskChain * wChain;
     wChain=FirstChain;
     if (wChain)
     {
       do {
         if (wChain->ContactRequest==hContact)
         {
           LockChainDeletion=0;
           return 0;
         }
       } while(wChain=(AskChain *)wChain->Next);
     }
  }
  if (!FirstChain)  
  {
    FirstChain=mir_alloc(sizeof(AskChain));
    workChain=FirstChain;

  }
  else 
  {
    LastChain->Next=mir_alloc(sizeof(AskChain));
    workChain=(AskChain *)LastChain->Next;
  }
  LastChain=workChain;
  workChain->Next=NULL;
  workChain->ContactRequest=hContact;
  LockChainDeletion=0;
  return 1;
}


/*
 *	Gets handle from queue for request
 */
HANDLE GetCurrChain()
{
  struct AskChain * workChain;
  HANDLE res=NULL;
  while (LockChainDeletion)   
  {
    SleepEx(0,TRUE);
    if (Miranda_Terminated()) return 0;
  }
  LockChainAddition=1;
  if (FirstChain)
  {
    res=FirstChain->ContactRequest;
    workChain=FirstChain->Next;
    mir_free(FirstChain);
    FirstChain=(AskChain *)workChain;
  }
  LockChainAddition=0;
  return res;
}


/*
 *	Tread sub to ask protocol to retrieve away message
 */
int AskStatusMessageThread(HWND hwnd)
{
  DWORD time;
  HANDLE h;
  HANDLE ACK=0;
  pdisplayNameCacheEntry pdnce=NULL;
  h=GetCurrChain(); 
  if (!h) return 0;

  ISTREADSTARTED=1;
  while (h)
  { 
    time=GetTickCount();
    if ((time-RequestTick)<ASKPERIOD)
    {
            SleepEx(ASKPERIOD-(time-RequestTick)+10,TRUE);
            if (Miranda_Terminated()) 
            {
              ISTREADSTARTED=0;
              return 0; 
            }
    }

	{
		pdnce = (pdisplayNameCacheEntry)pcli->pfnGetCacheEntry((HANDLE)h);
		if (pdnce->ApparentMode!=ID_STATUS_OFFLINE) //don't ask if contact is always invisible (should be done with protocol)
			ACK=(HANDLE)CallContactService(h,PSS_GETAWAYMSG,0,0);
	}   
    if (!ACK)
    {
      ACKDATA ack;
      ack.hContact=h;
      ack.type=ACKTYPE_AWAYMSG;
      ack.result=ACKRESULT_FAILED;
      if (pdnce)
        ack.szModule=pdnce->szProto;
      else
        ack.szModule=NULL;
      ClcProtoAck((WPARAM)h,(LPARAM) &ack);
    }
    RequestTick=time;
    h=GetCurrChain();
    if (h) SleepEx(ASKPERIOD,TRUE); else break;
    if (Miranda_Terminated()) 
    {
      ISTREADSTARTED=0;
      return 0; 
    }

  }
  ISTREADSTARTED=0;
  return 1;
}



/*
 *	Sub to be called outside on status changing to retrieve away message
 */
void ReAskStatusMessage(HANDLE wParam)
{
  int res;
  if (!DBGetContactSettingByte(NULL,"ModernData","InternalAwayMsgDiscovery",0)) return;
  {//Do not re-ask if it is IRC protocol    
	char *szProto = (char *) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM) wParam, 0);
	if (szProto != NULL) 
	{
		if(DBGetContactSettingByte(wParam, szProto, "ChatRoom", 0) != 0) return;
	}
	else return;
  }
  res=AddHandleToChain(wParam); 
  if (!ISTREADSTARTED && res) 
  {
    TRACE("....start ask thread\n");
    forkthread(AskStatusMessageThread,0,0);
  }
  return;
}






/*
 *	Get time zone for contact
 */
void Cache_GetTimezone(struct ClcData *dat, struct ClcContact *contact)
{
	if (IsBadWritePtr(contact,sizeof(struct ClcContact))) return;
	if (!IsBadStringPtrA(contact->proto,100))
		contact->timezone = (DWORD)DBGetContactSettingByte(contact->hContact,"UserInfo","Timezone", 
									DBGetContactSettingByte(contact->hContact, contact->proto,"Timezone",-1));
	else contact->timezone =-1;
	contact->timediff = 0;

	if (contact->timezone != -1)
	{
		int contact_gmt_diff = contact->timezone;
		contact_gmt_diff = contact_gmt_diff > 128 ? 256 - contact_gmt_diff : 0 - contact_gmt_diff;
		contact_gmt_diff *= 60*60/2;

		// Only in case of same timezone, ignore DST
		if (contact_gmt_diff != dat->local_gmt_diff)
			contact->timediff = (int)dat->local_gmt_diff_dst - contact_gmt_diff;
	}
}


/*
 *	Get all lines of text
 */ 
void Cache_GetText(struct ClcData *dat, struct ClcContact *contact)
{
	Cache_GetFirstLineText(dat, contact);
	Cache_GetSecondLineText(dat, contact);
	Cache_GetThirdLineText(dat, contact);
}

/*
*	Destroy smiley list
*/ 
void Cache_DestroySmileyList( SortedList* p_list )
{
	if ( p_list == NULL )
		return;

	if ( p_list->items != NULL )
	{
		int i;
		for ( i = 0 ; i < p_list->realCount ; i++ )
		{
			if ( p_list->items[i] != NULL )
			{
				ClcContactTextPiece *piece = (ClcContactTextPiece *) p_list->items[i];

				if (!IsBadWritePtr(piece, sizeof(ClcContactTextPiece)))
				{
				if (piece->smiley)
					DestroyIcon(piece->smiley);

					mir_free(piece);
				}
			}
		}
		li.List_Destroy( p_list );
	}
	free(p_list);
	
}


/*
* Parsing of text for smiley
*/
void Cache_ReplaceSmileys(struct ClcData *dat, struct ClcContact *contact, TCHAR *text, int text_size, SortedList **plText, 
						 int *max_smiley_height, BOOL replace_smileys)
{
	SMADD_PARSET sp;
	int last_pos=0;
        *max_smiley_height = 0;

	if (!dat->text_replace_smileys || !replace_smileys || text == NULL || !ServiceExists(MS_SMILEYADD_PARSE))
	{
		Cache_DestroySmileyList(*plText);
		*plText = NULL;
		return;
	}

	// Free old list
	if (*plText != NULL)
	{
		Cache_DestroySmileyList(*plText);
		*plText = NULL;
	}

	// Call service for the first time to see if needs to be used...
	sp.cbSize = sizeof(sp);

	if (dat->text_use_protocol_smileys)
	{
		sp.Protocolname = contact->proto;

		if (DBGetContactSettingByte(NULL,"CLC","Meta",0) != 1 && contact->proto != NULL && strcmp(contact->proto, "MetaContacts") == 0)
		{
			HANDLE hContact = (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT, (UINT)contact->hContact, 0);
			if (hContact != 0)
			{
				sp.Protocolname = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (UINT)hContact, 0);
			}
		}
	}
	else
	{
		sp.Protocolname = "clist";
	}

	sp.str = text;
	sp.startChar = 0;
	sp.size = 0;
	CallService(MS_SMILEYADD_PARSET, 0, (LPARAM)&sp);

	if (sp.size == 0)
	{
		// Did not find a simley
		return;
	}

	// Lets add smileys
	*plText = li.List_Create( 0, 10 );

	do
	{
		if (sp.SmileyIcon != NULL)	// For deffective smileypacks
		{
		// Add text
		if (sp.startChar-last_pos > 0)
		{
			ClcContactTextPiece *piece = (ClcContactTextPiece *) mir_alloc(sizeof(ClcContactTextPiece));

			piece->type = TEXT_PIECE_TYPE_TEXT;
			piece->start_pos = last_pos ;//sp.str - text;
			piece->len = sp.startChar-last_pos;
			li.List_Insert(*plText, piece, plText[0]->realCount);
		}

		// Add smiley
		{
			BITMAP bm;
			ICONINFO icon;
			ClcContactTextPiece *piece = (ClcContactTextPiece *) mir_alloc(sizeof(ClcContactTextPiece));

			piece->type = TEXT_PIECE_TYPE_SMILEY;
			piece->len = sp.size;
			piece->smiley = sp.SmileyIcon;

			if (GetIconInfo(piece->smiley, &icon) && GetObject(icon.hbmColor,sizeof(BITMAP),&bm))
			{
				piece->smiley_width = bm.bmWidth;
				piece->smiley_height = bm.bmHeight;
			}
			else
			{
				piece->smiley_width = 16;
				piece->smiley_height = 16;
			}

				dat->text_smiley_height = max(piece->smiley_height, dat->text_smiley_height);
				*max_smiley_height = max(piece->smiley_height, *max_smiley_height);

			li.List_Insert(*plText, piece, plText[0]->realCount);
		}
}
		/*
		 *	Bokra SmileyAdd Fix:
		 */
		// Get next
		last_pos=sp.startChar+sp.size;
		CallService(MS_SMILEYADD_PARSET, 0, (LPARAM)&sp);
		
	}
	while (sp.size != 0);

	//	// Get next
	//	sp.str += sp.startChar + sp.size;
	//	sp.startChar = 0;
	//	sp.size = 0;
	//	CallService(MS_SMILEYADD_PARSE, 0, (LPARAM)&sp);
	//}
	//while (sp.SmileyIcon != NULL && sp.size != 0);

	// Add rest of text
	if (last_pos < text_size)
	{
		ClcContactTextPiece *piece = (ClcContactTextPiece *) mir_alloc(sizeof(ClcContactTextPiece));

		piece->type = TEXT_PIECE_TYPE_TEXT;
		piece->start_pos = last_pos;
		piece->len = text_size-last_pos;

		li.List_Insert(*plText, piece, plText[0]->realCount);
	}
}

/*
 *	Getting Status name
 *  -1 for XStatus, 1 for Status
 */
int GetStatusName(TCHAR *text, int text_size, struct ClcContact *contact, BOOL xstatus_has_priority) 
{
	BOOL noAwayMsg=FALSE;
	BOOL noXstatus=FALSE;
	// Hide status text if Offline  /// no offline		
	if (contact->status==0)
	{
		contact->status=GetContactCachedStatus(contact->hContact);
	}
	if ((contact->status==ID_STATUS_OFFLINE || contact->status==0) && DBGetContactSettingByte(NULL,"ModernData","RemoveAwayMessageForOffline",0)) noAwayMsg=TRUE;
	if (contact->status==ID_STATUS_OFFLINE || contact->status==0) noXstatus=TRUE;
	text[0] = '\0';
	// Get XStatusName
	if (!noAwayMsg&& !noXstatus&& xstatus_has_priority && contact->hContact && contact->proto)
	{
		DBVARIANT dbv={0};
		if (!DBGetContactSettingTString(contact->hContact, contact->proto, "XStatusName", &dbv)) 
		{
			//lstrcpyn(text, dbv.pszVal, text_size);
			CopySkipUnPrintableChars(text, dbv.ptszVal, text_size-1);
			DBFreeVariant(&dbv);

			if (text[0] != '\0')
				return -1;
		}
	}

	// Get Status name
	{
		TCHAR *tmp = (TCHAR *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)contact->status, GCMDF_TCHAR_MY);
		lstrcpyn(text, tmp, text_size);
		//CopySkipUnPrintableChars(text, dbv.pszVal, text_size-1);
		if (text[0] != '\0')
			return 1;
	}

	// Get XStatusName
	if (!noAwayMsg && !noXstatus && !xstatus_has_priority && contact->hContact && contact->proto)
	{
		DBVARIANT dbv={0};
		if (!DBGetContactSettingTString(contact->hContact, contact->proto, "XStatusName", &dbv)) 
		{
			//lstrcpyn(text, dbv.pszVal, text_size);
			CopySkipUnPrintableChars(text, dbv.ptszVal, text_size-1);
			DBFreeVariant(&dbv);

			if (text[0] != '\0')
				return -1;
		}
	}

	return 1;
}

/*
*	Getting Status message (Away message)
*  -1 for XStatus, 1 for Status
*/
int GetStatusMessage(TCHAR *text, int text_size, struct ClcContact *contact, BOOL xstatus_has_priority) 
{
	DBVARIANT dbv={0};
	BOOL noAwayMsg=FALSE;
	text[0] = '\0';
	if (contact->status==0)
	{
		contact->status=GetContactCachedStatus(contact->hContact);
	}
	// Hide status text if Offline  /// no offline
	if (contact->status==ID_STATUS_OFFLINE || contact->status==0) noAwayMsg=TRUE;
	// Get XStatusMsg
	if (!noAwayMsg &&xstatus_has_priority && contact->hContact && contact->proto)
	{
		// Try to get XStatusMsg
		if (!DBGetContactSettingTString(contact->hContact, contact->proto, "XStatusMsg", &dbv)) 
		{
			//lstrcpyn(text, dbv.pszVal, text_size);
			CopySkipUnPrintableChars(text, dbv.ptszVal, text_size-1);
			DBFreeVariant(&dbv);

			if (text[0] != '\0')
				return -1;
		}
	}

	// Get StatusMsg
	if (contact->hContact && text[0] == '\0')
	{
		if (!DBGetContactSettingTString(contact->hContact, "CList", "StatusMsg", &dbv)) 
		{
			//lstrcpyn(text, dbv.pszVal, text_size);
			CopySkipUnPrintableChars(text, dbv.ptszVal, text_size-1);
			DBFreeVariant(&dbv);

			if (text[0] != '\0')
				return 1;
		}
	}

	// Get XStatusMsg
	if (!noAwayMsg && !xstatus_has_priority && contact->hContact && contact->proto && text[0] == '\0')
	{
		// Try to get XStatusMsg
		if (!DBGetContactSettingTString(contact->hContact, contact->proto, "XStatusMsg", &dbv)) 
		{
			//lstrcpyn(text, dbv.pszVal, text_size);
			CopySkipUnPrintableChars(text, dbv.ptszVal, text_size-1);
			DBFreeVariant(&dbv);

			if (text[0] != '\0')
				return -1;
		}
	}

	return 1;
}


/*
 *	Get the text for specified lines
 */
void Cache_GetLineText(struct ClcContact *contact, int type, LPTSTR text, int text_size, TCHAR *variable_text, BOOL xstatus_has_priority, BOOL show_status_if_no_away, BOOL use_name_and_message_for_xstatus, BOOL contact_time_show_only_if_different)
{
	text[0] = '\0';
	switch(type)
	{
	case TEXT_STATUS:
		{
			if (GetStatusName(text, text_size, contact, xstatus_has_priority) == -1 && use_name_and_message_for_xstatus)
			{
				DBVARIANT dbv={0};

				// Try to get XStatusMsg
				if (!DBGetContactSettingTString(contact->hContact, contact->proto, "XStatusMsg", &dbv)) 
				{
					if (dbv.ptszVal != NULL && dbv.ptszVal[0] != 0)
					{
						TCHAR *tmp = mir_strdupT(text);
						mir_sntprintf(text, text_size, TEXT("%s: %s"), tmp, dbv.pszVal);
						mir_free(tmp);
					}
					DBFreeVariant(&dbv);
				}
			}

			break;
		}
	case TEXT_NICKNAME:
		{
			if (contact->hContact && contact->proto)
			{
				DBVARIANT dbv={0};
				if (!DBGetContactSettingTString(contact->hContact, contact->proto, "Nick", &dbv)) 
				{
					lstrcpyn(text, dbv.ptszVal, text_size);
					DBFreeVariant(&dbv);
				}
			}
			break;
		}
	case TEXT_STATUS_MESSAGE:
		{
			if (GetStatusMessage(text, text_size, contact, xstatus_has_priority) == -1 && use_name_and_message_for_xstatus)
			{
				DBVARIANT dbv={0};

				// Try to get XStatusName
				if (!DBGetContactSettingTString(contact->hContact, contact->proto, "XStatusName", &dbv)) 
				{
					if (dbv.pszVal != NULL && dbv.pszVal[0] != 0)
					{
						TCHAR *tmp = mir_strdupT(text);
						mir_sntprintf(text, text_size, TEXT("%s: %s"), dbv.pszVal, tmp);
						mir_free(tmp);
					}
					DBFreeVariant(&dbv);
				}
			}
			else if (use_name_and_message_for_xstatus && xstatus_has_priority)
			{
				DBVARIANT dbv={0};
				// Try to get XStatusName
				if (!DBGetContactSettingTString(contact->hContact, contact->proto, "XStatusName", &dbv)) 
				{
					if (dbv.pszVal != NULL && dbv.pszVal[0] != 0)
						mir_sntprintf(text, text_size, TEXT("%s"), dbv.pszVal);
					DBFreeVariant(&dbv);
				}
			}

			break;
		}
	case TEXT_TEXT:
		{
#ifndef UNICODE	
			if (!ServiceExists(MS_VARS_FORMATSTRING))
			{
				lstrcpyn(text, variable_text, text_size);
			}
			else
			{
				char *tmp = variables_parse(variable_text, contact->szText, contact->hContact);
				lstrcpyn(text, tmp, text_size);
				variables_free(tmp);
			}
#else
			lstrcpyn(text, variable_text, text_size);
#endif
			break;
		}
	case TEXT_CONTACT_TIME:
		{
			if (contact->timezone != -1 && (!contact_time_show_only_if_different || contact->timediff != 0))
			{
				// Get contact time
				DBTIMETOSTRINGT dbtts;
				time_t contact_time;

				contact_time = time(NULL) - contact->timediff;
				text[0] = '\0';

				dbtts.szDest = text;
				dbtts.cbDest = 70;
				dbtts.szFormat = TEXT("t");
				CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, contact_time, (LPARAM) & dbtts);
			}

			break;
		}
	}
	if (type==TEXT_STATUS_MESSAGE && show_status_if_no_away && text[0] == '\0')
	{
		//re-request status if no away
		Cache_GetLineText(contact, TEXT_STATUS, text, text_size, variable_text, xstatus_has_priority,0, use_name_and_message_for_xstatus, contact_time_show_only_if_different);		
	}

}

/*
*	Get the text for First Line
*/
void Cache_GetFirstLineText(struct ClcData *dat, struct ClcContact *contact)
{
	lstrcpyn(contact->szText, pcli->pfnGetContactDisplayName(contact->hContact,0),SIZEOF(contact->szText));
	Cache_ReplaceSmileys(dat, contact, contact->szText, lstrlen(contact->szText)+1, &(contact->plText),
		&contact->iTextMaxSmileyHeight,dat->first_line_draw_smileys);
}

/*
*	Get the text for Second Line
*/
void Cache_GetSecondLineText(struct ClcData *dat, struct ClcContact *contact)
{
  TCHAR Text[120-MAXEXTRACOLUMNS]={0};
	Cache_GetLineText(contact, dat->second_line_type, (TCHAR*)Text, SIZEOF(Text), dat->second_line_text,
    dat->second_line_xstatus_has_priority,dat->second_line_show_status_if_no_away,
	dat->second_line_use_name_and_message_for_xstatus, dat->contact_time_show_only_if_different);
  if (contact->szSecondLineText) mir_free(contact->szSecondLineText);
  if (Text[0]!='\0')
    contact->szSecondLineText=mir_strdupT((TCHAR*)Text);
  else
    contact->szSecondLineText=NULL;
  Text[120-MAXEXTRACOLUMNS-1]='\0';
	Cache_ReplaceSmileys(dat, contact, contact->szSecondLineText, lstrlen(contact->szSecondLineText), &contact->plSecondLineText, 
    &contact->iSecondLineMaxSmileyHeight,dat->second_line_draw_smileys);

}

/*
*	Get the text for Third Line
*/
void Cache_GetThirdLineText(struct ClcData *dat, struct ClcContact *contact)
{
  TCHAR Text[120-MAXEXTRACOLUMNS]={0};
	Cache_GetLineText(contact, dat->third_line_type,(TCHAR*)Text, SIZEOF(Text), dat->third_line_text,
		dat->third_line_xstatus_has_priority,dat->third_line_show_status_if_no_away,
		dat->third_line_use_name_and_message_for_xstatus, dat->contact_time_show_only_if_different);
  if (contact->szThirdLineText) mir_free(contact->szThirdLineText);
  if (Text[0]!='\0')
    contact->szThirdLineText=mir_strdupT((TCHAR*)Text);
  else
    contact->szThirdLineText=NULL;
  Text[120-MAXEXTRACOLUMNS-1]='\0';
	Cache_ReplaceSmileys(dat, contact, contact->szThirdLineText, lstrlen(contact->szThirdLineText), &contact->plThirdLineText, 
		&contact->iThirdLineMaxSmileyHeight,dat->third_line_draw_smileys);
}



/*
*	Copy string with removing Escape chars from text
*/
int CopySkipUnPrintableChars(TCHAR *to, TCHAR * buf, DWORD size)
{
	DWORD i;
	BOOL keep=0;
	TCHAR * cp=to;
	for (i=0; i<size; i++)
	{
		if (buf[i]==0) break;
		if (buf[i]>0 && buf[i]<' ')
		{
			*cp=' ';
			if (!keep) cp++;
			keep=1;
		}
		else
		{     
			keep=0;
			*cp=buf[i];
			cp++;
		} 
	}
	*cp=0;
	return i;
}


/*
 *	Avatar working routines
 */
// If ExecuteOnAllContactsFuncPtr returns FALSE, stop loop
// Return TRUE if finished, FALSE if was stoped
BOOL ExecuteOnAllContacts(struct ClcData *dat, ExecuteOnAllContactsFuncPtr func, void *param)
{
	return ExecuteOnAllContactsOfGroup(&dat->list, func, param);
}

BOOL ExecuteOnAllContactsOfGroup(struct ClcGroup *group, ExecuteOnAllContactsFuncPtr func, void *param)
{
	int scanIndex, i;

	for(scanIndex = 0 ; scanIndex < group->cl.count ; scanIndex++)
	{
		if (group->cl.items[scanIndex]->type == CLCIT_CONTACT)
		{
			if (!func(group->cl.items[scanIndex], FALSE, param))
			{
				return FALSE;
			}

			if (group->cl.items[scanIndex]->SubAllocated > 0)
			{
				for (i = 0 ; i < group->cl.items[scanIndex]->SubAllocated ; i++)
				{
					if (!func(&group->cl.items[scanIndex]->subcontacts[i], TRUE, param))
					{
						return FALSE;
					}
				}
			}
		}
		else if (group->cl.items[scanIndex]->type == CLCIT_GROUP) 
		{
			if (!ExecuteOnAllContactsOfGroup(group->cl.items[scanIndex]->group, func, param))
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

BOOL UpdateAllAvatarsProxy(struct ClcContact *contact, BOOL subcontact, void *param)
{
	Cache_GetAvatar((struct ClcData *)param, contact);
	return TRUE;
}

void UpdateAllAvatars(struct ClcData *dat)
{
	ExecuteOnAllContacts(dat,UpdateAllAvatarsProxy,dat);
}

BOOL ReduceAvatarPosition(struct ClcContact *contact, BOOL subcontact, void *param)
{
	if (contact->avatar_pos >= *((int *)param))
	{
		contact->avatar_pos--;
	}

	return TRUE;
}


void Cache_GetAvatar(struct ClcData *dat, struct ClcContact *contact)
{
	if (dat->use_avatar_service)
	{
		if (dat->avatars_show && !DBGetContactSettingByte(contact->hContact, "CList", "HideContactAvatar", 0))
		{
			contact->avatar_data = (struct avatarCacheEntry *)CallService(MS_AV_GETAVATARBITMAP, (WPARAM)contact->hContact, 0);

			if (contact->avatar_data == NULL || contact->avatar_data->cbSize != sizeof(struct avatarCacheEntry) 
				|| contact->avatar_data->dwFlags == AVS_BITMAP_EXPIRED)
			{
				contact->avatar_data = NULL;
			}

			if (contact->avatar_data != NULL)
				contact->avatar_data->t_lastAccess = time(NULL);
		}
		else
		{
			contact->avatar_data = NULL;
		}
	}
	else
	{
		int old_pos = contact->avatar_pos;

		contact->avatar_pos = AVATAR_POS_DONT_HAVE;
		if (dat->avatars_show && !DBGetContactSettingByte(contact->hContact, "CList", "HideContactAvatar", 0))
		{
			DBVARIANT dbv={0};
			if (!DBGetContactSetting(contact->hContact, "ContactPhoto", "File", &dbv) && (dbv.type == DBVT_ASCIIZ || dbv.type == DBVT_UTF8))
			{
				HBITMAP hBmp = (HBITMAP) CallService(MS_UTILS_LOADBITMAP, 0, (LPARAM)dbv.pszVal);
				if (hBmp != NULL)
				{
					// Make bounds
					BITMAP bm;
					if (GetObject(hBmp,sizeof(BITMAP),&bm))
					{
						// Create data...
						HDC hdc; 
						HBITMAP hDrawBmp,oldBmp;

						// Make bounds -> keep aspect radio
						LONG width_clip;
						LONG height_clip;
						RECT rc = {0};

						// Clipping width and height
						width_clip = dat->avatars_size;
						height_clip = dat->avatars_size;

						if (height_clip * bm.bmWidth / bm.bmHeight <= width_clip)
						{
							width_clip = height_clip * bm.bmWidth / bm.bmHeight;
						}
						else
						{
							height_clip = width_clip * bm.bmHeight / bm.bmWidth;					
						}

						// Create objs
						hdc = CreateCompatibleDC(dat->avatar_cache.hdc); 
						hDrawBmp = CreateBitmap32(width_clip, height_clip);
						oldBmp=SelectObject(hdc, hDrawBmp);
						SetBkMode(hdc,TRANSPARENT);
						{
							POINT org;
							GetBrushOrgEx(hdc, &org);
							SetStretchBltMode(hdc, HALFTONE);
							SetBrushOrgEx(hdc, org.x, org.y, NULL);
						}

						rc.right = width_clip - 1;
						rc.bottom = height_clip - 1;

						// Draw bitmap             8//8
						{
							HDC dcMem = CreateCompatibleDC(hdc);
							HBITMAP obmp=SelectObject(dcMem, hBmp);						
							StretchBlt(hdc, 0, 0, width_clip, height_clip,dcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
							SelectObject(dcMem,obmp);
							DeleteDC(dcMem);
						}
            {
              RECT rtr={0};
              rtr.right=width_clip+1;
              rtr.bottom=height_clip+1;
              FillRect255Alpha(hdc,&rtr);
            }

            hDrawBmp = GetCurrentObject(hdc, OBJ_BITMAP);
			SelectObject(hdc,oldBmp);
            DeleteDC(hdc);

						// Add to list
						if (old_pos >= 0)
						{
							ImageArray_ChangeImage(&dat->avatar_cache, hDrawBmp, old_pos);
							contact->avatar_pos = old_pos;
						}
						else
						{
							contact->avatar_pos = ImageArray_AddImage(&dat->avatar_cache, hDrawBmp, -1);
						}

						DeleteObject(hDrawBmp);
					} // if (GetObject(hBmp,sizeof(BITMAP),&bm))
					DeleteObject(hBmp);
				} //if (hBmp != NULL)
			}
			DBFreeVariant(&dbv);
		}

		// Remove avatar if needed
		if (old_pos >= 0 && contact->avatar_pos == AVATAR_POS_DONT_HAVE)
		{
			ImageArray_RemoveImage(&dat->avatar_cache, old_pos);

			// Update all itens
			ExecuteOnAllContacts(dat, ReduceAvatarPosition, (void *)&old_pos);
		}
	}
}












