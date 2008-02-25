/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan
Copyright ( C ) 2007     Maxim Mluhov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

File name      : $URL$
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "jabber_list.h"
#include "jabber_ssl.h"
#include <commctrl.h>
#include "resource.h"
#include <uxtheme.h>
#include "jabber_caps.h"
#include "jabber_opttree.h"
#include "sdk/m_wizard.h"
#include "sdk/m_modernopt.h"

static BOOL (WINAPI *pfnEnableThemeDialogTexture)(HANDLE, DWORD) = 0;

/////////////////////////////////////////////////////////////////////////////////////////
// JabberRegisterDlgProc - the dialog proc for registering new account

#if defined( _UNICODE )
	#define STR_FORMAT _T("%s %s@%S:%d?")
#else
	#define STR_FORMAT _T("%s %s@%s:%d?")
#endif

struct { TCHAR *szCode; TCHAR *szDescription; } g_LanguageCodes[] = {
	{	_T("aa"),	_T("Afar")	},
	{	_T("ab"),	_T("Abkhazian")	},
	{	_T("af"),	_T("Afrikaans")	},
	{	_T("ak"),	_T("Akan")	},
	{	_T("sq"),	_T("Albanian")	},
	{	_T("am"),	_T("Amharic")	},
	{	_T("ar"),	_T("Arabic")	},
	{	_T("an"),	_T("Aragonese")	},
	{	_T("hy"),	_T("Armenian")	},
	{	_T("as"),	_T("Assamese")	},
	{	_T("av"),	_T("Avaric")	},
	{	_T("ae"),	_T("Avestan")	},
	{	_T("ay"),	_T("Aymara")	},
	{	_T("az"),	_T("Azerbaijani")	},
	{	_T("ba"),	_T("Bashkir")	},
	{	_T("bm"),	_T("Bambara")	},
	{	_T("eu"),	_T("Basque")	},
	{	_T("be"),	_T("Belarusian")	},
	{	_T("bn"),	_T("Bengali")	},
	{	_T("bh"),	_T("Bihari")	},
	{	_T("bi"),	_T("Bislama")	},
	{	_T("bs"),	_T("Bosnian")	},
	{	_T("br"),	_T("Breton")	},
	{	_T("bg"),	_T("Bulgarian")	},
	{	_T("my"),	_T("Burmese")	},
	{	_T("ca"),	_T("Catalan; Valencian")	},
	{	_T("ch"),	_T("Chamorro")	},
	{	_T("ce"),	_T("Chechen")	},
	{	_T("zh"),	_T("Chinese")	},
	{	_T("cu"),	_T("Church Slavic; Old Slavonic")	},
	{	_T("cv"),	_T("Chuvash")	},
	{	_T("kw"),	_T("Cornish")	},
	{	_T("co"),	_T("Corsican")	},
	{	_T("cr"),	_T("Cree")	},
	{	_T("cs"),	_T("Czech")	},
	{	_T("da"),	_T("Danish")	},
	{	_T("dv"),	_T("Divehi; Dhivehi; Maldivian")	},
	{	_T("nl"),	_T("Dutch; Flemish")	},
	{	_T("dz"),	_T("Dzongkha")	},
	{	_T("en"),	_T("English")	},
	{	_T("eo"),	_T("Esperanto")	},
	{	_T("et"),	_T("Estonian")	},
	{	_T("ee"),	_T("Ewe")	},
	{	_T("fo"),	_T("Faroese")	},
	{	_T("fj"),	_T("Fijian")	},
	{	_T("fi"),	_T("Finnish")	},
	{	_T("fr"),	_T("French")	},
	{	_T("fy"),	_T("Western Frisian")	},
	{	_T("ff"),	_T("Fulah")	},
	{	_T("ka"),	_T("Georgian")	},
	{	_T("de"),	_T("German")	},
	{	_T("gd"),	_T("Gaelic; Scottish Gaelic")	},
	{	_T("ga"),	_T("Irish")	},
	{	_T("gl"),	_T("Galician")	},
	{	_T("gv"),	_T("Manx")	},
	{	_T("el"),	_T("Greek, Modern (1453-)")	},
	{	_T("gn"),	_T("Guarani")	},
	{	_T("gu"),	_T("Gujarati")	},
	{	_T("ht"),	_T("Haitian; Haitian Creole")	},
	{	_T("ha"),	_T("Hausa")	},
	{	_T("he"),	_T("Hebrew")	},
	{	_T("hz"),	_T("Herero")	},
	{	_T("hi"),	_T("Hindi")	},
	{	_T("ho"),	_T("Hiri Motu")	},
	{	_T("hu"),	_T("Hungarian")	},
	{	_T("ig"),	_T("Igbo")	},
	{	_T("is"),	_T("Icelandic")	},
	{	_T("io"),	_T("Ido")	},
	{	_T("ii"),	_T("Sichuan Yi")	},
	{	_T("iu"),	_T("Inuktitut")	},
	{	_T("ie"),	_T("Interlingue")	},
	{	_T("ia"),	_T("Interlingua (International Auxiliary Language Association)")	},
	{	_T("id"),	_T("Indonesian")	},
	{	_T("ik"),	_T("Inupiaq")	},
	{	_T("it"),	_T("Italian")	},
	{	_T("jv"),	_T("Javanese")	},
	{	_T("ja"),	_T("Japanese")	},
	{	_T("kl"),	_T("Kalaallisut; Greenlandic")	},
	{	_T("kn"),	_T("Kannada")	},
	{	_T("ks"),	_T("Kashmiri")	},
	{	_T("kr"),	_T("Kanuri")	},
	{	_T("kk"),	_T("Kazakh")	},
	{	_T("km"),	_T("Central Khmer")	},
	{	_T("ki"),	_T("Kikuyu; Gikuyu")	},
	{	_T("rw"),	_T("Kinyarwanda")	},
	{	_T("ky"),	_T("Kirghiz; Kyrgyz")	},
	{	_T("kv"),	_T("Komi")	},
	{	_T("kg"),	_T("Kongo")	},
	{	_T("ko"),	_T("Korean")	},
	{	_T("kj"),	_T("Kuanyama; Kwanyama")	},
	{	_T("ku"),	_T("Kurdish")	},
	{	_T("lo"),	_T("Lao")	},
	{	_T("la"),	_T("Latin")	},
	{	_T("lv"),	_T("Latvian")	},
	{	_T("li"),	_T("Limburgan; Limburger; Limburgish")	},
	{	_T("ln"),	_T("Lingala")	},
	{	_T("lt"),	_T("Lithuanian")	},
	{	_T("lb"),	_T("Luxembourgish; Letzeburgesch")	},
	{	_T("lu"),	_T("Luba-Katanga")	},
	{	_T("lg"),	_T("Ganda")	},
	{	_T("mk"),	_T("Macedonian")	},
	{	_T("mh"),	_T("Marshallese")	},
	{	_T("ml"),	_T("Malayalam")	},
	{	_T("mi"),	_T("Maori")	},
	{	_T("mr"),	_T("Marathi")	},
	{	_T("ms"),	_T("Malay")	},
	{	_T("mg"),	_T("Malagasy")	},
	{	_T("mt"),	_T("Maltese")	},
	{	_T("mo"),	_T("Moldavian")	},
	{	_T("mn"),	_T("Mongolian")	},
	{	_T("na"),	_T("Nauru")	},
	{	_T("nv"),	_T("Navajo; Navaho")	},
	{	_T("nr"),	_T("Ndebele, South; South Ndebele")	},
	{	_T("nd"),	_T("Ndebele, North; North Ndebele")	},
	{	_T("ng"),	_T("Ndonga")	},
	{	_T("ne"),	_T("Nepali")	},
	{	_T("nn"),	_T("Norwegian Nynorsk; Nynorsk, Norwegian")	},
	{	_T("nb"),	_T("BokmЕl, Norwegian; Norwegian BokmЕl")	},
	{	_T("no"),	_T("Norwegian")	},
	{	_T("ny"),	_T("Chichewa; Chewa; Nyanja")	},
	{	_T("oc"),	_T("Occitan (post 1500); ProvenГal")	},
	{	_T("oj"),	_T("Ojibwa")	},
	{	_T("or"),	_T("Oriya")	},
	{	_T("om"),	_T("Oromo")	},
	{	_T("os"),	_T("Ossetian; Ossetic")	},
	{	_T("pa"),	_T("Panjabi; Punjabi")	},
	{	_T("fa"),	_T("Persian")	},
	{	_T("pi"),	_T("Pali")	},
	{	_T("pl"),	_T("Polish")	},
	{	_T("pt"),	_T("Portuguese")	},
	{	_T("ps"),	_T("Pushto")	},
	{	_T("qu"),	_T("Quechua")	},
	{	_T("rm"),	_T("Romansh")	},
	{	_T("ro"),	_T("Romanian")	},
	{	_T("rn"),	_T("Rundi")	},
	{	_T("ru"),	_T("Russian")	},
	{	_T("sg"),	_T("Sango")	},
	{	_T("sa"),	_T("Sanskrit")	},
	{	_T("sr"),	_T("Serbian")	},
	{	_T("hr"),	_T("Croatian")	},
	{	_T("si"),	_T("Sinhala; Sinhalese")	},
	{	_T("sk"),	_T("Slovak")	},
	{	_T("sl"),	_T("Slovenian")	},
	{	_T("se"),	_T("Northern Sami")	},
	{	_T("sm"),	_T("Samoan")	},
	{	_T("sn"),	_T("Shona")	},
	{	_T("sd"),	_T("Sindhi")	},
	{	_T("so"),	_T("Somali")	},
	{	_T("st"),	_T("Sotho, Southern")	},
	{	_T("es"),	_T("Spanish; Castilian")	},
	{	_T("sc"),	_T("Sardinian")	},
	{	_T("ss"),	_T("Swati")	},
	{	_T("su"),	_T("Sundanese")	},
	{	_T("sw"),	_T("Swahili")	},
	{	_T("sv"),	_T("Swedish")	},
	{	_T("ty"),	_T("Tahitian")	},
	{	_T("ta"),	_T("Tamil")	},
	{	_T("tt"),	_T("Tatar")	},
	{	_T("te"),	_T("Telugu")	},
	{	_T("tg"),	_T("Tajik")	},
	{	_T("tl"),	_T("Tagalog")	},
	{	_T("th"),	_T("Thai")	},
	{	_T("bo"),	_T("Tibetan")	},
	{	_T("ti"),	_T("Tigrinya")	},
	{	_T("to"),	_T("Tonga (Tonga Islands)")	},
	{	_T("tn"),	_T("Tswana")	},
	{	_T("ts"),	_T("Tsonga")	},
	{	_T("tk"),	_T("Turkmen")	},
	{	_T("tr"),	_T("Turkish")	},
	{	_T("tw"),	_T("Twi")	},
	{	_T("ug"),	_T("Uighur; Uyghur")	},
	{	_T("uk"),	_T("Ukrainian")	},
	{	_T("ur"),	_T("Urdu")	},
	{	_T("uz"),	_T("Uzbek")	},
	{	_T("ve"),	_T("Venda")	},
	{	_T("vi"),	_T("Vietnamese")	},
	{	_T("vo"),	_T("VolapЭk")	},
	{	_T("cy"),	_T("Welsh")	},
	{	_T("wa"),	_T("Walloon")	},
	{	_T("wo"),	_T("Wolof")	},
	{	_T("xh"),	_T("Xhosa")	},
	{	_T("yi"),	_T("Yiddish")	},
	{	_T("yo"),	_T("Yoruba")	},
	{	_T("za"),	_T("Zhuang; Chuang")	},
	{	_T("zu"),	_T("Zulu")	},
	{	NULL,	NULL	}
};

