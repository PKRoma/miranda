/*
Chat module plugin for Miranda IM

Copyright (C) 2003 Jörgen Persson

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
#include "chat.h"

extern char*	pszActiveWndID ;
extern char*	pszActiveWndModule ;

#define WINDOWS_COMMANDS_MAX 30

typedef struct COMMAND_INFO_TYPE
{
	char *lpCommand;
	struct COMMAND_INFO_TYPE *last, *next;
} COMMAND_INFO;


typedef struct WINDOW_INFO_TYPE
{
	HWND hWnd;
	char* pszID;
	char* pszModule;
	struct WINDOW_INFO_TYPE *next;
	WORD wCommandsNum;
	COMMAND_INFO *lpCommands; 
	COMMAND_INFO *lpCurrentCommand; 
} WINDOW_INFO;

WINDOW_INFO *m_WndList = 0;


void SetActiveChatWindow(char * pszID, char * pszModule)
{
	if(WM_FindWindow(pszID, pszModule))
	{
		pszActiveWndID = realloc(pszActiveWndID, lstrlen(pszID)+1);
		lstrcpyn(pszActiveWndID, pszID, lstrlen(pszID)+1);
		pszActiveWndModule = realloc(pszActiveWndModule, lstrlen(pszModule)+1);
		lstrcpyn(pszActiveWndModule, pszModule, lstrlen(pszModule)+1);
	}

}
HWND GetActiveChatWindow(void)
{
	HWND hwnd = WM_FindWindow(pszActiveWndID, pszActiveWndModule);
	if(hwnd)
		return hwnd;
	if (m_WndList)
		return m_WndList->hWnd;
	return NULL;
}


//---------------------------------------------------
//		Window Manager functions
//
//		Necessary to keep track of all windows
//---------------------------------------------------




BOOL WM_AddWindow(HWND hWnd, char * pszID, char * pszModule)
{
	if (!WM_FindWindow(pszID, pszModule))
	{
		WINDOW_INFO *node = (WINDOW_INFO*) malloc(sizeof(WINDOW_INFO));
		node->hWnd = hWnd;
		node->pszID = (char*) malloc(lstrlen(pszID) + 1); 
		node->pszModule = (char*)malloc(lstrlen(pszModule) + 1); 
		lstrcpy(node->pszModule, pszModule);
		lstrcpy(node->pszID, pszID);

		node->lpCommands = NULL;
		node->lpCurrentCommand = NULL;
		node->wCommandsNum = 0;

		if (m_WndList == NULL) // list is empty
		{
			m_WndList = node;
			node->next = NULL;
		}
		else
		{
			node->next = m_WndList;
			m_WndList = node;
		}
		return TRUE;
	}
	return FALSE;
}

BOOL WM_RemoveWindow(char *pszID, char * pszModule)
{
	WINDOW_INFO *pTemp = m_WndList, *pLast = NULL;
	while (pTemp != NULL)
	{
		if (lstrcmpi(pTemp->pszID,pszID) == 0 && lstrcmpi(pTemp->pszModule,pszModule) == 0) // match
		{
			COMMAND_INFO *pCurComm;

			if (pLast == NULL) 
				m_WndList = pTemp->next;
			else
				pLast->next = pTemp->next;
			free(pTemp->pszID);
			free(pTemp->pszModule);
			
			// delete commands
			pCurComm = pTemp->lpCommands;
			while (pCurComm != NULL)
			{
				COMMAND_INFO *pNext = pCurComm->next;
				free(pCurComm->lpCommand);
				free(pCurComm);
				pCurComm = pNext;
			}

			free(pTemp);
			return TRUE;
		}
		pLast = pTemp;
		pTemp = pTemp->next;
	}
	return FALSE;
}

HWND WM_FindWindow(char *pszID, char * pszModule)
{
	WINDOW_INFO *pTemp = m_WndList, *pLast = NULL;
	while (pTemp != NULL)
	{
		if (lstrcmpi(pTemp->pszID,pszID) == 0 && lstrcmpi(pTemp->pszModule,pszModule) == 0)
		{
			return pTemp->hWnd;
		}
		pLast = pTemp;
		pTemp = pTemp->next;
	}
	return 0;
}

LRESULT WM_SendMessage(char *pszID, char * pszModule, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WINDOW_INFO *pTemp = m_WndList, *pLast = NULL;

	if (!pszID)
	{
		HWND hwnd = GetActiveChatWindow();
		if(hwnd)
			return SendMessage(hwnd, msg, wParam, lParam);
	}
	while (pTemp != NULL)
	{
		if (lstrcmpi(pTemp->pszID,pszID) == 0 && lstrcmpi(pTemp->pszModule,pszModule) == 0)
		{
			return SendMessage(pTemp->hWnd, msg, wParam, lParam);
		}
		pLast = pTemp;
		pTemp = pTemp->next;
	}

	return 0;
}

BOOL WM_PostMessage(char *pszID, char * pszModule, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WINDOW_INFO *pTemp = m_WndList, *pLast = NULL;
	while (pTemp != NULL)
	{
		if (lstrcmpi(pTemp->pszID,pszID) == 0 && lstrcmpi(pTemp->pszModule,pszModule) == 0)
		{
			return PostMessage(pTemp->hWnd, msg, wParam, lParam);
		}
		pLast = pTemp;
		pTemp = pTemp->next;
	}
	return FALSE;
}

BOOL WM_BroadcastMessage(char * pszModule, UINT msg, WPARAM wParam, LPARAM lParam, BOOL bAsync)
{
	WINDOW_INFO *pTemp = m_WndList, *pLast = NULL;
	while (pTemp != NULL)
	{
		if(!pszModule || !lstrcmpi(pTemp->pszModule,pszModule))
		{
			if (bAsync) 
				PostMessage(pTemp->hWnd, msg, wParam, lParam);
			else 
				SendMessage(pTemp->hWnd, msg, wParam, lParam);

		}
		pLast = pTemp;
		pTemp = pTemp->next;
	}
	return TRUE;
}

BOOL WM_RemoveAll (void)
{
	while (m_WndList != NULL) 
    {
		WINDOW_INFO *pLast = m_WndList->next;
		HWND hWnd = m_WndList->hWnd;
		free (m_WndList->pszID);
		free (m_WndList->pszModule);

		while (m_WndList->lpCommands != NULL)
		{
			COMMAND_INFO *pNext = m_WndList->lpCommands->next;
			free(m_WndList->lpCommands->lpCommand);
			free(m_WndList->lpCommands);
			m_WndList->lpCommands = pNext;
		}

		free (m_WndList);
		m_WndList = pLast;
		SendMessage(hWnd, GC_CLOSEWINDOW, 0, 0);
	}
	m_WndList = NULL;
	return TRUE;
}



void WM_AddCommand(char *pszID, char * pszModule, const char *lpNewCommand) 
{
	WINDOW_INFO *pTemp = m_WndList;
	while (pTemp != NULL)
	{
		if (lstrcmpi(pTemp->pszID,pszID) == 0 && lstrcmpi(pTemp->pszModule,pszModule) == 0) // match
		{
			COMMAND_INFO *node = malloc(sizeof(COMMAND_INFO));
			node->lpCommand = malloc(lstrlen(lpNewCommand) + 1);
			lstrcpy(node->lpCommand,lpNewCommand);
			node->last = NULL; // always added at beginning!

			// new commands are added at start
			if (pTemp->lpCommands == NULL)
			{
				node->next = NULL;
				pTemp->lpCommands = node;
			}
			else
			{
				node->next = pTemp->lpCommands;
				pTemp->lpCommands->last = node; // hmm, weird
				pTemp->lpCommands = node;
			}
			pTemp->lpCurrentCommand = NULL; // current command
			pTemp->wCommandsNum++;

			if (pTemp->wCommandsNum > WINDOWS_COMMANDS_MAX) 
			{
				COMMAND_INFO *pCurComm = pTemp->lpCommands;
				COMMAND_INFO *pLast;
				while (pCurComm->next != NULL) { pCurComm = pCurComm->next; }
				pLast = pCurComm->last;
				free(pCurComm->lpCommand);
				free(pCurComm);
				pLast->next = NULL;
				// done
				pTemp->wCommandsNum--;
			}
		}
		pTemp = pTemp->next;
	}
}


char *WM_GetPrevCommand(char *pszID, char * pszModule) // get previous command. returns NULL if previous command does not exist. current command remains as it was.
{
	WINDOW_INFO *pTemp = m_WndList;
	while (pTemp != NULL)
	{
		if (lstrcmpi(pTemp->pszID,pszID) == 0 && lstrcmpi(pTemp->pszModule,pszModule) == 0) // match
		{
			COMMAND_INFO *pPrevCmd = NULL;
			if (pTemp->lpCurrentCommand != NULL)
			{
				if (pTemp->lpCurrentCommand->next != NULL) // not NULL
				{
					pPrevCmd = pTemp->lpCurrentCommand->next; // next command (newest at beginning)
				}
				else
				{
					pPrevCmd = pTemp->lpCurrentCommand;
				}
			}
			else
			{
				pPrevCmd = pTemp->lpCommands;
			}

			pTemp->lpCurrentCommand = pPrevCmd; // make it the new command

			return(((pPrevCmd) ? (pPrevCmd->lpCommand) : (NULL)));
		}
		pTemp = pTemp->next;
	}
	return(NULL);
}


char *WM_GetNextCommand(char *pszID, char * pszModule) // get next command. returns NULL if next command does not exist. current command becomes NULL (a prev command after this one will get you the last command)
{
	WINDOW_INFO *pTemp = m_WndList;
	while (pTemp != NULL)
	{
		if (lstrcmpi(pTemp->pszID,pszID) == 0 && lstrcmpi(pTemp->pszModule,pszModule) == 0) // match
		{
			COMMAND_INFO *pNextCmd = NULL;
			if (pTemp->lpCurrentCommand != NULL)
			{
				pNextCmd = pTemp->lpCurrentCommand->last; // last command (newest at beginning)
			}

			pTemp->lpCurrentCommand = pNextCmd; // make it the new command

			return(((pNextCmd) ? (pNextCmd->lpCommand) : (NULL)));
		}
		pTemp = pTemp->next;
	}
	return(NULL);
}







//---------------------------------------------------
//		Module Manager functions
//
//		Necessary to keep track of all modules
//		that has registered with the plugin
//---------------------------------------------------

MODULEINFO *m_ModList = 0;

BOOL MM_AddModule(MODULE* info)
{
	if (!MM_FindModule(info->pszModule))
	{
		MODULEINFO *node = (MODULEINFO*) malloc(sizeof(MODULEINFO));
		ZeroMemory(node, sizeof(MODULEINFO));
		node->data.bBold = info->bBold;	
		node->data.bUnderline = info->bUnderline;	
		node->data.bItalics = info->bItalics;	
		node->data.bColor = info->bColor;	
		node->data.bBkgColor = info->bBkgColor;
		node->data.bAckMsg = info->bAckMsg;
		node->data.bChanMgr = info->bChanMgr;
		node->data.iMaxText= info->iMaxText;
		node->data.nColorCount = info->nColorCount;
		if(info->nColorCount > 0)
		{
			node->data.crColors = malloc(sizeof(COLORREF) * info->nColorCount);
			memcpy(node->data.crColors, info->crColors, sizeof(COLORREF) * info->nColorCount);
		}
		node->data.pszModule = (char*)malloc(lstrlen(info->pszModule) + 1); 
		lstrcpy(node->data.pszModule, info->pszModule);
		node->data.pszModDispName = (char*)malloc(lstrlen(info->pszModDispName) + 1); 
		lstrcpy(node->data.pszModDispName, info->pszModDispName);
		node->data.hOnlineIcon= CopyIcon(LoadSkinnedProtoIcon(info->pszModule, ID_STATUS_ONLINE)); 
		node->data.hOfflineIcon= CopyIcon(LoadSkinnedProtoIcon(info->pszModule, ID_STATUS_OFFLINE)); 
		if (m_ModList == NULL) // list is empty
		{
			m_ModList = node;
			node->next = NULL;
		}
		else
		{
			node->next = m_ModList;
			m_ModList = node;
		}
		return TRUE;
	}
	return FALSE;
}

MODULE* MM_FindModule(char* pszModule)
{
	MODULEINFO *pTemp = m_ModList, *pLast = NULL;
	while (pTemp != NULL)
	{
		if (lstrcmpi(pTemp->data.pszModule,pszModule) == 0)
		{
			return &pTemp->data;
		}
		pLast = pTemp;
		pTemp = pTemp->next;
	}
	return 0;
}

// stupid thing.. 
void MM_FixColors()
{
	MODULEINFO *pTemp = m_ModList;

	while (pTemp != NULL)
	{
		CheckColorsInModule(pTemp->data.pszModule);
		pTemp = pTemp->next;
	}
	return;
}

BOOL MM_RemoveAll (void)
{
	while (m_ModList != NULL) 
    {
		MODULEINFO *pLast = m_ModList->next;
		free (m_ModList->data.pszModule);
		free(m_ModList->data.pszModDispName);
		if(m_ModList->data.crColors)
			free (m_ModList->data.crColors);
		if(m_ModList->data.hOnlineIcon)
		DestroyIcon(m_ModList->data.hOnlineIcon);
		if(m_ModList->data.hOfflineIcon)
		DestroyIcon(m_ModList->data.hOfflineIcon);
		free (m_ModList);
		m_ModList = pLast;
    }
	m_ModList = NULL;
	return TRUE;
}

//---------------------------------------------------
//		Status manager functions
//
//		Necessary to keep track of what user statuses
//		per window nicklist that is available
//---------------------------------------------------

BOOL SM_AddStatus(STATUSINFO** ppStatusList, char * pszStatus, HTREEITEM hItem)
{
	if (!SM_FindTVGroup(*ppStatusList, pszStatus))
	{
		STATUSINFO *node = (STATUSINFO*) malloc(sizeof(STATUSINFO));
		node->hItem= hItem;
		node->pszGroup = (char*)malloc(lstrlen(pszStatus) + 1); 
		lstrcpy(node->pszGroup, pszStatus);
		if (*ppStatusList == NULL) // list is empty
		{
			node->Status = 1;
			*ppStatusList = node;
			node->next = NULL;
		}
		else
		{
			node->Status = ppStatusList[0]->Status*2;
			node->next = *ppStatusList;
			*ppStatusList = node;
		}
		return TRUE;
		
	}
	return FALSE;
}

HTREEITEM SM_FindTVGroup(STATUSINFO* pStatusList, char* pszStatus)
{
	STATUSINFO *pTemp = pStatusList, *pLast = NULL;
	while (pTemp != NULL)
	{
		if (lstrcmpi(pTemp->pszGroup,pszStatus) == 0)
		{
			return pTemp->hItem;
		}
		pLast = pTemp;
		pTemp = pTemp->next;
	}
	return 0;
}

WORD SM_StringToWord(STATUSINFO* pStatusList, char* pszStatus)
{
	STATUSINFO *pTemp = pStatusList, *pLast = NULL;
	while (pTemp != NULL)
	{
		if (lstrcmpi(pTemp->pszGroup,pszStatus) == 0)
		{
			return pTemp->Status;
		}
		if (pTemp->next == NULL)
			return pStatusList->Status;
		pLast = pTemp;
		pTemp = pTemp->next;
	}
	return 0;
}

char * SM_WordToString(STATUSINFO* pStatusList, WORD Status)
{
	STATUSINFO *pTemp = pStatusList, *pLast = NULL;
	while (pTemp != NULL)
	{
		if (pTemp->Status&Status)
		{
			Status -= pTemp->Status;
			if (Status == 0)
			{
				return pTemp->pszGroup;
			}
		}
		pLast = pTemp;
		pTemp = pTemp->next;
	}
	return 0;
}

BOOL SM_RemoveAll (STATUSINFO** ppStatusList)
{
	while (*ppStatusList != NULL) 
    {
		STATUSINFO *pLast = ppStatusList[0]->next;
		free (ppStatusList[0]->pszGroup);
		free (*ppStatusList);
		*ppStatusList = pLast;
    }
	*ppStatusList = NULL;
	return TRUE;
}

//---------------------------------------------------
//		User manager functions
//
//		Necessary to keep track of the users
//		in a window nicklist 
//---------------------------------------------------

USERINFO * UM_AddUser(STATUSINFO* pStatusList, USERINFO** ppUserList, char * pszNick, char * pszUID, char* pszStatus, HTREEITEM hItem)
{
	if (!UM_FindUser(*ppUserList, pszUID))
	{
		USERINFO *node = (USERINFO*) malloc(sizeof(USERINFO));
		node->hItem= hItem;
		node->pszNick = (char*)malloc(lstrlen(pszNick) + 1); 
		lstrcpy(node->pszNick, pszNick);
		node->pszUID = (char*)malloc(lstrlen(pszUID) + 1); 
		lstrcpy(node->pszUID, pszUID);
		node->Status = SM_StringToWord(pStatusList, pszStatus);
		if (*ppUserList == NULL) // list is empty
		{
			*ppUserList = node;
			node->next = NULL;
		}
		else
		{
			node->next = *ppUserList;
			*ppUserList = node;
		}
		return node;
		
	}
	return NULL;
}

USERINFO* UM_FindUser(USERINFO* pUserList, char* pszUID)
{
	USERINFO *pTemp = pUserList, *pLast = NULL;
	while (pTemp != NULL)
	{
		if (lstrcmpi(pTemp->pszUID,pszUID) == 0)
		{
			return pTemp;
		}
		pLast = pTemp;
		pTemp = pTemp->next;
	}
	return 0;
}

char* UM_FindUserAutoComplete(USERINFO* pUserList, char * pszOriginal, char* pszCurrent)
{
	char * pszName = NULL;
	USERINFO *pTemp = pUserList, *pLast = NULL;

	while (pTemp != NULL)
	{
		if (my_strstri(pTemp->pszNick,pszOriginal) == pTemp->pszNick)
		{
			if(lstrcmpi(pTemp->pszNick, pszCurrent) > 0 && (!pszName || lstrcmpi(pTemp->pszNick, pszName) < 0) )
				pszName =pTemp->pszNick;
		}
		pLast = pTemp;
		pTemp = pTemp->next;
	}
	return pszName;
}
BOOL UM_RemoveUser(USERINFO** ppUserList, char* pszUID)
{
	USERINFO *pTemp = *ppUserList, *pLast = NULL;
	while (pTemp != NULL)
	{
		if (lstrcmpi(pTemp->pszUID,pszUID) == 0 ) 
		{
			if (pLast == NULL) 
				*ppUserList = pTemp->next;
			else
				pLast->next = pTemp->next;
			free(pTemp->pszNick);
			free(pTemp->pszUID);
			free(pTemp);
			return TRUE;
		}
		pLast = pTemp;
		pTemp = pTemp->next;
	}
	return FALSE;
}

BOOL UM_RemoveAll (USERINFO** ppUserList)
{
	while (*ppUserList != NULL) 
    {
		USERINFO *pLast = ppUserList[0]->next;
		free (ppUserList[0]->pszUID);
		free (ppUserList[0]->pszNick);
		free (*ppUserList);
		*ppUserList = pLast;
    }
	*ppUserList = NULL;
	return TRUE;
}

//---------------------------------------------------
//		Log manager functions
//
//		Necessary to keep track of events
//		in a window log 
//---------------------------------------------------

BOOL LM_AddEvent(LOGINFO** ppLogListStart, LOGINFO** ppLogListEnd, char * pszNick, int iType, char * pszText, char* pszStatus, char * pszUserInfo, BOOL bIsMe, BOOL bHighlight, time_t time, int* piCount, int iLimit)
{
	LOGINFO *node = (LOGINFO*) malloc(sizeof(LOGINFO));
	node->iType = iType-(WM_USER+500);
	if(pszNick )
	{
		node->pszNick = (char*)malloc(lstrlen(pszNick) + 1); 
		lstrcpy(node->pszNick, pszNick);
	}
	else
		node->pszNick = NULL;
	if(pszText)
	{
		node->pszText = (char*)malloc(lstrlen(pszText) + 1); 
		lstrcpy(node->pszText, pszText);
	}
	else
		node->pszText = NULL;
	if(pszStatus)
	{
		node->pszStatus = (char*)malloc(lstrlen(pszStatus) + 1); 
		lstrcpy(node->pszStatus, pszStatus);
	}
	else
		node->pszStatus = NULL;
	if(pszUserInfo)
	{
		node->pszUserInfo = (char*)malloc(lstrlen(pszUserInfo) + 1); 
		lstrcpy(node->pszUserInfo, pszUserInfo);
	}
	else
		node->pszUserInfo = NULL;

	node->bIsMe = bIsMe;
	node->bIsHighlighted = bHighlight;
	node->time = time;

	if (*ppLogListStart == NULL) // list is empty
	{
		*ppLogListStart = node;
		*ppLogListEnd = node;
		node->next = NULL;
		node->prev = NULL;
	}
	else
	{
		ppLogListStart[0]->prev = node;
		node->next = *ppLogListStart;
		*ppLogListStart = node;
		ppLogListStart[0]->prev=NULL;
	}
	*piCount += 1;
	if (iLimit > 0 && *piCount > iLimit + 20)
	{
		LM_TrimLog(ppLogListStart, ppLogListEnd, *piCount - iLimit);
		*piCount = iLimit;
		return FALSE;
	}
	return TRUE;
}

BOOL LM_TrimLog(LOGINFO** ppLogListStart, LOGINFO** ppLogListEnd, int iCount)
{
	LOGINFO *pTemp = *ppLogListEnd;
	while (pTemp != NULL && iCount > 0)
	{
		*ppLogListEnd = pTemp->prev;
		if (*ppLogListEnd == NULL) 
			*ppLogListStart = NULL;

		if(pTemp->pszNick)
			free(pTemp->pszNick);
		if(pTemp->pszUserInfo)
			free(pTemp->pszUserInfo);
		if(pTemp->pszText)
			free(pTemp->pszText);
		if(pTemp->pszStatus)
			free(pTemp->pszStatus);
		if(pTemp)
			free(pTemp);
		pTemp = *ppLogListEnd;
		iCount--;
	}
	ppLogListEnd[0]->next = NULL;

	return TRUE;
}

BOOL LM_RemoveAll (LOGINFO** ppLogListStart, LOGINFO** ppLogListEnd)
{
	while (*ppLogListStart != NULL) 
    {
		LOGINFO *pLast = ppLogListStart[0]->next;
		if(ppLogListStart[0]->pszText)
			free (ppLogListStart[0]->pszText);
		if(ppLogListStart[0]->pszNick)
			free (ppLogListStart[0]->pszNick);
		if(ppLogListStart[0]->pszStatus)
			free (ppLogListStart[0]->pszStatus);
		if(ppLogListStart[0]->pszUserInfo)
			free (ppLogListStart[0]->pszUserInfo);
		if(*ppLogListStart)
			free (*ppLogListStart);
		*ppLogListStart = pLast;
    }
	*ppLogListStart = NULL;
	*ppLogListEnd = NULL;
	return TRUE;
}

