unit Log;

{erstellt eine logdatei
das schließen der datei nach jedem schreibvorgang ist notwendig, damit die
datei auch wirklich geschrieben wird. vorher habe ich abgebrochene logdateien
bekommen weil delphi abgestützt ist bevor er in die datei geschrieben hat.
logtextfast ist für zeilen die häufig aufgerufen werden, aber wo es
unwahrscheinlich ist, dass submitta abstützt. damit wird die geschwindigkeit
erhöht.
critical section ist nötig, da logtext auch aus einem thread aufgerufen werden
kann}

interface

procedure CreateLog(LogFileName:string);
procedure FreeLog;

procedure LogText(Text:string);
procedure LogTextFast(Text:string);



implementation


uses
  sysutils,windows,syncobjs;

var
  f:textfile;
  f_assigned:Boolean;
  lasttime:tdatetime;
  lasttext:string;
  flogfilename:string;
  criticalsection:tcriticalsection;

function GetWindowsVersion:string;
var
  OsVinfo   : TOSVERSIONINFO;
  HilfStr   : array[0..50] of Char;
begin
  ZeroMemory(@OsVinfo,sizeOf(OsVinfo));
  OsVinfo.dwOSVersionInfoSize := sizeof(TOSVERSIONINFO);
  if GetVersionEx(OsVinfo) then begin
    if OsVinfo.dwPlatformId = VER_PLATFORM_WIN32_WINDOWS then 
     begin
      if (OsVinfo.dwMajorVersion = 4) and
       (OsVinfo.dwMinorVersion > 0) then
        StrFmt(HilfStr,'Windows 98 - Version %d.%.2d.%d',
               [OsVinfo.dwMajorVersion, OsVinfo.dwMinorVersion,
                OsVinfo.dwBuildNumber AND $FFFF])
      else
        StrFmt(HilfStr,'Windows 95 - Version %d.%d Build %d',
               [OsVinfo.dwMajorVersion, OsVinfo.dwMinorVersion,
                OsVinfo.dwBuildNumber AND $FFFF]);
    end;
    if OsVinfo.dwPlatformId = VER_PLATFORM_WIN32_NT then
      begin
      if (OsVinfo.dwMajorVersion = 5) then
        StrFmt(HilfStr,'Microsoft Windows 2000 (NT %d.%.2d.%d)',
               [OsVinfo.dwMajorVersion, OsVinfo.dwMinorVersion,
                OsVinfo.dwBuildNumber AND $FFFF])
      else
        StrFmt(HilfStr,'Microsoft Windows NT Version %d.%.2d.%d',
               [OsVinfo.dwMajorVersion, OsVinfo.dwMinorVersion,
                OsVinfo.dwBuildNumber AND $FFFF]);
      end
  end
  else
    StrCopy(HilfStr,'_______________________________________');
  Result:=string(HilfStr);
end;

type
  TLongVersion = record
    case Integer of
    0: (All: array[1..4] of Word);
    1: (MS, LS: LongInt);
  end;
function LongVersionToString(const Version: TLongVersion): string;
begin
  with Version do
    Result := Format('%d.%d.%d.%d', [All[2], All[1], All[4], All[3]]);
end;

function GetMirandaVersion:string;
var
  FixedFileInfo: PVSFixedFileInfo;
  Len: Cardinal;
  FBuffer: PChar;
  FFileName:PChar;
  lv:TLongVersion;
  FValid:Boolean;
  afilename:string;
  FSize:integer;
  FHandle:DWord;
begin
  Result:='';
  afilename:=ExtractFilePath(ParamStr(0))+'miranda32.exe';
  FFileName := StrPCopy(StrAlloc(Length(AFileName) + 1), AFileName);

  FValid:=False;
  FSize := GetFileVersionInfoSize(FFileName, FHandle);
  if FSize > 0 then
    try
    GetMem(FBuffer,FSize);
    FValid := GetFileVersionInfo(FFileName, FHandle, FSize, FBuffer);
    except
    FValid := False;
    end;

  if fvalid then
    begin
    VerQueryValue(FBuffer, '\', Pointer(FixedFileInfo), Len);

    lv.MS := FixedFileInfo^.dwFileVersionMS;
    lv.LS := FixedFileInfo^.dwFileVersionLS;
    Result:=LongVersionToString(lv);
    end;
end;


procedure CreateLog(LogFileName:string);
begin
  flogfilename:=logfilename;
  
  f_assigned:=False;                      
  try
  criticalsection:=tcriticalsection.Create;

  assignfile(f,flogfilename);
//  try
//  reset(f);
//  except
  rewrite(f);
//  end;
  writeln(f,'-------------------------------------------------------------------------------------------');
  writeln(f,format('-- Miranda ICQ Log File (%s)',[datetimetostr(now)]));
  writeln(f,'--');
  writeln(f,'-- Log file will be overwritten at next Miranda start.');
  writeln(f,'--');
  writeln(f,'-- Miranda '+GetMirandaVersion);
  writeln(f,'--');
  writeln(f,'-- Operating System: '+GetWindowsVersion);
  writeln(f,'-------------------------------------------------------------------------------------------');
  f_assigned:=True;
  lasttime:=-1;
//  closefile(f);//
//  f_assigned:=false;//
  except
  f_assigned:=False;
  end;
end;

procedure FreeLog;
begin
  logtext('');
  try
  if f_assigned then
    closefile(f);
  except end;
  criticalsection.Free;
  f_assigned:=False;
end;


procedure LogText(Text:string);
var
  tmp:TDateTime;
begin
  criticalsection.Enter;
  if not f_assigned then
    try
    assignfile(f,flogfilename);
//    system.reset(f);
    Append(F);
//    if seekeoln(f) then;
    f_assigned:=True;
    except
    f_assigned:=false;
    end;

  if f_assigned then
    try
    tmp:=time;
    if lasttime<>-1 then
      writeln(f,FormatDateTime('hh:nn:ss:zzz',lasttime)+'   ('+FormatDateTime('ss:zzz',tmp-lasttime)+')    '+lastText);
    lasttime:=tmp;
    lasttext:=Text;
    except end;

  if f_assigned then
    try
    closefile(f);
    f_assigned:=False;
    except
    f_assigned:=False;
    end;
  criticalsection.leave;
end;

procedure LogTextFast(Text:string);
var
  tmp:TDateTime;
begin
  criticalsection.Enter;
  if not f_assigned then
    begin
    try
    assignfile(f,flogfilename);
    append(f);
    f_assigned:=True;
    except f_assigned:=false; end;
    end;
  if f_assigned then
    try                             
    tmp:=time;
    if lasttime<>-1 then
      writeln(f,FormatDateTime('hh:nn:ss:zzz',lasttime)+'   ('+FormatDateTime('ss:zzz',tmp-lasttime)+')    '+lastText);
    lasttime:=tmp;
    lasttext:=Text;
    except end;
  criticalsection.leave;
end;

end.
