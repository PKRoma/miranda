

#define WM_DOCKCB (WM_USER+121)

void Docking_Init(HWND hwnd);
void Docking_UpdateSize(HWND hwnd);
void Docking_Kill(HWND hwnd);
void Docking_ResizeWndToDock();
void Docking_UpdateMenu(HWND hwnd);
