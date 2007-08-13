////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2003-2006 Adam Strzelecki <ono+miranda@java.pl>
//
// Portions taken from:
//	 Jabber Protocol Plugin for Miranda IM
//	 Tlen Protocol Plugin for Miranda IM
//	 Copyright (C) 2002-2003  Santithorn Bunchua
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

#include "gg.h"

#ifdef GG_CONFIG_HAVE_OPENSSL
HANDLE hLibSSL;								// SSL main library handle
HANDLE hLibEAY;								// SSL/EAY misc library handle

PFN_SSL_int_void			SSL_library_init;		// int SSL_library_init()
PFN_SSL_pvoid_void			TLSv1_client_method;	// SSL_METHOD *TLSv1_client_method()
PFN_SSL_pvoid_pvoid			SSL_CTX_new;			// SSL_CTX *SSL_CTX_new(SSL_METHOD *method)
PFN_SSL_void_pvoid			SSL_CTX_free;			// void SSL_CTX_free(SSL_CTX *ctx);
PFN_SSL_pvoid_pvoid			SSL_new;				// SSL *SSL_new(SSL_CTX *ctx)
PFN_SSL_void_pvoid			SSL_free;				// void SSL_free(SSL *ssl);
PFN_SSL_int_pvoid_int		SSL_set_fd;				// int SSL_set_fd(SSL *ssl, int fd);
PFN_SSL_int_pvoid			SSL_connect;			// int SSL_connect(SSL *ssl);
PFN_SSL_int_pvoid_pvoid_int	SSL_read;				// int SSL_read(SSL *ssl, void *buffer, int bufsize)
PFN_SSL_int_pvoid_pvoid_int	SSL_write;				// int SSL_write(SSL *ssl, void *buffer, int bufsize)
PFN_SSL_int_pvoid_int		SSL_get_error;			// int SSL_get_error(SSL *s,int ret_code)
PFN_SSL_void_pvoid_int_pvoid SSL_CTX_set_verify;	// void SSL_CTX_set_verify(SSL_CTX *ctx,int mode, int (*callback)(int, X509_STORE_CTX *))
PFN_SSL_int_pvoid			SSL_shutdown;			// int SSL_shutdown(SSL *s)
PFN_SSL_pvoid_pvoid			SSL_get_current_cipher; // SSL_CIPHER *SSL_get_current_cipher(SSL *s)
PFN_SSL_pvoid_pvoid			SSL_CIPHER_get_name;	// const char *	SSL_CIPHER_get_name(SSL_CIPHER *c)
PFN_SSL_pvoid_pvoid			SSL_get_peer_certificate; // X509 * SSL_get_peer_certificate(SSL *s)

PFN_SSL_pvoid_pvoid			X509_get_subject_name;	// X509_NAME * X509_get_subject_name(X509 *a)
PFN_SSL_pvoid_pvoid			X509_get_issuer_name;	// X509_NAME * X509_get_issuer_name(X509 *a)
PFN_SSL_pvoid_pvoid_pvoid_int X509_NAME_oneline;	// char * X509_NAME_oneline(X509_NAME *a,char *buf,int size);

PFN_SSL_int_void			RAND_status;			// int RAND_status(void)
PFN_SSL_void_pvoid_int		RAND_seed;				// void RAND_seed(const void *buf,int num)

PFN_SSL_ulong_void			ERR_get_error;			// unsigned long ERR_get_error(void)
PFN_SSL_ulong_pvoid_size_t	ERR_error_string_n;		// void ERR_error_string_n(unsigned long e, char *buf, size_t len)


static CRITICAL_SECTION sslHandleMutex;

