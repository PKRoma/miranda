{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : langpacktools
 * Description : Contains function to translate a string with the
 *               langpack
 * Author      : Christian Kästner
 * Date        : 19.06.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit langpacktools;

interface


uses windows,globals,m_langpack,newpluginapi;

function Translate(s:string):string;
function Translatep(s:PChar):PChar;
function TranslateDialogDefault(hwndDlg:thandle):integer;


implementation

function Translate(s:string):string;
begin
  if CompareVersion(mirandaversion,PLUGIN_MAKE_VERSION(0,1,1,0))>=0 then
    Result:=string(PChar(PluginLink.CallService(MS_LANGPACK_TRANSLATESTRING,0,dword(PChar(s)))))
  else
    Result:=s;
end;
function Translatep(s:PChar):PChar;
begin
  if CompareVersion(mirandaversion,PLUGIN_MAKE_VERSION(0,1,1,0))>=0 then
    Result:=PChar(PluginLink.CallService(MS_LANGPACK_TRANSLATESTRING,0,dword(s)))
  else
    Result:=s;
end;

function TranslateDialogDefault(hwndDlg:thandle):integer;
//not testet yet
var
  lptd:TLANGPACKTRANSLATEDIALOG;
begin
  if CompareVersion(mirandaversion,PLUGIN_MAKE_VERSION(0,1,1,0))<0 then
    Exit;
  lptd.cbSize:=sizeof(lptd);
  lptd.flags:=0;
  lptd.hwndDlg:=hwndDlg;
  lptd.ignoreControls:=nil;
  Result:=pluginlink.CallService(MS_LANGPACK_TRANSLATEDIALOG,0,dword(@lptd));
end;

end.
