/*
Easytalk: the free text to speech dll for MS Windows
Copyright (C) 2000  Roland Rabien

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

For more information, e-mail figbug@users.sourceforge.net
*/

#include <windows.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <mmsystem.h>
#include <initguid.h>
#include <objbase.h>
#include <objerror.h>
#include <ole2ver.h>
#include <commctrl.h>

#include <speech.h>

#include "resource.h"
#include "talk.h"

#include "..\..\miranda32\miranda.h"
#include "..\..\miranda32\msgque.h"
#include "..\..\miranda32\pluginapi.h"

int easytalk_go(void);
int easytalk_stop(void);

int easytalk_say(char *text);
int easytalk_about(HWND hwnd);
int easytalk_general(HWND hwnd);
int easytalk_config(HWND hwnd);
int easytalk_lexicon(HWND hwnd);

#define WM_UPDATESLIDERS		WM_USER + 5

#define MAXVOICE  100

#define F_AIM	0x01
#define F_RIM	0x02
#define F_ACS	0x04
#define F_AUS	0x08

typedef struct 
{
   RECT     rPosn;      
   BOOL     fUseTagged; 
   GUID     gMode;      
} MOUTHREG, *PMOUTHREG;

MOUTHREG			gMouthReg;
GUID				agVoice[MAXVOICE];

PITTSCENTRAL		gpITTSCentral    = NULL;
PITTSDIALOGS		gpITTSDialogs    = NULL;
PITTSATTRIBUTES		gpITTSAttributes = NULL;
PIATTRIBUTES		gpIAttributes    = NULL;
LPUNKNOWN			gpIAudioUnk      = NULL;

WORD				wPitch;
DWORD				dwSpeed;
DWORD				dwVolume;
DWORD				dwVoice;

int					flags;

PI_CALLBACK			picb;
HINSTANCE			ghInstance;
HWND				gAppWnd;

char szTitle[MAX_PLUG_TITLE] = "Talk";

int enabled = 0;
int running = 0;

HRESULT LoadTTS (GUID *pgMode);

PITTSCENTRAL FindAndSelect (PTTSMODEINFO pTTSInfo)
{
	HRESULT						hRes;
    TTSMODEINFO					ttsResult;
    CHAR						Zero = 0;
    PITTSFIND					pITTSFind;
    PIAUDIOMULTIMEDIADEVICE     pIAMM;    
    PITTSCENTRAL				pITTSCentral;

    hRes = CoCreateInstance(CLSID_TTSEnumerator, NULL, CLSCTX_ALL, IID_ITTSFind, (void**)&pITTSFind);
    if (FAILED(hRes)) return NULL;

    hRes = pITTSFind->Find(pTTSInfo, NULL, &ttsResult);

    if (hRes)
    {
		pITTSFind->Release();
		return NULL;
    }


    hRes = CoCreateInstance(CLSID_MMAudioDest, NULL, CLSCTX_ALL, IID_IAudioMultiMediaDevice, (void**)&pIAMM);
    if (hRes)
    {
		pITTSFind->Release();
        return NULL;
    }
	pIAMM->DeviceNumSet(WAVE_MAPPER);

    hRes = pITTSFind->Select(ttsResult.gModeID, &pITTSCentral, (LPUNKNOWN) pIAMM);

    if (hRes) 
	{
		pITTSFind->Release();
        return NULL;
	}

    pITTSFind->Release();

    return pITTSCentral;
}

