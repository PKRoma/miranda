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
#include <time.h>
#include <sys/stat.h>

#include "yahoo.h"
#include <m_protosvc.h>
#include "file_transfer.h"

extern yahoo_local_account * ylad;

YList *file_transfers;

static y_filetransfer* new_ft(int id, HANDLE hContact, const char *who, const char *msg,  
					const char *url, const char *ft_token, int y7, YList *fs, int sending)
{
	y_filetransfer* ft;
	struct yahoo_file_info * fi;
	int i=0;
	YList *l=fs;
	

	LOG(("[new_ft]"));
	
	ft = (y_filetransfer*) malloc(sizeof(y_filetransfer));
	ft->id  = id;
	ft->who = strdup(who);
	ft->hWaitEvent = INVALID_HANDLE_VALUE;
	
	ft->hContact = hContact;
	ft->files = fs;
	
	ft->url = (url == NULL) ? NULL : strdup(url);
	ft->ftoken = (ft_token == NULL) ? NULL : strdup(ft_token);
	ft->msg = (msg != NULL) ? strdup(msg) : strdup("[no description given]");
	
	ft->cancel = 0;
	ft->y7 = y7;
	ft->savepath = NULL;
	
    ft->hWaitEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	
	ZeroMemory(&ft->pfts, sizeof(PROTOFILETRANSFERSTATUS));
	ft->pfts.cbSize = sizeof(PROTOFILETRANSFERSTATUS);
	ft->pfts.hContact = hContact;
	ft->pfts.sending = sending;
	ft->pfts.workingDir = NULL;
	ft->pfts.currentFileTime = 0;
	
	ft->pfts.totalFiles = y_list_length(fs);
	
	ft->pfts.files = (char**) malloc(sizeof(char *) * ft->pfts.totalFiles);
	ft->pfts.totalBytes = 0;
	
	while(l) {
		fi = l->data;
		
		ft->pfts.files[i++] = strdup(fi->filename);
		ft->pfts.totalBytes += fi->filesize;
		
		l=l->next;
	}
	
	ft->pfts.currentFileNumber = 0;
	
	fi = fs->data; 
	ft->pfts.currentFile = strdup(fi->filename);
	ft->pfts.currentFileSize = fi->filesize; 
	
	file_transfers = y_list_prepend(file_transfers, ft);
	
	return ft;
}

y_filetransfer* find_ft(const char *ft_token) 
{
	YList *l;
	y_filetransfer* f;
	LOG(("[find_ft] Searching for: %s", ft_token));
	
	for(l = file_transfers; l; l = y_list_next(l)) {
		f = (y_filetransfer* )l->data;
		if (lstrcmp(f->ftoken, ft_token) == 0) {
			LOG(("[find_ft] Got it!"));
			return f;
		}
	}

	LOG(("[find_ft] FT not found?"));
	return NULL;
}

static void free_ft(y_filetransfer* ft)
{
	YList *l;
	int i;
	
	LOG(("[free_ft]"));
	for(l = file_transfers; l; l = y_list_next(l)) {
		if (l->data == ft){
			LOG(("[free_ft] Ft found and removed from the list"));
			file_transfers = y_list_remove_link(file_transfers, l);
			y_list_free_1(l);
			break;
		}
	}

	if ( ft->hWaitEvent != INVALID_HANDLE_VALUE )
		CloseHandle( ft->hWaitEvent );
	
	FREE(ft->who);
	FREE(ft->msg);
	FREE(ft->url);
	FREE(ft->savepath);
	FREE(ft->ftoken);
	
	while(ft->files) {
		YList *tmp = ft->files;
		struct yahoo_file_info * c = ft->files->data;
		FREE(c->filename);
		FREE(c);
		ft->files = y_list_remove_link(ft->files, ft->files);
		y_list_free_1(tmp);
	}
	
	for (i=0; i< ft->pfts.totalFiles; i++)
		free(ft->pfts.files[i]);
	
	FREE(ft->pfts.currentFile);
	FREE(ft->pfts.files);
	FREE(ft);
	
}

