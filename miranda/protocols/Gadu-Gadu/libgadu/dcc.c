/* $Id$ */

/*
 *  (C) Copyright 2001-2006 Wojtek Kaniewski <wojtekka@irc.pl>
 *                          Tomasz Chiliñski <chilek@chilan.com>
 *                          Adam Wysocki <gophi@ekg.chmurka.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License Version
 *  2.1 as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307,
 *  USA.
 */

#include "libgadu-config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef sun
#  include <sys/filio.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "compat.h"
#include "libgadu.h"
#ifdef GG_CONFIG_MIRANDA
#undef small
#endif

#ifndef GG_DEBUG_DISABLE
/*
 * gg_dcc_debug_data() // funkcja wewnêtrzna
 *
 * wy¶wietla zrzut pakietu w hexie.
 * 
 *  - prefix - prefiks zrzutu pakietu
 *  - fd - deskryptor gniazda
 *  - buf - bufor z danymi
 *  - size - rozmiar danych
 */
static void gg_dcc_debug_data(const char *prefix, int fd, const void *buf, unsigned int size)
{
	unsigned int i;
	
	gg_debug(GG_DEBUG_MISC, "++ gg_dcc %s (fd=%d,len=%d)", prefix, fd, size);
	
	for (i = 0; i < size; i++)
		gg_debug(GG_DEBUG_MISC, " %.2x", ((unsigned char*) buf)[i]);
	
	gg_debug(GG_DEBUG_MISC, "\n");
}
#else
#define gg_dcc_debug_data(a,b,c,d) do { } while (0)
#endif

/*
 * gg_dcc_request()
 *
 * wysy³a informacjê o tym, ¿e dany klient powinien siê z nami po³±czyæ.
 * wykorzystywane, kiedy druga strona, której chcemy co¶ wys³aæ jest za
 * maskarad±.
 *
 *  - sess - struktura opisuj±ca sesjê GG
 *  - uin - numerek odbiorcy
 *
 * patrz gg_send_message_ctcp().
 */
int gg_dcc_request(struct gg_session *sess, uin_t uin)
{
	return gg_send_message_ctcp(sess, GG_CLASS_CTCP, uin, (unsigned char*) "\002", 1);
}

/* 
 * gg_dcc_fill_filetime()  // funkcja wewnêtrzna
 *
 * zamienia czas w postaci unixowej na windowsowy.
 *
 *  - unix - czas w postaci unixowej
 *  - filetime - czas w postaci windowsowej
 */
static void gg_dcc_fill_filetime(uint32_t ut, uint32_t *ft)
{
#ifdef GG_CONFIG_HAVE_LONG_LONG
	unsigned long long tmp;

	tmp = ut;
	tmp += 11644473600LL;
	tmp *= 10000000LL;

#ifndef GG_CONFIG_BIGENDIAN
	ft[0] = (uint32_t) tmp;
	ft[1] = (uint32_t) (tmp >> 32);
#else
	ft[0] = gg_fix32((uint32_t) (tmp >> 32));
	ft[1] = gg_fix32((uint32_t) tmp);
#endif

#endif
}

/*
 * gg_dcc_fill_file_info()
 *
 * wype³nia pola struct gg_dcc niezbêdne do wys³ania pliku.
 *
 *  - d - struktura opisuj±ca po³±czenie DCC
 *  - filename - nazwa pliku
 *
 * 0, -1.
 */
int gg_dcc_fill_file_info(struct gg_dcc *d, const char *filename)
{
	return gg_dcc_fill_file_info2(d, filename, filename);
}

/*
 * gg_dcc_fill_file_info2()
 *
 * wype³nia pola struct gg_dcc niezbêdne do wys³ania pliku.
 *
 *  - d - struktura opisuj±ca po³±czenie DCC
 *  - filename - nazwa pliku
 *  - local_filename - nazwa na lokalnym systemie plików
 *
 * 0, -1.
 */
int gg_dcc_fill_file_info2(struct gg_dcc *d, const char *filename, const char *local_filename)
{
	return gg_dcc_fill_file_info3(d, filename, local_filename, NULL);
}

/*
 * gg_dcc_fill_file_info3()
 *
 * wyplenia pole struct gg_dcc niezbedne do wyslania pliku.
 *
 *  - d - struktura opisujaca polaczenie DCC
 *  - filename - nazwa pliku
 *  - local_filename - nazwa na lokalnym systemie plikow
 *  - hash - ewentualne miejsce gdzie polozyc hash pliku [SHA1] 20 znakow.
 *
 *  0, -1.
 */
int gg_dcc_fill_file_info3(struct gg_dcc *d, const char *filename, const char *local_filename, unsigned char *hash)
{
	struct stat st;
	const char *name, *ext, *p;
	unsigned char *q;
	int i, j;

	gg_debug(GG_DEBUG_FUNCTION, "** gg_dcc_fill_file_info3(%p, \"%s\", \"%s\" 0x%x);\n", d, filename, local_filename, hash);

	if (!d || d->type != GG_SESSION_DCC_SEND) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_fill_file_info3() invalid arguments\n");
		errno = EINVAL;
		return -1;
	}

	if (stat(local_filename, &st) == -1) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_fill_file_info3() stat() failed (%s)\n", strerror(errno));
		return -1;
	}

	if ((st.st_mode & S_IFDIR)) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_fill_file_info3() that's a directory\n");
		errno = EINVAL;
		return -1;
	}