static LRESULT CALLBACK DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND		hWndChld;
	PITTSENUM   pITTSEnum = NULL;

	switch (uMsg)
	{
		case WM_INITDIALOG:	
			// initilize the list box
			if (enabled)
			{
				SetDlgItemText(hwndDlg, IDC_ENABLE, "&Enable");
			}
			else
			{
				SetDlgItemText(hwndDlg, IDC_ENABLE, "&Disable");
			}
			
			CoCreateInstance(CLSID_TTSEnumerator, NULL, CLSCTX_ALL, IID_ITTSEnum, (void**)&pITTSEnum);

			DWORD i;
			TTSMODEINFO inf;
			DWORD dwTimes;
			for (i = 0; ; i++) 
			{
				if (pITTSEnum->Next(1, &inf, &dwTimes))
				{
					break;
				}

				// add the name
		        agVoice[i] = inf.gModeID;
			    SendDlgItemMessage (hwndDlg, IDC_VOICE, CB_ADDSTRING, 0, (LPARAM) inf.szModeName);
				if (IsEqualGUID (inf.gModeID, gMouthReg.gMode))
				{
		           SendDlgItemMessage (hwndDlg, IDC_VOICE, CB_SETCURSEL, i, 0);
				}
			}
			SendDlgItemMessage(hwndDlg, IDC_VOICE, CB_SETCURSEL, dwVoice, 0);

			pITTSEnum->Release();

			PostMessage(hwndDlg, WM_UPDATESLIDERS, 0, 0);

			return TRUE;
		case WM_UPDATESLIDERS:
	        DWORD dwMin, dwMax, dwCur;
			WORD wMin, wMax, wCur;

			// pitch
			wMin = wMax = wCur = 0;
			gpITTSAttributes->PitchGet(&wCur);
			gpITTSAttributes->PitchSet(TTSATTR_MINPITCH);
			gpITTSAttributes->PitchGet(&wMin);
			gpITTSAttributes->PitchSet(TTSATTR_MAXPITCH);
			gpITTSAttributes->PitchGet(&wMax);
			gpITTSAttributes->PitchSet(wCur);

			SendDlgItemMessage(hwndDlg, IDC_PITCH, TBM_SETRANGE, FALSE, MAKELONG((WORD)wMin, (WORD)wMax));
			SendDlgItemMessage(hwndDlg, IDC_PITCH, TBM_SETPOS, TRUE, wCur);

			// speed
			dwMin = dwMax = dwCur = 0;
			gpITTSAttributes->SpeedGet(&dwCur);
			gpITTSAttributes->SpeedSet(TTSATTR_MINSPEED);
			gpITTSAttributes->SpeedGet(&dwMin);
			gpITTSAttributes->SpeedSet(TTSATTR_MAXSPEED);
			gpITTSAttributes->SpeedGet(&dwMax);
			gpITTSAttributes->SpeedSet(dwCur);

			SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_SETRANGE, FALSE, MAKELONG((WORD)dwMin, (WORD)dwMax));
			SendDlgItemMessage(hwndDlg, IDC_SPEED, TBM_SETPOS, TRUE, dwCur);			

			// volume
			dwMin = dwMax = dwCur = 0;
			gpITTSAttributes->VolumeGet(&dwCur);

			dwCur &= 0xffff;
			dwCur /= 655;

			SendDlgItemMessage(hwndDlg, IDC_VOLUME, TBM_SETRANGE, FALSE, MAKELONG((WORD)0, (WORD)100));
			SendDlgItemMessage(hwndDlg, IDC_VOLUME, TBM_SETPOS, TRUE, dwCur);			

			break;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_LEXICON:
					easytalk_lexicon(hwndDlg);
					break;
				case IDOK:					
					hWndChld = GetDlgItem(hwndDlg, IDC_VOLUME);
					dwVolume = SendMessage(hWndChld, TBM_GETPOS, 0, 0);

					dwVolume = 0xffff * dwVolume / 100;
					dwVolume |= dwVolume << 16;

					hWndChld = GetDlgItem(hwndDlg, IDC_SPEED);
					dwSpeed = SendMessage(hWndChld, TBM_GETPOS, 0, 0);	

					hWndChld = GetDlgItem(hwndDlg, IDC_PITCH);
					wPitch = (WORD)SendMessage(hWndChld, TBM_GETPOS, 0, 0);	

					EndDialog(hwndDlg, 1);
					return TRUE;
				case IDC_TEST:	
					hWndChld = GetDlgItem(hwndDlg, IDC_VOLUME);
					dwVolume = SendMessage(hWndChld, TBM_GETPOS, 0, 0);

					hWndChld = GetDlgItem(hwndDlg, IDC_SPEED);
					dwSpeed = SendMessage(hWndChld, TBM_GETPOS, 0, 0);	

					hWndChld = GetDlgItem(hwndDlg, IDC_PITCH);
					wPitch = (WORD)SendMessage(hWndChld, TBM_GETPOS, 0, 0);	

					gpITTSAttributes->PitchSet(wPitch);
					gpITTSAttributes->SpeedSet(dwSpeed);

		            dwVolume = 0xffff * dwVolume / 100;
					dwVolume |= dwVolume << 16;

					gpITTSAttributes->VolumeSet(dwVolume);

					srand(time(NULL));
					switch (rand() % 5)
					{
						case 0:
							easytalk_say("mike check 1 2 1 2 we in the house"); 
							break;
						case 1:
							easytalk_say("its tricky to rock a rhyme, to rock a rhyme thats right on time"); 
							break;
						case 2:
							easytalk_say("my rhymes make me wealthy, and the funky bunch helps me"); 
							break;
						case 3:
							easytalk_say("and if your name happens to be herb, just say i'm not the herb your looking for"); 
							break;
						case 4:
							easytalk_say("man makes the money, money never makes the man"); 
							break;
					}
					return TRUE;
			}
			if (HIWORD(wParam) == LBN_SELCHANGE)
			{				
				dwVoice = SendDlgItemMessage (hwndDlg, IDC_VOICE, CB_GETCURSEL, 0, 0);
				if (dwVoice < MAXVOICE) 
				{
					gMouthReg.gMode = agVoice[dwVoice];

					HCURSOR   hCur;
					hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
					LoadTTS (&gMouthReg.gMode);
					SetCursor (hCur);

					// Get the pitch & volume & set those
					SendMessage (hwndDlg, WM_UPDATESLIDERS, 0, 0);
				}
            }
			break;
	}
	return 0;
}

