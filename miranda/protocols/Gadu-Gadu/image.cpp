////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2003 Adam Strzelecki <ono+miranda@java.pl>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////////////////////////////////////////////////////

extern "C" {
    #include "gg.h"
}

#include <ocidl.h>
#include <olectl.h>

#define HIMETRIC_INCH 2540

#define WM_ADDIMAGE  WM_USER + 1
#define WM_SENDIMG   WM_USER + 2
#define WM_CHOOSEIMG WM_USER + 3

////////////////////////////////////////////////////////////////////////////
// Image Window : Data

// tablica zawiera uin kolesia i uchwyt do okna, okno zawiera GGIMAGEDLGDATA
// ktore przechowuje handle kontaktu, i wskaznik na pierwszy  obrazek
// obrazki sa poukladane jako lista jednokierunkowa.
// przy tworzeniu okna podaje sie handle do kontaktu
// dodajac obrazek tworzy sie element listy i wysyla do okna
// wyswietlajac obrazek idzie po liscie jednokierunkowej

typedef struct _GGIMAGEENTRY
{
    LPPICTURE imagePicture;
    DWORD imageSize;
    char *lpszFileName;
    HGLOBAL hPicture;
    struct _GGIMAGEENTRY *lpNext;
} GGIMAGEENTRY;

typedef struct
{
    HANDLE hContact;
    HANDLE hEvent;
    HWND hWnd;
    uin_t uin;
    int crc32;
    int nImg, nImgTotal;
    GGIMAGEENTRY *lpImages;
    SIZE size, minSize;
    BOOL bReceiving;
} GGIMAGEDLGDATA;

// List of image windows
list_t gg_imagedlgs;

HICON hIcons[5]; // icon handles

// Mutex & Event
HANDLE hImgMutex;

// Prototypes
int gg_img_add(GGIMAGEDLGDATA *dat);
int gg_img_remove(GGIMAGEDLGDATA *dat);
int gg_img_sendimg(WPARAM wParam, LPARAM lParam);
extern "C" GGIMAGEENTRY * gg_img_loadpicture(struct gg_event* e, HANDLE hContact, char *szFileName);

////////////////////////////////////////////////////////////////////////////
// Image Module : Adding item to contact menu, creating sync objects
int gg_img_load()
{
    CLISTMENUITEM mi;
    ZeroMemory(&mi,sizeof(mi));
    mi.cbSize = sizeof(mi);
    char service[64];

    // Send image contact menu item
    snprintf(service, sizeof(service), GGS_SENDIMAGE, GG_PROTO);
    CreateServiceFunction(service, gg_img_sendimg);
    mi.pszPopupName = GG_PROTONAME;
    mi.position = -2000010000;
    mi.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_IMAGE));
    mi.pszName = Translate("&Image");
    mi.pszService = service;
    mi.pszContactOwner = GG_PROTO;
    CallService(MS_CLIST_ADDCONTACTMENUITEM, 0, (LPARAM) &mi);

    // Init mutex
    hImgMutex = CreateMutex(NULL, FALSE, NULL);

    hIcons[0] = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_PREV), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    hIcons[1] = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_NEXT), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    hIcons[2] = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_DELETE), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    hIcons[3] = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_SAVE), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    hIcons[4] = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_SCALE), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
}

