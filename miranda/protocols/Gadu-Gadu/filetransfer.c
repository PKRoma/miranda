////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2003-2006 Adam Strzelecki <ono+miranda@java.pl>
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
#include <errno.h>
#include <io.h>
#include <fcntl.h>

////////////////////////////////////////////////////////////
// Stop dcc
void gg_dccwait(GGPROTO *gg)
{
	// Wait for DCC socket service to die
	DWORD exitCode = 0;
	GetExitCodeThread(gg->pth_dcc.hThread, &exitCode);
	// Wait for main connection server to die
	if (GetCurrentThreadId() != gg->pth_dcc.dwThreadId && exitCode == STILL_ACTIVE)
	{
#ifdef DEBUGMODE
		gg_netlog("gg_dccstop(): Waiting until gg_dccmainthread() finished.");
#endif
		while (WaitForSingleObjectEx(gg->pth_dcc.hThread, INFINITE, TRUE) != WAIT_OBJECT_0);
	}
	pthread_detach(&gg->pth_dcc);
}

void *__stdcall gg_dccmainthread(void *empty);

void gg_dccstart(GGPROTO *gg)
{
	DWORD exitCode = 0;

	if(gg->dcc) return;

	// Startup dcc thread
	GetExitCodeThread(gg->pth_dcc.hThread, &exitCode);
	// Check if dcc thread isn't running already
	if(exitCode == STILL_ACTIVE)
	{
#ifdef DEBUGMODE
		gg_netlog("gg_dccstart(): DCC thread still active. Exiting...");
#endif
		// Signalize mainthread it's started
		if(gg->event) SetEvent(gg->event);
		return;
	}

	// Check if we wan't direct connections
	if(!DBGetContactSettingByte(NULL, GG_PROTO, GG_KEY_DIRECTCONNS, GG_KEYDEF_DIRECTCONNS))
	{
#ifdef DEBUGMODE
		gg_netlog("gg_dccstart(): No direct connections setup.");
#endif
		if(gg->event) SetEvent(gg->event);
		return;
	}

	// Start thread
	pthread_create(&gg->pth_dcc, NULL, gg_dccmainthread, gg);
}

void gg_dccconnect(GGPROTO *gg, uin_t uin)
{
	struct gg_dcc *dcc;
	HANDLE hContact = gg_getcontact(gg, uin, 0, 0, NULL);
	DWORD ip, myuin; WORD port;

#ifdef DEBUGMODE
	gg_netlog("gg_dccconnect(): Connecting to uin %d.", uin);
#endif

	// If unknown user or not on list ignore
	if(!hContact) return;

	// Read user IP and port
	ip = swap32(DBGetContactSettingDword(hContact, GG_PROTO, GG_KEY_CLIENTIP, 0));
	port = DBGetContactSettingWord(hContact, GG_PROTO, GG_KEY_CLIENTPORT, 0);
	myuin = DBGetContactSettingDword(NULL, GG_PROTO, GG_KEY_UIN, 0);

	// If not port nor ip nor my uin (?) specified
	if(!ip || !port || !uin) return;

	if(!(dcc = gg_dcc_get_file(ip, port, myuin, uin)))
		return;

	// Add client dcc to watches
	pthread_mutex_lock(&gg->ft_mutex);
	list_add(&gg->watches, dcc, 0);
	pthread_mutex_unlock(&gg->ft_mutex);
}

//////////////////////////////////////////////////////////
// THREAD: File transfer fail
struct ftfaildata
{
	HANDLE hContact;
	HANDLE hProcess;
	GGPROTO *gg;
};
static void *__stdcall gg_ftfailthread(void *empty)
{
	struct ftfaildata *ft = (struct ftfaildata *)empty;
	const GGPROTO *gg = ft->gg;
	SleepEx(100, FALSE);
#ifdef DEBUGMODE
	gg_netlog("gg_ftfailthread(): Sending failed file transfer.");
#endif
	ProtoBroadcastAck(GG_PROTO, ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft->hProcess, 0);
	free(ft);
	return NULL;
}
int ftfail(GGPROTO *gg, HANDLE hContact)
{
	pthread_t tid;
	struct ftfaildata *ft = malloc(sizeof(struct ftfaildata));
#ifdef DEBUGMODE
	gg_netlog("gg_ftfail(): Failing file transfer...");
#endif
	srand(time(NULL));
	ft->hProcess = (HANDLE)rand();
	ft->hContact = hContact;
	ft->gg = gg;
	pthread_create(&tid, NULL, gg_ftfailthread, ft);
	pthread_detach(&tid);
	return (int)ft->hProcess;
}