static void upload_file(int id, int fd, int error, void *data) 
{
    y_filetransfer *sf = (y_filetransfer*) data;
	struct yahoo_file_info *fi = (struct yahoo_file_info *)sf->files->data;
    char buf[1024];
    long size = 0;
	DWORD dw = 0;
	int   rw = 0;
	struct _stat statbuf;
	
	if (fd < 0) {
		LOG(("[get_fd] Connect Failed!"));
		error = 1;
	}

	if (_stat( fi->filename, &statbuf ) != 0 )
		error = 1;
	
    if(!error) {
		 HANDLE myhFile    = CreateFile(fi->filename,
                                   GENERIC_READ,
                                   FILE_SHARE_READ|FILE_SHARE_WRITE,
			           NULL,
			           OPEN_EXISTING,
			           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
			           0);


		if(myhFile !=INVALID_HANDLE_VALUE) {
			DWORD lNotify = GetTickCount();
			
			LOG(("proto: %s, hContact: %p", yahooProtocolName, sf->hContact));
			
			LOG(("Sending file: %s", fi->filename));
			//ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTING, sf, 0);
			//ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_NEXTFILE, sf, 0);
			ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTED, sf, 0);
			//ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_SENTREQUEST, sf, 0);
			//ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, sf, 0);
						
			do {
				ReadFile(myhFile, buf, sizeof(buf), &dw, NULL);
			
				if (dw) {
					rw = Netlib_Send((HANDLE)fd, buf, dw, MSG_NODUMP);
					
					if (rw < 1) {
						LOG(("Upload Failed. Send error? Got: %d", rw));
						error = 1;
						break;
					} else 
						size += rw;
					
					if(GetTickCount() >= lNotify + 500 || rw < 1024 || size == statbuf.st_size) {
						LOG(("DOING UI Notify. Got %lu/%lu", size, statbuf.st_size));
						sf->pfts.totalProgress = size;
						sf->pfts.currentFileProgress = size;
						
						ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_DATA, sf, (LPARAM) & sf->pfts);
						lNotify = GetTickCount();
					}
					
				}
				
				if (sf->cancel) {
					LOG(("Upload Cancelled! "));
					error = 1;
					break;
				}
			} while ( rw > 0 && dw > 0 && !error);
			
			CloseHandle(myhFile);

			sf->pfts.totalProgress = size;
			sf->pfts.currentFileProgress = size;
			
			ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_DATA, sf, (LPARAM) & sf->pfts);

		}
    }	

	if (fd > 0) {
		int tr = 0;
		
		do {
			rw = Netlib_Recv((HANDLE)fd, buf, sizeof(buf), 0);
			LOG(("Got: %d bytes", rw));
			
			if (tr == 0) {
				//"HTTP/1.1 999" 12
				// 012345678901
				if (rw > 12) {
					if (buf[9] != '2' || buf[10] != '0' || buf[11] != '0') {
						LOG(("File Transfer Failed: %c%c%c", buf[9], buf[10], buf[11]));
						error=1;
					}
				}
			}
			tr +=rw;
		} while (rw > 0);
	
		Netlib_CloseHandle((HANDLE)fd);
	}
	
	LOG(("File send complete!"));

	if (! error) {
		sf->pfts.currentFileNumber++;
		
		if (sf->pfts.currentFileNumber >= sf->pfts.totalFiles) {
			ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, sf, 0);
		} else {
			LOG(("Waiting for next file request packet..."));
		}
		
	} else {
		ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, sf, 0);
	}
}