int easytalk_go(void)
{
	__try
	{
		if (!running)
		{
			HRESULT			hRes;
			TTSMODEINFO		ModeInfo;

			if (gpITTSCentral) return 0;

			CoInitialize(NULL);

			memset(&ModeInfo, 0, sizeof(ModeInfo));
			gpITTSCentral = FindAndSelect(&ModeInfo);

			if (gpITTSCentral)
			{
				hRes = gpITTSCentral->QueryInterface (IID_ITTSDialogs, (void**)&gpITTSDialogs);
				hRes = gpITTSCentral->QueryInterface (IID_ITTSAttributes, (void**)&gpITTSAttributes);
				hRes = gpITTSCentral->QueryInterface(IID_IAttributes, (void**)&gpIAttributes);
				
				PITTSENUM   pITTSEnum = NULL;
				CoCreateInstance(CLSID_TTSEnumerator, NULL, CLSCTX_ALL, IID_ITTSEnum, (void**)&pITTSEnum);

				DWORD i;
				TTSMODEINFO inf;
				DWORD dwTimes;
				for (i = 0; ; i++) 
				{
					if (pITTSEnum->Next(1, &inf, &dwTimes))
					{
						break;
					}
					agVoice[i] = inf.gModeID;
				}

				pITTSEnum->Release();

				dwVoice = dwVoice;
				gMouthReg.gMode = agVoice[dwVoice];

				HCURSOR   hCur;
				hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
				LoadTTS (&gMouthReg.gMode);
				SetCursor (hCur);

				gpITTSAttributes->PitchSet(wPitch);
				gpITTSAttributes->SpeedSet(dwSpeed);
				gpITTSAttributes->VolumeSet(dwVolume);

				running = 1;
				return 0;
			}
			else
			{
				return -1;
			}
		}
		else
		{
			return -1;
		}
	}
	__except (1)
	{
		return -1;
	}
}

