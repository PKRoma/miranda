// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
//
// Copyright © 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001-2002 Jon Keating, Richard Hughes
// Copyright © 2002-2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004-2008 Joe Kucera
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
//
// -----------------------------------------------------------------------------
//
// File name      : $URL$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"

HINSTANCE hInst;
PLUGINLINK* pluginLink;
MM_INTERFACE mmi;
MD5_INTERFACE md5i;
LIST_INTERFACE li;

BYTE gbUnicodeCore;
DWORD MIRANDA_VERSION;

icq_mode_messages modeMsgs;

PLUGININFOEX pluginInfo = {
	sizeof(PLUGININFOEX),
	"IcqOscarJ Protocol",
	PLUGIN_MAKE_VERSION(0,8,0,0),
	"Support for ICQ network, enhanced.",
	"Joe Kucera, Bio, Martin Öberg, Richard Hughes, Jon Keating, etc",
	"jokusoftware@miranda-im.org",
	"(C) 2000-2008 M.Öberg, R.Hughes, J.Keating, Bio, Angeli-Ka, J.Kucera",
	"http://addons.miranda-im.org/details.php?action=viewfile&id=1683",
	UNICODE_AWARE,
	0,   //doesn't replace anything built-in
	{0x847bb03c, 0x408c, 0x4f9b, { 0xaa, 0x5a, 0xf5, 0xc0, 0xb7, 0xb5, 0x60, 0x1e }} //{847BB03C-408C-4f9b-AA5A-F5C0B7B5601E}
};

extern "C" PLUGININFOEX __declspec(dllexport) *MirandaPluginInfoEx(DWORD mirandaVersion)
{
	// Only load for 0.8.0.10 or greater
	// We need the new CList Group events
	if (mirandaVersion < PLUGIN_MAKE_VERSION(0, 8, 0, 10))
	{
		MessageBoxA( NULL, "ICQ plugin cannot be loaded. It requires Miranda IM 0.8.0.10 or later.", "ICQ Plugin",
			MB_OK|MB_ICONWARNING|MB_SETFOREGROUND|MB_TOPMOST );
		return NULL;
	}

	MIRANDA_VERSION = mirandaVersion;
	return &pluginInfo;
}

static const MUUID interfaces[] = {MIID_PROTOCOL, MIID_LAST};
extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	hInst = hinstDLL;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////

static PROTO_INTERFACE* icqProtoInit( const char* pszProtoName, const TCHAR* tszUserName )
{
	return new CIcqProto( pszProtoName, tszUserName );
}

static int icqProtoUninit( PROTO_INTERFACE* ppro )
{
	delete ( CIcqProto* )ppro;
	return 0;
}

extern "C" int __declspec(dllexport) Load(PLUGINLINK *link)
{
	pluginLink = link;
	mir_getLI( &li );
	mir_getMMI( &mmi );
	mir_getMD5I( &md5i );

	{ // Are we running under unicode Miranda core ?
		char szVer[MAX_PATH];

		CallService(MS_SYSTEM_GETVERSIONTEXT, MAX_PATH, (LPARAM)szVer);
		_strlwr(szVer); // make sure it is lowercase
		gbUnicodeCore = (strstrnull(szVer, "unicode") != NULL);

		if (strstrnull(szVer, "alpha") != NULL)
		{ // Are we running under Alpha Core
			MIRANDA_VERSION |= 0x80000000;
		}
		else if (MIRANDA_VERSION >= 0x00050000 && strstrnull(szVer, "preview") == NULL)
		{ // for Final Releases of Miranda 0.5+ clear build number
			MIRANDA_VERSION &= 0xFFFFFF00;
		}
	}

	srand(time(NULL));
	_tzset();

	// Register the module
	PROTOCOLDESCRIPTOR pd = {0};
	pd.cbSize   = sizeof(pd);
	pd.szName   = "ICQ";
	pd.type     = PROTOTYPE_PROTOCOL;
	pd.fnInit   = icqProtoInit;
	pd.fnUninit = icqProtoUninit;
	CallService(MS_PROTO_REGISTERMODULE, 0, (LPARAM)&pd);

	// Initialize charset conversion routines
	InitI18N();
	return 0;
}

