/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2008 Miranda ICQ/IM project,
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
#include "schannel.h"

//#include <SCHNLSP.H>

static HMODULE     g_hSchannel;
static PSecurityFunctionTableA g_pSSPI;

struct SslHandle
{
	SOCKET s;

	CtxtHandle hContext;
	CredHandle hCreds;

	BYTE *pbRecDataBuf;
	DWORD cbRecDataBuf;
	DWORD sbRecDataBuf;

	BYTE *pbIoBuffer;
	DWORD cbIoBuffer;
	DWORD sbIoBuffer;
};

static int SSL_library_init(void)
{
	INIT_SECURITY_INTERFACE_A pInitSecurityInterface;

	if (g_hSchannel) return 1;

	g_hSchannel = LoadLibraryA("schannel.dll");
	if (g_hSchannel == NULL) return 0;

	pInitSecurityInterface = (INIT_SECURITY_INTERFACE_A)GetProcAddress(g_hSchannel, SECURITY_ENTRYPOINT_ANSIA);
	if (pInitSecurityInterface != NULL) 
		g_pSSPI = pInitSecurityInterface();

	if (g_pSSPI == NULL) {
		FreeLibrary(g_hSchannel);
		g_hSchannel = NULL;
		return 0;
	}

	return 1;
}

static BOOL AcquireCredentials(SslHandle *ssl, BOOL verify)
{
	SCHANNEL_CRED   SchannelCred;
	TimeStamp       tsExpiry;
	SECURITY_STATUS scRet;

	ZeroMemory(&SchannelCred, sizeof(SchannelCred));

	SchannelCred.dwVersion  = SCHANNEL_CRED_VERSION;
	SchannelCred.grbitEnabledProtocols = SP_PROT_SSL3TLS1_CLIENTS;

	SchannelCred.dwFlags |= SCH_CRED_NO_DEFAULT_CREDS;

	if (!verify) 
		SchannelCred.dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION;

	// Create an SSPI credential.
	scRet = g_pSSPI->AcquireCredentialsHandleA(
		NULL,                   // Name of principal    
		UNISP_NAME_A,           // Name of package
		SECPKG_CRED_OUTBOUND,   // Flags indicating use
		NULL,                   // Pointer to logon ID
		&SchannelCred,          // Package specific data
		NULL,                   // Pointer to GetKey() func
		NULL,                   // Value to pass to GetKey()
		&ssl->hCreds,			// (out) Cred Handle
		&tsExpiry);             // (out) Lifetime (optional)

	return scRet == SEC_E_OK;
}

void NetlibSslFree(SslHandle *ssl)
{
	if (ssl == NULL) return;

	g_pSSPI->FreeCredentialsHandle(&ssl->hCreds);
	g_pSSPI->DeleteSecurityContext(&ssl->hContext);

	mir_free(ssl->pbRecDataBuf);
	mir_free(ssl->pbIoBuffer);
    memset(ssl, 0, sizeof(SslHandle));
	mir_free(ssl);
}

BOOL NetlibSslPending(SslHandle *ssl)
{
	return ssl != NULL && ( ssl->cbRecDataBuf != 0 || ssl->cbIoBuffer != 0 );
}

