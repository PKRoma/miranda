library logger2;

{ Important note about DLL memory management: ShareMem must be the
  first unit in your library's USES clause AND your project's (select
  Project-View Source) USES clause if your DLL exports any procedures or
  functions that pass strings as parameters or function results. This
  applies to all strings passed to and from your DLL--even those that
  are nested in records and classes. ShareMem is the interface unit to
  the BORLNDMM.DLL shared memory manager, which must be deployed along
  with your DLL. To avoid using BORLNDMM.DLL, pass string information
  using PChar or ShortString parameters. }

uses
  windows,
  SysUtils,
  Classes,
  log,
  globals in '..\units\globals.pas',
  newpluginapi in '..\units\newpluginapi.pas',
  m_icq,
  m_system;

{$DEFINE LOG}
{$R *.RES}


var
  hookhandle:dword;

function MirandaPluginInfo(mirandaVersion:DWord):PPLUGININFO;cdecl;
//Tell Miranda about this plugin
begin
  PluginInfo.cbSize:=sizeof(TPLUGININFO);
  PluginInfo.shortName:='Logger2';
  PluginInfo.version:=PLUGIN_MAKE_VERSION(1,0,0,0);
  PluginInfo.description:='ICQ Logger for Miranda 0.1 (as replacement for the old logger plugins for former Miranda versions)';
  PluginInfo.author:='Christian Kästner';
  PluginInfo.authorEmail:='christian.k@stner.de';
  PluginInfo.copyright:='© 2001 Christian Kästner';
  PluginInfo.homepage:='http://www.kaestnerpro.de/logger2.zip';
  PluginInfo.isTransient:=0;
  PluginInfo.replacesDefaultModule:=0;
  PluginInfoVersion:='1.0';

  Result:=@PluginInfo;
end;

function OnModulesLoad(wParam,lParam:DWord):integer;cdecl;forward;

function Load(link:PPLUGINLINK):Integer;cdecl;
//load function called by miranda
begin
  PluginLink:=link^;

  CreateLog('.\miranda.icq.log');
  hookhandle:=0;

  PluginLink.HookEvent(ME_SYSTEM_MODULESLOADED,OnModulesLoad);

  Result:=0;
end;

function Unload:Integer;cdecl;
//unload
begin
  if hookhandle<>0 then
    PluginLink.UnhookEvent(hookhandle);
  FreeLog;

  Result:=0;
end;

function OnLog(wParam{Level},lParam{Message}:DWord):integer;cdecl;
//init plugin
var
  Msg:string;
begin
  Msg:=string(PChar(lParam));
  Msg:=trim(Msg);
  log.LogText(IntToStr(Byte(wParam))+' '+msg);
  Result:=0;
end;

function OnModulesLoad(wParam{0},lParam{0}:DWord):integer;cdecl;
//init plugin
begin
  //register service for showing history
  hookhandle:=PluginLink.HookEvent(ME_ICQ_LOG,OnLog);
  Result:=0;
end;


exports
  Unload,
  Load,
  MirandaPluginInfo;

begin
end.
