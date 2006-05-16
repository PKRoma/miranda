#include "aim.h"

static pthread_mutex_t freeMutex;

void aim_filerecv_init()
{
    pthread_mutex_init(&freeMutex);
}

void aim_filerecv_destroy()
{
    pthread_mutex_destroy(&freeMutex);
}

void aim_filerecv_free_fr(struct aim_filerecv_request *ft)
{
    pthread_mutex_lock(&freeMutex);
    if (ft) {
        XFREE(ft->user);
        XFREE(ft->cookie);
        XFREE(ft->ip);
        XFREE(ft->vip);
        XFREE(ft->message);
        XFREE(ft->filename);
        XFREE(ft->savepath);
        XFREE(ft->cfullname);
        free(ft);
        ft = NULL;
    }
    pthread_mutex_unlock(&freeMutex);
}

void aim_filerecv_deny(char *user, char *cookie)
{
    char buf[MSG_LEN];

    mir_snprintf(buf, sizeof(buf), "toc_rvous_cancel %s %s %s", user, cookie, UID_AIM_FILE_RECV);
    aim_toc_sflapsend(buf, -1, TYPE_DATA);
}

void aim_filerecv_accept(char *user, char *cookie)
{
    char buf[MSG_LEN];

    mir_snprintf(buf, sizeof(buf), "toc_rvous_accept %s %s %s", user, cookie, UID_AIM_FILE_RECV);
    aim_toc_sflapsend(buf, -1, TYPE_DATA);
}

void aim_filerecv_debug_header(struct aim_filerecv_request *ft)
{
    struct aim_filerecv_header *f = &ft->hdr;

    PLOG(LOG_INFO, "[%s] FRH: hl=%d tp=0x%04x fc=%d l=%d p=%d pl=%d ts=%d s=%d r=%d t=%d f=0x%02x id=%s name=%s",
         ft->cookie,
         ntohs(f->hdrlen),
         f->hdrtype,
         ntohs(f->totfiles),
         ntohs(f->filesleft),
         ntohs(f->totparts),
         ntohs(f->partsleft), ntohl(f->totsize), ntohl(f->size), ntohl(f->nrecvd), ntohl(f->modtime), f->flags, f->idstring, f->name);
}