////////////////////////////////////////////////////////////
// Main DCC connection session thread

// Info refresh min time (msec) / half-sec
#define GGSTATREFRESHEVERY	500

void *__stdcall gg_dccmainthread(void *empty)
{
	uin_t uin;
	struct gg_event *e;
	struct timeval tv;
	fd_set rd, wd;
	int ret, maxfd;
	DWORD tick;
	list_t l;
	char filename[MAX_PATH];
	GGPROTO *gg = empty;

	// Zero up lists
	gg->watches = gg->transfers = gg->requests = l = NULL;

#ifdef DEBUGMODE
	gg_netlog("gg_dccmainthread(): DCC Server Thread Starting");
#endif

	// Readup number
	if (!(uin = DBGetContactSettingDword(NULL, GG_PROTO, GG_KEY_UIN, 0)))
	{
#ifdef DEBUGMODE
		gg_netlog("gg_dccmainthread(): No Gadu-Gadu number specified. Exiting.");
#endif
		if(gg->event) SetEvent(gg->event);
		return NULL;
	}

	// Create listen socket on config direct port
	if(!(gg->dcc = gg_dcc_socket_create(uin, DBGetContactSettingWord(NULL, GG_PROTO, GG_KEY_DIRECTPORT, GG_KEYDEF_DIRECTPORT))))
	{
#ifdef DEBUGMODE
		gg_netlog("gg_dccmainthread(): Cannot create DCC listen socket. Exiting.");
#endif
		// Signalize mainthread we haven't start
		if(gg->event) SetEvent(gg->event);
		return NULL;
	}

	gg_dcc_port = gg->dcc->port;
	gg_dcc_ip = inet_addr("255.255.255.255");
#ifdef DEBUGMODE
	gg_netlog("gg_dccmainthread(): Listening on port %d.", gg_dcc_port);
#endif

	// Signalize mainthread we started
	if(gg->event) SetEvent(gg->event);

	// Add main dcc handler to watches
	list_add(&gg->watches, gg->dcc, 0);

	// Do while we are in the main server thread
	while(gg->pth_sess.dwThreadId && gg->dcc)
	{
		// Timeouts
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		// Prepare descriptiors for select
		FD_ZERO(&rd);
		FD_ZERO(&wd);

		for (maxfd = 0, l = gg->watches; l; l = l->next)
		{
			struct gg_dcc *w = l->data;

			if (!w || w->state == GG_STATE_ERROR || w->state == GG_STATE_IDLE || w->state == GG_STATE_DONE)
				continue;

			// Check if it's proper descriptor
			if (w->fd == -1) continue;

			if (w->fd > maxfd)
				maxfd = w->fd;
			if ((w->check & GG_CHECK_READ))
				FD_SET(w->fd, &rd);
			if ((w->check & GG_CHECK_WRITE))
				FD_SET(w->fd, &wd);
		}

		// Wait for data on selects
		ret = select(maxfd + 1, &rd, &wd, NULL, &tv);

		// Check for select error
		if (ret == -1)
		{
#ifdef DEBUGMODE
			if (errno == EBADF)
				gg_netlog("gg_dccmainthread(): Bad descriptor on select().");
			else if (errno != EINTR)
				gg_netlog("gg_dccmainthread(): Unknown error on select().");
#endif
			continue;
		}

		// Process watches (carefull with l)
		l = gg->watches;
		pthread_mutex_lock(&gg->ft_mutex);
		while (l)
		{
			struct gg_dcc *dcc = l->data;
			l = l->next;

			if (!dcc || (!FD_ISSET(dcc->fd, &rd) && !FD_ISSET(dcc->fd, &wd)))
				continue;

			/////////////////////////////////////////////////////////////////
			// Process DCC events

			// Connection broken/closed
			if (!(e = gg_dcc_socket_watch_fd(dcc)))
			{
#ifdef DEBUGMODE
				gg_netlog("gg_dccmainthread(): Socket closed.");
#endif
				// Remove socket and _close
				list_remove(&gg->watches, dcc, 0);
				gg_dcc_socket_free(dcc);

				// Check if it's main socket
				if(dcc == gg->dcc) dcc = NULL;
				continue;
			}
#ifdef DEBUGMODE
			else gg_netlog("gg_dccmainthread(): Event: %s", ggdebug_eventtype(e));
#endif

			switch(e->type)
			{
				// Client connected
				case GG_EVENT_DCC_NEW:
					list_add(&gg->watches, e->event.dcc_new, 0);
					e->event.dcc_new = NULL;
					break;

				//
				case GG_EVENT_NONE:
					// If transfer in progress do status
					if(dcc->file_fd != -1 && dcc->offset > 0 && (((tick = GetTickCount()) - dcc->tick) > GGSTATREFRESHEVERY))
					{
						PROTOFILETRANSFERSTATUS pfts;
						dcc->tick = tick;
						strncpy(filename, dcc->folder, sizeof(filename));
						strncat(filename, dcc->file_info.filename, sizeof(filename) - strlen(filename));
						memset(&pfts, 0, sizeof(PROTOFILETRANSFERSTATUS));
						pfts.cbSize = sizeof(PROTOFILETRANSFERSTATUS);
						pfts.hContact = (HANDLE)dcc->contact;
						pfts.sending = (dcc->type == GG_SESSION_DCC_SEND);
						pfts.files = NULL;
						pfts.totalFiles = 1;
						pfts.currentFileNumber = 0;
						pfts.totalBytes = dcc->file_info.size;
						pfts.totalProgress = dcc->offset;
						pfts.workingDir = dcc->folder;
						pfts.currentFile = filename;
						pfts.currentFileSize = dcc->file_info.size;
						pfts.currentFileProgress = dcc->offset;
						pfts.currentFileTime = 0;
						ProtoBroadcastAck(GG_PROTO, dcc->contact, ACKTYPE_FILE, ACKRESULT_DATA, dcc, (LPARAM)&pfts);
					}
					break;
				// Connection was successfuly ended
				case GG_EVENT_DCC_DONE:
#ifdef DEBUGMODE
					gg_netlog("gg_dccmainthread(): Client: %d, Transfer done ! Closing connection.", dcc->peer_uin);
#endif
					// Remove from watches
					list_remove(&gg->watches, dcc, 0);
					// Close file & success
					if(dcc->file_fd != -1)
					{
						PROTOFILETRANSFERSTATUS pfts;
						strncpy(filename, dcc->folder, sizeof(filename));
						strncat(filename, dcc->file_info.filename, sizeof(filename) - strlen(filename));
						memset(&pfts, 0, sizeof(PROTOFILETRANSFERSTATUS));
						pfts.cbSize = sizeof(PROTOFILETRANSFERSTATUS);
						pfts.hContact = (HANDLE)dcc->contact;
						pfts.sending = (dcc->type == GG_SESSION_DCC_SEND);
						pfts.files = NULL;
						pfts.totalFiles = 1;
						pfts.currentFileNumber = 0;
						pfts.totalBytes = dcc->file_info.size;
						pfts.totalProgress = dcc->file_info.size;
						pfts.workingDir = dcc->folder;
						pfts.currentFile = filename;
						pfts.currentFileSize = dcc->file_info.size;
						pfts.currentFileProgress = dcc->file_info.size;
						pfts.currentFileTime = 0;
						ProtoBroadcastAck(GG_PROTO, dcc->contact, ACKTYPE_FILE, ACKRESULT_DATA, dcc, (LPARAM)&pfts);
						_close(dcc->file_fd); dcc->file_fd = -1;
						ProtoBroadcastAck(GG_PROTO, dcc->contact, ACKTYPE_FILE, ACKRESULT_SUCCESS, dcc, 0);
					}
					// Free dcc
					gg_free_dcc(dcc); if(dcc == gg->dcc) gg->dcc = NULL;
					break;

				// Client error
				case GG_EVENT_DCC_ERROR:
#ifdef DEBUGMODE
					switch (e->event.dcc_error)
					{
						case GG_ERROR_DCC_HANDSHAKE:
							gg_netlog("gg_dccmainthread(): Client: %d, Error handshake.", dcc->peer_uin);
							break;
						case GG_ERROR_DCC_NET:
							gg_netlog("gg_dccmainthread(): Client: %d, Error network.", dcc->peer_uin);
							break;
						case GG_ERROR_DCC_REFUSED:
							gg_netlog("gg_dccmainthread(): Client: %d, Error refused.", dcc->peer_uin);
							break;
						default:
							gg_netlog("gg_dccmainthread(): Client: %d, Error unknown.", dcc->peer_uin);
					}
#endif
					// Don't do anything if it's main socket
					if(dcc == gg->dcc) break;

					// Remove from watches
					list_remove(&gg->watches, dcc, 0);

					// Close file & fail
					if(dcc->contact)
					{
						_close(dcc->file_fd); dcc->file_fd = -1;
						ProtoBroadcastAck(GG_PROTO, dcc->contact, ACKTYPE_FILE, ACKRESULT_FAILED, dcc, 0);
					}
					// Free dcc
					gg_free_dcc(dcc); if(dcc == gg->dcc) gg->dcc = NULL;
					break;

				// Need file acknowledgement
				case GG_EVENT_DCC_NEED_FILE_ACK:
#ifdef DEBUGMODE
					gg_netlog("gg_dccmainthread(): Client: %d, File ack filename \"%s\" size %d.", dcc->peer_uin,
						dcc->file_info.filename, dcc->file_info.size);
#endif
					// Do not watch for transfer until user accept it
					list_remove(&gg->watches, dcc, 0);
					// Add to waiting transfers
					list_add(&gg->transfers, dcc, 0);

					//////////////////////////////////////////////////
					// Add file recv request
					{
						CCSDATA ccs;
						PROTORECVEVENT pre;
						char *szBlob;
						char *szFilename = dcc->file_info.filename;
						char *szMsg = dcc->file_info.filename;

						// Make new ggtransfer struct
						dcc->contact = gg_getcontact(gg, dcc->peer_uin, 0, 0, NULL);
						szBlob = (char *)malloc(sizeof(DWORD) + strlen(szFilename) + strlen(szMsg) + 2);
						// Store current dcc
						*(PDWORD)szBlob = (DWORD)dcc;
						// Store filename
						strcpy(szBlob + sizeof(DWORD), szFilename);
						// Store description
						strcpy(szBlob + sizeof(DWORD) + strlen(szFilename) + 1, szMsg);
						ccs.szProtoService = PSR_FILE;
						ccs.hContact = dcc->contact;
						ccs.wParam = 0;
						ccs.lParam = (LPARAM)&pre;
						pre.flags = 0;
						pre.timestamp = time(NULL);
						pre.szMessage = szBlob;
						pre.lParam = 0;
						CallService(MS_PROTO_CHAINRECV, 0, (LPARAM)&ccs);
						free(szBlob);
					}
					break;

				// Need client accept
				case GG_EVENT_DCC_CLIENT_ACCEPT:
#ifdef DEBUGMODE
					gg_netlog("gg_dccmainthread(): Client: %d, Client accept.", dcc->peer_uin);
#endif
					// Check if user is on the list and if it is my uin
					if(gg_getcontact(gg, dcc->peer_uin, 0, 0, NULL) &&
						DBGetContactSettingDword(NULL, GG_PROTO, GG_KEY_UIN, -1) == dcc->uin)
						break;

					// Kill unauthorized dcc
					list_remove(&gg->watches, dcc, 0);
					gg_free_dcc(dcc); if(dcc == gg->dcc) gg->dcc = NULL;
					break;

				// Client connected as we wished to (callback)
				case GG_EVENT_DCC_CALLBACK:
				{
					int found = 0;

#ifdef DEBUGMODE
					gg_netlog("gg_dccmainthread(): Callback from client.", dcc->peer_uin);
#endif
					// Seek for stored callback request
					for (l = gg->requests; l; l = l->next)
					{
						struct gg_dcc *req = l->data;

						if (req && req->peer_uin == dcc->peer_uin)
						{
							gg_dcc_set_type(dcc, GG_SESSION_DCC_SEND);
							found = 1;

							// Copy data req ===> dcc
							dcc->folder = req->folder;
							dcc->contact = req->contact;
							dcc->file_fd = req->file_fd;
							memcpy(&dcc->file_info, &req->file_info, sizeof(struct gg_file_info));
							// Copy data back to dcc ===> req
							memcpy(req, dcc, sizeof(struct gg_dcc));

							// Remove request
							list_remove(&gg->requests, req, 0);
							// Remove dcc from watches
							list_remove(&gg->watches, dcc, 0);
							// Add request to watches
							list_add(&gg->watches, req, 0);
							// Free old dat
							free(dcc);

#ifdef DEBUGMODE
							gg_netlog("gg_dccmainthread(): Found stored request to client %d, filename \"%s\" size %d, folder \"%s\".",
								req->peer_uin, req->file_info.filename, req->file_info.size, req->folder);
#endif
							break;
						}
					}

					if (!found)
					{
#ifdef DEBUGMODE
						gg_netlog("gg_dccmainthread(): Unknown request to client %d.", dcc->peer_uin);
#endif
						// Kill unauthorized dcc
						list_remove(&gg->watches, dcc, 0);
						gg_free_dcc(dcc); if(dcc == gg->dcc) gg->dcc = NULL;
					}

					break;
				}
			}

			// Free event
			gg_free_event(e);
		}
		pthread_mutex_unlock(&gg->ft_mutex);
	}

	// Close all dcc client sockets
	for (l = gg->watches; l; l = l->next)
	{
		struct gg_dcc *dcc = l->data;
		if(dcc) gg_dcc_socket_free(dcc);
	}
	// Close all waiting for aknowledgle transfers
	for (l = gg->transfers; l; l = l->next)
	{
		struct gg_dcc *dcc = l->data;
		if(dcc) gg_dcc_socket_free(dcc);
	}
	// Close all waiting dcc requests
	for (l = gg->requests; l; l = l->next)
	{
		struct gg_dcc *dcc = l->data;
		if(dcc) gg_free_dcc(dcc);
	}
	list_destroy(gg->watches, 0);
	list_destroy(gg->transfers, 0);
	list_destroy(gg->requests, 0);

	gg_dcc_port = 0;
	gg_dcc_ip = 0;
#ifdef DEBUGMODE
	gg_netlog("gg_dccmainthread(): DCC Server Thread Ending");
#endif

	ZeroMemory(&gg->pth_dcc, sizeof(gg->pth_dcc));

	return NULL;
}