#ifdef GG_CONFIG_MIRANDA
	if ((d->file_fd = fopen(filename, "rb")) == NULL) {
#else
	if ((d->file_fd = open(local_filename, O_RDONLY)) == -1) {
#endif
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_fill_file_info3() open() failed (%s)\n", strerror(errno));
		return -1;
	}

	memset(&d->file_info, 0, sizeof(d->file_info));

	if (!(st.st_mode & S_IWUSR))
		d->file_info.mode |= gg_fix32(GG_DCC_FILEATTR_READONLY);

	gg_dcc_fill_filetime(st.st_atime, d->file_info.atime);
	gg_dcc_fill_filetime(st.st_mtime, d->file_info.mtime);
	gg_dcc_fill_filetime(st.st_ctime, d->file_info.ctime);

	d->file_info.size = gg_fix32(st.st_size);
	d->file_info.mode = gg_fix32(0x20);	/* FILE_ATTRIBUTE_ARCHIVE */

#ifdef GG_CONFIG_MIRANDA
	if (!(name = strrchr(filename, '\\')))
#else
	if (!(name = strrchr(filename, '/')))
#endif
		name = filename;
	else
		name++;

	if (!(ext = strrchr(name, '.')))
		ext = name + strlen(name);

	for (i = 0, p = name; i < 8 && p < ext; i++, p++)
		d->file_info.short_filename[i] = toupper(name[i]);

	if (i == 8 && p < ext) {
		d->file_info.short_filename[6] = '~';
		d->file_info.short_filename[7] = '1';
	}

	if (strlen(ext) > 0) {
		for (j = 0; *ext && j < 4; j++, p++)
			d->file_info.short_filename[i + j] = toupper(ext[j]);
	}

	for (q = d->file_info.short_filename; *q; q++) {
		if (*q == 185) {
			*q = 165;
		} else if (*q == 230) {
			*q = 198;
		} else if (*q == 234) {
			*q = 202;
		} else if (*q == 179) {
			*q = 163;
		} else if (*q == 241) {
			*q = 209;
		} else if (*q == 243) {
			*q = 211;
		} else if (*q == 156) {
			*q = 140;
		} else if (*q == 159) {
			*q = 143;
		} else if (*q == 191) {
			*q = 175;
		}
	}
	
	gg_debug(GG_DEBUG_MISC, "// gg_dcc_fill_file_info3() short name \"%s\", dos name \"%s\"\n", name, d->file_info.short_filename);
	strncpy((char*) d->file_info.filename, name, sizeof(d->file_info.filename) - 1);

	/* windowsowe gadu-gadu wymaga dobrego hasha, nic nie zrobimy.. trzeba policzyc. a ze dla duzych plikow to jest wolne,
	 * 	tylko klopot w tym ze jak aplikacja jednowatkowa to wywola to mamy locka w aplikacji...
	 *
	 * XXX, moze niech aplikacje to licza za nas?
	 */
	if (hash) {
		SHA_CTX ctx;
		unsigned char buf[1024*16];	/* rozmiar bufora wygrzebany ze zrodel openssl/sha */
		int len;
		
		SHA1_Init(&ctx);
		
		while ((len = read(d->file_fd, buf, sizeof(buf))) > 0)
			SHA1_Update(&ctx, (const unsigned char*) buf, len);

		if (len == -1) 
			gg_debug(GG_DEBUG_MISC, "// gg_dcc_fill_file_info3() hash read() fail\n");

		SHA1_Final(hash, &ctx);

		if (lseek(d->file_fd, SEEK_SET, 0) == -1)
			gg_debug(GG_DEBUG_MISC, "// gg_dcc_fill_file_info3() hash lseek() fail\n");
	}

	return 0;
}

/*
 * gg_dcc_transfer() // funkcja wewnêtrzna
 * 
 * inicjuje proces wymiany pliku z danym klientem.
 *
 *  - ip - adres ip odbiorcy
 *  - port - port odbiorcy
 *  - my_uin - w³asny numer
 *  - peer_uin - numer obiorcy
 *  - type - rodzaj wymiany (GG_SESSION_DCC_SEND lub GG_SESSION_DCC_GET)
 *
 * zaalokowana struct gg_dcc lub NULL je¶li wyst±pi³ b³±d.
 */
static struct gg_dcc *gg_dcc_transfer(uint32_t ip, uint16_t port, uin_t my_uin, uin_t peer_uin, int type)
{
	struct gg_dcc *d = NULL;
	struct in_addr addr;

	addr.s_addr = ip;
	
	gg_debug(GG_DEBUG_FUNCTION, "** gg_dcc_transfer(%s, %d, %ld, %ld, %s);\n", inet_ntoa(addr), port, my_uin, peer_uin, (type == GG_SESSION_DCC_SEND) ? "SEND" : "GET");
	
	if (!ip || ip == INADDR_NONE || !port || !my_uin || !peer_uin) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_transfer() invalid arguments\n");
		errno = EINVAL;
		return NULL;
	}

	if (!(d = (void*) calloc(1, sizeof(*d)))) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_transfer() not enough memory\n");
		return NULL;
	}

	d->check = GG_CHECK_WRITE;
	d->state = GG_STATE_CONNECTING;
	d->type = type;
	d->timeout = GG_DEFAULT_TIMEOUT;
#ifdef GG_CONFIG_MIRANDA
	d->file_fd = NULL;
#else
	d->file_fd = -1;
#endif
	d->active = 1;
	d->fd = -1;
	d->uin = my_uin;
	d->peer_uin = peer_uin;

	if ((d->fd = gg_connect(&addr, port, 1)) == -1) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_transfer() connection failed\n");
		free(d);
		return NULL;
	}

	return d;
}

/*
 * gg_dcc_get_file()
 * 
 * inicjuje proces odbierania pliku od danego klienta, gdy ten wys³a³ do
 * nas ¿±danie po³±czenia.
 *
 *  - ip - adres ip odbiorcy
 *  - port - port odbiorcy
 *  - my_uin - w³asny numer
 *  - peer_uin - numer obiorcy
 *
 * zaalokowana struct gg_dcc lub NULL je¶li wyst±pi³ b³±d.
 */
struct gg_dcc *gg_dcc_get_file(uint32_t ip, uint16_t port, uin_t my_uin, uin_t peer_uin)
{
	gg_debug(GG_DEBUG_MISC, "// gg_dcc_get_file() handing over to gg_dcc_transfer()\n");

	return gg_dcc_transfer(ip, port, my_uin, peer_uin, GG_SESSION_DCC_GET);
}

/*
 * gg_dcc_send_file()
 * 
 * inicjuje proces wysy³ania pliku do danego klienta.
 *
 *  - ip - adres ip odbiorcy
 *  - port - port odbiorcy
 *  - my_uin - w³asny numer
 *  - peer_uin - numer obiorcy
 *
 * zaalokowana struct gg_dcc lub NULL je¶li wyst±pi³ b³±d.
 */
struct gg_dcc *gg_dcc_send_file(uint32_t ip, uint16_t port, uin_t my_uin, uin_t peer_uin)
{
	gg_debug(GG_DEBUG_MISC, "// gg_dcc_send_file() handing over to gg_dcc_transfer()\n");

	return gg_dcc_transfer(ip, port, my_uin, peer_uin, GG_SESSION_DCC_SEND);
}

/*
 * gg_dcc_voice_chat()
 * 
 * próbuje nawi±zaæ po³±czenie g³osowe.
 *
 *  - ip - adres ip odbiorcy
 *  - port - port odbiorcy
 *  - my_uin - w³asny numer
 *  - peer_uin - numer obiorcy
 *
 * zaalokowana struct gg_dcc lub NULL je¶li wyst±pi³ b³±d.
 */
struct gg_dcc *gg_dcc_voice_chat(uint32_t ip, uint16_t port, uin_t my_uin, uin_t peer_uin)
{
	gg_debug(GG_DEBUG_MISC, "// gg_dcc_voice_chat() handing over to gg_dcc_transfer()\n");

	return gg_dcc_transfer(ip, port, my_uin, peer_uin, GG_SESSION_DCC_VOICE);
}

/*
 * gg_dcc_set_type()
 *
 * po zdarzeniu GG_EVENT_DCC_CALLBACK nale¿y ustawiæ typ po³±czenia za
 * pomoc± tej funkcji.
 *
 *  - d - struktura opisuj±ca po³±czenie
 *  - type - typ po³±czenia (GG_SESSION_DCC_SEND lub GG_SESSION_DCC_VOICE)
 */
