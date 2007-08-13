/* coding: UTF-8 */
/* $Id$ */

/*
 *  (C) Copyright 2001-2003 Wojtek Kaniewski <wojtekka@irc.pl>
 *                          Robert J. Woźny <speedy@ziew.org>
 *                          Arkadiusz Miśkiewicz <arekm@pld-linux.org>
 *                          Tomasz Chiliński <chilek@chilan.com>
 *                          Piotr Wysocki <wysek@linux.bydg.org>
 *                          Dawid Jarosz <dawjar@poczta.onet.pl>
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

/**
 * \file libgadu.h
 *
 * \brief Główny plik nagłówkowy biblioteki
 */

#ifndef __GG_LIBGADU_H
#define __GG_LIBGADU_H

#include "libgadu-config.h"

#if defined(__cplusplus) || defined(GG_CONFIG_MIRANDA)
#ifdef _WIN32
#pragma pack(push, 1)
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef GG_CONFIG_HAVE_OPENSSL
#ifdef GG_CONFIG_MIRANDA
#include "../ssl.h"
#else
#include <openssl/ssl.h>
#endif
#endif
#if defined(GG_CONFIG_MIRANDA) || !defined(GG_CONFIG_HAVE_OPENSSL)
typedef struct {
    unsigned long state[5];
    unsigned long count[2];
    unsigned char buffer[64];
} SHA_CTX;

void SHA1_Transform(unsigned long state[5], const unsigned char buffer[64]);
void SHA1_Init(SHA_CTX* context);
void SHA1_Update(SHA_CTX* context, const unsigned char* data, unsigned int len);
void SHA1_Final(unsigned char digest[20], SHA_CTX* context);
#endif

#ifdef GG_CONFIG_MIRANDA
#define kill(x, y)
#endif

/** \endcond */

/**
 * Numer Gadu-Gadu.
 */
typedef uint32_t uin_t;

/**
 * Identyfikator połączenia bezpośredniego Gadu-Gadu 7.x.
 */
typedef struct {
	uint8_t id[8];
} gg_dcc7_id_t;

/*
 * Makro deklarujące pola wspólne dla struktur sesji.
 */
#define gg_common_head(x) \
	int fd;			/**< Obserwowany deskryptor */ \
	int check;		/**< Informacja o żądaniu odczytu/zapisu (patrz \c gg_check_t) */ \
	int state;		/**< Aktualny stan połączenia (patrz \c gg_state_t) */ \
	int error;		/**< Kod błędu dla \c GG_STATE_ERROR (patrz \c gg_error_t) */ \
	int type;		/**< Rodzaj sesji (patrz \c gg_session_t) */ \
	int id;			/**< Identyfikator sesji */ \
	int timeout;		/**< Czas pozostały do zakończenia stanu */ \
	int (*callback)(x*); 	/**< Funkcja zwrotna */ \
	void (*destroy)(x*); 	/**< Funkcja zwalniania zasobów */

/**
 * Struktura wspólna dla wszystkich sesji i połączeń. Pozwala na proste
 * rzutowanie niezależne od rodzaju połączenia.
 */
struct gg_common {
	gg_common_head(struct gg_common)
};

struct gg_image_queue;

struct gg_dcc7;

/**
 * Sesja Gadu-Gadu.
 *
 * Tworzona przez funkcję \c gg_login, zwalniana przez \c gg_free_session.
 */
struct gg_session {
	gg_common_head(struct gg_session)

	int async;      	/**< Flaga połączenia asynchronicznego */
	int pid;        	/**< Numer procesu rozwiązującego nazwę serwera */
	int port;       	/**< Port serwera */
	int seq;        	/**< Numer sekwencyjny ostatniej wiadomości */
	int last_pong;  	/**< Czas otrzymania ostatniej ramki utrzymaniowej */
	int last_event;		/**< Czas otrzymania ostatniego pakietu */

	struct gg_event *event;	/**< Zdarzenie po wywołaniu \c callback */

	uint32_t proxy_addr;	/**< Adres serwera pośredniczącego */
	uint16_t proxy_port;	/**< Port serwera pośredniczącego */

	uint32_t hub_addr;	/**< Adres huba po rozwiązaniu nazwy */
	uint32_t server_addr;	/**< Adres serwera otrzymany od huba */

	uint32_t client_addr;	/**< Adres gniazda dla połączeń bezpośrednich do wersji Gadu-Gadu 6.x */
	uint16_t client_port;	/**< Port gniazda dla połączeń bezpośrednich do wersji Gadu-Gadu 6.x */

	uint32_t external_addr;	/**< Publiczny adres dla połączeń bezpośrednich do wersji Gadu-Gadu 6.x */
	uint16_t external_port;	/**< Publiczny port dla połączeń bezpośrednich do wersji Gadu-Gadu 6.x */

	uin_t uin;		/**< Własny numer Gadu-Gadu */
	char *password;		/**< Hasło (zwalniane po użyciu) */

	int initial_status;	/**< Początkowy status */
	int status;		/**< Aktualny status */

	char *recv_buf;		/**< Bufor na odbierany pakiety */
	int recv_done;		/**< Liczba wczytanych bajtów pakietu */
	int recv_left;		/**< Liczba pozostałych do wczytania bajtów pakietu */

	int protocol_version;	/**< Wersja protokołu */
	char *client_version;	/**< Wersja klienta */
	int last_sysmsg;	/**< Numer ostatniej wiadomości systemowej */

	char *initial_descr;	/**< Początkowy opis statusu */

	void *resolver;		/**< Dane prywatne procesu lub wątku rozwiązującego nazwę serwera */

	char *header_buf;	/**< Bufor na początek nagłówka pakietu */
	unsigned int header_done;	/**< Liczba wczytanych bajtów nagłówka pakietu */

#ifdef GG_CONFIG_HAVE_OPENSSL
	SSL *ssl;		/**< Struktura TLS */
	SSL_CTX *ssl_ctx;	/**< Kontekst sesji TLS */
#else
	void *ssl;		/**< Struktura TLS */
	void *ssl_ctx;		/**< Kontekst sesji TLS */
#endif

	int image_size;		/**< Maksymalny rozmiar obsługiwanych obrazków w KiB */

	char *userlist_reply;	/**< Bufor z odbieraną listą kontaktów */

	int userlist_blocks;	/**< Liczba części listy kontaktów */

	struct gg_image_queue *images;	/**< Lista wczytywanych obrazków */

	int hash_type;		/**< Rodzaj funkcji skrótu hasła */

	char *send_buf;		/**< Bufor z danymi do wysłania */
	int send_left;		/**< Liczba bajtów do wysłania */

	struct gg_dcc7 *dcc7_list;	/**< Lista połączeń bezpośrednich skojarzonych z sesją */
	
	int soft_timeout;	/**< Flaga mówiąca, że po przekroczeniu \c timeout należy wywołać \c gg_watch_fd */
};

/**
 * Połączenie HTTP.
 *
 * Tworzone przez \c gg_http_connect, zwalniane przez \c gg_http_free.
 */
struct gg_http {
	gg_common_head(struct gg_http)

	int async;              /**< Flaga połączenia asynchronicznego */
	int pid;                /**< Identyfikator procesu rozwiązującego nazwę serwera */
	int port;               /**< Port */

	char *query;            /**< Zapytanie HTTP */
	char *header;           /**< Odebrany nagłówek */
	int header_size;        /**< Rozmiar wczytanego nagłówka */
	char *body;             /**< Odebrana strona */
	unsigned int body_size; /**< Rozmiar strony */

	void *data;             /**< Dane prywatne usługi HTTP */

	char *user_data;	/**< Dane prywatne użytkownika (nie są zwalniane) */

	void *resolver;		/**< Dane prywatne procesu lub wątku rozwiązującego nazwę */

	unsigned int body_done;	/**< Liczba odebranych bajtów strony */
};

/** \cond ignore */

#ifdef __GNUC__
#define GG_PACKED __attribute__ ((packed))
#else
#define GG_PACKED
#endif

/** \endcond */

#define GG_MAX_PATH 276		/**< Maksymalny rozmiar nazwy pliku w strukturze \c gg_file_info */

/**
 * Odpowiednik struktury WIN32_FIND_DATA z API WIN32.
 *
 * Wykorzystywana przy połączeniach bezpośrednich do wersji Gadu-Gadu 6.x.
 */
struct gg_file_info {
	uint32_t mode;			/**< dwFileAttributes */
	uint32_t ctime[2];		/**< ftCreationTime */
	uint32_t atime[2];		/**< ftLastAccessTime */
	uint32_t mtime[2];		/**< ftLastWriteTime */
	uint32_t size_hi;		/**< nFileSizeHigh */
	uint32_t size;			/**< nFileSizeLow */
	uint32_t reserved0;		/**< dwReserved0 */
	uint32_t reserved1;		/**< dwReserved1 */
	unsigned char filename[GG_MAX_PATH - 14];	/**< cFileName */
	unsigned char short_filename[14];		/**< cAlternateFileName */
} /** \cond ignore */ GG_PACKED /** \endcond */;

/**
 * Połączenie bezpośrednie do wersji Gadu-Gadu 6.x.
 *
 * Tworzone przez \c gg_dcc_socket_create, \c gg_dcc_get_file,
 * \c gg_dcc_send_file lub \c gg_dcc_voice_chat, zwalniane przez
 * \c gg_dcc_free.
 *
 * \ingroup dcc6
 */
