#ifndef OSCAR_SVC_H__
#define OSCAR_SVC_H__

void LoadServices(void);
void UnloadServices(void);
int OscarGetCaps(WPARAM wParam, LPARAM lParam);
int OscarGetName(WPARAM wParam, LPARAM lParam);
int OscarGetStatus(WPARAM wParam, LPARAM lParam);
int OscarSetStatus(WPARAM wParam, LPARAM lParam);

int OscarRecvMessage(WPARAM wParam, LPARAM lParam);
int OscarRequestStatusMessage(WPARAM wParam,LPARAM lParam);
int OscarRecvStatusMessage(WPARAM wParam, LPARAM lParam);

/* Inter thread Messages */
#define WM_ITR_GETSTATUSMSG (WM_ITR_BASE+1) /* wParam=hContact, lParam=seq */

#endif /* OSCAR_SVC_H__ */
