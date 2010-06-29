/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

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
*/
#include "commonheaders.h"
#include "netlib.h"

struct NetlibTempSettings 
{
	DWORD flags;
	char *szSettingsModule;
	NETLIBUSERSETTINGS settings;
};

static LIST <NetlibTempSettings> tempSettings(5);

static const UINT outgoingConnectionsControls[] =
{
	IDC_STATIC12,
	IDC_USEPROXY,
	  IDC_STATIC21,IDC_PROXYTYPE,
	  IDC_STATIC22,IDC_PROXYHOST,IDC_STATIC23,IDC_PROXYPORT,IDC_STOFTENPORT,
	  IDC_PROXYAUTH,
	    IDC_STATIC31,IDC_PROXYUSER,IDC_STATIC32,IDC_PROXYPASS,
	  IDC_PROXYDNS,
	IDC_SPECIFYPORTSO,
	  IDC_PORTSRANGEO,
	  IDC_STATIC54,
    IDC_VALIDATESSL};
static const UINT useProxyControls[]={
	IDC_STATIC21,IDC_PROXYTYPE,
	IDC_STATIC22,IDC_PROXYHOST,IDC_STATIC23,IDC_PROXYPORT,IDC_STOFTENPORT,
	IDC_PROXYAUTH,
	  IDC_STATIC31,IDC_PROXYUSER,IDC_STATIC32,IDC_PROXYPASS,
	IDC_PROXYDNS};
static const UINT specifyOPortsControls[]={
	IDC_PORTSRANGEO,
	  IDC_STATIC54
};
static const UINT incomingConnectionsControls[]={
	IDC_STATIC43,
	IDC_SPECIFYPORTS,
	IDC_PORTSRANGE,
	IDC_STATIC52,
    IDC_ENABLEUPNP};
static const UINT specifyPortsControls[]={
	IDC_PORTSRANGE,
	IDC_STATIC52};

static const TCHAR* szProxyTypes[]={_T("<mixed>"),_T("SOCKS4"),_T("SOCKS5"),_T("HTTP"),_T("HTTPS"),_T("Internet Explorer")};
static const WORD oftenProxyPorts[]={1080,1080,1080,8080,8080,8080};

#define M_REFRESHALL      (WM_USER+100)
#define M_REFRESHENABLING (WM_USER+101)

static void ShowMultipleControls(HWND hwndDlg,const UINT *controls,int cControls,int state)
{
	int i;
	for(i=0;i<cControls;i++) ShowWindow(GetDlgItem(hwndDlg,controls[i]),state);
}

static void EnableMultipleControls(HWND hwndDlg,const UINT *controls,int cControls,int state)
{
	int i;
	for(i=0;i<cControls;i++) EnableWindow(GetDlgItem(hwndDlg,controls[i]),state);
}

static void AddProxyTypeItem(HWND hwndDlg,int type,int selectType)
{
	int i;
	i = SendDlgItemMessage(hwndDlg,IDC_PROXYTYPE,CB_ADDSTRING,0,(LPARAM)(type==0?TranslateTS(szProxyTypes[type]):szProxyTypes[type]));
	SendDlgItemMessage(hwndDlg, IDC_PROXYTYPE, CB_SETITEMDATA, i, type);
	if (type == selectType) SendDlgItemMessage(hwndDlg, IDC_PROXYTYPE, CB_SETCURSEL, i, 0);
}

static void CopySettingsStruct(NETLIBUSERSETTINGS *dest,NETLIBUSERSETTINGS *source)
{
	*dest=*source;
	if(dest->szIncomingPorts) dest->szIncomingPorts=mir_strdup(dest->szIncomingPorts);
	if(dest->szOutgoingPorts) dest->szOutgoingPorts=mir_strdup(dest->szOutgoingPorts);
	if(dest->szProxyAuthPassword) dest->szProxyAuthPassword=mir_strdup(dest->szProxyAuthPassword);
	if(dest->szProxyAuthUser) dest->szProxyAuthUser=mir_strdup(dest->szProxyAuthUser);
	if(dest->szProxyServer) dest->szProxyServer=mir_strdup(dest->szProxyServer);
}

