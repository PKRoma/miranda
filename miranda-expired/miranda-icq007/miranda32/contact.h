#ifndef _contact_h
#define _contact_h


void LoadGroups(void);
void SaveGroups(void);

void DisplayGroup(CNT_GROUP *grp);
void DisplayContact(CONTACT *cnt);
void SortContacts(void);
void SetContactIcon(int uin, int status,int cnttype);
void CreateGroup(HWND hwnd);
void RenameGroup(HWND hwnd);
void SetGrpName(int grp, char *name);
void DeleteGroup(HWND hwnd);
void MoveContact(HTREEITEM item, HTREEITEM newparent);
void RefreshContacts(void);

void IncreaseGroupArray(void); 
void DecreaseGroupArray(void);
void KillGroupArray(void);

int TreeToUin(HTREEITEM hitem);
BOOL IsGroup(unsigned int uin);
BOOL IsMSN(unsigned int uin);

HTREEITEM UinToTree(int uin);


#endif