static SECURITY_STATUS ClientHandshakeLoop(SslHandle *ssl, BOOL fDoInitialRead)    
{
	SecBufferDesc   InBuffer;
	SecBuffer       InBuffers[2];
	SecBufferDesc   OutBuffer;
	SecBuffer       OutBuffers[1];
	DWORD           dwSSPIFlags;
	DWORD           dwSSPIOutFlags;
	TimeStamp       tsExpiry;
	SECURITY_STATUS scRet;
	DWORD           cbData;

	BOOL            fDoRead;


	dwSSPIFlags = 
		ISC_REQ_SEQUENCE_DETECT   |
		ISC_REQ_REPLAY_DETECT     |
		ISC_REQ_CONFIDENTIALITY   |
		ISC_REQ_EXTENDED_ERROR    |
		ISC_REQ_ALLOCATE_MEMORY   |
		ISC_REQ_STREAM;

	ssl->cbIoBuffer = 0;

	fDoRead = fDoInitialRead;

	scRet = SEC_I_CONTINUE_NEEDED;

	// Loop until the handshake is finished or an error occurs.
	while(scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_INCOMPLETE_MESSAGE || scRet == SEC_I_INCOMPLETE_CREDENTIALS) 
	{
		// Read server data
		if (0 == ssl->cbIoBuffer || scRet == SEC_E_INCOMPLETE_MESSAGE) 
		{
			if (fDoRead) 
			{
				TIMEVAL tv = {10, 0};
				fd_set fd;

				// If buffer not large enough reallocate buffer
				if (ssl->sbIoBuffer <= ssl->cbIoBuffer) 
				{
					ssl->sbIoBuffer += 2048;
					ssl->pbIoBuffer = (PUCHAR)mir_realloc(ssl->pbIoBuffer, ssl->sbIoBuffer);
				}

				FD_ZERO(&fd);
				FD_SET(ssl->s, &fd);
				if (select(1, &fd, NULL, NULL, &tv) != 1)
				{
					scRet = SEC_E_INTERNAL_ERROR;
					break;
				}

				cbData = recv(ssl->s, 
					(char*)ssl->pbIoBuffer + ssl->cbIoBuffer, 
					ssl->sbIoBuffer - ssl->cbIoBuffer, 
					0);
				if (cbData == SOCKET_ERROR) 
				{
					scRet = SEC_E_INTERNAL_ERROR;
					break;
				}
				if (cbData == 0) 
				{
					scRet = SEC_E_INTERNAL_ERROR;
					break;
				}

				ssl->cbIoBuffer += cbData;
			}
			else fDoRead = TRUE;
		}

		// Set up the input buffers. Buffer 0 is used to pass in data
		// received from the server. Schannel will consume some or all
		// of this. Leftover data (if any) will be placed in buffer 1 and
		// given a buffer type of SECBUFFER_EXTRA.

		InBuffers[0].pvBuffer   = ssl->pbIoBuffer;
		InBuffers[0].cbBuffer   = ssl->cbIoBuffer;
		InBuffers[0].BufferType = SECBUFFER_TOKEN;

		InBuffers[1].pvBuffer   = NULL;
		InBuffers[1].cbBuffer   = 0;
		InBuffers[1].BufferType = SECBUFFER_EMPTY;

		InBuffer.cBuffers       = 2;
		InBuffer.pBuffers       = InBuffers;
		InBuffer.ulVersion      = SECBUFFER_VERSION;

		// Set up the output buffers. These are initialized to NULL
		// so as to make it less likely we'll attempt to free random
		// garbage later.

		OutBuffers[0].pvBuffer  = NULL;
		OutBuffers[0].BufferType= SECBUFFER_TOKEN;
		OutBuffers[0].cbBuffer  = 0;

		OutBuffer.cBuffers      = 1;
		OutBuffer.pBuffers      = OutBuffers;
		OutBuffer.ulVersion     = SECBUFFER_VERSION;

		scRet = g_pSSPI->InitializeSecurityContextA(&ssl->hCreds,
			&ssl->hContext,
			NULL,
			dwSSPIFlags,
			0,
			SECURITY_NATIVE_DREP,
			&InBuffer,
			0,
			NULL,
			&OutBuffer,
			&dwSSPIOutFlags,
			&tsExpiry);

		// If success (or if the error was one of the special extended ones), 
		// send the contents of the output buffer to the server.
		if (scRet == SEC_E_OK                ||
			scRet == SEC_I_CONTINUE_NEEDED   ||
			FAILED(scRet) && (dwSSPIOutFlags & ISC_REQ_EXTENDED_ERROR))
		{
			if (OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL) 
			{
				cbData = send(ssl->s,
					(char*)OutBuffers[0].pvBuffer,
					OutBuffers[0].cbBuffer,
					0);
				if (cbData == SOCKET_ERROR || cbData == 0) 
				{
					g_pSSPI->FreeContextBuffer(OutBuffers[0].pvBuffer);
					g_pSSPI->DeleteSecurityContext(&ssl->hContext);
					return SEC_E_INTERNAL_ERROR;
				}

				// Free output buffer.
				g_pSSPI->FreeContextBuffer(OutBuffers[0].pvBuffer);
				OutBuffers[0].pvBuffer = NULL;
			}
		}

		// we need to read more data from the server and try again.
		if (scRet == SEC_E_INCOMPLETE_MESSAGE) continue;

		// handshake completed successfully.
		if (scRet == SEC_E_OK) 
		{
			// Store remaining data for further use
			if (InBuffers[1].BufferType == SECBUFFER_EXTRA) 
			{
				MoveMemory(ssl->pbIoBuffer,
					ssl->pbIoBuffer + (ssl->cbIoBuffer - InBuffers[1].cbBuffer),
					InBuffers[1].cbBuffer);
				ssl->cbIoBuffer = InBuffers[1].cbBuffer;
			}
			else
				ssl->cbIoBuffer = 0;
			break;
		}

		// Check for fatal error.
		if (FAILED(scRet)) break;

		// server just requested client authentication. 
		if (scRet == SEC_I_INCOMPLETE_CREDENTIALS) {
			// Server has requested client authentication and
			//			GetNewClientCredentials(ssl);

			// Go around again.
			fDoRead = FALSE;
			scRet = SEC_I_CONTINUE_NEEDED;
			continue;
		}


		// Copy any leftover data from the buffer, and go around again.
		if ( InBuffers[1].BufferType == SECBUFFER_EXTRA ) 
		{
			MoveMemory(ssl->pbIoBuffer,
				ssl->pbIoBuffer + (ssl->cbIoBuffer - InBuffers[1].cbBuffer),
				InBuffers[1].cbBuffer);

			ssl->cbIoBuffer = InBuffers[1].cbBuffer;
		}
		else ssl->cbIoBuffer = 0;
	}

	// Delete the security context in the case of a fatal error.
	if (FAILED(scRet))
		g_pSSPI->DeleteSecurityContext(&ssl->hContext);

	if (ssl->cbIoBuffer == 0) 
	{
		mir_free(ssl->pbIoBuffer);
		ssl->pbIoBuffer = NULL;
		ssl->sbIoBuffer = 0;
	}

	return scRet;
}