void gg_dcc_set_type(struct gg_dcc *d, int type)
{
	d->type = type;
	d->state = (type == GG_SESSION_DCC_SEND) ? GG_STATE_SENDING_FILE_INFO : GG_STATE_SENDING_VOICE_REQUEST;
}
	
/*
 * gg_dcc_callback() // funkcja wewnêtrzna
 *
 * wywo³ywana z struct gg_dcc->callback, odpala gg_dcc_watch_fd i umieszcza
 * rezultat w struct gg_dcc->event.
 *
 *  - d - structura opisuj±ca po³±czenie
 *
 * 0, -1.
 */
static int gg_dcc_callback(struct gg_dcc *d)
{
	struct gg_event *e = gg_dcc_watch_fd(d);

	d->event = e;

	return (e != NULL) ? 0 : -1;
}

/*
 * gg_dcc_socket_create()
 *
 * tworzy gniazdo dla bezpo¶redniej komunikacji miêdzy klientami.
 *
 *  - uin - w³asny numer
 *  - port - preferowany port, je¶li równy 0 lub -1, próbuje domy¶lnego
 *
 * zaalokowana struct gg_dcc, któr± po¼niej nale¿y zwolniæ funkcj±
 * gg_dcc_free(), albo NULL je¶li wyst±pi³ b³±d.
 */
struct gg_dcc *gg_dcc_socket_create(uin_t uin, uint16_t port)
{
	struct gg_dcc *c;
	struct sockaddr_in sin;
	int sock, bound = 0, errno2;
	
	gg_debug(GG_DEBUG_FUNCTION, "** gg_create_dcc_socket(%d, %d);\n", uin, port);
	
	if (!uin) {
		gg_debug(GG_DEBUG_MISC, "// gg_create_dcc_socket() invalid arguments\n");
		errno = EINVAL;
		return NULL;
	}

	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		gg_debug(GG_DEBUG_MISC, "// gg_create_dcc_socket() can't create socket (%s)\n", strerror(errno));
		return NULL;
	}

	if (!port)
		port = GG_DEFAULT_DCC_PORT;
	
	while (!bound) {
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = INADDR_ANY;
		sin.sin_port = htons(port);
	
		gg_debug(GG_DEBUG_MISC, "// gg_create_dcc_socket() trying port %d\n", port);
		if (!bind(sock, (struct sockaddr*) &sin, sizeof(sin)))
			bound = 1;
		else {
			if (++port == 65535) {
				gg_debug(GG_DEBUG_MISC, "// gg_create_dcc_socket() no free port found\n");
				close(sock);
				return NULL;
			}
		}
	}

	if (listen(sock, 10)) {
		gg_debug(GG_DEBUG_MISC, "// gg_create_dcc_socket() unable to listen (%s)\n", strerror(errno));
		errno2 = errno;
		close(sock);
		errno = errno2;
		return NULL;
	}
	
	gg_debug(GG_DEBUG_MISC, "// gg_create_dcc_socket() bound to port %d\n", port);

	if (!(c = malloc(sizeof(*c)))) {
		gg_debug(GG_DEBUG_MISC, "// gg_create_dcc_socket() not enough memory for struct\n");
		close(sock);
		return NULL;
	}
	memset(c, 0, sizeof(*c));

	c->port = c->id = port;
	c->fd = sock;
	c->type = GG_SESSION_DCC_SOCKET;
	c->uin = uin;
	c->timeout = -1;
	c->state = GG_STATE_LISTENING;
	c->check = GG_CHECK_READ;
	c->callback = gg_dcc_callback;
	c->destroy = gg_dcc_free;
	
	return c;
}

/*
 * gg_dcc7_send_file()
 *
 * Wysyla pakiet z requestem unikatowego id dcc do serwerka
 *
 * wyczekujcie na GG_DCC7_RECEIVED_ID!
 */

int gg_dcc7_request_id(struct gg_session *sess) {
	struct gg_dcc7_request_id s;

	if (!sess) {
		errno = EFAULT;
		return -1;
	}
	
	if (sess->state != GG_STATE_CONNECTED) {
		errno = ENOTCONN;
		return -1;
	}

	s.type = gg_fix32(GG_DCC7_MAGIC1);
	return gg_send_packet(sess, GG_DCC7_REQUEST_ID, &s, sizeof(s), NULL);
}

/*
 * gg_dcc7_send_file()
 *
 * Wysyla pakiet z requestem unikatowego id do serwerka, jesli sie udalo to inicjuje gg_dcc.
 *
 */

struct gg_dcc *gg_dcc7_send_file(struct gg_session *sess, uin_t peer_uin, const char *id, const char *filename) {
	struct gg_dcc *d;
	struct gg_dcc7_new s;
	int __MAX_LEN;

	if (!sess || !id || !filename) {
		errno = EFAULT;
		return NULL;
	}
	if (sess->state != GG_STATE_CONNECTED) {
		errno = ENOTCONN;
		return NULL;
	}

	if (!(d = (void*) calloc(1, sizeof(*d)))) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc7_get_file() not enough memory\n");
		return NULL;
	}
	memset(&s, 0, sizeof(struct gg_dcc7_new));

	d->uin		= id[0] << 24 | id[1] << 16 | id[2] << 8 | id[3];	/* XXX LE only */
	d->peer_uin	= id[4] << 24 | id[5] << 16 | id[6] << 8 | id[7];	/* XXX LE only */

	d->fd		= -1;
//	d->uin		= sess->uin;
//	d->peer_uin	= peer_uin;
	d->offset	= 0; 
	d->type		= GG_SESSION_DCC_SEND;
		/* XXX */
	d->check = GG_CHECK_WRITE;
	d->state = GG_STATE_CONNECTING;
	d->timeout = GG_DEFAULT_TIMEOUT;
	d->file_fd = -1;
	d->active = 2;


	if (gg_dcc_fill_file_info3(d, filename, filename, s.hash) == -1) {
		free(d);
		return NULL;
	}

	__MAX_LEN = (sizeof(d->file_info.filename) > (sizeof(s.filename)) ? sizeof(s.filename) : sizeof(d->file_info.filename));

	memcpy(s.code1, id, sizeof(s.code1));
	s.uinfrom	= gg_fix32(sess->uin);
	s.uinto		= gg_fix32(peer_uin);
	s.dunno1	= gg_fix32(GG_DCC7_MAGIC1);
	memcpy(s.filename, d->file_info.filename, __MAX_LEN);
	s.emp1		= 0x0000;
	s.dunno2	= gg_fix32(0x0010);
	s.emp2		= 0x0000;

	s.dunno31	= 0x0000;	/* XXX */
	s.dunno32	= 0x00;		/* XXX */
	s.dunno33	= 0x02;		/* XXX */
	s.emp3		= 0x00000000;
	s.dunno4	= 0x00325eb4;
	s.dunno5	= 0x004cd08e;
	s.dunno6	= 0x00000010;
	s.dunno7	= 0x0000;	/* XXX */
	s.dunno8	= 0x00;		/* XXX */

	/* s.hash zainicjowany przez gg_dcc_fill_file_info3() */
	s.size		= d->file_info.size;
	s.emp4		= 0x00000000;
	
	d->chunk_size	= d->file_info.size;		/* bez kawalkow */

	if (gg_send_packet(sess, GG_DCC7_NEW, &s, sizeof(s), NULL) == 0)
		return d;

	free(d);
	return NULL;
}

