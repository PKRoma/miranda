unit m_clui;

interface

(*
Miranda ICQ: the free icq client for MS Windows
Copyright (C) 2000-1  Richard Hughes, Roland Rabien & Tristan Van de Vreede

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*)

//this module was created in 0.1.1.0
//you probably shouldn't need to call anything in here. Look in
//ui/contactlist/m_clist.h instead

//gets the handle for the contact list window
//wParam=lParam=0
//returns the HWND
//This call has a few very specific purposes internally in Miranda, and
//shouldn't be gratuitously used. In almost all cases there's another call to
//do whatever it is you are trying to do.
const
  MS_CLUI_GETHWND='CLUI/GetHwnd';
  

//change protocol-specific status indicators
//wParam=new status
//lParam=(LPARAM)(const char*)szProtocolID
//returns 0 on success, nonzero on failure
//protocol modules don't want to call this. They want
//clist/protocolstatuschanged instead
const
  MS_CLUI_PROTOCOLSTATUSCHANGED='CLUI/ProtocolStatusChanged';
  

//a new group was created. Add it to the list
//wParam=(WPARAM)(HANDLE)hGroup
//lParam=newGroup
//returns 0 on success, nonzero on failure
//newGroup is set to 1 if the user just created the group, and 0 otherwise
//this is also called when the contact list is being rebuilt
//new groups are always created with the name='New Group';

const
  MS_CLUI_GROUPADDED='CLUI/GroupCreated';
  

//change the icon for a contact
//wParam=(WPARAM)(HANDLE)hContact
//lParam=iconid
//returns 0 on sucess, nonzero on failure
//iconid is an offset in the image list. see clist/geticonsimagelist
const
  MS_CLUI_CONTACTSETICON='CLUI/ContactSetIcon';
  

//remove a contact from the list
//wParam=(WPARAM)(HANDLE)hContact
//lParam=0
//returns 0 on success, nonzero on failure
//this is not necessarily the same as a contact being actually deleted, since
//if a contact goes offline while 'hide offline' is on, this will be called
const
  MS_CLUI_CONTACTDELETED='CLUI/ContactDeleted';
  

//add a contact to the list
//wParam=(WPARAM)(HANDLE)hContact
//lParam=iconId
//returns 0 on success, nonzero on failure
//The caller processes the 'hide offline' setting, so the callee should not do
//further processing based on the value of this setting.
//warning: this will be called to re-add a contact when they come online if
//'hide offline' is on, but it cannot determine if the contact is already on
//the list, so you may get requests to add a contact when it is already on the
//list, which you should ignore.
//You'll also get this whenever an event is added for a contact, since if the
//contact was offline it needs to be shown to display the message, even if hide
//offline is on.
//You should not re-sort the list on this call. A separate resort request will
//be sent
//iconid is an offset in the image list. see clist/geticonsimagelist
const
  MS_CLUI_CONTACTADDED='CLUI/ContactAdded';
  

//rename a contact in the list
//wParam=(WPARAM)(HANDLE)hContact
//lParam=0
//returns 0 on success, nonzero on failure
//you should not re-sort the list on this call. A separate resort request will
//be sent
//you get the new name from clist/getcontactdisplayname
const
  MS_CLUI_CONTACTRENAMED='CLUI/ContactRenamed';
  

//start a rebuild of the contact list
//wParam=lParam=0
//returns 0 on success, nonzero on failure
//this is the cue to clear the existing contents of the list
//expect to get a series of clui/groupadded calls followed by a series of
//clui/contactadded calls, then a clui/resortlist
const
  MS_CLUI_LISTBEGINREBUILD='CLUI/ListBeginRebuild';
  

//end a rebuild of the contact list
//wParam=lParam=0
//returns 0 on success, nonzero on failure
//if you displayed an hourglass in beginrebuild, set it back here
//you do not need to explicitly sort the list
const
  MS_CLUI_LISTENDREBUILD='CLUI/ListEndRebuild';
  

//sort the contact list now
//wParam=lParam=0
//returns 0 on success, nonzero on failure
//sorts are buffered so you won't get this message lots of times if the list
//needs to be re-sorted many times rapidly.
const
  MS_CLUI_SORTLIST='CLUI/SortList';
  

implementation

end.