int easytalk_stop(void)
{
	__try
	{
		if (running)
		{
			if (gpITTSAttributes) gpITTSAttributes->Release(), gpITTSAttributes=NULL;
			if (gpITTSDialogs) gpITTSDialogs->Release(), gpITTSDialogs=NULL;
			if (gpIAttributes) gpIAttributes->Release(), gpIAttributes=NULL;
			
			if (gpITTSCentral) gpITTSCentral->Release(), gpITTSCentral=NULL;

			CoUninitialize();
			running = 0;
		}
		return 0;
	}
	__except (1)
	{
		return -1;
	}
}

int easytalk_say(char *text)
{
	__try
	{
		if (running)
		{
			SDATA data;

			data.dwSize = strlen(text);
			data.pData = text;
			gpITTSCentral->TextData(CHARSET_TEXT, 0, data, NULL, IID_ITTSBufNotifySinkW);
		}
		return 0;
	}
	__except (1)
	{
		return -1;
	}
}

int easytalk_about(HWND hwnd)
{
	__try
	{
		if (gpITTSDialogs && running)
		{
			gpITTSDialogs->AboutDlg(hwnd, NULL);
		}
		return 0;
	}
	__except (1)
	{
		return -1;
	}
}

int easytalk_general(HWND hwnd)
{
	__try
	{
		if (gpITTSDialogs && running)
		{
			gpITTSDialogs->GeneralDlg(hwnd, NULL);
		}
		return 0;
	}
	__except (1)
	{
		return -1;
	}
}

int easytalk_lexicon(HWND hwnd)
{
	__try
	{
		if (gpITTSDialogs && running)
		{
			gpITTSDialogs->LexiconDlg(hwnd, NULL);
		}
		return 0;
	}
	__except (1)
	{
		return -1;
	}
}

int easytalk_config(HWND hwnd)
{
	__try
	{
		if (gpITTSAttributes && running)
		{
			gpITTSAttributes->PitchGet(&wPitch);
			gpITTSAttributes->SpeedGet(&dwSpeed);
			gpITTSAttributes->VolumeGet(&dwVolume);

			if (DialogBox(ghInstance, MAKEINTRESOURCE(IDD_CONFIG), hwnd, (DLGPROC)DlgProc))
			{
				if (gpITTSAttributes->PitchSet(wPitch) != NOERROR)
				{
					MessageBox(hwnd, "Unable to set pitch.", "Error", MB_OK);
				}
				if(gpITTSAttributes->SpeedSet(dwSpeed) != NOERROR)
				{
					MessageBox(hwnd, "Unable to set speed.", "Error", MB_OK);
				}
				if (gpITTSAttributes->VolumeSet(dwVolume) != NOERROR)
				{
					MessageBox(hwnd, "Unable to set volume.", "Error", MB_OK);
				}
			}
		}
		return 0;
	}
	__except (1)
	{
		return -1;
	}
}

HRESULT LoadTTS (GUID *pgMode)
{
	if (gpITTSAttributes) 
	{
		gpITTSAttributes->Release();
		gpITTSAttributes = NULL;
	}
	if (gpITTSCentral)	
	{
		gpITTSCentral->Release();
		gpITTSCentral = NULL;
	}

	// HACK - __try waiting till all messages are cleared
    MSG msg;
	while( PeekMessage(&msg, NULL, NULL, NULL,PM_REMOVE) )
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// HACK - Release audio source
	if (gpIAudioUnk) 
	{
		while (gpIAudioUnk->Release());
		gpIAudioUnk = NULL;
	}

    if (!pgMode) return NOERROR;

    // audio object
    HRESULT  hRes;
	hRes = CoCreateInstance(CLSID_MMAudioDest, NULL, CLSCTX_ALL, IID_IUnknown, (void**)&gpIAudioUnk);
    if (hRes) return hRes;

    TTSMODEINFO    ttsResult;       
    PITTSFIND      pITTSFind;       
    TTSMODEINFO    mi;

    // fill in the moed inf
    memset (&mi, 0 ,sizeof(mi));
    mi.gModeID = *pgMode;
    mi.dwFeatures = TTSFEATURE_VISUAL;

    hRes = CoCreateInstance(CLSID_TTSEnumerator, NULL, CLSCTX_ALL, IID_ITTSFind,
                                                   (void**)&pITTSFind);
    if (FAILED(hRes)) return hRes;

    hRes = pITTSFind->Find(&mi, NULL, &ttsResult);

    if (hRes)
    {
        pITTSFind->Release();
        return hRes;     // error
    }

    // rewrite the mode
    *pgMode = ttsResult.gModeID;

    // Should do select now

    gpIAudioUnk->AddRef();
    hRes = pITTSFind->Select(ttsResult.gModeID, &gpITTSCentral, (LPUNKNOWN) gpIAudioUnk);
    if (hRes) 
	{
		pITTSFind->Release();
		gpIAudioUnk->Release();
		return NULL;
    }


    pITTSFind->Release();
    gpITTSCentral->QueryInterface(IID_ITTSAttributes, (LPVOID *)&gpITTSAttributes);

    return NOERROR;
}