static int ClientConnect(SslHandle *ssl, const char *host)
{
	SecBufferDesc   OutBuffer;
	SecBuffer       OutBuffers[1];
	DWORD           dwSSPIFlags;
	DWORD           dwSSPIOutFlags;
	TimeStamp       tsExpiry;
	SECURITY_STATUS scRet;
	DWORD           cbData;

	dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT   |
		ISC_REQ_REPLAY_DETECT     |
		ISC_REQ_CONFIDENTIALITY   |
		ISC_RET_EXTENDED_ERROR    |
		ISC_REQ_ALLOCATE_MEMORY   |
		ISC_REQ_STREAM;

	//  Initiate a ClientHello message and generate a token.

	OutBuffers[0].pvBuffer   = NULL;
	OutBuffers[0].BufferType = SECBUFFER_TOKEN;
	OutBuffers[0].cbBuffer   = 0;

	OutBuffer.cBuffers = 1;
	OutBuffer.pBuffers = OutBuffers;
	OutBuffer.ulVersion = SECBUFFER_VERSION;

	scRet = g_pSSPI->InitializeSecurityContextA(
		&ssl->hCreds,
		NULL,
		(SEC_CHAR*)host,
		dwSSPIFlags,
		0,
		SECURITY_NATIVE_DREP,
		NULL,
		0,
		&ssl->hContext,
		&OutBuffer,
		&dwSSPIOutFlags,
		&tsExpiry);

	if (scRet != SEC_I_CONTINUE_NEEDED)
		return 0;

	// Send response to server if there is one.
	if (OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL) 
	{
		cbData = send(ssl->s, (char*)OutBuffers[0].pvBuffer, OutBuffers[0].cbBuffer, 0);
		if (cbData == SOCKET_ERROR || cbData == 0) 
		{
			g_pSSPI->FreeContextBuffer(OutBuffers[0].pvBuffer);
			g_pSSPI->DeleteSecurityContext(&ssl->hContext);
			return 0;
		}

		// Free output buffer.
		g_pSSPI->FreeContextBuffer(OutBuffers[0].pvBuffer);
		OutBuffers[0].pvBuffer = NULL;
	}

	return ClientHandshakeLoop(ssl, TRUE) == SEC_E_OK;
}


