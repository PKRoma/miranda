#ifndef PROXY_H
#define PROXY_H
#include <stdio.h>
#include <windows.h>
#include <newpluginapi.h>
#include <m_netlib.h>
#include <m_database.h>
#include "defines.h"
#include "aim.h"
#include "connection.h"
#include "direct_connect.h"
void aim_proxy_helper(HANDLE hContact);
int proxy_initialize_send(HANDLE connection,char* sn, char* cookie);
int proxy_initialize_recv(HANDLE connection,char* sn, char* cookie,unsigned short port_check);
#endif