static BOOL CALLBACK JabberRegisterDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	ThreadData *thread, *regInfo;

	switch ( msg ) {
	case WM_INITDIALOG:
	{
		TranslateDialogDefault( hwndDlg );
		regInfo = ( ThreadData* ) lParam;
		TCHAR text[256];
		mir_sntprintf( text, SIZEOF(text), STR_FORMAT, TranslateT( "Register" ), regInfo->username, regInfo->server, regInfo->port );
		SetDlgItemText( hwndDlg, IDC_REG_STATUS, text );
		SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG )regInfo );
		return TRUE;
	}
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDOK:
			ShowWindow( GetDlgItem( hwndDlg, IDOK ), SW_HIDE );
			ShowWindow( GetDlgItem( hwndDlg, IDCANCEL ), SW_HIDE );
			ShowWindow( GetDlgItem( hwndDlg, IDC_PROGRESS_REG ), SW_SHOW );
			ShowWindow( GetDlgItem( hwndDlg, IDCANCEL2 ), SW_SHOW );
			regInfo = ( ThreadData* ) GetWindowLong( hwndDlg, GWL_USERDATA );
			thread = new ThreadData( regInfo->proto, JABBER_SESSION_REGISTER );
			_tcsncpy( thread->username, regInfo->username, SIZEOF( thread->username ));
			strncpy( thread->password, regInfo->password, SIZEOF( thread->password ));
			strncpy( thread->server, regInfo->server, SIZEOF( thread->server ));
			strncpy( thread->manualHost, regInfo->manualHost, SIZEOF( thread->manualHost ));
			thread->port = regInfo->port;
			thread->useSSL = regInfo->useSSL;
			thread->reg_hwndDlg = hwndDlg;
			mir_forkthread(( pThreadFunc )JabberServerThread, thread );
			return TRUE;
		case IDCANCEL:
		case IDOK2:
			EndDialog( hwndDlg, 0 );
			return TRUE;
		}
		break;
	case WM_JABBER_REGDLG_UPDATE:	// wParam=progress ( 0-100 ), lparam=status string
		if (( TCHAR* )lParam == NULL )
			SetDlgItemText( hwndDlg, IDC_REG_STATUS, TranslateT( "No message" ));
		else
			SetDlgItemText( hwndDlg, IDC_REG_STATUS, ( TCHAR* )lParam );
		if ( wParam >= 0 )
			SendMessage( GetDlgItem( hwndDlg, IDC_PROGRESS_REG ), PBM_SETPOS, wParam, 0 );
		if ( wParam >= 100 ) {
			ShowWindow( GetDlgItem( hwndDlg, IDCANCEL2 ), SW_HIDE );
			ShowWindow( GetDlgItem( hwndDlg, IDOK2 ), SW_SHOW );
		}
		else SetFocus( GetDlgItem( hwndDlg, IDC_PROGRESS_REG ));
		return TRUE;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberOptDlgProc - main options dialog procedure

class CCtrlEditJid: public CCtrlEdit
{
public:
	CCtrlEditJid( CDlgBase* dlg, int ctrlId );

	void OnInit()
	{
		CCtrlEdit::OnInit();
		Subclass();
	}

protected:
	virtual LRESULT CustomWndProc(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if ( msg == WM_CHAR )
		{
			switch( wParam )
			{
			case '\"':  case '&':	case '\'':	case '/':
			case ':':	case '<':	case '>':	case '@':
				MessageBeep(MB_ICONASTERISK);
				return 0;
			}
		}
		return CCtrlEdit::CustomWndProc(msg, wParam, lParam);
	}
};

CCtrlEditJid::CCtrlEditJid( CDlgBase* dlg, int ctrlId ):
	CCtrlEdit( dlg, ctrlId )
{
}

class CDlgOptAccount: public CJabberDlgBase
{
	CCtrlEditJid	m_txtUsername;
	CCtrlEdit		m_txtPassword;
	CCtrlEdit		m_txtPriority;
	CCtrlCheck		m_chkSavePassword;
	CCtrlCombo		m_cbResource;
	CCtrlCheck		m_chkUseHostnameAsResource;
	CCtrlCombo		m_cbServer;
	CCtrlEdit		m_txtPort;
	CCtrlCheck		m_chkUseSsl;
	CCtrlCheck		m_chkUseTls;
	CCtrlCheck		m_chkManualHost;
	CCtrlEdit		m_txtManualHost;
	CCtrlEdit		m_txtManualPort;
	CCtrlCheck		m_chkKeepAlive;
	CCtrlCheck		m_chkAutoDeleteContacts;
	CCtrlEdit		m_txtUserDirectory;
	CCtrlCombo		m_cbLocale;
	CCtrlButton		m_btnRegister;
	CCtrlButton		m_btnUnregister;
	CCtrlHyperlink	m_lnkServers;
	CCtrlHyperlink	m_lnkOpenSsl;

public:
	CDlgOptAccount(CJabberProto *proto):
		CJabberDlgBase(proto, IDD_OPT_JABBER, NULL),
		m_txtUsername(this, IDC_EDIT_USERNAME),
		m_txtPassword(this, IDC_EDIT_PASSWORD),
		m_txtPriority(this, IDC_PRIORITY),
		m_chkSavePassword(this, IDC_SAVEPASSWORD),
		m_cbResource(this, IDC_COMBO_RESOURCE),
		m_chkUseHostnameAsResource(this,IDC_HOSTNAME_AS_RESOURCE),
		m_cbServer(this, IDC_EDIT_LOGIN_SERVER),
		m_txtPort(this, IDC_PORT),
		m_chkUseSsl(this, IDC_USE_SSL),
		m_chkUseTls(this, IDC_USE_TLS),
		m_chkManualHost(this, IDC_MANUAL),
		m_txtManualHost(this, IDC_HOST),
		m_txtManualPort(this, IDC_HOSTPORT),
		m_chkKeepAlive(this, IDC_KEEPALIVE),
		m_chkAutoDeleteContacts(this, IDC_ROSTER_SYNC),
		m_txtUserDirectory(this, IDC_JUD),
		m_cbLocale(this, IDC_MSGLANG),
		m_btnRegister(this, IDC_BUTTON_REGISTER),
		m_btnUnregister(this, IDC_UNREGISTER),
		m_lnkServers(this, IDC_LINK_PUBLIC_SERVER, "http://www.jabber.org/user/publicservers.shtml"),
		m_lnkOpenSsl(this, IDC_DOWNLOAD_OPENSSL, "http://www.slproweb.com/products/Win32OpenSSL.html")

