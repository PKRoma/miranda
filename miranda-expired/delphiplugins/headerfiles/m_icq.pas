{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : m_icq
 * Description : Converted Headerfile from m_icq.h
 *
 * Author      : Christian Kästner
 * Date        : 28.04.2001
 * Last Change : 22.12.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_icq;

interface

uses windows, m_protosvc;




//start a search of all ICQ users by e-mail
//wParam=0
//lParam=(LPARAM)(const char*)email
//returns a handle to the search on success, NULL on failure
//Results are returned using the same scheme documented in PSS_BASICSEARCH
type
  ICQSEARCHRESULT=record   //extended search result structure, used for all searches
    hdr:TPROTOSEARCHRESULT;
    uin:DWORD;
    auth:byte;
  end;
const
  MS_ICQ_SEARCHBYEMAIL  ='ICQ/SearchByEmail';

//start a search of all ICQ users by details
//wParam=0
//lParam=(LPARAM)(ICQDETAILSSEARCH*)&ids
//returns a handle to the search on success, NULL on failure
//Results are returned using the same scheme documented in PSS_BASICSEARCH
type
  ICQDETAILSSEARCH=record
    nick:PCHAR;
    firstName:PCHAR;
    lastName:PCHAR;
  end;
const
  MS_ICQ_SEARCHBYDETAILS  ='ICQ/SearchByDetails';

//Send an SMS via the ICQ network
//wParam=(WPARAM)(const char*)szPhoneNumber
//lParam=(LPARAM)(const char*)szMessage
//Returns a HANDLE to the send on success, or NULL on failure
//szPhoneNumber should be the full number with international code and preceeded
//by a +

//When the server acks the send, an ack will be broadcast:
// type=ICQACKTYPE_SMS, result=ACKRESULT_SENTREQUEST, lParam=(LPARAM)(char*)szInfo
//At this point the message is queued to be delivered. szInfo contains the raw
//XML data of the ack. Here's what I got when I tried:
//"<sms_response><source>airbornww.com</source><deliverable>Yes</deliverable><network>BT Cellnet, United Kingdom</network><message_id>[my uin]-1-1955988055-[destination phone#, without +]</message_id><messages_left>0</messages_left></sms_response>\r\n';

//Now the hProcess has been deleted. The only way to track which receipt
//corresponds with which response is to parse the <message_id> field.

//At a (possibly much) later time the SMS will have been delivered. An ack will
//be broadcast:
// type=ICQACKTYPE_SMS, result=ACKRESULT_SUCCESS, hProcess=NULL, lParam=(LPARAM)(char*)szInfo
//Note that the result will always be success even if the send failed, just to
//save needing to have an attempt at an XML parser in the ICQ module.
//Here's the szInfo for a success:
//"<sms_delivery_receipt><message_id>[my uin]-1--1461632229-[dest phone#, without +]</message_id><destination>[dest phone#, without +]</destination><delivered>Yes</delivered><text>[first 20 bytes of message]</text><submition_time>Tue, 30 Oct 2001 22:35:16 GMT</submition_time><delivery_time>Tue, 30 Oct 2001 22:34:00 GMT</delivery_time></sms_delivery_receipt>';
//And here's a failure:
//"<sms_delivery_receipt><message_id>[my uin]-1-1955988055-[destination phone#, without leading +]</message_id><destination>[destination phone#, without leading +]</destination><delivered>No</delivered><submition_time>Tue, 23 Oct 2001 23:17:02 GMT</submition_time><error_code>999999</error_code><error><id>15</id><params><param>0</param><param>Multiple message submittion failed</param></params></error></sms_delivery_receipt>';

//SMSes received from phones come through this same ack, again to avoid having
//an XML parser in the protocol module. Here's one I got:
//"<sms_message><source>MTN</source><destination_UIN>[UIN of recipient, ie this account]</destination_UIN><sender>[sending phone number, without +]</sender><senders_network>[contains one space, because I sent from ICQ]</senders_network><text>[body of the message]</text><time>Fri, 16 Nov 2001 03:12:33 GMT</time></sms_message>';
CONST
  ICQACKTYPE_SMS      =1001;
  ICQEVENTTYPE_SMS    =2001;	  //database event type
const
  MS_ICQ_SENDSMS     ='ICQ/SendSMS';

//Changing user info:
//See documentation of PS_CHANGEINFO
//The changing user info stuff built into the protocol is purposely extremely
//thin, to the extent that your data is passed as-is to the server without
//verification. Don't mess up.
//Everything is byte-aligned
//WORD:  2 bytes, little-endian (that's x86 order)
//DWORD: 4 bytes, little-endian
//LNTS:  a WORD containing the length of the string, followed by the string
//       itself. No zero terminator.
const
  ICQCHANGEINFO_MAIN     =$EA03;
(* pInfoData points to:
    WORD    datalen
    LNTS    nick
    LNTS    first
    LNTS    last
    LNTS    email
    LNTS    city
    LNTS    state
    LNTS    phone
    LNTS    fax
    LNTS    street
    LNTS    cellular (if SMS-able string contains an ending ' SMS')
    LNTS    zip
    WORD    country
    BYTE    gmt
    BYTE    unknown, usually 0
*)
const
  ICQCHANGEINFO_MORE     =$FD03;
(* pInfoData points to:
    WORD    datalen
    BYTE    age
    BYTE    0
    BYTE    sex
    LNTS    homepage
    WORD    birth-year
    BYTE    birth-month
    BYTE    birth-day
    BYTE    lang1
    BYTE    lang2
    BYTE    lang3
*)
const
  ICQCHANGEINFO_ABOUT	   =$0604;
(* pInfoData points to:
    WORD    datalen
	LNTS    about
*)
const
  ICQCHANGEINFO_WORK	   =$F303;
(* pInfoData points to:
    WORD    datalen
    LNTS    city
    LNTS    state
    DWORD   0
    LNTS    street
    LNTS    zip
    WORD    country
    LNTS    company-name
    LNTS    company-dept
    LNTS    company-position
    WORD    0
    LNTS    company-web
*)
const
  ICQCHANGEINFO_PASSWORD =$2E04;
(* pInfoData points to:
    WORD    datalen
	LNTS    newpassword
*)









implementation

end.