static void dl_file(int id, int fd, int error,	const char *filename, unsigned long size, void *data) 
{
    y_filetransfer *sf = (y_filetransfer*) data;
	struct yahoo_file_info *fi = (struct yahoo_file_info *)sf->files->data;
    char buf[1024];
    unsigned long rsize = 0;
	DWORD dw, c;

	if (fd < 0) {
		LOG(("[get_url] Connect Failed!"));
		
		if (sf->ftoken != NULL) {
			LOG(("[get_url] DC Detected: asking sender to upload to Yahoo FileServers!"));
			yahoo_ftdc_deny(ylad->id, sf->who, fi->filename, sf->ftoken, 3);	
		}
		
		error = 1;
	}
	
    if(!error) {
		HANDLE myhFile;

		sf->pfts.workingDir = sf->savepath;//ft->savepath;
			
		LOG(("dir: %s, file: %s", sf->savepath, fi->filename ));
		wsprintf(buf, "%s\\%s", sf->savepath, fi->filename);
		
		/*
		 * We need FULL Path for File Resume to work properly!!!
		 *
		 * Don't rely on workingDir to be right, since it's not used to check if file exists.
		 */
		FREE(sf->pfts.currentFile);
		sf->pfts.currentFile = strdup(buf);		
		LOG(("Saving: %s",  sf->pfts.currentFile));
		
		ResetEvent(sf->hWaitEvent);
		
		if ( YAHOO_SendBroadcast( sf->hContact, ACKTYPE_FILE, ACKRESULT_FILERESUME, sf, ( LPARAM )&sf->pfts )) {
			WaitForSingleObject( sf->hWaitEvent, INFINITE );
			
			switch(sf->action){
				case FILERESUME_RENAME:
				case FILERESUME_OVERWRITE:	
				case FILERESUME_RESUME:	
					// no action needed at this point, just break out of the switch statement
					break;

				case FILERESUME_CANCEL	:
					sf->cancel = 1;
					break;

				case FILERESUME_SKIP	:
				default:
					//delete this; // per usual dcc objects destroy themselves when they fail or when connection is closed
					//return FALSE; 
					sf->cancel = 2;
					break;
				}
		}

		
		
		if (! sf->cancel) {
			
			if (sf->action != FILERESUME_RENAME ) {
				LOG(("dir: %s, file: %s", sf->savepath, fi->filename ));
			
				wsprintf(buf, "%s\\%s", sf->savepath, fi->filename);
			} else {
				LOG(("file: %s", fi->filename ));
				//wsprintf(buf, "%s\%s", sf->filename);
				
				free(sf->pfts.currentFile);
				lstrcpy(buf, fi->filename);
				sf->pfts.currentFile = strdup(buf);
			}
			
			LOG(("Getting file: %s", buf));
			myhFile    = CreateFile(buf,
									GENERIC_WRITE,
									FILE_SHARE_WRITE,
									NULL, OPEN_ALWAYS,  FILE_ATTRIBUTE_NORMAL,  0);
	
			if(myhFile !=INVALID_HANDLE_VALUE) {
				DWORD lNotify = GetTickCount();
				
				SetEndOfFile(myhFile);
				
				LOG(("proto: %s, hContact: %p", yahooProtocolName, sf->hContact));
				
				ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTED, sf, 0);
				
				do {
					dw = Netlib_Recv((HANDLE)fd, buf, 1024, MSG_NODUMP);
				
					if (dw > 0) {
						WriteFile(myhFile, buf, dw, &c, NULL);
						rsize += dw;
						
						/*LOG(("Got %d/%d", rsize, size));*/
						if(GetTickCount() >= lNotify + 500 || dw <= 0 || rsize == size) {
							
						LOG(("DOING UI Notify. Got %lu/%lu", rsize, size));
						
						sf->pfts.currentFileTime = (DWORD)time(NULL);//ntohl(ft->hdr.modtime);
						sf->pfts.currentFileProgress = rsize;
						
						ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_DATA, sf, (LPARAM) & sf->pfts);
						lNotify = GetTickCount();
						}
					} else {
						LOG(("Recv Failed! Socket Error?"));
						error = 1;
						break;
					}
					
					if (sf->cancel) {
						LOG(("Recv Cancelled! "));
						error = 1;
						break;
					}
				} while ( dw > 0 && rsize < size);
				
				while (dw > 0) {
					dw = Netlib_Recv((HANDLE)fd, buf, 1024, MSG_NODUMP);
					LOG(("Ack."));
				}
				
				sf->pfts.totalProgress += rsize;
				ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_DATA, sf, (LPARAM) & sf->pfts);
				
				LOG(("[Finished DL] Got %lu/%lu", rsize, size));
				CloseHandle(myhFile);
				
			} else {
				LOG(("Can not open file for writing: %s", buf));
				error = 1;
			}
			
			//free(sf->pfts.currentFile);
		} 
    }
	
	if (fd > 0) {
		LOG(("Closing connection: %d", fd));
		Netlib_CloseHandle((HANDLE)fd);
	
		if (sf->cancel || error) {
			/* abort FT transfer */
			yahoo_ft7dc_abort(ylad->id, sf->who, sf->ftoken);
		}
	}
	
    if (! error) {
		sf->pfts.currentFileNumber++;
		
		LOG(("File %d/%d download complete!", sf->pfts.currentFileNumber, sf->pfts.totalFiles));
		
		if (sf->pfts.currentFileNumber >= sf->pfts.totalFiles) {
			ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_SUCCESS, sf, 0);
		} else {
			YList *l;
			struct yahoo_file_info * fi;
			
			// Do Next file
			yahoo_ft7dc_nextfile(ylad->id, sf->who, sf->ftoken);
			free(sf->pfts.currentFile);
			
			l = sf->files;
			
			fi= sf->files->data;
			FREE(fi->filename);
			FREE(fi);
			
			sf->files = y_list_remove_link(sf->files, sf->files);
			y_list_free_1(l);
			
			// need to move to the next file on the list and fill the file information
			fi = sf->files->data; 
			sf->pfts.currentFile = strdup(fi->filename);
			sf->pfts.currentFileSize = fi->filesize; 
			sf->pfts.currentFileProgress = 0;
			
			ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_NEXTFILE, sf, 0);
		}
	} else {
		LOG(("File download failed!"));
		
		ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, sf, 0);
	}
}