////////////////////////////////////////////////////////////
// Called when received an file
int gg_recvfile(PROTO_INTERFACE *proto, HANDLE hContact, PROTORECVFILE *pre)
{
	CCSDATA ccs = { hContact, PSR_FILE, 0, ( LPARAM )pre };
	return CallService( MS_PROTO_RECVFILE, 0, ( LPARAM )&ccs );
}

////////////////////////////////////////////////////////////
// Called when user sends a file
int gg_sendfile(PROTO_INTERFACE *proto, HANDLE hContact, const char* szDescription, char** files)
{
	GGPROTO *gg = (GGPROTO *) proto;
	char *bslash;
	struct gg_dcc *dcc;
	DWORD ip; WORD port;
	uin_t myuin, uin;

	// Check if main dcc thread is on
	if(!gg_isonline(gg)) return ftfail(gg, hContact);

	// Read user IP and port
	ip = swap32(DBGetContactSettingDword(hContact, GG_PROTO, GG_KEY_CLIENTIP, 0));
	port = DBGetContactSettingWord(hContact, GG_PROTO, GG_KEY_CLIENTPORT, 0);
	myuin = DBGetContactSettingDword(NULL, GG_PROTO, GG_KEY_UIN, 0);
	uin = DBGetContactSettingDword(hContact, GG_PROTO, GG_KEY_UIN, 0);

	// Return if bad connection info
	if(!port || !uin || !myuin)
	{
#ifdef DEBUGMODE
		gg_netlog("gg_sendfile(): Bad contact uin or my uin. Exit.");
#endif
		return ftfail(gg, hContact);
	}

	// Try to connect if not ask user to connect me
	if((ip && port >= 10 && !(dcc = gg_dcc_send_file(ip, port, myuin, uin))) || (port < 10 && port > 0))
	{
		// Make fake dcc structure
		dcc = malloc(sizeof(struct gg_dcc));
		memset(dcc, 0, sizeof(struct gg_dcc));
		// Fill up structures
		dcc->uin = myuin;
		dcc->peer_uin = uin;
		dcc->fd = -1;
		dcc->type = GG_SESSION_DCC_SEND;
#ifdef DEBUGMODE
		gg_netlog("gg_sendfile(): Requesting user to connect us and scheduling gg_dcc struct for a later use.");
#endif
		gg_dcc_request(gg->sess, uin);
		list_add(&gg->requests, dcc, 0);
	}

	// Write filename
	if(gg_dcc_fill_file_info(dcc, files[0]) == -1)
	{
#ifdef DEBUGMODE
		gg_netlog("gg_sendfile(): Cannot open and file fileinfo \"%s\".", files[0]);
#endif
		gg_free_dcc(dcc);
		return ftfail(gg, hContact);
	}

#ifdef DEBUGMODE
	gg_netlog("gg_sendfile(): Sending file \"%s\" to %d in %s mode.", files[0], uin, (dcc->fd != -1) ? "active" : "passive");
#endif

	// Add dcc to watches if not passive
	if(dcc->fd != -1) list_add(&gg->watches, dcc, 0);

	// Store handle
	dcc->contact = hContact;
	dcc->folder = _strdup(files[0]);
	dcc->tick = 0;
	// Make folder name
	bslash = strrchr(dcc->folder, '\\');
	if(bslash)
		*(bslash + 1) = 0;
	else
		*(dcc->folder) = 0;

	return (int)(HANDLE)dcc;
}

