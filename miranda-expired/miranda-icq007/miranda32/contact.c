#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "miranda.h"
#include "internal.h"
#include "global.h"
#include "contact.h"


#include "MSN_global.h"


void DisplayGroup(CNT_GROUP *grp);


//CNT_GROUP groups[MAX_GROUPS];
CNT_GROUP *groups;
int grp_cnt;

char *inttotxt(int d)
{
	char txt[64];
	sprintf(txt, "%d", d);
	return _strdup(txt);
}

void LoadGroups(void)
{
	FILE *fp;
	int sz;
	int i;

	grp_cnt = 0;

	fp = fopen(opts.grouplist, "rb");
	if (fp)
	{
		while (fread(&sz, sizeof(int), 1, fp))
		{
			
			IncreaseGroupArray();

			memset(&groups[grp_cnt-1], 0, sizeof(CNT_GROUP));
			fseek(fp, -(int)sizeof(int),	SEEK_CUR);
			

			fread(&groups[grp_cnt-1], sizeof(CNT_GROUP), 1, fp);

			groups[grp_cnt-1].structsize = sizeof(CNT_GROUP);
			
			groups[grp_cnt-1].item = NULL;
			
		}
		fclose(fp);	
	}
	if (grp_cnt == 0)
	{
		IncreaseGroupArray();
		groups[0].grp = 0;
		groups[0].item = TVI_ROOT;
		strcpy(groups[0].name, "main");
		groups[0].parent = -1;
	}

	for (i = 0; i < grp_cnt; i++)
	{
		DisplayGroup(&groups[i]);
	}
}

void SaveGroups(void)
{
	FILE *fp;
	fp = fopen(opts.grouplist, "wb");
	if (fp)
	{
		int i;
		for (i = 0; i < grp_cnt; i++)
		{
			fwrite(&groups[i], sizeof(CNT_GROUP), 1, fp);
		}
		fclose(fp);
	}
	
}

void DisplayGroup(CNT_GROUP *grp)
{
	TVINSERTSTRUCT is;
	
	if (grp->grp == 0 || rto.hwndContact == NULL) return;

	is.hParent = groups[grp->parent].item;
	is.hInsertAfter = TVI_SORT;
	
	is.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	is.item.lParam = grp->grp;
	is.item.pszText = _strdup(grp->name);	
	is.item.iImage = 29; 
	is.item.iSelectedImage=is.item.iImage;

	grp->item = TreeView_InsertItem(rto.hwndContact, &is);	

	//if (grp->parent > 0)
	if (grp->parent >0)
	{
		if (grp->expanded)
			TreeView_Expand(rto.hwndContact, groups[grp->parent].item, TVE_EXPAND);
		
	}
}

void DisplayContact(CONTACT *cnt)
{
	TVINSERTSTRUCT is;

	if ((opts.flags1 & FG_HIDEOFFLINE) && (cnt->status & 0xFFFF) == 0xFFFF)
		return;

	if (cnt->groupid<=grp_cnt)
		is.hParent = groups[cnt->groupid].item;
	else
		is.hParent=NULL; //root, cause its an invalid grp

	is.hInsertAfter = TVI_SORT;

	is.item.mask = TVIF_PARAM | TVIF_TEXT| TVIF_IMAGE | TVIF_SELECTEDIMAGE;

	

	if (cnt->id==CT_MSN)
		is.item.iImage=cnt->status;
	else
		is.item.iImage=GetStatusIconIndex(cnt->status);

	is.item.iSelectedImage=is.item.iImage;
	
	is.item.lParam = cnt->uin;
	
	if (*cnt->nick)
		is.item.pszText = _strdup(cnt->nick);
	else
		is.item.pszText = inttotxt(cnt->uin);

	TreeView_InsertItem(rto.hwndContact, &is);

	if (cnt->groupid > 0 && cnt->groupid<=grp_cnt && groups[cnt->groupid].expanded)
		TreeView_Expand(rto.hwndContact, groups[cnt->groupid].item, TVE_EXPAND);
}

int TreeToUin(HTREEITEM hitem)
{
	TVITEM item;
	unsigned int uin = 0;
	
	if (hitem)
	{
		item.mask = TVIF_PARAM;
		item.hItem = hitem;	
		TreeView_GetItem(rto.hwndContact, &item);
		uin = item.lParam;
	}
	return uin;
}

