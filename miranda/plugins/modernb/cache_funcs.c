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

Created by Pescuma
Modified by FYR

*/

/************************************************************************/
/*             Module for working with lines text and avatars           */
/************************************************************************/

#include "commonheaders.h"
#include "cache_funcs.h"
#include "newpluginapi.h"


/*
 *	Module Prototypes
 */
int CopySkipUnPrintableChars(TCHAR *to, TCHAR * buf, DWORD size);
SortedList *CopySmileyString(SortedList *plInput);
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
    if (MirandaExiting()) return 0;
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
    if (MirandaExiting()) return 0;
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
  if (!h) 
  {
      hAskStatusMessageThread=NULL;
      return 0;
  }

  ISTREADSTARTED=1;
  while (h)
  { 
    time=GetTickCount();
    if ((time-RequestTick)<ASKPERIOD)
    {
            SleepEx(ASKPERIOD-(time-RequestTick)+10,TRUE);
            if (MirandaExiting()) 
            {
              ISTREADSTARTED=0;
              hAskStatusMessageThread=NULL;
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
    if (MirandaExiting()) 
    {
      hAskStatusMessageThread=NULL;
      ISTREADSTARTED=0;
      return 0; 
    }

  }
  ISTREADSTARTED=0;
  hAskStatusMessageThread=NULL;
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
    hAskStatusMessageThread=(HANDLE)forkthread(AskStatusMessageThread,0,0);
  }
  return;
}








/*
 *	Get time zone for contact
 */
void Cache_GetTimezone(struct ClcData *dat, HANDLE hContact)
{
	//if (IsBadWritePtr(contact,sizeof(struct ClcContact))) return;
	PDNCE pdnce=(PDNCE)pcli->pfnGetCacheEntry(hContact);
	if (dat==NULL && pcli->hwndContactTree) 
		dat=(struct ClcData *)GetWindowLong(pcli->hwndContactTree,0);
	if (!IsBadStringPtrA(pdnce->szProto,10))
		pdnce->timezone = (DWORD)DBGetContactSettingByte(hContact,"UserInfo","Timezone", 
									DBGetContactSettingByte(hContact, pdnce->szProto,"Timezone",-1));
	else pdnce->timezone =-1;
	pdnce->timediff = 0;

	if (pdnce->timezone != -1)
	{
		int pdnce_gmt_diff = pdnce->timezone;
		pdnce_gmt_diff = pdnce_gmt_diff > 128 ? 256 - pdnce_gmt_diff : 0 - pdnce_gmt_diff;
		pdnce_gmt_diff *= 60*60/2;

		// Only in case of same timezone, ignore DST
		if (pdnce_gmt_diff != (dat?dat->local_gmt_diff:0))
			pdnce->timediff = (int)(dat?dat->local_gmt_diff_dst:0) - pdnce_gmt_diff;
	}
}

/*
 *		Pooling 
 */

typedef struct _CacheAskChain {
	HANDLE ContactRequest;
	struct ClcData *dat;
	struct ClcContact *contact;
	struct CacheAskChain *Next;
} CacheAskChain;

CacheAskChain * FirstCacheChain=NULL;
CacheAskChain * LastCacheChain=NULL;
BOOL LockCacheChain=0;
BOOL ISCacheTREADSTARTED=0;

BOOL GetCacheChain(CacheAskChain * chain)
{
	while (LockCacheChain) 
	{
	    SleepEx(0,TRUE);
		if (MirandaExiting()) return FALSE;
	}
	if (!FirstCacheChain) return FALSE;
	else if (chain)
	{
		CacheAskChain * ch;
		ch=FirstCacheChain;
		*chain=*ch;
		FirstCacheChain=(CacheAskChain *)ch->Next;
		if (!FirstCacheChain) LastCacheChain=NULL;
		mir_free(ch);
		return TRUE;
	}
	return FALSE;
}

