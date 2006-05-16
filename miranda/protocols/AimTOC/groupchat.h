#ifndef GROUPCHAT_H
#define GROUPCHAT_H

#define AIM_GROUPCHAT_DEFEXCHANGE 4
#define AIM_GCHAT_PREFIX "GroupChats"

void aim_gchat_delete_by_contact(HANDLE hContact);
void aim_gchat_joinrequest(char *room, int exchange);
void aim_gchat_chatinvite(char *szRoom, char *szUser, char *chatid, char *msg);
void aim_gchat_updatestatus();
void aim_gchat_create(int dwRoom, char *szRoom);
void aim_gchat_sendmessage(int dwRoom, char *szUser, char *szMessage, int whisper);
void aim_gchat_updatebuddy(int dwRoom, char *szUser, int joined);
void aim_gchat_init();
void aim_gchat_destroy();

#endif