static void CombineSettingsStrings(char **dest,char **source)
{
	if(*dest!=NULL && (*source==NULL || lstrcmpiA(*dest,*source))) {mir_free(*dest); *dest=NULL;}
}

static void CombineSettingsStructs(NETLIBUSERSETTINGS *dest,DWORD *destFlags,NETLIBUSERSETTINGS *source,DWORD sourceFlags)
{
	if(sourceFlags&NUF_OUTGOING) {
		if(*destFlags&NUF_OUTGOING) {
			if(dest->validateSSL!=source->validateSSL) dest->validateSSL=2;
			if(dest->useProxy!=source->useProxy) dest->useProxy=2;
			if(dest->proxyType!=source->proxyType) dest->proxyType=0;
			CombineSettingsStrings(&dest->szProxyServer,&source->szProxyServer);
			if(dest->wProxyPort!=source->wProxyPort) dest->wProxyPort=0;
			if(dest->useProxyAuth!=source->useProxyAuth) dest->useProxyAuth=2;
			CombineSettingsStrings(&dest->szProxyAuthUser,&source->szProxyAuthUser);
			CombineSettingsStrings(&dest->szProxyAuthPassword,&source->szProxyAuthPassword);
			if(dest->dnsThroughProxy!=source->dnsThroughProxy) dest->dnsThroughProxy=2;
			if(dest->specifyOutgoingPorts!=source->specifyOutgoingPorts) dest->specifyOutgoingPorts=2;
			CombineSettingsStrings(&dest->szOutgoingPorts,&source->szOutgoingPorts);
		}
		else {
            dest->validateSSL=source->validateSSL;
			dest->useProxy=source->useProxy;
			dest->proxyType=source->proxyType;
			dest->szProxyServer=source->szProxyServer;
			if(dest->szProxyServer) dest->szProxyServer=mir_strdup(dest->szProxyServer);
			dest->wProxyPort=source->wProxyPort;
			dest->useProxyAuth=source->useProxyAuth;
			dest->szProxyAuthUser=source->szProxyAuthUser;
			if(dest->szProxyAuthUser) dest->szProxyAuthUser=mir_strdup(dest->szProxyAuthUser);
			dest->szProxyAuthPassword=source->szProxyAuthPassword;
			if(dest->szProxyAuthPassword) dest->szProxyAuthPassword=mir_strdup(dest->szProxyAuthPassword);
			dest->dnsThroughProxy=source->dnsThroughProxy;
			dest->specifyOutgoingPorts=source->specifyOutgoingPorts;
			dest->szOutgoingPorts=source->szOutgoingPorts;
			if (dest->szOutgoingPorts) dest->szOutgoingPorts=mir_strdup(dest->szOutgoingPorts);
		}
	}
	if(sourceFlags&NUF_INCOMING) {
		if(*destFlags&NUF_INCOMING) {
            if(dest->enableUPnP!=source->enableUPnP) dest->enableUPnP=2;
			if(dest->specifyIncomingPorts!=source->specifyIncomingPorts) dest->specifyIncomingPorts=2;
			CombineSettingsStrings(&dest->szIncomingPorts,&source->szIncomingPorts);
		}
		else {
			dest->enableUPnP=source->enableUPnP;
			dest->specifyIncomingPorts=source->specifyIncomingPorts;
			dest->szIncomingPorts=source->szIncomingPorts;
			if(dest->szIncomingPorts) dest->szIncomingPorts=mir_strdup(dest->szIncomingPorts);
		}
	}
	if((*destFlags&NUF_NOHTTPSOPTION)!=(sourceFlags&NUF_NOHTTPSOPTION))
		*destFlags=(*destFlags|sourceFlags)&~NUF_NOHTTPSOPTION;
	else *destFlags|=sourceFlags;
}

