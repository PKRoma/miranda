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

#define SECURITY_WIN32
#include <security.h>
#include <rpcdce.h>

static HMODULE g_hSecurity = NULL;
static PSecurityFunctionTable g_pSSPI = NULL;

typedef struct
{
	CtxtHandle hClientContext;
	CredHandle hClientCredential;
	char* provider;
	unsigned cbMaxToken;
	bool hasDomain;
}
	NtlmHandleType;

typedef struct
{
	WORD     len;
	WORD     allocedSpace;
	DWORD    offset;
}
	NTLM_String;

typedef struct
{
	char        sign[8];
	DWORD       type;   // == 2
	NTLM_String targetName;
	DWORD       flags;
	BYTE        challenge[8];
	BYTE        context[8];
	NTLM_String targetInfo;
}
	NtlmType2packet;

static unsigned secCnt = 0, ntlmCnt = 0;
static HANDLE hSecMutex;
static TCHAR* szSPN;

static void LoadSecurityLibrary(void)
{
	INIT_SECURITY_INTERFACE pInitSecurityInterface;

	g_hSecurity = LoadLibraryA("secur32.dll");
	if (g_hSecurity == NULL)
		g_hSecurity = LoadLibraryA("security.dll");

	if (g_hSecurity == NULL)
		return;

	pInitSecurityInterface = (INIT_SECURITY_INTERFACE)GetProcAddress(g_hSecurity, SECURITY_ENTRYPOINT_ANSI);
	if (pInitSecurityInterface != NULL)
	{
		g_pSSPI = pInitSecurityInterface();
		
		TCHAR szCompName[128];
		unsigned long szCompNameLen = SIZEOF(szCompName);
		GetComputerName(szCompName, &szCompNameLen);

		TCHAR szUserName[128];
		unsigned long szUserNameLen = SIZEOF(szUserName);
		GetUserName(szUserName, &szUserNameLen);

		unsigned long szSPNLen = szCompNameLen + szUserNameLen + 10;
		szSPN = (TCHAR*)mir_alloc(szSPNLen * sizeof(TCHAR));
		mir_sntprintf(szSPN, szSPNLen, _T("Miranda-%s-%s"), szCompName, szUserName);
	}

	if (g_pSSPI == NULL) 
	{
		FreeLibrary(g_hSecurity);
		g_hSecurity = NULL;
		mir_free(szSPN);
		szSPN = NULL;
	}
}

static void FreeSecurityLibrary(void)
{
	FreeLibrary(g_hSecurity);
	g_hSecurity = NULL;
	g_pSSPI = NULL;
	mir_free(szSPN);
	szSPN = NULL;
}

HANDLE NetlibInitSecurityProvider(char* provider)
{
	HANDLE hSecurity = NULL;

	if (strcmp(provider, "Basic") == 0)
	{
		NtlmHandleType* hNtlm = (NtlmHandleType*)mir_calloc(sizeof(NtlmHandleType));
		hNtlm->provider = mir_strdup(provider);
		SecInvalidateHandle(&hNtlm->hClientContext);
		SecInvalidateHandle(&hNtlm->hClientCredential);
		ntlmCnt++;

		return hNtlm;
	}

	WaitForSingleObject(hSecMutex, INFINITE);

	if (secCnt == 0 ) 
	{
		LoadSecurityLibrary();
		secCnt += g_hSecurity != NULL;
	}
	else secCnt++;

	if (g_pSSPI != NULL) 
	{
		TCHAR* tProvider = mir_a2t(provider);
		PSecPkgInfo ntlmSecurityPackageInfo;
		SECURITY_STATUS sc = g_pSSPI->QuerySecurityPackageInfo(tProvider, &ntlmSecurityPackageInfo);
		if (sc == SEC_E_OK)
		{
			NtlmHandleType* hNtlm;

			hSecurity = hNtlm = (NtlmHandleType*)mir_calloc(sizeof(NtlmHandleType));
			hNtlm->cbMaxToken = ntlmSecurityPackageInfo->cbMaxToken;
			g_pSSPI->FreeContextBuffer(ntlmSecurityPackageInfo);
			hNtlm->provider = mir_strdup(provider);
			SecInvalidateHandle(&hNtlm->hClientContext);
			SecInvalidateHandle(&hNtlm->hClientCredential);
			ntlmCnt++;
		}
		mir_free(tProvider);
	}

	ReleaseMutex(hSecMutex);
	return hSecurity;
}

