{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : m_protosvc
 * Description : Converted Headerfile from m_protosvc.h
 *
 * Author      : Christian Kästner
 * Date        : 26.10.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_protosvc;

interface


//this module was created in v0.1.1.0

//none of these services should be used on their own (ie using CallService,
//CreateServiceFunction(), etc), hence the PS_ prefix. Instead use the services
//exposed in m_protocols.h

{$ifndef M_PROTOSVC_H_}
{$define M_PROTOSVC_H_}

uses m_protocols,windows;

//*************************** NON-CONTACT SERVICES ************************/
//these should be called with CallProtoService()

//Get the capability flags of the module.
//wParam=flagNum
//lParam=0
//Returns a bitfield corresponding to wParam. See the #defines below
//Should return 0 for unknown values of flagNum
//Non-network-access modules should return flags to represent the things they
//actually actively use, not the values that it is known to pass through
//correctly
const
  PFLAGNUM_1        =1         ;
  PF1_IMSEND        =$00000001  ;     //supports IM sending
  PF1_IMRECV        =$00000002 ;      //supports IM receiving
  PF1_IM            =(PF1_IMSEND or PF1_IMRECV) ;
  PF1_URLSEND       =$00000004 ;      //supports separate URL sending
  PF1_URLRECV       =$00000008;       //supports separate URL receiving
  PF1_URL           =(PF1_URLSEND or PF1_URLRECV);
  PF1_FILESEND      =$00000010;       //supports file sending
  PF1_FILERECV      =$00000020;       //supports file receiving
  PF1_FILE          =(PF1_FILESEND or PF1_FILERECV);
  PF1_MODEMSGSEND   =$00000040;       //supports broadcasting away messages
  PF1_MODEMSGRECV   =$00000080;       //supports reading others' away messages
  PF1_MODEMSG       =(PF1_MODEMSGSEND or PF1_MODEMSGRECV);
  PF1_SERVERCLIST   =$00000100;       //contact lists are stored on the server, not locally. See notes below
  PF1_AUTHREQ       =$00000200;       //will get authorisation requests for some or all contacts
  PF1_ADDED         =$00000400;       //will get 'you were added' notifications
  PF1_VISLIST       =$00000800;       //has a visible list for when in invisible mode
  PF1_INVISLIST     =$00001000;       //has an invisible list
  PF1_INDIVSTATUS   =$00002000;       //supports setting different status modes to each contact
  PF1_EXTENSIBLE    =$00004000;       //the protocol is extensible and supports plugin-defined messages
  PF1_PEER2PEER     =$00008000;       //supports direct (not server mediated) communication between clients
  PF1_NEWUSER       =$00010000;       //supports creation of new user IDs
  PF1_CHAT          =$00020000;       //has a realtime chat capability
  PF1_INDIVMODEMSG  =$00040000;       //supports replying to a mode message request with different text depending on the contact requesting
  PF1_BASICSEARCH   =$00080000;       //supports a basic user searching facility
  PF1_EXTSEARCH     =$00100000;	   //supports one or more protocol-specific extended search schemes
  PF1_CANRENAMEFILE =$00200000;       //supports renaming of incoming files as they are transferred
  PF1_FILERESUME    =$00400000;       //can resume broken file transfers
  PF1_ADDSEARCHRES  =$00800000;       //can add search results to the contact list
  PF1_CONTACTSEND   =$01000000;	   //can send contacts to other users
  PF1_CONTACTRECV   =$02000000;	   //can receive contacts from other users
  PF1_CONTACT       =(PF1_CONTACTSEND or PF1_CONTACTRECV);
  PF1_CHANGEINFO    =$04000000;       //can change our user information stored on server

  PFLAGNUM_2   =2;			 //the status modes that the protocol supports
  PF2_ONLINE        =$00000001;   //an unadorned online mode
  PF2_INVISIBLE     =$00000002;
  PF2_SHORTAWAY     =$00000004;   //Away on ICQ, BRB on MSN
  PF2_LONGAWAY      =$00000008;   //NA on ICQ, Away on MSN
  PF2_LIGHTDND      =$00000010;   //Occupied on ICQ, Busy on MSN
  PF2_HEAVYDND      =$00000020;   //DND on ICQ
  PF2_FREECHAT      =$00000040;
  PF2_OUTTOLUNCH    =$00000080;
  PF2_ONTHEPHONE    =$00000100;

  PFLAGNUM_3   =3;          //the status modes that the protocol supports
                 //away-style messages for. Uses the PF2_ flags.

(* Deleting contacts from protocols that store the contact list on the server:
If a contact is deleted while the protocol is online, it is expected that the
protocol will have hooked me_db_contact_deleted and take the appropriate
action by itself.
If a contact is deleted while the protocol is offline, the contact list will
display a message to the user about the problem, and set the byte setting
"CList"/"Delete" to 1. Each time such a protocol changes status from offline
or connecting to online the contact list will check for contacts with this
flag set and delete them at that time. Your hook for me_db_contact_deleted
will pick this up and everything will be good.
*)
const
  PS_GETCAPS     ='/GetCaps';

//Get a human-readable name for the protocol
//wParam=cchName
//lParam=(LPARAM)(char*)szName
//Returns 0 on success, nonzero on failure
//cchName is the number of characters in the buffer szName
//This should be translated before being returned
//Some example strings are:
//"ICQ", "AIM", "RSA-1024 Encryption"
const
  PS_GETNAME     ='/GetName';

//Loads one of the protocol-specific icons
//wParam=whichIcon
//lParam=0
//Returns the HICON, or NULL on failure
//The returned HICON must be DestroyIcon()ed.
//The UI should overlay the online icon with a further UI-specified icon to
//represent the exact status mode.
const
  PLI_PROTOCOL   =1;    //An icon representing the protocol (eg the multicoloured flower for ICQ)
  PLI_ONLINE     =2;    //Online state icon for that protocol (eg green flower for ICQ)
  PLI_OFFLINE    =3;    //Offline state icon for that protocol (eg red flower for ICQ)
  PLIF_LARGE     =0;		//OR with one of the above to get the large (32x32 by default) icon
  PLIF_SMALL     =$10000;  //OR with one of the above to get the small (16x16 by default) icon
  PS_LOADICON    = '/LoadIcon';

//Change the protocol's status mode
//wParam=newMode, from ui/contactlist/statusmodes.h
//lParam=0
//returns 0 on success, nonzero on failure
//Will send an ack with:
//type=ACKTYPE_STATUS, result=ACKRESULT_SUCCESS, hProcess=(HANDLE)previousMode, lParam=newMode
//when the change completes. This ack is sent for all changes, not just ones
//caused by calling this function.
//Note that newMode can be ID_STATUS_CONNECTING<=newMode<ID_STATUS_CONNECTING+
//MAX_CONNECT_RETRIES to signify that it's connecting and it's the nth retry.
//Protocols are initially always in offline mode.
//Non-network-level protocol modules do not have the concept of a status and
//should leave this service unimplemented
//If a protocol doesn't support the specific status mode, it should pick the
//closest one that it does support, and change to that.
//If the new mode requires that the protocol switch from offline to online then
//it will do so, but errors will be reported in the form of an additional ack:
//type=ACKTYPE_LOGIN, result=ACKRESULT_FAILURE, hProcess=NULL, lParam=LOGINERR_
const
  LOGINERR_WRONGPASSWORD  =1;
  LOGINERR_NONETWORK      =2;
  LOGINERR_PROXYFAILURE   =3;
  LOGINERR_BADUSERID      =4;
  LOGINERR_NOSERVER       =5;
  LOGINERR_TIMEOUT        =6;
  LOGINERR_WRONGPROTOCOL  =7;
//protocols may define more error codes starting at 1000
  PS_SETSTATUS   ='/SetStatus';

//Sets the status-mode specific message for yourself
//wParam=status mode
//lParam=(LPARAM)(const char*)szMessage
//Returns 0 on success, nonzero on failure
//Note that this service will not be available unless PF1_MODEMSGSEND is set
//and PF1_INDIVMODEMSG is *not* set.
//If PF1_INDIVMODEMSG is set, then see PSS_AWAYMSG for details of
//the operation of away messages
//Protocol modules must support szMessage=NULL. It may either mean to use an
//empty message, or (preferably) to not reply at all to any requests
const
  PS_SETAWAYMSG  ='/SetAwayMsg';
  

//Get the status mode that a protocol is currently in
//wParam=lParam=0
//Returns the status mode
//Non-network-level protocol modules do not have the concept of a status and
//should leave this service unimplemented
const
  PS_GETSTATUS	='/GetStatus';
  

//Allow somebody to add us to their contact list
//wParam=(WPARAM)(HANDLE)hDbEvent
//lParam=0
//Returns 0 on success, nonzero on failure
//Auth requests come in the form of an event added to the database for the NULL
//user. The form is:
//DWORD protocolSpecific
//ASCIIZ nick, firstName, lastName, e-mail, requestReason
//hDbEvent must be the handle of such an event
//One or more fields may be empty if the protocol doesn't support them
const
  PS_AUTHALLOW  = '/Authorize';
  

//Deny an authorisation request
//wParam=(WPARAM)(HANDLE)hDbEvent
//lParam=(LPARAM)(const char*)szReason
//Returns 0 on success, nonzero on failure
//Protocol modules must be able to cope with szReason=NULL
const
  PS_AUTHDENY  =  '/AuthDeny';
  

//Send a basic search request
//wParam=0
//lParam=(LPARAM)(const char*)szId
//Returns a handle for the search, or zero on failure
//All protocols identify users uniquely by a single field. This service will
//search by that field.
//Protocol modules may (likely will) support more advanced searches, but they
//cannot be abstracted neatly so are left to be protocol-specific.
//Note that if users are identified by an integer (eg ICQ) szId should be a
//string containing that integer, not the integer itself.
//All search replies (even protocol-specific extended searches) are replied by
//means of a series of acks:
//result acks, one of more of:
//type=ACKTYPE_SEARCH, result=ACKRESULT_DATA, lParam=(LPARAM)(PROTOSEARCHRESULT*)&psr
//ending ack:
//type=ACKTYPE_SEARCH, result=ACKRESULT_SUCCESS, lParam=0
//Note that the pointers in the structure are not guaranteed to be valid after
//the ack is complete.
type
  TPROTOSEARCHRESULT=record
    cbSize:Integer;
    nick:PChar;
    firstName:PChar;
    lastName:PChar;
    email:PChar;
    reserved:array [0..16] of char;
    //Protocols may extend this structure with extra members at will and supply
    //a larger cbSize to reflect the new information, but they must not change
    //any elements above this comment
    //The 'reserved' field is part of the basic structure, not space to
    //overwrite with protocol-specific information.
    //If modules do this, they should take steps to ensure that information
    //they put there will be retained by anyone trying to save this structure.
  end;
const
  PS_BASICSEARCH = '/BasicSearch';
  

//Adds a search result to the contact list
//wParam=flags
//lParam=(LPARAM)(PROTOSEARCHRESULT*)&psr
//Returns a HANDLE to the new contact, or NULL on failure
//psr must be a result returned by a search function, since the extended
//information past the end of the official structure may contain important
//data required by the protocol.
//The protocol library should not allow duplicate contacts to be added, but if
//such a request is received it should return the original hContact, and do the
//appropriate thing with the temporary flag (ie newflag=(oldflag&thisflag))
const
  PALF_TEMPORARY =   1;    //add the contact temporarily and invisibly, just to get user info or something
  PS_ADDTOLIST  = '/AddToList';
  

//Adds a contact to the contact list given an auth or added event
//wParam=flags
//lParam=(LPARAM)(HANDLE)hDbEvent
//Returns a HANDLE to the new contact, or NULL on failure
//hDbEvent must be either EVENTTYPE_AUTHREQ or EVENTTYPE_ADDED
//flags are the same as for PS_ADDTOLIST.
const
  PS_ADDTOLISTBYEVENT = '/AddToListByEvent';
  

//Changes our user details as stored on the server   v0.1.2.0+
//wParam=infoType
//lParam=(LPARAM)(void*)pInfoData
//Returns a HANDLE to the change request, or NULL on failure
//The details information that is stored on the server is very protocol-
//specific, so this service just supplies an outline for protocols to use.
//See protocol-specific documentation for what infoTypes are available and
//what pInfoData should be for each infoType.
//Sends an ack type=ACKTYPE_SETINFO, result=ACKRESULT_SUCCESS/FAILURE,
//lParam=0 on completion.
const
  PS_CHANGEINFO   =  '/ChangeInfo';
  

//****************************** SENDING SERVICES *************************/
//these should be called with CallContactService()

//Updates a contact's details from the server
//wParam=flags
//lParam=0
//returns 0 on success, nonzero on failure
//Will update all the information in the database, and then send acks with
//type=ACKTYPE_GETINFO, result=ACKRESULT_SUCCESS, hProcess=(HANDLE)(int)nReplies, lParam=thisReply
//Since some protocols do not allow the library to tell when it has got all
//the information so it can send a final ack, one ack will be sent after each
//chunk of data has been received. nReplies contains the number of distinct
//acks that will be sent to get all the information, thisReply is the zero-
//based index of this ack. When thisReply=0 the 'minimal' information has just
//been received. All other numbering is arbitrary.
const
  SGIF_MINIMAL  = 1 ;    //get only the most basic information. This should
                             //contain at least a Nick and e-mail.
const
  PSS_GETINFO     = '/GetInfo';
  

//Send an instant message
//wParam=flags
//lParam=(LPARAM)(const char*)szMessage
//returns a hProcess corresponding to the one in the ack event.
//Will send an ack when the message actually gets sent
//type=ACKTYPE_MESSAGE, result=success/failure, lParam=0
//Protocols modules are free to define flags starting at 0x10000
//The event will *not* be added to the database automatically.
const
  PSS_MESSAGE     = '/SendMsg';
  

//Send an URL message
//wParam=flags
//lParam=(LPARAM)(const char*)szMessage
//returns a hProcess corresponding to the one in the ack event.
//szMessage should be encoded as the URL followed by the description, the
//separator being a single nul (\0). If there is no description, do not forget
//to end the URL with two nuls.
//Will send an ack when the message actually gets sent
//type=ACKTYPE_URL, result=success/failure, lParam=0
//Protocols modules are free to define flags starting at 0x10000
//The event will *not* be added to the database automatically.
const
  PSS_URL       =   '/SendUrl';
  

//Send a request to retrieve somebody's mode message.
//wParam=lParam=0
//returns an hProcess identifying the request, or 0 on failure
//This function will fail if the contact's current status mode doesn't have an
//associated message
//The reply will be in the form of an ack:
//type=ACKTYPE_AWAYMSG, result=success/failure, lParam=(const char*)szMessage
const
  PSS_GETAWAYMSG   = '/GetAwayMsg';
  

//Sends an away message reply to a user
//wParam=(WPARAM)(HANDLE)hProcess (of ack)
//lParam=(LPARAM)(const char*)szMessage
//Returns 0 on success, nonzero on failure
//This function must only be used if the protocol has PF1_MODEMSGSEND and
//PF1_INDIVMODEMSG set. Otherwise, PS_SETAWAYMESSAGE should be used.
//This function must only be called in response to an ack that a user has
//requested our away message. The ack is sent as:
//type=ACKTYPE_AWAYMSG, result=ACKRESULT_SENTREQUEST, lParam=0
const
  PSS_AWAYMSG    = '/SendAwayMsg';
  

//Allows a file transfer to begin
//wParam=(WPARAM)(HANDLE)hTransfer
//lParam=(LPARAM)(const char*)szPath
//Returns a new handle to the transfer, to be used from now on
//If szPath does not point to a directory then:
//  if a single file is being transferred and the protocol supports file
//    renaming (PF1_CANRENAMEFILE) then the file is given this name
//  otherwise the filename is removed and the file(s) are placed in the
//    resulting directory
//File transfers are marked by an EVENTTYPE_FILE added to the database. The
//format is:
//DWORD hTransfer
//ASCIIZ filename(s), description
const
  PSS_FILEALLOW  = '/FileAllow';
  

//Refuses a file transfer request
//wParam=(WPARAM)(HANDLE)hTransfer
//lParam=(LPARAM)(const char*)szReason
//Returns 0 on success, nonzero on failure
const
  PSS_FILEDENY   = '/FileDeny';
  

//Cancel an in-progress file transfer
//wParam=(WPARAM)(HANDLE)hTransfer
//lParam=0
//Returns 0 on success, nonzero on failure
const
  PSS_FILECANCEL = '/FileCancel';
  

//Initiate a file send
//wParam=(WPARAM)(const char*)szDescription
//lParam=(LPARAM)(char **)ppszFiles
//Returns a transfer handle on success, NULL on failure
//All notification is done through acks, with type=ACKTYPE_FILE
//If result=ACKRESULT_FAILED then lParam=(LPARAM)(const char*)szReason
const
  PSS_FILE   = '/SendFile';
  

//Set the status mode you will appear in to a user
//wParam=statusMode
//lParam=0
//Returns 0 on success, nonzero on failure
//Set statusMode=0 to revert to normal behaviour for the contact
//ID_STATUS_ONLINE is possible iff PF1_VISLIST
//ID_STATUS_OFFLINE is possible iff PF1_INVISLIST
//Other modes are possible iff PF1_INDIVSTATUS
const
  PSS_SETAPPARENTMODE = '/SetApparentMode';
  

//**************************** RECEIVING SERVICES *************************/
//These services are not for calling by general modules. They serve a specific
//role in communicating through protocol module chains before the whole app is
//notified that an event has occurred.
//When the respective event is received over the network, the network-level
//protocol module initiates the chain by calling MS_PROTO_CHAINRECV with wParam
//set to zero and lParam pointing to the CCSDATA structure.
//Protocol modules should continue the message up the chain by calling
//MS_PROTO_CHAINRECV with the same wParam they received and a modified (or not)
//lParam (CCSDATA). If they do not do this and return nonzero then all further
//processing for the event will cease and the event will be ignored.
//Once all non-network protocol modules have been called (in reverse order),
//the network protocol module will be called so that it can finish its
//processing using the modified information.
//In all cases, the database should store what the user read/wrote.

//An instant message has been received
//wParam=0
//lParam=(LPARAM)(PROTORECVEVENT*)&pre
type
  TPROTORECVEVENT=record
    flags:DWord;
    timestamp:DWord;   //unix time
    szMessage:PChar;
    lParam:DWord;     //extra space for the network level protocol module
  end;
const
  PREF_CREATEREAD =  1;     //create the database event with the 'read' flag set
  PSR_MESSAGE  = '/RecvMessage';
  

//An URL has been received
//wParam=0
//lParam=(LPARAM)(PROTORECVEVENT*)&pre
//szMessage is encoded the same as for PSS_URL
const
  PSR_URL      = '/RecvUrl';
  

//File(s) have been received
//wParam=0
//lParam=(LPARAM)(PROTORECVFILE*)&prf
type
  TPROTORECVFILE=record
    flags:Dword;
    timestamp:DWord;   //unix time
    szDescription:PChar;
    pFiles:PChar;
    lParam:DWOrd;     //extra space for the network level protocol module
  end;
const
  PSR_FILE      = '/RecvFile';
  

//An away message reply has been received
//wParam=statusMode
//lParam=(LPARAM)(PROTORECVEVENT*)&pre
const
  PSR_AWAYMSG  =  '/RecvAwayMsg';


{$endif}  //M_PROTOSVC_H_

implementation

end.
