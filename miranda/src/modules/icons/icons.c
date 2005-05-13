#include "../../core/commonheaders.h"

// Todo:
//  Add hashes for quicker search
//  Don't add same id twice

typedef struct {
	char  *section, *id, *name, *description;
	HICON  default_icon;
	char  *default_file;
	int    default_index;
	char  *curr_file;
	int    curr_index;
	HICON  curr_icon;
} SkinIconItem;

static int Icons_Shutdown(WPARAM wParam, LPARAM lParam);
static int Icons_AddIcon(WPARAM wParam, LPARAM lParam);
static int Icons_GetIcon(WPARAM wParam, LPARAM lParam);

static SkinIconItem *icons;
static int iconsCount;
static HANDLE gHookShutdown;

#define ICONS_MOD "IconsEx"

int LoadIconsModule(void) {
	icons = NULL;
	iconsCount = 0;
	CreateServiceFunction(MS_ICONS_ADDICON, Icons_AddIcon);
	CreateServiceFunction(MS_ICONS_GETICON, Icons_GetIcon);
	gHookShutdown =	HookEvent(ME_SYSTEM_PRESHUTDOWN, Icons_Shutdown);
	return 0;
}

static int Icons_Shutdown(WPARAM wParam, LPARAM lParam) {
	int i;
	for (i=0; i<iconsCount; i++) {
		if (icons[i].section) free(icons[i].section);
		if (icons[i].id) free(icons[i].id);
		if (icons[i].name) free(icons[i].name);
		if (icons[i].description) free(icons[i].description);
		if (icons[i].default_icon) DestroyIcon(icons[i].default_icon);
		if (icons[i].default_file) free(icons[i].default_file);
		if (icons[i].curr_file) free(icons[i].curr_file);
		if (icons[i].curr_icon) DestroyIcon(icons[i].curr_icon);
	}
	if (icons) free(icons);
	UnhookEvent(gHookShutdown);
	return 0;
}

static int Icons_AddIcon(WPARAM wParam, LPARAM lParam) {
	SkinIcon *si = (SkinIcon*)lParam;

	if (!si||si->cbSize!=sizeof(SkinIcon))
		return 1;
	icons = (SkinIconItem*)realloc(icons, sizeof(SkinIconItem)*(iconsCount+1));
	ZeroMemory(&icons[iconsCount], sizeof(SkinIconItem));
	if (si->pszSection) icons[iconsCount].section = _strdup(si->pszSection);
	if (si->pszId) icons[iconsCount].id = _strdup(si->pszId);
	if (si->pszName) icons[iconsCount].name = _strdup(si->pszName);
	if (si->pszDescription) icons[iconsCount].description = _strdup(si->pszDescription);

	icons[iconsCount].default_icon = si->hIconDefault;
	if (si->pszDefaultFile) icons[iconsCount].default_file = _strdup(si->pszDefaultFile);
	icons[iconsCount].default_index = si->iDefaultIndex;
	iconsCount++;
	return 0;
}

static HICON Icons_ExtractFromPath(const char *path) {
	char *comma;
	char file[MAX_PATH],fileFull[MAX_PATH];
	int n;
	HICON hIcon;

	if (path == NULL) return (HICON)NULL;

	lstrcpyn(file,path,sizeof(file));
	comma=strrchr(file,',');
	if(comma==NULL) n=0;
	else {n=atoi(comma+1); *comma=0;}
    CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)file, (LPARAM)fileFull);
	hIcon=NULL;
	ExtractIconEx(fileFull,n,NULL,&hIcon,1);
	return hIcon;
}

static int Icons_GetIcon(WPARAM wParam, LPARAM lParam) {
	char *pszId = (char *)lParam;
	DBVARIANT dbv;
	int i;

	if(pszId) {
		for(i=0; i<iconsCount; i++) {
			if(strcmp(icons[i].id, pszId)==0) {
				if (icons[i].curr_icon) 
					return (int)icons[i].curr_icon;
				else {
					if(DBGetContactSetting(NULL, ICONS_MOD, pszId, &dbv)==0) {
						icons[i].curr_icon = Icons_ExtractFromPath(dbv.pszVal);
						DBFreeVariant(&dbv);
					}
					if(icons[i].curr_icon==NULL) {
						if (icons[i].default_icon!=NULL)
							return (int)icons[i].default_icon;
						if (icons[i].default_file==NULL)
							return (int)NULL;
						ExtractIconEx(icons[i].default_file, icons[i].default_index, NULL, &icons[i].curr_icon, 1);
					}
					return (int)icons[i].curr_icon;
				}
			}
		}
	}
	return 0;
}
