#ifndef MIRANDA_BITS_H__
#define MIRANDA_BITS_H__ 1

#include <windows.h>
#include "newpluginapi.h"
#include "m_system.h"
#include "m_database.h"
#include "m_protomod.h"
#include "m_protocols.h"
#include "m_protomod.h"
#include "m_protosvc.h"
#include "m_options.h"
#include "m_langpack.h"

// shared vars
extern HINSTANCE g_hInst;
extern char g_Name[MAX_PATH];

// database keys

// oscar_opt.c
void LoadOptionsServices(void);

#endif // MIRANDA_BITS_H__ 