static void ChangeSettingIntByCheckbox(HWND hwndDlg,UINT ctrlId,int iUser,int memberOffset)
{
	int newValue,i;

	newValue=IsDlgButtonChecked(hwndDlg,ctrlId)!=BST_CHECKED;
	CheckDlgButton(hwndDlg,ctrlId,newValue?BST_CHECKED:BST_UNCHECKED);
	if (iUser == -1)
	{
		for (i=0; i<tempSettings.getCount(); i++)
			if (!(tempSettings[i]->flags & NUF_NOOPTIONS))
				*(int*)(((PBYTE)&tempSettings[i]->settings) + memberOffset) = newValue;
	}
	else *(int*)(((PBYTE)&tempSettings[iUser]->settings) + memberOffset)=newValue;
	SendMessage(hwndDlg,M_REFRESHENABLING,0,0);
}

static void ChangeSettingStringByEdit(HWND hwndDlg,UINT ctrlId,int iUser,int memberOffset)
{
	int i,newValueLen;
	char *szNewValue,**ppszNew;

	newValueLen=GetWindowTextLength(GetDlgItem(hwndDlg,ctrlId));
	szNewValue=(char*)mir_alloc(newValueLen+1);
	GetDlgItemTextA(hwndDlg,ctrlId,szNewValue,newValueLen+1);
	if (iUser == -1) 
	{
		for (i=0; i<tempSettings.getCount(); ++i)
			if (!(tempSettings[i]->flags & NUF_NOOPTIONS))
			{
				ppszNew=(char**)(((PBYTE)&tempSettings[i]->settings)+memberOffset);
				if(*ppszNew) mir_free(*ppszNew);
				*ppszNew=mir_strdup(szNewValue);
			}
		mir_free(szNewValue);
	}
	else {
		ppszNew=(char**)(((PBYTE)&tempSettings[iUser]->settings)+memberOffset);
		if(*ppszNew) mir_free(*ppszNew);
		*ppszNew=szNewValue;
	}
}

static void WriteSettingsStructToDb(const char *szSettingsModule,NETLIBUSERSETTINGS *settings,DWORD flags)
{
	if(flags&NUF_OUTGOING) {
		char szEncodedPassword[512];
		DBWriteContactSettingByte(NULL,szSettingsModule,"NLValidateSSL",(BYTE)settings->validateSSL);
		DBWriteContactSettingByte(NULL,szSettingsModule,"NLUseProxy",(BYTE)settings->useProxy);
		DBWriteContactSettingByte(NULL,szSettingsModule,"NLProxyType",(BYTE)settings->proxyType);
		DBWriteContactSettingString(NULL,szSettingsModule,"NLProxyServer",settings->szProxyServer?settings->szProxyServer:"");
		DBWriteContactSettingWord(NULL,szSettingsModule,"NLProxyPort",(WORD)settings->wProxyPort);
		DBWriteContactSettingByte(NULL,szSettingsModule,"NLUseProxyAuth",(BYTE)settings->useProxyAuth);
		DBWriteContactSettingString(NULL,szSettingsModule,"NLProxyAuthUser",settings->szProxyAuthUser?settings->szProxyAuthUser:"");
		lstrcpynA(szEncodedPassword,settings->szProxyAuthPassword?settings->szProxyAuthPassword:"",SIZEOF(szEncodedPassword));
		CallService(MS_DB_CRYPT_ENCODESTRING,SIZEOF(szEncodedPassword),(LPARAM)szEncodedPassword);
		DBWriteContactSettingString(NULL,szSettingsModule,"NLProxyAuthPassword",szEncodedPassword);
		DBWriteContactSettingByte(NULL,szSettingsModule,"NLDnsThroughProxy",(BYTE)settings->dnsThroughProxy);
		DBWriteContactSettingByte(NULL,szSettingsModule,"NLSpecifyOutgoingPorts",(BYTE)settings->specifyOutgoingPorts);
		DBWriteContactSettingString(NULL,szSettingsModule,"NLOutgoingPorts",settings->szOutgoingPorts?settings->szOutgoingPorts:"");
	}
	if(flags&NUF_INCOMING) {
        DBWriteContactSettingByte(NULL,szSettingsModule,"NLEnableUPnP",(BYTE)settings->enableUPnP);
		DBWriteContactSettingByte(NULL,szSettingsModule,"NLSpecifyIncomingPorts",(BYTE)settings->specifyIncomingPorts);
		DBWriteContactSettingString(NULL,szSettingsModule,"NLIncomingPorts",settings->szIncomingPorts?settings->szIncomingPorts:"");
	}
}