BOOL IsGroup(unsigned int uin)
{
	if ((int)uin<=grp_cnt)
		return TRUE;
	else
		return FALSE;
	
}
BOOL IsMSN(unsigned int uin)
{

	if (uin>=MSN_BASEUIN && uin<=(unsigned int)(MSN_BASEUIN+MSN_ccount))
		return TRUE;
	else
		return FALSE;

}

HTREEITEM FindUIN(HTREEITEM hitem, int uin)
{
	while (hitem)
	{
		TVITEM item;

		item.hItem = hitem;
		item.mask = TVIF_PARAM;
		TreeView_GetItem(rto.hwndContact, &item);
		
		if (item.lParam == uin) return hitem;
		
		if (1)
		{
			HTREEITEM citem;
			citem = FindUIN(TreeView_GetChild(rto.hwndContact, hitem), uin);		
			if (citem) return citem;
		}
		hitem = TreeView_GetNextSibling(rto.hwndContact, hitem);
	}
	return NULL;
}

HTREEITEM UinToTree(int uin)
{
	return FindUIN(TreeView_GetRoot(rto.hwndContact), uin);
}

void SetContactIcon(int uin, int status,int cnttype)
{
	HTREEITEM hitem;

	hitem = UinToTree(uin);

	if ((opts.flags1 & FG_HIDEOFFLINE))
	{
		int i;
		for (i = 0; i < opts.ccount; i++)
		{
			if ((int)opts.clist[i].uin == uin)
			{
				if ((opts.clist[i].status & 0xFFFF) == 0xFFFF)
				{
					hitem = UinToTree(uin);
					if (hitem)
						TreeView_DeleteItem(rto.hwndContact, hitem);
				}
				else
				{
					if (!hitem)
					{
						DisplayContact(&opts.clist[i]);
						hitem = UinToTree(uin);
					}
					if (hitem)
					{
					
						goto changeico;
					}
				}
			}
		}		
	}
	else
	{
		if (hitem)
		{
			TVITEM item;
			
changeico:
			
			item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
			item.hItem = hitem;
			if (cnttype==CT_MSN)
				item.iImage = status;
			else
				item.iImage = GetStatusIconIndex(status);
			
			item.iSelectedImage = item.iImage;
			TreeView_SetItem(rto.hwndContact, &item);
		}
	}
}

int CALLBACK pfsort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	
	CONTACT *a;
	CONTACT *b;

	int i;

	for (i = 0; i < opts.ccount; i++)
	{
		if ((unsigned int)lParam1 == opts.clist[i].uin)
			a = &opts.clist[i];
		if ((unsigned int)lParam2 == opts.clist[i].uin)
			b = &opts.clist[i];	
	}

	if (IsGroup(lParam1) || IsGroup(lParam2))
	{//AT LEAST 1 IS A GROUP
		if (IsGroup(lParam1) && IsGroup(lParam2))
		{//BOTH GROUPS
			return _stricmp(groups[lParam1].name, groups[lParam2].name);
		}
		else
		{//ONE CONTACT & gropp CONTACT
			if (IsGroup(lParam1)) return +1;
			if (IsGroup(lParam2)) return -1;
		}
	}
	else
	{//BOTH CONTACTS
		if ((a->status == 0xFFFF && b->status == 0xFFFF) ||
			(a->status != 0xFFFF && b->status != 0xFFFF))
		{//both online OR both offline
			if (a->status==b->status)
				return _stricmp(a->nick, b->nick);//both same status
			else
			{
				if (a->status<b->status)
					return -1;
				else
					return +1;
					
			}

		}
		else
		{//one is offline & one is online
			if (a->status == 0xFFFF) return +1;
			if (b->status == 0xFFFF) return -1;
		}
	}
	return 0;
}

void Sort(HTREEITEM item)
{
	while (item)
	{
		if (TreeToUin(item) < 1000)
		{
			TVSORTCB sort;

			sort.hParent = item;
			sort.lParam = 0;
			sort.lpfnCompare = pfsort;

			TreeView_SortChildrenCB(rto.hwndContact, &sort, FALSE);
			Sort(TreeView_GetChild(rto.hwndContact, item));
		}
		item = TreeView_GetNextSibling(rto.hwndContact, item);
	}
}