int gg_dcc7_send_info(struct gg_session *sess, uin_t peer_uin, const char *id, const char *ipport) {
	struct gg_dcc7_info s;

	memset(&s, 0, sizeof(s));

	s.uin	= gg_fix32(peer_uin);
	s.dunno1= 0x00000001;		/* p2p -> 0x01 via-server: 0x02 */
	
	memcpy(s.code1, id, sizeof(s.code1));

	strncpy(s.ipport, ipport, sizeof(s.ipport));

/* unk */
	return gg_send_packet(sess, GG_DCC7_INFO, &s, sizeof(s), NULL);
}

/* where w postaci ip<spacja>port
 * lub to nowe via serwer XXX 
 */
int gg_dcc7_connect(struct gg_dcc *dcc, const char *where) {
	uint32_t ip = INADDR_NONE;
	uint16_t port = 0;

	int i;

	gg_debug(GG_DEBUG_FUNCTION, "** gg_dcc7_connect(%p, %s);\n", dcc, where);

	if (!dcc || !where) {
		errno = EFAULT;
		return -1;
	}

	for (i = 0; i < where[i]; i++) {
		if (where[i] == ' ') {
			ip = inet_addr(where);
			port = atoi(&where[i+1]);
			break;
		}
	}

	if (ip == INADDR_NONE || !port) {
		errno = EFAULT;
		return -1;
	}

	dcc->check = GG_CHECK_WRITE;
	dcc->state = GG_STATE_CONNECTING;
	dcc->timeout = GG_DEFAULT_TIMEOUT;
	/* d->active = 2 */

	if ((dcc->fd = gg_connect(&ip, port, 1)) == -1) {
		int saved_errno = errno;		/* errno set by gg_connect() ? */
		gg_debug(GG_DEBUG_MISC, "// gg_dcc7_connect() connection failed\n");
		errno = saved_errno;
		return -1;
	}

	return 0;
}

/*
 * gg_dcc7_fix()
 *
 * Funkcja naprawiajaca dcc, dla 7.0.. zeby nie mieszac bardzo z api.
 * Uzywana na socketach ktore byly accept()'niete ale uzywaja nowego protokolu..
 * Gdy aplikacja w handlerze GG_EVENT_DCC_CLIENT_ACCEPT, bedzie prawie pewna lub pewna ze uzywa 
 * klient nowego dcc, powinna wywolac ta funkcje.
 * [Np. przez sprawdzenie ze uin,peer_uin zlaczone razem to tak naprawde jakis unikalny kod polaczenia.
 * Lub protokol deklarowany przez uzytkownika ma wersje ktora umie tylko nowy protkol, itd, itp hth]
 *
 * Funkcja przyjmuje dwa parametry. 
 * 	Pierwszy to jest dcc z jakiego przyszedl ten event.
 * 	Drugi to jaki myslimy ze to jest dcc, czyli to co mamy po gg_dcc7_get_file() lub gg_dcc7_send_file()
 *
 * Zwraca podany drugi parametr ze skopiowanymi danymi o fd, aktualnym stanie polaczenia, itd [z pierwszego]
 * albo NULL gdy blad (zle parametry)
 *
 * UWAGA: event_dcc jest pozniej zwalniane!
 */

struct gg_dcc *gg_dcc7_fix(struct gg_dcc *event_dcc, struct gg_dcc *dest_dcc) {
	gg_debug(GG_DEBUG_MISC, "// gg_dcc7_fix() fixing!! [%p,%p] (%d,%d fd:%d)\n", event_dcc, dest_dcc,
		event_dcc->active, event_dcc->state, event_dcc->fd);

/* XXX, tutaj sprawdzic czy dest_dcc->fd == -1, dest_dcc->active == 2? */
	if (!event_dcc || !dest_dcc)
		return NULL;

	dest_dcc->fd	= event_dcc->fd;
	dest_dcc->check = event_dcc->check;
	dest_dcc->state = event_dcc->state;
	dest_dcc->error	= event_dcc->error;
	dest_dcc->type	= event_dcc->type;
	dest_dcc->id	= event_dcc->id;
	dest_dcc->timeout = event_dcc->timeout;
	dest_dcc->callback = event_dcc->callback;
	dest_dcc->destroy = event_dcc->destroy;
	dest_dcc->event = event_dcc->event;
	dest_dcc->active = 3;
	dest_dcc->port = event_dcc->port;
	dest_dcc->uin = event_dcc->uin;
	dest_dcc->peer_uin = event_dcc->peer_uin;
	dest_dcc->established = event_dcc->established;
	dest_dcc->incoming = event_dcc->incoming;
	dest_dcc->remote_addr = event_dcc->remote_addr;
	dest_dcc->remote_port = event_dcc->remote_port;

	if (dest_dcc->state == GG_STATE_SENDING_ACK) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc7_fix() changed GG_STATE_SENDING_ACK to GG_STATE_SENDING_ACK7!\n");
		dest_dcc->state = GG_STATE_SENDING_ACK7;
	}

/* XXX, zwolnij event_dcc */
	
	return dest_dcc;
}

/*
 * gg_dcc7_get_file()
 *
 * Inicjuje strukturke opisujaca gg_dcc, oraz wysyla pakiet 0x21 z potwierdzeniem checi odbioru pliku 
 *
 * offset - od ktorego miejsca pliku chcemy dostawac dane.
 *
 * zwraca NULL jesli cos bylo zle, w przeciwnym wypadku zainicjowana strukurke gg_dcc
 */

struct gg_dcc *gg_dcc7_get_file(struct gg_session *sess, uin_t peer_uin, const char *id, uint32_t filesize, uint32_t offset) {
	struct gg_dcc7_accept s;
	gg_debug_session(sess, GG_DEBUG_FUNCTION, "** gg_dcc7_get_file(%p, %d, ?, %d, %d);\n", sess, peer_uin, filesize, offset);

	if (!sess || !id) {
		errno = EFAULT;
		return NULL;
	}

	if (sess->state != GG_STATE_CONNECTED) {
		errno = ENOTCONN;
		return NULL;
	}

	memset(&s, 0, sizeof(struct gg_dcc7_accept));

	s.uin = gg_fix32(peer_uin);
	memcpy(s.code1, id, 8);			/* XXX uint64_t? */
	s.offset = gg_fix32(offset);
	s.empty = 0x00000000;