int GetTextThread(void * a)
{
	BOOL exit=FALSE;
	BOOL err=FALSE;
	do
	{
		SleepEx(0,TRUE); //1000 contacts per second
		if (MirandaExiting()) 
        {
            hGetTextThread=NULL;
			return 0;
        }
		else
		{
			CacheAskChain chain={0};
			struct ClcData *dat;
//			struct ClcContact * contact;
			if (!GetCacheChain(&chain)) break;
			if (chain.dat==NULL) chain.dat=(struct ClcData*)GetWindowLong(pcli->hwndContactTree,0);
			if (!IsBadReadPtr(chain.dat,sizeof(struct ClcData))) dat=chain.dat;
			else err=TRUE;
			if (!err)
			{
				{

					PDNCE cacheEntry=NULL;
					cacheEntry=(PDNCE)pcli->pfnGetCacheEntry(chain.ContactRequest);
					Cache_GetSecondLineText(dat, cacheEntry);
					Cache_GetThirdLineText(dat, cacheEntry);
				}
				KillTimer(dat->hWnd,TIMERID_INVALIDATE_FULL);
				SetTimer(dat->hWnd,TIMERID_INVALIDATE_FULL,500,NULL);
			}
			err=FALSE;
		}
	}
    while (!exit);
	ISCacheTREADSTARTED=FALSE;	
	TRACE("ALL Done\n");
    hGetTextThread=NULL;
	return 1;
}
int AddToCacheChain(struct ClcData *dat,struct ClcContact *contact,HANDLE ContactRequest)
{
	while (LockCacheChain) 
	{
	    SleepEx(0,TRUE);
		if (MirandaExiting()) return 0;
	}
	LockCacheChain=TRUE;
	{
		CacheAskChain * chain=(CacheAskChain *)mir_alloc(sizeof(CacheAskChain));
		chain->ContactRequest=ContactRequest;
		chain->dat=dat;
		chain->contact=contact;
		chain->Next=NULL;
		if (LastCacheChain) 
		{
			LastCacheChain->Next=(struct CacheAskChain *)chain;
			LastCacheChain=chain;
		}
		else 
		{
			FirstCacheChain=chain;
			LastCacheChain=chain;
			if (!ISCacheTREADSTARTED)
			{
				//StartThreadHere();
				hGetTextThread=(HANDLE)forkthread(GetTextThread,0,0);
				ISCacheTREADSTARTED=TRUE;
			}
		}
	}
	LockCacheChain=FALSE;
	return FALSE;
}

/*
 *	Get all lines of text
 */ 

//void Cache_GetNewTextFromPDNCE(struct ClcData *dat, struct ClcContact *contact, pdisplayNameCacheEntry cacheEntry)
//{
//
//}
void Cache_RenewText(HANDLE hContact)
{
	AddToCacheChain(NULL,NULL, hContact);
}

void Cache_GetText(struct ClcData *dat, struct ClcContact *contact, BOOL forceRenew)
{
	Cache_GetFirstLineText(dat, contact);
	if (!dat->force_in_dialog)// && !dat->isStarting)
		if (1)
		{
			PDNCE pdnce=(PDNCE)pcli->pfnGetCacheEntry(contact->hContact);
			if (  (dat->second_line_show&&(forceRenew||pdnce->szSecondLineText==NULL))
				||(dat->third_line_show&&(forceRenew||pdnce->szThirdLineText==NULL))  )
			{
				AddToCacheChain(dat,contact, contact->hContact);
			}
		}
		/*
		else 
			if (1)
				AddToCacheChain(dat,contact, contact->hContact);
			else
			{
				Cache_GetSecondLineText(dat, contact);
				Cache_GetThirdLineText(dat, contact);
			}
		*/
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
				if (piece->smiley && piece->smiley != listening_to_icon)
					DestroyIcon_protect(piece->smiley);

					mir_free(piece);
				}
			}
		}
		li.List_Destroy( p_list );
	}
	mir_free(p_list);
	
}