SslHandle *NetlibSslConnect(BOOL verify, SOCKET s, const char* host)
{
	BOOL res;

	SslHandle *ssl = (SslHandle*)mir_calloc(sizeof(SslHandle));
	ssl->s = s;

	res = SSL_library_init();

	if (res) res = AcquireCredentials(ssl, verify);
	if (res) res = ClientConnect(ssl, host);

	if (!res) 
	{
		NetlibSslFree(ssl); 
		ssl = NULL;
	}
	return ssl;
}


void NetlibSslShutdown(SslHandle *ssl)
{
	DWORD           dwType;

	SecBufferDesc   OutBuffer;
	SecBuffer       OutBuffers[1];
	DWORD           dwSSPIFlags;
	DWORD           dwSSPIOutFlags;
	TimeStamp       tsExpiry;
	DWORD           scRet;

	if (ssl == NULL) return;

	dwType = SCHANNEL_SHUTDOWN;

	OutBuffers[0].pvBuffer   = &dwType;
	OutBuffers[0].BufferType = SECBUFFER_TOKEN;
	OutBuffers[0].cbBuffer   = sizeof(dwType);

	OutBuffer.cBuffers  = 1;
	OutBuffer.pBuffers  = OutBuffers;
	OutBuffer.ulVersion = SECBUFFER_VERSION;

	scRet = g_pSSPI->ApplyControlToken(&ssl->hContext, &OutBuffer);
	if (FAILED(scRet)) return;

	//
	// Build an SSL close notify message.
	//

	dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT   |
		ISC_REQ_REPLAY_DETECT     |
		ISC_REQ_CONFIDENTIALITY   |
		ISC_RET_EXTENDED_ERROR    |
		ISC_REQ_ALLOCATE_MEMORY   |
		ISC_REQ_STREAM;

	OutBuffers[0].pvBuffer   = NULL;
	OutBuffers[0].BufferType = SECBUFFER_TOKEN;
	OutBuffers[0].cbBuffer   = 0;

	OutBuffer.cBuffers  = 1;
	OutBuffer.pBuffers  = OutBuffers;
	OutBuffer.ulVersion = SECBUFFER_VERSION;

	scRet = g_pSSPI->InitializeSecurityContextA(
		&ssl->hCreds,
		&ssl->hContext,
		NULL,
		dwSSPIFlags,
		0,
		SECURITY_NATIVE_DREP,
		NULL,
		0,
		&ssl->hContext,
		&OutBuffer,
		&dwSSPIOutFlags,
		&tsExpiry);

	if (FAILED(scRet)) return;

	// Send the close notify message to the server.
	if (OutBuffers[0].pvBuffer != NULL && OutBuffers[0].cbBuffer != 0) 
	{
		send(ssl->s, (char*)OutBuffers[0].pvBuffer, OutBuffers[0].cbBuffer, 0);
		g_pSSPI->FreeContextBuffer(OutBuffers[0].pvBuffer);
	}
}