void NetlibDestroySecurityProvider(HANDLE hSecurity)
{
	if (hSecurity == NULL) return;

	WaitForSingleObject(hSecMutex, INFINITE);

	if (ntlmCnt != 0) 
	{
		NtlmHandleType* hNtlm = (NtlmHandleType*)hSecurity;
		if (SecIsValidHandle(&hNtlm->hClientContext)) g_pSSPI->DeleteSecurityContext(&hNtlm->hClientContext);
		if (SecIsValidHandle(&hNtlm->hClientCredential)) g_pSSPI->FreeCredentialsHandle(&hNtlm->hClientCredential);
		mir_free(hNtlm->provider);

		--ntlmCnt;

		mir_free(hNtlm);
	}

	if (secCnt && --secCnt == 0)
		FreeSecurityLibrary();

	ReleaseMutex(hSecMutex);
}

char* NtlmCreateResponseFromChallenge(HANDLE hSecurity, char *szChallenge, const char* login, const char* psw, 
									  bool http, int& complete)
{
	SECURITY_STATUS sc;
	SecBufferDesc outputBufferDescriptor,inputBufferDescriptor;
	SecBuffer outputSecurityToken,inputSecurityToken;
	TimeStamp tokenExpiration;
	ULONG contextAttributes;
	NETLIBBASE64 nlb64 = { 0 };

	NtlmHandleType* hNtlm = (NtlmHandleType*)hSecurity;

	if (hSecurity == NULL || ntlmCnt == 0) return NULL;

	if (strcmp(hNtlm->provider, "Basic"))
	{
		bool hasChallenge = szChallenge != NULL && szChallenge[0] != '\0';
		if (hasChallenge) 
		{
			nlb64.cchEncoded = lstrlenA(szChallenge);
			nlb64.pszEncoded = szChallenge;
			nlb64.cbDecoded = Netlib_GetBase64DecodedBufferSize(nlb64.cchEncoded);
			nlb64.pbDecoded = (PBYTE)alloca(nlb64.cbDecoded);
			if (!NetlibBase64Decode(0, (LPARAM)&nlb64)) return NULL;

			inputBufferDescriptor.cBuffers=1;
			inputBufferDescriptor.pBuffers=&inputSecurityToken;
			inputBufferDescriptor.ulVersion=SECBUFFER_VERSION;
			inputSecurityToken.BufferType=SECBUFFER_TOKEN;
			inputSecurityToken.cbBuffer=nlb64.cbDecoded;
			inputSecurityToken.pvBuffer=nlb64.pbDecoded;

			// try to decode the domain name from the NTLM challenge
			if (login != NULL && login[0] != '\0' && !hNtlm->hasDomain) 
			{
				NtlmType2packet* pkt = ( NtlmType2packet* )nlb64.pbDecoded;
				if (!strncmp(pkt->sign, "NTLMSSP", 8) && pkt->type == 2) 
				{
					char* domainName = (char*)&nlb64.pbDecoded[pkt->targetName.offset];
					int domainLen = pkt->targetName.len;

					// Negotiate Unicode? if yes, convert the unicode name to ANSI
					if (pkt->flags & 1) 
					{
						char* buf = (char*)alloca(domainLen);
						domainLen = WideCharToMultiByte(CP_ACP, 0, (WCHAR*)domainName, domainLen, buf, domainLen, NULL, NULL);
						domainName = buf;
					}

					if (domainLen) 
					{
						size_t newLoginLen = strlen(login) + domainLen + 1;
						char *newLogin = (char*)alloca(newLoginLen);
						mir_snprintf(newLogin, newLoginLen, "%s\\%s", domainName, login);

						NtlmCreateResponseFromChallenge(hSecurity, NULL, newLogin, psw, http, complete);
					}
				}
			}
		}
		else 
		{
			if (SecIsValidHandle(&hNtlm->hClientContext)) g_pSSPI->DeleteSecurityContext(&hNtlm->hClientContext);
			if (SecIsValidHandle(&hNtlm->hClientCredential)) g_pSSPI->FreeCredentialsHandle(&hNtlm->hClientCredential);

			SEC_WINNT_AUTH_IDENTITY_A auth;

			if (login != NULL && login[0] != '\0') 
			{
				memset(&auth, 0, sizeof(auth));

				const char* loginName = login;
				const char* domainName = strchr(login,'\\');
				int domainLen = 0;
				int loginLen = lstrlenA(loginName);
				if (domainName != NULL) 
				{
					loginName = domainName + 1;
					loginLen = lstrlenA(loginName);
					domainLen = domainName - login;
					domainName = login;
				}
				else if ((domainName = strchr(login,'@')) != NULL) 
				{
					loginName = login;
					loginLen = domainName - login;
					domainLen = lstrlenA(++domainName);
				}

				auth.User = (unsigned char*)loginName;
				auth.UserLength = loginLen;
				auth.Password = (unsigned char*)psw;
				auth.PasswordLength = lstrlenA(psw);
				auth.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
				auth.Domain = (unsigned char*)domainName;
				auth.DomainLength = domainLen;

				hNtlm->hasDomain = true;
			}

			TCHAR* tProvider = mir_a2t(hNtlm->provider);
			sc = g_pSSPI->AcquireCredentialsHandle(NULL, tProvider, SECPKG_CRED_OUTBOUND,
				NULL, hNtlm->hasDomain ? &auth : NULL, NULL, NULL, 
				&hNtlm->hClientCredential, &tokenExpiration);
			mir_free(tProvider);
			if (sc != SEC_E_OK) return NULL;
		}

		outputBufferDescriptor.cBuffers = 1;
		outputBufferDescriptor.pBuffers = &outputSecurityToken;
		outputBufferDescriptor.ulVersion = SECBUFFER_VERSION;
		outputSecurityToken.BufferType = SECBUFFER_TOKEN;
		outputSecurityToken.cbBuffer = hNtlm->cbMaxToken;
		outputSecurityToken.pvBuffer = alloca(outputSecurityToken.cbBuffer);

		sc = g_pSSPI->InitializeSecurityContext(&hNtlm->hClientCredential,
			hasChallenge ? &hNtlm->hClientContext : NULL,
			szSPN, 0, 0, SECURITY_NATIVE_DREP,
			hasChallenge ? &inputBufferDescriptor : NULL, 0, &hNtlm->hClientContext,
			&outputBufferDescriptor, &contextAttributes, &tokenExpiration);

		complete = (sc != SEC_I_COMPLETE_AND_CONTINUE && sc != SEC_I_CONTINUE_NEEDED);

		if (sc == SEC_I_COMPLETE_NEEDED || sc == SEC_I_COMPLETE_AND_CONTINUE)
		{
			sc = g_pSSPI->CompleteAuthToken(&hNtlm->hClientContext, &outputBufferDescriptor);
		}

		if (sc != SEC_E_OK && sc != SEC_I_CONTINUE_NEEDED)
			return NULL;

		nlb64.cbDecoded = outputSecurityToken.cbBuffer;
		nlb64.pbDecoded = (PBYTE)outputSecurityToken.pvBuffer;
	}
	else
	{
		size_t authLen = strlen(login) + strlen(psw) + 5;
		char *szAuth = (char*)alloca(authLen);
		nlb64.cbDecoded = mir_snprintf(szAuth, authLen, "%s:%s", login, psw);
		nlb64.pbDecoded=(PBYTE)szAuth;
		complete = true;
	}

	nlb64.cchEncoded = Netlib_GetBase64EncodedBufferSize(nlb64.cbDecoded);
	nlb64.pszEncoded = (char*)alloca(nlb64.cchEncoded);
	if (!NetlibBase64Encode(0,(LPARAM)&nlb64)) return NULL;

	nlb64.cchEncoded += (int)strlen(hNtlm->provider) + 10;
	char* result = (char*)mir_alloc(nlb64.cchEncoded);
	if (http)
		mir_snprintf(result, nlb64.cchEncoded, "%s %s", hNtlm->provider, nlb64.pszEncoded);
	else
		strcpy(result, nlb64.pszEncoded);

	return result;
}

