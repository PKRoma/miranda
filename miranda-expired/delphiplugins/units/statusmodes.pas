{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : statusmodes
 * Description : Contains constants for miranda and icq and icon
 *               statusmodes and routines for their conversion
 * Author      : Christian Kästner
 * Date        : 28.04.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit statusmodes;

interface

uses m_skin,langpacktools;

const
  //add 1 to ID_STATUS_CONNECTING to mark retries (v0.1.0.1+)
  //eg ID_STATUS_CONNECTING+2 is the third connection attempt, or the second retry
  ID_STATUS_CONNECTING	  =1;
  //max retries is just a marker so that the clist knows what numbers represent
  //retries. It does not set any kind of limit on the number of retries you can
  //and/or should do.
  MAX_CONNECT_RETRIES     =  10000;

  //Status modes used for Icons
  ID_STATUS_OFFLINE         =      40071;
  ID_STATUS_ONLINE          =      40072;
  ID_STATUS_AWAY            =      40073;
  ID_STATUS_DND             =      40074;
  ID_STATUS_NA              =      40075;
  ID_STATUS_OCCUPIED        =      40076;
  ID_STATUS_FREECHAT        =      40077;
  ID_STATUS_INVISIBLE       =      40078;
  ID_STATUS_ONTHEPHONE      =      40079;
  ID_STATUS_OUTTOLUNCH      =      40080;

const
  //ICQ status modes
  STATUS_OFFLINE    = $FFFF;
  STATUS_ONLINE     = $0000;
  STATUS_AWAY       = $0001;
  STATUS_DND        = $0002;
  STATUS_NA         = $0004;
  STATUS_OCCUPIED   = $0010;
  STATUS_FREE_CHAT  = $0020;
  STATUS_INVISIBLE  = $0100;

const//translated internally
  statusstr_connecting='Connecting';
  statusstr_offline='Offline';
  statusstr_online='Online';
  statusstr_away='Away';
  statusstr_na='NA';
  statusstr_occupied='Occupied';
  statusstr_dnd='DND';
  statusstr_invisible='Invisible';
  statusstr_freeforchat='Free For Chat';


function Miranda_ConvertStatus2Str(mirandastatus:Word):string;
function ICQ_ConvertStatus2Str(status:Word):string;
function ICQStatusToMiranda(icqStatus:Word):Word;
function MirandaStatusToIcq(mirandaStatus:Word):Word;
function MirandaStatusToIconID(MirandaStatus:Word):Word;


implementation


function ICQStatusToMiranda(icqStatus:Word):Word;
begin
  Result:=ID_STATUS_ONLINE;
  if(icqStatus=STATUS_OFFLINE)         then result:=ID_STATUS_OFFLINE;
  if(icqStatus and STATUS_DND)=0       then result:=ID_STATUS_DND;
  if(icqStatus and STATUS_NA)=0        then result:=ID_STATUS_NA;
  if(icqStatus and STATUS_OCCUPIED)=0  then result:=ID_STATUS_OCCUPIED;
  if(icqStatus and STATUS_AWAY)=0      then result:=ID_STATUS_AWAY;
  if(icqStatus and STATUS_INVISIBLE)=0 then result:=ID_STATUS_INVISIBLE;
  if(icqStatus and STATUS_FREE_CHAT)=0 then result:=ID_STATUS_FREECHAT;
end;

function MirandaStatusToIcq(mirandaStatus:Word):Word;
begin
  case mirandaStatus of
    ID_STATUS_OFFLINE:   result:=STATUS_OFFLINE;
    ID_STATUS_ONTHEPHONE,
    ID_STATUS_AWAY:      result:=STATUS_AWAY;
    ID_STATUS_OUTTOLUNCH,
    ID_STATUS_NA:        result:=STATUS_NA;
    ID_STATUS_OCCUPIED:  result:=STATUS_OCCUPIED;
    ID_STATUS_DND:       result:=STATUS_DND;
    ID_STATUS_INVISIBLE: result:=STATUS_INVISIBLE;
    ID_STATUS_FREECHAT:  result:=STATUS_FREE_CHAT;
  else
    result:=STATUS_ONLINE;
  end;
end;


function icq_ConvertStatus2Str(status:Word):string;
begin
  if status = STATUS_OFFLINE then
    Result:=translate(statusstr_offline)
  else
  if (status and STATUS_INVISIBLE)=STATUS_INVISIBLE then
    Result:=translate(statusstr_invisible)
  else
  if (status and STATUS_FREE_CHAT)=STATUS_FREE_CHAT then
    Result:=translate(statusstr_freeforchat)
  else
  if (status and STATUS_DND)=STATUS_DND then
    Result:=translate(statusstr_DND)
  else
  if (status and STATUS_NA)=STATUS_NA then
    Result:=translate(statusstr_na)
  else
  if (status and STATUS_AWAY)=STATUS_AWAY then
    Result:=translate(statusstr_away)
  else
  if (status and STATUS_OCCUPIED)=STATUS_OCCUPIED then
    Result:=translate(statusstr_occupied)
  else
  if (status and $01FF)=0 then
    Result:=translate(statusstr_online)
  else
    Result:='Error';
end;

function Miranda_ConvertStatus2Str(mirandastatus:Word):string;
begin
  if mirandastatus = ID_STATUS_CONNECTING then
    Result:=translate(statusstr_connecting)
  else
  if mirandastatus = ID_STATUS_OFFLINE then
    Result:=translate(statusstr_offline)
  else
  if mirandastatus=ID_STATUS_INVISIBLE then
    Result:=translate(statusstr_invisible)
  else
  if mirandastatus=ID_STATUS_FREECHAT then
    Result:=translate(statusstr_freeforchat)
  else
  if mirandastatus=ID_STATUS_DND then
    Result:=translate(statusstr_DND)
  else
  if mirandastatus=ID_STATUS_NA then
    Result:=translate(statusstr_na)
  else
  if mirandastatus=ID_STATUS_AWAY then
    Result:=translate(statusstr_away)
  else
  if mirandastatus=ID_STATUS_OCCUPIED then
    Result:=translate(statusstr_occupied)
  else
  if mirandastatus=ID_STATUS_ONLINE then
    Result:=translate(statusstr_online)
  else
    Result:='Error';
end;

function MirandaStatusToIconID(MirandaStatus:Word):Word;
begin
  Result:=SKINICON_STATUS_OFFLINE;

  if MirandaStatus=ID_STATUS_ONLINE then
    Result:=SKINICON_STATUS_ONLINE;
  if MirandaStatus=ID_STATUS_AWAY then
    Result:=SKINICON_STATUS_AWAY;
  if MirandaStatus=ID_STATUS_NA then
    Result:=SKINICON_STATUS_NA;
  if MirandaStatus=ID_STATUS_OCCUPIED then
    Result:=SKINICON_STATUS_OCCUPIED;
  if MirandaStatus=ID_STATUS_DND then
    Result:=SKINICON_STATUS_DND;
  if MirandaStatus=ID_STATUS_FREECHAT then
    Result:=SKINICON_STATUS_FREE4CHAT;
  if MirandaStatus=ID_STATUS_INVISIBLE then
    Result:=SKINICON_STATUS_INVISIBLE;
  if MirandaStatus=ID_STATUS_ONTHEPHONE then
    Result:=SKINICON_STATUS_ONTHEPHONE;
  if MirandaStatus=ID_STATUS_OUTTOLUNCH then
    Result:=SKINICON_STATUS_OUTTOLUNCH;
end;


end.
