{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : skintools
 * Description : Some Tools for the m_clist and m_database
 *               Functions like playing a sound or loading
 *               an icon
 * Author      : Christian Kästner
 * Date        : 28.04.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit skintools;

interface

uses windows,newpluginapi,m_skin;

function LoadSkinnedIcon(PluginLink:TPluginLink;id:Integer):hicon;
function SkinPlaySound(PluginLink:TPluginLink;Name:Pchar):integer;
function SkinAddNewSound(PluginLink:TPluginLink;Name,Description,defaultFile:PChar):Integer;

implementation

function LoadSkinnedIcon(PluginLink:TPluginLink;id:Integer):hicon;
begin
  Result:=THandle(PluginLink.CallService(MS_SKIN_LOADICON,id,0));
end;

function SkinPlaySound(PluginLink:TPluginLink;Name:Pchar):integer;
begin
  Result:=Integer(PluginLink.CallService(MS_SKIN_PLAYSOUND,0,dword(Name)));
end;

function SkinAddNewSound(PluginLink:TPluginLink;Name,Description,defaultFile:PChar):Integer;
var
  ssd:TSKINSOUNDDESC;
begin
  ssd.cbSize:=sizeof(ssd);
  ssd.pszName:=name;
  ssd.pszDescription:=description;
  ssd.pszDefaultFile:=defaultFile;
  Result:=PluginLink.CallService(MS_SKIN_ADDNEWSOUND,0,DWORD(@ssd));
end;

end.
