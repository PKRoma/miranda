{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : databasetools
 * Description : Contains functions for database operations
 *               except read/write settings what is in clisttools
 * Author      : Christian Kästner
 * Date        : 28.04.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit databasetools;

interface

uses windows,newpluginapi, m_database,sysutils;

function MirandaFormatDateTime(PluginLink:TPluginLink;Format:PChar;DateTime:Cardinal):string;
function GMTToLocalTime(PluginLink:TPluginLink;GMTTime:Cardinal):Cardinal;
function LocalToGMTTime(LocalTime:TDateTime):TDateTime;

implementation

//****************** Timefunctions by Christian NineBerry Schwarz *********************************************
//all functions for this copied from InternetDateTime by Christian NineBerry Schwarz (InternetDateTime@NineBerry.de), http://www.NineBerry.de
const
  {/----------------------------------------------------------------------
  //  Constants representing certain amounts of time  
  //----------------------------------------------------------------------}
  DateTimeMinute = 1 / (24 * 60);
  DateTimeHour = 1 / 24;
{/----------------------------------------------------------------------
// Calculate concrete date for a given year
// from a description like 
// 2. tuesday in march
// 5. sunday in june ...
//----------------------------------------------------------------------}
function GetDayFromDayDesc(DoW, Week, Month: Byte; Year: Word): TDateTime;
var
  Day: TDateTime;
  DayInMonth: Integer;
  DOWDifference: Integer;
  MaxDaysInMont: Integer;
  I: Integer;
begin
  Result:= 0;

  // Abort on invalid values;
  if not DoW in [1..7] then
    exit;
  if not Month in [1..12] then
    exit;
  if not (Week > 0) then
    exit;

  // First day in month we are interested in
  Day:= EncodeDate(Year, Month, 1);

  // First of the wanted day of week in this month
  DOWDifference:= DoW - DayOfWeek(Day);
  if DOWDifference < 0 then
    DOWDifference:= 7 + DOWDifference;
  DayInMonth:= 1 + DOWDifference;

  // maximum number of day in this month
  MaxDaysInMont:= MonthDays[IsLeapYear(Year), Month];
  // start of the last seven days in this month
  MaxDaysInMont:= MaxDaysInMont - 7;

  // n. week in this month
  for I:= 2 to Week do
  begin
    if DayInMonth > MaxDaysInMont then
      Break
    else
      Inc(DayInMonth, 7);
  end;

  Result:= EncodeDate(Year, Month, DayInMonth);
end;
function IsDayLightSaving(DT: TDateTime; TZ: TTimeZoneInformation): Boolean;
var
  Year, T: Word;
  DSStart, DSEnd: TDateTime;
begin
  // test if there is daylight saving in this time zone at all.  
  if TZ.DaylightDate.wMonth = 0 then
  begin
    Result:= False;
    Exit;
  end;

  // Get Year part of DT.
  DecodeDate(DT, Year, T, T);

  // Get start of daylight saving
  if TZ.DaylightDate.wYear = 0 then
  begin
    DSStart:= GetDayFromDayDesc(TZ.DaylightDate.wDayOfWeek + 1,
      TZ.DaylightDate.wDay, TZ.DaylightDate.wMonth, Year);
    DSStart:= DSStart + (TZ.DaylightDate.wHour * DateTimeHour);
    DSStart:= DSStart + (TZ.DaylightDate.wMinute * DateTimeMinute);
  end
  else
  begin
    DSStart:= SystemTimeToDateTime(TZ.DaylightDate);
  end;

  // Get end of daylight saving
  if TZ.StandardDate.wYear = 0 then
  begin
    DSEnd:= GetDayFromDayDesc(TZ.StandardDate.wDayOfWeek + 1,
      TZ.StandardDate.wDay, TZ.StandardDate.wMonth, Year);
    DSEnd:= DSEnd + (TZ.StandardDate.wHour * DateTimeHour);
    DSEnd:= DSEnd + (TZ.StandardDate.wMinute * DateTimeMinute);
  end
  else
  begin
    DSEnd:= SystemTimeToDateTime(TZ.StandardDate);
  end;

  // Is DT in Daylight Saving Time?
  Result:= (DT >= DSStart) and (DT <= DSEnd);
end;
{/----------------------------------------------------------------------
//  GetCurrentTimeZoneBias
//  Returns the time that has to be added to local time to calculate
//  GMT
//----------------------------------------------------------------------}
function GetCurrentTimeZoneBias(DT: TDateTime): TDateTime;
var
  TZ: TTimeZoneInformation;
  Bias: Integer;
begin
  // Get system information about time zone
  if GetTimeZoneInformation(TZ) = $FFFFFFFF then
  begin
    // error situation
    Result:= 0;
    Exit;
  end;

  if IsDayLightSaving(DT, TZ) then
    Bias:= TZ.DaylightBias + TZ.Bias
  else
    Bias:= TZ.StandardBias + TZ.Bias;

  // Make TDateTime out of minutes
  Result:= Bias * DateTimeMinute;
end;
{/----------------------------------------------------------------------
//  LocalTimeToGMT
//  Calculates Greenwich Time equivalent to specified Local Time
//----------------------------------------------------------------------}
function m_LocalTimeToGMT(LocalTime: TDateTime): TDateTime;
begin
  Result:= LocalTime + GetCurrentTimeZoneBias(LocalTime);
end;

{/----------------------------------------------------------------------
//  GMTToLocalTime
//  Calculates Local Time equivalent to specified Greenwich Time
//----------------------------------------------------------------------}
function m_GMTToLocalTime(GMT: TDateTime): TDateTime;
begin
  Result:= GMT - GetCurrentTimeZoneBias(GMT);
end;
//*********************************************





function MirandaFormatDateTime(PluginLink:TPluginLink;Format:PChar;DateTime:Cardinal):string;
var
  tts:TDBTIMETOSTRING;
  strdatetime:array [1..64] of char;
begin
  tts.szFormat:=format;
  tts.cbDest:=sizeof(strdatetime);
  tts.szDest:=@strdatetime;

  PluginLink.CallService(MS_DB_TIME_TIMESTAMPTOSTRING,DateTime,dword(@tts));
  Result:=PChar(@strdatetime);
end;

function GMTToLocalTime(PluginLink:TPluginLink;GMTTime:Cardinal):Cardinal;
//caution: uses c time (seconds since 1970)
begin
  Result:=pluginlink.CallService(MS_DB_TIME_TIMESTAMPTOLOCAL,GMTTime,0);
end;

function LocalToGMTTime(LocalTime:TDateTime):TDateTime;
//caution: uses delphi TDateTime
begin
  Result:=m_LocalTimeToGMT(LocalTime);
end;

end.
