/*
 * $Id$
 *
 * myYahoo Miranda Plugin 
 *
 * Authors: Gennady Feldman (aka Gena01) 
 *          Laurent Marechal (aka Peorth)
 *
 * This code is under GPL and is based on AIM, MSN and Miranda source code.
 * I want to thank Robert Rainwater and George Hazan for their code and support
 * and for answering some of my questions during development of this plugin.
 */
#ifndef _YAHOO_FILE_TRANSFER_H_
#define _YAHOO_FILE_TRANSFER_H_

#define FILERESUME_CANCEL	11

typedef struct {
	int id;
	char *who;
	char *msg;
	char *filename;
	char *ftoken;
	HANDLE hContact;
	int  cancel;
	char *url;
	char *savepath;
	unsigned long fsize;
	HANDLE hWaitEvent;
	DWORD action;
	int y7;
} y_filetransfer;

void YAHOO_SendFile(y_filetransfer *ft);
void YAHOO_RecvFile(y_filetransfer *ft);

void ext_yahoo_got_file(int id, const char *me, const char *who, const char *url, long expires, const char *msg, const char *fname, unsigned long fesize, const char *ft_token, int y7);

/* service functions */
int YahooFileAllow(WPARAM wParam,LPARAM lParam);
int YahooFileDeny(WPARAM wParam, LPARAM lParam);
int YahooFileResume(WPARAM wParam, LPARAM lParam);
int YahooFileCancel(WPARAM wParam, LPARAM lParam);
int YahooSendFile(WPARAM wParam, LPARAM lParam);
int YahooRecvFile(WPARAM wParam, LPARAM lParam); 

#endif
