#ifndef _internal_h
#define _internal_h

#include "../icqlib/icq.h"
#include "miranda.h"
#include "msgque.h"


///////////////
// Callbacks //
///////////////

void CbLogged(struct icq_link *link);
void CbDisconnected(struct icq_link *link);
void CbRecvMsg(struct icq_link *link, unsigned long uin, unsigned char hour, unsigned char minute, unsigned char day, unsigned char month, unsigned short year, const char *msg);
void CbUserOnline(struct icq_link *link, unsigned long uin, unsigned long status, unsigned long ip, unsigned short port, unsigned long real_ip, unsigned char tcp_flag);
void CbUserOffline(struct icq_link *link, unsigned long uin);
void CbUserStatusUpdate(struct icq_link *link, unsigned long uin, unsigned long status);
void CbInfoReply(struct icq_link *link, unsigned long uin, const char *nick, const char *first, const char *last, const char *email, char auth);
void CbSrvAck(struct icq_link *link, unsigned short seq);
void CbRecvUrl(struct icq_link *link, unsigned long uin, unsigned char hour, unsigned char minute, unsigned char day, unsigned char month, unsigned short year, const char *url, const char *desc);
void CbRecvAdded(struct icq_link *link, unsigned long uin, unsigned char hour, unsigned char minute, unsigned char day, unsigned char month, unsigned short year, const char *nick, const char *first, const char *last, const char *email);
void CbRecvAuthReq(struct icq_link *link, unsigned long uin, unsigned char hour, unsigned char minute, unsigned char day, unsigned char month, unsigned short year, const char *nick, const char *first, const char *last, const char *email, const char *reason);
void CbUserFound(struct icq_link *link, unsigned long uin, const char *nick, const char *first, const char *last, const char *email, char auth);
void CbSearchDone(struct icq_link *link);
void CbExtInfoReply(struct icq_link *link, unsigned long uin, const char *city, unsigned short country_code, char country_stat, const char *state, unsigned short age, char gender, const char *phone, const char *hp, const char *about);
void CbWrongPassword(struct icq_link *link);
void CbInvalidUIN(struct icq_link *link);
void CbRequestNotify(struct icq_link *link, unsigned long id, int type, int arg, void *data);
void CbLog(struct icq_link *link, time_t time, unsigned char level, const char *str);
void CbRecvFileReq(struct icq_link *link, unsigned long uin, unsigned char hour, unsigned char minute, unsigned char day, unsigned char month, unsigned short year, const char *descr, const char *filename, unsigned long filesize, unsigned long seq);
void CbMetaUserFound(struct icq_link *link, unsigned short seq2, unsigned long uin, const char *nick, const char *first, const char *last, const char *email, char auth);
void CbMetaUserInfo(struct icq_link *link, unsigned short seq2, const char *nick, const char *first, const char *last, const char *pri_eml, const char *sec_eml, const char *old_eml, const char *city, const char *state, const char *phone, const char *fax, const char *street, const char *cellular, unsigned long zip, unsigned short country, unsigned char timezone, unsigned char auth, unsigned char webaware, unsigned char hideip);
void CbMetaUserWork(struct icq_link *link, unsigned short seq2, const char *wcity, const char *wstate, const char *wphone, const char *wfax, const char *waddress, unsigned long wzip, unsigned short wcountry, const char *company, const char *department, const char *job, unsigned short occupation, const char *whomepage);
void CbMetaUserMore(struct icq_link *link, unsigned short seq2, unsigned short age, unsigned char gender, const char *homepage, unsigned char byear, unsigned char bmonth, unsigned char bday, unsigned char lang1, unsigned char lang2, unsigned char lang3);
void CbMetaUserAbout(struct icq_link *link, unsigned short seq2, const char *about);
void CbMetaUserInterests(struct icq_link *link, unsigned short seq2, unsigned char num, unsigned short icat1, const char *int1, unsigned short icat2, const char *int2, unsigned short icat3, const char *int3, unsigned short icat4, const char *int4);
void CbMetaUserAffiliations(struct icq_link *link, unsigned short seq2, unsigned char anum, unsigned short acat1, const char *aff1, unsigned short acat2, const char *aff2, unsigned short acat3, const char *aff3, unsigned short acat4, const char *aff4, unsigned char bnum, unsigned short bcat1, const char *back1, unsigned short bcat2, const char *back2, unsigned short bcat3, const char *back3, unsigned short bcat4, const char *back4);
void CbMetaUserHomePageCategory(struct icq_link *link, unsigned short seq2, unsigned char num, unsigned short hcat1, const char *htext1);
void CbNewUIN(struct icq_link *link, unsigned long uin);
void CbSetTimeout(struct icq_link *link, long interval);