struct gg_dcc {
	gg_common_head(struct gg_dcc)

	struct gg_event *event;	/**< Zdarzenie po wywołaniu \c callback */

	int active;		/**< Flaga połączenia aktywnego (nieużywana) */
	int port;		/**< Port gniazda nasłuchującego */
	uin_t uin;		/**< Własny numer Gadu-Gadu */
	uin_t peer_uin;		/**< Numer Gadu-Gadu drugiej strony połączenia */
#ifdef GG_CONFIG_MIRANDA
	FILE *file_fd;		/**< deskryptor pliku */
#else
	int file_fd;		/**< deskryptor pliku */
#endif
	unsigned int offset;	/**< Położenie w pliku */
	unsigned int chunk_size;
				/**< Rozmiar kawałka pliku */
	unsigned int chunk_offset;
				/**< Położenie w aktualnym kawałku pliku */
	struct gg_file_info file_info;
				/**< Informacje o pliku */
	int established;	/**< Flaga ustanowienia połączenia */
	char *voice_buf;	/**< Bufor na pakiet połączenia głosowego */
	int incoming;		/**< Flaga połączenia przychodzącego */
	char *chunk_buf;	/**< Bufor na fragment danych */
	uint32_t remote_addr;	/**< Adres drugiej strony */
	uint16_t remote_port;	/**< Port drugiej strony */

#ifdef GG_CONFIG_MIRANDA
	void *contact;
	char *folder;
	uint32_t tick;
#endif
};

#define GG_DCC7_HASH_LEN	20	/**< Maksymalny rozmiar skrótu pliku w połączeniach bezpośrenich */
#define GG_DCC7_FILENAME_LEN	255	/**< Maksymalny rozmiar nazwy pliku w połączeniach bezpośrednich */
#define GG_DCC7_INFO_LEN	64	/**< Maksymalny rozmiar informacji o połączeniach bezpośrednich */

/**
 * Połączenie bezpośrednie od wersji Gadu-Gadu 7.x.
 *
 * \ingroup dcc7
 */
struct gg_dcc7 {
	gg_common_head(struct gg_dcc7)

	gg_dcc7_id_t cid;	/**< Identyfikator połączenia */

	struct gg_event *event;	/**< Struktura zdarzenia */

	uin_t uin;		/**< Własny numer Gadu-Gadu */
	uin_t peer_uin;		/**< Numer Gadu-Gadu drugiej strony połączenia */

#ifdef GG_CONFIG_MIRANDA
	FILE *file_fd;		/**< Deskryptor przesyłanego pliku */
#else
	int file_fd;		/**< Deskryptor przesyłanego pliku */
#endif
	unsigned int offset;	/**< Aktualne położenie w przesyłanym pliku */
	unsigned int size;	/**< Rozmiar przesyłanego pliku */
	unsigned char filename[GG_DCC7_FILENAME_LEN + 1];
				/**< Nazwa przesyłanego pliku */
	unsigned char hash[GG_DCC7_HASH_LEN];
				/**< Skrót SHA1 przesyłanego pliku */

	int dcc_type;		/**< Rodzaj połączenia bezpośredniego */
	int established;	/**< Flaga ustanowienia połączenia */
	int incoming;		/**< Flaga połączenia przychodzącego */
	int reverse;		/**< Flaga połączenia zwrotnego */

	uint32_t local_addr;	/**< Adres lokalny */
	uint16_t local_port;	/**< Port lokalny */

	uint32_t remote_addr;	/**< Adres drugiej strony */
	uint16_t remote_port;	/**< Port drugiej strony */

	struct gg_session *sess;
				/**< Sesja do której przypisano połączenie */
	struct gg_dcc7 *next;	/**< Następne połączenie w liście */

	int soft_timeout;	/**< Flaga mówiąca, że po przekroczeniu \c timeout należy wywołać \c gg_dcc7_watch_fd */
};

/**
 * Rodzaj sesji.
 */
enum gg_session_t {
	GG_SESSION_GG = 1,	/**< Połączenie z serwerem Gadu-Gadu */
	GG_SESSION_HTTP,	/**< Połączenie HTTP */
	GG_SESSION_SEARCH,	/**< Wyszukiwanie w katalogu publicznym (nieaktualne) */
	GG_SESSION_REGISTER,	/**< Rejestracja nowego konta */
	GG_SESSION_REMIND,	/**< Przypominanie hasła */
	GG_SESSION_PASSWD,	/**< Zmiana hasła */
	GG_SESSION_CHANGE,	/**< Zmiana informacji w katalogu publicznym (nieaktualne) */
	GG_SESSION_DCC,		/**< Połączenie bezpośrednie (do wersji 6.x) */
	GG_SESSION_DCC_SOCKET,	/**< Gniazdo nasłuchujące (do wersji 6.x) */
	GG_SESSION_DCC_SEND,	/**< Wysyłanie pliku (do wersji 6.x) */
	GG_SESSION_DCC_GET,	/**< Odbieranie pliku (do wersji 6.x) */
	GG_SESSION_DCC_VOICE,	/**< Rozmowa głosowa (do wersji 6.x) */
	GG_SESSION_USERLIST_GET,	/**< Import listy kontaktów z serwera (nieaktualne) */
	GG_SESSION_USERLIST_PUT,	/**< Eksport listy kontaktów do serwera (nieaktualne) */
	GG_SESSION_UNREGISTER,	/**< Usuwanie konta */
	GG_SESSION_USERLIST_REMOVE,	/**< Usuwanie listy kontaktów z serwera (nieaktualne) */
	GG_SESSION_TOKEN,	/**< Pobieranie tokenu */
	GG_SESSION_DCC7_SOCKET,	/**< Gniazdo nasłuchujące (od wersji 7.x) */
	GG_SESSION_DCC7_SEND,	/**< Wysyłanie pliku (od wersji 7.x) */
	GG_SESSION_DCC7_GET,	/**< Odbieranie pliku (od wersji 7.x) */
	GG_SESSION_DCC7_VOICE,	/**< Rozmowa głosowa (od wersji 7.x) */

	GG_SESSION_USER0 = 256,	/**< Rodzaj zadeklarowany dla użytkownika */
	GG_SESSION_USER1,	/**< Rodzaj zadeklarowany dla użytkownika */
	GG_SESSION_USER2,	/**< Rodzaj zadeklarowany dla użytkownika */
	GG_SESSION_USER3,	/**< Rodzaj zadeklarowany dla użytkownika */
	GG_SESSION_USER4,	/**< Rodzaj zadeklarowany dla użytkownika */
	GG_SESSION_USER5,	/**< Rodzaj zadeklarowany dla użytkownika */
	GG_SESSION_USER6,	/**< Rodzaj zadeklarowany dla użytkownika */
	GG_SESSION_USER7	/**< Rodzaj zadeklarowany dla użytkownika */
};

/**
 * Aktualny stan sesji.
 */
enum gg_state_t {
	/* wspólne */
	GG_STATE_IDLE = 0,		/**< Nie dzieje się nic */
	GG_STATE_RESOLVING,             /**< Oczekiwanie na rozwiązanie nazwy serwera */
	GG_STATE_CONNECTING,            /**< Oczekiwanie na połączenie */
	GG_STATE_READING_DATA,		/**< Oczekiwanie na dane */
	GG_STATE_ERROR,			/**< Kod błędu w polu \c error */

	/* gg_session */
	GG_STATE_CONNECTING_HUB,	/**< Oczekiwanie na połączenie z hubem */
	GG_STATE_CONNECTING_GG,         /**< Oczekiwanie na połączenie z serwerem */
	GG_STATE_READING_KEY,           /**< Oczekiwanie na klucz */
	GG_STATE_READING_REPLY,         /**< Oczekiwanie na odpowiedź serwera */
	GG_STATE_CONNECTED,             /**< Połączono z serwerem */

	/* gg_http */
	GG_STATE_SENDING_QUERY,		/**< Wysłano zapytanie HTTP */
	GG_STATE_READING_HEADER,	/**< Oczekiwanie na nagłówek HTTP */
	GG_STATE_PARSING,               /**< Przetwarzanie danych */
	GG_STATE_DONE,                  /**< Połączenie zakończone */

	/* gg_dcc */
	GG_STATE_LISTENING,		/* czeka na połączenia */
	GG_STATE_READING_UIN_1,		/* czeka na uin peera */
	GG_STATE_READING_UIN_2,		/* czeka na swój uin */
	GG_STATE_SENDING_ACK,		/* wysyła potwierdzenie dcc */
	GG_STATE_READING_ACK,		/* czeka na potwierdzenie dcc */
	GG_STATE_READING_REQUEST,	/* czeka na komendę */
	GG_STATE_SENDING_REQUEST,	/* wysyła komendę */
	GG_STATE_SENDING_FILE_INFO,	/* wysyła informacje o pliku */
	GG_STATE_READING_PRE_FILE_INFO,	/* czeka na pakiet przed file_info */
	GG_STATE_READING_FILE_INFO,	/* czeka na informacje o pliku */
	GG_STATE_SENDING_FILE_ACK,	/* wysyła potwierdzenie pliku */
	GG_STATE_READING_FILE_ACK,	/* czeka na potwierdzenie pliku */
	GG_STATE_SENDING_FILE_HEADER,	/* wysyła nagłówek pliku */
	GG_STATE_READING_FILE_HEADER,	/* czeka na nagłówek */
	GG_STATE_GETTING_FILE,		/* odbiera plik */
	GG_STATE_SENDING_FILE,		/* wysyła plik */
	GG_STATE_READING_VOICE_ACK,	/* czeka na potwierdzenie voip */
	GG_STATE_READING_VOICE_HEADER,	/* czeka na rodzaj bloku voip */
	GG_STATE_READING_VOICE_SIZE,	/* czeka na rozmiar bloku voip */
	GG_STATE_READING_VOICE_DATA,	/* czeka na dane voip */
	GG_STATE_SENDING_VOICE_ACK,	/* wysyła potwierdzenie voip */
	GG_STATE_SENDING_VOICE_REQUEST,	/* wysyła żądanie voip */
	GG_STATE_READING_TYPE,		/* czeka na typ połączenia */

