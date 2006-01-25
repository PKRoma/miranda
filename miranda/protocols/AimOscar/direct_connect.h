#ifndef DIRECT_CONNECT_H
#define DIRECT_CONNECT_H
#include <stdio.h>
#include <windows.h>
#include <newpluginapi.h>
#include <m_netlib.h>
#include <m_database.h>
#include "defines.h"
#include "connection.h"
void aim_dc_helper(HANDLE hContact);
void aim_direct_connection_initiated(HANDLE hNewConnection, DWORD dwRemoteIP, void*);
#endif