void SortContacts(void)
{
	TVSORTCB sort;

	sort.hParent = TVI_ROOT;
	sort.lParam = 0;
	sort.lpfnCompare = pfsort;

	TreeView_SortChildrenCB(rto.hwndContact, &sort, FALSE);

	Sort(TreeView_GetRoot(rto.hwndContact));
}

void CreateGroup(HWND hwnd)
{
	int grp = 0;
	TVITEM item;
	HTREEITEM hitem;
	if (grp_cnt>=MAX_GROUPS)
	{
		MessageBox(hwnd,"You can't have any more groups.",MIRANDANAME,MB_OK);	
		return;
	}

	hitem = TreeView_GetSelection(hwnd);
	if (hitem)
	{
		item.hItem = hitem;
		item.mask = TVIF_PARAM;

		TreeView_GetItem(hwnd, &item);
		grp = item.lParam;
	}
	
	IncreaseGroupArray();

	groups[grp_cnt-1].structsize = sizeof(CNT_GROUP);
	groups[grp_cnt-1].grp = grp_cnt-1;
	strcpy(groups[grp_cnt-1].name, "New Group");
	groups[grp_cnt-1].parent = grp;

	DisplayGroup(&groups[grp_cnt-1]);

	
	
}

void DeleteGroup(HWND hwnd)
//must be call from removecontact for checking
{
	
	int i;
	int b;
	int grp;
	

	grp=TreeToUin(TreeView_GetSelection(hwnd));
	for (i = 0; i < grp_cnt; i++)
	{
		if (groups[i].parent == grp)
		{
			MessageBox(ghwnd, "This group is not empty", "Error", MB_OK);
			return;
		}
	}
	for (i = 0; i < opts.ccount; i++)
	{
		if (opts.clist[i].groupid == grp)
		{
			MessageBox(ghwnd, "This group is not empty", "Error", MB_OK);
			return;
		}
	}

	TreeView_DeleteItem(hwnd, TreeView_GetSelection(hwnd));

	
	for (i = grp; i < (grp_cnt-1); i++)
	{
	

		groups[i] = groups[i + 1];
		
		if (groups[i].parent > grp) groups[i].parent--;
		groups[i].grp = i;
	}
	DecreaseGroupArray();

	//contacts need to have their grp id shifted
	for (b=0;b<opts.ccount;b++)
	{
		if (opts.clist[b].groupid>grp)
		{//contact needs to shift down his grpid
			opts.clist[b].groupid--;
		}
	}	
	
}

void RenameGroup(HWND hwnd)
{
	HTREEITEM hitem;

	hitem = TreeView_GetSelection(hwnd);
	if (hitem)
	{
		SetFocus(hwnd);
		TreeView_EditLabel(hwnd, hitem);
	}
}

void SetGrpName(int grp, char *name)
{
	strcpy(groups[grp].name, name);
}

void MoveContact(HTREEITEM item, HTREEITEM newparent)
{
	int i;
	int uin = TreeToUin(item);

	for (i = 0; i < opts.ccount; i++)
	{
		if (uin == (int)opts.clist[i].uin)
		{
			opts.clist[i].groupid = TreeToUin(newparent);
			DisplayContact(&opts.clist[i]);
		}
	}
	TreeView_DeleteItem(rto.hwndContact, item);
}

void RefreshContacts(void)
{
}

void IncreaseGroupArray(void)
{
	grp_cnt++;
	if (grp_cnt==1)
	{
		groups=malloc(sizeof(CNT_GROUP));
	}
	else
	{
		groups=realloc(groups,sizeof(CNT_GROUP)*grp_cnt);
	}
}

void DecreaseGroupArray(void)
{
	grp_cnt--;
	if (grp_cnt==0)
	{
		free(groups);
	}
	else
	{
		groups=realloc(groups,sizeof(CNT_GROUP)*grp_cnt);
	}
}

void KillGroupArray(void)
{
	free(groups);
	grp_cnt=0;
}