	{
		CreateLink(m_txtUsername, "LoginName", _T(""));
		CreateLink(m_txtPriority, "Priority", DBVT_WORD, 0);
		CreateLink(m_chkSavePassword, "SavePassword", DBVT_BYTE, 1);
		CreateLink(m_cbResource, "Resource", _T("Miranda"));
		CreateLink(m_chkUseHostnameAsResource, "HostNameAsResource", DBVT_BYTE, 0);
		CreateLink(m_cbServer, "LoginServer", _T("jabber.org"));
		CreateLink(m_txtPort, "Port", DBVT_WORD, 5222);
		CreateLink(m_chkUseSsl, "UseSSL", DBVT_BYTE, 0);
		CreateLink(m_chkUseTls, "UseTLS", DBVT_BYTE, 1);
		CreateLink(m_chkManualHost, "ManualConnect", DBVT_BYTE, 0);
		CreateLink(m_txtManualHost, "ManualHost", _T(""));
		CreateLink(m_txtManualPort, "ManualPort", DBVT_WORD, 0);
		CreateLink(m_chkKeepAlive, "KeepAlive", DBVT_BYTE, 1);
		CreateLink(m_chkAutoDeleteContacts, "RosterSync", DBVT_BYTE, 0);
		CreateLink(m_txtUserDirectory, "Jud", _T(""));

		// Bind events
		m_cbServer.OnDropdown = Callback(this, &CDlgOptAccount::cbServer_OnDropdown);
		m_chkManualHost.OnChange = Callback(this, &CDlgOptAccount::chkManualHost_OnChange);
		m_chkUseHostnameAsResource.OnChange = Callback(this, &CDlgOptAccount::chkUseHostnameAsResource_OnChange);
		m_chkUseSsl.OnChange = Callback(this, &CDlgOptAccount::chkUseSsl_OnChange);

		m_btnRegister.OnClick = Callback(this, &CDlgOptAccount::btnRegister_OnClick);
		m_btnUnregister.OnClick = Callback(this, &CDlgOptAccount::btnUnregister_OnClick);
	}

	static CDlgBase *Create(void *param) { return new CDlgOptAccount((CJabberProto *)param); }

protected:
	void OnInitDialog()
	{
		CJabberDlgBase::OnInitDialog();

		int i;
		DBVARIANT dbv;

		m_gotservers = false;

		SendDlgItemMessage(m_hwnd, IDC_PRIORITY_SPIN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(127, -128));

		if (!DBGetContactSettingString(NULL, m_proto->m_szModuleName, "Password", &dbv))
		{
			JCallService(MS_DB_CRYPT_DECODESTRING, lstrlenA(dbv.pszVal) + 1, (LPARAM)dbv.pszVal);
			TCHAR *tmp = mir_a2t(dbv.pszVal);
			m_txtPassword.SetText(tmp);
			mir_free(tmp);
			JFreeVariant(&dbv);
		}

		m_cbServer.AddString(TranslateT("Loading..."));

		// fill predefined resources
		TCHAR* szResources[] = { _T("Home"), _T("Work"), _T("Office"), _T("Miranda") };
		for (i = 0; i < SIZEOF(szResources); ++i)
			m_cbResource.AddString(szResources[i]);

		// append computer name to the resource list
		TCHAR szCompName[ MAX_COMPUTERNAME_LENGTH + 1];
		DWORD dwCompNameLength = MAX_COMPUTERNAME_LENGTH;
		if (GetComputerName(szCompName, &dwCompNameLength))
			m_cbResource.AddString(szCompName);

		if (!DBGetContactSettingTString(NULL, m_proto->m_szModuleName, "Resource", &dbv))
		{
			m_cbResource.AddString(dbv.ptszVal);
			m_cbResource.SetText(dbv.ptszVal);
			JFreeVariant(&dbv);
		} else
		{
			m_cbResource.SetText(_T("Miranda"));
		}

		TCHAR *szSelectedLang = m_proto->GetXmlLang();
		for (i = 0; g_LanguageCodes[i].szCode; ++i)
		{
			int iItem = m_cbLocale.AddString(TranslateTS(g_LanguageCodes[i].szDescription), (LPARAM)g_LanguageCodes[i].szCode);
			if (!_tcscmp(szSelectedLang, g_LanguageCodes[i].szCode))
				m_cbLocale.SetCurSel(iItem);
		}
		if ( szSelectedLang ) mir_free( szSelectedLang );

		EnableWindow(GetDlgItem(m_hwnd, IDC_COMBO_RESOURCE ), m_chkUseHostnameAsResource.GetState() != BST_CHECKED);
		EnableWindow(GetDlgItem(m_hwnd, IDC_UNREGISTER), m_proto->m_bJabberConnected);
		if (!m_proto->SslInit())
		{
			m_chkUseSsl.Disable();
			m_chkUseTls.Disable();
			EnableWindow(GetDlgItem(m_hwnd, IDC_DOWNLOAD_OPENSSL), TRUE);
		} else
		{
			m_chkUseTls.Enable(!m_proto->JGetByte("UseSSL", FALSE));
			EnableWindow(GetDlgItem(m_hwnd, IDC_DOWNLOAD_OPENSSL), FALSE);
		}

		if (m_proto->JGetByte( "ManualConnect", FALSE ))
		{
			m_txtManualHost.Enable();
			m_txtManualPort.Enable();
			m_txtPort.Disable();
		}

		CheckRegistration();
	}

	void OnApply()
	{
		if (m_chkSavePassword.GetState() == BST_CHECKED)
		{
			char *text = m_txtPassword.GetTextA();
			JCallService(MS_DB_CRYPT_ENCODESTRING, lstrlenA(text), (LPARAM)text);
			m_proto->JSetString(NULL, "Password", text);
			mir_free(text);
		} else
		{
			m_proto->JDeleteSetting(NULL, "Password");
		}

		int index = m_cbLocale.GetCurSel();
		if ( index >= 0 )
		{
			TCHAR *szDefaultLanguage = m_proto->GetXmlLang();
			TCHAR *szLanguageCode = (TCHAR *)m_cbLocale.GetItemData(index);
			if ( szLanguageCode )
				m_proto->JSetStringT(NULL, "XmlLang", szLanguageCode);
		}

		if (m_proto->m_bJabberConnected)
		{
			if (m_txtUsername.IsChanged() || m_txtPassword.IsChanged() || m_cbResource.IsChanged() ||
				m_cbServer.IsChanged() || m_chkUseHostnameAsResource.IsChanged() || m_txtPort.IsChanged() ||
				m_txtManualHost.IsChanged() || m_txtManualPort.IsChanged() || m_cbLocale.IsChanged())
			{
				MessageBox(m_hwnd,
					TranslateT("These changes will take effect the next time you connect to the Jabber network."),
					TranslateT( "Jabber Protocol Option" ), MB_OK|MB_SETFOREGROUND );
			}

			m_proto->SendPresence(m_proto->m_iStatus, true);
		}
	}

	void OnChange(CCtrlBase *ctrl)
	{
		if (m_initialized)
			CheckRegistration();
	}

	BOOL DlgProc(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_JABBER_REFRESH:
			RefreshServers((XmlNode *)lParam);
			break;
		}
		return CDlgBase::DlgProc(msg, wParam, lParam);
	}

