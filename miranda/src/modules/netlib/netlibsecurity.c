/*
Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project,
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

static HMODULE g_hSecurity = NULL;
static PSecurityFunctionTableA g_pSSPI = NULL;
static PSecPkgInfoA ntlmSecurityPackageInfo = NULL;

typedef struct
{
	CtxtHandle hClientContext;
	CredHandle hClientCredential;
	unsigned stage;
}
	NtlmHandleType;

static unsigned secCnt = 0, ntlmCnt = 0;
static HANDLE hSecMutex; 

static void LoadSecurityLibrary(void)
{
	INIT_SECURITY_INTERFACE_A pInitSecurityInterface;

	g_hSecurity = LoadLibraryA("secur32.dll");
	if (g_hSecurity == NULL)
		g_hSecurity = LoadLibraryA("security.dll");

	if (g_hSecurity == NULL) 
		return;

	pInitSecurityInterface = (INIT_SECURITY_INTERFACE_A)GetProcAddress(g_hSecurity, "InitSecurityInterfaceA");
	if (pInitSecurityInterface != NULL) 
		g_pSSPI = pInitSecurityInterface();

	if (g_pSSPI == NULL) {
		FreeLibrary(g_hSecurity);
		g_hSecurity = NULL;
	}
}

static void FreeSecurityLibrary(void)
{
	FreeLibrary(g_hSecurity);
	g_hSecurity = NULL;
	g_pSSPI = NULL;
}

HANDLE NetlibInitSecurityProvider(char* provider)
{
	HANDLE hSecurity = NULL;

	WaitForSingleObject(hSecMutex, INFINITE);

	if (secCnt == 0 ) {
		LoadSecurityLibrary();
		secCnt += g_hSecurity != NULL;
	}
	else secCnt++;

	if (g_pSSPI != NULL) {
		if (strcmp(provider, "NTLM") == 0) {
			NtlmHandleType* hNtlm = NULL;
			SECURITY_STATUS sc = SEC_E_OK;

			if (ntlmCnt == 0)
				sc = g_pSSPI->QuerySecurityPackageInfoA(provider, &ntlmSecurityPackageInfo);

			if (sc == SEC_E_OK) {
				hNtlm = mir_alloc(sizeof(NtlmHandleType));
				hNtlm->stage = 0;
				hSecurity = (HANDLE)hNtlm;
				ntlmCnt++;
	}	}	}

	ReleaseMutex(hSecMutex);
	return hSecurity;
}

void NetlibDestroySecurityProvider(char* provider, HANDLE hSecurity)
{
	if (hSecurity == NULL) return;

	WaitForSingleObject(hSecMutex, INFINITE);

	if (strcmp(provider, "NTLM") == 0 && ntlmCnt != 0) {
		NtlmHandleType* hNtlm = (NtlmHandleType*)hSecurity;
		if (hNtlm->stage != 0) {
			g_pSSPI->DeleteSecurityContext(&hNtlm->hClientContext);
			g_pSSPI->FreeCredentialsHandle(&hNtlm->hClientCredential);
		}

		if (ntlmCnt != 0) --ntlmCnt;

		if (ntlmCnt == 0)
			g_pSSPI->FreeContextBuffer(ntlmSecurityPackageInfo);
		mir_free( hNtlm );
	}

	if (secCnt != 0) --secCnt;

	if (secCnt == 0)
		FreeSecurityLibrary();

	ReleaseMutex(hSecMutex);
}

char* NtlmCreateResponseFromChallenge(HANDLE hSecurity, char *szChallenge)
{
	SECURITY_STATUS sc;
	SecBufferDesc outputBufferDescriptor,inputBufferDescriptor;
	SecBuffer outputSecurityToken,inputSecurityToken;
	TimeStamp tokenExpiration;
	ULONG contextAttributes;
	NETLIBBASE64 nlb64;

	NtlmHandleType* hNtlm = (NtlmHandleType*)hSecurity;

	if (hSecurity == NULL || ntlmCnt == 0) return NULL;

	if (*szChallenge != 0) {
		nlb64.cchEncoded = lstrlenA(szChallenge);
		nlb64.pszEncoded = szChallenge;
		nlb64.cbDecoded = Netlib_GetBase64DecodedBufferSize(nlb64.cchEncoded);
		nlb64.pbDecoded = (PBYTE)mir_alloc(nlb64.cbDecoded);
		if (!NetlibBase64Decode(0, (LPARAM)&nlb64)) {
			mir_free(nlb64.pbDecoded); 
			return NULL;
		}

		inputBufferDescriptor.cBuffers=1;
		inputBufferDescriptor.pBuffers=&inputSecurityToken;
		inputBufferDescriptor.ulVersion=SECBUFFER_VERSION;
		inputSecurityToken.BufferType=SECBUFFER_TOKEN;
		inputSecurityToken.cbBuffer=nlb64.cbDecoded;
		inputSecurityToken.pvBuffer=nlb64.pbDecoded;
	}
	else {
		if (hNtlm->stage != 0) {
			g_pSSPI->DeleteSecurityContext(&hNtlm->hClientContext);
			g_pSSPI->FreeCredentialsHandle(&hNtlm->hClientCredential);
		}

		sc = g_pSSPI->AcquireCredentialsHandleA(NULL, "NTLM", SECPKG_CRED_OUTBOUND,
			NULL, NULL, NULL, NULL, &hNtlm->hClientCredential, &tokenExpiration);
		hNtlm->stage = 0;
	}

	hNtlm->stage++;

	outputBufferDescriptor.cBuffers = 1;
	outputBufferDescriptor.pBuffers = &outputSecurityToken;
	outputBufferDescriptor.ulVersion = SECBUFFER_VERSION;
	outputSecurityToken.BufferType = SECBUFFER_TOKEN;
	outputSecurityToken.cbBuffer = ntlmSecurityPackageInfo->cbMaxToken;
	outputSecurityToken.pvBuffer = mir_alloc(outputSecurityToken.cbBuffer);

	sc = g_pSSPI->InitializeSecurityContextA(&hNtlm->hClientCredential, 
		*szChallenge ? &hNtlm->hClientContext : NULL,
		NULL, 0, 0, SECURITY_NATIVE_DREP, 
		*szChallenge ? &inputBufferDescriptor : NULL, 0, &hNtlm->hClientContext,
		&outputBufferDescriptor, &contextAttributes, &tokenExpiration);

	if (*szChallenge != 0) {
		mir_free(nlb64.pbDecoded);
		g_pSSPI->DeleteSecurityContext(&hNtlm->hClientContext);
		g_pSSPI->FreeCredentialsHandle(&hNtlm->hClientCredential);
		hNtlm->stage = 0;
	}

	if (sc != SEC_E_OK && sc != SEC_I_CONTINUE_NEEDED) {
		mir_free(outputSecurityToken.pvBuffer); 
		return NULL;
	}

	nlb64.cbDecoded = outputSecurityToken.cbBuffer;
	nlb64.pbDecoded = outputSecurityToken.pvBuffer;
	nlb64.cchEncoded = Netlib_GetBase64EncodedBufferSize(nlb64.cbDecoded);
	nlb64.pszEncoded = (char*)mir_alloc(nlb64.cchEncoded);
	if (!NetlibBase64Encode(0,(LPARAM)&nlb64)) {
		mir_free(nlb64.pszEncoded); 
		mir_free(outputSecurityToken.pvBuffer); 
		return NULL;
	}
	mir_free(outputSecurityToken.pvBuffer);
	return nlb64.pszEncoded;
}

///////////////////////////////////////////////////////////////////////////////

static int InitSecurityProviderService( WPARAM wParam, LPARAM lParam )
{
	return (int)NetlibInitSecurityProvider(( char* )lParam );
}

static int DestroySecurityProviderService( WPARAM wParam, LPARAM lParam )
{
	NetlibDestroySecurityProvider(( char* )wParam, ( HANDLE )lParam );
	return 0;
}

static int NtlmCreateResponseService( WPARAM wParam, LPARAM lParam )
{
	return (int)NtlmCreateResponseFromChallenge(( HANDLE )wParam, ( char* )lParam );
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