void Cache_AddListeningToIcon(struct ClcData *dat, PDNCE pdnce, TCHAR *text, int text_size, SortedList **plText, 
						 int *max_smiley_height, BOOL replace_smileys)
{
	*max_smiley_height = 0;

	if (!dat->text_replace_smileys || !replace_smileys || text == NULL)
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

	*plText = li.List_Create( 0, 2 );

	// Add Icon
	{
		BITMAP bm;
		ICONINFO icon;
		ClcContactTextPiece *piece = (ClcContactTextPiece *) mir_alloc(sizeof(ClcContactTextPiece));

		piece->type = TEXT_PIECE_TYPE_SMILEY;
		piece->len = 0;
		piece->smiley = listening_to_icon;

		piece->smiley_width = 16;
		piece->smiley_height = 16;
		if (GetIconInfo(piece->smiley, &icon))
		{
			if (GetObject(icon.hbmColor,sizeof(BITMAP),&bm))
			{
				piece->smiley_width = bm.bmWidth;
				piece->smiley_height = bm.bmHeight;
			}

			DeleteObject(icon.hbmMask);
			DeleteObject(icon.hbmColor);
		}

		dat->text_smiley_height = max(piece->smiley_height, dat->text_smiley_height);
		*max_smiley_height = max(piece->smiley_height, *max_smiley_height);

		li.List_Insert(*plText, piece, plText[0]->realCount);
	}

	// Add text
	{
		ClcContactTextPiece *piece = (ClcContactTextPiece *) mir_alloc(sizeof(ClcContactTextPiece));

		piece->type = TEXT_PIECE_TYPE_TEXT;
		piece->start_pos = 0;
		piece->len = text_size;
		li.List_Insert(*plText, piece, plText[0]->realCount);
	}
}