private:
	bool m_gotservers;

	void btnRegister_OnClick(CCtrlButton *)
	{
		ThreadData regInfo(m_proto, JABBER_SESSION_NORMAL);
		m_txtUsername.GetText(regInfo.username, SIZEOF(regInfo.username));
		m_txtPassword.GetTextA(regInfo.password, SIZEOF(regInfo.password));
		m_cbServer.GetTextA(regInfo.server, SIZEOF(regInfo.server));
		if (m_chkManualHost.GetState() == BST_CHECKED)
		{
			regInfo.port = (WORD)m_txtManualPort.GetInt();
			m_txtManualHost.GetTextA(regInfo.manualHost, SIZEOF(regInfo.manualHost));
		} else
		{
			regInfo.port = (WORD)m_txtPort.GetInt();
			regInfo.manualHost[0] = '\0';
		}

		if (regInfo.username[0] && regInfo.password[0] && regInfo.server[0] && regInfo.port>0 && ( (m_chkManualHost.GetState() != BST_CHECKED) || regInfo.manualHost[0] ))
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_OPT_REGISTER), m_hwnd, JabberRegisterDlgProc, (LPARAM)&regInfo);
	}

	void btnUnregister_OnClick(CCtrlButton *)
	{
		int res = MessageBox(NULL,
			TranslateT("This operation will kill your account, roster and all another information stored at the server. Are you ready to do that?"),
			TranslateT("Account removal warning"), MB_YESNOCANCEL);

		if (res == IDYES)
		{
			XmlNodeIq iq("set", NOID, m_proto->m_szJabberJID);
			iq.addQuery(JABBER_FEAT_REGISTER )->addChild("remove");
			m_proto->m_ThreadInfo->send(iq);
		}
	}

	void cbServer_OnDropdown(CCtrlCombo *sender)
	{
		if (!m_gotservers) mir_forkthread(QueryServerListThread, (void *)this);
	}

	void chkManualHost_OnChange(CCtrlData *sender)
	{
		CCtrlCheck *chk = (CCtrlCheck *)sender;

		if (chk->GetState() == BST_CHECKED)
		{
			m_txtManualHost.Enable();
			m_txtManualPort.Enable();
			m_txtPort.Disable();
		} else
		{
			m_txtManualHost.Disable();
			m_txtManualPort.Disable();
			m_txtPort.Enable();
		}
	}

	void chkUseHostnameAsResource_OnChange(CCtrlData *sender)
	{
		CCtrlCheck *chk = (CCtrlCheck *)sender;

		m_cbResource.Enable(chk->GetState() != BST_CHECKED);
		if (chk->GetState() == BST_CHECKED)
		{
			TCHAR szCompName[MAX_COMPUTERNAME_LENGTH + 1];
			DWORD dwCompNameLength = MAX_COMPUTERNAME_LENGTH;
			if (GetComputerName(szCompName, &dwCompNameLength))
				m_cbResource.SetText(szCompName);
		}
	}

	void chkUseSsl_OnChange(CCtrlData *sender)
	{
		if (m_chkManualHost.GetState() != BST_CHECKED)
		{
			if (m_chkUseSsl.GetState() == BST_CHECKED)
			{
				m_chkUseTls.Disable();
				m_txtPort.SetInt(5223);
			} else
			{
				m_chkUseTls.Enable();
				m_txtPort.SetInt(5222);
			}
		}
	}

	void CheckRegistration()
	{
		ThreadData regInfo(m_proto, JABBER_SESSION_NORMAL);
		m_txtUsername.GetText(regInfo.username, SIZEOF(regInfo.username));
		m_txtPassword.GetTextA(regInfo.password, SIZEOF(regInfo.password));
		m_cbServer.GetTextA(regInfo.server, SIZEOF(regInfo.server));
		if (m_chkManualHost.GetState() == BST_CHECKED)
		{
			regInfo.port = (WORD)m_txtManualPort.GetInt();
			m_txtManualHost.GetTextA(regInfo.manualHost, SIZEOF(regInfo.manualHost));
		} else
		{
			regInfo.port = (WORD)m_txtPort.GetInt();
			regInfo.manualHost[0] = '\0';
		}

		if (regInfo.username[0] && regInfo.password[0] && regInfo.server[0] && regInfo.port>0 && ( (m_chkManualHost.GetState() != BST_CHECKED) || regInfo.manualHost[0] ))
			EnableWindow( GetDlgItem( m_hwnd, IDC_BUTTON_REGISTER ), TRUE );
		else
			EnableWindow( GetDlgItem( m_hwnd, IDC_BUTTON_REGISTER ), FALSE );
	}

	void RefreshServers(XmlNode *node)
	{
		m_gotservers = node ? true : false;

		TCHAR *server = m_cbServer.GetText();
		bool bDropdown = m_cbServer.GetDroppedState();
		if (bDropdown) m_cbServer.ShowDropdown(false);

		m_cbServer.ResetContent();
		if ( node )
		{
			for (int i = 0; i < node->numChild; ++i)
				if (!lstrcmpA(node->child[i]->name, "item"))
					if (TCHAR *jid = JabberXmlGetAttrValue(node->child[i], "jid"))
						if (m_cbServer.FindString(jid, -1, true) == CB_ERR)
							m_cbServer.AddString(jid);
		}

		m_cbServer.SetText(server);

		if (bDropdown) m_cbServer.ShowDropdown();
		mir_free(server);
	}

	static void QueryServerListXmlCallback(XmlNode *node, void *userdata)
	{
		HWND hwnd = (HWND)userdata;

		TCHAR *xmlns = JabberXmlGetAttrValue(node, "xmlns");
		if (xmlns && !lstrcmp(xmlns, _T(JABBER_FEAT_DISCO_ITEMS)) && !lstrcmpA(node->name, "query") && IsWindow(hwnd))
			SendMessage(hwnd, WM_JABBER_REFRESH, 0, (LPARAM)node);
		else
			SendMessage(hwnd, WM_JABBER_REFRESH, 0, (LPARAM)NULL);
	}

	static void QueryServerListThread(void *arg)
	{
		CDlgOptAccount *wnd = (CDlgOptAccount *)arg;
		HWND hwnd = wnd->GetHwnd();

		NETLIBHTTPREQUEST request = {0};
		request.cbSize = sizeof(request);
		request.requestType = REQUEST_GET;
		request.flags = NLHRF_GENERATEHOST|NLHRF_SMARTREMOVEHOST|NLHRF_SMARTAUTHHEADER|NLHRF_HTTP11;
		request.szUrl = "http://www.jabber.org/servers.xml";

		NETLIBHTTPREQUEST *result = (NETLIBHTTPREQUEST *)CallService(MS_NETLIB_HTTPTRANSACTION, (WPARAM)wnd->GetProto()->m_hNetlibUser, (LPARAM)&request);

		if (!result)
		{
			SendMessage(hwnd, WM_JABBER_REFRESH, 0, (LPARAM)NULL);
			return;
		}

		if (IsWindow(hwnd))
		{
			if ((result->resultCode == 200) && result->dataLength && result->pData)
			{
				XmlState xmlstate;
				JabberXmlInitState(&xmlstate);
				JabberXmlSetCallback(&xmlstate, 1, ELEM_CLOSE, QueryServerListXmlCallback, (void *)hwnd);
				wnd->GetProto()->OnXmlParse(&xmlstate, result->pData);
				JabberXmlDestroyState(&xmlstate);
			} else
			{
				SendMessage(hwnd, WM_JABBER_REFRESH, 0, (LPARAM)NULL);
			}
		}

		CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT, 0, (LPARAM)result);
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// JabberAdvOptDlgProc - advanced options dialog procedure

class CDlgOptAdvanced: public CJabberDlgBase
{
	CCtrlCheck		m_chkDirect;
	CCtrlCheck		m_chkDirectManual;
	CCtrlCheck		m_chkProxy;
	CCtrlEdit		m_txtDirect;
	CCtrlEdit		m_txtProxy;
	CCtrlTreeOpts	m_otvOptions;

public:
	CDlgOptAdvanced(CJabberProto *proto):
		CJabberDlgBase(proto, IDD_OPT_JABBER2, NULL),
		m_chkDirect(this, IDC_DIRECT),
		m_chkDirectManual(this, IDC_DIRECT_MANUAL),
		m_chkProxy(this, IDC_PROXY_MANUAL),
		m_txtDirect(this, IDC_DIRECT_ADDR),
		m_txtProxy(this, IDC_PROXY_ADDR),
		m_otvOptions(this, IDC_OPTTREE, proto->m_szModuleName)
	{
		CreateLink(m_chkDirect, "BsDirect", DBVT_BYTE, FALSE);
		CreateLink(m_chkDirectManual, "BsDirectManual", DBVT_BYTE, FALSE);
		CreateLink(m_chkProxy, "BsProxyManual", DBVT_BYTE, FALSE);
		CreateLink(m_txtDirect, "BsDirectAddr", _T(""));
		CreateLink(m_txtProxy, "BsProxyServer", _T(""));

		m_chkDirect.OnChange =
		m_chkDirectManual.OnChange = Callback(this, &CDlgOptAdvanced::chkDirect_OnChange);
		m_chkProxy.OnChange = Callback(this, &CDlgOptAdvanced::chkProxy_OnChange);

		m_otvOptions.AddOption(LPGENT("Messaging") _T("/") LPGENT("Send messages slower, but with full acknowledgement"), "MsgAck", FALSE);
		m_otvOptions.AddOption(LPGENT("Messaging") _T("/") LPGENT("Enable avatars"), "EnableAvatars", TRUE);
		m_otvOptions.AddOption(LPGENT("Messaging") _T("/") LPGENT("Log chat state changes"), "LogChatstates", FALSE);
		m_otvOptions.AddOption(LPGENT("Messaging") _T("/") LPGENT("Enable user moods receiving"), "EnableUserMood", TRUE);
		m_otvOptions.AddOption(LPGENT("Messaging") _T("/") LPGENT("Enable user tunes receiving"), "EnableUserTune", FALSE);

		m_otvOptions.AddOption(LPGENT("Server options") _T("/") LPGENT("Disable SASL authentication (for old servers)"), "Disable3920auth", FALSE);
		m_otvOptions.AddOption(LPGENT("Server options") _T("/") LPGENT("Enable stream compression (if possible)"), "EnableZlib", FALSE);

		m_otvOptions.AddOption(LPGENT("Other") _T("/") LPGENT("Enable remote controlling (from another resource of same JID only)"), "EnableRemoteControl", FALSE);
		m_otvOptions.AddOption(LPGENT("Other") _T("/") LPGENT("Show transport agents on contact list"), "ShowTransport", TRUE);
		m_otvOptions.AddOption(LPGENT("Other") _T("/") LPGENT("Automatically add contact when accept authorization"), "AutoAdd", TRUE);
		m_otvOptions.AddOption(LPGENT("Other") _T("/") LPGENT("Automatically accept authorization requests"), "AutoAcceptAuthorization", FALSE);
		m_otvOptions.AddOption(LPGENT("Other") _T("/") LPGENT("Fix incorrect timestamps in incoming messages"), "FixIncorrectTimestamps", TRUE);
		
		m_otvOptions.AddOption(LPGENT("Security") _T("/") LPGENT("Show information about operating system in version replies"), "ShowOSVersion", TRUE);
		m_otvOptions.AddOption(LPGENT("Security") _T("/") LPGENT("Accept only in band incoming filetransfers (don't disclose own IP)"), "BsOnlyIBB", FALSE);
	}

	void OnInitDialog()
	{
		CJabberDlgBase::OnInitDialog();

		chkDirect_OnChange(&m_chkDirect);
		chkProxy_OnChange(&m_chkProxy);
	}

