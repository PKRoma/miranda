/**/

#include <windows.h>
#include <newpluginapi.h>
#include <m_system.h>
#include <m_database.h>
#include "sqlite3/sqlite3.h"
#include "db.h"

#define mir_alloc(n) memoryManagerInterface.mmi_malloc(n)
#define mir_free(ptr) memoryManagerInterface.mmi_free(ptr)
#define mir_realloc(ptr,size) memoryManagerInterface.mmi_realloc(ptr,size)