////////////////////////////////////////////////////////////////////////////
// Image Module : closing dialogs, sync objects
int gg_img_unload()
{
    int i=0;

    // Close event & mutex
    CloseHandle(hImgMutex);

    // Delete all / release all dialogs
    while(gg_imagedlgs && gg_img_remove((GGIMAGEDLGDATA *)gg_imagedlgs->data));
    list_destroy(gg_imagedlgs, 1);

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////
// Painting image
int gg_img_paint(HWND hwnd, GGIMAGEENTRY *dat)
{
    PAINTSTRUCT paintStruct;
    HDC hdc = BeginPaint(hwnd, &paintStruct);
    RECT rc; GetWindowRect(GetDlgItem(hwnd, IDC_IMG_IMAGE), &rc);
    ScreenToClient(hwnd, (POINT *)&rc.left);
    ScreenToClient(hwnd, (POINT *)&rc.right);
    FillRect(hdc, &rc, (HBRUSH)GetSysColorBrush(COLOR_WINDOW));

    if (dat->imagePicture)
    {
        // Get width and height of picture
        long hmWidth;
        long hmHeight;
        dat->imagePicture->get_Width(&hmWidth);
        dat->imagePicture->get_Height(&hmHeight);

        // Convert himetric to pixels
        int nWidth = MulDiv(hmWidth, GetDeviceCaps(hdc, LOGPIXELSX), HIMETRIC_INCH);
        int nHeight = MulDiv(hmHeight, GetDeviceCaps(hdc, LOGPIXELSY), HIMETRIC_INCH);

        // Display picture using IPicture::Render
        if ( nWidth > (rc.right-rc.left) || nHeight > (rc.bottom-rc.top) )
        {
            if((double)nWidth / (double)nHeight > (double) (rc.right-rc.left) / (double)(rc.bottom-rc.top))
                dat->imagePicture->Render(hdc,
                    rc.left,
                    ((rc.top + rc.bottom) - (rc.right - rc.left) * nHeight / nWidth) / 2,
                    (rc.right - rc.left),
                    (rc.right - rc.left) * nHeight / nWidth,
                    0, hmHeight, hmWidth, -hmHeight, &rc);
            else
                dat->imagePicture->Render(hdc,
                    ((rc.left + rc.right) - (rc.bottom - rc.top) * nWidth / nHeight) / 2,
                    rc.top,
                    (rc.bottom - rc.top) * nWidth / nHeight,
                    (rc.bottom - rc.top),
                    0, hmHeight, hmWidth, -hmHeight, &rc);
        }
        else
        {
            dat->imagePicture->Render(hdc,
                (rc.left + rc.right - nWidth) / 2,
                (rc.top + rc.bottom - nHeight) / 2,
                nWidth, nHeight,
                0, hmHeight, hmWidth, -hmHeight, &rc);
        }
    }
    EndPaint(hwnd, &paintStruct);

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// Save specified image entry
int gg_img_saveimage(HWND hwnd, GGIMAGEENTRY *dat)
{
    LPVOID pvData;

    OPENFILENAME ofn;
    char szFileName[MAX_PATH];

    char szFilter[128], *pFilter = szFilter;
    strcpy(pFilter, Translate("Image files (*.bmp,*.jpg,*.gif)"));
    pFilter += strlen(pFilter) + 1;
    strcpy(pFilter, "*.bmp;*.jpg;*.gif");
    pFilter += strlen(pFilter) + 1;
    *pFilter = 0;

    memset(&ofn, 0, sizeof(OPENFILENAME));

    if(!dat) return FALSE;

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = hInstance;
    strcpy(szFileName, dat->lpszFileName);
    ofn.lpstrFile = szFileName;
    ofn.lpstrFilter = szFilter;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_NOCHANGEDIR;
    if(GetSaveFileName(&ofn))
    {
        FILE *fp = fopen(szFileName, "w+b" );
        if(fp)
        {
            pvData = GlobalLock(dat->hPicture);
            fwrite(pvData, dat->imageSize, 1, fp);
            fclose(fp);
            GlobalUnlock(dat->hPicture);
#ifdef DEBUGMODE
            gg_netlog("gg_img_saveimage(): Image saved to %s.", szFileName);
#endif
        }
        else
        {
#ifdef DEBUGMODE
            gg_netlog("gg_img_saveimage(): Cannot save image to %s.", szFileName);
#endif
            MessageBox(hwnd, Translate("Image cannot be written to disk."), GG_PROTO, MB_ICONERROR);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// Resize layouting functions
#define LAYOUT_SIZE(hwndDlg, id, rect, size, oldSize) \
                GetWindowRect(GetDlgItem(hwndDlg, id), &rect); \
                SetWindowPos(GetDlgItem(hwndDlg, id), NULL, 0, 0, \
                    rect.right - rect.left + size.cx - oldSize.cx, \
                    rect.bottom - rect.top + size.cy - oldSize.cy, \
                    SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW)
#define LAYOUT_SIZE_X(hwndDlg, id, rect, size, oldSize) \
                GetWindowRect(GetDlgItem(hwndDlg, id), &rect); \
                SetWindowPos(GetDlgItem(hwndDlg, id), NULL, 0, 0, \
                    rect.right - rect.left + size.cx - oldSize.cx, \
                    rect.bottom - rect.top, \
                    SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW)
#define LAYOUT_MOVE(hwndDlg, id, rect, size, oldSize) \
                GetWindowRect(GetDlgItem(hwndDlg, id), &rect); \
                ScreenToClient(hwndDlg, (POINT *)&rect); \
                SetWindowPos(GetDlgItem(hwndDlg, id), NULL, \
                    rect.left + size.cx - oldSize.cx, \
                    rect.top + size.cy - oldSize.cy, \
                    0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW)
#define LAYOUT_MOVE_X(hwndDlg, id, rect, size, oldSize) \
                GetWindowRect(GetDlgItem(hwndDlg, id), &rect); \
                ScreenToClient(hwndDlg, (POINT *)&rect); \
                SetWindowPos(GetDlgItem(hwndDlg, id), NULL, \
                    rect.left + size.cx - oldSize.cx, \
                    rect.top, \
                    0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW)

////////////////////////////////////////////////////////////////////////////
// Send / Recv main dialog proc
static BOOL CALLBACK gg_img_dlgproc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    GGIMAGEDLGDATA *dat = (GGIMAGEDLGDATA *)GetWindowLong(hwndDlg, GWL_USERDATA);

    switch(msg)
    {
        case WM_INITDIALOG:
            {
                TranslateDialogDefault(hwndDlg);
                InitCommonControls();

                // Get dialog data
                dat = (GGIMAGEDLGDATA *)lParam;
                SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)dat);

                // Save dialog handle
                dat->hWnd = hwndDlg;

                // Send event if someone's waiting
                if(dat->hEvent) SetEvent(dat->hEvent);
#ifdef DEBUGMODE
                else gg_netlog("gg_img_dlgproc(): Creation event not found, but someone might be waiting.");
#endif

                // Making buttons flat
                SendDlgItemMessage(hwndDlg,IDC_IMG_SAVE,BUTTONSETASFLATBTN,0,0);
                SendDlgItemMessage(hwndDlg,IDC_IMG_PREV,BUTTONSETASFLATBTN,0,0);
                SendDlgItemMessage(hwndDlg,IDC_IMG_NEXT,BUTTONSETASFLATBTN,0,0);
                SendDlgItemMessage(hwndDlg,IDC_IMG_SCALE,BUTTONSETASFLATBTN,0,0);
                SendDlgItemMessage(hwndDlg,IDC_IMG_DELETE,BUTTONSETASFLATBTN,0,0);

                // Setting images for buttons
                SendDlgItemMessage(hwndDlg,IDC_IMG_PREV,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIcons[0]);
                SendDlgItemMessage(hwndDlg,IDC_IMG_NEXT,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIcons[1]);
                SendDlgItemMessage(hwndDlg,IDC_IMG_DELETE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIcons[2]);
                SendDlgItemMessage(hwndDlg,IDC_IMG_SAVE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIcons[3]);
                SendDlgItemMessage(hwndDlg,IDC_IMG_SCALE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)hIcons[4]);

                // Set main window image
                SendMessage(hwndDlg,WM_SETICON,ICON_BIG,(LPARAM)LoadIcon(hInstance,MAKEINTRESOURCE(IDI_IMAGE)));
                char *szName = (char *) CallService(MS_CLIST_GETCONTACTDISPLAYNAME, (WPARAM)dat->hContact, 0);

                char szTitle[128];
                if(dat->bReceiving)
                    snprintf(szTitle, sizeof(szTitle), Translate("Image from %s"), szName);
                else
                    snprintf(szTitle, sizeof(szTitle), Translate("Image for %s"), szName);
                SetWindowText(hwndDlg, szTitle);

                // Store client extents
                RECT rect; GetClientRect(hwndDlg, &rect);
                dat->size.cx = rect.right - rect.left;
                dat->size.cy = rect.bottom - rect.top;
                dat->minSize = dat->size;
            }
            return TRUE;

        case WM_SIZE:
            if(wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED)
            {
                RECT rect; GetClientRect(hwndDlg, &rect);
                SIZE size = {LOWORD(lParam), HIWORD(lParam)};

                LockWindowUpdate(hwndDlg);
                // Image area
                LAYOUT_SIZE_X(hwndDlg, IDC_IMG_NAME, rect, size, dat->size);
                LAYOUT_SIZE(hwndDlg, IDC_IMG_IMAGE, rect, size, dat->size);
                // Buttons
                if(dat->bReceiving)
                {
                    LAYOUT_MOVE_X(hwndDlg, IDC_IMG_PREV, rect, size, dat->size);
                    LAYOUT_MOVE_X(hwndDlg, IDC_IMG_NEXT, rect, size, dat->size);
                    LAYOUT_MOVE_X(hwndDlg, IDC_IMG_SAVE, rect, size, dat->size);
                    LAYOUT_MOVE_X(hwndDlg, IDC_IMG_DELETE, rect, size, dat->size);
                }
                else
                    LAYOUT_MOVE(hwndDlg, IDC_IMG_SEND, rect, size, dat->size);
                LAYOUT_MOVE(hwndDlg, IDC_IMG_CANCEL, rect, size, dat->size);

                // Store new size
                dat->size = size;
                LockWindowUpdate(NULL);
                InvalidateRect(hwndDlg, NULL, FALSE);
            }
            break;

        case WM_SIZING:
            {
                RECT *pRect = (RECT *)lParam;
                if(pRect->right - pRect->left < dat->minSize.cx)
                {
                    if(wParam == WMSZ_BOTTOMLEFT || wParam == WMSZ_LEFT || wParam == WMSZ_TOPLEFT)
                        pRect->left = pRect->right - dat->minSize.cx;
                    else
                        pRect->right = pRect->left + dat->minSize.cx;
                }
                if(pRect->bottom - pRect->top < dat->minSize.cy)
                {
                    if(wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOP || wParam == WMSZ_TOPRIGHT)
                        pRect->top = pRect->bottom - dat->minSize.cy;
                    else
                        pRect->bottom = pRect->top + dat->minSize.cy;
                }
            }
            break;

        case WM_CLOSE:
            EndDialog(hwndDlg, 0);
            break;

        case WM_PAINT:
            if (dat->lpImages)
            {
                GGIMAGEENTRY *img = dat->lpImages;

                for(int i = 1; img && (i < dat->nImg); i++)
                    img = img->lpNext;

                if(!img)
                {
#ifdef DEBUGMODE
                    gg_netlog("gg_img_dlgproc(): Image was not found on the list. Cannot paint the window.");
#endif
                    return FALSE;
                }

                if(dat->bReceiving)
                {
                    char szTitle[128];
                    snprintf(szTitle, sizeof(szTitle),
                        "%s (%d / %d)", img->lpszFileName, dat->nImg, dat->nImgTotal);
                    SetDlgItemText(hwndDlg, IDC_IMG_NAME, szTitle);
                }
                else
                    SetDlgItemText(hwndDlg, IDC_IMG_NAME, img->lpszFileName);
                gg_img_paint(hwndDlg, img);
            }
            break;

        case WM_DESTROY:
            if(dat)
            {
                // Deleting all image entries
                GGIMAGEENTRY *temp, *img = dat->lpImages;
                while(temp = img)
                {
                    img = img->lpNext;

                    free(temp->lpszFileName);
                    GlobalFree(temp->hPicture);
                    free(temp);
                }

                if(WaitForSingleObject(hImgMutex, 30000) == WAIT_TIMEOUT)
                {
#ifdef DEBUGMODE
                    gg_netlog("gg_img_dlgproc(): Waiting for mutex failed. Cannot destroy image window properly.");
#endif
                    CloseHandle(hImgMutex);
                    return FALSE;
                }
                list_remove(&gg_imagedlgs, dat, 1);
                ReleaseMutex(hImgMutex);
            }
            return TRUE;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_IMG_CANCEL:
                    EndDialog(hwndDlg, 0);
                    return TRUE;

                case IDC_IMG_PREV:
                    if(dat->nImg > 1)
                    {
                        dat->nImg--;
                        InvalidateRect(hwndDlg, NULL, FALSE);
                    }
                    return TRUE;

                case IDC_IMG_NEXT:
                    if (dat->nImg < dat->nImgTotal)
                    {
                        dat->nImg++;
                        InvalidateRect(hwndDlg, NULL, FALSE);
                    }
                    return TRUE;

                case IDC_IMG_DELETE:
                    {
                        GGIMAGEENTRY *del, *img = dat->lpImages;
                        if(dat->nImg == 1)
                        {
                            del = dat->lpImages;
                            dat->lpImages = img->lpNext;
                        }
                        else
                        {
                            for(int i = 1; img && (i < dat->nImg - 1); i++)
                                img = img->lpNext;
                            if(!img)
                            {
#ifdef DEBUGMODE
                                gg_netlog("gg_img_dlgproc(): Image was not found on the list. Cannot delete it from the list.");
#endif
                                return FALSE;
                            }
                            del = img->lpNext;
                            img->lpNext = del->lpNext;
                            dat->nImg --;
                        }

                        free(del->lpszFileName);
                        GlobalFree(del->hPicture);
                        free(del);

                        if((-- dat->nImgTotal) == 0)
                            EndDialog(hwndDlg, 0);
                        else
                            InvalidateRect(hwndDlg, NULL, FALSE);
                    }
                    return TRUE;

                case IDC_IMG_SAVE:
                    {
                        GGIMAGEENTRY *img = dat->lpImages;

                        for(int i = 1; img && (i < dat->nImg); i++)
                            img = img->lpNext;
                        if(!img)
                        {
#ifdef DEBUGMODE
                            gg_netlog("gg_img_dlgproc(): Image was not found on the list. Cannot launch saving.");
#endif
                            return FALSE;
                        }
                        gg_img_saveimage(hwndDlg, img);
                    }
                    return TRUE;

                case IDC_IMG_SEND:
                    {
                        unsigned char format[20];
                        char *msg = "\0"; // empty message

                        if (dat->lpImages)
                        {
                            uin_t uin = (uin_t)DBGetContactSettingDword(dat->hContact, GG_PROTO, GG_KEY_UIN, 0);

                            ((struct gg_msg_richtext*)format)->flag=2;

                            struct gg_msg_richtext_format * r = (struct gg_msg_richtext_format *)(format+sizeof(struct gg_msg_richtext));
                            r->position = 0;
                            r->font = 0x80;
                            struct gg_msg_richtext_image * p = (struct gg_msg_richtext_image *)
                            (format + sizeof(struct gg_msg_richtext)+ sizeof(struct gg_msg_richtext_format));
                            p->unknown1 = 0x109;
                            p->size = dat->lpImages->imageSize;
                            LPVOID pvData = GlobalLock(dat->lpImages->hPicture);

                            p->crc32 = gg_fix32(gg_crc32(0, (unsigned char*)pvData, dat->lpImages->imageSize ));
                            GlobalUnlock(dat->lpImages->hPicture);

                            int len = sizeof(struct gg_msg_richtext_format)+sizeof(struct gg_msg_richtext_image);
                            ((struct gg_msg_richtext*)format)->length = len;

                            gg_send_message_richtext(ggSess, GG_CLASS_CHAT, (uin_t)uin,(unsigned char*)msg,format,len+sizeof(struct gg_msg_richtext));

                            // Protect dat from releasing
                            SetWindowLong(hwndDlg, GWL_USERDATA, (LONG)NULL);

                            EndDialog(hwndDlg, 0);
                        }
                        return TRUE;
                    }
                    break;
            }
            break;

        case WM_ADDIMAGE: // lParam == GGIMAGEENTRY *dat
            {
                GGIMAGEENTRY *lpImage = (GGIMAGEENTRY *)lParam;
                GGIMAGEENTRY *lpImages = dat->lpImages;

                if(!dat->lpImages) // first image entry
                    dat->lpImages = lpImage;
                else // adding at the end of the list
                {
                    while(lpImages->lpNext)
                        lpImages = lpImages->lpNext;
                    lpImages->lpNext = lpImage;
                }
                dat->nImg = ++ dat->nImgTotal;
            }
            InvalidateRect(hwndDlg,0,0);
            return TRUE;

        case WM_CHOOSEIMG:
        {
            char szFilter[128], *pFilter = szFilter;
            strcpy(pFilter, Translate("Image files (*.bmp,*.jpg,*.gif)"));
            pFilter += strlen(pFilter) + 1;
            strcpy(pFilter, "*.bmp;*.jpg;*.gif");
            pFilter += strlen(pFilter) + 1;
            *pFilter = 0;

            OPENFILENAME ofn;
            char szFileName[MAX_PATH];
            ZeroMemory(&ofn, sizeof(ofn));
            *szFileName = 0;
            ofn.lStructSize = sizeof(OPENFILENAME);
            ofn.hwndOwner = hwndDlg;
            ofn.hInstance = hInstance;
            ofn.lpstrFilter = szFilter;
            ofn.lpstrFile = szFileName;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrTitle = Translate("Select picture to send");
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
            if(GetOpenFileName(&ofn))
            {
                if (dat->lpImages)
                {
                    free(dat->lpImages->lpszFileName);
                    GlobalFree(dat->lpImages->hPicture);
                    free(dat->lpImages);
                }
                dat->lpImages = gg_img_loadpicture(0, dat->hContact, szFileName);
                ShowWindow(hwndDlg, SW_SHOW);
            }
            else
            {
                EndDialog(hwndDlg, 0);
                return FALSE;
            }
            return TRUE;
        }
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////
// Calls opening of the dialog
extern "C" void gg_img_dlgcall(void *empty)
{
    HWND hMIWnd = 0; //(HWND) CallService(MS_CLUI_GETHWND, 0, 0);

    GGIMAGEDLGDATA *dat = (GGIMAGEDLGDATA *)empty;
    DialogBoxParam(hInstance, dat->bReceiving ? MAKEINTRESOURCE(IDD_IMAGE_RECV) : MAKEINTRESOURCE(IDD_IMAGE_SEND),
        hMIWnd, gg_img_dlgproc, (LPARAM) dat);
}

////////////////////////////////////////////////////////////////////////////
// Open dialog receive for specified contact
GGIMAGEDLGDATA *gg_img_recvdlg(HANDLE hContact)
{
    pthread_t dwThreadID;

    // Create dialog data
    GGIMAGEDLGDATA *dat = (GGIMAGEDLGDATA *)malloc(sizeof(GGIMAGEDLGDATA));
    ZeroMemory(dat, sizeof(GGIMAGEDLGDATA));
    dat->hContact = hContact;
    dat->lpImages = NULL;
    dat->nImg = 0;
    dat->nImgTotal = 0;
    dat->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    dat->bReceiving = TRUE;
    ResetEvent(dat->hEvent);
    pthread_create(&dwThreadID, NULL, gg_img_dlgthread, (void *)dat);
    return dat;
}

////////////////////////////////////////////////////////////////////////////
// Return if uin has it's window already opened
extern "C" BOOL gg_img_opened(uin_t uin)
{
    list_t l = gg_imagedlgs;
    while(l)
    {
        GGIMAGEDLGDATA *dat = (GGIMAGEDLGDATA *)l->data;
        if(dat->uin == uin)
            return TRUE;
        l = l->next;
    }
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////
// Image Module : Looking for window entry, create if not found
extern "C" int gg_img_display(uin_t uin, HANDLE hContact, GGIMAGEENTRY *img)
{
    if(WaitForSingleObject(hImgMutex, 20000) == WAIT_TIMEOUT)
    {
#ifdef DEBUGMODE
        gg_netlog("gg_img_display(): Waiting for mutex failed. Cannot create new image object.");
#endif
        return FALSE;
    }

    list_t l = gg_imagedlgs;
    GGIMAGEDLGDATA *dat;
    while(l)
    {
        dat = (GGIMAGEDLGDATA *)l->data;
        if (dat->hContact == hContact)
        {
            break;
        }
        l = l->next;
    }
    if(!l) dat = NULL;

    if(!dat)
    {
        dat = gg_img_recvdlg(hContact);
        dat->uin = uin;
        if(WaitForSingleObject(dat->hEvent, 10000) == WAIT_TIMEOUT) // Waiting 10 seconds for handle
        {
#ifdef DEBUGMODE
            gg_netlog("gg_img_display(): Cannot create image display window. No signal received.");
#endif
            CloseHandle(dat->hEvent);
            ReleaseMutex(hImgMutex);
            return FALSE;
        }

        list_add(&gg_imagedlgs, dat, 0);
        CloseHandle(dat->hEvent);
        dat->hEvent = NULL;
    }

    SendMessage(dat->hWnd, WM_ADDIMAGE, 0, (LPARAM)img);
    SetForegroundWindow(dat->hWnd);
    SetFocus(dat->hWnd);

    ReleaseMutex(hImgMutex);
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////
// Image Window : Loading picture and sending for display
extern "C" GGIMAGEENTRY * gg_img_loadpicture(struct gg_event* e, HANDLE hContact, char *szFileName)
{
    HANDLE hImageFile = 0;

    GGIMAGEENTRY *dat;
    dat = (GGIMAGEENTRY *)malloc(sizeof(GGIMAGEENTRY));
    memset(dat, 0, sizeof(GGIMAGEENTRY) );
    dat->lpNext =0;

    if(szFileName)
    {
        hImageFile = CreateFile(szFileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
        if(hImageFile == INVALID_HANDLE_VALUE)
            return FALSE;
    }

    if(hImageFile)
        dat->imageSize = GetFileSize(hImageFile, 0);
    else
        dat->imageSize = e->event.image_reply.size;

    if(szFileName)
    {
        char szFileTitle[MAX_PATH];
        GetFileTitle(szFileName, szFileTitle, MAX_PATH);
        dat->lpszFileName = (char *)malloc(strlen(szFileTitle) + 1);
        strcpy(dat->lpszFileName, szFileTitle);
    }
    else
    {
        dat->lpszFileName = (char *)malloc(strlen(e->event.image_reply.filename) + 1);
        strcpy(dat->lpszFileName, e->event.image_reply.filename);
    }

    /////////////////////
    // Creating Picture
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, dat->imageSize);

    dat->hPicture = hGlobal;

    // Lock memory for reading
    LPVOID pvData = GlobalLock(hGlobal);
    if(pvData == NULL)
    {
        GlobalUnlock(hGlobal);
#ifdef DEBUGMODE
        gg_netlog("gg_img_loadpicture(): GlobalLock(...) for image failed.");
#endif

        if (hImageFile>0) CloseHandle(hImageFile);
        free(dat);
        return FALSE;
    }

    DWORD dwBytes;
    if (hImageFile)
        ReadFile( hImageFile, pvData, dat->imageSize, &dwBytes, 0);
    else
        memcpy(pvData, e->event.image_reply.image, e->event.image_reply.size);

    GlobalUnlock(hGlobal);

    // Create IStream* from global memory
    LPSTREAM pstm = NULL;
    HRESULT hr = CreateStreamOnHGlobal(hGlobal,FALSE, &pstm);
    if (!(SUCCEEDED(hr)) || (pstm == NULL))
    {
#ifdef DEBUGMODE
        gg_netlog("gg_img_loadpicture(): CreateStreamOnHGlobal(...) failed for token.");
#endif
        if(hImageFile > 0)
            CloseHandle(hImageFile);
        if(pstm != NULL)
            pstm->Release();
        free(dat);
        return FALSE;
    }

    // Create IPicture from image file
    hr = OleLoadPicture(pstm, dat->imageSize, FALSE, IID_IPicture,
                        (LPVOID *)&dat->imagePicture);

    // Check results
    if (!(SUCCEEDED(hr)) || (dat->imagePicture == NULL))
    {
        pstm->Release();
        MessageBox(
            NULL,
            Translate("Could not load image. Image might have unsupported file format."),
            GG_PROTOERROR,
            MB_OK | MB_ICONSTOP
        );
        if (hImageFile>0) CloseHandle(hImageFile);
        free(dat);
        return FALSE;
    }

    pstm->Release();

    if(hImageFile > 0) CloseHandle(hImageFile);

    return dat;
}

////////////////////////////////////////////////////////////////////////////
// Image Recv : AddEvent proc
int gg_img_recvimage(WPARAM wParam, LPARAM lParam)
{
    CLISTEVENT *cle = (CLISTEVENT *)lParam;
    struct gg_event* e = (struct gg_event *)cle->lParam;

    GGIMAGEENTRY *img = gg_img_loadpicture(e, cle->hContact, 0);
    gg_img_display(
        e->event.image_reply.sender,
        gg_getcontact(e->event.image_reply.sender, 1, 0, NULL), img);

    gg_free_event(e);
}

////////////////////////////////////////////////////////////////////////////
// Windows queue management
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
int gg_img_add(GGIMAGEDLGDATA *dat)
{
    if(!dat) return FALSE;

    if(WaitForSingleObject(hImgMutex, 20000) == WAIT_TIMEOUT)
    {
#ifdef DEBUGMODE
        gg_netlog("gg_img_add(): Waiting for mutex failed. Cannot add new image to the list.");
#endif
        return FALSE;
    }

    list_add(&gg_imagedlgs, dat, 0);

    ReleaseMutex(hImgMutex);
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////
int gg_img_remove(GGIMAGEDLGDATA *dat)
{
    if(!dat) return FALSE;

    if(WaitForSingleObject(hImgMutex, 20000) == WAIT_TIMEOUT)
    {
#ifdef DEBUGMODE
        gg_netlog("gg_img_remove(): Waiting for mutex failed. Cannot remove image from the list.");
#endif
        return FALSE;
    }

    GGIMAGEENTRY *temp, *img = dat->lpImages;

    while(temp = img)
    {
        img = img->lpNext;

        free(temp->lpszFileName);
        GlobalFree(temp->hPicture);
        free(temp);
    }
    list_remove(&gg_imagedlgs, dat, 1);

    ReleaseMutex(hImgMutex);
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////
//
GGIMAGEDLGDATA *gg_img_find(uin_t uin, DWORD crc32)
{
    if(WaitForSingleObject(hImgMutex, 20000) == WAIT_TIMEOUT)
    {
#ifdef DEBUGMODE
        gg_netlog("gg_img_find(): Waiting for mutex failed. Cannot find image on the list.");
#endif
        CloseHandle(hImgMutex);
        return FALSE;
    }

    int res = 0;

    list_t l = gg_imagedlgs;
    GGIMAGEDLGDATA *dat;

    while(l)
    {
        dat = (GGIMAGEDLGDATA *)l->data;
        if(!dat) break;

        uin_t c_uin = DBGetContactSettingDword(dat->hContact, GG_PROTO, GG_KEY_UIN, 0);

        if (!dat->bReceiving && /*( f_dat->crc32 == crc32 ) &&*/ ( c_uin == uin ) )
        {
            ReleaseMutex(hImgMutex);
            return dat;
        }

        l = l->next;
    }

    ReleaseMutex(hImgMutex);

#ifdef DEBUGMODE
    gg_netlog("gg_img_find(): Image not found on the list. It might be released before calling this function.");
#endif
    return NULL;
}


////////////////////////////////////////////////////////////////////////////
// Image Module : Send on Request
BOOL gg_img_sendonrequest(struct gg_event* e)
{
    GGIMAGEDLGDATA *dat = gg_img_find(e->event.image_request.sender, e->event.image_request.crc32);

    if(!dat) return FALSE;

    LPVOID pvData = GlobalLock(dat->lpImages->hPicture);
    gg_image_reply(ggSess, e->event.image_request.sender, dat->lpImages->lpszFileName, (char*)pvData, dat->lpImages->imageSize);
    GlobalUnlock(dat->lpImages->hPicture);

    gg_img_remove(dat);
}

////////////////////////////////////////////////////////////////////////////
// Send Image : Run (Thread and main)
int gg_img_sendimg(WPARAM wParam, LPARAM lParam)
{
    pthread_t dwThreadID;

    if(WaitForSingleObject(hImgMutex, 20000) == WAIT_TIMEOUT)
    {
#ifdef DEBUGMODE
        gg_netlog("gg_img_sendimg(): Waiting for mutex failed. Cannot send image from the list.");
#endif
        CloseHandle(hImgMutex);
        return FALSE;
    }

    HANDLE hContact = (HANDLE)wParam;
    //list_t l = gg_imagedlgs;
    GGIMAGEDLGDATA *dat = NULL;

    // Lookup for the sending window of specified uin
    // Depreciated... one dialog for one sender
    /*
    while(l)
    {
        dat = (GGIMAGEDLGDATA *) l->data;
        if(dat->hContact == hContact)
            break;
        l = l->next;
    }
    if(!l) dat = NULL;
    */
    if(!dat)
    {
        dat = (GGIMAGEDLGDATA *)malloc(sizeof(GGIMAGEDLGDATA));
        dat->hContact = hContact;
        dat->lpImages = 0;
        dat->nImg = 0;
        dat->nImgTotal = 0;
        dat->bReceiving = FALSE;
        dat->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        ResetEvent(dat->hEvent);

        // Create new dialog
        pthread_create(&dwThreadID, NULL, gg_img_dlgthread, (void *)dat);

        // Waiting 10 seconds for handle
        if(WaitForSingleObject(dat->hEvent, 10000) == WAIT_TIMEOUT)
        {
#ifdef DEBUGMODE
            gg_netlog("gg_img_sendimg(): Cannot create image preview window. No signal received.");
#endif
            CloseHandle(dat->hEvent);
            return FALSE;
        }
        list_add(&gg_imagedlgs, dat, 0);
        CloseHandle(dat->hEvent);
    }

    // Request choose dialog
    SendMessage(dat->hWnd, WM_CHOOSEIMG, 0, 0);

    ReleaseMutex(hImgMutex);
}