	void OnApply()
	{
		BOOL bChecked = m_proto->JGetByte("ShowTransport", TRUE);
		int index = 0;
		while (( index = m_proto->ListFindNext( LIST_ROSTER, index )) >= 0 ) {
			JABBER_LIST_ITEM* item = m_proto->ListGetItemPtrFromIndex( index );
			if ( item != NULL ) {
				if ( _tcschr( item->jid, '@' ) == NULL ) {
					HANDLE hContact = m_proto->HContactFromJID( item->jid );
					if ( hContact != NULL ) {
						if ( bChecked ) {
							if ( item->itemResource.status != m_proto->JGetWord( hContact, "Status", ID_STATUS_OFFLINE )) {
								m_proto->JSetWord( hContact, "Status", ( WORD )item->itemResource.status );
						}	}
						else if ( m_proto->JGetWord( hContact, "Status", ID_STATUS_OFFLINE ) != ID_STATUS_OFFLINE )
							m_proto->JSetWord( hContact, "Status", ID_STATUS_OFFLINE );
			}	}	}
			index++;
		}

		m_proto->SendPresence( m_proto->m_iStatus, true );
	}

	void chkDirect_OnChange(CCtrlData *)
	{
		if (m_chkDirect.GetState() == BST_CHECKED)
		{
			if (m_chkDirectManual.GetState() == BST_CHECKED)
				m_txtDirect.Enable();
			else
				m_txtDirect.Disable();

			m_chkDirectManual.Enable();
		} else
		{
			m_txtDirect.Disable();
			m_chkDirectManual.Disable();
		}
	}

	void chkProxy_OnChange(CCtrlData *)
	{
		if (m_chkProxy.GetState() == BST_CHECKED)
			m_txtProxy.Enable();
		else
			m_txtProxy.Disable();
	}

	static CDlgBase *Create(void *param) { return new CDlgOptAdvanced((CJabberProto *)param); }
};

/////////////////////////////////////////////////////////////////////////////////////////
// JabberGcOptDlgProc - chat options dialog procedure

class CDlgOptGc: public CJabberDlgBase
{
	CCtrlEdit		m_txtSlap;
	CCtrlEdit		m_txtQuit;
	CCtrlTreeOpts	m_otvOptions;

public:
	CDlgOptGc(CJabberProto *proto):
		CJabberDlgBase(proto, IDD_OPT_JABBER4, NULL),
		m_txtSlap(this, IDC_TXT_SLAP),
		m_txtQuit(this, IDC_TXT_QUIT),
		m_otvOptions(this, IDC_OPTTREE, proto->m_szModuleName)
	{
		CreateLink(m_txtSlap, "GcMsgSlap", TranslateTS(JABBER_GC_MSG_SLAP));
		CreateLink(m_txtQuit, "GcMsgQuit", TranslateTS(JABBER_GC_MSG_QUIT));

		m_otvOptions.AddOption(LPGENT("General") _T("/") LPGENT("Autoaccept multiuser chat invitations"),		"AutoAcceptMUC",		FALSE);
		m_otvOptions.AddOption(LPGENT("General") _T("/") LPGENT("Automatically join bookmarks on login"),		"AutoJoinConferences",	FALSE);
		m_otvOptions.AddOption(LPGENT("General") _T("/") LPGENT("Automatically join conferences on login"),		"IgnoreMUCInvites",		FALSE);
		m_otvOptions.AddOption(LPGENT("General") _T("/") LPGENT("Do not show multiuser chat invitations"),		"AutoJoinBookmarks",	TRUE);
		m_otvOptions.AddOption(LPGENT("Log events") _T("/") LPGENT("Ban notifications"),						"GcLogBans",			TRUE);
		m_otvOptions.AddOption(LPGENT("Log events") _T("/") LPGENT("Room configuration changes"),				"GcLogConfig",			FALSE);
		m_otvOptions.AddOption(LPGENT("Log events") _T("/") LPGENT("Affiliation changes"),						"GcLogAffiliations",	FALSE);
		m_otvOptions.AddOption(LPGENT("Log events") _T("/") LPGENT("Role changes"),								"GcLogRoles",			FALSE);
	}

	static CDlgBase *Create(void *param) { return new CDlgOptGc((CJabberProto *)param); }
};

//////////////////////////////////////////////////////////////////////////
// roster editor
//
#include <io.h>
#define JM_STATUSCHANGED WM_USER+0x0001
#ifdef UNICODE
#define fputtc(str, file) fputw(str, file)
#define fopent(name, mode) _wfopen(name, mode)
#else
#define fputtc(str, file) fputc(str, file)
#define fopent(name, mode) fopen(name, mode)
#endif

enum {
	RRA_FILLLIST = 0,
	RRA_SYNCROSTER,
	RRA_SYNCDONE
};

typedef struct _tag_RosterhEditDat{
	WNDPROC OldEditProc;
	HWND hList;
	int index;
	int subindex;
} ROSTEREDITDAT;

static WNDPROC _RosterOldListProc=NULL;

static int	_RosterInsertListItem(HWND hList, TCHAR * jid, TCHAR * nick, TCHAR * group, TCHAR * subscr, BOOL bChecked)
{
	LVITEM item={0};
	int index;
	item.mask=LVIF_TEXT|LVIF_STATE;

	item.iItem=ListView_GetItemCount(hList);
	item.iSubItem=0;
	item.pszText=jid;

	index=ListView_InsertItem(hList, &item);

	if ( index<0 )
		return index;

	ListView_SetCheckState(hList, index, bChecked);

	ListView_SetItemText(hList, index, 0, jid);
	ListView_SetItemText(hList, index, 1, nick);
	ListView_SetItemText(hList, index, 2, group);
	ListView_SetItemText(hList, index, 3, subscr);

	return index;
}

static void _RosterListClear(HWND hwndDlg)
{
	HWND hList=GetDlgItem(hwndDlg, IDC_ROSTER);
	if (!hList)	return;
	ListView_DeleteAllItems(hList);
	while (	ListView_DeleteColumn( hList, 0) );

	LV_COLUMN column={0};
	column.mask=LVCF_TEXT;
	column.cx=500;

	column.pszText=_T("JID");
	int ret=ListView_InsertColumn(hList, 1, &column);

	column.pszText=_T("Nick Name");
	ListView_InsertColumn(hList, 2, &column);

	column.pszText=_T("Group");
	ListView_InsertColumn(hList, 3, &column);

	column.pszText=_T("Subscription");
	ListView_InsertColumn(hList, 4, &column);

	RECT rc;
	GetClientRect(hList, &rc);
	int width=rc.right-rc.left;

	ListView_SetColumnWidth(hList,0,width*40/100);
	ListView_SetColumnWidth(hList,1,width*25/100);
	ListView_SetColumnWidth(hList,2,width*20/100);
	ListView_SetColumnWidth(hList,3,width*10/100);
}