	/* jesli gg_send_packet() sie powiedzie, inicjujemy strukturke */
	if (gg_send_packet(sess, GG_DCC7_ACCEPT, &s, sizeof(s), NULL) == 0) {
		struct gg_dcc *d;

		if (!(d = (void*) calloc(1, sizeof(*d)))) {
			gg_debug(GG_DEBUG_MISC, "// gg_dcc7_get_file() not enough memory\n");
			return NULL;
		}

		d->fd		= -1;
		d->uin		= sess->uin;
		d->peer_uin	= peer_uin;
		d->chunk_size	= filesize;		/* bez kawalkow */
		d->file_info.size= filesize;
		d->offset 	= offset; 
		d->type		= GG_SESSION_DCC_GET;

		d->uin		= id[0] << 24 | id[1] << 16 | id[2] << 8 | id[3];
		d->peer_uin	= id[4] << 24 | id[5] << 16 | id[6] << 8 | id[7];
		/* XXX */

		d->check = GG_CHECK_WRITE;
		d->state = GG_STATE_CONNECTING;
		d->timeout = GG_DEFAULT_TIMEOUT;
		d->file_fd = -1;
		d->active = 2;

		return d;
	}

	return NULL;
}

/*
 * gg_dcc7_reject_file()
 *
 * Wysyla pakiecik 0x22 z odmowa przyjecia pliku
 *
 */

int gg_dcc7_reject_file(struct gg_session *sess, uin_t peer_uin, const char *id, uint32_t reason) {
	struct gg_dcc7_reject s;
	gg_debug_session(sess, GG_DEBUG_FUNCTION, "** gg_dcc7_reject_file(%p, %d, ?, 0x%x);\n", sess, peer_uin, reason);

	if (!sess || !id) {
		errno = EFAULT;
		return -1;
	}
	
	if (sess->state != GG_STATE_CONNECTED) {
		errno = ENOTCONN;
		return -1;
	}

	s.uin	= gg_fix32(peer_uin);
	memcpy(s.code1, id, 8);		/* XXX!!!!!!! uint64_t ? */
	s.reason= gg_fix32(reason);

	return gg_send_packet(sess, GG_DCC7_REJECT, &s, sizeof(s), NULL);
}

/*
 * gg_dcc_voice_send()
 *
 * wysy³a ramkê danych dla rozmowy g³osowej.
 *
 *  - d - struktura opisuj±ca po³±czenie dcc
 *  - buf - bufor z danymi
 *  - length - rozmiar ramki
 *
 * 0, -1.
 */
int gg_dcc_voice_send(struct gg_dcc *d, char *buf, int length)
{
	struct packet_s {
		uint8_t type;
		uint32_t length;
	} GG_PACKED;
	struct packet_s packet;

	gg_debug(GG_DEBUG_FUNCTION, "++ gg_dcc_voice_send(%p, %p, %d);\n", d, buf, length);
	if (!d || !buf || length < 0 || d->type != GG_SESSION_DCC_VOICE) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_voice_send() invalid argument\n");
		errno = EINVAL;
		return -1;
	}

	packet.type = 0x03; /* XXX */
	packet.length = gg_fix32(length);

	if (write(d->fd, &packet, sizeof(packet)) < (signed)sizeof(packet)) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_voice_send() write() failed\n");
		return -1;
	}
	gg_dcc_debug_data("write", d->fd, &packet, sizeof(packet));

	if (write(d->fd, buf, length) < length) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_voice_send() write() failed\n");
		return -1;
	}
	gg_dcc_debug_data("write", d->fd, buf, length);

	return 0;
}