int NetlibSslRead(SslHandle *ssl, char *buf, int num, int peek)
{
	SECURITY_STATUS scRet;
	DWORD           cbData;
	DWORD			resNum;
	int				i;

	SecBufferDesc   Message;
	SecBuffer       Buffers[4];
	SecBuffer *     pDataBuffer;
	SecBuffer *     pExtraBuffer;

	if (ssl == NULL) return SOCKET_ERROR;

	if (num == 0) return 0;

	if (ssl->cbRecDataBuf != 0 && (!peek || ssl->cbRecDataBuf >= (DWORD)num))
	{
getdata:
		DWORD bytes = min((DWORD)num, ssl->cbRecDataBuf);
		DWORD rbytes = ssl->cbRecDataBuf - bytes;

		CopyMemory(buf, ssl->pbRecDataBuf, bytes);
		if (!peek) 
		{
			MoveMemory(ssl->pbRecDataBuf, ((char*)ssl->pbRecDataBuf)+bytes, rbytes);
			ssl->cbRecDataBuf = rbytes;
		}

		return bytes;
	}

	scRet = SEC_E_OK;

	for (;;) 
	{
		if (0 == ssl->cbIoBuffer || scRet == SEC_E_INCOMPLETE_MESSAGE) 
		{
			if (ssl->sbIoBuffer <= ssl->cbIoBuffer) 
			{
				ssl->sbIoBuffer += 2048;
				ssl->pbIoBuffer = (PUCHAR)mir_realloc(ssl->pbIoBuffer, ssl->sbIoBuffer);
			}

			if (peek) 
			{
				TIMEVAL tv = {0};
				fd_set fd;
				FD_ZERO(&fd);
				FD_SET(ssl->s, &fd);

				cbData = select(1, &fd, NULL, NULL, &tv);
				if (cbData == SOCKET_ERROR) return SOCKET_ERROR;
				
				if (cbData == 0 && ssl->cbRecDataBuf)
				{
					DWORD bytes = min((DWORD)num, ssl->cbRecDataBuf);
					CopyMemory(buf, ssl->pbRecDataBuf, bytes);
					return bytes;
				}
			}

			cbData = recv(ssl->s, (char*)ssl->pbIoBuffer + ssl->cbIoBuffer, ssl->sbIoBuffer - ssl->cbIoBuffer, 0);
			if (cbData == SOCKET_ERROR) return SOCKET_ERROR;
			
			if (cbData == 0) 
			{
				if (peek && ssl->cbRecDataBuf)
				{
					DWORD bytes = min((DWORD)num, ssl->cbRecDataBuf);
					CopyMemory(buf, ssl->pbRecDataBuf, bytes);
					return bytes;
				}

				// Server disconnected.
				if (ssl->cbIoBuffer) 
				{
					scRet = SEC_E_INTERNAL_ERROR;
					return SOCKET_ERROR;
				}
				
				return 0;
			}
			else ssl->cbIoBuffer += cbData;
		}

		// Attempt to decrypt the received data.
		Buffers[0].pvBuffer     = ssl->pbIoBuffer;
		Buffers[0].cbBuffer     = ssl->cbIoBuffer;
		Buffers[0].BufferType   = SECBUFFER_DATA;

		Buffers[1].BufferType   = SECBUFFER_EMPTY;
		Buffers[2].BufferType   = SECBUFFER_EMPTY;
		Buffers[3].BufferType   = SECBUFFER_EMPTY;

		Message.ulVersion       = SECBUFFER_VERSION;
		Message.cBuffers        = 4;
		Message.pBuffers        = Buffers;

		if (g_pSSPI->DecryptMessage != NULL)
			scRet = g_pSSPI->DecryptMessage(&ssl->hContext, &Message, 0, NULL);
		else
			scRet = ((DECRYPT_MESSAGE_FN)g_pSSPI->Reserved4)(&ssl->hContext, &Message, 0, NULL);

		// The input buffer contains only a fragment of an
		// encrypted record. Loop around and read some more
		// data.
		if (scRet == SEC_E_INCOMPLETE_MESSAGE)
			continue;

		// Server signaled end of session
		if (scRet == SEC_I_CONTEXT_EXPIRED) 
		{
			NetlibSslShutdown(ssl);
			if (peek && ssl->cbRecDataBuf) goto getdata;
            return 0;
		}

		if ( scRet != SEC_E_OK && scRet != SEC_I_RENEGOTIATE && scRet != SEC_I_CONTEXT_EXPIRED)
        {
			if (peek && ssl->cbRecDataBuf) goto getdata;
			return SOCKET_ERROR;
        }

		// Locate data and (optional) extra buffers.
		pDataBuffer  = NULL;
		pExtraBuffer = NULL;
		for(i = 1; i < 4; i++) 
		{
			if (pDataBuffer == NULL && Buffers[i].BufferType == SECBUFFER_DATA)
				pDataBuffer = &Buffers[i];

			if (pExtraBuffer == NULL && Buffers[i].BufferType == SECBUFFER_EXTRA)
				pExtraBuffer = &Buffers[i];
		}

		// Return decrypted data.
		if (pDataBuffer) 
		{
			DWORD bytes, rbytes;

			bytes = peek ? 0 : min((DWORD)num, pDataBuffer->cbBuffer);
			rbytes = pDataBuffer->cbBuffer - bytes;

			if (rbytes > 0) 
			{
				DWORD nbytes = ssl->cbRecDataBuf + rbytes;
				if (ssl->sbRecDataBuf < nbytes) 
				{
					ssl->sbRecDataBuf = nbytes;
					ssl->pbRecDataBuf = (PUCHAR)mir_realloc(ssl->pbRecDataBuf, nbytes);
				}
				CopyMemory(ssl->pbRecDataBuf + ssl->cbRecDataBuf, (char*)pDataBuffer->pvBuffer+bytes, rbytes);
				ssl->cbRecDataBuf = nbytes;
			}

			if (peek)
			{
				resNum = bytes = min((DWORD)num, ssl->cbRecDataBuf);
				CopyMemory(buf, ssl->pbRecDataBuf, bytes);
			}
			else
			{
				resNum = bytes;
				CopyMemory(buf, pDataBuffer->pvBuffer, bytes);
			}
		}

		// Move any "extra" data to the input buffer.
		if (pExtraBuffer) {
			MoveMemory(ssl->pbIoBuffer, pExtraBuffer->pvBuffer, pExtraBuffer->cbBuffer);
			ssl->cbIoBuffer = pExtraBuffer->cbBuffer;
		}
		else ssl->cbIoBuffer = 0;

		if (pDataBuffer && resNum) return resNum;

		if (scRet == SEC_I_RENEGOTIATE)
		{
			// The server wants to perform another handshake
			// sequence.

			scRet = ClientHandshakeLoop(ssl, FALSE);
			if (scRet != SEC_E_OK) return SOCKET_ERROR;
		}
	}
}

