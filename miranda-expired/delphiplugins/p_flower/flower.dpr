library flower;

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
  mmsystem,
  newpluginapi in '..\units\newpluginapi.pas',
  m_clist in '..\headerfiles\m_clist.pas',
  statusmodes in '..\units\statusmodes.pas',
  m_system in '..\headerfiles\m_system.pas',
  m_plugins in '..\headerfiles\m_plugins.pas',
  m_skin in '..\headerfiles\m_skin.pas',
  skintools in '..\units\skintools.pas',
  langpacktools in '..\units\langpacktools.pas',
  m_langpack in '..\headerfiles\m_langpack.pas',
  globals in '..\units\globals.pas',
  flowerform in 'flowerform.pas' {FlowerFrm},
  reswork in 'reswork.pas';

{$R *.RES}

var
  PluginInfo:TPluginInfo;

function MirandaPluginInfo(mirandaVersion:DWord):PPLUGININFO;cdecl;
//Tell Miranda about this plugin
begin
  PluginInfo.cbSize:=sizeof(TPLUGININFO);
  PluginInfo.shortName:='Flower Plugin';
  PluginInfo.version:=PLUGIN_MAKE_VERSION(1,0,0,2);
  PluginInfo.description:='This Plugin shows a flower on startup and plays a sound. ;-)';
  PluginInfo.author:='Christian Kästner';
  PluginInfo.authorEmail:='no hatemails please ;-)';
  PluginInfo.copyright:='© 2001 Christian Kästner';
  PluginInfo.homepage:='See miranda homepage!';
  PluginInfo.isTransient:=0;
  PluginInfo.replacesDefaultModule:=0;  //doesn't replace anything built-in

  Result:=@PluginInfo;
end;


function Load(link:PPLUGINLINK):Integer;cdecl;
//load function called by miranda
var
  FlowerFrm: TFlowerFrm;
  Befehl:string;
begin
  Result:=0;

  FlowerFrm:=TFlowerFrm.Create(nil);
  FlowerFrm.show;
  FlowerFrm.Update;
  ExtractSound;


  sleep(500);
  FlowerFrm.Update;
  SndPlaySound(PChar(GetSoundName),SND_SYNC);
  FlowerFrm.Update;
  sleep(1000);
  FlowerFrm.Update;

  FlowerFrm.release;
end;

function Unload:Integer;cdecl;
//unload
begin
  Result:=0;
end;


exports
  MirandaPluginInfo,
  Load,
  Unload;


begin
end.
 