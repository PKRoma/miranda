library convers;

{
Conversation Style Messaging Plugin 
Version 0.99.3
by Christian Kästner
for Miranda ICQ 0.1
written with Delphi 5 Pro


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


For more information, e-mail christian.k@stner.de
}


uses
  SysUtils,
  Classes,
  windows,
  dialogs,
  windowmanager in 'windowmanager.pas',
  msgform in 'msgform.pas' {MsgWindow},
  optionfrm in 'optionfrm.pas' {OptionForm},
  clisttools in '..\units\clisttools.pas',
  globals in '..\units\globals.pas',
  newpluginapi in '..\units\newpluginapi.pas',
  statusmodes in '..\units\statusmodes.pas',
  m_system in '..\headerfiles\m_system.pas',
  m_database in '..\headerfiles\m_database.pas',
  m_history in '..\headerfiles\m_history.pas',
  m_icq in '..\headerfiles\m_icq.pas',
  m_plugins in '..\headerfiles\m_plugins.pas',
  m_clist in '..\headerfiles\m_clist.pas',
  m_skin in '..\headerfiles\m_skin.pas',
  skintools in '..\units\skintools.pas',
  databasetools in '..\units\databasetools.pas',
  m_userinfo in '..\headerfiles\m_userinfo.pas',
  forms,
  misc in 'misc.pas',
  m_options in '..\headerfiles\m_options.pas',
  timeoutfrm in 'timeoutfrm.pas' {TimeoutForm},
  m_email in '..\headerfiles\m_email.pas',
  optionsd in 'optionsd.pas';





{$R *.RES}
{ $R optdlgs.res}




function MirandaPluginInfo(mirandaVersion:DWord):PPLUGININFO;cdecl;
//Tell Miranda about this plugin
begin
  globals.MirandaVersion:=mirandaVersion;

  ZeroMemory(@PluginInfo,SizeOf(PluginInfo));
  PluginInfo.cbSize:=sizeof(TPLUGININFO);
  PluginInfo.shortName:='Conversation Style Messaging';
  PluginInfo.version:=PLUGIN_MAKE_VERSION(0,99,3,0);
  PluginInfo.description:='This plugin offers a conversation style messaging ability for Miranda ICQ. Like most instant message programs you see the history above the input field. Additionally it has a 3 different display stiles and couple of nice features and options.';
  PluginInfo.author:='Christian Kästner';
  PluginInfo.authorEmail:='christian.k@stner.de';
  PluginInfo.copyright:='© 2001 Christian Kästner';
  PluginInfo.homepage:='http://www.kaestnerpro.de/convers.zip';
  PluginInfo.isTransient:=0;
  PluginInfo.replacesDefaultModule:=DEFMOD_SRMESSAGE;
  PluginInfoVersion:='0.993';

  Result:=@PluginInfo;
end;


function OnModulesLoad(wParam,lParam:DWord):integer;cdecl;forward;

function Load(link:PPLUGINLINK):Integer;cdecl;
//load function called by miranda
begin
  PluginLink:=link^;

  windowmgr:=twindowmanager.Create;
  blinkid:=0;

  //init history functions later
  PluginLink.HookEvent(ME_SYSTEM_MODULESLOADED,OnModulesLoad);

  Result:=0;
end;

function Unload:Integer;cdecl;
//unload plugin
begin
  windowmgr.Free;
  Result:=0;
end;



function LaunchMessageWindow(hcontact:thandle;NewMessage:Boolean=False):TMsgWindow;forward;

function OnLaunchMessageWindow(wParam{Contact},lParam{0}:DWord):integer;cdecl;
//brings to front an existing conversation dialog for hContact or creates a new one
//called when user doubleclicks the contact or clicks send in the user's context menu
var
  msgwindow:TMsgWindow;
begin
  msgwindow:=LaunchMessageWindow(wParam);

  msgwindow.Show;
  msgwindow.BringWindowToFront;
  msgwindow.SetFocus;
  setfocus(msgwindow.Handle);

  Result:=0;
end;

function OnReadBlinkMessage(wParam{Contact},lParam{BlinkID}:DWord):integer;cdecl;
//brings to front a hided dialog when incoming messages blink only on contactlist
var
  msgwindow:TMsgWindow;
begin
  msgwindow:=LaunchMessageWindow(PCLISTEVENT(lParam)^.hContact);

  msgwindow.Show;
  msgwindow.BringWindowToFront;
  msgwindow.SetFocus;

  Result:=0;
end;

function OnMessageEventAdded(wParam{hContact},lParam{hdbEvent}:DWord):integer;cdecl;
//when a new item was added to the database show it (it's an incoming or outgoing
//message)
//main function for showing new incoming messages
var
  MsgWindow:TMsgWindow;

  BlobSize:Integer;
  dbei:TDBEVENTINFO;

  icqmessage:TIcqMessage;
