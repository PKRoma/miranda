#include <windows.h>
#include "pluginapi.h" //used for MAX_PLUGIN_TITLE

typedef int  (__stdcall *FARPROC_LOAD)(HWND,HINSTANCE,char*);
typedef void (__stdcall *FARPROC_CLOSE)(void);
typedef long (__stdcall *FARPROC_NOTIFY)(long,WPARAM,LPARAM);
typedef struct
{
	HINSTANCE hinst;
	FARPROC_LOAD loadfunc;
	FARPROC_CLOSE closefunc;
	FARPROC_NOTIFY notifyfunc;
	char Title[MAX_PLUG_TITLE];
} tagPlugin;
#define MAX_PLUGINS 10

void Plugin_GetDir(char*plugpath); //just finds the plugin dir, <app path>\Plugins\ //
void Plugin_Load();//find all dlls and fill up the array...but not call the load func

//these func, call the funcs in ALL loaded dlls
void Plugin_Init();
void Plugin_LoadPlugins(HWND,HINSTANCE); 
void Plugin_ClosePlugins();

int Plugin_NotifyPlugins(long,WPARAM,LPARAM);
int Plugin_NotifyPlugin(int id,long msg,WPARAM wParam,LPARAM lParam); //overloading for a single plugin call (ie show ops

#define IDC_PLUGMNUBASE 8000