//=======================================================
//File Transfer
//=======================================================
static void __cdecl yahoo_recv_filethread(void *psf) 
{
	y_filetransfer *sf = psf;
	struct yahoo_file_info *fi = (struct yahoo_file_info *)sf->files->data;
	
//    ProtoBroadcastAck(yahooProtocolName, hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
	ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTING, sf, 0);
	
	YAHOO_DebugLog("[yahoo_recv_filethread] who: %s, msg: %s, filename: %s ", sf->who, sf->msg, fi->filename);
	
	
	yahoo_get_url_handle(ylad->id, sf->url, &dl_file, sf);
	
	if (sf->pfts.currentFileNumber >= sf->pfts.totalFiles) {
		free_ft(sf);
	} else {
		YAHOO_DebugLog("[yahoo_recv_filethread] More files coming?");
	}
}

void ext_yahoo_got_file(int id, const char *me, const char *who, const char *url, long expires, const char *msg, const char *fname, unsigned long fesize, const char *ft_token, int y7)
{
    CCSDATA ccs;
    PROTORECVEVENT pre;
	HANDLE hContact;
	char *szBlob;
	y_filetransfer *ft;
	char fn[1024];
	struct yahoo_file_info *fi;
	YList *files=NULL;
	
    LOG(("[ext_yahoo_got_file] id: %i, ident:%s, who: %s, url: %s, expires: %lu, msg: %s, fname: %s, fsize: %lu ftoken: %s y7: %d", id, me, who, url, expires, msg, fname, fesize, ft_token == NULL ? "NULL" : ft_token, y7));
	
	hContact = getbuddyH(who);
	if (hContact == NULL) 
		hContact = add_buddy(who, who, 0 /* NO FT for other IMs */, PALF_TEMPORARY);
	
	ZeroMemory(fn, 1024);
	
	if (fname != NULL)
		lstrcpyn(fn, fname, 1024);
	else {
		char *start, *end;
		
		/* based on how gaim does this */
		start = strrchr(url, '/');
		if (start)
			start++;
		
		end = strrchr(url, '?');
		
		if (start && *start && end) {
			lstrcpyn(fn, start, end-start+1);
		} else 
			lstrcpy(fn, "filename.ext");
	}

	fi = y_new(struct yahoo_file_info,1);
	fi->filename = fn;
	fi->filesize = fesize;
	
	files = y_list_append(files, fi);
	
	ft = new_ft(id, hContact, who, msg,	url, ft_token, y7, files, 0 /* downloading */);
	if (ft == NULL) {
		YAHOO_DebugLog("SF IS NULL!!!");
		return;
	}

    // blob is DWORD(*ft), ASCIIZ(filenames), ASCIIZ(description)
    szBlob = (char *) malloc(sizeof(DWORD) + lstrlen(fn) + lstrlen(msg) + 2);
    *((PDWORD) szBlob) = (DWORD) ft;
    strcpy(szBlob + sizeof(DWORD), fn);
    strcpy(szBlob + sizeof(DWORD) + lstrlen(fn) + 1, msg);

	pre.flags = 0;
	pre.timestamp = (DWORD)time(NULL);
    pre.szMessage = szBlob;
    pre.lParam = 0;
    ccs.szProtoService = PSR_FILE;
    ccs.hContact = hContact;
    ccs.wParam = 0;
    ccs.lParam = (LPARAM) & pre;
    CallService(MS_PROTO_CHAINRECV, 0, (LPARAM) & ccs);
	free(szBlob);
}