int gg_fileallow(PROTO_INTERFACE *proto, HANDLE hContact, HANDLE hTransfer, const char* szPath)
{
	GGPROTO *gg = (GGPROTO *) proto;
	struct gg_dcc *dcc = (struct gg_dcc *) hTransfer;
	char fileName[MAX_PATH];
	strncpy(fileName, szPath, sizeof(fileName));
	strncat(fileName, dcc->file_info.filename, sizeof(fileName) - strlen(fileName));
	dcc->folder = _strdup((char *) szPath);
	dcc->tick = 0;

	// Check if its proper dcc
	if(!dcc) return 0;

	// Remove transfer from waiting list
	pthread_mutex_lock(&gg->ft_mutex);
	list_remove(&gg->transfers, dcc, 0);
	pthread_mutex_unlock(&gg->ft_mutex);

	// Open file for appending and check if ok
	if(!(dcc->file_fd = _open(fileName, O_APPEND | O_BINARY)))
	{
		// Free transfer
		gg_free_dcc(dcc);
#ifdef DEBUGMODE
		gg_netlog("gg_fileallow(): Failed to create file \"%s\".", fileName);
#endif
		ProtoBroadcastAck(proto->m_szModuleName, dcc->contact, ACKTYPE_FILE, ACKRESULT_FAILED, dcc, 0);
		return 0;
	}

	// Put an offset to the file
	dcc->offset = _lseek(dcc->file_fd, 0, SEEK_END);

	// Add to watches and start transfer
	pthread_mutex_lock(&gg->ft_mutex);
	list_add(&gg->watches, dcc, 0);
	pthread_mutex_unlock(&gg->ft_mutex);

#ifdef DEBUGMODE
	gg_netlog("gg_fileallow(): Receiving file \"%s\" from %d.", dcc->file_info.filename, dcc->peer_uin);
#endif

	return (int) hTransfer;
}

