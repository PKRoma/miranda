#ifndef PROXY_H
#define PROXY_H
#include "defines.h"
void aim_proxy_helper(HANDLE hContact);
int proxy_initialize_send(HANDLE connection,char* sn, char* cookie);
int proxy_initialize_recv(HANDLE connection,char* sn, char* cookie,unsigned short port_check);
#endif