void ext_yahoo_got_files(int id, const char *me, const char *who, const char *ft_token, int y7, YList* files)
{
    CCSDATA ccs;
    PROTORECVEVENT pre;
	HANDLE hContact;
	char *szBlob;
	y_filetransfer *ft;
	YList *f;
	char fn[4096];
	int fc = 0;
	
    LOG(("[ext_yahoo_got_files] id: %i, ident:%s, who: %s, ftoken: %s ", id, me, who, ft_token == NULL ? "NULL" : ft_token));
	
	hContact = getbuddyH(who);
	if (hContact == NULL) 
		hContact = add_buddy(who, who, 0 /* NO FT for other IMs */, PALF_TEMPORARY);
	
	ft = new_ft(id, hContact, who, NULL, NULL, ft_token, y7, files, 0 /* downloading */);
	if (ft == NULL) {
		YAHOO_DebugLog("SF IS NULL!!!");
		return;
	}

	fn[0] = '\0';
	for (f=files; f; f = y_list_next(f)) {
		char z[1024];
		struct yahoo_file_info *fi = (struct yahoo_file_info *) f->data;
		
		snprintf(z, 1024, "%s (%lu)\r\n", fi->filename, fi->filesize);
		lstrcat(fn, z);
		fc++;
	}
	
	if (fc > 1) {
		/* multiple files */
		
	}
	
    // blob is DWORD(*ft), ASCIIZ(filenames), ASCIIZ(description)
    szBlob = (char *) malloc(sizeof(DWORD) + lstrlen(fn) + 2);
    *((PDWORD) szBlob) = (DWORD) ft;
    strcpy(szBlob + sizeof(DWORD), fn);
    strcpy(szBlob + sizeof(DWORD) + lstrlen(fn) + 1, "");

	pre.flags = 0;
	pre.timestamp = (DWORD)time(NULL);
    pre.szMessage = szBlob;
    pre.lParam = 0;
    ccs.szProtoService = PSR_FILE;
    ccs.hContact = ft->hContact;
    ccs.wParam = 0;
    ccs.lParam = (LPARAM) & pre;
    CallService(MS_PROTO_CHAINRECV, 0, (LPARAM) & ccs);
	free(szBlob);
}

