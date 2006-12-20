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

typedef int (*PFN_SSL_int_void) (void);
typedef void * (*PFN_SSL_pvoid_void) (void);
typedef void * (*PFN_SSL_pvoid_pvoid) (void *);
typedef void * (*PFN_SSL_pvoid_pvoid_pvoid_int) (void *, void *, int);
typedef void (*PFN_SSL_void_pvoid) (void *);
typedef void (*PFN_SSL_void_pvoid_int) (void *, int);
typedef void (*PFN_SSL_void_pvoid_int_pvoid) (void *, int, void *);
typedef void (*PFN_SSL_ulong_pvoid_size_t) (unsigned long, void *, size_t);
typedef int (*PFN_SSL_int_pvoid_int) (void *, int);
typedef int (*PFN_SSL_int_pvoid) (void *);
typedef int (*PFN_SSL_int_pvoid_pvoid_int) (void *, void *, int);
typedef unsigned long (*PFN_SSL_ulong_void) (void);

typedef void SSL;
typedef void SSL_CTX;
typedef void X509;

extern PFN_SSL_int_void				SSL_library_init;		// int SSL_library_init()
extern PFN_SSL_pvoid_void			TLSv1_client_method;	// SSL_METHOD *TLSv1_client_method()
extern PFN_SSL_pvoid_pvoid			SSL_CTX_new;			// SSL_CTX *SSL_CTX_new(SSL_METHOD *method)
extern PFN_SSL_void_pvoid			SSL_CTX_free;			// void SSL_CTX_free(SSL_CTX *ctx);
extern PFN_SSL_pvoid_pvoid			SSL_new;				// SSL *SSL_new(SSL_CTX *ctx)
extern PFN_SSL_void_pvoid			SSL_free;				// void SSL_free(SSL *ssl);
extern PFN_SSL_int_pvoid_int		SSL_set_fd;				// int SSL_set_fd(SSL *ssl, int fd);
extern PFN_SSL_int_pvoid			SSL_connect;			// int SSL_connect(SSL *ssl);
extern PFN_SSL_int_pvoid_pvoid_int	SSL_read;				// int SSL_read(SSL *ssl, void *buffer, int bufsize)
extern PFN_SSL_int_pvoid_pvoid_int	SSL_write;				// int SSL_write(SSL *ssl, void *buffer, int bufsize)
extern PFN_SSL_int_pvoid_int		SSL_get_error;			// int SSL_get_error(SSL *s,int ret_code)
extern PFN_SSL_void_pvoid_int_pvoid SSL_CTX_set_verify; 	// void SSL_CTX_set_verify(SSL_CTX *ctx,int mode, int (*callback)(int, X509_STORE_CTX *))
extern PFN_SSL_int_pvoid			SSL_shutdown;			// int SSL_shutdown(SSL *s)
extern PFN_SSL_pvoid_pvoid			SSL_get_current_cipher; // SSL_CIPHER *SSL_get_current_cipher(SSL *s)
extern PFN_SSL_pvoid_pvoid			SSL_CIPHER_get_name;	// const char *	SSL_CIPHER_get_name(SSL_CIPHER *c)
extern PFN_SSL_pvoid_pvoid			SSL_get_peer_certificate; // X509 * SSL_get_peer_certificate(SSL *s)

extern PFN_SSL_pvoid_pvoid			X509_get_subject_name;	// X509_NAME * X509_get_subject_name(X509 *a)
extern PFN_SSL_pvoid_pvoid			X509_get_issuer_name;	// X509_NAME * X509_get_issuer_name(X509 *a)
extern PFN_SSL_pvoid_pvoid_pvoid_int X509_NAME_oneline;		// char * X509_NAME_oneline(X509_NAME *a,char *buf,int size);

extern PFN_SSL_int_void 			RAND_status;			// int RAND_status(void)
extern PFN_SSL_void_pvoid_int		RAND_seed;				// void RAND_seed(const void *buf,int num)

extern PFN_SSL_ulong_void			ERR_get_error;			// unsigned long ERR_get_error(void)
extern PFN_SSL_ulong_pvoid_size_t	ERR_error_string_n;		// void ERR_error_string_n(unsigned long e, char *buf, size_t len)

extern int gg_ssl_init();
extern void gg_ssl_uninit();

#define OpenSSL_add_ssl_algorithms()	SSL_library_init()
#define SSL_get_cipher_name(s) \
		SSL_CIPHER_get_name(SSL_get_current_cipher(s))


#define SSL_ERROR_NONE			0
#define SSL_ERROR_SSL			1
#define SSL_ERROR_WANT_READ		2
#define SSL_ERROR_WANT_WRITE		3
#define SSL_ERROR_WANT_X509_LOOKUP	4
#define SSL_ERROR_SYSCALL		5 /* look at error stack/return value/errno */
#define SSL_ERROR_ZERO_RETURN		6
#define SSL_ERROR_WANT_CONNECT		7
#define SSL_ERROR_WANT_ACCEPT		8
#define SSL_VERIFY_NONE			0x00
#define SSL_VERIFY_PEER			0x01
#define SSL_VERIFY_FAIL_IF_NO_PEER_CERT	0x02
#define SSL_VERIFY_CLIENT_ONCE		0x04