int gg_filedeny(PROTO_INTERFACE *proto, HANDLE hContact, HANDLE hTransfer, const char* szReason)
{
	GGPROTO *gg = (GGPROTO *) proto;
	struct gg_dcc *dcc = (struct gg_dcc *) hTransfer;

	// Check if its proper dcc
	if(!dcc) return 0;

	// Remove transfer from any list
	pthread_mutex_lock(&gg->ft_mutex);
	if(gg->watches) list_remove(&gg->watches, dcc, 0);
	if(gg->requests) list_remove(&gg->requests, dcc, 0);
	if(gg->transfers) list_remove(&gg->transfers, dcc, 0);
	pthread_mutex_unlock(&gg->ft_mutex);

#ifdef DEBUGMODE
	gg_netlog("gg_filedeny(): Rejected file \"%s\" from/to %d.", dcc->file_info.filename, dcc->peer_uin);
#endif

	// Free transfer
	gg_free_dcc(dcc);

	return 0;
}

int gg_filecancel(PROTO_INTERFACE *proto, HANDLE hContact, HANDLE hTransfer)
{
	GGPROTO *gg = (GGPROTO *) proto;
	struct gg_dcc *dcc = (struct gg_dcc *) hTransfer;

	// Check if its proper dcc
	if(!dcc) return 0;

	// Remove transfer from any list
	pthread_mutex_lock(&gg->ft_mutex);
	if(gg->watches) list_remove(&gg->watches, dcc, 0);
	if(gg->requests) list_remove(&gg->requests, dcc, 0);
	if(gg->transfers) list_remove(&gg->transfers, dcc, 0);
	pthread_mutex_unlock(&gg->ft_mutex);

	// Send failed info
	ProtoBroadcastAck(GG_PROTO, dcc->contact, ACKTYPE_FILE, ACKRESULT_FAILED, dcc, 0);
	// Close file
	if(dcc->file_fd != -1)
	{
		_close(dcc->file_fd);
		dcc->file_fd = -1;
	}

#ifdef DEBUGMODE
	gg_netlog("gg_filecancel(): Canceled file \"%s\" from/to %d.", dcc->file_info.filename, dcc->peer_uin);
#endif

	// Free transfer
	gg_free_dcc(dcc);

	return 0;
}
