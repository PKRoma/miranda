{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : m_icq
 * Description : Converted Headerfile from m_icq.h
 *
 * Author      : Christian Kästner
 * Date        : 28.04.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_icq;

interface

uses windows,m_protosvc;

const
  ICQEVENTTYPE_YOUWEREADDED  =1000;
  ICQEVENTTYPE_AUTHREQUEST   =1001;
  ICQEVENTTYPE_FILE          =1002;


//triggered when the server acknowledges that a message or url or whatever has
//gone, sent loads of times during file transfers
//wParam=event id, returned when you initiated the send
//lParam=message type, see icq_notify_* in icq.h

const
  ICQ_NOTIFY_SUCCESS        =0;
  ICQ_NOTIFY_FAILED         =1;
  ICQ_NOTIFY_CONNECTING     =2;
  ICQ_NOTIFY_CONNECTED      =3;
  ICQ_NOTIFY_SENT           =4;
  ICQ_NOTIFY_ACK            =5;

  ICQ_NOTIFY_CHAT           =6;
  ICQ_NOTIFY_CHATDATA       =7;

  ICQ_NOTIFY_FILE           =10;
  ICQ_NOTIFY_FILESESSION    =11;
  ICQ_NOTIFY_FILEDATA       =12;

const
  ME_ICQ_EVENTSENT    ='ICQ/EventSent';
  

//send a request to the server to get the information about a user
//wParam=(WPARAM)(HANDLE)hContact
//lParam=0
//returns 0 if hContact is a valid ICQ contact, nonzero otherwise, or if
//you are offline
//This never replies, the information is just updated in the database.
//Users should hook db/contact/settingchanged if they want to know when the
//information arrives
//const
//  MS_ICQ_UPDATECONTACTINFO  ='ICQ/UpdateContactInfo';
  

//send a request to the server to get the extended information about a user
//wParam=(WPARAM)(HANDLE)hContact
//lParam=0
//returns 0 if hContact is a valid ICQ contact, nonzero otherwise, or if
//you are offline
//This never replies, the information is just updated in the database.
//Users should hook db/contact/settingchanged if they want to know when the
//information arrives
//const
//  MS_ICQ_UPDATECONTACTINFOEXT  ='ICQ/UpdateContactInfoExt';
  

//send a message
//wParam=0
//lParam=(LPARAM)(ICQSENDMESSAGE* )ism
//hook icq/eventsent to check that the message went
//returns an id that should correspond to wParam of icq/eventsent
type
  PICQSENDMESSAGE=^TICQSENDMESSAGE;
  TICQSENDMESSAGE=record
    cbSize:Integer;
    uin:DWord;
    pszMessage:PChar;
    routeOverride:Integer;
  end;
const
  //new
  PIMF_ROUTE_DEFAULT    =0     ;
  PIMF_ROUTE_DIRECT     =$10000;
  PIMF_ROUTE_THRUSERVER =$20000;
  PIMF_ROUTE_BESTWAY    =$30000;
  PIMF_ROUTE_MASK       =$30000;

const
  MS_ICQ_SENDMESSAGE        ='ICQ/SendMessage';
  

//send a URL
//wParam=0
//lParam=(LPARAM)(ICQSENDURL* )isu
//hook icq/eventsent to check that the message went
//returns an id that should correspond to wParam of icq/eventsent
type
  TICQSENDURL=record
    cbSize:integer;
    uin:DWord;
    pszUrl:PChar;
    pszDescription:PChar;
  end;
//const
//  MS_ICQ_SENDURL        ='ICQ/SendURL';
  

//allow somebody to add you to their contact list
//wParam=uin of requester
//lParam=0
//returns 0 on success, nonzero otherwise
//const
//  MS_ICQ_AUTHORIZE      ='ICQ/Authorize';
  

//start a search of all ICQ users by e-mail. results are returned with
//icq/searchresult and icq/searchdone events
//wParam=0
//lParam=(LPARAM)(const char* )email
//returns 0 on success, nonzero otherwise
const
  MS_ICQ_SEARCHBYEMAIL  ='ICQ/SearchByEmail';
  

//start a search of all ICQ users by details. results are returned with
//icq/searchresult and icq/searchdone events
//wParam=0
//lParam=(LPARAM)(ICQSEARCHRESULT* )&isr
//returns 0 on success, nonzero otherwise
//uin, email and auth fields of the structure are ignored
type
  TICQSEARCHRESULT=record
    uin:DWord;
    nick:PChar;
    firstName:PChar;
    lastName:PChar;
    email:PChar;
    auth:Byte;
  end;
const
  MS_ICQ_SEARCHBYDETAILS  ='ICQ/SearchByDetails';
  

//triggered when the server sends a search result
//wParam=0
//lParam=(LPARAM)(ICQSEARCHRESULT* )&isr
const
  ME_ICQ_SEARCHRESULT   ='ICQ/SearchResult';
  

//triggered when a search has finished
//wParam=lParam=0
const
  ME_ICQ_SEARCHDONE	  ='ICQ/SearchDone';
  

//accept an incoming file transfer request and begin the transfer
//wParam=0
//lParam=(LPARAM)(ICQACCEPTFILES* )ias
//icqSeq is the sequence number put in the first DWORD of the database blob for
//this event
//pszWorkingDir is the path to put the files in, with trailing backslash
//returns a handle to the file transfer, to be used in some other functions
type
  TICQACCEPTFILES=record
    cbSize:Integer;
    hContact:THandle;
    icqSeq:DWord;
    pszWorkingDir:PChar;
  end;
//const
//  MS_ICQ_ACCEPTFILEREQUEST  ='ICQ/AcceptFileRequest';
  

//refuse an incoming file request
//wParam=(WPARAM)icqSeq
//lParam=(LPARAM)(ICQSENDMESSAGE* )ism
//icqSeq is the sequence number put in the first DWORD of the database blob for
//this event
//ism has the uin to send to, and the decline message to send
//returns 0 on success, nonzero otherwise
//const
//  MS_ICQ_REFUSEFILEREQUEST  ='ICQ/RefuseFileRequest';
  

//close a handle returned by icq/acceptfilerequest that has finished receiving
//wParam=(WPARAM)(HANDLE)hTransfer
//lParam=0
//returns 0 on success, nonzero otherwise
//NOTE: I have no idea when/if this function is supposed to be called
//const
//  MS_ICQ_CLOSEFILETRANSFERHANDLE ='ICQ/CloseFileTransferHandle';
  

//send a set of files
//wParam=0
//lParam=(LPARAM)(ICQFILEREQUEST* )&ifr
//returns the sequence ID that this transfer will use. Hook icq/eventsent.
type
  TICQFILEREQUEST=record
    cbSize:Integer;		//size of this struct in bytes
    uin:DWord;			//UIN to send files to
    pszMessage:PChar;	        //description to send
    files:^PChar;//??		//array of ASCIIZ filenames, NULL-terminated
    hSession:THandle;	        //output: handle to the file session, to be closed when
                                //done
  end;
//const
//  MS_ICQ_SENDFILETRANSFER     ='ICQ/SendFileTransfer';
  

//gets a load of information about the state of an active file transfer
//wParam=(WPARAM)(HANDLE)hTransfer
//lParam=(LPARAM)(ICQFILETRANSFERSTATUS* )&ifts
//returns 0 for success, nonzero on error
//ifts.cbSize should be filled by the caller before calling this service.
//note that icqlib is very agressive about closing transfer handles, so callers
//should be very careful about calling this only during file transfer
//notifications.
const
  FILE_STATUS_SENDING      =8;
const
  FILE_STATUS_RECEIVING    =9;
type
  TICQFILETRANSFERSTATUS=record
//conversion questions: char **x = ^PChar??? unsigned long = LongWord???
    cbSize:Integer;		   //fill before calling
    status:Integer;		   //see the icq_notify_* at the top of this file
    direction:Integer;	   //one of the file_status_*ing above
    remoteUin:LongWord;
    remoteNick:PChar;
    files:^PChar;//??
    totalFiles:Integer;
    currentFileNumber:Integer;
    totalBytes:LongWord;
    totalProgress:LongWord;
    workingDir:PChar;
    currentFile:PChar;
    currentFileSize:LongWord;
    currentFileProgress:LongWord;
    currentSpeed:Integer;	  //0-100
  end;
//const
//  MS_ICQ_GETFILETRANSFERSTATUS='ICQ/GetFileTransferStatus';
  

//a logging message was sent from icqlib
//wParam=(WPARAM)(BYTE)level
//lParam=(LPARAM)(char* )pszMsg
//level is a detail level as defined by icqlib
const
  ME_ICQ_LOG          ='ICQ/Log';


//all the missing events that you were expecting to see here are elsewhere.
//incoming messages, urls, etc are added straight to the database so you should
//pick up db/event/added
//online status changes are also added to the database, but as contact settings
//so db/contact/settingchanged events are sent


implementation

end.