BOOL APIENTRY DllMain(HANDLE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH) 
		ghInstance = (HINSTANCE)hinstDLL;

    return TRUE;
}

static LRESULT CALLBACK MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:	
			if (enabled)
			{
				SetDlgItemText(hwndDlg, IDC_ENABLE, "&Disable");
			}
			else
			{
				SetDlgItemText(hwndDlg, IDC_ENABLE, "&Enable");
			}
			if (flags & F_AIM) CheckDlgButton(hwndDlg, IDC_O1, BST_CHECKED);
			if (flags & F_RIM) CheckDlgButton(hwndDlg, IDC_O2, BST_CHECKED);
			if (flags & F_ACS) CheckDlgButton(hwndDlg, IDC_O3, BST_CHECKED);
			if (flags & F_AUS) CheckDlgButton(hwndDlg, IDC_O4, BST_CHECKED);

			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_CFG:
					easytalk_config(hwndDlg);
					break;
				case IDC_ENABLE:
					if (enabled)
					{
						SetDlgItemText(hwndDlg, IDC_ENABLE, "&Enable");
						easytalk_stop();
					}
					else
					{
						SetDlgItemText(hwndDlg, IDC_ENABLE, "&Disable");
						easytalk_go();
					}
					enabled = !enabled;
					break;
				case IDCANCEL:
					EndDialog(hwndDlg, 0);
					return TRUE;
				case IDOK:
					{
					flags = 0;
					if (IsDlgButtonChecked(hwndDlg, IDC_O1) == BST_CHECKED) flags |= F_AIM;
					if (IsDlgButtonChecked(hwndDlg, IDC_O2) == BST_CHECKED) flags |= F_RIM;
					if (IsDlgButtonChecked(hwndDlg, IDC_O3) == BST_CHECKED) flags |= F_ACS;
					if (IsDlgButtonChecked(hwndDlg, IDC_O4) == BST_CHECKED) flags |= F_AUS;

					picb.pSetPluginInt("Talk", (int)enabled,  "enable");
					picb.pSetPluginInt("Talk", (int)wPitch,   "pitch");
					picb.pSetPluginInt("Talk", (int)dwSpeed,  "speed");
					picb.pSetPluginInt("Talk", (int)dwVolume, "volume");
					picb.pSetPluginInt("Talk", (int)dwVoice,  "voice");
					picb.pSetPluginInt("Talk", (int)flags,    "flags");

					EndDialog(hwndDlg, 1);
					}
					return TRUE;
			}
			break;
	}
	return 0;
}


MIRANDAPLUGIN_API int __stdcall Load(HWND hwnd, HINSTANCE hinst, char *title)
{	
	strcpy(title, szTitle);
	gAppWnd = hwnd;

	return TRUE;
}

MIRANDAPLUGIN_API int __stdcall Unload(void)
{
	easytalk_stop();

	return TRUE;
}