extern "C" int __declspec(dllexport) Unload(void)
{
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnPrebuildContactMenu event

void CListShowMenuItem(HANDLE hMenuItem, BYTE bShow)
{
	CLISTMENUITEM mi = {0};

	mi.cbSize = sizeof(mi);
	if (bShow)
		mi.flags = CMIM_FLAGS;
	else
		mi.flags = CMIM_FLAGS | CMIF_HIDDEN;

	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hMenuItem, (LPARAM)&mi);
}

static void CListSetMenuItemIcon(HANDLE hMenuItem, HICON hIcon)
{
	CLISTMENUITEM mi = {0};

	mi.cbSize = sizeof(mi);
	mi.flags = CMIM_FLAGS | CMIM_ICON;

	mi.hIcon = hIcon;
	CallService(MS_CLIST_MODIFYMENUITEM, (WPARAM)hMenuItem, (LPARAM)&mi);
}

int CIcqProto::OnPrebuildContactMenu(WPARAM wParam, LPARAM lParam)
{
	BYTE bXStatus;

	CListShowMenuItem(hUserMenuAuth, getByte((HANDLE)wParam, "Auth", 0));
	CListShowMenuItem(hUserMenuGrant, getByte((HANDLE)wParam, "Grant", 0));
	CListShowMenuItem(hUserMenuRevoke, (BYTE)(getByte(NULL, "PrivacyItems", 0) && !getByte((HANDLE)wParam, "Grant", 0)));
	if (m_bSsiEnabled && !getWord((HANDLE)wParam, "ServerId", 0) && !getWord((HANDLE)wParam, "SrvIgnoreId", 0))
		CListShowMenuItem(hUserMenuAddServ, 1);
	else
		CListShowMenuItem(hUserMenuAddServ, 0);

	bXStatus = ICQGetContactXStatus((HANDLE)wParam);
	CListShowMenuItem(hUserMenuXStatus, (BYTE)(m_bHideXStatusUI ? 0 : bXStatus));
	if (bXStatus && !m_bHideXStatusUI)
	{
		CListSetMenuItemIcon(hUserMenuXStatus, getXStatusIcon(bXStatus, LR_SHARED));
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnPrebuildContactMenu event

int CIcqProto::OnReloadIcons(WPARAM wParam, LPARAM lParam)
{
	memset(bXStatusCListIconsValid,0,sizeof(bXStatusCListIconsValid));
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnPrebuildContactMenu event

void CIcqProto::UpdateGlobalSettings()
{
	if (m_hServerNetlibUser)
	{
		NETLIBUSERSETTINGS nlus = {0};

		nlus.cbSize = sizeof(NETLIBUSERSETTINGS);
		if (CallService(MS_NETLIB_GETUSERSETTINGS, (WPARAM)m_hServerNetlibUser, (LPARAM)&nlus))
		{
			if (nlus.useProxy && nlus.proxyType == PROXYTYPE_HTTP)
				m_bGatewayMode = 1;
			else
				m_bGatewayMode = 0;
		}
		else
			m_bGatewayMode = 0;
	}

	m_bSecureLogin = getByte(NULL, "SecureLogin", DEFAULT_SECURE_LOGIN);
	m_bAimEnabled = getByte(NULL, "AimEnabled", DEFAULT_AIM_ENABLED);
	m_bUtfEnabled = getByte(NULL, "UtfEnabled", DEFAULT_UTF_ENABLED);
	m_wAnsiCodepage = getWord(NULL, "AnsiCodePage", DEFAULT_ANSI_CODEPAGE);
	m_bDCMsgEnabled = getByte(NULL, "DirectMessaging", DEFAULT_DCMSG_ENABLED);
	m_bTempVisListEnabled = getByte(NULL, "TempVisListEnabled", DEFAULT_TEMPVIS_ENABLED);
	m_bSsiEnabled = getByte(NULL, "UseServerCList", DEFAULT_SS_ENABLED);
	m_bSsiSimpleGroups = FALSE; /// TODO: enable, after server-list revolution is over
	m_bAvatarsEnabled = getByte(NULL, "AvatarsEnabled", DEFAULT_AVATARS_ENABLED);
	m_bXStatusEnabled = getByte(NULL, "XStatusEnabled", DEFAULT_XSTATUS_ENABLED);
}
