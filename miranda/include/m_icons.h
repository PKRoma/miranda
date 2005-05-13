#ifndef M_ICONS_H
#define M_ICONS_H 1

typedef struct {
	int  cbSize;
    char  *pszSection;
	char  *pszId;
	char  *pszName;
	char  *pszDescription;
	HICON hIconDefault;
    char  *pszDefaultFile;
	int   iDefaultIndex;
} SkinIcon;
// Comment: You should pass either a HICON or set pszDefaultFile and iDefaultIndex
//          If both are set then HICON is always used

//  Add a skinned icon
//  pszId should be unique to that section
//
//  wParam = (WPARAM)0
//  lParam = (LPARAM)(SkinIcon*)si;
//  Returns 0 on success
#define MS_ICONS_ADDICON "SkinIconEx/AddIcon"

static int __inline SkinIcon_AddIcon(char *section, char *id, char *name, char *desc, HICON hicon) {
	SkinIcon si;
	
	si.cbSize = sizeof(si);
	si.pszSection = section;
	si.pszId = id;
	si.pszName = name;
	si.pszDescription = desc;
	si.hIconDefault = hicon;
	si.pszDefaultFile = NULL;
	si.iDefaultIndex = 0;
	return CallService(MS_ICONS_ADDICON, 0, (LPARAM)&si);
}

// Get icon
//
// wParam = (WPARAM)0;
// lParam = (LPARAM)id;
// Returns hIcon on success, 0 on failure
#define MS_ICONS_GETICON "SkinIconEx/GetIcon"

static HICON __inline SkinIcon_GetIcon(char *id) {
	HICON hIcon = (HICON)CallService(MS_ICONS_GETICON, 0, (LPARAM)id);
	return hIcon?hIcon:NULL;
}

#endif
