library dtemplate;

{
Delphi Plugin Template
Version 0.0.0.1
by Christian Kästner
for Miranda ICQ 0.1.0.0
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
  newpluginapi in '..\units\newpluginapi.pas',
  globals in '..\units\globals.pas',
  m_clist in '..\headerfiles\m_clist.pas',
  statusmodes in '..\units\statusmodes.pas',
  m_system in '..\headerfiles\m_system.pas',
  m_plugins in '..\headerfiles\m_plugins.pas',
  m_skin in '..\headerfiles\m_skin.pas',
  skintools in '..\units\skintools.pas';

{$R *.RES}


function MirandaPluginInfo(mirandaVersion:DWord):PPLUGININFO;cdecl;
//Tell Miranda about this plugin
begin
  PluginInfo.cbSize:=sizeof(TPLUGININFO);
  PluginInfo.shortName:='Plugin Name';
  PluginInfo.version:=PLUGIN_MAKE_VERSION(0,0,0,1);
  PluginInfo.description:='The long description of your plugin, to go in the plugin options dialog';
  PluginInfo.author:='Your Name';
  PluginInfo.authorEmail:='your@email';
  PluginInfo.copyright:='© 2001 Your Name';
  PluginInfo.homepage:='http://www.download.url';
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
  Result:=0;
end;

function PluginMenuCommand(wParam{hContact},lParam{0}:DWord):integer;cdecl;
begin
  MessageBox(0,'Just groovy, baby!','Plugin-o-rama',MB_OK);
  Result:=0;
end;

function OnModulesLoad(wParam{0},lParam{0}:DWord):integer;cdecl;
//init plugin
var
  menuitem:TCLISTMENUITEM;
begin
  //register service for showing history
  PluginLink.CreateServiceFunction('PluginTemplate/MenuCommand',PluginMenuCommand);

  //create menu item in contact menu
  menuitem.cbSize:=sizeof(menuitem);
  menuitem.Position:=-$7FFFFFFF;
  menuitem.flags:=0;
  menuitem.hIcon:=LoadSkinnedIcon(PluginLink,SKINICON_OTHER_MIRANDA);
  menuitem.pszContactOwner:=nil;    //all contacts
  menuitem.pszName:='&Plugin Template';
  menuitem.pszService:='PluginTemplate/MenuCommand';
  PluginLink.CallService(MS_CLIST_ADDCONTACTMENUITEM,0,DWord(@menuitem));
  
  Result:=0;
end;


exports
  MirandaPluginInfo,
  Load,
  Unload;


begin
end.
 