#ifndef FILERECV_H
#define FILERECV_H

#define FR_STATE_RECEIVING 0
#define FR_STATE_DONE      1
#define FR_STATE_ERROR     2

struct aim_filerecv_header {
	char  magic[4];		    /* 0 */
	short hdrlen;		    /* 4 */
	short hdrtype;		    /* 6 */
	char  bcookie[8];    	/* 8 */
	short encrypt;		    /* 16 */
	short compress;	    	/* 18 */
	short totfiles;	    	/* 20 */
	short filesleft;    	/* 22 */
	short totparts;	    	/* 24 */
	short partsleft;    	/* 26 */
	long  totsize;	    	/* 28 */
	long  size;		        /* 32 */
	long  modtime;	    	/* 36 */
	long  checksum;		    /* 40 */
	long  rfrcsum;		    /* 44 */
	long  rfsize;	    	/* 48 */
	long  cretime;	    	/* 52 */
	long  rfcsum;	    	/* 56 */
	long  nrecvd;	    	/* 60 */
	long  recvcsum;		    /* 64 */
	char  idstring[32];	    /* 68 */
	char  flags;		    /* 100 */
	char  lnameoffset;	    /* 101 */
	char  lsizeoffset;	    /* 102 */
	char  dummy[69];	    /* 103 */
	char  macfileinfo[16];	/* 172 */
	short nencode;		    /* 188 */
	short nlanguage;	    /* 190 */
	char  name[64];		    /* 192 */
				            /* 256 */
};

struct aim_filerecv_request {
	struct aim_filerecv_header hdr;
	HANDLE hContact;
	char *user;
	char UID[2048];
	char *cookie;
	char *ip;
	char *vip;
	int port;
	char *message;
	char *filename;
	int files;
	int size;
	char *savepath;
	int state;
	HANDLE s;
	int fileid;
	DWORD recvsize;
	int current;
	char *cfullname;
	long totalt;
};

void aim_filerecv_init();
void aim_filerecv_destroy();
void aim_filerecv_free_fr(struct aim_filerecv_request *fr);
void aim_filerecv_deny(char *user, char *cookie);
void aim_filerecv_accept(char *user, char *cookie);
void __cdecl aim_filerecv_thread(void *vft);

#endif
