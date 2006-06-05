// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004,2005 Martin Öberg, Sam Kothari, Robert Rainwater
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
//
// -----------------------------------------------------------------------------
//
// File name      : $Source: /cvsroot/miranda/miranda/include/m_icq.h,v $
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#ifndef M_ICQ_H__
#define M_ICQ_H__ 1


// Note: In v0.3 the part before "/Servicename" is dynamic. It will be the name of the protocol.
// Example: If the plugin was loaded from ICQ.dll, the service name is "ICQ/Servicename", and if
// the dll was Icq2.dll, the service name will be "Icq2/Servicename". This behaviour is temporary
// until proper multiaccounts are implemented.


//start a search of all ICQ users by e-mail
//wParam=0
//lParam=(LPARAM)(const char*)email
//returns a handle to the search on success, NULL on failure
//Results are returned using the same scheme documented in PSS_BASICSEARCH
//**DEPRECATED** in favour of PS_SEARCHBYEMAIL
typedef struct {   //extended search result structure, used for all searches
	PROTOSEARCHRESULT hdr;
	DWORD uin;
	BYTE auth;
} ICQSEARCHRESULT;
#define MS_ICQ_SEARCHBYEMAIL   "/SearchByEmail"

//start a search of all ICQ users by details
//wParam=0
//lParam=(LPARAM)(ICQDETAILSSEARCH*)&ids
//returns a handle to the search on success, NULL on failure
//Results are returned using the same scheme documented in PSS_BASICSEARCH
//**DEPRECATED** in favour of PS_SEARCHBYNAME
typedef struct {
	char *nick;
	char *firstName;
	char *lastName;
} ICQDETAILSSEARCH;
#define MS_ICQ_SEARCHBYDETAILS   "/SearchByDetails"

// Request authorization
// wParam=(WPARAM)hContact
#define MS_REQ_AUTH "/ReqAuth"

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
//"<sms_response><source>airbornww.com</source><deliverable>Yes</deliverable><network>BT Cellnet, United Kingdom</network><message_id>[my uin]-1-1955988055-[destination phone#, without +]</message_id><messages_left>0</messages_left></sms_response>\r\n"

//Now the hProcess has been deleted. The only way to track which receipt
//corresponds with which response is to parse the <message_id> field.

//At a (possibly much) later time the SMS will have been delivered. An ack will
//be broadcast:
// type=ICQACKTYPE_SMS, result=ACKRESULT_SUCCESS, hProcess=NULL, lParam=(LPARAM)(char*)szInfo
//Note that the result will always be success even if the send failed, just to
//save needing to have an attempt at an XML parser in the ICQ module.
//Here's the szInfo for a success:
//"<sms_delivery_receipt><message_id>[my uin]-1--1461632229-[dest phone#, without +]</message_id><destination>[dest phone#, without +]</destination><delivered>Yes</delivered><text>[first 20 bytes of message]</text><submition_time>Tue, 30 Oct 2001 22:35:16 GMT</submition_time><delivery_time>Tue, 30 Oct 2001 22:34:00 GMT</delivery_time></sms_delivery_receipt>"
//And here's a failure:
//"<sms_delivery_receipt><message_id>[my uin]-1-1955988055-[destination phone#, without leading +]</message_id><destination>[destination phone#, without leading +]</destination><delivered>No</delivered><submition_time>Tue, 23 Oct 2001 23:17:02 GMT</submition_time><error_code>999999</error_code><error><id>15</id><params><param>0</param><param>Multiple message submittion failed</param></params></error></sms_delivery_receipt>"

//SMSes received from phones come through this same ack, again to avoid having
//an XML parser in the protocol module. Here's one I got:
//"<sms_message><source>MTN</source><destination_UIN>[UIN of recipient, ie this account]</destination_UIN><sender>[sending phone number, without +]</sender><senders_network>[contains one space, because I sent from ICQ]</senders_network><text>[body of the message]</text><time>Fri, 16 Nov 2001 03:12:33 GMT</time></sms_message>"
#define ICQACKTYPE_SMS      1001
#define ICQEVENTTYPE_SMS    2001	  //database event type
#define MS_ICQ_SENDSMS      "/SendSMS" 

//e-mail express
//db event added to NULL contact
//blob format is:
//ASCIIZ    text, usually of the form "Subject: %s\r\n%s"
//ASCIIZ    from name
//ASCIIZ    from e-mail
#define ICQEVENTTYPE_EMAILEXPRESS 2002    //database event type

//www pager
//db event added to NULL contact
//blob format is:
//ASCIIZ    text, usually "Sender IP: xxx.xxx.xxx.xxx\r\n%s"
//ASCIIZ    from name
//ASCIIZ    from e-mail
#define ICQEVENTTYPE_WEBPAGER 2003    //database event type

//for server-side lists, used internally only
//hProcess=dwSequence
//lParam=server's error code, 0 for success
#define ICQACKTYPE_SERVERCLIST  1003

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
#define ICQCHANGEINFO_MAIN     0xEA03
/* pInfoData points to:
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
*/
#define ICQCHANGEINFO_MORE     0xFD03
/* pInfoData points to:
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
*/
#define ICQCHANGEINFO_ABOUT	   0x0604
/* pInfoData points to:
    WORD    datalen
	LNTS    about
*/
#define ICQCHANGEINFO_WORK	   0xF303
/* pInfoData points to:
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
*/
#define ICQCHANGEINFO_PASSWORD 0x2E04
/* pInfoData points to:
    WORD    datalen
	LNTS    newpassword
*/

#define ICQCHANGEINFO_SECURITY 0x2404
/* pInfoData points to:
    BYTE    AUTH     0x01 - don't require authorization
                     0x00 - require authorization to add to contact list  
    BYTE    WEBAWARE 0x01 - disallow seeing status on the web
                     0x00 - be web aware  
    BYTE    DIRECT   0x00 - allow direct connection with any user.
                     0x01 - allow direct connection with users on the contact list.
                     0x02 - allow direct connections only upon authorization.  
    BYTE    KIND     User kind (unknown, use 0)
*/

//miranda/icqoscar/statusmsgreq event
//called when our status message is requested
//wParam=(BYTE)msgType
//lParam=(DWORD)uin
//msgType is one of the ICQ_MSGTYPE_GET###MSG constants in icq_constants.h
//uin is the UIN of the contact requesting our status message
#define ME_ICQ_STATUSMSGREQ			"/StatusMsgReq"

#endif // M_ICQ_H__

