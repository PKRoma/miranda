#ifndef DIRECT_CONNECT_H
#define DIRECT_CONNECT_H
#include "defines.h"
void aim_dc_helper(HANDLE hContact);
void aim_direct_connection_initiated(HANDLE hNewConnection, DWORD dwRemoteIP, void*);
#endif