	/* nowe. bez sensu jest to API. */
	GG_STATE_TLS_NEGOTIATION,	/**< Negocjacja połączenia szyfrowanego */

	GG_STATE_REQUESTING_ID,		/**< Oczekiwanie na nadanie identyfikatora połączenia bezpośredniego */
	GG_STATE_WAITING_FOR_ACCEPT,	/**< Oczekiwanie na potwierdzenie lub odrzucenie połączenia bezpośredniego */
	GG_STATE_WAITING_FOR_INFO,	/**< Oczekiwanie na informacje o połączeniu bezpośrednim */

	GG_STATE_READING_ID,		/**< Odebranie identyfikatora połączenia bezpośredniego */
	GG_STATE_SENDING_ID		/**< Wysłano identyfikatora połączenia bezpośredniego */
};

/**
 * Informacja o tym, czy biblioteka chce zapisywać i/lub czytać
 * z deskryptora. Maska bitowa.
 */
enum gg_check_t {
	GG_CHECK_NONE = 0,		/**< Nie sprawdzaj niczego */
	GG_CHECK_WRITE = 1,		/**< Sprawdź możliwość zapisu */
	GG_CHECK_READ = 2		/**< Sprawdź możliwość odczytu */
};

/**
 * Parametry połączenia z serwerem Gadu-Gadu. Parametry zostały przeniesione
 * do struktury, by uniknąć zmian API po rozszerzeniu protokołu.
 *
 * \ingroup login
 */
struct gg_login_params {
	uin_t uin;			/**< Numer Gadu-Gadu */
	char *password;			/**< Hasło */
	int async;			/**< Flaga asynchronicznego połączenia (domyślnie nie) */
	int status;			/**< Początkowy status użytkownika (domyślnie \c GG_STATUS_AVAIL */
	char *status_descr;		/**< Początkowy opis użytkownika (domyślnie brak) */
	uint32_t server_addr;		/**< Adres serwera Gadu-Gadu (domyślnie pobierany automatycznie) */
	uint16_t server_port;		/**< Port serwera Gadu-Gadu (domyślnie pobierany automatycznie) */
	uint32_t client_addr;		/**< Adres połączeń bezpośrednich (nieaktualne) */
	uint16_t client_port;		/**< Port połączeń bezpośrednich (nieaktualne) */
	int protocol_version;		/**< Wymuszona wersja protokołu (nie ma wpływu na zachowanie biblioteki) */
	char *client_version;		/**< Wymuszona wersja klienta */
	int has_audio;			/**< Flaga obsługi połączeń głosowych */
	int last_sysmsg;		/**< Numer ostatniej wiadomości systemowej */
	uint32_t external_addr;		/**< Adres publiczny dla połączeń bezpośrednich do wersji Gadu-Gadu 6.x */
	uint16_t external_port;		/**< Port publiczny dla połączeń bezpośrednich do wersji Gadu-Gadu 6.x */
	int tls;			/**< Flaga połączenia szyfrowanego (nieaktualna) */
	int image_size;			/**< Maksymalny rozmiar obsługiwanych obrazków w KiB */
	int era_omnix;			/**< Flaga udawania klienta Era Omnix (nieaktualna) */
	int hash_type;			/**< Rodzaj skrótu hasła (domyślnie SHA1) */

/** \cond internal */

	char dummy[5 * sizeof(int)];	/**< \internal Miejsce na kilka kolejnych
					  parametrów, żeby wraz z dodawaniem kolejnych
					  parametrów nie zmieniał się rozmiar struktury */

/** \endcond */

};

struct gg_session *gg_login(const struct gg_login_params *p);
void gg_free_session(struct gg_session *sess);
void gg_logoff(struct gg_session *sess);
int gg_change_status(struct gg_session *sess, int status);
int gg_change_status_descr(struct gg_session *sess, int status, const char *descr);
int gg_change_status_descr_time(struct gg_session *sess, int status, const char *descr, int time);
int gg_send_message(struct gg_session *sess, int msgclass, uin_t recipient, const unsigned char *message);
int gg_send_message_richtext(struct gg_session *sess, int msgclass, uin_t recipient, const unsigned char *message, const unsigned char *format, int formatlen);
int gg_send_message_confer(struct gg_session *sess, int msgclass, int recipients_count, uin_t *recipients, const unsigned char *message);
int gg_send_message_confer_richtext(struct gg_session *sess, int msgclass, int recipients_count, uin_t *recipients, const unsigned char *message, const unsigned char *format, int formatlen);
int gg_send_message_ctcp(struct gg_session *sess, int msgclass, uin_t recipient, const unsigned char *message, int message_len);
int gg_ping(struct gg_session *sess);
int gg_userlist_request(struct gg_session *sess, char type, const char *request);
int gg_image_request(struct gg_session *sess, uin_t recipient, int size, uint32_t crc32);
int gg_image_reply(struct gg_session *sess, uin_t recipient, const char *filename, const char *image, int size);

uint32_t gg_crc32(uint32_t crc, const unsigned char *buf, int len);

/**
 * Rodzaj zdarzenia.
 */
enum gg_event_t {
	GG_EVENT_NONE = 0,		/**< Nic się nie wydarzyło */
	GG_EVENT_MSG,			/**< Otrzymano wiadomość */
	GG_EVENT_NOTIFY,		/**< Zmiana statusu kontaktu */
	GG_EVENT_NOTIFY_DESCR,		/**< Zmiana statusu kontaktu */
	GG_EVENT_STATUS,		/**< Zmiana statusu kontaktu */
	GG_EVENT_ACK,			/**< Potwierdzenie wiadomości */
	GG_EVENT_PONG,			/**< Utrzymanie połączenia */
	GG_EVENT_CONN_FAILED,		/**< Połączenie się nie udało */
	GG_EVENT_CONN_SUCCESS,		/**< Połączono */
	GG_EVENT_DISCONNECT,		/**< Serwer zrywa połączenie */

	GG_EVENT_DCC_NEW,		/**< Nowe połączenie bezpośrednie 6.x */
	GG_EVENT_DCC_ERROR,		/**< Błąd połączenia 6.x */
	GG_EVENT_DCC_DONE,		/**< Zakończono połączenie 6.x */
	GG_EVENT_DCC_CLIENT_ACCEPT,	/**< Moment akceptacji klienta */
	GG_EVENT_DCC_CALLBACK,		/**< Połączenie zwrotne */
	GG_EVENT_DCC_NEED_FILE_INFO,	/**< Należy wypełnić \c file_info */
	GG_EVENT_DCC_NEED_FILE_ACK,	/**< Czeka na potwierdzenie pliku */
	GG_EVENT_DCC_NEED_VOICE_ACK,	/**< Czeka na potwierdzenie rozmowy */
	GG_EVENT_DCC_VOICE_DATA, 	/**< Dane połączenia głosowego 6.x */

	GG_EVENT_PUBDIR50_SEARCH_REPLY,	/**< Odpowiedz katalogu publicznego */
	GG_EVENT_PUBDIR50_READ,		/**< Odczytano własne dane z katalogu publicznego */
	GG_EVENT_PUBDIR50_WRITE,	/**< Zmienino własne dane w katalogu publicznym */

	GG_EVENT_STATUS60,		/**< Zmiana statusu kontaktu */
	GG_EVENT_NOTIFY60,		/**< Zmiana statusu kontaktu */
	GG_EVENT_USERLIST,		/**< Odpowiedź listy kontaktów */
	GG_EVENT_IMAGE_REQUEST,		/**< Żądanie obrazka */
	GG_EVENT_IMAGE_REPLY,		/**< Odpowiedź z obrazkiem */
	GG_EVENT_DCC_ACK,		/**< Potwierdzenie transmisji */

	GG_EVENT_DCC7_NEW,		/**< Nowe połączenie bezpośrednie 7.x */
	GG_EVENT_DCC7_ACCEPT,		/**< Zaakceptowano połączenie 7.x */
	GG_EVENT_DCC7_REJECT,		/**< Odrzucono połączenie 7.x */
	GG_EVENT_DCC7_CONNECTED,	/**< Zestawiono połączenie 7.x */
	GG_EVENT_DCC7_ERROR,		/**< Błąd połączenia 7.x */
	GG_EVENT_DCC7_DONE		/**< Zakończono połączenie 7.x */
};

#define GG_EVENT_SEARCH50_REPLY GG_EVENT_PUBDIR50_SEARCH_REPLY