void CJabberProto::_RosterHandleGetRequest( XmlNode* node, void* userdata )
{
	HWND hList=GetDlgItem(rrud.hwndDlg, IDC_ROSTER);
	if (rrud.bRRAction==RRA_FILLLIST)
	{
		_RosterListClear(rrud.hwndDlg);
		XmlNode * query=JabberXmlGetChild(node, "query");
		if (!query) return;
		int i = 1;
		while (TRUE) {
			XmlNode *item = JabberXmlGetNthChild(query, "item", i++);
			if (!item) break;
			TCHAR *jid=JabberXmlGetAttrValue(item,"jid");
			if (!jid) continue;
			TCHAR *name=JabberXmlGetAttrValue(item,"name");
			TCHAR *subscription=JabberXmlGetAttrValue(item,"subscription");
			TCHAR *group=NULL;
			XmlNode * groupNode=JabberXmlGetChild(item, "group");
			if ( groupNode ) group = groupNode->text;
			_RosterInsertListItem( hList, jid, name, group, subscription, TRUE );
		}
		// now it is require to process whole contact list to add not in roster contacts
		{
			HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
			while ( hContact != NULL )
			{
				char* str = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
				if ( str != NULL && !strcmp( str, m_szModuleName ))
				{
					DBVARIANT dbv;
					if ( !JGetStringT( hContact, "jid", &dbv ))
					{
						LVFINDINFO lvfi={0};
						lvfi.flags = LVFI_STRING;
						lvfi.psz = dbv.ptszVal;
						TCHAR *p = _tcschr(dbv.ptszVal,_T('@'));
						if ( p ) {
							p = _tcschr( dbv.ptszVal, _T('/'));
							if ( p ) *p = _T('\0');
						}
						if ( ListView_FindItem(hList, -1, &lvfi) == -1) {
							TCHAR *jid = mir_tstrdup( dbv.ptszVal );
							TCHAR *name = NULL;
							TCHAR *group = NULL;
							DBVARIANT dbvtemp;
							if ( !DBGetContactSettingTString( hContact, "CList", "MyHandle", &dbvtemp )) {
								name = mir_tstrdup( dbvtemp.ptszVal );
								DBFreeVariant( &dbvtemp );
							}
							if ( !DBGetContactSettingTString( hContact, "CList", "Group", &dbvtemp )) {
								group = mir_tstrdup( dbvtemp.ptszVal );
								DBFreeVariant( &dbvtemp );
							}
							_RosterInsertListItem( hList, jid, name, group, NULL, FALSE );
							if ( jid ) mir_free( jid );
							if ( name ) mir_free( name );
							if ( group ) mir_free( group );
						}
						DBFreeVariant( &dbv );
					}
				}
				hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
			}
		}
		rrud.bReadyToDownload = FALSE;
		rrud.bReadyToUpload = TRUE;
		SetDlgItemText( rrud.hwndDlg, IDC_DOWNLOAD, TranslateT( "Download" ));
		SetDlgItemText( rrud.hwndDlg, IDC_UPLOAD, TranslateT( "Upload" ));
		SendMessage( rrud.hwndDlg, JM_STATUSCHANGED, 0, 0 );
        return;
	}
	else if ( rrud.bRRAction == RRA_SYNCROSTER )
	{
		SetDlgItemText(rrud.hwndDlg, IDC_UPLOAD, TranslateT("Uploading..."));
		XmlNode * queryRoster=JabberXmlGetChild(node, "query");
		if (!queryRoster) return;

		int iqId = SerialNext();
		IqAdd( iqId, IQ_PROC_NONE, (JABBER_IQ_PFUNC)&CJabberProto::_RosterHandleGetRequest );

		XmlNode iq( "iq" );
		iq.addAttr( "type", "set" );
		iq.addAttrID( iqId );
		XmlNode* query = iq.addChild( "query" );
		query->addAttr( "xmlns", JABBER_FEAT_IQ_ROSTER );
		int itemCount=0;
		int ListItemCount=ListView_GetItemCount(hList);
		for (int index=0; index<ListItemCount; index++)
		{
			TCHAR jid[260]={0};
			TCHAR name[260]={0};
			TCHAR group[260]={0};
			TCHAR subscr[260]={0};
			ListView_GetItemText(hList, index, 0, jid, SIZEOF(jid));
			ListView_GetItemText(hList, index, 1, name, SIZEOF(name));
			ListView_GetItemText(hList, index, 2, group, SIZEOF(group));
			ListView_GetItemText(hList, index, 3, subscr, SIZEOF(subscr));
			XmlNode *itemRoster=JabberXmlGetChildWithGivenAttrValue(queryRoster, "item", "jid", jid);
			BOOL bRemove = !ListView_GetCheckState(hList,index);
			if (itemRoster && bRemove)
			{
				//delete item
				XmlNode* item = query->addChild( "item" );
				item->addAttr( "jid", jid );
				item->addAttr( "subscription","remove");
				itemCount++;
			}
			else if ( !bRemove )
			{
				BOOL bPushed=FALSE;
				{
					TCHAR *rosterName=JabberXmlGetAttrValue(itemRoster,"name");
					if ( (rosterName!=NULL || name[0]!=_T('\0')) && lstrcmpi(rosterName,name) )
						bPushed=TRUE;
					if ( !bPushed)
					{
						rosterName=JabberXmlGetAttrValue(itemRoster,"subscription");
						if ((rosterName!=NULL || subscr[0]!=_T('\0')) && lstrcmpi(rosterName,subscr) )
							bPushed=TRUE;
					}
					if ( !bPushed)
					{
						XmlNode * groupNode=JabberXmlGetChild(itemRoster,"group");
						TCHAR * rosterGroup=NULL;
						if (groupNode!=NULL)
							rosterGroup=groupNode->text;
						if ((rosterGroup!=NULL || group[0]!=_T('\0')) && lstrcmpi(rosterGroup,group) )
							bPushed=TRUE;
					}

				}
				if (bPushed)
				{
					XmlNode* item = query->addChild( "item" );
					if ( group && _tcslen( group ))
						item->addChild( "group", group );
					if ( name && _tcslen( name ))
						item->addAttr( "name", name );
					item->addAttr( "jid", jid );
					item->addAttr( "subscription", subscr[0] ? subscr : _T("none"));
					itemCount++;
				}
			}
		}
		rrud.bRRAction=RRA_SYNCDONE;
		if (itemCount)
			m_ThreadInfo->send( iq );
		else
			_RosterSendRequest(rrud.hwndDlg,RRA_FILLLIST);
	}
	else
	{
		SetDlgItemText(rrud.hwndDlg,IDC_UPLOAD,TranslateT("Upload"));
		rrud.bReadyToUpload=FALSE;
		rrud.bReadyToDownload=FALSE;
		SendMessage(rrud.hwndDlg, JM_STATUSCHANGED,0,0);
		SetDlgItemText(rrud.hwndDlg,IDC_DOWNLOAD,TranslateT("Downloading..."));
		_RosterSendRequest(rrud.hwndDlg,RRA_FILLLIST);

	}
}

void CJabberProto::_RosterSendRequest(HWND hwndDlg, BYTE rrAction)
{
	rrud.bRRAction=rrAction;
	rrud.hwndDlg=hwndDlg;

	int iqId = SerialNext();
	IqAdd( iqId, IQ_PROC_NONE, (JABBER_IQ_PFUNC)&CJabberProto::_RosterHandleGetRequest );

	XmlNode iq( "iq" );
	iq.addAttr( "type", "get" );
	iq.addAttrID( iqId );
	XmlNode* query = iq.addChild( "query" );
	query->addAttr( "xmlns", JABBER_FEAT_IQ_ROSTER );
	m_ThreadInfo->send( iq );
}




static void _RosterItemEditEnd( HWND hEditor, ROSTEREDITDAT * edat, BOOL bCancel )
{
	if (!bCancel)
	{
		int len = GetWindowTextLength(hEditor) + 1;
		TCHAR *buff=(TCHAR*)mir_alloc(len*sizeof(TCHAR));
		if ( buff ) {
			GetWindowText(hEditor,buff,len);
			ListView_SetItemText(edat->hList,edat->index, edat->subindex,buff);
		}
		mir_free(buff);
	}
	DestroyWindow(hEditor);

}

static BOOL CALLBACK _RosterItemNewEditProc( HWND hEditor, UINT msg, WPARAM wParam, LPARAM lParam )
{
	ROSTEREDITDAT * edat = (ROSTEREDITDAT *) GetWindowLong(hEditor,GWL_USERDATA);
	if (!edat) return 0;
	switch(msg)
	{

	case WM_KEYDOWN:
		switch(wParam)
		{
		case VK_RETURN:
			_RosterItemEditEnd(hEditor, edat, FALSE);
			return 0;
		case VK_ESCAPE:
			_RosterItemEditEnd(hEditor, edat, TRUE);
			return 0;
		}
		break;
	case WM_GETDLGCODE:
		if ( lParam ) {
			MSG *msg2 = (MSG*)lParam;
			if (msg2->message==WM_KEYDOWN && msg2->wParam==VK_TAB) return 0;
			if (msg2->message==WM_CHAR && msg2->wParam=='\t') return 0;
		}
		return DLGC_WANTMESSAGE;
	case WM_KILLFOCUS:
		_RosterItemEditEnd(hEditor, edat, FALSE);
		return 0;
	}

	if (msg==WM_DESTROY)
	{
		SetWindowLong(hEditor, GWL_WNDPROC, (LONG) edat->OldEditProc);
		SetWindowLong(hEditor, GWL_USERDATA, (LONG) 0);
		free(edat);
		return 0;
	}
	else return CallWindowProc( edat->OldEditProc, hEditor, msg, wParam, lParam);
}

#ifdef UNICODE
void fputw( TCHAR ch, FILE * fp)
{
	TCHAR buf[2]={0};
	char utf[10]={0};
	char *str=utf;
	buf[0]=ch;
	WideCharToMultiByte(CP_UTF8,0,buf,-1,utf,sizeof(utf),NULL, NULL);
	while ( *str!='\0' )
	{
		fputc(*str,fp);
		str++;
	}

}
#endif

static void _RosterSaveString(FILE * fp, TCHAR * str, BOOL quotes=FALSE)
{
	if (quotes) fputtc(_T('\"'),fp);
	while ( *str!=_T('\0') )
	{
		fputtc(*str,fp);
		if (quotes && *str==_T('\"')) fputtc(*str,fp);
		str++;
	}
	if (quotes) fputtc(_T('\"'),fp);
}