void NetlibSaveUserSettingsStruct(const char *szSettingsModule,NETLIBUSERSETTINGS *settings)
{
	int i;
	NETLIBUSERSETTINGS combinedSettings={0};
	DWORD flags;

	EnterCriticalSection(&csNetlibUser);
	
	NetlibUser *thisUser, tUser;
	tUser.user.szSettingsModule = (char*)szSettingsModule;
	thisUser = netlibUser.find(&tUser);

	if (thisUser == NULL)
	{
		LeaveCriticalSection(&csNetlibUser);
		return;
	}

	NetlibFreeUserSettingsStruct(&thisUser->settings);
	CopySettingsStruct(&thisUser->settings, settings);
	WriteSettingsStructToDb(thisUser->user.szSettingsModule, &thisUser->settings, thisUser->user.flags);
	combinedSettings.cbSize = sizeof(combinedSettings);
	for (i=0, flags=0; i < netlibUser.getCount(); ++i)
	{
		if (thisUser->user.flags & NUF_NOOPTIONS) continue;
		CombineSettingsStructs(&combinedSettings, &flags, &thisUser->settings, thisUser->user.flags);
	}
    if(combinedSettings.validateSSL==2) combinedSettings.validateSSL=0;
	if(combinedSettings.useProxy==2) combinedSettings.useProxy=0;
	if(combinedSettings.proxyType==0) combinedSettings.proxyType=PROXYTYPE_SOCKS5;
	if(combinedSettings.useProxyAuth==2) combinedSettings.useProxyAuth=0;
	if(combinedSettings.dnsThroughProxy==2) combinedSettings.dnsThroughProxy=1;
    if(combinedSettings.enableUPnP==2) combinedSettings.enableUPnP=1;
	if(combinedSettings.specifyIncomingPorts==2) combinedSettings.specifyIncomingPorts=0;
	WriteSettingsStructToDb("Netlib",&combinedSettings,flags);
	NetlibFreeUserSettingsStruct(&combinedSettings);
	LeaveCriticalSection(&csNetlibUser);
}

