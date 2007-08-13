#ifndef CONNECTION_H
#define CONNECTION_H
#include "defines.h"
extern char* COOKIE;
extern int COOKIE_LENGTH;
extern char* MAIL_COOKIE;
extern int MAIL_COOKIE_LENGTH;
extern char* AVATAR_COOKIE;
extern int AVATAR_COOKIE_LENGTH;
extern char* CWD;//current working directory
extern char* AIM_PROTOCOL_NAME;
extern char* GROUP_ID_KEY;
extern char* ID_GROUP_KEY;
extern char* FILE_TRANSFER_KEY;
extern CRITICAL_SECTION modeMsgsMutex;
extern CRITICAL_SECTION statusMutex;
extern CRITICAL_SECTION connectionMutex;
extern CRITICAL_SECTION SendingMutex;
int LOG(const char *fmt, ...);
HANDLE aim_connect(char* server);
HANDLE aim_peer_connect(char* ip,unsigned short port);
void aim_connection_authorization();
void aim_protocol_negotiation();
void aim_mail_negotiation();
void aim_avatar_negotiation();
#endif
