library pingdetect;

{
Ping Detect
Version 0.0.0.1
by Christian Kästner
for Miranda ICQ 0.1.0.0
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
  Windows,
  Classes,
  messages,
  sysutils,
  newpluginapi in '..\units\newpluginapi.pas',
  globals in '..\units\globals.pas',
  m_clist in '..\headerfiles\m_clist.pas',
  statusmodes in '..\units\statusmodes.pas',
  m_system in '..\headerfiles\m_system.pas',
  m_plugins in '..\headerfiles\m_plugins.pas',
  m_skin in '..\headerfiles\m_skin.pas',
  skintools in '..\units\skintools.pas',
  pingunit in 'pingunit.pas',
  clisttools in '..\units\clisttools.pas',
  m_database in '..\headerfiles\m_database.pas',
  m_icq in '..\headerfiles\m_icq.pas',
  options in 'options.pas',
  m_options in '..\headerfiles\m_options.pas';

{$R *.RES}

var
  timerhandle:integer;
  pingobject:TPingObject;


function MirandaPluginInfo(mirandaVersion:DWord):PPLUGININFO;cdecl;
//Tell Miranda about this plugin
begin
  PluginInfo.cbSize:=sizeof(TPLUGININFO);
  PluginInfo.shortName:='Ping Detect';
  PluginInfo.version:=PLUGIN_MAKE_VERSION(1,0,0,0);
  PluginInfo.description:='Detects if an online connection is available by pinging a host...';
  PluginInfo.author:='Christian Kästner';
  PluginInfo.authorEmail:='christian.k@stner.de';
  PluginInfo.copyright:='© 2001 Christian Kästner';
  PluginInfo.homepage:='http://www.kaestnerpro.de/pingdetect.zip';
  PluginInfo.isTransient:=0;
  PluginInfo.replacesDefaultModule:=0;  //doesn't replace anything built-in

  Result:=@PluginInfo;
end;

function OnModulesLoad(wParam,lParam:DWord):integer;cdecl;forward;

function Load(link:PPLUGINLINK):Integer;cdecl;
//load function called by miranda
begin
  PluginLink:=link^;

  //init history functions later
  PluginLink.HookEvent(ME_SYSTEM_MODULESLOADED,OnModulesLoad);

  Result:=0;
end;

function Unload:Integer;cdecl;
//unload
begin
  killtimer(0,timerhandle);
  if Assigned(pingobject) then
    FreeAndNil(pingobject);
  Result:=0;
end;

procedure PingReadyEvent(Sender:TObject);
var
  currentstatus:integer;
begin
  if not Assigned(Sender) then
    Exit;

  currentstatus:=PluginLink.CallService(MS_CLIST_GETSTATUSMODE,0,0);

  if Tpingobject(Sender).Success>=0.6 then//min 60% success
    begin
    if currentstatus=ID_STATUS_OFFLINE then
      PluginLink.CallService(MS_CLIST_SETSTATUSMODE,ID_STATUS_ONLINE,0);
    end
  else
    begin
    if currentstatus<>ID_STATUS_OFFLINE then
      PluginLink.CallService(MS_CLIST_SETSTATUSMODE,ID_STATUS_OFFLINE,0);
    end;
end;

procedure TimerProc(Wnd: HWnd; Msg, TimerID, SysTime: Longint);stdcall;
begin
  if Assigned(pingobject) then
    FreeAndNil(pingobject);
  pingobject:=TPingObject.Create;
  pingobject.Callback:=@PingReadyEvent;


  pingobject.StartPingHost(ReadSettingStr(PluginLink,0,'PingDetect','PingTarget','heise.de'),ReadSettingInt(PluginLink,0,'PingDetect','Timeout',1000),5);
end;


function OnOptInitialise(wParam{addinfo},lParam{0}:DWord):integer;cdecl;
var
  odp:TOPTIONSDIALOGPAGE;
begin
  ZeroMemory(@odp,sizeof(odp));
  odp.cbSize:=sizeof(odp);
  odp.Position:=900002000;
  odp.hInstance:=hInstance;
  odp.pszTemplate:='DLG_PINGOPTIONS';
  odp.pszTitle:='Ping Detect';
  odp.pfnDlgProc:=@options.DlgProcOptions;
  PluginLink.CallService(MS_OPT_ADDPAGE,wParam,dword(@odp));

  Result:=0;
end;


function OnModulesLoad(wParam{0},lParam{0}:DWord):integer;cdecl;
//init plugin
begin
  //set timer for regular online checking...
  timerhandle:=settimer(0,0,ReadSettingInt(PluginLink,0,'PingDetect','Interval',30)*1000,@TimerProc);

  PluginLink.HookEvent(ME_OPT_INITIALISE,OnOptInitialise);

  Result:=0;
end;




exports
  MirandaPluginInfo,
  Load,
  Unload;


begin
end.