void CJabberProto::_RosterExportToFile(HWND hwndDlg)
{
	TCHAR filename[MAX_PATH]={0};

	TCHAR *filter=_T("XML for MS Excel (UTF-8 encoded)(*.xml)\0*.xml\0\0");
	OPENFILENAME ofn={0};
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner = hwndDlg;
	ofn.hInstance = NULL;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = filename;
	ofn.Flags = OFN_HIDEREADONLY;
	ofn.nMaxFile = SIZEOF(filename);
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrDefExt = _T("xml");
	if(!GetSaveFileName(&ofn)) return;

	FILE * fp=fopent(filename,_T("w"));
	if (!fp) return;
	HWND hList=GetDlgItem(hwndDlg, IDC_ROSTER);
	int ListItemCount=ListView_GetItemCount(hList);
	TCHAR *xmlExcelHeader=
		_T(			"<?xml version=\"1.0\"?>	\n"											)
		_T(			"<?mso-application progid=\"Excel.Sheet\"?> \n"							)
		_T(			"<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\" \n"	)
		_T("\t\t\txmlns:o=\"urn:schemas-microsoft-com:office:office\"\n"					)
		_T("\t\t\txmlns:x=\"urn:schemas-microsoft-com:office:excel\"\n"					)
		_T("\t\t\txmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\"\n"			)
		_T("\t\t\txmlns:html=\"http://www.w3.org/TR/REC-html40\">\n"						)
		_T("\t<ExcelWorkbook xmlns=\"urn:schemas-microsoft-com:office:excel\">\n"	)
		_T("\t</ExcelWorkbook>\n"													)
		_T("\t<Worksheet ss:Name=\"Exported roster\">\n"								)
		_T("\t\t<Table>\n"																);

	TCHAR *xmlExcelFooter=
		_T("\t\t</Table>\n")
		_T("\t</Worksheet>\n")
		_T("</Workbook>\n");

	_RosterSaveString(fp,xmlExcelHeader);
	for (int index=0; index<ListItemCount; index++)
	{
		TCHAR jid[260]={0};
		TCHAR name[260]={0};
		TCHAR group[260]={0};
		TCHAR subscr[260]={0};
		ListView_GetItemText(hList, index, 0, jid, SIZEOF(jid));
		ListView_GetItemText(hList, index, 1, name, SIZEOF(name));
		ListView_GetItemText(hList, index, 2, group, SIZEOF(group));
		ListView_GetItemText(hList, index, 3, subscr, SIZEOF(subscr));
		_RosterSaveString(fp,_T("\t\t\t<Row>\n"));
		_RosterSaveString(fp,_T("\t\t\t\t<Cell><Data ss:Type=\"String\">+</Data></Cell>\n"));
		_RosterSaveString(fp,_T("\t\t\t\t<Cell><Data ss:Type=\"String\">"));
		_RosterSaveString(fp,jid);
		_RosterSaveString(fp,_T("</Data></Cell>\n"));

		_RosterSaveString(fp,_T("\t\t\t\t<Cell><Data ss:Type=\"String\">"));
		_RosterSaveString(fp,name);
		_RosterSaveString(fp,_T("</Data></Cell>\n"));

		_RosterSaveString(fp,_T("\t\t\t\t<Cell><Data ss:Type=\"String\">"));
		_RosterSaveString(fp,group);
		_RosterSaveString(fp,_T("</Data></Cell>\n"));

		_RosterSaveString(fp,_T("\t\t\t\t<Cell><Data ss:Type=\"String\">"));
		_RosterSaveString(fp,subscr);
		_RosterSaveString(fp,_T("</Data></Cell>\n"));

		_RosterSaveString(fp,_T("\t\t\t</Row>\n"));
	}
	_RosterSaveString(fp,xmlExcelFooter);
	fclose(fp);
}

static void _RosterParseXmlWorkbook( XmlNode *node, void *userdata )
{
	HWND hList=(HWND)userdata;
	if ( !lstrcmpiA(node->name,"Workbook"))
	{
		XmlNode * Worksheet=JabberXmlGetChild(node,"Worksheet");
		if (Worksheet)
		{
			XmlNode * Table=JabberXmlGetChild(Worksheet,"Table");
			if (Table)
			{
				int index=1;
				XmlNode *Row;
				while (TRUE)
				{
					Row=JabberXmlGetNthChild(Table,"Row",index++);
					if (!Row) break;
					BOOL bAdd=FALSE;
					TCHAR * jid=NULL;
					TCHAR * name=NULL;
					TCHAR * group=NULL;
					TCHAR * subscr=NULL;
					XmlNode * Cell;
					XmlNode * Data;
					Cell=JabberXmlGetNthChild(Row,"Cell",1);
					if (Cell) Data=JabberXmlGetChild(Cell,"Data");
					else Data=NULL;
					if (Data)
					{
						if (!lstrcmpi(Data->text,_T("+"))) bAdd=TRUE;
						else if (lstrcmpi(Data->text,_T("-"))) continue;

						Cell=JabberXmlGetNthChild(Row,"Cell",2);
						if (Cell) Data=JabberXmlGetChild(Cell,"Data");
						else Data=NULL;
						if (Data)
						{
							jid=Data->text;
							if (!jid || lstrlen(jid)==0) continue;
						}

						Cell=JabberXmlGetNthChild(Row,"Cell",3);
						if (Cell) Data=JabberXmlGetChild(Cell,"Data");
						else Data=NULL;
						if (Data) name=Data->text;

						Cell=JabberXmlGetNthChild(Row,"Cell",4);
						if (Cell) Data=JabberXmlGetChild(Cell,"Data");
						else Data=NULL;
						if (Data) group=Data->text;

						Cell=JabberXmlGetNthChild(Row,"Cell",5);
						if (Cell) Data=JabberXmlGetChild(Cell,"Data");
						else Data=NULL;
						if (Data) subscr=Data->text;
					}
					_RosterInsertListItem(hList,jid,name,group,subscr,bAdd);
}	}	}	}	}

void CJabberProto::_RosterImportFromFile(HWND hwndDlg)
{
	char filename[MAX_PATH]={0};
	char *filter="XML for MS Excel (UTF-8 encoded)(*.xml)\0*.xml\0\0";
	OPENFILENAMEA ofn={0};
	ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
	ofn.hwndOwner = hwndDlg;
	ofn.hInstance = NULL;
	ofn.lpstrFilter = filter;
	ofn.lpstrFile = filename;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.nMaxFile = sizeof(filename);
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrDefExt = "xml";
	if(!GetOpenFileNameA(&ofn)) return;

	FILE * fp=fopen(filename,"r");
	if (!fp) return;
	HWND hList=GetDlgItem(hwndDlg, IDC_ROSTER);
	char * buffer;
	DWORD bufsize=filelength(fileno(fp));
	if (bufsize>0)
		buffer=(char*)malloc(bufsize);
	fread(buffer,1,bufsize,fp);
	fclose(fp);
	_RosterListClear(hwndDlg);
	XmlState xmlstate;
	JabberXmlInitState(&xmlstate);
	JabberXmlSetCallback( &xmlstate, 2, ELEM_CLOSE, _RosterParseXmlWorkbook, (void*)hList );
	OnXmlParse(&xmlstate,buffer);
	xmlstate=xmlstate;
	JabberXmlDestroyState(&xmlstate);
	free(buffer);
	SendMessage(hwndDlg,JM_STATUSCHANGED, 0 , 0);
}