///////////////////////////////////////////////////////////////////////////////

static INT_PTR InitSecurityProviderService( WPARAM, LPARAM lParam )
{
	return (INT_PTR)NetlibInitSecurityProvider(( char* )lParam );
}

static INT_PTR DestroySecurityProviderService( WPARAM wParam, LPARAM lParam )
{
	NetlibDestroySecurityProvider(( HANDLE )lParam );
	return 0;
}

static INT_PTR NtlmCreateResponseService( WPARAM wParam, LPARAM lParam )
{
	NETLIBNTLMREQUEST* req = ( NETLIBNTLMREQUEST* )lParam;
	return (INT_PTR)NtlmCreateResponseFromChallenge(( HANDLE )wParam, req->szChallenge, 
		req->userName, req->password, false, req->complete );
}

void NetlibSecurityInit(void)
{
	hSecMutex = CreateMutex(NULL, FALSE, NULL);

	CreateServiceFunction( MS_NETLIB_INITSECURITYPROVIDER, InitSecurityProviderService );
	CreateServiceFunction( MS_NETLIB_DESTROYSECURITYPROVIDER, DestroySecurityProviderService );
	CreateServiceFunction( MS_NETLIB_NTLMCREATERESPONSE, NtlmCreateResponseService );
}

void NetlibSecurityDestroy(void)
{
	CloseHandle(hSecMutex);
}