/**
 * Powód nieudanego połączenia.
 */
enum gg_failure_t {
	GG_FAILURE_RESOLVING = 1,	/**< Nie znaleziono serwera */
	GG_FAILURE_CONNECTING,		/**< Błąd połączenia */
	GG_FAILURE_INVALID,		/**< Serwer zwrócił nieprawidłowe dane */
	GG_FAILURE_READING,		/**< Zerwano połączenie podczas odczytu */
	GG_FAILURE_WRITING,		/**< Zerwano połączenie podczas zapisu */
	GG_FAILURE_PASSWORD,		/**< Nieprawidłowe hasło */
	GG_FAILURE_404, 		/**< Nieużywane */
	GG_FAILURE_TLS,			/**< Błąd negocjacji szyfrowanego połączenia */
	GG_FAILURE_NEED_EMAIL, 		/**< Serwer rozłączył nas z prośbą o zmianę adresu e-mail */
	GG_FAILURE_INTRUDER,		/**< Zbyt wiele prób połączenia z nieprawidłowym hasłem */
	GG_FAILURE_UNAVAILABLE		/**< Serwery są wyłączone */
};

/**
 * Kod błędu danej operacji.
 *
 * Nie zawiera przesadnie szczegółowych informacji o powodach błędów, by nie
 * komplikować ich obsługi. Jeśli wymagana jest większa dokładność, należy
 * sprawdzić zawartość zmiennej systemowej \c errno.
 */
enum gg_error_t {
	GG_ERROR_RESOLVING = 1,		/**< Nie znaleziono hosta */
	GG_ERROR_CONNECTING,		/**< Błąd połączenia */
	GG_ERROR_READING,		/**< Błąd odczytu/odbierania */
	GG_ERROR_WRITING,		/**< Błąd zapisu/wysyłania */

	GG_ERROR_DCC_HANDSHAKE,		/**< Błąd negocjacji */
	GG_ERROR_DCC_FILE,		/**< Błąd odczytu/zapisu pliku */
	GG_ERROR_DCC_EOF,		/**< Przedwczesny koniec pliku */
	GG_ERROR_DCC_NET,		/**< Błąd wysyłania/odbierania */
	GG_ERROR_DCC_REFUSED, 		/**< Połączenie odrzucone */

	GG_ERROR_DCC7_HANDSHAKE,	/**< Błąd negocjacji */
	GG_ERROR_DCC7_FILE,		/**< Błąd odczytu/zapisu pliku */
	GG_ERROR_DCC7_EOF,		/**< Przedwczesny koniec pliku */
	GG_ERROR_DCC7_NET,		/**< Błąd wysyłania/odbierania */
	GG_ERROR_DCC7_REFUSED 		/**< Połączenie odrzucone */
};

/**
 * Pole zapytania lub odpowiedzi katalogu publicznego.
 */
struct gg_pubdir50_entry {
	int num;	/**< Numer wyniku */
	char *field;	/**< Nazwa pola */
	char *value;	/**< Wartość pola */
};

/**
 * Zapytanie lub odpowiedź katalogu publicznego.
 *
 * Patrz \c gg_pubdir50_t.
 */
struct gg_pubdir50_s {
	int count;	/**< Liczba wyników odpowiedzi */
	uin_t next;	/**< Numer początkowy następnego zapytania */
	int type;	/**< Rodzaj zapytania */
	uint32_t seq;	/**< Numer sekwencyjny */
	struct gg_pubdir50_entry *entries;	/**< Pola zapytania lub odpowiedzi */
	int entries_count;	/**< Liczba pól */
};

/**
 * Zapytanie lub odpowiedź katalogu publicznego.
 *
 * Do pól nie należy się odwoływać bezpośrednio -- wszystkie niezbędne
 * informacje są dostępne za pomocą funkcji \c gg_pubdir50_...
 */
typedef struct gg_pubdir50_s *gg_pubdir50_t;

/**
 * Opis zdarzenia.
 *
 * Zwracany przez funkcje \c gg_watch_fd, \c gg_dcc_watch_fd
 * i \c gg_dcc7_watch_fd. Po przeanalizowaniu należy zwolnić
 * za pomocą \c gg_event_free.
 */
struct gg_event {
	int type;	/**< Rodzaj zdarzenia */

	union {
		/**
		 * Zmiana statusu kontaktów (\c GG_EVENT_NOTIFY)
		 */
		struct gg_notify_reply *notify;

		/**
		 * Błąd połączenia (\c GG_EVENT_CONN_FAILED)
		 */
		enum gg_failure_t failure;

		/**
		 * Nowe połączenie bezpośrednie (\c GG_EVENT_DCC_NEW)
		 */
		struct gg_dcc *dcc_new;

		/**
		 * Błąd połączenia bezpośredniego (\c GG_EVENT_DCC_ERROR)
		 */
		int dcc_error;

		/**
		 * Odpowiedź katalogu publicznego (\c GG_EVENT_PUBDIR50_...)
		 */
		gg_pubdir50_t pubdir50;

		/**
		 * Otrzymano wiadomość (\c GG_EVENT_MSG)
		 */
		struct {
			uin_t sender;		/**< Numer nadawcy */
			int msgclass;		/**< Klasa wiadomości */
			time_t time;		/**< Czas nadania */
			unsigned char *message;	/**< Treść wiadomości */

			int recipients_count;	/**< Liczba odbiorców konferencji */
			uin_t *recipients;	/**< Odbiorcy konferencji */

			int formats_length;	/**< Długość informacji o formatowaniu tekstu */
			void *formats;		/**< Informacje o formatowaniu tekstu */
		} msg;

		/**
		 * Zmiana statusu kontaktów (\c GG_EVENT_NOTIFY_DESCR)
		 */
		struct {
			struct gg_notify_reply *notify;	/**< Informacje o liście kontaktów */
			char *descr;		/**< Opis status */
		} notify_descr;

		/**
		 * Zmiana statusu kontaktów (\c GG_EVENT_STATUS)
		 */
		struct {
			uin_t uin;		/**< Numer Gadu-Gadu */
			uint32_t status;	/**< Nowy status */
			char *descr;		/**< Opis */
		} status;

		/**
		 * Zmiana statusu kontaktów (\c GG_EVENT_STATUS60)
		 */
		struct {
			uin_t uin;		/**< Numer Gadu-Gadu */
			int status;		/**< Nowy status */
			uint32_t remote_ip;	/**< Adres IP dla połączeń bezpośrednich */
			uint16_t remote_port;	/**< Port dla połączeń bezpośrednich */
			int version;		/**< Wersja protokołu */
			int image_size;		/**< Maksymalny rozmiar obsługiwanych obrazków w KiB */
			char *descr;		/**< Opis statusu */
			time_t time;		/**< Czas powrotu */
		} status60;

		/**
		 * Zmiana statusu kontaktów (\c GG_EVENT_NOTIFY60)
		 */
		struct {
			uin_t uin;		/**< Numer Gadu-Gadu */
			int status;		/**< Nowy status */
			uint32_t remote_ip;	/**< Adres IP dla połączeń bezpośrednich */
			uint16_t remote_port;	/**< Port dla połączeń bezpośrednich */
			int version;		/**< Wersja protokołu */
			int image_size;		/**< Maksymalny rozmiar obsługiwanych obrazków w KiB */
			char *descr;		/**< Opis statusu */
			time_t time;		/**< Czas powrotu */
		} *notify60;

		/**
		 * Potwierdzenie wiadomości (\c GG_EVENT_ACK)
		 */
		struct {
			uin_t recipient;	/**< Numer odbiorcy */
			int status;		/**< Status doręczenia */
			int seq;		/**< Numer sekwencyjny wiadomości */
		} ack;

		/**
		 * Dane połączenia głosowego (\c GG_EVENT_DCC_VOICE_DATA)
		 */
		struct {
			uint8_t *data;		/**< Dane dźwiękowe */
			int length;		/**< Rozmiar danych dźwiękowych */
		} dcc_voice_data;

		/**
		 * Odpowiedź listy kontaktów (\c GG_EVENT_USERLIST)
		 */
		struct {
			char type;		/**< Rodzaj odpowiedzi */
			char *reply;		/**< Treść odpowiedzi */
		} userlist;

		/**
		 * Żądanie wysłania obrazka (\c GG_EVENT_IMAGE_REQUEST)
		 */
		struct {
			uin_t sender;		/**< Nadawca żądania */
			uint32_t size;		/**< Rozmiar obrazka */
			uint32_t crc32;		/**< Suma kontrolna CRC32 */
		} image_request;

		/**
		 * Odpowiedź z obrazkiem (\c GG_EVENT_IMAGE_REPLY)
		 */
		struct {
			uin_t sender;		/**< Nadawca obrazka */
			uint32_t size;		/**< Rozmiar obrazka */
			uint32_t crc32;		/**< Suma kontrolna CRC32 */
			char *filename;		/**< Nazwa pliku */
			char *image;		/**< Bufor z obrazkiem */
		} image_reply;

		/**
		 * Nowe połączenie bezpośrednie (\c GG_EVENT_DCC7_NEW)
		 */
		struct gg_dcc7 *dcc7_new;

		/**
		 * Błąd połączenia bezpośredniego (\c GG_EVENT_DCC7_ERROR)
		 */
		int dcc7_error;