char *statToString(int status)
{
	if (status == 0x0000) return "online";
	if (status == 0x0001) return "away";
	if (status == 0x0002) return "d n d";
	if (status == 0x0013) return "d n d";
	if (status == 0x0004) return "n a";
	if (status == 0x0005) return "n a";
	if (status == 0x0010) return "occupied";
	if (status == 0x0011) return "occupied";
	if (status == 0x0020) return "free chat";
	if (status == 0x0100) return "invisible";

	return "offline";
}

MIRANDAPLUGIN_API int __stdcall Notify(long msg, WPARAM wParam, LPARAM lParam)
{
	char buffer[1024];
	CONTACT *c;

	switch (msg)
	{
		case PM_SHOWOPTIONS:
			DialogBox(ghInstance, MAKEINTRESOURCE(IDD_MAIN), gAppWnd, (DLGPROC)MainDlgProc);
			break;
		case PM_STATUSCHANGE:
			if (!enabled) return FALSE;
			if (flags & F_AUS)
			{
				sprintf(buffer, "you are now in %s mode", statToString(wParam));
				easytalk_say(buffer);
			}
			break;
		case PM_CONTACTSTATUSCHANGE:
			if (!enabled) return FALSE;
			if (flags & F_ACS)
			{
				sprintf(buffer, "%s is now in %s mode", ((CONTACT*)lParam)->nick, statToString(((CONTACT*)lParam)->status));
				easytalk_say(buffer);
			}
			break;
		case PM_GOTMESSAGE:
			if (!enabled) return FALSE;
			if (flags & F_AIM)
			{
				c = picb.pGetContact(((MESSAGE*)lParam)->uin);
				if (!c) return FALSE;
				sprintf(buffer, "incoming message from %s", c->nick);
				easytalk_say(buffer);
			}
			if (flags & F_RIM)
			{
				c = picb.pGetContact(((MESSAGE*)lParam)->uin);
				if (!c) return FALSE;
				sprintf(buffer, "%s says %s", c->nick, ((MESSAGE*)lParam)->msg);
				easytalk_say(buffer);
			}
			break;
		case PM_GOTURL:
			if (!enabled) return FALSE;
			if (flags & F_AIM)
			{
				c = picb.pGetContact(((MESSAGE*)lParam)->uin);
				if (!c) return FALSE;
				sprintf(buffer, "incoming U R L from %s", c->nick);
				easytalk_say(buffer);
			}
			if (flags & F_RIM)
			{
				c = picb.pGetContact(((MESSAGE*)lParam)->uin);
				if (!c) return FALSE;
				sprintf(buffer, "%s says you should go to %s because %s", c->nick, ((MESSAGE*)lParam)->url, ((MESSAGE*)lParam)->msg);
				easytalk_say(buffer);
			}
			break;
		case PM_ICQDEBUGMSG:
			break;
		case PM_REGCALLBACK:
			memcpy(&picb, (void*)wParam, sizeof(picb));
			break;
		case PM_START:
			picb.pGetPluginInt("Talk", (int*)&enabled,  "enable",  0);
			picb.pGetPluginInt("Talk", (int*)&wPitch,   "pitch",   169);
			picb.pGetPluginInt("Talk", (int*)&dwSpeed,  "speed",   170);
			picb.pGetPluginInt("Talk", (int*)&dwVolume, "volume",  0xFFFFFFFF);
			picb.pGetPluginInt("Talk", (int*)&dwVoice,  "voice",   1);
			picb.pGetPluginInt("Talk", (int*)&flags,    "flags",   0);

			if (enabled) easytalk_go();
			break;
		case PM_SAVENOW:
			picb.pSetPluginInt("Talk", (int)enabled,  "enable");
			picb.pSetPluginInt("Talk", (int)wPitch,   "pitch");
			picb.pSetPluginInt("Talk", (int)dwSpeed,  "speed");
			picb.pSetPluginInt("Talk", (int)dwVolume, "volume");
			picb.pSetPluginInt("Talk", (int)dwVoice,  "voice");
			picb.pSetPluginInt("Talk", (int)flags,    "flags");
			break;
	}
	return FALSE;
}



