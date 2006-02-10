#ifndef WINDOWS_H
#define WINDOWS_H
#include <stdio.h>
#include <windows.h>
#include <prsht.h>
#include <newpluginapi.h>
#include <m_system.h>
#include <m_clist.h>
#include <m_skin.h>
#include <m_protomod.h>
#include <m_protosvc.h>
#include <m_database.h>
#include <m_options.h>
#include <m_langpack.h>
#include <m_userinfo.h>
#include "defines.h"
#include "connection.h"
int OptionsInit(WPARAM wParam,LPARAM lParam);
int UserInfoInit(WPARAM wParam,LPARAM lParam);
static BOOL CALLBACK options_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK userinfo_dialog(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