static void aim_filerecv_mkdirtree(char *szDir)
{
    DWORD dwAttributes;
    char *pszLastBackslash, szTestDir[MAX_PATH];

    lstrcpyn(szTestDir, szDir, sizeof(szTestDir));
    if ((dwAttributes = GetFileAttributes(szTestDir)) != 0xffffffff && dwAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return;
    pszLastBackslash = strrchr(szTestDir, '\\');
    if (pszLastBackslash == NULL)
        return;
    *pszLastBackslash = '\0';
    aim_filerecv_mkdirtree(szTestDir);
    CreateDirectory(szTestDir, NULL);
}

static void aim_filerecv_mkdir(const char *file)
{
    char *str1, *str2;

    if (!file || !strlen(file))
        return;
    str2 = (char *) malloc(strlen(file) + 1);
    strcpy(str2, file);
    str1 = strrchr(str2, '\\');
    if (str1 != NULL) {
        if (str1 < (str2 + strlen(str2) - 1)) {
            *(str1 + 1) = 0;
            aim_filerecv_mkdirtree(str2);
        }
    }
    free(str2);
}

static char *aim_filerecv_fixpath(const char *file)
{
    char *f;
    int i;

    if (!file)
        return NULL;
    f = (char *) malloc(strlen(file) + 1);
    strcpy(f, file);
    for (i = 0; i < (int) strlen(f); i++) {
        if (f[i] == 1)
            f[i] = '\\';
        if (f[i] == ':')
            f[i] = '\\';
    }
    return f;
}

static int aim_filerecv_openfile(struct aim_filerecv_request *ft)
{
    char file[MAX_PATH], *szfilename, *szname;

    szfilename = szname = 0;
    if (ft->filename)
        szfilename = aim_filerecv_fixpath(ft->filename);
    if (ft->hdr.name)
        szname = aim_filerecv_fixpath(ft->hdr.name);
    mir_snprintf(file, sizeof(file), "%s\\%s%s%s", ft->savepath, szfilename, ntohs(ft->hdr.totfiles) == 1 ? "" : "\\",
                 ntohs(ft->hdr.totfiles) == 1 ? "" : szname);
    XFREE(szfilename);
    XFREE(szname);
    aim_filerecv_mkdir(file);
    ft->fileid = _open(file, _O_BINARY | _O_WRONLY | _O_CREAT | _O_TRUNC, _S_IREAD | _S_IWRITE);
    if (ft->fileid < 0) {
        PLOG(LOG_ERROR, "[%s] Could not open %s", ft->cookie, file);
        return 0;
    }
    PLOG(LOG_DEBUG, "[%s] Receiving %s", ft->cookie, file);
    ft->cfullname = _strdup(file);      // free'd later
    return 1;
}

static void aim_filerecv_closefile(struct aim_filerecv_request *ft)
{
    if (ft->fileid >= 0) {
        _close(ft->fileid);
        ft->fileid = -1;
    }
}

void __cdecl aim_filerecv_thread(void *vft)
{
    NETLIBOPENCONNECTION nloc = { 0 };
    struct aim_filerecv_request *ft = (struct aim_filerecv_request *) vft;
    char data[BUF_LONG], *buf;
    int recvResult;
    PROTOFILETRANSFERSTATUS pfts;

    nloc.cbSize = sizeof(NETLIBOPENCONNECTION);
    nloc.szHost = ft->vip;
    nloc.wPort = ft->port ? ft->port : AIM_AUTH_PORT;
    nloc.flags = 0;
    ProtoBroadcastAck(AIM_PROTO, ft->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTING, ft, 0);
    PLOG(LOG_INFO, "Trying to connect to %s:%d", nloc.szHost, nloc.wPort);
    ft->s = (HANDLE) CallService(MS_NETLIB_OPENCONNECTION, (WPARAM) hNetlibPeer, (LPARAM) & nloc);
    if (!ft->s) {
        nloc.szHost = ft->ip;
        PLOG(LOG_INFO, "Trying to connect to %s:%d", nloc.szHost, nloc.wPort);
        ft->s = (HANDLE) CallService(MS_NETLIB_OPENCONNECTION, (WPARAM) hNetlibPeer, (LPARAM) & nloc);
        if (!ft->s) {
            aim_filerecv_deny(ft->user, ft->cookie);
            ProtoBroadcastAck(AIM_PROTO, ft->hContact, ACKTYPE_FILE, ACKRESULT_FAILED, ft, 0);
            aim_filerecv_free_fr(ft);
            return;
        }
    }
    ProtoBroadcastAck(AIM_PROTO, ft->hContact, ACKTYPE_FILE, ACKRESULT_CONNECTED, ft, 0);
    ProtoBroadcastAck(AIM_PROTO, ft->hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, ft, 0);
    if (ft->savepath[strlen(ft->savepath) - 1] == '\\');
    ft->savepath[strlen(ft->savepath) - 1] = 0;
    while (ft->state == FR_STATE_RECEIVING) {
        if (ft->hdr.hdrtype != 0x202) {
            if (ft->cfullname) {
                free(ft->cfullname);
                ft->cfullname = NULL;
            }
            ft->current++;
            aim_util_base64decode(ft->cookie, &buf);
            recvResult = Netlib_Recv(ft->s, (char *) ft, 8, MSG_NODUMP);
            if (recvResult <= 0)
                break;
            ft->recvsize = 0;
            recvResult = Netlib_Recv(ft->s, (char *) &ft->hdr.bcookie, ntohs(ft->hdr.hdrlen) - 8, MSG_NODUMP);
            if (recvResult <= 0)
                break;
            aim_filerecv_debug_header(ft);
            ft->hdr.hdrtype = 0x202;
            memcpy(ft->hdr.bcookie, buf, 8);
            free(buf);
            ft->hdr.encrypt = 0;
            ft->hdr.compress = 0;
            Netlib_Send(ft->s, (char *) ft, 256, MSG_NODUMP);
            if (!aim_filerecv_openfile(ft))
                break;
        }
        else {
            recvResult = Netlib_Recv(ft->s, data, MIN(ntohl(ft->hdr.size) - ft->recvsize, sizeof(data) - 1), MSG_NODUMP);
            if (recvResult <= 0)
                break;
            ft->recvsize += recvResult;
            ft->totalt += recvResult;
            data[recvResult] = 0;
            if (_write(ft->fileid, data, recvResult) != recvResult)
                break;
            memset(&pfts, 0, sizeof(PROTOFILETRANSFERSTATUS));
            pfts.cbSize = sizeof(PROTOFILETRANSFERSTATUS);
            pfts.hContact = ft->hContact;
            pfts.sending = FALSE;
            pfts.files = NULL;
            pfts.totalFiles = ntohs(ft->hdr.totfiles);
            pfts.currentFileNumber = ft->current - 1;
            pfts.totalBytes = ft->size;
            pfts.totalProgress = ft->totalt;
            pfts.workingDir = ft->savepath;
            pfts.currentFile = ft->cfullname;
            pfts.currentFileSize = ntohl(ft->hdr.size);
            pfts.currentFileProgress = ft->recvsize;
            pfts.currentFileTime = ntohl(ft->hdr.modtime);
            ProtoBroadcastAck(AIM_PROTO, ft->hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, (LPARAM) & pfts);
            if (ft->recvsize >= ntohl(ft->hdr.size)) {
                PLOG(LOG_INFO, "[%s] Received file %d of %d", ft->cookie, ft->current, ntohs(ft->hdr.totfiles));
                ft->hdr.hdrtype = htons(0x0204);
                ft->hdr.filesleft = htons(ntohs(ft->hdr.filesleft) - 1);
                ft->hdr.partsleft = htons(ntohs(ft->hdr.partsleft) - 1);
                ft->hdr.recvcsum = ft->hdr.checksum;
                ft->hdr.nrecvd = htonl(ft->recvsize);
                ft->hdr.flags = 0;
                Netlib_Send(ft->s, (char *) ft, 256, MSG_NODUMP);
                aim_filerecv_debug_header(ft);
                ft->recvsize = 0;
                aim_filerecv_closefile(ft);
                if (ft->hdr.filesleft == 0) {
                    ft->state = FR_STATE_DONE;
                    PLOG(LOG_DEBUG, "[%s] File transfer complete", ft->cookie);
                }
                else {
                    ProtoBroadcastAck(AIM_PROTO, ft->hContact, ACKTYPE_FILE, ACKRESULT_NEXTFILE, ft, 0);
                }
            }
        }
    }
    if (ft->fileid >= 0)
        aim_filerecv_closefile(ft);
    if (ft->s) {
        Netlib_CloseHandle(ft->s);
        ft->s = NULL;
    }
    ProtoBroadcastAck(AIM_PROTO, ft->hContact, ACKTYPE_FILE, ft->state == FR_STATE_DONE ? ACKRESULT_SUCCESS : ACKRESULT_FAILED, ft, 0);
    aim_filerecv_free_fr(ft);
}
