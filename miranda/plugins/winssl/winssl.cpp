/*
SChannel to OpenSSL wrapper
Copyright (c) 2008 Boris Krasnovskiy.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <windows.h>

#define SECURITY_WIN32
#include <security.h>

#include "schannel.h"

//#include <SCHNLSP.H>

HMODULE     g_hSecurity;
PSecurityFunctionTableA g_pSSPI;

struct SSL_CTX
{
	DWORD dwProtocol;
	BOOL  bVerify;
};

struct SSL_METHOD { unsigned dummy; };

struct SSL
{
	SOCKET s;

	SSL_CTX *ctx;

	CtxtHandle hContext;
	CredHandle hCreds;

	BYTE *pbRecDataBuf;
	DWORD cbRecDataBuf;
	DWORD sbRecDataBuf;

	BYTE *pbIoBuffer;
	DWORD cbIoBuffer;
	DWORD sbIoBuffer;

	BOOL rmshtdn;
};


extern "C" __declspec(dllexport)
int SSL_library_init(void)
{
	INIT_SECURITY_INTERFACE_A pInitSecurityInterface;

	if (g_hSecurity) return 1;

	g_hSecurity = LoadLibraryA("schannel.dll");
	if (g_hSecurity == NULL) return 0;

	pInitSecurityInterface = (INIT_SECURITY_INTERFACE_A)GetProcAddress(g_hSecurity, SECURITY_ENTRYPOINT_ANSIA);
	if (pInitSecurityInterface != NULL) 
		g_pSSPI = pInitSecurityInterface();

	if (g_pSSPI == NULL) 
	{
		FreeLibrary(g_hSecurity);
		g_hSecurity = NULL;
		return 0;
	}
	
	return 1;
}

extern "C" __declspec(dllexport)
int SSL_set_fd(SSL *ssl, int fd)
{
	ssl->s = (SOCKET)fd;
	return 1;
}

extern "C" __declspec(dllexport)
SSL_CTX *SSL_CTX_new(SSL_METHOD *method)
{
	SSL_CTX* ctx = (SSL_CTX*)calloc(1, sizeof(SSL_CTX));

	ctx->dwProtocol = (DWORD)method;

	return ctx;
}


extern "C" __declspec(dllexport)
void SSL_CTX_free(SSL_CTX *ctx)
{
	free(ctx);
}

extern "C" __declspec(dllexport)
SSL *SSL_new(SSL_CTX *ctx)
{
	SCHANNEL_CRED   SchannelCred;
    TimeStamp       tsExpiry;
    SECURITY_STATUS scRet;

	SSL *ssl = (SSL*)calloc(1, sizeof(SSL));
	ssl->ctx = ctx;

    ZeroMemory(&SchannelCred, sizeof(SchannelCred));

    SchannelCred.dwVersion  = SCHANNEL_CRED_VERSION;
	SchannelCred.grbitEnabledProtocols = ctx->dwProtocol;

//	SchannelCred.dwFlags |= SCH_CRED_NO_SERVERNAME_CHECK;

	if (!ctx->bVerify) 
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
   
	
	if (scRet != SEC_E_OK)
    {
		free(ssl); 
		ssl = NULL;
    }

	return ssl;
}

extern "C" __declspec(dllexport)
void SSL_free(SSL *ssl)
{
	g_pSSPI->FreeCredentialsHandle(&ssl->hCreds);
    g_pSSPI->DeleteSecurityContext(&ssl->hContext);

	free(ssl->pbRecDataBuf);
	free(ssl->pbIoBuffer);
	free(ssl);
}

static SECURITY_STATUS ClientHandshakeLoop(SSL *ssl, BOOL fDoInitialRead)    
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


    dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT   |
                  ISC_REQ_REPLAY_DETECT     |
                  ISC_REQ_CONFIDENTIALITY   |
                  ISC_RET_EXTENDED_ERROR    |
                  ISC_REQ_ALLOCATE_MEMORY   |
                  ISC_REQ_STREAM;

    ssl->cbIoBuffer = 0;

    fDoRead = fDoInitialRead;

    scRet = SEC_I_CONTINUE_NEEDED;

    // Loop until the handshake is finished or an error occurs.
    while(scRet == SEC_I_CONTINUE_NEEDED        ||
          scRet == SEC_E_INCOMPLETE_MESSAGE     ||
          scRet == SEC_I_INCOMPLETE_CREDENTIALS) 
   {
        // Read server data
        if(0 == ssl->cbIoBuffer || scRet == SEC_E_INCOMPLETE_MESSAGE)
        {
            if(fDoRead)
            {
				// If buffer not large enough reallocate buffer
				if (ssl->sbIoBuffer <= ssl->cbIoBuffer)
				{
					ssl->sbIoBuffer += 2048;
					ssl->pbIoBuffer = (PUCHAR)realloc(ssl->pbIoBuffer, ssl->sbIoBuffer);
				}

                cbData = recv(ssl->s,  
					          (char*)ssl->pbIoBuffer + ssl->cbIoBuffer, 
                              ssl->sbIoBuffer - ssl->cbIoBuffer, 
                              0);
                if(cbData == SOCKET_ERROR)
                {
                    scRet = SEC_E_INTERNAL_ERROR;
                    break;
                }
                else if(cbData == 0)
                {
                    scRet = SEC_E_INTERNAL_ERROR;
                    break;
                }

                ssl->cbIoBuffer += cbData;
            }
            else
            {
                fDoRead = TRUE;
            }
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
        if(scRet == SEC_E_OK                ||
           scRet == SEC_I_CONTINUE_NEEDED   ||
           FAILED(scRet) && (dwSSPIOutFlags & ISC_RET_EXTENDED_ERROR))
        {
            if(OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL)
            {
                cbData = send(ssl->s,
                              (char*)OutBuffers[0].pvBuffer,
                              OutBuffers[0].cbBuffer,
                              0);
                if(cbData == SOCKET_ERROR || cbData == 0)
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
        if(scRet == SEC_E_INCOMPLETE_MESSAGE) continue;

        // handshake completed successfully.
        if(scRet == SEC_E_OK)
        {
            // Store remaining data for further use
            if(InBuffers[1].BufferType == SECBUFFER_EXTRA)
            {
                MoveMemory(ssl->pbIoBuffer,
                           ssl->pbIoBuffer + (ssl->cbIoBuffer - InBuffers[1].cbBuffer),
                           InBuffers[1].cbBuffer);
            }
            else
            {
                ssl->cbIoBuffer   = 0;
            }
            break;
        }


        // Check for fatal error.
        if(FAILED(scRet)) break;

        // server just requested client authentication. 
        if(scRet == SEC_I_INCOMPLETE_CREDENTIALS)
        {
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
        else
            ssl->cbIoBuffer = 0;
    }

    // Delete the security context in the case of a fatal error.
    if (FAILED(scRet))
    {
        g_pSSPI->DeleteSecurityContext(&ssl->hContext);
    }

	if (ssl->cbIoBuffer == 0)
	{
		free(ssl->pbIoBuffer);
		ssl->pbIoBuffer = NULL;
        ssl->sbIoBuffer = 0;
	}

    return scRet;
}

extern "C" __declspec(dllexport)
int SSL_connect(SSL *ssl)
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

	struct sockaddr_in sock;
	int	slen = sizeof(struct sockaddr);
	getpeername(ssl->s, (struct sockaddr *) &sock, &slen);

    scRet = g_pSSPI->InitializeSecurityContextA(
					&ssl->hCreds,
                    NULL,
                    inet_ntoa(sock.sin_addr),
                    dwSSPIFlags,
                    0,
                    SECURITY_NATIVE_DREP,
                    NULL,
                    0,
                    &ssl->hContext,
                    &OutBuffer,
                    &dwSSPIOutFlags,
                    &tsExpiry);

    if(scRet != SEC_I_CONTINUE_NEEDED)
    {
        return 0;
    }

    // Send response to server if there is one.
    if(OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL)
    {
        cbData = send(ssl->s, 
                      (char*)OutBuffers[0].pvBuffer,
                      OutBuffers[0].cbBuffer,
                      0);
        if(cbData == SOCKET_ERROR || cbData == 0)
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

extern "C" __declspec(dllexport)
int SSL_shutdown(SSL *ssl)
{
    DWORD           dwType;

    SecBufferDesc   OutBuffer;
    SecBuffer       OutBuffers[1];
    DWORD           dwSSPIFlags;
    DWORD           dwSSPIOutFlags;
    TimeStamp       tsExpiry;
    DWORD           Status;

    dwType = SCHANNEL_SHUTDOWN;

    OutBuffers[0].pvBuffer   = &dwType;
    OutBuffers[0].BufferType = SECBUFFER_TOKEN;
    OutBuffers[0].cbBuffer   = sizeof(dwType);

    OutBuffer.cBuffers  = 1;
    OutBuffer.pBuffers  = OutBuffers;
    OutBuffer.ulVersion = SECBUFFER_VERSION;

    Status = g_pSSPI->ApplyControlToken(&ssl->hContext, &OutBuffer);
    if (FAILED(Status)) return ssl->rmshtdn;

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

    Status = g_pSSPI->InitializeSecurityContextA(
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

    if (FAILED(Status)) return ssl->rmshtdn;

    // Send the close notify message to the server.
    if(OutBuffers[0].pvBuffer != NULL && OutBuffers[0].cbBuffer != 0)
    {
        send(ssl->s, (char*)OutBuffers[0].pvBuffer, OutBuffers[0].cbBuffer, 0);
        g_pSSPI->FreeContextBuffer(OutBuffers[0].pvBuffer);
    }
    
    // Free the security context.
    g_pSSPI->DeleteSecurityContext(&ssl->hContext);

	return ssl->rmshtdn;
}

extern "C" __declspec(dllexport)
int SSL_read(SSL *ssl, char *buf, int num)
{
    SECURITY_STATUS scRet;
    DWORD           cbData;
	int				i;

    SecBufferDesc   Message;
    SecBuffer       Buffers[4];
    SecBuffer *     pDataBuffer;
    SecBuffer *     pExtraBuffer;

	if (num == 0) return 0;

	if (ssl->cbRecDataBuf != 0)
	{
		DWORD bytes = min((DWORD)num, ssl->cbRecDataBuf);
		CopyMemory(buf, ssl->pbRecDataBuf, bytes);

		DWORD rbytes = ssl->cbRecDataBuf - bytes;
		MoveMemory(ssl->pbRecDataBuf, ((char*)ssl->pbRecDataBuf)+bytes, rbytes);
		ssl->cbRecDataBuf = rbytes;

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
				ssl->pbIoBuffer = (PUCHAR)realloc(ssl->pbIoBuffer, ssl->sbIoBuffer);
			}

			cbData = recv(ssl->s, (char*)ssl->pbIoBuffer + ssl->cbIoBuffer, ssl->sbIoBuffer - ssl->cbIoBuffer, 0);
            if(cbData == SOCKET_ERROR)
            {
                return SOCKET_ERROR;
            }
            else if(cbData == 0)
            {
                // Server disconnected.
                if (ssl->cbIoBuffer)
                {
                    scRet = SEC_E_INTERNAL_ERROR;
                    return SOCKET_ERROR;
                }
                else
                    return 0;
            }
            else
                ssl->cbIoBuffer += cbData;
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

        if(scRet == SEC_E_INCOMPLETE_MESSAGE)
        {
            // The input buffer contains only a fragment of an
            // encrypted record. Loop around and read some more
            // data.
            continue;
        }

        // Server signaled end of session
        if(scRet == SEC_I_CONTEXT_EXPIRED)
		{
			ssl->rmshtdn = TRUE;
			SSL_shutdown(ssl);
            return 0;
		}

        if( scRet != SEC_E_OK && 
            scRet != SEC_I_RENEGOTIATE && 
            scRet != SEC_I_CONTEXT_EXPIRED)
        {
            return SOCKET_ERROR;
        }

        // Locate data and (optional) extra buffers.
        pDataBuffer  = NULL;
        pExtraBuffer = NULL;
        for(i = 1; i < 4; i++)
        {
            if(pDataBuffer == NULL && Buffers[i].BufferType == SECBUFFER_DATA)
            {
                pDataBuffer = &Buffers[i];
            }

            if(pExtraBuffer == NULL && Buffers[i].BufferType == SECBUFFER_EXTRA)
            {
                pExtraBuffer = &Buffers[i];
            }
        }

        // Return decrypted data.
		DWORD bytes;
		if (pDataBuffer)
        {
			bytes = min((DWORD)num, pDataBuffer->cbBuffer);
			CopyMemory(buf, pDataBuffer->pvBuffer, bytes);

			DWORD rbytes = pDataBuffer->cbBuffer - bytes;
			if (rbytes > 0)
			{
				if (ssl->sbRecDataBuf < rbytes) 
				{
					ssl->sbRecDataBuf = rbytes;
					ssl->pbRecDataBuf = (PUCHAR)realloc(ssl->pbRecDataBuf, ssl->sbRecDataBuf);
				}
				CopyMemory(ssl->pbRecDataBuf, (char*)pDataBuffer->pvBuffer+bytes, rbytes);
				ssl->cbRecDataBuf = rbytes;
			}
        }

        // Move any "extra" data to the input buffer.
        if (pExtraBuffer)
        {
            MoveMemory(ssl->pbIoBuffer, pExtraBuffer->pvBuffer, pExtraBuffer->cbBuffer);
            ssl->cbIoBuffer = pExtraBuffer->cbBuffer;
        }
        else
            ssl->cbIoBuffer = 0;

		if (pDataBuffer && bytes) return bytes;

		if(scRet == SEC_I_RENEGOTIATE)
        {
            // The server wants to perform another handshake
            // sequence.

            scRet = ClientHandshakeLoop(ssl, FALSE);
            if(scRet != SEC_E_OK) return SOCKET_ERROR;
        }
    }
}


extern "C" __declspec(dllexport)
int SSL_write(SSL *ssl, const char *buf, int num)
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

	scRet = g_pSSPI->QueryContextAttributesA(&ssl->hContext, SECPKG_ATTR_STREAM_SIZES, &Sizes);
    if (scRet != SEC_E_OK) return scRet;

	pbDataBuffer = (PUCHAR)calloc(1, Sizes.cbMaximumMessage + Sizes.cbHeader + Sizes.cbTrailer);

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
		if(cbData == SOCKET_ERROR || cbData == 0)
		{
			g_pSSPI->DeleteSecurityContext(&ssl->hContext);
			scRet = SEC_E_INTERNAL_ERROR;
			break;
		}

		sendOff += cbMessage;
	}

	free(pbDataBuffer);
	return scRet == SEC_E_OK ? num : SOCKET_ERROR;
}


extern "C" __declspec(dllexport)
int SSL_pending(SSL *ssl)
{
	return ssl->cbRecDataBuf;
}

extern "C" __declspec(dllexport)
SSL_METHOD *SSLv23_client_method(void)
{
	return NULL;
}

extern "C" __declspec(dllexport)
SSL_METHOD *SSLv2_client_method(void)
{
	return (SSL_METHOD*)SP_PROT_SSL2;
}

extern "C" __declspec(dllexport)
SSL_METHOD *SSLv3_client_method(void)
{
	return (SSL_METHOD*)SP_PROT_SSL3;
}

extern "C" __declspec(dllexport)
SSL_METHOD *TLSv1_client_method(void)
{
	return (SSL_METHOD*)SP_PROT_TLS1;
}

extern "C" __declspec(dllexport)
void SSL_CTX_set_verify(SSL_CTX *ctx, int mode, void* func)
{
	ctx->bVerify = mode != 0;
}