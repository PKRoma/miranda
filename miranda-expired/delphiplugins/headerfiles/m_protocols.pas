{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : m_protocols
 * Description : Converted Headerfile from m_protocols.h
 *
 * Author      : Christian Kästner
 * Date        : 26.10.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_protocols;

interface

//this module was created in v0.1.1.0

{$ifndef M_PROTOSVC_H_}
{$define M_PROTOSVC_H_}

uses windows,newpluginapi,sysutils;

//call a specific protocol service. See the PS_ constants in m_protosvc.h
function CallProtoService(PluginLink:TPluginLink;szModule,szService:PChar;wParam,lParam:DWord):Integer;


//send a general request through the protocol chain for a contact
//wParam=0
//lParam=(LPARAM)(CCSDATA*)&ccs
//returns the value as documented in the PS_ definition (m_protosvc.h)
type
  TCCSDATA=record
    hContact:THandle;
    szProtoService:Pchar;   //a PS_ constant
    wParam:DWord;
    lParam:DWord;
  end;

const
  MS_PROTO_CALLCONTACTSERVICE  = 'Proto/CallContactService';
function CallContactService(PluginLink:TPluginLink;hContact:THandle;szProtoService:PChar;wParam,lParam:DWord):Integer;

//a general network 'ack'
//wParam=0
//lParam=(LPARAM)(ACKDATA*)&ack
//Note that just because definitions are here doesn't mean they will be sent.
//Read the documentation for the function you are calling to see what replies
//you will receive.
type
  PACKDATA=^TACKDATA;
  TACKDATA=record
    cbSize:Integer;
    szModule:PChar;  //the name of the protocol module which initiated this ack
    hContact:THandle;
    type_:Integer;     //an ACKTYPE_ constant
    Result:Integer; 	//an ACKRESULT_ constant
    hProcess:THandle;   //a caller-defined process code
    lParam:DWord;	   //caller-defined extra info
  end;
const
  ACKTYPE_MESSAGE    =0;
  ACKTYPE_URL        =1;
  ACKTYPE_FILE       =2;
  ACKTYPE_CHAT       =3;
  ACKTYPE_AWAYMSG    =4;
  ACKTYPE_AUTHREQ    =5;
  ACKTYPE_ADDED      =6;
  ACKTYPE_GETINFO    =7;
  ACKTYPE_SETINFO    =8;
  ACKTYPE_LOGIN      =9;
  ACKTYPE_SEARCH     =10;
  ACKTYPE_NEWUSER    =11;
  ACKTYPE_STATUS     =12;
  ACKRESULT_SUCCESS     =0;
  ACKRESULT_FAILED      =1;
//'in progress' result codes:
  ACKRESULT_CONNECTING  =100;
  ACKRESULT_CONNECTED   =101;
  ACKRESULT_INITIALISING= 102;
  ACKRESULT_SENTREQUEST =103;  //waiting for reply...
  ACKRESULT_DATA     =   104;  //blob of file data sent/recved, or search result
  ACKRESULT_NEXTFILE =   105;  //file transfer went to next file
  ME_PROTO_ACK       ='Proto/Ack';

//when type==ACKTYPE_FILE && result==ACKRESULT_DATA, lParam points to this
type
  TPROTOFILETRANSFERSTATUS=record
    cbSize:Integer;
    hContact:THandle;
    sending:Integer;	//true if sending, false if receiving
    files:PChar;
    totalFiles:Integer;
    currentFileNumber:Integer;
    totalBytes:LongWord;
    totalProgress:LongWord;
    workingDir:PChar;
    currentFile:PChar;
    currentFileSize:LongWord;
    currentFileProgress:LongWord;
  end;

//Enumerate the currently running protocols
//wParam=(WPARAM)(int*)&numberOfProtocols
//lParam=(LPARAM)(PROTOCOLDESCRIPTOR***)&ppProtocolDescriptors
//Returns 0 on success, nonzero on failure
//Neither wParam nor lParam may be NULL
//The list returned by this service is the protocol modules currently installed
//and running. It is not the complete list of all protocols that have ever been
//installed.
//Note that a protocol module need not be an interface to an Internet server,
//they can be encryption and loads of other things, too.
//And yes, before you ask, that is triple indirection. Deal with it.
//Access members using ppProtocolDescriptors[index]->element
type
  TPROTOCOLDESCRIPTOR=record
    cbSize:Integer;
    szName:PChar;    //unique name of the module
    type_:Integer;      //module type, see PROTOTYPE_ constants
  end;
const
  PROTOTYPE_PROTOCOL    =1000 ;
  PROTOTYPE_ENCRYPTION  =2000 ;
  PROTOTYPE_FILTER      =3000 ;
  PROTOTYPE_TRANSLATION =4000 ;
  PROTOTYPE_OTHER       =10000;   //avoid using this if at all possible
  MS_PROTO_ENUMPROTOCOLS     ='Proto/EnumProtocols';

//determines if a protocol module is loaded or not
//wParam=0
//lParam=(LPARAM)(const char*)szName
//Returns a pointer to the PROTOCOLDESCRIPTOR if the protocol is loaded, or
//NULL if it isn't.
const
  MS_PROTO_ISPROTOCOLLOADED  ='Proto/IsProtocolLoaded';

//gets the network-level protocol associated with a contact
//wParam=(WPARAM)(HANDLE)hContact
//lParam=0
//Returns a char* pointing to the asciiz name of the protocol or NULL if the
//contact has no protocol. There is no need to free() it or anything.
//This is the name of the module that actually accesses the network for that
//contact.
const
  MS_PROTO_GETCONTACTBASEPROTO  ='Proto/GetContactBaseProto';

//determines whether the specified contact has the given protocol in its chain
//wParam=(WPARAM)(HANDLE)hContact
//lParam=(LPARAM)(const char*)szName
//Returns nonzero if it does and 0 if it doesn't
const
  MS_PROTO_ISPROTOONCONTACT  ='Proto/IsProtoOnContact';

implementation


function CallProtoService(PluginLink:TPluginLink;szModule,szService:PChar;wParam,lParam:DWord):Integer;
var
  str:array [0..MAXMODULELABELLENGTH] of char;
begin
  strcopy(@str[0],szModule);
  strcat(@str[0],szService);
  Result:=PluginLink.CallService(@str[0],wParam,lParam);
end;

function CallContactService(PluginLink:TPluginLink;hContact:THandle;szProtoService:PChar;wParam,lParam:DWord):Integer;
var
  ccs:TCCSDATA;
begin
  ccs.hContact:=hContact;
  ccs.szProtoService:=szProtoService;
  ccs.wParam:=wParam;
  ccs.lParam:=lParam;
  Result:=PluginLink.CallService(MS_PROTO_CALLCONTACTSERVICE,0,dword(@ccs));
end;

{$else}
implementation
{$endif}  //M_PROTOSVC_H_

end.