//////////////////////////////////////////////////////////
// init SSL library
BOOL gg_ssl_init()
{
	BOOL error = FALSE;
	char *failFunction;

	InitializeCriticalSection(&sslHandleMutex);
	hLibSSL = LoadLibrary("SSLEAY32.DLL");
	hLibEAY = LoadLibrary("LIBEAY32.DLL");

	// Load main functions
	if (hLibSSL)
	{
		if (error || (SSL_library_init=(PFN_SSL_int_void)GetProcAddress(hLibSSL, failFunction = "SSL_library_init")) == NULL)
			error = TRUE;
		if (error || (TLSv1_client_method=(PFN_SSL_pvoid_void)GetProcAddress(hLibSSL, failFunction = "TLSv1_client_method")) == NULL)
			error = TRUE;
		if (error || (SSL_CTX_new=(PFN_SSL_pvoid_pvoid)GetProcAddress(hLibSSL, failFunction = "SSL_CTX_new")) == NULL)
			error = TRUE;
		if (error || (SSL_CTX_free=(PFN_SSL_void_pvoid)GetProcAddress(hLibSSL, failFunction = "SSL_CTX_free")) == NULL)
			error = TRUE;
		if (error || (SSL_new=(PFN_SSL_pvoid_pvoid)GetProcAddress(hLibSSL, failFunction = "SSL_new")) == NULL)
			error = TRUE;
		if (error || (SSL_free=(PFN_SSL_void_pvoid)GetProcAddress(hLibSSL, failFunction = "SSL_free")) == NULL)
			error = TRUE;
		if (error || (SSL_set_fd=(PFN_SSL_int_pvoid_int)GetProcAddress(hLibSSL, failFunction = "SSL_set_fd")) == NULL)
			error = TRUE;
		if (error || (SSL_connect=(PFN_SSL_int_pvoid)GetProcAddress(hLibSSL, failFunction = "SSL_connect")) == NULL)
			error = TRUE;
		if (error || (SSL_read=(PFN_SSL_int_pvoid_pvoid_int)GetProcAddress(hLibSSL, failFunction = "SSL_read")) == NULL)
			error = TRUE;
		if (error || (SSL_write=(PFN_SSL_int_pvoid_pvoid_int)GetProcAddress(hLibSSL, failFunction = "SSL_write")) == NULL)
			error = TRUE;
		if (error || (SSL_get_error=(PFN_SSL_int_pvoid_int)GetProcAddress(hLibSSL, failFunction = "SSL_get_error")) == NULL)
			error = TRUE;
		if (error || (SSL_CTX_set_verify=(PFN_SSL_void_pvoid_int_pvoid)GetProcAddress(hLibSSL, failFunction = "SSL_CTX_set_verify")) == NULL)
			error = TRUE;
		if (error || (SSL_shutdown=(PFN_SSL_int_pvoid)GetProcAddress(hLibSSL, failFunction = "SSL_shutdown")) == NULL)
			error = TRUE;
		if (error || (SSL_get_current_cipher=(PFN_SSL_pvoid_pvoid)GetProcAddress(hLibSSL, failFunction = "SSL_get_current_cipher")) == NULL)
			error = TRUE;
		if (error || (SSL_CIPHER_get_name=(PFN_SSL_pvoid_pvoid)GetProcAddress(hLibSSL, failFunction = "SSL_CIPHER_get_name")) == NULL)
			error = TRUE;
		if (error || (SSL_get_peer_certificate=(PFN_SSL_pvoid_pvoid)GetProcAddress(hLibSSL, failFunction = "SSL_get_peer_certificate")) == NULL)
			error = TRUE;

		if (error == TRUE)
		{
			FreeLibrary(hLibSSL);
			hLibSSL = NULL;
#ifdef DEBUGMODE
			gg_netlog("gg_ssl_init(): Failed on loading function \"%s\".", failFunction);
#endif
		}
	}
#ifdef DEBUGMODE
	else
		gg_netlog("gg_ssl_init(): Failed on loading library SSLEAY32.DLL.");
#endif

	// Load misc functions
	if (hLibEAY && hLibSSL)
	{
		if (error || (X509_get_subject_name=(PFN_SSL_pvoid_pvoid)GetProcAddress(hLibEAY, failFunction = "X509_get_subject_name")) == NULL)
			error = TRUE;
		if (error || (X509_get_issuer_name=(PFN_SSL_pvoid_pvoid)GetProcAddress(hLibEAY, failFunction = "X509_get_issuer_name")) == NULL)
			error = TRUE;
		if (error || (X509_NAME_oneline=(PFN_SSL_pvoid_pvoid_pvoid_int)GetProcAddress(hLibEAY, failFunction = "X509_NAME_oneline")) == NULL)
			error = TRUE;

		if (error || (RAND_status=(PFN_SSL_int_void)GetProcAddress(hLibEAY, failFunction = "RAND_status")) == NULL)
			error = TRUE;
		if (error || (RAND_seed=(PFN_SSL_void_pvoid_int)GetProcAddress(hLibEAY, failFunction = "RAND_seed")) == NULL)
			error = TRUE;

		if (error || (ERR_get_error=(PFN_SSL_ulong_void)GetProcAddress(hLibEAY, failFunction = "ERR_get_error")) == NULL)
			error = TRUE;
		if (error || (ERR_error_string_n=(PFN_SSL_ulong_pvoid_size_t)GetProcAddress(hLibEAY, failFunction = "ERR_error_string_n")) == NULL)
			error = TRUE;

		if (error == TRUE)
		{
			FreeLibrary(hLibEAY);
			hLibEAY = NULL;
#ifdef DEBUGMODE
			gg_netlog("gg_ssl_init(): Failed on loading function \"%s\".", failFunction);
#endif
		}
	}
#ifdef DEBUGMODE
	else
		gg_netlog("gg_ssl_init(): Failed on loading library LIBEAY32.DLL.");
#endif

	// Unload main library if misc not available
	if (hLibSSL && !hLibEAY)
	{
		FreeLibrary(hLibSSL);
		hLibSSL = NULL;
	}


#ifdef DEBUGMODE
	if (hLibSSL)
		gg_netlog("gg_ssl_init(): SSL library load was successful.");
	else
		gg_netlog("gg_ssl_init(): Cannot load SSL library.");
#endif

	return (hLibSSL != NULL);
}

//////////////////////////////////////////////////////////
// uninit SSL library
void gg_ssl_uninit()
{
	if (hLibSSL) {
		FreeLibrary(hLibSSL);
		hLibSSL = NULL;
		FreeLibrary(hLibEAY);
		hLibEAY = NULL;
	}

	DeleteCriticalSection(&sslHandleMutex);
}
#endif