static INT_PTR CALLBACK DlgProcNetlibOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{	int iUser,iItem;

			TranslateDialogDefault(hwndDlg);
			iItem=SendDlgItemMessage(hwndDlg,IDC_NETLIBUSERS,CB_ADDSTRING,0,(LPARAM)TranslateT("<All connections>"));
			SendDlgItemMessage(hwndDlg,IDC_NETLIBUSERS,CB_SETITEMDATA,iItem,(LPARAM)-1);
			SendDlgItemMessage(hwndDlg,IDC_NETLIBUSERS,CB_SETCURSEL,iItem,0);
			
			EnterCriticalSection(&csNetlibUser);
			for (iUser = 0; iUser < netlibUser.getCount(); ++iUser)
			{
				NetlibTempSettings *thisSettings = (NetlibTempSettings*)mir_calloc(sizeof(NetlibTempSettings));
				thisSettings->flags = netlibUser[iUser]->user.flags;
				thisSettings->szSettingsModule = mir_strdup(netlibUser[iUser]->user.szSettingsModule);
				CopySettingsStruct(&thisSettings->settings, &netlibUser[iUser]->settings);
				tempSettings.insert(thisSettings);

				if (netlibUser[iUser]->user.flags & NUF_NOOPTIONS) continue;
				iItem = SendDlgItemMessage(hwndDlg, IDC_NETLIBUSERS, CB_ADDSTRING, 0, 
					(LPARAM)netlibUser[iUser]->user.ptszDescriptiveName);
				SendDlgItemMessage(hwndDlg, IDC_NETLIBUSERS,CB_SETITEMDATA, iItem, iUser);
			}
			LeaveCriticalSection(&csNetlibUser);
			
			SendMessage(hwndDlg,M_REFRESHALL,0,0);
			return TRUE;
		}
		case M_REFRESHALL:
		{	int iUser=SendDlgItemMessage(hwndDlg,IDC_NETLIBUSERS,CB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_NETLIBUSERS,CB_GETCURSEL,0,0),0);
			NETLIBUSERSETTINGS settings = {0};
			DWORD flags;

			if (iUser == -1)
			{
				int i;
				settings.cbSize=sizeof(settings);
				for (i = 0, flags = 0; i < tempSettings.getCount(); ++i) 
				{
					if (tempSettings[i]->flags & NUF_NOOPTIONS) continue;
					CombineSettingsStructs(&settings, &flags, &tempSettings[i]->settings, tempSettings[i]->flags);
				}
			}
			else
			{
				NetlibFreeUserSettingsStruct(&settings);
				CopySettingsStruct(&settings, &tempSettings[iUser]->settings);
				flags = tempSettings[iUser]->flags;
			}
			ShowMultipleControls(hwndDlg,outgoingConnectionsControls,SIZEOF(outgoingConnectionsControls),flags&NUF_OUTGOING?SW_SHOW:SW_HIDE);
			CheckDlgButton(hwndDlg,IDC_USEPROXY,settings.useProxy);
			SendDlgItemMessage(hwndDlg,IDC_PROXYTYPE,CB_RESETCONTENT,0,0);
			if (settings.proxyType == 0) AddProxyTypeItem(hwndDlg,0,settings.proxyType);
			AddProxyTypeItem(hwndDlg, PROXYTYPE_SOCKS4, settings.proxyType);
			AddProxyTypeItem(hwndDlg, PROXYTYPE_SOCKS5, settings.proxyType);
			if (flags & (NUF_HTTPCONNS | NUF_HTTPGATEWAY)) AddProxyTypeItem(hwndDlg,PROXYTYPE_HTTP,settings.proxyType);
			if (!(flags & NUF_NOHTTPSOPTION)) AddProxyTypeItem(hwndDlg,PROXYTYPE_HTTPS,settings.proxyType);
			if (flags & (NUF_HTTPCONNS | NUF_HTTPGATEWAY) || !(flags & NUF_NOHTTPSOPTION)) 
				AddProxyTypeItem(hwndDlg,PROXYTYPE_IE,settings.proxyType);
			SetDlgItemTextA(hwndDlg,IDC_PROXYHOST,settings.szProxyServer?settings.szProxyServer:"");
			if (settings.wProxyPort) SetDlgItemInt(hwndDlg,IDC_PROXYPORT,settings.wProxyPort,FALSE);
			else SetDlgItemTextA(hwndDlg,IDC_PROXYPORT,"");
			CheckDlgButton(hwndDlg,IDC_PROXYAUTH,settings.useProxyAuth);
			SetDlgItemTextA(hwndDlg,IDC_PROXYUSER,settings.szProxyAuthUser?settings.szProxyAuthUser:"");
			SetDlgItemTextA(hwndDlg,IDC_PROXYPASS,settings.szProxyAuthPassword?settings.szProxyAuthPassword:"");
			CheckDlgButton(hwndDlg,IDC_PROXYDNS,settings.dnsThroughProxy);
			CheckDlgButton(hwndDlg,IDC_VALIDATESSL,settings.validateSSL);

			ShowMultipleControls(hwndDlg,incomingConnectionsControls,SIZEOF(incomingConnectionsControls),flags&NUF_INCOMING?SW_SHOW:SW_HIDE);
			CheckDlgButton(hwndDlg,IDC_SPECIFYPORTS,settings.specifyIncomingPorts);
			SetDlgItemTextA(hwndDlg,IDC_PORTSRANGE,settings.szIncomingPorts?settings.szIncomingPorts:"");

			CheckDlgButton(hwndDlg,IDC_SPECIFYPORTSO,settings.specifyOutgoingPorts);
			SetDlgItemTextA(hwndDlg,IDC_PORTSRANGEO,settings.szOutgoingPorts?settings.szOutgoingPorts:"");

			CheckDlgButton(hwndDlg,IDC_ENABLEUPNP,settings.enableUPnP);

			NetlibFreeUserSettingsStruct(&settings);
			SendMessage(hwndDlg,M_REFRESHENABLING,0,0);
			break;
		}
		case M_REFRESHENABLING:
		{	int selectedProxyType;
			TCHAR str[80];

			selectedProxyType=SendDlgItemMessage(hwndDlg,IDC_PROXYTYPE,CB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_PROXYTYPE,CB_GETCURSEL,0,0),0);
			mir_sntprintf(str, SIZEOF(str), TranslateT("(often %d)"),oftenProxyPorts[selectedProxyType]);
			SetDlgItemText(hwndDlg,IDC_STOFTENPORT,str);
			if (IsDlgButtonChecked(hwndDlg,IDC_USEPROXY) != BST_UNCHECKED)
			{
				int enableAuth = 0, enableUser = 0, enablePass = 0, enableServer = 1;
				EnableMultipleControls(hwndDlg, useProxyControls, SIZEOF(useProxyControls), TRUE);
				if (selectedProxyType == 0)
				{
					int i;
					for (i = 0; i < tempSettings.getCount(); ++i)
					{
						if (!tempSettings[i]->settings.useProxy ||
							tempSettings[i]->flags & NUF_NOOPTIONS || !(tempSettings[i]->flags & NUF_OUTGOING))
							continue;

						if (tempSettings[i]->settings.proxyType==PROXYTYPE_SOCKS4) enableUser=1;
						else 
						{
							enableAuth=1;
							if (tempSettings[i]->settings.useProxyAuth)
							{
								enableUser=enablePass=1;
							}
						}
					}
				}
				else 
				{
					if (selectedProxyType == PROXYTYPE_SOCKS4) enableUser=1;
					else 
					{
						if (selectedProxyType == PROXYTYPE_IE) enableServer=0;
						enableAuth=1;
						if (IsDlgButtonChecked(hwndDlg,IDC_PROXYAUTH) != BST_UNCHECKED)
							enableUser = enablePass = 1;
					}
				}
				EnableWindow(GetDlgItem(hwndDlg,IDC_PROXYAUTH), enableAuth);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC31),  enableUser);
				EnableWindow(GetDlgItem(hwndDlg,IDC_PROXYUSER), enableUser);
				EnableWindow(GetDlgItem(hwndDlg,IDC_STATIC32),  enablePass);
				EnableWindow(GetDlgItem(hwndDlg,IDC_PROXYPASS), enablePass);
				EnableWindow(GetDlgItem(hwndDlg,IDC_PROXYHOST), enableServer);
				EnableWindow(GetDlgItem(hwndDlg,IDC_PROXYPORT), enableServer);
			}
			else EnableMultipleControls(hwndDlg,useProxyControls,SIZEOF(useProxyControls),FALSE);
			EnableMultipleControls(hwndDlg,specifyPortsControls,SIZEOF(specifyPortsControls),IsDlgButtonChecked(hwndDlg,IDC_SPECIFYPORTS)!=BST_UNCHECKED);
			EnableMultipleControls(hwndDlg,specifyOPortsControls,SIZEOF(specifyOPortsControls),IsDlgButtonChecked(hwndDlg,IDC_SPECIFYPORTSO)!=BST_UNCHECKED);
			break;
		}
		case WM_COMMAND:
		{	int iUser=SendDlgItemMessage(hwndDlg,IDC_NETLIBUSERS,CB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_NETLIBUSERS,CB_GETCURSEL,0,0),0);
			switch(LOWORD(wParam)) 
			{
				case IDC_NETLIBUSERS:
					if(HIWORD(wParam)==CBN_SELCHANGE) SendMessage(hwndDlg,M_REFRESHALL,0,0);
					return 0;

				case IDC_LOGOPTIONS:
					NetlibLogShowOptions();
					return 0;

				case IDC_PROXYTYPE:
					if (HIWORD(wParam) != CBN_SELCHANGE) return 0;
					{	int newValue,i;
						newValue = SendDlgItemMessage(hwndDlg,IDC_PROXYTYPE,CB_GETITEMDATA,SendDlgItemMessage(hwndDlg,IDC_PROXYTYPE,CB_GETCURSEL,0,0),0);
						if (iUser == -1)
						{
							if (newValue == 0) return 0;
							for (i = 0; i < tempSettings.getCount(); ++i)
							{
								if (tempSettings[i]->flags & NUF_NOOPTIONS) continue;
								if (newValue == PROXYTYPE_HTTP && !(tempSettings[i]->flags & (NUF_HTTPCONNS|NUF_HTTPGATEWAY)))
									tempSettings[i]->settings.proxyType = PROXYTYPE_HTTPS;
								else if (newValue == PROXYTYPE_HTTPS && tempSettings[i]->flags & NUF_NOHTTPSOPTION)
									tempSettings[i]->settings.proxyType = PROXYTYPE_HTTP;
								else tempSettings[i]->settings.proxyType = newValue;
							}
							SendMessage(hwndDlg, M_REFRESHALL, 0, 0);
						}
						else 
						{
							tempSettings[iUser]->settings.proxyType = newValue;
							SendMessage(hwndDlg,M_REFRESHENABLING,0,0);
						}
					}
					break;
				case IDC_USEPROXY:
					ChangeSettingIntByCheckbox(hwndDlg,LOWORD(wParam),iUser,offsetof(NETLIBUSERSETTINGS,useProxy));
					break;
				case IDC_PROXYAUTH:
					ChangeSettingIntByCheckbox(hwndDlg,LOWORD(wParam),iUser,offsetof(NETLIBUSERSETTINGS,useProxyAuth));
					break;
				case IDC_PROXYDNS:
					ChangeSettingIntByCheckbox(hwndDlg,LOWORD(wParam),iUser,offsetof(NETLIBUSERSETTINGS,dnsThroughProxy));
					break;
				case IDC_SPECIFYPORTS:
					ChangeSettingIntByCheckbox(hwndDlg,LOWORD(wParam),iUser,offsetof(NETLIBUSERSETTINGS,specifyIncomingPorts));
					break;
				case IDC_SPECIFYPORTSO:
					ChangeSettingIntByCheckbox(hwndDlg,LOWORD(wParam),iUser,offsetof(NETLIBUSERSETTINGS,specifyOutgoingPorts));
					break;
                case IDC_ENABLEUPNP:
					ChangeSettingIntByCheckbox(hwndDlg,LOWORD(wParam),iUser,offsetof(NETLIBUSERSETTINGS,enableUPnP));
                    break;
                case IDC_VALIDATESSL:
					ChangeSettingIntByCheckbox(hwndDlg,LOWORD(wParam),iUser,offsetof(NETLIBUSERSETTINGS,validateSSL));
                    break;
                case IDC_PROXYHOST:
					if(HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()) return 0;
					ChangeSettingStringByEdit(hwndDlg,LOWORD(wParam),iUser,offsetof(NETLIBUSERSETTINGS,szProxyServer));
					break;
				case IDC_PROXYPORT:
					if(HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()) return 0;
					{	int newValue,i;
						newValue=GetDlgItemInt(hwndDlg,LOWORD(wParam),NULL,FALSE);
						if (iUser == -1)
						{
							for (i = 0; i < tempSettings.getCount(); ++i)
								if (!(tempSettings[i]->flags & NUF_NOOPTIONS))
									tempSettings[i]->settings.wProxyPort = newValue;
						}
						else tempSettings[iUser]->settings.wProxyPort = newValue;
					}
					break;
				case IDC_PROXYUSER:
					if(HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()) return 0;
					ChangeSettingStringByEdit(hwndDlg,LOWORD(wParam),iUser,offsetof(NETLIBUSERSETTINGS,szProxyAuthUser));
					break;
				case IDC_PROXYPASS:
					if(HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()) return 0;
					ChangeSettingStringByEdit(hwndDlg,LOWORD(wParam),iUser,offsetof(NETLIBUSERSETTINGS,szProxyAuthPassword));
					break;
				case IDC_PORTSRANGE:
					if(HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()) return 0;
					ChangeSettingStringByEdit(hwndDlg,LOWORD(wParam),iUser,offsetof(NETLIBUSERSETTINGS,szIncomingPorts));
					break;
				case IDC_PORTSRANGEO:
					if(HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()) return 0;
					ChangeSettingStringByEdit(hwndDlg,LOWORD(wParam),iUser,offsetof(NETLIBUSERSETTINGS,szOutgoingPorts));
					break;
			}
			ShowWindow(GetDlgItem(hwndDlg,IDC_RECONNECTREQD),SW_SHOW);
			SendMessage(GetParent(hwndDlg),PSM_CHANGED,0,0);
			break;
		}
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->idFrom) {
				case 0:
					switch (((LPNMHDR)lParam)->code)
					{
						case PSN_APPLY:
						{	int iUser;
							for (iUser = 0; iUser < tempSettings.getCount(); iUser++)
								NetlibSaveUserSettingsStruct(tempSettings[iUser]->szSettingsModule, 
									&tempSettings[iUser]->settings);
							return TRUE;
						}
					}
					break;
			}
			break;
		case WM_DESTROY:
		{	int iUser;
			for (iUser = 0; iUser < tempSettings.getCount(); ++iUser) 
			{
				mir_free(tempSettings[iUser]->szSettingsModule);
				NetlibFreeUserSettingsStruct(&tempSettings[iUser]->settings);
				mir_free(tempSettings[iUser]);
			}
			tempSettings.destroy();
			break;
		}
	}
	return FALSE;
}

static UINT expertOnlyControls[]={IDC_LOGOPTIONS};
int NetlibOptInitialise(WPARAM wParam,LPARAM)
{
	int optionsCount = 0;
	EnterCriticalSection(&csNetlibUser);
	for (int i = 0; i < netlibUser.getCount(); ++i)
		if (!(netlibUser[i]->user.flags & NUF_NOOPTIONS)) ++optionsCount;
	LeaveCriticalSection(&csNetlibUser);
	if (optionsCount == 0) return 0;

	OPTIONSDIALOGPAGE odp = { 0 };

	odp.cbSize = sizeof(odp);
	odp.position = 900000000;
	odp.hInstance = hMirandaInst;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_NETLIB);
	odp.pszTitle = LPGEN("Network");
	odp.pfnDlgProc = DlgProcNetlibOpts;
	odp.flags = ODPF_BOLDGROUPS;
	odp.expertOnlyControls = expertOnlyControls;
	odp.nExpertOnlyControls = SIZEOF( expertOnlyControls );
	CallService( MS_OPT_ADDPAGE, wParam, ( LPARAM )&odp );
	return 0;
}