void ext_yahoo_got_file7info(int id, const char *me, const char *who, const char *url, const char *fname, const char *ft_token)
{
	y_filetransfer *ft;
/*	NETLIBHTTPREQUEST nlhr={0},*nlhrReply;
	NETLIBHTTPHEADER httpHeaders[3];
	char  z[1024];
*/	
	LOG(("[ext_yahoo_got_file7info] id: %i, ident:%s, who: %s, url: %s, fname: %s, ft_token: %s", id, me, who, url, fname, ft_token));
	
	ft = find_ft(ft_token);
	
	if (ft == NULL) {
		LOG(("ERROR: Can't find the token: %s in my file transfers list...", ft_token));
		return;
	}
	
	ProtoBroadcastAck(yahooProtocolName, ft->hContact, ACKTYPE_GETINFO, ACKRESULT_SUCCESS, (HANDLE) 1, 0);
	
	FREE(ft->url);
	
	ft->url = strdup(url);
	
	//SleepEx(2000, TRUE); // Need to make sure our ACCEPT is delivered before we try to HEAD request
/*	
	nlhr.cbSize		= sizeof(nlhr);
	nlhr.requestType= REQUEST_HEAD;
	nlhr.flags		= NLHRF_DUMPASTEXT;
	nlhr.szUrl		= (char *)url;
	nlhr.headers = httpHeaders;
	nlhr.headersCount= 4;
	
	httpHeaders[0].szName="Cookie";
	
	mir_snprintf(z, 1024, "Y=%s; T=%s; B=%s", yahoo_get_cookie(id, "y"), yahoo_get_cookie(id, "t"), yahoo_get_cookie(id, "b"));
	httpHeaders[0].szValue=z;
	
	httpHeaders[1].szName="User-Agent";
	httpHeaders[1].szValue="Mozilla/4.0 (compatible; MSIE 5.5)";
	httpHeaders[2].szName="Content-Length";
	httpHeaders[2].szValue="0";
	httpHeaders[3].szName="Cache-Control";
	httpHeaders[3].szValue="no-cache";
	
	nlhrReply=(NETLIBHTTPREQUEST*)CallService(MS_NETLIB_HTTPTRANSACTION,(WPARAM)hNetlibUser,(LPARAM)&nlhr);

	if(nlhrReply) {
		int i;
		
		LOG(("Update server returned '%d'. It also sent the following: %s", nlhrReply->resultCode, nlhrReply->szResultDescr));
		LOG(("Got %d bytes.", nlhrReply->dataLength));
		LOG(("Got %d headers!", nlhrReply->headersCount));
		
		for (i=0; i < nlhrReply->headersCount; i++) {
			LOG(("%s: %s", nlhrReply->headers[i].szName, nlhrReply->headers[i].szValue));
		}
		
		CallService(MS_NETLIB_FREEHTTPREQUESTSTRUCT,0,(LPARAM)nlhrReply);
		
		
	} else {
		LOG(("ERROR: No Reply???"));
	}
	*/
	
	mir_forkthread(yahoo_recv_filethread, (void *) ft);
}

void ext_yahoo_send_file7info(int id, const char *me, const char *who, const char *ft_token)
{
	y_filetransfer *ft;
	char *c;
	LOG(("[ext_yahoo_send_file7info] id: %i, ident:%s, who: %s, ft_token: %s", id, me, who, ft_token));
	
	ft = find_ft(ft_token);
	
	if (ft == NULL) {
		LOG(("ERROR: Can't find the token: %s in my file transfers list...", ft_token));
		return;
	}
	
	c = strrchr(ft->pfts.files[0], '\\');
	if (c != NULL ) {
		c++;
	} else {
		c = ft->pfts.files[0];
	}
	
	yahoo_send_file7info(id, me, who, ft_token, c, /*relay_ip*/ "98.136.112.33");
		
}

void ext_yahoo_ft7_send_file(int id, const char *me, const char *who, const char *filename, const char *token, const char *ft_token)
{
	y_filetransfer *sf;
	struct yahoo_file_info *fi;
	
	LOG(("[ext_yahoo_send_file7info] id: %i, ident:%s, who: %s, ft_token: %s", id, me, who, ft_token));
	
	sf = find_ft(ft_token);
	
	if (sf == NULL) {
		LOG(("ERROR: Can't find the token: %s in my file transfers list...", ft_token));
		return;
	}
	
	fi = (struct yahoo_file_info *)sf->files->data;
	
	ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTING, sf, 0);
	
	LOG(("who %s, msg: %s, filename: %s filesize: %ld", sf->who, sf->msg, fi->filename, fi->filesize));
	
	yahoo_send_file_y7(id, me, who, "98.136.112.33", fi->filesize, token,  &upload_file, sf);
	
	if (sf->pfts.currentFileNumber >= sf->pfts.totalFiles) {
		free_ft(sf);
	} else {
		YAHOO_DebugLog("[yahoo_send_filethread] More files coming?");
	}

}