		/**
		 * Informacja o zestawieniu połączenia bezpośredniego
		 * (\c GG_EVENT_DCC7_CONNECTED)
		 */
		struct {
			struct gg_dcc7 *dcc7;	/**< Struktura połączenia */
			// XXX czy coś się przyda?
		} dcc7_connected;

		/**
		 * Odrzucono połączenia bezpośredniego
		 * (\c GG_EVENT_DCC7_REJECT)
		 */
		struct {
			struct gg_dcc7 *dcc7;	/**< Struktura połączenia */
			int reason;		/**< powód odrzucenia */
		} dcc7_reject;

		/**
		 * Zaakceptowano połączenie bezpośrednie
		 * (\c GG_EVENT_DCC7_ACCEPT)
		 */
		struct {
			struct gg_dcc7 *dcc7;	/**< Struktura połączenia */
			int type;		/**< Sposób połączenia (P2P, przez serwer) */
			uint32_t remote_ip;	/**< Adres zdalnego klienta */
			uint16_t remote_port;	/**< Port zdalnego klienta */
		} dcc7_accept;
	} event;
};

struct gg_event *gg_watch_fd(struct gg_session *sess);
void gg_event_free(struct gg_event *e);

int gg_notify_ex(struct gg_session *sess, uin_t *userlist, char *types, int count);
int gg_notify(struct gg_session *sess, uin_t *userlist, int count);
int gg_add_notify_ex(struct gg_session *sess, uin_t uin, char type);
int gg_add_notify(struct gg_session *sess, uin_t uin);
int gg_remove_notify_ex(struct gg_session *sess, uin_t uin, char type);
int gg_remove_notify(struct gg_session *sess, uin_t uin);

struct gg_http *gg_http_connect(const char *hostname, int port, int async, const char *method, const char *path, const char *header);
int gg_http_watch_fd(struct gg_http *h);
void gg_http_stop(struct gg_http *h);
void gg_http_free(struct gg_http *h);

uint32_t gg_pubdir50(struct gg_session *sess, gg_pubdir50_t req);
gg_pubdir50_t gg_pubdir50_new(int type);
int gg_pubdir50_add(gg_pubdir50_t req, const char *field, const char *value);
int gg_pubdir50_seq_set(gg_pubdir50_t req, uint32_t seq);
const char *gg_pubdir50_get(gg_pubdir50_t res, int num, const char *field);
int gg_pubdir50_type(gg_pubdir50_t res);
int gg_pubdir50_count(gg_pubdir50_t res);
uin_t gg_pubdir50_next(gg_pubdir50_t res);
uint32_t gg_pubdir50_seq(gg_pubdir50_t res);
void gg_pubdir50_free(gg_pubdir50_t res);

#define GG_PUBDIR50_UIN "FmNumber"		/**< Numer Gadu-Gadu */
#define GG_PUBDIR50_STATUS "FmStatus"		/**< Status */
#define GG_PUBDIR50_FIRSTNAME "firstname"	/**< Imię */
#define GG_PUBDIR50_LASTNAME "lastname"		/**< Nazwisko */
#define GG_PUBDIR50_NICKNAME "nickname"		/**< Pseudonim */
#define GG_PUBDIR50_BIRTHYEAR "birthyear"	/**< Rok urodzenia lub przedział lat oddzielony spacją */
#define GG_PUBDIR50_CITY "city"			/**< Miejscowość */
#define GG_PUBDIR50_GENDER "gender"		/**< Płeć */
#define GG_PUBDIR50_GENDER_FEMALE "1"		/**< Kobieta (przy wyszukiwaniu) */
#define GG_PUBDIR50_GENDER_MALE "2"		/**< Mężczyzna (przy wyszukiwaniu) */
#define GG_PUBDIR50_GENDER_SET_FEMALE "2"	/**< Kobieta (przy zmianie danych) */
#define GG_PUBDIR50_GENDER_SET_MALE "1"		/**< Mężczyzna (przy wyszukiwaniu) */
#define GG_PUBDIR50_ACTIVE "ActiveOnly"		/**< Flaga wyszukiwania osób dostępnych */
#define GG_PUBDIR50_ACTIVE_TRUE "1"		/**< Wartość flagi wyszukiwania osób dostępnych */
#define GG_PUBDIR50_START "fmstart"		/**< Numer początkowy wyszukiwania */
#define GG_PUBDIR50_FAMILYNAME "familyname"	/**< Nazwisko rodowe */
#define GG_PUBDIR50_FAMILYCITY "familycity"	/**< Miejscowość pochodzenia */

/**
 * Wynik operacji na katalogu publicznym.
 */
struct gg_pubdir {
	int success;		/**< Flaga powodzenia operacji */
	uin_t uin;		/**< Otrzymany numer lub 0 w przypadku błędu */
};

int gg_pubdir_watch_fd(struct gg_http *f);
void gg_pubdir_free(struct gg_http *f);

/**
 * Token autoryzacji niektórych operacji HTTP.
 * 
 * \ingroup token
 */
struct gg_token {
	int width;		/**< Szerokość obrazka */
	int height;		/**< Wysokość obrazka */
	int length;		/**< Liczba znaków w tokenie */
	char *tokenid;		/**< Identyfikator tokenu */
};

struct gg_http *gg_token(int async);
int gg_token_watch_fd(struct gg_http *h);
void gg_token_free(struct gg_http *h);

struct gg_http *gg_register3(const char *email, const char *password, const char *tokenid, const char *tokenval, int async);
#ifndef DOXYGEN
#define gg_register_watch_fd gg_pubdir_watch_fd
#define gg_register_free gg_pubdir_free
#endif

struct gg_http *gg_unregister3(uin_t uin, const char *password, const char *tokenid, const char *tokenval, int async);
#ifndef DOXYGEN
#define gg_unregister_watch_fd gg_pubdir_watch_fd
#define gg_unregister_free gg_pubdir_free
#endif

struct gg_http *gg_remind_passwd3(uin_t uin, const char *email, const char *tokenid, const char *tokenval, int async);
#ifndef DOXYGEN
#define gg_remind_passwd_watch_fd gg_pubdir_watch_fd
#define gg_remind_passwd_free gg_pubdir_free
#endif

struct gg_http *gg_change_passwd4(uin_t uin, const char *email, const char *passwd, const char *newpasswd, const char *tokenid, const char *tokenval, int async);
#ifndef DOXYGEN
#define gg_change_passwd_watch_fd gg_pubdir_watch_fd
#define gg_change_passwd_free gg_pubdir_free
#endif

extern int gg_dcc_port;
extern unsigned long gg_dcc_ip;

int gg_dcc_request(struct gg_session *sess, uin_t uin);

struct gg_dcc *gg_dcc_send_file(uint32_t ip, uint16_t port, uin_t my_uin, uin_t peer_uin);
struct gg_dcc *gg_dcc_get_file(uint32_t ip, uint16_t port, uin_t my_uin, uin_t peer_uin);
struct gg_dcc *gg_dcc_voice_chat(uint32_t ip, uint16_t port, uin_t my_uin, uin_t peer_uin);
void gg_dcc_set_type(struct gg_dcc *d, int type);
int gg_dcc_fill_file_info(struct gg_dcc *d, const char *filename);
int gg_dcc_fill_file_info2(struct gg_dcc *d, const char *filename, const char *local_filename);
int gg_dcc_voice_send(struct gg_dcc *d, char *buf, int length);

#define GG_DCC_VOICE_FRAME_LENGTH 195		/**< Rozmiar pakietu głosowego przed wersją Gadu-Gadu 5.0.5 */
#define GG_DCC_VOICE_FRAME_LENGTH_505 326	/**< Rozmiar pakietu głosowego od wersji Gadu-Gadu 5.0.5 */

struct gg_dcc *gg_dcc_socket_create(uin_t uin, uint16_t port);
#ifndef DOXYGEN
#define gg_dcc_socket_free gg_dcc_free
#define gg_dcc_socket_watch_fd gg_dcc_watch_fd
#endif

struct gg_event *gg_dcc_watch_fd(struct gg_dcc *d);

void gg_dcc_free(struct gg_dcc *c);

struct gg_event *gg_dcc7_watch_fd(struct gg_dcc7 *d);
struct gg_dcc7 *gg_dcc7_send_file(struct gg_session *sess, uin_t rcpt, const char *filename, const char *filename1250, const char *hash);
int gg_dcc7_accept(struct gg_dcc7 *dcc, unsigned int offset);
int gg_dcc7_reject(struct gg_dcc7 *dcc, int reason);
void gg_dcc7_free(struct gg_dcc7 *d);

extern int gg_debug_level;

extern void (*gg_debug_handler)(int level, const char *format, va_list ap);
extern void (*gg_debug_handler_session)(struct gg_session *sess, int level, const char *format, va_list ap);

extern FILE *gg_debug_file;

#define GG_DEBUG_NET 1		/**< Rejestracja zdarzeń związanych z siecią */
#define GG_DEBUG_TRAFFIC 2	/**< Rejestracja ruchu sieciowego */
#define GG_DEBUG_DUMP 4		/**< Rejestracja zawartości pakietów */
#define GG_DEBUG_FUNCTION 8	/**< Rejestracja wywołań funkcji */
#define GG_DEBUG_MISC 16	/**< Rejestracja różnych informacji */