///////////////
// Wnd Procs //
///////////////

BOOL CALLBACK DlgProcAbout(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcMsgRcv(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcMsgSend(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcUrlSend(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcPassword(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcSound(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcAdded(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcUrlRecv(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcAddContact1(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcAddContact2(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcAddContact3(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcResult(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcDetails(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcGenOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcOpsHotkey(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcProxy(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcHistory(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcAuthReq(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcMsgRecv(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcGPL(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcICQOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcMSNOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcMSN_AddContact(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcOpsTransparency(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcOpsAdvanced(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcHistoryFind(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DlgProcRecvFile(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
////////////////////
// Misc Functions //
////////////////////
void SetStatusTextEx(char*buff,int slot); 
void SetStatusText(char *buff);
void IPtoString(unsigned long IP,char* strIP);
void PlaySoundEvent(char *event);
void ChangeStatus(HWND hWnd, int newStatus);
void LoadContactList(OPTIONS *opts, OPTIONS_RT *rto);
void SaveContactList(OPTIONS *opts, OPTIONS_RT *rto);
void ChangeContactStatus(unsigned long uin, unsigned long status);
void DisplayMessageRecv(int msgid);
void SetCaptionToNext(unsigned long uin);
void ShowNextMsg(int mqid); //used with URLs too
void SetGotMsgIcon(unsigned long uin,BOOL gotmsg); //For when a u recved a msg/url
void DisplayUrlRecv(int msgid);
void DeleteWindowFromList(HWND hwnd);
void UpdateUserStatus(void);
void ContactDoubleClicked(HWND hwnd);
void SendIcqMessage(unsigned long uin, char *name);
void AddContactbyIcq(HWND hwnd);
void AddContactbyEmail(HWND hwnd);
void AddContactbyName(HWND hwnd);
void TrayIcon(HWND hwnd, int action);
void AddToContactList(unsigned int uin);
void SendIcqUrl(unsigned long uin, char *name);
void RemoveContact(HWND hwnd);
void RenameContact(HWND hwnd);
void OpenResultWindow(void);
void DisplayDetails(HWND hwnd);
void DisplayUINDetails(unsigned int uin);
void GetStatusString(int status,BOOL shortmsg,char*buffer);
void tcp_engage(void);
void tcp_disengage(void);
void parseCmdLine(char *p);
void SetUserFlags(unsigned int uin, unsigned int flg);
void ClrUserFlags(unsigned int uin, unsigned int flg);
void AddToHistory(unsigned int uin, time_t evt_t, int dir, char *msg);
void DisplayHistory(HWND hwnd, unsigned int uin);
void ShowHide(); //show/hide ttoggle of main wnd
void SendFiles(HWND hwnd, unsigned long uin);
void InitMenuIcons();
void LoadOptions(OPTIONS *opts, char *postkey);
void SaveOptions(OPTIONS *opts, char *postkey);
void OptionsWindow(HWND hwnd);

int GetStatusIconIndex(unsigned long status);
int GetModelessType(HWND hwnd);
int FirstTime(char *lpCmdLine);
int GetOpenURLs(char *buffer, int sz);
int InContactList(unsigned int uin);
int LoadSound(char *key, char *sound);
int SaveSound(char *key, char *sound);
int BrowseForWave(HWND wnnd);
int GetMenuIconNo(long ID, BOOL *isCheck);

char *LookupContactNick(unsigned long uin);

unsigned int GetSelectedUIN(HWND hwnd);
unsigned int *GetAckPtr(HWND hwnd);
unsigned int GetUserFlags(unsigned int uin);

unsigned long GetDlgUIN(HWND hwnd);

HWND CheckForDupWindow(unsigned long uin,long wintype); //is there a send msg window already open for that user

time_t MakeTime(int hour, int minute, int day, int month, int year);

#endif