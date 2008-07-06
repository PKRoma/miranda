/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-08  George Hazan

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
#include "jabber_ssl.h"

PFN_SSL_int_void             pfn_SSL_library_init;      // int SSL_library_init()
PFN_SSL_pvoid_void           pfn_SSLv23_client_method;  // SSL_METHOD *SSLv23_client_method()
PFN_SSL_pvoid_void           pfn_TLSv1_client_method;
PFN_SSL_pvoid_pvoid          pfn_SSL_CTX_new;           // SSL_CTX *SSL_CTX_new( SSL_METHOD *method )
PFN_SSL_void_pvoid           pfn_SSL_CTX_free;          // void SSL_CTX_free( SSL_CTX *ctx );
PFN_SSL_pvoid_pvoid          pfn_SSL_new;               // SSL *SSL_new( SSL_CTX *ctx )
PFN_SSL_void_pvoid           pfn_SSL_free;              // void SSL_free( SSL *ssl );
PFN_SSL_int_pvoid_int        pfn_SSL_set_fd;            // int SSL_set_fd( SSL *ssl, int fd );
PFN_SSL_int_pvoid            pfn_SSL_connect;           // int SSL_connect( SSL *ssl );
PFN_SSL_int_pvoid_pvoid_int  pfn_SSL_read;              // int SSL_read( SSL *ssl, void *buffer, int bufsize )
PFN_SSL_int_pvoid_pvoid_int  pfn_SSL_write;             // int SSL_write( SSL *ssl, void *buffer, int bufsize )
PFN_SSL_int_pvoid_int_pvoid  pfn_SSL_CTX_set_verify;

static BOOL jabberSslInit()
{
	if ( hLibSSL )
		return TRUE;

	BOOL error = FALSE;

	hLibSSL = LoadLibraryA( "WINSSL.DLL" );
	if ( !hLibSSL )
		hLibSSL = LoadLibraryA( "CYASSL.DLL" );
	if ( !hLibSSL )
		hLibSSL = LoadLibraryA( "SSLEAY32.DLL" );
	if ( !hLibSSL )
		hLibSSL = LoadLibraryA( "LIBSSL32.DLL" );

	if ( hLibSSL ) {
		if (( pfn_SSL_library_init=( PFN_SSL_int_void )GetProcAddress( hLibSSL, "SSL_library_init" )) == NULL )
			error = TRUE;
		if (( pfn_SSLv23_client_method=( PFN_SSL_pvoid_void )GetProcAddress( hLibSSL, "SSLv23_client_method" )) == NULL )
			error = TRUE;
		if (( pfn_TLSv1_client_method = ( PFN_SSL_pvoid_void )GetProcAddress( hLibSSL, "TLSv1_client_method" )) == NULL )
			error = TRUE;
		if (( pfn_SSL_CTX_new=( PFN_SSL_pvoid_pvoid )GetProcAddress( hLibSSL, "SSL_CTX_new" )) == NULL )
			error = TRUE;
		if (( pfn_SSL_CTX_free=( PFN_SSL_void_pvoid )GetProcAddress( hLibSSL, "SSL_CTX_free" )) == NULL )
			error = TRUE;
		if (( pfn_SSL_new=( PFN_SSL_pvoid_pvoid )GetProcAddress( hLibSSL, "SSL_new" )) == NULL )
			error = TRUE;
		if (( pfn_SSL_free=( PFN_SSL_void_pvoid )GetProcAddress( hLibSSL, "SSL_free" )) == NULL )
			error = TRUE;
		if (( pfn_SSL_set_fd=( PFN_SSL_int_pvoid_int )GetProcAddress( hLibSSL, "SSL_set_fd" )) == NULL )
			error = TRUE;
		if (( pfn_SSL_connect=( PFN_SSL_int_pvoid )GetProcAddress( hLibSSL, "SSL_connect" )) == NULL )
			error = TRUE;
		if (( pfn_SSL_read=( PFN_SSL_int_pvoid_pvoid_int )GetProcAddress( hLibSSL, "SSL_read" )) == NULL )
			error = TRUE;
		if (( pfn_SSL_write=( PFN_SSL_int_pvoid_pvoid_int )GetProcAddress( hLibSSL, "SSL_write" )) == NULL )
			error = TRUE;
		if (( pfn_SSL_CTX_set_verify = ( PFN_SSL_int_pvoid_int_pvoid )GetProcAddress( hLibSSL, "SSL_CTX_set_verify" )) == NULL )
			error = TRUE;

		if ( error == TRUE ) {
			FreeLibrary( hLibSSL );
			hLibSSL = NULL;
	}	}

	if ( !hLibSSL )
		return FALSE;
		
	pfn_SSL_library_init();
	return TRUE;
}

BOOL CJabberProto::SslInit()
{
	if ( !jabberSslInit() ) {
		Log( "SSL library cannot load" );
		JSetByte( "UseTLS", 0 );
		JSetByte( "UseSSL", 0 );
		return FALSE;
	}

	Log( "SSL library load successful" );
	if ( m_sslCtx == NULL )
	{
		m_sslCtx = pfn_SSL_CTX_new( JGetByte( "UseSSL", 0 ) ? pfn_SSLv23_client_method() : pfn_TLSv1_client_method());
		pfn_SSL_CTX_set_verify(m_sslCtx, 0, NULL);
	}

	return TRUE;
}