/**************** Receive File ********************/
int YahooFileAllow(WPARAM wParam,LPARAM lParam) 
{
    CCSDATA *ccs = (CCSDATA *) lParam;
    y_filetransfer *ft = (y_filetransfer *) ccs->wParam;
	int len;
	
	YAHOO_DebugLog("[YahooFileAllow]");
	
    //LOG(LOG_INFO, "[%s] Requesting file from %s", ft->cookie, ft->user);
    ft->savepath = strdup((char *) ccs->lParam);
	
	len = lstrlen(ft->savepath) - 1;
	if (ft->savepath[len] == '\\')
		ft->savepath[len] = '\0';

	if (ft->y7) {
		YAHOO_DebugLog("[YahooFileAllow] We don't handle y7 stuff yet.");
		//void yahoo_ft7dc_accept(int id, const char *buddy, const char *ft_token);
		yahoo_ft7dc_accept(ft->id, ft->who, ft->ftoken);

		return ccs->wParam;
	}
	
    mir_forkthread(yahoo_recv_filethread, (void *) ft);
	
    return ccs->wParam;
}

int YahooFileDeny(WPARAM wParam,LPARAM lParam) 
{
	/* deny file receive request.. just ignore it! */
	CCSDATA *ccs = (CCSDATA *) lParam;
    y_filetransfer *ft = (y_filetransfer *) ccs->wParam;
	
	YAHOO_DebugLog("[YahooFileDeny]");
	
	if ( !yahooLoggedIn || ft == NULL ) {
		YAHOO_DebugLog("[YahooFileDeny] Not logged-in or some other error!");
		return 1;
	}

	if (ft->y7) {
		YAHOO_DebugLog("[YahooFileDeny] We don't handle y7 stuff yet.");
		//void yahoo_ft7dc_accept(int id, const char *buddy, const char *ft_token);
		yahoo_ft7dc_deny(ft->id, ft->who, ft->ftoken);
		return 0;
	}

	if (ft->ftoken != NULL) {
		struct yahoo_file_info *fi = (struct yahoo_file_info *)ft->files->data;
		
		YAHOO_DebugLog("[] DC Detected: Denying File Transfer!");
		yahoo_ftdc_deny(ylad->id, ft->who, fi->filename, ft->ftoken, 2);	
	}
	return 0;
}

int YahooFileResume( WPARAM wParam, LPARAM lParam )
{
    PROTOFILERESUME *pfr;
	y_filetransfer *ft = (y_filetransfer *) wParam;
	
	YAHOO_DebugLog("[YahooFileResume]");
	
	if ( !yahooLoggedIn || ft == NULL ) {
		YAHOO_DebugLog("[YahooFileResume] Not loggedin or some other error!");
		return 1;
	}

	pfr = (PROTOFILERESUME*)lParam;
	
	ft->action = pfr->action;
	
	YAHOO_DebugLog("[YahooFileResume] Action: %d", pfr->action);
	
	if ( pfr->action == FILERESUME_RENAME ) {
		struct yahoo_file_info *fi = (struct yahoo_file_info *)ft->files->data;
		
		YAHOO_DebugLog("[YahooFileResume] Renamed file!");
		FREE( fi->filename );

		fi->filename = strdup( pfr->szFilename );
	}	
	

	SetEvent( ft->hWaitEvent );
	return 0;
}

/**************** Send File ********************/
static void __cdecl yahoo_send_filethread(void *psf) 
{
	y_filetransfer *sf = psf;
	struct yahoo_file_info *fi = (struct yahoo_file_info *)sf->files->data;
	
	ProtoBroadcastAck(yahooProtocolName, sf->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTING, sf, 0);
	
	LOG(("who %s, msg: %s, filename: %s filesize: %ld", sf->who, sf->msg, fi->filename, fi->filesize));
	
	yahoo_send_file(ylad->id, sf->who, sf->msg, fi->filename, fi->filesize, &upload_file, sf);
	
	if (sf->pfts.currentFileNumber >= sf->pfts.totalFiles) {
		free_ft(sf);
	} else {
		YAHOO_DebugLog("[yahoo_send_filethread] More files coming?");
	}

}

