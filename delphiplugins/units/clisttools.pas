{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : clisttools
 * Description : Some Tools for the m_clist and m_database
 *               Functions like reading database settings or
 *               getting a contacts nickname or uin
 * Author      : Christian Kästner
 * Date        : 28.04.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit clisttools;

interface

uses windows,newpluginapi,m_database,m_icq,statusmodes;

//readwrite settings
procedure WriteSettingInt(PluginLink:TPluginLink;hContact:THandle;ModuleName,SettingName:PChar;Value:Integer);

function ReadSettingInt(PluginLink:TPluginLink;hContact:THandle;ModuleName,SettingName:PChar;default:Integer):Integer;
function ReadSettingWord(PluginLink:TPluginLink;hContact:THandle;ModuleName,SettingName:PChar;default:Word):Word;

procedure ReadSettingBlob(PluginLink:TPluginLink;hContact:THandle;ModuleName,SettingName:PChar;var pSize:Word;var pbBlob:Pointer);
procedure WriteSettingBlob(PluginLink:TPluginLink;hContact:THandle;ModuleName,SettingName:PChar;pSize:Word;pbBlob:Pointer);
procedure FreeSettingBlob(PluginLink:TPluginLink;pSize:Word;pbBlob:Pointer);

function ReadSettingStr(PluginLink:TPluginLink;hContact:THandle;ModuleName,SettingName:PChar;default:PChar):PChar;
procedure WriteSettingStr(PluginLink:TPluginLink;hContact:THandle;ModuleName,SettingName:PChar;Value:PChar);


//readwirte special settings
function GetContactStatus(PluginLink:TPluginLink;hContact:THandle):Word;
function GetContactUin(PluginLink:TPluginLink;hContact:THandle):DWord;


implementation

procedure WriteSettingInt(PluginLink:TPluginLink;hContact:THandle;ModuleName,SettingName:PChar;Value:Integer);
var
  cws:TDBCONTACTWRITESETTING;
  dbv:TDBVariant;
begin
  dbv.ttype:=DBVT_DWORD;
  dbv.val:=Pointer(Value);

  cws.szModule:=ModuleName;
  cws.szSetting:=SettingName;
  cws.Value:=dbv;
  PluginLink.CallService(MS_DB_CONTACT_WRITESETTING,DWord(hContact),DWord(@cws));
end;

function ReadSettingInt(PluginLink:TPluginLink;hContact:THandle;ModuleName,SettingName:PChar;default:Integer):Integer;
var
  cws:TDBCONTACTGETSETTING;
  dbv:TDBVariant;
begin
  dbv.ttype:=DBVT_DWORD;
  dbv.val:=Pointer(default);
  cws.szModule:=ModuleName;
  cws.szSetting:=SettingName;
  cws.pValue:=@dbv;
  if PluginLink.CallService(MS_DB_CONTACT_GETSETTING,DWord(hContact),DWord(@cws))<>0 then
    Result:=default
  else
    Result:=integer(dbv.val);
end;

function ReadSettingWord(PluginLink:TPluginLink;hContact:THandle;ModuleName,SettingName:PChar;default:Word):Word;
var
  cws:TDBCONTACTGETSETTING;
  dbv:TDBVariant;
begin
  dbv.ttype:=DBVT_WORD;
  dbv.val:=Pointer(dword(default));
  cws.szModule:=ModuleName;
  cws.szSetting:=SettingName;
  cws.pValue:=@dbv;
  if PluginLink.CallService(MS_DB_CONTACT_GETSETTING,DWord(hContact),DWord(@cws))<>0 then
    Result:=default
  else
    Result:=word(dbv.val);
end;

//blob
procedure ReadSettingBlob(PluginLink:TPluginLink;hContact:THandle;ModuleName,SettingName:PChar;var pSize:Word;var pbBlob:Pointer);
var
  cgs:TDBCONTACTGETSETTING;
  dbv:TDBVariant;
begin
  dbv.ttype:=DBVT_BLOB;
  dbv.val:=Pointer( LoWord(pSize) or HiWord(Byte(pbBlob)) );
  cgs.szModule:=ModuleName;
  cgs.szSetting:=SettingName;
  cgs.pValue:=@dbv;
  if PluginLink.CallService(MS_DB_CONTACT_GETSETTING,DWord(hContact),DWord(@cgs))<>0 then
    begin
    pSize:=0;
    pbBlob:=nil;
    end
  else
    begin
    pSize:=LoWord(dbv.val);
    pbBlob:=dbv.pblob;
    end;
end;
procedure WriteSettingBlob(PluginLink:TPluginLink;hContact:THandle;ModuleName,SettingName:PChar;pSize:Word;pbBlob:Pointer);
var
  cgs:TDBCONTACTWRITESETTING;
  dbv:TDBVariant;
begin
  dbv.ttype:=DBVT_BLOB;
  dbv.val:=Pointer(pSize);
  dbv.pblob:=pbBlob;
  cgs.szModule:=ModuleName;
  cgs.szSetting:=SettingName;
  cgs.Value:=dbv;
  PluginLink.CallService(MS_DB_CONTACT_WRITESETTING,DWord(hContact),DWord(@cgs));
end;
procedure FreeSettingBlob(PluginLink:TPluginLink;pSize:Word;pbBlob:Pointer);
var
  dbv:TDBVariant;
begin
  dbv.ttype:=DBVT_BLOB;
  dbv.val:=Pointer(pSize);
  dbv.pblob:=pbBlob;

  PluginLink.CallService(MS_DB_CONTACT_FREEVARIANT,0,dword(@dbv));
end;


//string
function ReadSettingStr(PluginLink:TPluginLink;hContact:THandle;ModuleName,SettingName:PChar;default:PChar):PChar;
var
  cgs:TDBCONTACTGETSETTING;
  dbv:TDBVariant;
begin
  dbv.ttype:=DBVT_ASCIIZ;
  dbv.val:=nil;
  cgs.szModule:=ModuleName;
  cgs.szSetting:=SettingName;
  cgs.pValue:=@dbv;
  if PluginLink.CallService(MS_DB_CONTACT_GETSETTING,DWord(hContact),DWord(@cgs))<>0 then
    Result:=default
  else
    Result:=PChar(dbv.val);
end;
procedure WriteSettingStr(PluginLink:TPluginLink;hContact:THandle;ModuleName,SettingName:PChar;Value:PChar);
var
  cws:TDBCONTACTWRITESETTING;
  dbv:TDBVariant;
begin
  dbv.ttype:=DBVT_ASCIIZ;
  dbv.val:=Value;

  cws.szModule:=ModuleName;
  cws.szSetting:=SettingName;
  cws.Value:=dbv;
  PluginLink.CallService(MS_DB_CONTACT_WRITESETTING,DWord(hContact),DWord(@cws));
end;


//special values
function GetContactStatus(PluginLink:TPluginLink;hContact:THandle):Word;
begin
  Result:=ID_STATUS_OFFLINE;

  //if ICQ UIN exists (msn not interesting here)
  if ReadSettingInt(PluginLink,hContact,'ICQ','UIN',0)<>0 then
    Result:=ReadSettingWord(PluginLink,hContact,'ICQ','Status',ID_STATUS_OFFLINE);
end;

function GetContactUin(PluginLink:TPluginLink;hContact:THandle):DWord;
begin
  result:=ReadSettingInt(PluginLink,hContact,'ICQ','UIN',0);
end;

end.