/*
* Parsing of text for smiley
*/
void Cache_ReplaceSmileys(struct ClcData *dat, PDNCE pdnce, TCHAR *text, int text_size, SortedList **plText, 
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
		sp.Protocolname = pdnce->szProto;

		if (DBGetContactSettingByte(NULL,"CLC","Meta",0) != 1 && pdnce->szProto != NULL && strcmp(pdnce->szProto, "MetaContacts") == 0)
		{
			HANDLE hContact = (HANDLE)CallService(MS_MC_GETMOSTONLINECONTACT, (UINT)pdnce->hContact, 0);
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
	
	if (ServiceExists(MS_SMILEYADD_PARSET))
		CallService(MS_SMILEYADD_PARSET, 0, (LPARAM)&sp);

	if (sp.size == 0)
	{
		// Did not find a simley
		return;
	}

	// Lets add smileys
	*plText = li.List_Create( 0, 1 );

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

			piece->smiley_width = 16;
			piece->smiley_height = 16;
			if (GetIconInfo(piece->smiley, &icon))
			{
				if (GetObject(icon.hbmColor,sizeof(BITMAP),&bm))
				{
					piece->smiley_width = bm.bmWidth;
					piece->smiley_height = bm.bmHeight;
				}

				DeleteObject(icon.hbmMask);
				DeleteObject(icon.hbmColor);
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
		if (ServiceExists(MS_SMILEYADD_PARSET))
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
int GetStatusName(TCHAR *text, int text_size, PDNCE pdnce, BOOL xstatus_has_priority) 
{
	BOOL noAwayMsg=FALSE;
	BOOL noXstatus=FALSE;
	// Hide status text if Offline  /// no offline		
	if ((pdnce->status==ID_STATUS_OFFLINE || pdnce->status==0) && DBGetContactSettingByte(NULL,"ModernData","RemoveAwayMessageForOffline",0)) noAwayMsg=TRUE;
	if (pdnce->status==ID_STATUS_OFFLINE || pdnce->status==0) noXstatus=TRUE;
	text[0] = '\0';
	// Get XStatusName
	if (!noAwayMsg&& !noXstatus&& xstatus_has_priority && pdnce->hContact && pdnce->szProto)
	{
		DBVARIANT dbv={0};
		if (!DBGetContactSettingTString(pdnce->hContact, pdnce->szProto, "XStatusName", &dbv)) 
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
		TCHAR *tmp = (TCHAR *)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION, (WPARAM)pdnce->status, GCMDF_TCHAR_MY);
		lstrcpyn(text, tmp, text_size);
		//CopySkipUnPrintableChars(text, dbv.pszVal, text_size-1);
		if (text[0] != '\0')
			return 1;
	}

	// Get XStatusName
	if (!noAwayMsg && !noXstatus && !xstatus_has_priority && pdnce->hContact && pdnce->szProto)
	{
		DBVARIANT dbv={0};
		if (!DBGetContactSettingTString(pdnce->hContact, pdnce->szProto, "XStatusName", &dbv)) 
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
 * Get Listening to information
 */
void GetListeningTo(TCHAR *text, int text_size,  PDNCE pdnce)
{
	DBVARIANT dbv={0};
	text[0] = _T('\0');

	if (pdnce->status==ID_STATUS_OFFLINE || pdnce->status==0)
		return;

	if (!DBGetContactSettingTString(pdnce->hContact, pdnce->szProto, "ListeningTo", &dbv)) 
	{
		CopySkipUnPrintableChars(text, dbv.ptszVal, text_size-1);
		DBFreeVariant(&dbv);
	}
}

/*
*	Getting Status message (Away message)
*  -1 for XStatus, 1 for Status
*/
int GetStatusMessage(TCHAR *text, int text_size,  PDNCE pdnce, BOOL xstatus_has_priority) 
{
	DBVARIANT dbv={0};
	BOOL noAwayMsg=FALSE;
	text[0] = '\0';
	// Hide status text if Offline  /// no offline
	if (pdnce->status==ID_STATUS_OFFLINE || pdnce->status==0) noAwayMsg=TRUE;
	// Get XStatusMsg
	if (!noAwayMsg &&xstatus_has_priority && pdnce->hContact && pdnce->szProto)
	{
		// Try to get XStatusMsg
		if (!DBGetContactSettingTString(pdnce->hContact, pdnce->szProto, "XStatusMsg", &dbv)) 
		{
			//lstrcpyn(text, dbv.pszVal, text_size);
			CopySkipUnPrintableChars(text, dbv.ptszVal, text_size-1);
			DBFreeVariant(&dbv);

			if (text[0] != '\0')
				return -1;
		}
	}

	// Get StatusMsg
	if (pdnce->hContact && text[0] == '\0')
	{
		if (!DBGetContactSettingTString(pdnce->hContact, "CList", "StatusMsg", &dbv)) 
		{
			//lstrcpyn(text, dbv.pszVal, text_size);
			CopySkipUnPrintableChars(text, dbv.ptszVal, text_size-1);
			DBFreeVariant(&dbv);

			if (text[0] != '\0')
				return 1;
		}
	}

	// Get XStatusMsg
	if (!noAwayMsg && !xstatus_has_priority && pdnce->hContact && pdnce->szProto && text[0] == '\0')
	{
		// Try to get XStatusMsg
		if (!DBGetContactSettingTString(pdnce->hContact, pdnce->szProto, "XStatusMsg", &dbv)) 
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
int Cache_GetLineText(PDNCE pdnce, int type, LPTSTR text, int text_size, TCHAR *variable_text, BOOL xstatus_has_priority, 
					  BOOL show_status_if_no_away, BOOL show_listening_if_no_away, BOOL use_name_and_message_for_xstatus, 
					  BOOL pdnce_time_show_only_if_different)
{
	text[0] = '\0';
	switch(type)
	{
	case TEXT_STATUS:
		{
			if (GetStatusName(text, text_size, pdnce, xstatus_has_priority) == -1 && use_name_and_message_for_xstatus)
			{
				DBVARIANT dbv={0};

				// Try to get XStatusMsg
				if (!DBGetContactSettingTString(pdnce->hContact, pdnce->szProto, "XStatusMsg", &dbv)) 
				{
					if (dbv.ptszVal != NULL && dbv.ptszVal[0] != 0)
					{
						TCHAR *tmp = mir_tstrdup(text);
						mir_sntprintf(text, text_size, TEXT("%s: %s"), tmp, dbv.pszVal);
						mir_free(tmp);
					}
					DBFreeVariant(&dbv);
				}
			}

			return TEXT_STATUS;
		}
	case TEXT_NICKNAME:
		{
			if (pdnce->hContact && pdnce->szProto)
			{
				DBVARIANT dbv={0};
				if (!DBGetContactSettingTString(pdnce->hContact, pdnce->szProto, "Nick", &dbv)) 
				{
					lstrcpyn(text, dbv.ptszVal, text_size);
					DBFreeVariant(&dbv);
				}
			}

			return TEXT_NICKNAME;
		}
	case TEXT_STATUS_MESSAGE:
		{
			if (GetStatusMessage(text, text_size, pdnce, xstatus_has_priority) == -1 && use_name_and_message_for_xstatus)
			{
				DBVARIANT dbv={0};

				// Try to get XStatusName
				if (!DBGetContactSettingTString(pdnce->hContact, pdnce->szProto, "XStatusName", &dbv)) 
				{
					if (dbv.pszVal != NULL && dbv.pszVal[0] != 0)
					{
						TCHAR *tmp = mir_tstrdup(text);
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
				if (!DBGetContactSettingTString(pdnce->hContact, pdnce->szProto, "XStatusName", &dbv)) 
				{
					if (dbv.pszVal != NULL && dbv.pszVal[0] != 0)
						mir_sntprintf(text, text_size, TEXT("%s"), dbv.pszVal);
					DBFreeVariant(&dbv);
				}
			}

			if (text[0] == '\0')
			{
				if (show_listening_if_no_away)
				{
					Cache_GetLineText(pdnce, TEXT_LISTENING_TO, text, text_size, variable_text, xstatus_has_priority, 0, 0, use_name_and_message_for_xstatus, pdnce_time_show_only_if_different);
					if (text[0] != '\0')
						return TEXT_LISTENING_TO;
				}

				if (show_status_if_no_away)
				{
					//re-request status if no away
					return Cache_GetLineText(pdnce, TEXT_STATUS, text, text_size, variable_text, xstatus_has_priority, 0, 0, use_name_and_message_for_xstatus, pdnce_time_show_only_if_different);		
				}
			}
			return TEXT_STATUS_MESSAGE;
		}
	case TEXT_LISTENING_TO:
		{
			GetListeningTo(text, text_size, pdnce);
			return TEXT_LISTENING_TO;
		}
	case TEXT_TEXT:
		{
			TCHAR *tmp = variables_parsedup(variable_text, pdnce->name, pdnce->hContact);
			lstrcpyn(text, tmp, text_size);
			if (tmp) free(tmp);

			return TEXT_TEXT;
		}
	case TEXT_CONTACT_TIME:
		{
			if (pdnce->timezone != -1 && (!pdnce_time_show_only_if_different || pdnce->timediff != 0))
			{
				// Get pdnce time
				DBTIMETOSTRINGT dbtts;
				time_t pdnce_time;

				pdnce_time = time(NULL) - pdnce->timediff;
				text[0] = '\0';

				dbtts.szDest = text;
				dbtts.cbDest = 70;
				dbtts.szFormat = TEXT("t");
				CallService(MS_DB_TIME_TIMESTAMPTOSTRINGT, (WPARAM)pdnce_time, (LPARAM) & dbtts);
			}

			return TEXT_CONTACT_TIME;
		}
	}

	return TEXT_EMPTY;
}

/*
*	Get the text for First Line
*/
void Cache_GetFirstLineText(struct ClcData *dat, struct ClcContact *contact)
{
	PDNCE pdnce=(PDNCE)pcli->pfnGetCacheEntry(contact->hContact);
	TCHAR *name = pcli->pfnGetContactDisplayName(contact->hContact,0);
	if (dat->first_line_append_nick) {
		DBVARIANT dbv = {0};
		if (!DBGetContactSettingTString(pdnce->hContact, pdnce->szProto, "Nick", &dbv)) 
		{
			TCHAR nick[SIZEOF(contact->szText)];
			lstrcpyn(nick, dbv.ptszVal, SIZEOF(contact->szText));
			DBFreeVariant(&dbv);
			
			if (_tcsnicmp(name, nick, lstrlen(name)) == 0) {
				// They are the same -> use the nick to keep the case
				lstrcpyn(contact->szText, nick, SIZEOF(contact->szText));
			} else if (_tcsnicmp(name, nick, lstrlen(nick)) == 0) {
				// They are the same -> use the nick to keep the case
				lstrcpyn(contact->szText, name, SIZEOF(contact->szText));
			} else {
				// Append then
				mir_sntprintf(contact->szText, SIZEOF(contact->szText), _T("%s - %s"), name, nick);
			}
		} else {
			lstrcpyn(contact->szText, name, SIZEOF(contact->szText));
		}
	} else {
		lstrcpyn(contact->szText, name, SIZEOF(contact->szText));
	}
	Cache_ReplaceSmileys(dat, pdnce, contact->szText, lstrlen(contact->szText)+1, &(contact->plText),
		&contact->iTextMaxSmileyHeight,dat->first_line_draw_smileys);
}

/*
*	Get the text for Second Line
*/
void Cache_GetSecondLineText(struct ClcData *dat, PDNCE pdnce)
{
  HANDLE hContact=pdnce->hContact;
  TCHAR Text[120-MAXEXTRACOLUMNS]={0};
  int type = TEXT_EMPTY;
  if (dat->second_line_show)	
	type = Cache_GetLineText(pdnce, dat->second_line_type, (TCHAR*)Text, SIZEOF(Text), dat->second_line_text,
		dat->second_line_xstatus_has_priority,dat->second_line_show_status_if_no_away,dat->second_line_show_listening_if_no_away,
		dat->second_line_use_name_and_message_for_xstatus, dat->contact_time_show_only_if_different);
 
  LockCacheItem(hContact, __FILE__,__LINE__);
  if (pdnce->szSecondLineText) mir_free(pdnce->szSecondLineText);
  if (dat->second_line_show)// Text[0]!='\0')
    pdnce->szSecondLineText=mir_tstrdup((TCHAR*)Text);
  else
    pdnce->szSecondLineText=NULL;
  Text[120-MAXEXTRACOLUMNS-1]='\0';
  if (pdnce->szSecondLineText) 
  {
    if (type == TEXT_LISTENING_TO && pdnce->szSecondLineText[0] != _T('\0'))
	{
      Cache_AddListeningToIcon(dat, pdnce, pdnce->szSecondLineText, lstrlen(pdnce->szSecondLineText), &pdnce->plSecondLineText, 
        &pdnce->iSecondLineMaxSmileyHeight,dat->second_line_draw_smileys);
	}
	else
	{
	  Cache_ReplaceSmileys(dat, pdnce, pdnce->szSecondLineText, lstrlen(pdnce->szSecondLineText), &pdnce->plSecondLineText, 
        &pdnce->iSecondLineMaxSmileyHeight,dat->second_line_draw_smileys);
	}
  }
  UnlockCacheItem(hContact);
}

/*
*	Get the text for Third Line
*/
void Cache_GetThirdLineText(struct ClcData *dat, PDNCE pdnce)
{
  TCHAR Text[120-MAXEXTRACOLUMNS]={0};
  HANDLE hContact=pdnce->hContact;
  int type = TEXT_EMPTY;
  if (dat->third_line_show)
	type = Cache_GetLineText(pdnce, dat->third_line_type,(TCHAR*)Text, SIZEOF(Text), dat->third_line_text,
		dat->third_line_xstatus_has_priority,dat->third_line_show_status_if_no_away,dat->third_line_show_listening_if_no_away,
		dat->third_line_use_name_and_message_for_xstatus, dat->contact_time_show_only_if_different);
  
  LockCacheItem(hContact, __FILE__,__LINE__);
  if (pdnce->szThirdLineText) mir_free(pdnce->szThirdLineText);
  if (dat->third_line_show)//Text[0]!='\0')
    pdnce->szThirdLineText=mir_tstrdup((TCHAR*)Text);
  else
    pdnce->szThirdLineText=NULL;
  Text[120-MAXEXTRACOLUMNS-1]='\0';
  if (pdnce->szThirdLineText) 
  {
    if (type == TEXT_LISTENING_TO && pdnce->szThirdLineText[0] != _T('\0'))
	{
      Cache_AddListeningToIcon(dat, pdnce, pdnce->szThirdLineText, lstrlen(pdnce->szThirdLineText), &pdnce->plThirdLineText, 
		&pdnce->iThirdLineMaxSmileyHeight,dat->third_line_draw_smileys);
	}
	else
	{
	  Cache_ReplaceSmileys(dat, pdnce, pdnce->szThirdLineText, lstrlen(pdnce->szThirdLineText), &pdnce->plThirdLineText, 
		&pdnce->iThirdLineMaxSmileyHeight,dat->third_line_draw_smileys);
	}
  }
  UnlockCacheItem(hContact);
}


void RemoveTag(TCHAR *to, TCHAR *tag)
{
	TCHAR * st=to;
	int len=_tcslen(tag);
	int lastsize=_tcslen(to);
	while (st=_tcsstr(st,tag)) 
	{
		lastsize-=len;
		memmove((void*)st,(void*)(st+len),(lastsize)*sizeof(TCHAR));
	}
}

/*
*	Copy string with removing Escape chars from text
*   And BBcodes
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
	{
		//remove bbcodes: [b] [i] [u] <b> <i> <u>
		RemoveTag(to,_T("[b]")); RemoveTag(to,_T("[/b]"));
		RemoveTag(to,_T("[u]")); RemoveTag(to,_T("[/u]"));
		RemoveTag(to,_T("[i]")); RemoveTag(to,_T("[/i]"));

		RemoveTag(to,_T("<b>")); RemoveTag(to,_T("</b>"));
		RemoveTag(to,_T("<u>")); RemoveTag(to,_T("</u>"));
		RemoveTag(to,_T("<i>")); RemoveTag(to,_T("</i>"));		

		RemoveTag(to,_T("[B]")); RemoveTag(to,_T("[/b]"));
		RemoveTag(to,_T("[U]")); RemoveTag(to,_T("[/u]"));
		RemoveTag(to,_T("[I]")); RemoveTag(to,_T("[/i]"));

		RemoveTag(to,_T("<B>")); RemoveTag(to,_T("</B>"));
		RemoveTag(to,_T("<U>")); RemoveTag(to,_T("</U>"));
		RemoveTag(to,_T("<I>")); RemoveTag(to,_T("</I>"));
	}
	return i;
}

typedef struct _CONTACTDATASTORED
{
	BOOL used;
	HANDLE hContact;
	TCHAR * szSecondLineText;
	TCHAR * szThirdLineText;
	SortedList *plSecondLineText;
	SortedList *plThirdLineText;
} CONTACTDATASTORED;

CONTACTDATASTORED * StoredContactsList=NULL;
static int ContactsStoredCount=0;

SortedList *CopySmileyString(SortedList *plInput)
{
	SortedList * plText;
	int i;
	if (!plInput || plInput->realCount==0) return NULL;
	plText=li.List_Create( 0,1 );
	for (i=0; i<plInput->realCount; i++)
	{
			ClcContactTextPiece *pieceFrom=plInput->items[i];
			if (pieceFrom!=NULL)
			{
				ClcContactTextPiece *piece = (ClcContactTextPiece *) mir_alloc(sizeof(ClcContactTextPiece));			
				*piece=*pieceFrom;
				if (pieceFrom->smiley)
					piece->smiley=CopyIcon(pieceFrom->smiley);
				li.List_Insert(plText, piece, plText->realCount);
			}
	}
	return plText;
}

BOOL StoreOneContactData(struct ClcContact *contact, BOOL subcontact, void *param)
{
	StoredContactsList=mir_realloc(StoredContactsList,sizeof(CONTACTDATASTORED)*(ContactsStoredCount+1));
	{
		CONTACTDATASTORED empty={0};
		StoredContactsList[ContactsStoredCount]=empty;
		StoredContactsList[ContactsStoredCount].hContact=contact->hContact;	
/*		if (contact->szSecondLineText)
			StoredContactsList[ContactsStoredCount].szSecondLineText=mir_tstrdup(pdnce->szSecondLineText);
		if (contact->szThirdLineText)
			StoredContactsList[ContactsStoredCount].szThirdLineText=mir_tstrdup(pdnce->szThirdLineText);		
		if (contact->plSecondLineText)
			StoredContactsList[ContactsStoredCount].plSecondLineText=CopySmileyString(contact->plSecondLineText);		
		if (contact->plThirdLineText)
			StoredContactsList[ContactsStoredCount].plThirdLineText=CopySmileyString(contact->plThirdLineText);
*/
	}
	ContactsStoredCount++;
	return 1;
}

BOOL RestoreOneContactData(struct ClcContact *contact, BOOL subcontact, void *param)
{
	int i;
	for (i=0; i<ContactsStoredCount; i++)
	{
		if (StoredContactsList[i].hContact==contact->hContact)
		{
//			CONTACTDATASTORED data=StoredContactsList[i];
			memmove(StoredContactsList+i,StoredContactsList+i+1,sizeof(CONTACTDATASTORED)*(ContactsStoredCount-i-1));
			ContactsStoredCount--;
/*	
			{
				if (data.szSecondLineText)
					if (!pdnce->szSecondLineText)
						pdnce->szSecondLineText=data.szSecondLineText;
					else 
						mir_free(data.szSecondLineText);
				if (data.szThirdLineText)
					if (!pdnce->szThirdLineText)
						pdnce->szThirdLineText=data.szThirdLineText;
					else 
						mir_free(data.szThirdLineText);
				if (data.plSecondLineText)
					if (!contact->plSecondLineText)
						contact->plSecondLineText=data.plSecondLineText;
					else
						Cache_DestroySmileyList(data.plSecondLineText);
				if (data.plThirdLineText)
					if (!contact->plThirdLineText)
						contact->plThirdLineText=data.plThirdLineText;
					else
						Cache_DestroySmileyList(data.plThirdLineText);
			}
*/
			break;
		}
	}
	return 1;
}

int StoreAllContactData(struct ClcData *dat)
{
	return 0;
	ExecuteOnAllContacts(dat,StoreOneContactData,NULL);
	return 1;
}

int RestoreAllContactData(struct ClcData *dat)
{
	int i;
	return 0;
	ExecuteOnAllContacts(dat,RestoreOneContactData,NULL);
	for (i=0; i<ContactsStoredCount; i++)
	{
		if (StoredContactsList[i].szSecondLineText)
			mir_free(StoredContactsList[i].szSecondLineText);
		if (StoredContactsList[i].szThirdLineText)
			mir_free(StoredContactsList[i].szThirdLineText);
		if (StoredContactsList[i].plSecondLineText)
			Cache_DestroySmileyList(StoredContactsList[i].plSecondLineText);
		if (StoredContactsList[i].plThirdLineText)
			Cache_DestroySmileyList(StoredContactsList[i].plThirdLineText);
	}
	if (StoredContactsList) mir_free(StoredContactsList);
	StoredContactsList=NULL;
	ContactsStoredCount=0;
	return 1;
}

// If ExecuteOnAllContactsFuncPtr returns FALSE, stop loop
// Return TRUE if finished, FALSE if was stoped
BOOL ExecuteOnAllContacts(struct ClcData *dat, ExecuteOnAllContactsFuncPtr func, void *param)
{
	BOOL res;
	lockdat;
	res=ExecuteOnAllContactsOfGroup(&dat->list, func, param);
	ulockdat;
	return res;
}

BOOL ExecuteOnAllContactsOfGroup(struct ClcGroup *group, ExecuteOnAllContactsFuncPtr func, void *param)
{
	int scanIndex, i;
	if (group)
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


/*
 *	Avatar working routines
 */
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
            if (contact->avatar_data==(struct avatarCacheEntry *)0x80000000) 
                return;
			if (contact->avatar_data == NULL || contact->avatar_data->cbSize != sizeof(struct avatarCacheEntry) 
				|| contact->avatar_data->dwFlags == AVS_BITMAP_EXPIRED)
			{
				contact->avatar_data = NULL;
			}

			if (contact->avatar_data != NULL)
				contact->avatar_data->t_lastAccess = (DWORD)time(NULL);
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
						width_clip = dat->avatars_maxheight_size;
						height_clip = dat->avatars_maxheight_size;

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
							mod_DeleteDC(dcMem);
						}
            {
              RECT rtr={0};
              rtr.right=width_clip+1;
              rtr.bottom=height_clip+1;
              SetRectAlpha_255(hdc,&rtr);
            }

            hDrawBmp = GetCurrentObject(hdc, OBJ_BITMAP);
			      SelectObject(hdc,oldBmp);
            mod_DeleteDC(hdc);

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