int NetlibSslWrite(SslHandle *ssl, const char *buf, int num)
{
	SecPkgContext_StreamSizes Sizes;
	SECURITY_STATUS scRet;
	DWORD           cbData;

	SecBufferDesc   Message;
	SecBuffer       Buffers[4] = {0};

	PUCHAR			pbDataBuffer;

	PUCHAR			pbMessage;
	DWORD			cbMessage;

	DWORD			sendOff = 0;

	if (ssl == NULL) return SOCKET_ERROR;

	scRet = g_pSSPI->QueryContextAttributesA(&ssl->hContext, SECPKG_ATTR_STREAM_SIZES, &Sizes);
	if (scRet != SEC_E_OK) return scRet;

	pbDataBuffer = (PUCHAR)mir_calloc(Sizes.cbMaximumMessage + Sizes.cbHeader + Sizes.cbTrailer);

	pbMessage = pbDataBuffer + Sizes.cbHeader;

	while (sendOff < (DWORD)num)
	{
		cbMessage = min(Sizes.cbMaximumMessage, (DWORD)num - sendOff);
		CopyMemory(pbMessage, buf+sendOff, cbMessage);

		Buffers[0].pvBuffer     = pbDataBuffer;
		Buffers[0].cbBuffer     = Sizes.cbHeader;
		Buffers[0].BufferType   = SECBUFFER_STREAM_HEADER;

		Buffers[1].pvBuffer     = pbMessage;
		Buffers[1].cbBuffer     = cbMessage;
		Buffers[1].BufferType   = SECBUFFER_DATA;

		Buffers[2].pvBuffer     = pbMessage + cbMessage;
		Buffers[2].cbBuffer     = Sizes.cbTrailer;
		Buffers[2].BufferType   = SECBUFFER_STREAM_TRAILER;

		Buffers[3].BufferType   = SECBUFFER_EMPTY;

		Message.ulVersion       = SECBUFFER_VERSION;
		Message.cBuffers        = 4;
		Message.pBuffers        = Buffers;

		if (g_pSSPI->EncryptMessage != NULL)
			scRet = g_pSSPI->EncryptMessage(&ssl->hContext, 0, &Message, 0);
		else
			scRet = ((ENCRYPT_MESSAGE_FN)g_pSSPI->Reserved3)(&ssl->hContext, 0, &Message, 0);

		if (FAILED(scRet)) break;

		// Calculate encrypted packet size 
		cbData = Buffers[0].cbBuffer + Buffers[1].cbBuffer + Buffers[2].cbBuffer;

		// Send the encrypted data to the server.
		cbData = send(ssl->s, (char*)pbDataBuffer, cbData, 0);
		if (cbData == SOCKET_ERROR || cbData == 0)
		{
			g_pSSPI->DeleteSecurityContext(&ssl->hContext);
			scRet = SEC_E_INTERNAL_ERROR;
			break;
		}

		sendOff += cbMessage;
	}

	mir_free(pbDataBuffer);
	return scRet == SEC_E_OK ? num : SOCKET_ERROR;
}