int YahooFileCancel(WPARAM wParam,LPARAM lParam) 
{
	CCSDATA* ccs = ( CCSDATA* )lParam;
	y_filetransfer* ft = (y_filetransfer*)ccs->wParam;
	
	YAHOO_DebugLog("[YahooFileCancel]");
	
	if ( ft->hWaitEvent != INVALID_HANDLE_VALUE )
		SetEvent( ft->hWaitEvent );
	
	ft->action = FILERESUME_CANCEL;
	ft->cancel = 1;
	return 0;
}

/*
 *
 */
int YahooSendFile(WPARAM wParam,LPARAM lParam) 
{
	DBVARIANT dbv;
	y_filetransfer *sf;
	CCSDATA *ccs;
	char** files;
	
	LOG(("[YahooSendFile]"));
	
	if ( !yahooLoggedIn )
		return 0;

	YAHOO_DebugLog("Gathering Data");
	
	ccs = ( CCSDATA* )lParam;
	//if ( YAHOO_GetWord( ccs->hContact, "Status", ID_STATUS_OFFLINE ) == ID_STATUS_OFFLINE )
	//	return 0;

	YAHOO_DebugLog("Getting Files");
	
	files = ( char** )ccs->lParam;
	if ( files[1] != NULL ){
		MessageBox(NULL, "YAHOO protocol allows only one file to be sent at a time", "Yahoo", MB_OK | MB_ICONINFORMATION);
		return 0;
 	}
	
	YAHOO_DebugLog("Getting Yahoo ID");
	
	if (!DBGetContactSettingString(ccs->hContact, yahooProtocolName, YAHOO_LOGINID, &dbv)) {
		long tFileSize = 0;
		struct _stat statbuf;
		struct yahoo_file_info *fi;
		YList *fs=NULL;
	
		if ( _stat( files[0], &statbuf ) == 0 )
			tFileSize = statbuf.st_size;

		fi = y_new(struct yahoo_file_info,1);
		fi->filename = files[0];
		fi->filesize = tFileSize;
	
		fs = y_list_append(fs, fi);
	
		sf = new_ft(ylad->id, ccs->hContact, dbv.pszVal, ( char* )ccs->wParam,
					NULL, NULL, 0, fs, 1 /* sending */);
					
		DBFreeVariant(&dbv);
		
		if (sf == NULL) {
			YAHOO_DebugLog("SF IS NULL!!!");
			return 0;
		}

		LOG(("who: %s, msg: %s, filename: %s", sf->who, sf->msg, fi->filename));
		//mir_forkthread(yahoo_send_filethread, sf);
		
		sf->ftoken=yahoo_ft7dc_send(ylad->id, sf->who, fs);
		
		LOG(("Exiting SendRequest..."));
		return (int)(HANDLE)sf;
	}
	
	LOG(("Exiting SendFile"));
	return 0;
}

int YahooRecvFile(WPARAM wParam,LPARAM lParam) 
{
    DBEVENTINFO dbei;
    CCSDATA *ccs = (CCSDATA *) lParam;
    PROTORECVEVENT *pre = (PROTORECVEVENT *) ccs->lParam;
    char *szDesc, *szFile;

    DBDeleteContactSetting(ccs->hContact, "CList", "Hidden");
    szFile = pre->szMessage + sizeof(DWORD);
    szDesc = szFile + lstrlen(szFile) + 1;
    ZeroMemory(&dbei, sizeof(dbei));
    dbei.cbSize = sizeof(dbei);
    dbei.szModule = yahooProtocolName;
    dbei.timestamp = pre->timestamp;
    dbei.flags = pre->flags & (PREF_CREATEREAD ? DBEF_READ : 0);
    dbei.eventType = EVENTTYPE_FILE;
    dbei.cbBlob = sizeof(DWORD) + lstrlen(szFile) + lstrlen(szDesc) + 2;
    dbei.pBlob = (PBYTE) pre->szMessage;
    CallService(MS_DB_EVENT_ADD, (WPARAM) ccs->hContact, (LPARAM) & dbei);
    return 0;
}



