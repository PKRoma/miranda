{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Description : Converted Headerfile
 *
 * Author      : Christian Kästner
 * Date        : 22.12.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_protomod;

interface

//this module was created in v0.1.1.0

//this header file is for the use of protocol modules only. Other users should
//use the functions exposed in m_protocols.h and m_protosvc.h

{$ifndef M_PROTOMOD_H_}
{$DEFINE M_PROTOMOD_H_}

uses windows,m_protocols;

//notify the protocol manager that you're around
//wParam=0
//lParam=(PROTOCOLDESCRIPTOR*)&descriptor
//returns 0 on success, nonzero on failure
//This service must be called in your module's Load() routine.
//descriptor.type can be a value other than the PROTOTYPE_ constants specified
//above to provide more precise positioning information for the contact
//protocol lists. It is strongly recommended that you give values relative to
//the constants, however, by adding or subtracting small integers (<=100).
//PROTOTYPE_PROTOCOL modules must not do this. The value must be exact.
//See MS_PROTO_ENUMPROTOCOLS for more notes.
const
  MS_PROTO_REGISTERMODULE   ='Proto/RegisterModule';

//adds the specified protocol module to the chain for a contact
//wParam=(WPARAM)(HANDLE)hContact
//lParam=(LPARAM)(const char*)szName
//returns 0 on success, nonzero on failure
//The module is added in the correct position according to the type given when
//it was registered.
const
  MS_PROTO_ADDTOCONTACT     ='Proto/AddToContact';

//removes the specified protocol module from the chain for a contact
//wParam=(WPARAM)(HANDLE)hContact
//lParam=(LPARAM)(const char*)szName
//returns 0 on success, nonzero on failure
const
  MS_PROTO_REMOVEFROMCONTACT     ='Proto/RemoveFromContact';

//Create a protocol service
//Protocol services are called with wParam and lParam as standard if they are
//to be called with CallProtoService() (as PS_ services are)
//If they are called with CallContactService() (PSS_ and PSR_ services) then
//they are called with lParam=(CCSDATA*)&ccs and wParam an opaque internal
//reference that should be passed unchanged to MS_PROTO_CHAIN*.
function CreateProtoServiceFunction(szModule:PChar,szService:PChar,serviceProc:MIRANDASERVICE):THandle;

//Call the next service in the chain for this send operation
//wParam=wParam
//lParam=lParam
//The return value should be returned immediately
//wParam and lParam should be passed as the parameters that your service was
//called with. wParam must remain untouched but lParam is a CCSDATA structure
//that can be copied and modified if needed.
//Typically, the last line of any chaining protocol function is
//return CallService(MS_PROTO_CHAINSEND,wParam,lParam);
const
  MS_PROTO_CHAINSEND      ='Proto/ChainSend';

//Call the next service in the chain for this receive operation
//wParam=wParam
//lParam=lParam
//The return value should be returned immediately
//wParam and lParam should be passed as the parameters that your service was
//called with. wParam must remain untouched but lParam is a CCSDATA structure
//that can be copied and modified if needed.
//When being initiated by the network-access protocol module, wParam should be
//zero.
//Thread safety: ms_proto_chainrecv is completely thread safe since 0.1.2.0
//Calls to it are translated to the main thread and passed on from there. The
//function will not return until all callees have returned, irrepective of
//differences between threads the functions are in.
const
  MS_PROTO_CHAINRECV      ='Proto/ChainRecv';

//Broadcast a ME_PROTO_ACK event
//wParam=0
//lParam=(LPARAM)(ACKDATA*)&ack
//returns the return value of the notifyeventhooks() call
//Thread safety: me_proto_ack is completely thread safe since 0.1.2.0
//See the notes in core/modules.h under NotifyEventHooks()
const
  MS_PROTO_BROADCASTACK   ='Proto/BroadcastAck';

function ProtoBroadcastAck(szModule:PChar;hContact:THandle;type_,Result_:integer;hProcess:THandle;lParam:DWord):Integer;

{$endif}

implementation

function CreateProtoServiceFunction(szModule:PChar,szService:PChar,serviceProc:MIRANDASERVICE):THandle;
var
  str:string[MAXMODULELABELLENGTH];
begin
  strcpy(str,szModule);
  strcat(str,szService);
  Result:=CreateServiceFunction(str,serviceProc);
end;

function ProtoBroadcastAck(szModule:PChar;hContact:THandle;type_,Result_:integer;hProcess:THandle;lParam:DWord):Integer;
var
  ack:TACTDATA;
begin
  ack.cbSize=sizeof(ACKDATA);
  ack.szModule=szModule;
  ack.hContact=hContact;
  ack.type_=type_;
  ack.result=result_;
  ack.hProcess=hProcess; ack.lParam=lParam;
  Result:=CallService(MS_PROTO_BROADCASTACK,0,(LPARAM)&ack);
end;



{$else}
implementation
{$endif}


end.
