#ifndef SERVICES_H
#define SERVICES_H
#include <stdio.h>
#include <windows.h>
#include  <prsht.h>
#include <newpluginapi.h>
#include <m_system.h>
#include <m_clist.h>
#include <m_skin.h>
#include <m_protomod.h>
#include <m_protosvc.h>
#include <m_database.h>
#include <m_options.h>
#include <m_langpack.h>
#include "defines.h"
#include "aim.h"
#include "connection.h"
#include "packets.h"
#include "thread.h"
#include "utility.h"
#include "resource.h"
#include "direct_connect.h"
#include "proxy.h"
static int ModulesLoaded(WPARAM wParam,LPARAM lParam);
static int GetCaps(WPARAM wParam, LPARAM lParam);
static int GetName(WPARAM wParam, LPARAM lParam);
static int GetStatus(WPARAM wParam, LPARAM lParam);
static int SetStatus(WPARAM wParam, LPARAM lParam);
static int SendMsg(WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK dialog_message(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void CreateServices();
#endif