static BOOL CALLBACK _RosterNewListProc( HWND hList, UINT msg, WPARAM wParam, LPARAM lParam )
{
	if (msg==WM_MOUSEWHEEL || msg==WM_NCLBUTTONDOWN || msg==WM_NCRBUTTONDOWN)
	{
		SetFocus(hList);
	}

	if (msg==WM_LBUTTONDOWN)
	{
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(hList, &pt);

		LVHITTESTINFO lvhti={0};
		lvhti.pt=pt;
		ListView_SubItemHitTest(hList,&lvhti);
		if 	(lvhti.flags&LVHT_ONITEM && lvhti.iSubItem !=0)
		{
			RECT rc;
			TCHAR buff[260];
			ListView_GetSubItemRect(hList, lvhti.iItem, lvhti.iSubItem, LVIR_BOUNDS,&rc);
			ListView_GetItemText(hList, lvhti.iItem, lvhti.iSubItem, buff, SIZEOF(buff) );
			HWND hEditor=CreateWindow(TEXT("EDIT"),buff,WS_CHILD|ES_AUTOHSCROLL,rc.left+3, rc.top+2, rc.right-rc.left-3, rc.bottom - rc.top-3,hList, NULL, hInst, NULL);
			SendMessage(hEditor,WM_SETFONT,(WPARAM)SendMessage(hList,WM_GETFONT,0,0),0);
			ShowWindow(hEditor,SW_SHOW);
			SetWindowText(hEditor, buff);
			ClientToScreen(hList, &pt);
			ScreenToClient(hEditor, &pt);
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

			ROSTEREDITDAT * edat=(ROSTEREDITDAT *)malloc(sizeof(ROSTEREDITDAT));
			edat->OldEditProc=(WNDPROC)GetWindowLong(hEditor, GWL_WNDPROC);
			SetWindowLong(hEditor,GWL_WNDPROC,(LONG)_RosterItemNewEditProc);
			edat->hList=hList;
			edat->index=lvhti.iItem;
			edat->subindex=lvhti.iSubItem;
			SetWindowLong(hEditor,GWL_USERDATA,(LONG)edat);
		}
	}
	return CallWindowProc(_RosterOldListProc, hList, msg, wParam, lParam );
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberRosterOptDlgProc - advanced options dialog procedure

static int sttRosterEditorResizer(HWND hwndDlg, LPARAM lParam, UTILRESIZECONTROL *urc)
{
	switch (urc->wId)
	{
	case IDC_WHITERECT:
	case IDC_TITLE:
	case IDC_DESCRIPTION:
	case IDC_FRAME1:
		return RD_ANCHORX_LEFT|RD_ANCHORY_TOP|RD_ANCHORX_WIDTH;
	case IDC_ROSTER:
		return RD_ANCHORX_LEFT|RD_ANCHORY_TOP|RD_ANCHORY_HEIGHT|RD_ANCHORX_WIDTH;
	case IDC_DOWNLOAD:
	case IDC_UPLOAD:
		return RD_ANCHORX_LEFT|RD_ANCHORY_BOTTOM;
	case IDC_EXPORT:
	case IDC_IMPORT:
		return RD_ANCHORX_RIGHT|RD_ANCHORY_BOTTOM;
//	case IDC_STATUSBAR:
//		return RD_ANCHORX_LEFT|RD_ANCHORX_WIDTH|RD_ANCHORY_BOTTOM;
	}
	return RD_ANCHORX_LEFT|RD_ANCHORY_TOP;
}

static BOOL CALLBACK JabberRosterOptDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	CJabberProto* ppro = ( CJabberProto* )GetWindowLong( hwndDlg, GWL_USERDATA );

	switch ( msg ) {
	case JM_STATUSCHANGED:
		{
			int count = ListView_GetItemCount(GetDlgItem(hwndDlg,IDC_ROSTER));
			EnableWindow( GetDlgItem( hwndDlg, IDC_DOWNLOAD ), ppro->m_bJabberConnected );
			EnableWindow( GetDlgItem( hwndDlg, IDC_UPLOAD ), count && ppro->m_bJabberConnected );
			EnableWindow( GetDlgItem( hwndDlg, IDC_EXPORT ), count > 0);
			break;
		}
	case WM_CLOSE:
		{
			DestroyWindow(hwndDlg);
			break;
		}
	case WM_DESTROY:
		{
			Utils_SaveWindowPosition(hwndDlg, NULL, ppro->m_szModuleName, "rosterCtrlWnd_");
			DeleteObject((HFONT)SendDlgItemMessage(hwndDlg, IDC_TITLE, WM_GETFONT, 0, 0));
			ppro->rrud.hwndDlg = NULL;
			break;
		}
	case WM_INITDIALOG:
		{
			ppro = ( CJabberProto* )lParam;
			SetWindowLong( hwndDlg, GWL_USERDATA, lParam );

			TranslateDialogDefault( hwndDlg );
			SendMessage( hwndDlg, WM_SETICON, ICON_BIG, ( LPARAM )ppro->LoadIconEx( "Agents" ));

			LOGFONT lf;
			GetObject((HFONT)SendDlgItemMessage(hwndDlg, IDC_TITLE, WM_GETFONT, 0, 0), sizeof(lf), &lf);
			lf.lfWeight = FW_BOLD;
			HFONT hfnt = CreateFontIndirect(&lf);
			SendDlgItemMessage(hwndDlg, IDC_TITLE, WM_SETFONT, (WPARAM)hfnt, TRUE);

			Utils_RestoreWindowPosition(hwndDlg, NULL, ppro->m_szModuleName, "rosterCtrlWnd_");

			ListView_SetExtendedListViewStyle(GetDlgItem(hwndDlg,IDC_ROSTER),  LVS_EX_CHECKBOXES | LVS_EX_BORDERSELECT /*| LVS_EX_FULLROWSELECT*/ | LVS_EX_GRIDLINES /*| LVS_EX_HEADERDRAGDROP*/ );
			_RosterOldListProc=(WNDPROC) GetWindowLong(GetDlgItem(hwndDlg,IDC_ROSTER), GWL_WNDPROC);
			SetWindowLong(GetDlgItem(hwndDlg,IDC_ROSTER), GWL_WNDPROC, (LONG) _RosterNewListProc);
			_RosterListClear(hwndDlg);
			ppro->rrud.hwndDlg = hwndDlg;
			ppro->rrud.bReadyToDownload = TRUE;
			ppro->rrud.bReadyToUpload = FALSE;
			SendMessage( hwndDlg, JM_STATUSCHANGED, 0, 0 );

			return TRUE;
		}
	case WM_CTLCOLORSTATIC:
		if ( ((HWND)lParam == GetDlgItem(hwndDlg, IDC_WHITERECT)) ||
			 ((HWND)lParam == GetDlgItem(hwndDlg, IDC_TITLE)) ||
			 ((HWND)lParam == GetDlgItem(hwndDlg, IDC_DESCRIPTION)) )
			return (BOOL)GetStockObject(WHITE_BRUSH);
		return FALSE;
	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
			lpmmi->ptMinTrackSize.x = 550;
			lpmmi->ptMinTrackSize.y = 390;
			return 0;
		}
	case WM_SIZE:
	{
		UTILRESIZEDIALOG urd = {0};
		urd.cbSize = sizeof(urd);
		urd.hInstance = hInst;
		urd.hwndDlg = hwndDlg;
		urd.lpTemplate = MAKEINTRESOURCEA(IDD_OPT_JABBER3);
		urd.pfnResizer = sttRosterEditorResizer;
		CallService(MS_UTILS_RESIZEDIALOG, 0, (LPARAM)&urd);
		break;
	}
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_DOWNLOAD:
			ppro->rrud.bReadyToUpload = FALSE;
			ppro->rrud.bReadyToDownload = FALSE;
			SendMessage( ppro->rrud.hwndDlg, JM_STATUSCHANGED,0,0);
			SetDlgItemText( ppro->rrud.hwndDlg, IDC_DOWNLOAD, TranslateT("Downloading..."));
			ppro->_RosterSendRequest(hwndDlg, RRA_FILLLIST);
			break;

		case IDC_UPLOAD:
			ppro->rrud.bReadyToUpload = FALSE;
			SendMessage( ppro->rrud.hwndDlg, JM_STATUSCHANGED, 0, 0 );
			SetDlgItemText( ppro->rrud.hwndDlg, IDC_UPLOAD, TranslateT("Connecting..."));
			ppro->_RosterSendRequest( hwndDlg, RRA_SYNCROSTER );
			break;

		case IDC_EXPORT:
			ppro->_RosterExportToFile( hwndDlg );
			break;

		case IDC_IMPORT:
			ppro->_RosterImportFromFile( hwndDlg );
			break;
		}
		break;
	}
	return FALSE;
}

int __cdecl CJabberProto::OnMenuHandleRosterControl( WPARAM wParam, LPARAM lParam )
{
	if ( rrud.hwndDlg && IsWindow( rrud.hwndDlg ))
		SetForegroundWindow( rrud.hwndDlg );
	else
		CreateDialogParam( hInst, MAKEINTRESOURCE( IDD_OPT_JABBER3 ), NULL, JabberRosterOptDlgProc, ( LPARAM )this );

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberOptInit - initializes all options dialogs

int CJabberProto::OnOptionsInit( WPARAM wParam, LPARAM lParam )
{
	OPTIONSDIALOGPAGE odp = { 0 };

	odp.cbSize      = sizeof( odp );
	odp.hInstance   = hInst;
	odp.ptszGroup   = LPGENT("Network");
	odp.ptszTitle   = m_tszUserName;	// title!!!!!!!!!!!
	odp.flags       = ODPF_BOLDGROUPS|ODPF_TCHAR;

	odp.ptszTab     = LPGENT("Account");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_JABBER);
	odp.pfnDlgProc  = CDlgBase::DynamicDlgProc;
	odp.dwInitParam	= (LPARAM)&OptCreateAccount;
	OptCreateAccount.create = CDlgOptAccount::Create;
	OptCreateAccount.param = this;
	JCallService( MS_OPT_ADDPAGE, wParam, ( LPARAM )&odp );

	odp.ptszTab     = LPGENT("Conferences");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_JABBER4);
	odp.pfnDlgProc  = CDlgBase::DynamicDlgProc;
	odp.dwInitParam	= (LPARAM)&OptCreateGc;
	OptCreateGc.create = CDlgOptGc::Create;
	OptCreateGc.param = this;
	JCallService( MS_OPT_ADDPAGE, wParam, ( LPARAM )&odp );

	odp.flags |= ODPF_EXPERTONLY;

	odp.ptszTab     = LPGENT("Advanced");
	odp.pszTemplate = MAKEINTRESOURCEA( IDD_OPT_JABBER2 );
	odp.pfnDlgProc  = CDlgBase::DynamicDlgProc;
	odp.dwInitParam	= (LPARAM)&OptCreateAdvanced;
	OptCreateAdvanced.create = CDlgOptAdvanced::Create;
	OptCreateAdvanced.param = this;
	JCallService( MS_OPT_ADDPAGE, wParam, ( LPARAM )&odp );

	return 0;
}

void CJabberProto::JabberUpdateDialogs( BOOL bEnable )
{
	if ( rrud.hwndDlg )
		SendMessage(rrud.hwndDlg, JM_STATUSCHANGED, 0,0);
}

int CJabberProto::OnModernOptInit( WPARAM wParam, LPARAM lParam )
{/*
	static int iBoldControls[] =
	{
		IDC_TITLE1, MODERNOPT_CTRL_LAST
	};

	MODERNOPTOBJECT obj = {0};
	obj.cbSize = sizeof(obj);
	obj.dwFlags = MODEROPT_FLG_TCHAR;
	obj.hIcon = LoadIconEx("main");
	obj.hInstance = hInst;
	obj.iSection = MODERNOPT_PAGE_ACCOUNTS;
	obj.iType = MODERNOPT_TYPE_SUBSECTIONPAGE;
	obj.lptzSubsection = mir_a2t(m_szModuleName);	// title!!!!!!!!!!!
	obj.lpzTemplate = MAKEINTRESOURCEA(IDD_MODERNOPT);
	obj.iBoldControls = iBoldControls;
	obj.pfnDlgProc = JabberWizardDlgProc;
	obj.lpszClassicGroup = "Network";
	obj.lpszClassicPage = m_szModuleName;	// title!!!!!!!!!!!
	obj.lpszHelpUrl = "http://forums.miranda-im.org/showthread.php?t=14294";
	CallService(MS_MODERNOPT_ADDOBJECT, wParam, (LPARAM)&obj);
	mir_free(obj.lptzSubsection); */
	return 0;
}