#ifdef GG_DEBUG_DISABLE
#define gg_debug(x, y...) do { } while(0)
#define gg_debug_session(z, x, y...) do { } while(0)
#else
void gg_debug(int level, const char *format, ...);
void gg_debug_session(struct gg_session *sess, int level, const char *format, ...);
#endif

const char *gg_libgadu_version(void);

extern int gg_proxy_enabled;
extern char *gg_proxy_host;
extern int gg_proxy_port;
extern char *gg_proxy_username;
extern char *gg_proxy_password;
extern int gg_proxy_http_only;

extern unsigned long gg_local_ip;

#define GG_LOGIN_HASH_GG32 0x01	/**< Algorytm Gadu-Gadu */
#define GG_LOGIN_HASH_SHA1 0x02	/**< Algorytm SHA1 */

#define GG_PUBDIR50_WRITE 0x01
#define GG_PUBDIR50_READ 0x02
#define GG_PUBDIR50_SEARCH 0x03
#define GG_PUBDIR50_SEARCH_REQUEST GG_PUBDIR50_SEARCH
#define GG_PUBDIR50_SEARCH_REPLY 0x05

/** \cond obsolete */

#define gg_free_event gg_event_free
#define gg_free_http gg_http_free
#define gg_free_pubdir gg_pubdir_free
#define gg_free_register gg_pubdir_free
#define gg_free_remind_passwd gg_pubdir_free
#define gg_free_dcc gg_dcc_free
#define gg_free_change_passwd gg_pubdir_free

struct gg_search_request {
	int active;
	unsigned int start;
	char *nickname;
	char *first_name;
	char *last_name;
	char *city;
	int gender;
	int min_birth;
	int max_birth;
	char *email;
	char *phone;
	uin_t uin;
};

struct gg_search {
	int count;
	struct gg_search_result *results;
};

struct gg_search_result {
	uin_t uin;
	char *first_name;
	char *last_name;
	char *nickname;
	int born;
	int gender;
	char *city;
	int active;
};

#define GG_GENDER_NONE 0
#define GG_GENDER_FEMALE 1
#define GG_GENDER_MALE 2

struct gg_http *gg_search(const struct gg_search_request *r, int async);
int gg_search_watch_fd(struct gg_http *f);
void gg_free_search(struct gg_http *f);
#define gg_search_free gg_free_search

const struct gg_search_request *gg_search_request_mode_0(char *nickname, char *first_name, char *last_name, char *city, int gender, int min_birth, int max_birth, int active, int start);
const struct gg_search_request *gg_search_request_mode_1(char *email, int active, int start);
const struct gg_search_request *gg_search_request_mode_2(char *phone, int active, int start);
const struct gg_search_request *gg_search_request_mode_3(uin_t uin, int active, int start);
void gg_search_request_free(struct gg_search_request *r);

struct gg_http *gg_register(const char *email, const char *password, int async);
struct gg_http *gg_register2(const char *email, const char *password, const char *qa, int async);

struct gg_http *gg_unregister(uin_t uin, const char *password, const char *email, int async);
struct gg_http *gg_unregister2(uin_t uin, const char *password, const char *qa, int async);

struct gg_http *gg_remind_passwd(uin_t uin, int async);
struct gg_http *gg_remind_passwd2(uin_t uin, const char *tokenid, const char *tokenval, int async);

struct gg_http *gg_change_passwd(uin_t uin, const char *passwd, const char *newpasswd, const char *newemail, int async);
struct gg_http *gg_change_passwd2(uin_t uin, const char *passwd, const char *newpasswd, const char *email, const char *newemail, int async);
struct gg_http *gg_change_passwd3(uin_t uin, const char *passwd, const char *newpasswd, const char *qa, int async);

struct gg_change_info_request {
	char *first_name;
	char *last_name;
	char *nickname;
	char *email;
	int born;
	int gender;
	char *city;
};

struct gg_change_info_request *gg_change_info_request_new(const char *first_name, const char *last_name, const char *nickname, const char *email, int born, int gender, const char *city);
void gg_change_info_request_free(struct gg_change_info_request *r);

struct gg_http *gg_change_info(uin_t uin, const char *passwd, const struct gg_change_info_request *request, int async);
#define gg_change_pubdir_watch_fd gg_pubdir_watch_fd
#define gg_change_pubdir_free gg_pubdir_free
#define gg_free_change_pubdir gg_pubdir_free

struct gg_http *gg_userlist_get(uin_t uin, const char *password, int async);
int gg_userlist_get_watch_fd(struct gg_http *f);
void gg_userlist_get_free(struct gg_http *f);

struct gg_http *gg_userlist_put(uin_t uin, const char *password, const char *contacts, int async);
int gg_userlist_put_watch_fd(struct gg_http *f);
void gg_userlist_put_free(struct gg_http *f);

struct gg_http *gg_userlist_remove(uin_t uin, const char *password, int async);
int gg_userlist_remove_watch_fd(struct gg_http *f);
void gg_userlist_remove_free(struct gg_http *f);

/** \endcond */

int gg_pubdir50_handle_reply(struct gg_event *e, const char *packet, int length);

#ifdef GG_CONFIG_MIRANDA
int gg_file_hash_sha1(FILE *fd, uint8_t *result);
#else
int gg_file_hash_sha1(int fd, uint8_t *result);
#endif

#ifdef GG_CONFIG_HAVE_PTHREAD
int gg_resolve_pthread(int *fd, void **resolver, const char *hostname);
void gg_resolve_pthread_cleanup(void *resolver, int kill);
#endif

#ifdef _WIN32
int gg_thread_socket(int thread_id, int socket);
#endif

int gg_resolve(int *fd, int *pid, const char *hostname);