begin
  Result:=0;
  if blinkid=0 then
    blinkid:=lParam;

  dbei.cbSize:=sizeof(dbei);
  //alloc memory for message
  blobsize:=PluginLink.CallService(MS_DB_EVENT_GETBLOBSIZE,lParam{hDbEvent},0);
  getmem(dbei.pBlob,blobsize);
  dbei.cbBlob:=BlobSize;
  //getmessage
  PluginLink.CallService(MS_DB_EVENT_GET,lParam{hdbEvent},DWord(@dbei));

  if //((dbei.flags and DBEF_SENT)<>0) or //send yourself
    (StrComp(dbei.szModule,'ICQ')<>0) or
    (dbei.eventType<>EVENTTYPE_MESSAGE) then
    Exit;

  //mark message read etc.
  if not (dbei.eventType=EVENTTYPE_MESSAGE) then
    Exit;

  //if unread
  if ((dbei.flags and DBEF_READ)=0) then
    begin
    //mark read
    PluginLink.CallService(MS_DB_EVENT_MARKREAD,wParam{hContact},lParam{hDbEvent});
    //remove flashing icon for incoming message at the contact list
    PluginLink.CallService(MS_CLIST_REMOVEEVENT,wParam{hContact},lParam{hDbEvent});
    end;
  //if incoming message
  if (dbei.flags and DBEF_SENT=0) then
    icqmessage.hContact:=wParam{hContact}
  else//if outgoing message
    icqmessage.hContact:=0;

  icqmessage.Msg:=string(PChar(dbei.pBlob));

  icqmessage.MSGTYPE:=EVENTTYPE_MESSAGE;
  icqmessage.Time:=dbei.timestamp;
  icqmessage.Recent:=False;

  //workaround for "CloseWindowAfterSend" feature
  if icqmessage.hContact=0 then
    begin
    MsgWindow:=windowmgr.GetMsgWindow(wParam{hContact});
    if Assigned(msgwindow) then
      if MsgWindow.fCloseWindowAfterSend then
        Exit;
    end;
  MsgWindow:=LaunchMessageWindow(wParam{hContact},True);
  MsgWindow.ShowMessage(icqmessage);

  //only bring to front/play sound on incoming message
  if icqmessage.hContact<>0 then
    begin
    MsgWindow.ActivateWindow;


    SkinPlaySound(PluginLink,'RecvMsg');
    end;
end;

function OnUserSettingChage(wParam{hContact},lParam{DBCONTACTWRITESETTING}:DWord):integer;cdecl;
//used to watch user status changes
var
  msgwindow:Tmsgwindow;
begin
  Result:=0;

  if StrComp(PDBCONTACTWRITESETTING(lParam)^.szModule,'ICQ')=0 then
    if StrComp(PDBCONTACTWRITESETTING(lParam)^.szSetting,'Status')=0 then
      begin
      msgwindow:=windowmgr.GetMsgWindow(wParam{hContact});
      if Assigned(msgwindow) then
        msgwindow.ShowUserStatus(Word(PDBCONTACTWRITESETTING(lParam)^.Value.val));
      end;
end;

function OnOptInitialise(wParam{addinfo},lParam{0}:DWord):integer;cdecl;
var
  odp:TOPTIONSDIALOGPAGE;
begin
{ ZeroMemory(@odp,sizeof(odp));
  odp.cbSize:=sizeof(odp);
  odp.Position:=900000000;
  odp.hInstance:=hInstance;
  odp.pszTemplate:='DLG_MISCOPTIONS';
  odp.pszTitle:='Convers. Plugin';
  odp.pfnDlgProc:=@optionfrm.DlgProcMiscOptions;
  PluginLink.CallService(MS_OPT_ADDPAGE,wParam,dword(@odp));}
  Result:=0;
end;


function OnModulesLoad(wParam{0},lParam{0}:DWord):integer;cdecl;
//init plugin
var
  menuitem:TCLISTMENUITEM;
  dca:TCLISTDOUBLECLICKACTION;
begin
  //register two callback services, called when one of the menu items is clicked
  PluginLink.CreateServiceFunction(PluginService_LaunchMessageWindow,OnLaunchMessageWindow);
  PluginLink.CreateServiceFunction(PluginService_ReadBlinkMessage,OnReadBlinkMessage);


  PluginLink.HookEvent(ME_DB_EVENT_ADDED,OnMessageEventAdded);
  PluginLink.HookEvent(ME_DB_CONTACT_SETTINGCHANGED,OnUserSettingChage);
  PluginLink.HookEvent(ME_OPT_INITIALISE,OnOptInitialise);

  //create menu item in contact menu
  menuitem.cbSize:=sizeof(menuitem);
  menuitem.Position:=-2000090000;
  menuitem.flags:=0;
  menuitem.pszName:='&Message...';
  menuitem.pszContactOwner:='ICQ';//only for ICQ contacts
  menuitem.pszService:=PluginService_LaunchMessageWindow;//callback this function when menuitem is clicked
  PluginLink.CallService(MS_CLIST_ADDCONTACTMENUITEM,0,DWord(@menuitem));

  //Doubleclick
  dca.cbSize:=sizeof(dca);
  dca.flags:=0;    //on every icq contact
  dca.pszContactOwner:='ICQ';  //icq contacts only
  dca.pszService:=PluginService_LaunchMessageWindow;//callback this function when doubleclick
  PluginLink.CallService(MS_CLIST_SETDOUBLECLICKACTION,0,DWord(@dca));

  SkinAddNewSound(PluginLink,'RecvMsg','Incoming Message','message.wav');

  Result:=0;
end;


function LaunchMessageWindow(hcontact:THandle;NewMessage:Boolean=False):TMsgWindow;
//used to launch a window for an incoming message
//(checkes if window exists otherwise creates and initializes it)
var
  msgwindow:TMsgWindow;
begin
  //get/create window
  Assert(Assigned(windowmgr));
  msgwindow:=windowmgr.GetMsgWindow(hcontact);
  if not Assigned(msgwindow) then
    begin
    msgwindow:=windowmgr.CreateMsgWindow(hcontact);
    //show userinfo in window
    msgwindow.ShowUserStatus(-1);
    msgwindow.LoadRecentMessages(NewMessage);
    end;

  //do not show the window yet!

  Result:=msgwindow;
end;



exports
  Load,
  Unload,
  MirandaPluginInfo;

begin

end.
