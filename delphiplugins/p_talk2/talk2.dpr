library talk2;

{
Talk2 Plugin
Version 0.1
by Christian Kästner
for Miranda ICQ 0.1.1.0+
written with Delphi 5 Pro
converted from the testplug by cyreve


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
  Windows,
  Classes,
  ComObj,
  sysutils,
  newpluginapi in '..\units\newpluginapi.pas',
  globals in '..\units\globals.pas',
  m_clist in '..\headerfiles\m_clist.pas',
  statusmodes in '..\units\statusmodes.pas',
  langpacktools in '..\units\langpacktools.pas',
  m_langpack in '..\headerfiles\m_langpack.pas',
  m_system in '..\headerfiles\m_system.pas',
  m_plugins in '..\headerfiles\m_plugins.pas',
  m_skin in '..\headerfiles\m_skin.pas',
  skintools in '..\units\skintools.pas',
  OLE2 in '..\..\..\..\..\borland\Delphi5\Source\Rtl\Win\ole2.pas',
  m_talk in '..\headerfiles\m_talk.pas',
  m_database in '..\headerfiles\m_database.pas',
  clisttools in '..\units\clisttools.pas',
  m_icq in '..\headerfiles\m_icq.pas',
  talktools in '..\units\talktools.pas';

{$R *.RES}



Const 
  //Speech Types 
  vtxtst_STATEMENT = $1; 
  vtxtst_QUESTION = $2; 
  vtxtst_COMMAND = $4; 
  vtxtst_WARNING = $8; 
  vtxtst_READING = $10; 
  vtxtst_NUMBERS = $20; 
  vtxtst_SPREADSHEET = $40; 
  //Speech Priorities 
  vtxtsp_VERYHIGH = $80; 
  vtxtsp_HIGH = $100; 
  vtxtsp_NORMAL = $200;
var
  VTxt: Variant;
  hmenuitem:THandle;


function OnModulesLoad(wParam{0},lParam{0}:DWord):integer;cdecl;forward;

function MirandaPluginInfo(mirandaVersion:DWord):PPLUGININFO;cdecl;
//Tell Miranda about this plugin
begin
  PluginInfo.cbSize:=sizeof(TPLUGININFO);
  PluginInfo.shortName:='Talk2 Plugin';
  PluginInfo.version:=PLUGIN_MAKE_VERSION(0,1,0,0);
  PluginInfo.description:='The long description of your plugin, to go in the plugin options dialog';
  PluginInfo.author:='Your Name';
  PluginInfo.authorEmail:='your@email';
  PluginInfo.copyright:='© 2001 Your Name';
  PluginInfo.homepage:='http://www.download.url';
  PluginInfo.isTransient:=0;
  PluginInfo.replacesDefaultModule:=0;  //doesn't replace anything built-in

  Result:=@PluginInfo;

  globals.mirandaVersion:=mirandaVersion;
end;

function Load(link:PPLUGINLINK):Integer;cdecl;
//load function called by miranda
begin
  PluginLink:=link^;

  //init history functions later
  PluginLink.HookEvent(ME_SYSTEM_MODULESLOADED,OnModulesLoad);

  try
    CoInitialize(nil);
    VTxt := CreateOLEObject('Speech.VoiceText');
    VTxt.Register('', ParamStr(0));
  except
    on EOLEError do;
  end;


  Result:=0;
end;

function Unload:Integer;cdecl;
//unload
begin
  VTxt.StopSpeaking;
  VTxt := varNull;

  Result:=0;
end;

procedure Speak(Text:string);
begin
  VTxt.Speak(Text, vtxtst_SPREADSHEET or vtxtsp_VERYHIGH);
end;

function GetMyStatus:integer;
begin
  Result:=ReadSettingWord(pluginlink,0,'ICQ','Status',0);
end;
function GetNick(hContact: THandle): string;
//get Nickname from hContact
begin
  Result:=string(PChar(PluginLink.CallService(MS_CLIST_GETCONTACTDISPLAYNAME,DWord(hContact),0)));
end;

function OnUserSettingChange(wParam{hContact},lParam{DBCONTACTWRITESETTING}:DWord):integer;cdecl;
//used to watch user status changes
var
  newstatus:string;
  nick:string;
  Text:string;
begin
  Result:=0;

  if StrComp(PDBCONTACTWRITESETTING(lParam)^.szModule,'ICQ')=0 then
    if StrComp(PDBCONTACTWRITESETTING(lParam)^.szSetting,'Status')=0 then
      begin
      newstatus:=miranda_ConvertStatus2Str(Word(PDBCONTACTWRITESETTING(lParam)^.Value.val));
      nick:=GetNick(wParam);
      Text:=Translate('%USER% ist %STATUS%');
      Text:=StringReplace(Text,'%USER%',nick,[]);
      Text:=StringReplace(Text,'%STATUS%',newstatus,[]);
      Speak(text);
      end;
end;


function OnStatusModeChange(wParam{NewStatus},lParam{0}:DWord):integer;cdecl;
var
  statusmode:string;
  sm:integer;
begin
  Result:=0;
  sm:=getmystatus;
  if sm<MAX_CONNECT_RETRIES then
    Exit;

  statusmode:=miranda_ConvertStatus2Str(wParam);

  Speak(StringReplace('Bin %STATUS%','%STATUS%',statusmode,[]));
end;

function DoSpeakText(wParam{Text:PChar},lParam{0}:DWord):integer;cdecl;
var
  Text:string;
begin
  Text:=string(PChar(wParam));
  Speak(Text);
  Result:=0;
end;

function DoStopSpeak(wParam{0},lParam{0}:DWord):integer;cdecl;
begin
  VTxt.StopSpeak;
end;


function OnMessageEventAdded(wParam{hContact},lParam{hdbEvent}:DWord):integer;cdecl;
//when a new item was added to the database read it
var
  BlobSize:Integer;
  dbei:TDBEVENTINFO;

  Text:string;
  msgtext:string;
  nick:string;
begin
  Result:=0;

  dbei.cbSize:=sizeof(dbei);
  //alloc memory for message
  blobsize:=PluginLink.CallService(MS_DB_EVENT_GETBLOBSIZE,lParam{hDbEvent},0);
  getmem(dbei.pBlob,blobsize);
  try
  dbei.cbBlob:=BlobSize;
  //getmessage
  PluginLink.CallService(MS_DB_EVENT_GET,lParam{hdbEvent},DWord(@dbei));


  if ((dbei.flags and DBEF_SENT)<>0) or //send yourself
    ((dbei.flags and DBEF_READ)<>0) or //already read?
    (StrComp(dbei.szModule,'ICQ')<>0) or
    (dbei.eventType<>EVENTTYPE_MESSAGE) then
    Exit;

  Text:=translate('Message from %USER%: %TEXT%');

  msgtext:=string(PChar(dbei.pBlob));
  nick:=GetNick(wParam);

  Text:=StringReplace(Text,'%USER%',nick,[]);
  Text:=StringReplace(Text,'%TEXT%',msgtext,[]);
  Speak(text);


  finally
  freemem(dbei.pBlob);
  end;
end;



function OnModulesLoad(wParam{0},lParam{0}:DWord):integer;cdecl;
//init plugin
var
  menuitem:TCLISTMENUITEM;
begin
  //register service for showing history

  PluginLink.HookEvent(ME_CLIST_STATUSMODECHANGE,OnStatusModeChange);
  PluginLink.HookEvent(ME_DB_CONTACT_SETTINGCHANGED,OnUserSettingChange);
  PluginLink.HookEvent(ME_DB_EVENT_ADDED,OnMessageEventAdded);

  PluginLink.CreateServiceFunction(MS_TALK_SPEAKTEXT,DoSpeakText);
  PluginLink.CreateServiceFunction(MS_TALK_STOPSPEAK,DoStopSpeak);

{  //create menu item in contact menu
  menuitem.cbSize:=sizeof(menuitem);
  menuitem.Position:=1;
  menuitem.flags:=0;
  menuitem.hIcon:=0;
  menuitem.pszContactOwner:=nil;    //all contacts
  menuitem.pszName:='Stop Talk!';
  menuitem.pszService:=MS_TALK_STOPSPEAK;
  hmenuitem:=PluginLink.CallService(MS_CLIST_ADDMAINMENUITEM,0,DWord(@menuitem));}


  Result:=0;
end;


exports
  MirandaPluginInfo,
  Load,
  Unload;


begin
end.