#ifdef __GNUC__
char *gg_saprintf(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
#else
char *gg_saprintf(const char *format, ...);
#endif

char *gg_vsaprintf(const char *format, va_list ap);

#define gg_alloc_sprintf gg_saprintf

char *gg_get_line(char **ptr);

int gg_connect(void *addr, int port, int async);
struct in_addr *gg_gethostbyname(const char *hostname);
char *gg_read_line(int sock, char *buf, int length);
void gg_chomp(char *line);
char *gg_urlencode(const char *str);
int gg_http_hash(const char *format, ...);
void gg_http_free_fields(struct gg_http *h);
int gg_read(struct gg_session *sess, char *buf, int length);
int gg_write(struct gg_session *sess, const char *buf, int length);
void *gg_recv_packet(struct gg_session *sess);
int gg_send_packet(struct gg_session *sess, int type, ...);
unsigned int gg_login_hash(const unsigned char *password, unsigned int seed);
void gg_login_hash_sha1(const char *password, uint32_t seed, uint8_t *result);
uint32_t gg_fix32(uint32_t x);
uint16_t gg_fix16(uint16_t x);
#define fix16 gg_fix16
#define fix32 gg_fix32
char *gg_proxy_auth(void);
char *gg_base64_encode(const char *buf);
char *gg_base64_decode(const char *buf);
int gg_image_queue_remove(struct gg_session *s, struct gg_image_queue *q, int freeq);

/**
 * Kolejka odbieranych obrazków.
 */
struct gg_image_queue {
	uin_t sender;			/**< Nadawca obrazka */
	uint32_t size;			/**< Rozmiar obrazka */
	uint32_t crc32;			/**< Suma kontrolna CRC32 */
	char *filename;			/**< Nazwa pliku */
	char *image;			/**< Bufor z odebranymi danymi */
	uint32_t done;			/**< Rozmiar odebranych danych */

	struct gg_image_queue *next;	/**< Kolejny element listy */
};

int gg_dcc7_handle_id(struct gg_session *sess, struct gg_event *e, void *payload, int len);
int gg_dcc7_handle_new(struct gg_session *sess, struct gg_event *e, void *payload, int len);
int gg_dcc7_handle_info(struct gg_session *sess, struct gg_event *e, void *payload, int len);
int gg_dcc7_handle_accept(struct gg_session *sess, struct gg_event *e, void *payload, int len);
int gg_dcc7_handle_reject(struct gg_session *sess, struct gg_event *e, void *payload, int len);

#define GG_APPMSG_HOST "appmsg.gadu-gadu.pl"
#define GG_APPMSG_PORT 80
#define GG_PUBDIR_HOST "pubdir.gadu-gadu.pl"
#define GG_PUBDIR_PORT 80
#define GG_REGISTER_HOST "register.gadu-gadu.pl"
#define GG_REGISTER_PORT 80
#define GG_REMIND_HOST "retr.gadu-gadu.pl"
#define GG_REMIND_PORT 80

#define GG_DEFAULT_PORT 8074
#define GG_HTTPS_PORT 443
#define GG_HTTP_USERAGENT "Mozilla/4.7 [en] (Win98; I)"

#define GG_DEFAULT_CLIENT_VERSION "6, 1, 0, 158"
#define GG_DEFAULT_PROTOCOL_VERSION 0x24
#define GG_DEFAULT_TIMEOUT 30
#define GG_HAS_AUDIO_MASK 0x40000000
#define GG_HAS_AUDIO7_MASK 0x20000000
#define GG_ERA_OMNIX_MASK 0x04000000
#define GG_LIBGADU_VERSION "CVS"

#define GG_DEFAULT_DCC_PORT 1550

struct gg_header {
	uint32_t type;			/* typ pakietu */
	uint32_t length;		/* długość reszty pakietu */
} GG_PACKED;

#define GG_WELCOME 0x0001
#define GG_NEED_EMAIL 0x0014

struct gg_welcome {
	uint32_t key;			/* klucz szyfrowania hasła */
} GG_PACKED;

#define GG_LOGIN 0x000c

struct gg_login {
	uint32_t uin;			/* mój numerek */
	uint32_t hash;			/* hash hasła */
	uint32_t status;		/* status na dzień dobry */
	uint32_t version;		/* moja wersja klienta */
	uint32_t local_ip;		/* mój adres ip */
	uint16_t local_port;		/* port, na którym słucham */
} GG_PACKED;

#define GG_LOGIN_EXT 0x0013

struct gg_login_ext {
	uint32_t uin;			/* mój numerek */
	uint32_t hash;			/* hash hasła */
	uint32_t status;		/* status na dzień dobry */
	uint32_t version;		/* moja wersja klienta */
	uint32_t local_ip;		/* mój adres ip */
	uint16_t local_port;		/* port, na którym słucham */
	uint32_t external_ip;		/* zewnętrzny adres ip */
	uint16_t external_port;		/* zewnętrzny port */
} GG_PACKED;

#define GG_LOGIN60 0x0015

struct gg_login60 {
	uint32_t uin;			/* mój numerek */
	uint32_t hash;			/* hash hasła */
	uint32_t status;		/* status na dzień dobry */
	uint32_t version;		/* moja wersja klienta */
	uint8_t dunno1;			/* 0x00 */
	uint32_t local_ip;		/* mój adres ip */
	uint16_t local_port;		/* port, na którym słucham */
	uint32_t external_ip;		/* zewnętrzny adres ip */
	uint16_t external_port;		/* zewnętrzny port */
	uint8_t image_size;		/* maksymalny rozmiar grafiki w KiB */
	uint8_t dunno2;			/* 0xbe */
} GG_PACKED;

#define GG_LOGIN70 0x19

struct gg_login70 {
	uint32_t uin;			/* mój numerek */
	uint8_t hash_type;		/* rodzaj hashowania hasła */
	uint8_t hash[64];		/* hash hasła dopełniony zerami */
	uint32_t status;		/* status na dzień dobry */
	uint32_t version;		/* moja wersja klienta */
	uint8_t dunno1;			/* 0x00 */
	uint32_t local_ip;		/* mój adres ip */
	uint16_t local_port;		/* port, na którym słucham */
	uint32_t external_ip;		/* zewnętrzny adres ip (???) */
	uint16_t external_port;		/* zewnętrzny port (???) */
	uint8_t image_size;		/* maksymalny rozmiar grafiki w KiB */
	uint8_t dunno2;			/* 0xbe */
} GG_PACKED;

#define GG_LOGIN_OK 0x0003

#define GG_LOGIN_FAILED 0x0009

#define GG_PUBDIR50_REQUEST 0x0014

struct gg_pubdir50_request {
	uint8_t type;			/* GG_PUBDIR50_* */
	uint32_t seq;			/* czas wysłania zapytania */
} GG_PACKED;

#define GG_PUBDIR50_REPLY 0x000e

struct gg_pubdir50_reply {
	uint8_t type;			/* GG_PUBDIR50_* */
	uint32_t seq;			/* czas wysłania zapytania */
} GG_PACKED;

#define GG_NEW_STATUS 0x0002

// XXX API
#define GG_STATUS_NOT_AVAIL 0x0001		/**< Niedostępny */
#define GG_STATUS_NOT_AVAIL_DESCR 0x0015	/**< Niedostępny z opisem */
#define GG_STATUS_AVAIL 0x0002			/**< Dostępny */
#define GG_STATUS_AVAIL_DESCR 0x0004		/**< Dostępny z opisem */
#define GG_STATUS_BUSY 0x0003			/**< Zajęty */
#define GG_STATUS_BUSY_DESCR 0x0005		/**< Zajęty z opisem */
#define GG_STATUS_INVISIBLE 0x0014		/**< Niewidoczny (tylko ustawianie) */
#define GG_STATUS_INVISIBLE_DESCR 0x0016	/**< Niewidoczny z opisem (tylko ustawianie) */
#define GG_STATUS_BLOCKED 0x0006		/**< Zablokowany */

#define GG_STATUS_FRIENDS_MASK 0x8000		/**< Tylko dla znajomych (flaga bitowa) */

#define GG_STATUS_DESCR_MAXSIZE 70		/**< Maksymalny rozmiar opisu */

/* GG_S_F() tryb tylko dla znajomych */
#define GG_S_F(x) (((x) & GG_STATUS_FRIENDS_MASK) != 0)

/* GG_S() stan bez uwzględnienia trybu tylko dla znajomych */
#define GG_S(x) ((x) & ~GG_STATUS_FRIENDS_MASK)

/* GG_S_A() dostępny */
#define GG_S_A(x) (GG_S(x) == GG_STATUS_AVAIL || GG_S(x) == GG_STATUS_AVAIL_DESCR)

/* GG_S_NA() niedostępny */
#define GG_S_NA(x) (GG_S(x) == GG_STATUS_NOT_AVAIL || GG_S(x) == GG_STATUS_NOT_AVAIL_DESCR)

/* GG_S_B() zajęty */
#define GG_S_B(x) (GG_S(x) == GG_STATUS_BUSY || GG_S(x) == GG_STATUS_BUSY_DESCR)

/* GG_S_I() niewidoczny */
#define GG_S_I(x) (GG_S(x) == GG_STATUS_INVISIBLE || GG_S(x) == GG_STATUS_INVISIBLE_DESCR)

/* GG_S_D() stan opisowy */
#define GG_S_D(x) (GG_S(x) == GG_STATUS_NOT_AVAIL_DESCR || GG_S(x) == GG_STATUS_AVAIL_DESCR || GG_S(x) == GG_STATUS_BUSY_DESCR || GG_S(x) == GG_STATUS_INVISIBLE_DESCR)

/* GG_S_BL() blokowany lub blokujący */
#define GG_S_BL(x) (GG_S(x) == GG_STATUS_BLOCKED)

/**
 * Zmiana statusu (pakiet \c GG_NEW_STATUS)
 */
struct gg_new_status {
	uint32_t status;			/**< Nowy status */
} GG_PACKED;

#define GG_NOTIFY_FIRST 0x000f
#define GG_NOTIFY_LAST 0x0010

#define GG_NOTIFY 0x0010

struct gg_notify {
	uint32_t uin;				/* numerek danej osoby */
	uint8_t dunno1;				/* rodzaj wpisu w liście */
} GG_PACKED;

// XXX API
#define GG_USER_OFFLINE 0x01	/* będziemy niewidoczni dla użytkownika */
#define GG_USER_NORMAL 0x03	/* zwykły użytkownik */
#define GG_USER_BLOCKED 0x04	/* zablokowany użytkownik */

#define GG_LIST_EMPTY 0x0012

#define GG_NOTIFY_REPLY 0x000c	/* tak, to samo co GG_LOGIN */

struct gg_notify_reply {
	uint32_t uin;			/* numerek */
	uint32_t status;		/* status danej osoby */
	uint32_t remote_ip;		/* adres ip delikwenta */
	uint16_t remote_port;		/* port, na którym słucha klient */
	uint32_t version;		/* wersja klienta */
	uint16_t dunno2;		/* znowu port? */
} GG_PACKED;

#define GG_NOTIFY_REPLY60 0x0011

struct gg_notify_reply60 {
	uint32_t uin;			/* numerek plus flagi w MSB */
	uint8_t status;			/* status danej osoby */
	uint32_t remote_ip;		/* adres ip delikwenta */
	uint16_t remote_port;		/* port, na którym słucha klient */
	uint8_t version;		/* wersja klienta */
	uint8_t image_size;		/* maksymalny rozmiar grafiki w KiB */
	uint8_t dunno1;			/* 0x00 */
} GG_PACKED;

#define GG_STATUS60 0x000f

struct gg_status60 {
	uint32_t uin;			/* numerek plus flagi w MSB */
	uint8_t status;			/* status danej osoby */
	uint32_t remote_ip;		/* adres ip delikwenta */
	uint16_t remote_port;		/* port, na którym słucha klient */
	uint8_t version;		/* wersja klienta */
	uint8_t image_size;		/* maksymalny rozmiar grafiki w KiB */
	uint8_t dunno1;			/* 0x00 */
} GG_PACKED;

#define GG_NOTIFY_REPLY77 0x0018

struct gg_notify_reply77 {
	uint32_t uin;			/* numerek plus flagi w MSB */
	uint8_t status;			/* status danej osoby */
	uint32_t remote_ip;		/* adres ip delikwenta */
	uint16_t remote_port;		/* port, na którym słucha klient */
	uint8_t version;		/* wersja klienta */
	uint8_t image_size;		/* maksymalny rozmiar grafiki w KiB */
	uint8_t dunno1;			/* 0x00 */
	uint32_t dunno2;		/* ? */
} GG_PACKED;

#define GG_STATUS77 0x0017

struct gg_status77 {
	uint32_t uin;			/* numerek plus flagi w MSB */
	uint8_t status;			/* status danej osoby */
	uint32_t remote_ip;		/* adres ip delikwenta */
	uint16_t remote_port;		/* port, na którym słucha klient */
	uint8_t version;		/* wersja klienta */
	uint8_t image_size;		/* maksymalny rozmiar grafiki w KiB */
	uint8_t dunno1;			/* 0x00 */
	uint32_t dunno2;		/* ? */
} GG_PACKED;

#define GG_ADD_NOTIFY 0x000d
#define GG_REMOVE_NOTIFY 0x000e

struct gg_add_remove {
	uint32_t uin;			/* numerek */
	uint8_t dunno1;			/* bitmapa */
} GG_PACKED;

#define GG_STATUS 0x0002

struct gg_status {
	uint32_t uin;			/* numerek */
	uint32_t status;		/* nowy stan */
} GG_PACKED;

#define GG_SEND_MSG 0x000b

// XXX API
#define GG_CLASS_QUEUED 0x0001
#define GG_CLASS_OFFLINE GG_CLASS_QUEUED
#define GG_CLASS_MSG 0x0004
#define GG_CLASS_CHAT 0x0008
#define GG_CLASS_CTCP 0x0010
#define GG_CLASS_ACK 0x0020
#define GG_CLASS_EXT GG_CLASS_ACK	/* kompatybilność wstecz */

#define GG_MSG_MAXSIZE 2000

struct gg_send_msg {
	uint32_t recipient;
	uint32_t seq;
	uint32_t msgclass;
} GG_PACKED;

struct gg_msg_richtext {
	uint8_t flag;
	uint16_t length;
} GG_PACKED;

struct gg_msg_richtext_format {
	uint16_t position;
	uint8_t font;
} GG_PACKED;

struct gg_msg_richtext_image {
	uint16_t unknown1;
	uint32_t size;
	uint32_t crc32;
} GG_PACKED;

#define GG_FONT_BOLD 0x01
#define GG_FONT_ITALIC 0x02
#define GG_FONT_UNDERLINE 0x04
#define GG_FONT_COLOR 0x08
#define GG_FONT_IMAGE 0x80

struct gg_msg_richtext_color {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} GG_PACKED;

struct gg_msg_recipients {
	uint8_t flag;
	uint32_t count;
} GG_PACKED;

struct gg_msg_image_request {
	uint8_t flag;
	uint32_t size;
	uint32_t crc32;
} GG_PACKED;

struct gg_msg_image_reply {
	uint8_t flag;
	uint32_t size;
	uint32_t crc32;
	/* char filename[]; */
	/* char image[]; */
} GG_PACKED;

#define GG_SEND_MSG_ACK 0x0005

// XXX API
#define GG_ACK_BLOCKED 0x0001
#define GG_ACK_DELIVERED 0x0002
#define GG_ACK_QUEUED 0x0003
#define GG_ACK_MBOXFULL 0x0004
#define GG_ACK_NOT_DELIVERED 0x0006

struct gg_send_msg_ack {
	uint32_t status;
	uint32_t recipient;
	uint32_t seq;
} GG_PACKED;

#define GG_RECV_MSG 0x000a

struct gg_recv_msg {
	uint32_t sender;
	uint32_t seq;
	uint32_t time;
	uint32_t msgclass;
} GG_PACKED;

#define GG_PING 0x0008

#define GG_PONG 0x0007

#define GG_DISCONNECTING 0x000b

#define GG_USERLIST_REQUEST 0x0016

// XXX API
#define GG_USERLIST_PUT 0x00
#define GG_USERLIST_PUT_MORE 0x01
#define GG_USERLIST_GET 0x02

struct gg_userlist_request {
	uint8_t type;
} GG_PACKED;

#define GG_USERLIST_REPLY 0x0010

// XXX API
#define GG_USERLIST_PUT_REPLY 0x00
#define GG_USERLIST_PUT_MORE_REPLY 0x02
#define GG_USERLIST_GET_REPLY 0x06
#define GG_USERLIST_GET_MORE_REPLY 0x04

struct gg_userlist_reply {
	uint8_t type;
} GG_PACKED;

struct gg_dcc_tiny_packet {
	uint8_t type;		/* rodzaj pakietu */
} GG_PACKED;

struct gg_dcc_small_packet {
	uint32_t type;		/* rodzaj pakietu */
} GG_PACKED;

struct gg_dcc_big_packet {
	uint32_t type;		/* rodzaj pakietu */
	uint32_t dunno1;		/* niewiadoma */
	uint32_t dunno2;		/* niewiadoma */
} GG_PACKED;

/*
 * póki co, nie znamy dokładnie protokołu. nie wiemy, co czemu odpowiada.
 * nazwy są niepoważne i tymczasowe.
 */
#define GG_DCC_WANT_FILE 0x0003		/* peer chce plik */
#define GG_DCC_HAVE_FILE 0x0001		/* więc mu damy */
#define GG_DCC_HAVE_FILEINFO 0x0003	/* niech ma informacje o pliku */
#define GG_DCC_GIMME_FILE 0x0006	/* peer jest pewny */
#define GG_DCC_CATCH_FILE 0x0002	/* wysyłamy plik */

#define GG_DCC_FILEATTR_READONLY 0x0020

#define GG_DCC_TIMEOUT_SEND 1800	/* 30 minut */
#define GG_DCC_TIMEOUT_GET 1800		/* 30 minut */
#define GG_DCC_TIMEOUT_FILE_ACK 300	/* 5 minut */
#define GG_DCC_TIMEOUT_VOICE_ACK 300	/* 5 minut */

#define GG_DCC7_INFO 0x1f

struct gg_dcc7_info {
	uint32_t uin;			/* numer nadawcy */
	uint32_t type;			/* sposób połączenia */
	gg_dcc7_id_t id;		/* identyfikator połączenia */
	char info[GG_DCC7_INFO_LEN];	/* informacje o połączeniu "ip port" */
} GG_PACKED;

#define GG_DCC7_NEW 0x20

struct gg_dcc7_new {
	gg_dcc7_id_t id;		/* identyfikator połączenia */
	uint32_t uin_from;		/* numer nadawcy */
	uint32_t uin_to;		/* numer odbiorcy */
	uint32_t type;			/* rodzaj transmisji */
	unsigned char filename[GG_DCC7_FILENAME_LEN];	/* nazwa pliku */
	uint32_t size;			/* rozmiar pliku */
	uint32_t dunno1;		/* 0x00000000 */
	unsigned char hash[GG_DCC7_HASH_LEN];	/* hash SHA1 */
} GG_PACKED;

#define GG_DCC7_ACCEPT 0x21

struct gg_dcc7_accept {
	uint32_t uin;			/* numer przyjmującego połączenie */
	gg_dcc7_id_t id;		/* identyfikator połączenia */
	uint32_t offset;		/* offset przy wznawianiu transmisji */
	uint32_t dunno1;		/* 0x00000000 */
} GG_PACKED;

// XXX API
#define GG_DCC7_TYPE_P2P 0x00000001	/**< Połączenie bezpośrednie */
#define GG_DCC7_TYPE_SERVER 0x00000002	/**< Połączenie przez serwer */

#define GG_DCC7_REJECT 0x22

struct gg_dcc7_reject {
	uint32_t uin;			/**< Numer odrzucającego połączenie */
	gg_dcc7_id_t id;		/**< Identyfikator połączenia */
	uint32_t reason;		/**< Powód rozłączenia */
} GG_PACKED;

// XXX API
#define GG_DCC7_REJECT_BUSY 0x00000001	/**< Połączenie bezpośrednie już trwa, nie umiem obsłużyć więcej */
#define GG_DCC7_REJECT_USER 0x00000002	/**< Użytkownik odrzucił połączenie */
#define GG_DCC7_REJECT_VERSION 0x00000006	/**< Druga strona ma wersję klienta nieobsługującą połączeń bezpośrednich tego typu */

#define GG_DCC7_ID_REQUEST 0x23

struct gg_dcc7_id_request {
	uint32_t type;			/**< Rodzaj tranmisji */
} GG_PACKED;

// XXX API
#define GG_DCC7_TYPE_VOICE 0x00000001	/**< Transmisja głosu */
#define GG_DCC7_TYPE_FILE 0x00000004	/**< transmisja pliku */

#define GG_DCC7_ID_REPLY 0x23

struct gg_dcc7_id_reply {
	uint32_t type;			/** Rodzaj transmisji */
	gg_dcc7_id_t id;		/** Przyznany identyfikator */
} GG_PACKED;

/*
#define GG_DCC7_DUNNO1 0x24

struct gg_dcc7_dunno1 {
	// XXX
} GG_PACKED;
*/

#define GG_DCC7_TIMEOUT_CONNECT 10	/* 10 sekund */
#define GG_DCC7_TIMEOUT_SEND 1800	/* 30 minut */
#define GG_DCC7_TIMEOUT_GET 1800	/* 30 minut */
#define GG_DCC7_TIMEOUT_FILE_ACK 300	/* 5 minut */
#define GG_DCC7_TIMEOUT_VOICE_ACK 300	/* 5 minut */

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus) || defined(GG_CONFIG_MIRANDA)
#ifdef _WIN32
#pragma pack(pop)
#endif
#endif

#endif /* __GG_LIBGADU_H */

/*
 * Local variables:
 * c-indentation-style: k&r
 * c-basic-offset: 8
 * indent-tabs-mode: notnil
 * End:
 *
 * vim: shiftwidth=8:
 */