#define gg_read(fd, buf, size) \
{ \
	int tmp = read(fd, buf, size); \
	\
	if (tmp < (int) size) { \
		if (tmp == -1) { \
			gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() failed (errno=%d, %s)\n", errno, strerror(errno)); \
		} else if (tmp == 0) { \
			gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() failed, connection broken\n"); \
		} else { \
			gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() failed (%d bytes, %d needed)\n", tmp, size); \
		} \
		e->type = GG_EVENT_DCC_ERROR; \
		e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE; \
		return e; \
	} \
	gg_dcc_debug_data("read", fd, buf, size); \
} 

#define gg_write(fd, buf, size) \
{ \
	int tmp; \
	gg_dcc_debug_data("write", fd, buf, size); \
	tmp = write(fd, buf, size); \
	if (tmp < (int) size) { \
		if (tmp == -1) { \
			gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() write() failed (errno=%d, %s)\n", errno, strerror(errno)); \
		} else { \
			gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() write() failed (%d needed, %d done)\n", size, tmp); \
		} \
		e->type = GG_EVENT_DCC_ERROR; \
		e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE; \
		return e; \
	} \
}

/*
 * gg_dcc_watch_fd()
 *
 * funkcja, któr± nale¿y wywo³aæ, gdy co¶ siê zmieni na gg_dcc->fd.
 *
 *  - h - struktura zwrócona przez gg_create_dcc_socket()
 *
 * zaalokowana struct gg_event lub NULL, je¶li zabrak³o pamiêci na ni±.
 */
struct gg_event *gg_dcc_watch_fd(struct gg_dcc *h)
{
	struct gg_event *e;
	int foo;

	gg_debug(GG_DEBUG_FUNCTION, "** gg_dcc_watch_fd(%p);\n", h);
	
	if (!h || (h->type != GG_SESSION_DCC && h->type != GG_SESSION_DCC_SOCKET && h->type != GG_SESSION_DCC_SEND && h->type != GG_SESSION_DCC_GET && h->type != GG_SESSION_DCC_VOICE)) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() invalid argument\n");
		errno = EINVAL;
		return NULL;
	}

	if (!(e = (void*) calloc(1, sizeof(*e)))) {
		gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() not enough memory\n");
		return NULL;
	}

	e->type = GG_EVENT_NONE;

	if (h->type == GG_SESSION_DCC_SOCKET) {
		struct sockaddr_in sin;
		struct gg_dcc *c;
		int fd, one = 1;
		unsigned int sin_len = sizeof(sin);
		
		if ((fd = accept(h->fd, (struct sockaddr*) &sin, &sin_len)) == -1) {
			gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() can't accept() new connection (errno=%d, %s)\n", errno, strerror(errno));
			return e;
		}

		gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() new direct connection from %s:%d\n", inet_ntoa(sin.sin_addr), htons(sin.sin_port));

#ifdef FIONBIO
		if (ioctl(fd, FIONBIO, &one) == -1) {
#else
		if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
#endif
			gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() can't set nonblocking (errno=%d, %s)\n", errno, strerror(errno));
			close(fd);
			e->type = GG_EVENT_DCC_ERROR;
			e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE;
			return e;
		}

		if (!(c = (void*) calloc(1, sizeof(*c)))) {
			gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() not enough memory for client data\n");

			free(e);
			close(fd);
			return NULL;
		}

		c->fd = fd;
		c->check = GG_CHECK_READ;
		c->state = GG_STATE_READING_UIN_1;
		c->type = GG_SESSION_DCC;
		c->timeout = GG_DEFAULT_TIMEOUT;
		c->file_fd = NULL; /* Miranda-IM fix */
		c->remote_addr = sin.sin_addr.s_addr;
		c->remote_port = ntohs(sin.sin_port);
		
		e->type = GG_EVENT_DCC_NEW;
		e->event.dcc_new = c;

		return e;
	} else {
		struct gg_dcc_tiny_packet tiny;
		struct gg_dcc_small_packet small;
		struct gg_dcc_big_packet big;
		int size, tmp, res;
		unsigned int utmp, res_size = sizeof(res);
		unsigned char buf[1024];
		char ack[] = "UDAG";

		struct gg_dcc_file_info_packet {
			struct gg_dcc_big_packet big;
			struct gg_file_info file_info;
		} GG_PACKED;
		struct gg_dcc_file_info_packet file_info_packet;

		switch (h->state) {
			case GG_STATE_READING_UIN_1:
			case GG_STATE_READING_UIN_2:
			{
				uin_t uin;

				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_READING_UIN_%d\n", (h->state == GG_STATE_READING_UIN_1) ? 1 : 2);
				
				gg_read(h->fd, &uin, sizeof(uin));

				if (h->state == GG_STATE_READING_UIN_1) {
					h->state = GG_STATE_READING_UIN_2;
					h->check = GG_CHECK_READ;
					h->timeout = GG_DEFAULT_TIMEOUT;
					h->peer_uin = gg_fix32(uin);
				} else {
					h->state = h->active == 3 ? GG_STATE_SENDING_ACK7 : GG_STATE_SENDING_ACK;
					h->check = GG_CHECK_WRITE;
					h->timeout = GG_DEFAULT_TIMEOUT;
					h->uin = gg_fix32(uin);
					e->type = GG_EVENT_DCC_CLIENT_ACCEPT;
				}

				return e;
			}

			case GG_STATE_SENDING_ACK:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_SENDING_ACK\n");

				gg_write(h->fd, ack, 4);

				h->state = GG_STATE_READING_TYPE;
				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;

				return e;

			case GG_STATE_SENDING_ACK7:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_SENDING_ACK7\n");
				memcpy(buf, &h->peer_uin, 4);
				memcpy(&buf[4], &h->uin, 4);

				gg_write(h->fd, buf, 8);

				h->state = GG_STATE_SENDING_FILE;
				h->check = GG_CHECK_WRITE;
				h->timeout = GG_DEFAULT_TIMEOUT;

				return e;

			case GG_STATE_READING_TYPE:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_TYPE\n");
				
				gg_read(h->fd, &small, sizeof(small));

				small.type = gg_fix32(small.type);

				switch (small.type) {
					case 0x0003:	/* XXX */
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() callback\n");
						h->type = GG_SESSION_DCC_SEND;
						h->state = GG_STATE_SENDING_FILE_INFO;
						h->check = GG_CHECK_WRITE;
						h->timeout = GG_DEFAULT_TIMEOUT;

						e->type = GG_EVENT_DCC_CALLBACK;
			
						break;

					case 0x0002:	/* XXX */
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() dialin\n");
						h->type = GG_SESSION_DCC_GET;
						h->state = GG_STATE_READING_REQUEST;
						h->check = GG_CHECK_READ;
						h->timeout = GG_DEFAULT_TIMEOUT;
						h->incoming = 1;

						break;

					default:
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() unknown dcc type (%.4x) from %ld\n", small.type, h->peer_uin);
						e->type = GG_EVENT_DCC_ERROR;
						e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE;
				}

				return e;

			case GG_STATE_READING_REQUEST:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_REQUEST\n");
				
				gg_read(h->fd, &small, sizeof(small));

				small.type = gg_fix32(small.type);

				switch (small.type) {
					case 0x0001:	/* XXX */
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() file transfer request\n");
						h->state = GG_STATE_READING_FILE_INFO;
						h->check = GG_CHECK_READ;
						h->timeout = GG_DEFAULT_TIMEOUT;
						break;
						
					case 0x0003:	/* XXX */
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() voice chat request\n");
						h->state = GG_STATE_SENDING_VOICE_ACK;
						h->check = GG_CHECK_WRITE;
						h->timeout = GG_DCC_TIMEOUT_VOICE_ACK;
						h->type = GG_SESSION_DCC_VOICE;
						e->type = GG_EVENT_DCC_NEED_VOICE_ACK;

						break;
						
					default:
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() unknown dcc request (%.4x) from %ld\n", small.type, h->peer_uin);
						e->type = GG_EVENT_DCC_ERROR;
						e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE;
				}
		 	
				return e;

			case GG_STATE_READING_FILE_INFO:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_FILE_INFO\n");
				
				gg_read(h->fd, &file_info_packet, sizeof(file_info_packet));

				memcpy(&h->file_info, &file_info_packet.file_info, sizeof(h->file_info));
		
				h->file_info.mode = gg_fix32(h->file_info.mode);
				h->file_info.size = gg_fix32(h->file_info.size);

				h->state = GG_STATE_SENDING_FILE_ACK;
				h->check = GG_CHECK_WRITE;
				h->timeout = GG_DCC_TIMEOUT_FILE_ACK;

				e->type = GG_EVENT_DCC_NEED_FILE_ACK;
				
				return e;

			case GG_STATE_SENDING_FILE_ACK:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_SENDING_FILE_ACK\n");
				
				big.type = gg_fix32(0x0006);	/* XXX */
				big.dunno1 = gg_fix32(h->offset);
				big.dunno2 = 0;

				gg_write(h->fd, &big, sizeof(big));

				h->state = GG_STATE_READING_FILE_HEADER;
				h->chunk_size = sizeof(big);
				h->chunk_offset = 0;
				if (!(h->chunk_buf = malloc(sizeof(big)))) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() out of memory\n");
					free(e);
					return NULL;
				}
				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;

				return e;
				
			case GG_STATE_SENDING_VOICE_ACK:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_SENDING_VOICE_ACK\n");
				
				tiny.type = 0x01;	/* XXX */

				gg_write(h->fd, &tiny, sizeof(tiny));

				h->state = GG_STATE_READING_VOICE_HEADER;
				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;

				h->offset = 0;
				
				return e;
				
			case GG_STATE_READING_FILE_HEADER:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_FILE_HEADER\n");
				
				tmp = read(h->fd, h->chunk_buf + h->chunk_offset, h->chunk_size - h->chunk_offset);

				if (tmp == -1) {
					gg_debug(GG_DEBUG_MISC, "// gg_watch_fd() read() failed (errno=%d, %s)\n", errno, strerror(errno));
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_NET;
					return e;
				}

				gg_dcc_debug_data("read", h->fd, h->chunk_buf + h->chunk_offset, h->chunk_size - h->chunk_offset);
				
				h->chunk_offset += tmp;

				if (h->chunk_offset < h->chunk_size)
					return e;

				memcpy(&big, h->chunk_buf, sizeof(big));
				free(h->chunk_buf);
				h->chunk_buf = NULL;

				big.type = gg_fix32(big.type);
				h->chunk_size = gg_fix32(big.dunno1);
				h->chunk_offset = 0;

				if (big.type == 0x0005)	{ /* XXX */
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() transfer refused\n");
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_REFUSED;
					return e;
				}

				if (h->chunk_size == 0) { 
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() empty chunk, EOF\n");
					e->type = GG_EVENT_DCC_DONE;
					return e;
				}

				h->state = GG_STATE_GETTING_FILE;
				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;
				h->established = 1;
			 	
				return e;

			case GG_STATE_READING_VOICE_HEADER:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_VOICE_HEADER\n");
				
				gg_read(h->fd, &tiny, sizeof(tiny));

				switch (tiny.type) {
					case 0x03:	/* XXX */
						h->state = GG_STATE_READING_VOICE_SIZE;
						h->check = GG_CHECK_READ;
						h->timeout = GG_DEFAULT_TIMEOUT;
						h->established = 1;
						break;
					case 0x04:	/* XXX */
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() peer breaking connection\n");
						/* XXX zwracaæ odpowiedni event */
					default:
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() unknown request (%.2x)\n", tiny.type);
						e->type = GG_EVENT_DCC_ERROR;
						e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE;
				}
			 	
				return e;

			case GG_STATE_READING_VOICE_SIZE:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_VOICE_SIZE\n");
				
				gg_read(h->fd, &small, sizeof(small));

				small.type = gg_fix32(small.type);

				if (small.type < 16 || small.type > sizeof(buf)) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() invalid voice frame size (%d)\n", small.type);
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_NET;
					
					return e;
				}

				h->chunk_size = small.type;
				h->chunk_offset = 0;

				if (!(h->voice_buf = malloc(h->chunk_size))) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() out of memory for voice frame\n");
					free(e);
					return NULL;
				}

				h->state = GG_STATE_READING_VOICE_DATA;
				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;
			 	
				return e;

			case GG_STATE_READING_VOICE_DATA:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_VOICE_DATA\n");
				
				tmp = read(h->fd, h->voice_buf + h->chunk_offset, h->chunk_size - h->chunk_offset);
				if (tmp < 1) {
					if (tmp == -1) {
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() failed (errno=%d, %s)\n", errno, strerror(errno));
					} else {
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() failed, connection broken\n");
					}
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_NET;
					return e;
				}

				gg_dcc_debug_data("read", h->fd, h->voice_buf + h->chunk_offset, tmp);

				h->chunk_offset += tmp;

				if (h->chunk_offset >= h->chunk_size) {
					e->type = GG_EVENT_DCC_VOICE_DATA;
					e->event.dcc_voice_data.data = (unsigned char*) h->voice_buf;
					e->event.dcc_voice_data.length = h->chunk_size;
					h->state = GG_STATE_READING_VOICE_HEADER;
					h->voice_buf = NULL;
				}

				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;
				
				return e;

			case GG_STATE_CONNECTING:
			{
				uin_t uins[2];

				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_CONNECTING\n");
				
				res = 0;
				if ((foo = getsockopt(h->fd, SOL_SOCKET, SO_ERROR, &res, &res_size)) || res) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() connection failed (fd=%d,errno=%d(%s),foo=%d,res=%d(%s))\n", h->fd, errno, strerror(errno), foo, res, strerror(res));
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE;
					return e;
				}

				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() connected, sending uins\n");
				
				uins[0] = gg_fix32(h->uin);
				uins[1] = gg_fix32(h->peer_uin);

				gg_write(h->fd, uins, sizeof(uins));
				
				h->state = h->active == 2 ? GG_STATE_READING_ACK7 : GG_STATE_READING_ACK;
				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;
				
				return e;
			}

			case GG_STATE_READING_ACK:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_ACK\n");
				
				gg_read(h->fd, buf, 4);
				if (strncmp(buf, ack, 4)) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() did't get ack\n");

					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE;
					return e;
				}
				h->check = GG_CHECK_WRITE;
				h->timeout = GG_DEFAULT_TIMEOUT;
				h->state = GG_STATE_SENDING_REQUEST;
				
				return e;

			case GG_STATE_READING_ACK7:
			{
				uin_t code1, code2;

				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_ACK7\n");

				gg_read(h->fd, &code1, 4);
				gg_read(h->fd, &code2, 4);

				/* sprawdz czy kod dcc sie zgadza */
				if (code1 != h->uin || code2 != h->peer_uin) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() bad code! %.8x != %.8x || %.8x != %.8x\n", 
						code1, h->uin, code2, h->peer_uin);
					
					/* XXX nizej, czy kody bledow nie mamy lepszych */
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE;
					return e;
				}

				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;
				h->state = GG_STATE_GETTING_FILE;

				return e;
			}

			case GG_STATE_SENDING_VOICE_REQUEST:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_SENDING_VOICE_REQUEST\n");

				small.type = gg_fix32(0x0003);
				
				gg_write(h->fd, &small, sizeof(small));

				h->state = GG_STATE_READING_VOICE_ACK;
				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;
				
				return e;
			
			case GG_STATE_SENDING_REQUEST:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_SENDING_REQUEST\n");

				small.type = (h->type == GG_SESSION_DCC_GET) ? gg_fix32(0x0003) : gg_fix32(0x0002);	/* XXX */
				
				gg_write(h->fd, &small, sizeof(small));
				
				switch (h->type) {
					case GG_SESSION_DCC_GET:
						h->state = GG_STATE_READING_REQUEST;
						h->check = GG_CHECK_READ;
						h->timeout = GG_DEFAULT_TIMEOUT;
						break;

					case GG_SESSION_DCC_SEND:
						h->state = GG_STATE_SENDING_FILE_INFO;
						h->check = GG_CHECK_WRITE;
						h->timeout = GG_DEFAULT_TIMEOUT;

						if (h->file_fd == NULL) /* Miranda IM fix */
							e->type = GG_EVENT_DCC_NEED_FILE_INFO;
						break;
						
					case GG_SESSION_DCC_VOICE:
						h->state = GG_STATE_SENDING_VOICE_REQUEST;
						h->check = GG_CHECK_WRITE;
						h->timeout = GG_DEFAULT_TIMEOUT;
						break;
				}

				return e;
			
			case GG_STATE_SENDING_FILE_INFO:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_SENDING_FILE_INFO\n");

				if (h->file_fd == NULL) { /* Miranda IM */
					e->type = GG_EVENT_DCC_NEED_FILE_INFO;
					return e;
				}

				small.type = gg_fix32(0x0001);	/* XXX */
				
				gg_write(h->fd, &small, sizeof(small));

				file_info_packet.big.type = gg_fix32(0x0003);	/* XXX */
				file_info_packet.big.dunno1 = 0;
				file_info_packet.big.dunno2 = 0;

				memcpy(&file_info_packet.file_info, &h->file_info, sizeof(h->file_info));

				/* zostaj± teraz u nas, wiêc odwracamy z powrotem */
				h->file_info.size = gg_fix32(h->file_info.size);
				h->file_info.mode = gg_fix32(h->file_info.mode);
				
				gg_write(h->fd, &file_info_packet, sizeof(file_info_packet));

				h->state = GG_STATE_READING_FILE_ACK;
				h->check = GG_CHECK_READ;
				h->timeout = GG_DCC_TIMEOUT_FILE_ACK;

				return e;
				
			case GG_STATE_READING_FILE_ACK:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_FILE_ACK\n");
				
				gg_read(h->fd, &big, sizeof(big));

				/* XXX sprawdzaæ wynik */
				h->offset = gg_fix32(big.dunno1);
				
				h->state = GG_STATE_SENDING_FILE_HEADER;
				h->check = GG_CHECK_WRITE;
				h->timeout = GG_DEFAULT_TIMEOUT;

				e->type = GG_EVENT_DCC_ACK;

				return e;

			case GG_STATE_READING_VOICE_ACK:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_READING_VOICE_ACK\n");
				
				gg_read(h->fd, &tiny, sizeof(tiny));

				if (tiny.type != 0x01) {
					gg_debug(GG_DEBUG_MISC, "// invalid reply (%.2x), connection refused\n", tiny.type);
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_REFUSED;
					return e;
				}

				h->state = GG_STATE_READING_VOICE_HEADER;
				h->check = GG_CHECK_READ;
				h->timeout = GG_DEFAULT_TIMEOUT;

				e->type = GG_EVENT_DCC_ACK;
				
				return e;

			case GG_STATE_SENDING_FILE_HEADER:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_SENDING_FILE_HEADER\n");
				
				h->chunk_offset = 0;
				
				if ((h->chunk_size = h->file_info.size - h->offset) > 4096) {
					h->chunk_size = 4096;
					big.type = gg_fix32(0x0003);  /* XXX */
				} else
					big.type = gg_fix32(0x0002);  /* XXX */

				big.dunno1 = gg_fix32(h->chunk_size);
				big.dunno2 = 0;
				
				gg_write(h->fd, &big, sizeof(big));

				h->state = GG_STATE_SENDING_FILE;
				h->check = GG_CHECK_WRITE;
				h->timeout = GG_DEFAULT_TIMEOUT;
				h->established = 1;

				return e;
				
			case GG_STATE_SENDING_FILE:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_SENDING_FILE\n");
				
				if ((utmp = h->chunk_size - h->chunk_offset) > sizeof(buf))
					utmp = sizeof(buf);
				
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() offset=%d, size=%d\n", h->offset, h->file_info.size);

				/* koniec pliku? */
				if (h->file_info.size == 0) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() reached eof on empty file\n");
					e->type = GG_EVENT_DCC_DONE;

					return e;
				}

				/* Miranda IM fix */
				fseek(h->file_fd, h->offset, SEEK_SET);

				size = fread(buf, 1, utmp, h->file_fd);

				/* b³±d */
				if (size == -1) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() failed. (errno=%d, %s)\n", errno, strerror(errno));

					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_FILE;

					return e;
				}

				/* koniec pliku? */
				if (size == 0) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() reached eof\n");
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_EOF;

					return e;
				}
				
				/* je¶li wczytali¶my wiêcej, utnijmy. */
				if (h->offset + size > h->file_info.size) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() too much (read=%d, ofs=%d, size=%d)\n", size, h->offset, h->file_info.size);
					size = h->file_info.size - h->offset;

					if (size < 1) {
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() reached EOF after cutting\n");
						e->type = GG_EVENT_DCC_DONE;
						return e;
					}
				}

				tmp = write(h->fd, buf, size);

				if (tmp == -1) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() write() failed (%s)\n", strerror(errno));
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_NET;
					return e;
				}

				h->offset += size;
				
				if (h->offset >= h->file_info.size) {
					e->type = GG_EVENT_DCC_DONE;
					return e;
				}
				
				h->chunk_offset += size;
				
				if (h->chunk_offset >= h->chunk_size) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() chunk finished\n");
					h->state = GG_STATE_SENDING_FILE_HEADER;
					h->timeout = GG_DEFAULT_TIMEOUT;
				} else {
					h->state = GG_STATE_SENDING_FILE;
					h->timeout = GG_DCC_TIMEOUT_SEND;
				}
				
				h->check = GG_CHECK_WRITE;

				return e;

			case GG_STATE_GETTING_FILE:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_GETTING_FILE\n");
				
				if ((utmp = h->chunk_size - h->chunk_offset) > sizeof(buf))
					utmp = sizeof(buf);
				
				size = read(h->fd, buf, utmp);

				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() ofs=%d, size=%d, read()=%d\n", h->offset, h->file_info.size, size);
				
				/* b³±d */
				if (size == -1) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() failed. (errno=%d, %s)\n", errno, strerror(errno));

					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_NET;

					return e;
				}

				/* koniec? */
				if (size == 0) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() read() reached eof\n");
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_EOF;

					return e;
				}
				
				tmp = fwrite(buf, 1, size, h->file_fd); /* Miranda IM fix */
				
				if (tmp == -1 || tmp < size) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() write() failed (%d:fd=%d:res=%d:%s)\n", tmp, h->file_fd, size, strerror(errno));
					e->type = GG_EVENT_DCC_ERROR;
					e->event.dcc_error = GG_ERROR_DCC_NET;
					return e;
				}

				h->offset += size;
				
				if (h->offset >= h->file_info.size) {
					e->type = GG_EVENT_DCC_DONE;
					return e;
				}

				h->chunk_offset += size;
				
				if (h->chunk_offset >= h->chunk_size) {
					gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() chunk finished\n");
					h->state = GG_STATE_READING_FILE_HEADER;
					h->timeout = GG_DEFAULT_TIMEOUT;
					h->chunk_offset = 0;
					h->chunk_size = sizeof(big);
					if (!(h->chunk_buf = malloc(sizeof(big)))) {
						gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() out of memory\n");
						free(e);
						return NULL;
					}
				} else {
					h->state = GG_STATE_GETTING_FILE;
					h->timeout = GG_DCC_TIMEOUT_GET;
				}
				
				h->check = GG_CHECK_READ;

				return e;
				
			default:
				gg_debug(GG_DEBUG_MISC, "// gg_dcc_watch_fd() GG_STATE_???\n");
				e->type = GG_EVENT_DCC_ERROR;
				e->event.dcc_error = GG_ERROR_DCC_HANDSHAKE;

				return e;
		}
	}
	
	return e;
}

#undef gg_read
#undef gg_write

/*
 * gg_dcc_free()
 *
 * zwalnia pamiêæ po strukturze po³±czenia dcc.
 *
 *  - d - zwalniana struktura
 */
void gg_dcc_free(struct gg_dcc *d)
{
	gg_debug(GG_DEBUG_FUNCTION, "** gg_dcc_free(%p);\n", d);
	
	if (!d)
		return;

	if (d->fd != -1)
		close(d->fd);

	if (d->chunk_buf) {
		free(d->chunk_buf);
		d->chunk_buf = NULL;
	}

	free(d);
}

/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: notnil
 * End:
 *
 * vim: shiftwidth=8:
 */
