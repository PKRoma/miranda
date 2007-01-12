#define _WIN32_WINNT 0x0500
#include <windows.h>
#include <tlhelp32.h>

#include <imagehlp.h>
#pragma comment(lib,"imagehlp.lib")
#pragma comment(lib,"version.lib")
#include <stdio.h>

#define _ALPHA_FUSE_
#include "m_fuse.h"

#include "fuse_log.h"