#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _MSVC_
#include <io.h>
#define open _open
#define close _close
#define read _read
#define write _write
#endif

#include "filesession.h"
#include "list.h"
#include "icqpacket.h"
#include "stdpackets.h"

icq_FileSession *icq_FileSessionNew(ICQLINK *icqlink)
{
  icq_FileSession *p=(icq_FileSession *)malloc(sizeof(icq_FileSession));

  if (p)
  {
    p->status=0;
    p->id=0L;
    p->icqlink=icqlink;
	p->tcplink=NULL;
    p->current_fd=-1;
    p->current_file_num=0;
    p->current_file_progress=0;
    p->current_file_size=0;
    p->files=0L;
    p->current_speed=100;
    p->total_bytes=0;
    p->total_files=0;
    p->total_transferred_bytes=0;
    p->working_dir[0]=0;
    list_insert(icqlink->icq_FileSessions, 0, p);
  }
	
  return p;
}

void icq_FileSessionDelete(void *pv)
{
  icq_FileSession *p=(icq_FileSession *)pv;

  if(p->files) {
    char **p2=p->files;
    while(*p2)
      free(*(p2++));
    free(p->files);
  }

  if (p->current_fd > -1 ) {
     close(p->current_fd);
     p->current_fd=-1;
  }

  free(p);
}

int _icq_FindFileSession(void *p, va_list data)
{
  icq_FileSession *psession=(icq_FileSession *)p;
  DWORD uin=va_arg(data, DWORD);
  unsigned long id=va_arg(data, unsigned long);
  
  return (psession->remote_uin == uin) && ( id ? (psession->id == id) : 1 );

}

icq_FileSession *icq_FindFileSession(ICQLINK *icqlink, DWORD uin,
  unsigned long id)
{
  return list_traverse(icqlink->icq_FileSessions, _icq_FindFileSession, 
    uin, id);
}

void icq_FileSessionSetStatus(icq_FileSession *p, int status)
{
  if(status!=p->status)
  {
    p->status=status;
    if(p->id && p->icqlink->icq_RequestNotify)
      (*p->icqlink->icq_RequestNotify)(p->icqlink, p->id, ICQ_NOTIFY_FILE,
       status, 0);
  }
}

void icq_FileSessionSetHandle(icq_FileSession *p, const char *handle)
{
  strncpy(p->remote_handle, handle, 64);
}

void icq_FileSessionSetCurrentFile(icq_FileSession *p, const char *filename)
{
  struct _stat file_status;
  char file[1024];

  strcpy(file, p->working_dir);
  strcat(file, filename);

  if (p->current_fd>-1) {
    close(p->current_fd);
    p->current_fd=-1;
  }

  strncpy(p->current_file, file, 64);
  p->current_file_progress=0;

  /* does the file already exist? */
  if (_stat(file, &file_status)==0) {
    p->current_file_progress=file_status.st_size;
    p->total_transferred_bytes+=file_status.st_size;
#ifdef _WIN32
    p->current_fd=open(file, O_WRONLY | O_APPEND | O_BINARY);
#else
    p->current_fd=open(file, O_WRONLY | O_APPEND);
#endif
  } else {
#ifdef _WIN32
    p->current_fd=open(file, O_WRONLY | O_CREAT | O_BINARY,_S_IREAD|_S_IWRITE);
#else
    p->current_fd=open(file, O_WRONLY | O_CREAT, S_IRWXU);
#endif
  }

  /* make sure we have a valid filehandle */
  if (p->current_fd == -1)
    perror("couldn't open file: ");
      
}

void icq_FileSessionPrepareNextFile(icq_FileSession *p)
{
  int i=0;
  char **files=p->files;

  p->current_file_num++;

  while(*files) {
    i++;
    if(i==p->current_file_num)
      break;
    else
      files++;
  }

  if(*files) {
    struct _stat file_status;

    if (p->current_fd>-1) {
       close(p->current_fd);
       p->current_fd=-1;
    }

    if (_stat(*files, &file_status)==0) {
       char *basename=*files;
#ifdef _WIN32
       char *pos=strrchr(basename, '\\');
#else
       char *pos=strrchr(basename, '/');
#endif
       if(pos) basename=pos+1;
       strncpy(p->current_file, basename, 64);
       p->current_file_progress=0;
       p->current_file_size=file_status.st_size;
#ifdef _WIN32
       p->current_fd=open(*files, O_RDONLY|_O_BINARY);
#else
       p->current_fd=open(*files, O_RDONLY);
#endif
    }

    /* make sure we have a valid filehandle */
    if (p->current_fd == -1)
       perror("couldn't open file: ");
  }     
}        

void icq_FileSessionSendData(icq_FileSession *p)
{
  /* for now just send a packet at a time */
  char buffer[2048];
  int count=read(p->current_fd, buffer, 2048);

  if(count>0) {
      icq_Packet *p2=icq_TCPCreateFile06Packet(count, buffer);
      icq_TCPLinkSend(p->tcplink, p2);
      p->total_transferred_bytes+=count;
      p->current_file_progress+=count;
      icq_FileSessionSetStatus(p, FILE_STATUS_SENDING);
      
      if (p->icqlink->icq_RequestNotify)
        (*p->icqlink->icq_RequestNotify)(p->icqlink, p->id,
          ICQ_NOTIFY_FILEDATA, count, NULL); 
  }

  /* done transmitting if read returns less that 2048 bytes */
  if(count<2048)
      icq_FileSessionClose(p);

  return;
}

/* public */

void icq_FileSessionSetSpeed(icq_FileSession *p, int speed)
{
  icq_Packet *packet=icq_TCPCreateFile05Packet(speed);

  icq_TCPLinkSend(p->tcplink, packet);
}

void icq_FileSessionClose(icq_FileSession *p)
{
  icq_TCPLink *plink=p->tcplink;

  /* TODO: handle closing already unallocated filesession? */

  /* if we're attached to a tcplink, unattach so the link doesn't try
   * to close us, and then close the tcplink */
  if (plink)
  {
    plink->session=0L;
    icq_TCPLinkClose(plink);
  }

  list_remove(p->icqlink->icq_FileSessions, p);		

  icq_FileSessionDelete(p);
}   

void icq_FileSessionSetWorkingDir(icq_FileSession *p, const char *dir)
{
  strncpy(p->working_dir, dir, sizeof(((icq_FileSession*)0)->working_dir));
}  

void icq_FileSessionSetFiles(icq_FileSession *p, char **files)
{
  p->files=files;
}

