library lsecurity;

{
Delphi Low Security Plugin
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
  dialogs,
  newpluginapi in '..\units\newpluginapi.pas',
  globals in '..\units\globals.pas',
  m_clist in '..\headerfiles\m_clist.pas',
  statusmodes in '..\units\statusmodes.pas',
  m_system in '..\headerfiles\m_system.pas',
  m_plugins in '..\headerfiles\m_plugins.pas',
  m_skin in '..\headerfiles\m_skin.pas',
  skintools in '..\units\skintools.pas',
  databasetools in '..\units\databasetools.pas',
  clisttools in '..\units\clisttools.pas',
  langpacktools in '..\units\langpacktools.pas',
  m_userinfo in '..\headerfiles\m_userinfo.pas',
  m_crypt in '..\headerfiles\m_crypt.pas',
  m_database in '..\headerfiles\m_database.pas',
  m_email in '..\headerfiles\m_email.pas',
  m_history in '..\headerfiles\m_history.pas',
  m_icq in '..\headerfiles\m_icq.pas',
  m_langpack in '..\headerfiles\m_langpack.pas',
  m_options in '..\headerfiles\m_options.pas',
  passwdlg in 'passwdlg.pas' {PasswordDlg},
  sysutils,
  messages,
  m_clui in '..\headerfiles\m_clui.pas',
  m_protosvc in '..\headerfiles\m_protosvc.pas',
  m_protocols in '..\headerfiles\m_protocols.pas';

{$R *.RES}

function MirandaPluginInfo(mirandaVersion:DWord):PPLUGININFO;cdecl;
//Tell Miranda about this plugin
begin
  PluginInfo.cbSize:=sizeof(TPLUGININFO);
  PluginInfo.shortName:='Low Security Plugin';
  PluginInfo.version:=PLUGIN_MAKE_VERSION(1,0,0,0);
  PluginInfo.description:='Plugin which asks for password on startup and hence provides a low level security.';
  PluginInfo.author:='Christian Kästner';
  PluginInfo.authorEmail:='christian.k@stner.de';
  PluginInfo.copyright:='© 2001 Christian Kästner';
  PluginInfo.homepage:='http://miranda-icq.sourceforge.net';
  PluginInfo.isTransient:=0;
  PluginInfo.replacesDefaultModule:=0;  //doesn't replace anything built-in

  Result:=@PluginInfo;
end;


const
    crctable:  array[0..255] of LongInt =
   ($00000000, $77073096, $EE0E612C, $990951BA,
    $076DC419, $706AF48F, $E963A535, $9E6495A3,
    $0EDB8832, $79DCB8A4, $E0D5E91E, $97D2D988,
    $09B64C2B, $7EB17CBD, $E7B82D07, $90BF1D91,
    $1DB71064, $6AB020F2, $F3B97148, $84BE41DE,
    $1ADAD47D, $6DDDE4EB, $F4D4B551, $83D385C7,
    $136C9856, $646BA8C0, $FD62F97A, $8A65C9EC,
    $14015C4F, $63066CD9, $FA0F3D63, $8D080DF5,
    $3B6E20C8, $4C69105E, $D56041E4, $A2677172,
    $3C03E4D1, $4B04D447, $D20D85FD, $A50AB56B,
    $35B5A8FA, $42B2986C, $DBBBC9D6, $ACBCF940,
    $32D86CE3, $45DF5C75, $DCD60DCF, $ABD13D59,
    $26D930AC, $51DE003A, $C8D75180, $BFD06116,
    $21B4F4B5, $56B3C423, $CFBA9599, $B8BDA50F,
    $2802B89E, $5F058808, $C60CD9B2, $B10BE924,
    $2F6F7C87, $58684C11, $C1611DAB, $B6662D3D,

    $76DC4190, $01DB7106, $98D220BC, $EFD5102A,
    $71B18589, $06B6B51F, $9FBFE4A5, $E8B8D433,
    $7807C9A2, $0F00F934, $9609A88E, $E10E9818,
    $7F6A0DBB, $086D3D2D, $91646C97, $E6635C01,
    $6B6B51F4, $1C6C6162, $856530D8, $F262004E,
    $6C0695ED, $1B01A57B, $8208F4C1, $F50FC457,
    $65B0D9C6, $12B7E950, $8BBEB8EA, $FCB9887C,
    $62DD1DDF, $15DA2D49, $8CD37CF3, $FBD44C65,
    $4DB26158, $3AB551CE, $A3BC0074, $D4BB30E2,
    $4ADFA541, $3DD895D7, $A4D1C46D, $D3D6F4FB,
    $4369E96A, $346ED9FC, $AD678846, $DA60B8D0,
    $44042D73, $33031DE5, $AA0A4C5F, $DD0D7CC9,
    $5005713C, $270241AA, $BE0B1010, $C90C2086,
    $5768B525, $206F85B3, $B966D409, $CE61E49F,
    $5EDEF90E, $29D9C998, $B0D09822, $C7D7A8B4,
    $59B33D17, $2EB40D81, $B7BD5C3B, $C0BA6CAD,

    $EDB88320, $9ABFB3B6, $03B6E20C, $74B1D29A,
    $EAD54739, $9DD277AF, $04DB2615, $73DC1683,
    $E3630B12, $94643B84, $0D6D6A3E, $7A6A5AA8,
    $E40ECF0B, $9309FF9D, $0A00AE27, $7D079EB1,
    $F00F9344, $8708A3D2, $1E01F268, $6906C2FE,
    $F762575D, $806567CB, $196C3671, $6E6B06E7,
    $FED41B76, $89D32BE0, $10DA7A5A, $67DD4ACC,
    $F9B9DF6F, $8EBEEFF9, $17B7BE43, $60B08ED5,
    $D6D6A3E8, $A1D1937E, $38D8C2C4, $4FDFF252,
    $D1BB67F1, $A6BC5767, $3FB506DD, $48B2364B,
    $D80D2BDA, $AF0A1B4C, $36034AF6, $41047A60,
    $DF60EFC3, $A867DF55, $316E8EEF, $4669BE79,
    $CB61B38C, $BC66831A, $256FD2A0, $5268E236,
    $CC0C7795, $BB0B4703, $220216B9, $5505262F,
    $C5BA3BBE, $B2BD0B28, $2BB45A92, $5CB36A04,
    $C2D7FFA7, $B5D0CF31, $2CD99E8B, $5BDEAE1D,

    $9B64C2B0, $EC63F226, $756AA39C, $026D930A,
    $9C0906A9, $EB0E363F, $72076785, $05005713,
    $95BF4A82, $E2B87A14, $7BB12BAE, $0CB61B38,
    $92D28E9B, $E5D5BE0D, $7CDCEFB7, $0BDBDF21,
    $86D3D2D4, $F1D4E242, $68DDB3F8, $1FDA836E,
    $81BE16CD, $F6B9265B, $6FB077E1, $18B74777,
    $88085AE6, $FF0F6A70, $66063BCA, $11010B5C,
    $8F659EFF, $F862AE69, $616BFFD3, $166CCF45,
    $A00AE278, $D70DD2EE, $4E048354, $3903B3C2,
    $A7672661, $D06016F7, $4969474D, $3E6E77DB,
    $AED16A4A, $D9D65ADC, $40DF0B66, $37D83BF0,
    $A9BCAE53, $DEBB9EC5, $47B2CF7F, $30B5FFE9,
    $BDBDF21C, $CABAC28A, $53B39330, $24B4A3A6,
    $BAD03605, $CDD70693, $54DE5729, $23D967BF,
    $B3667A2E, $C4614AB8, $5D681B02, $2A6F2B94,
    $B40BBE37, $C30C8EA1, $5A05DF1B, $2D02EF8D);

function CRC32 (const data; count: integer):  LongInt;
var p : PByte;
    i : Integer;
begin
  p := PByte(data);
  result := 0;
  for i := 1 to count do begin
    result := (result shr 8)  xor  crcTable[ p^ xor (result and $000000FF)];
    inc(p);
  end;
end;


function Load(link:PPLUGINLINK):Integer;cdecl;
//load function called by miranda
//gets called after db etc have been initialised, but before build in plugins like icq are loaded...
var
  hash:LongInt;
  password:PChar;
const
  ID_ICQ_EXIT=40001;//read from resources.h from Miranda
begin
  Result:=0;

  PluginLink:=link^;

  //check if password already given. if yes this seems to be the first start with this plugin
  password:=ReadSettingStr(PluginLink,0,'ICQ','Password','');
  PluginLink.CallService(MS_DB_CRYPT_DECODESTRING,strlen(password)+1,dword(password));
  hash:=ReadSettingInt(PluginLink,0,'ICQ','PasswordCRC32',0);

  if (password^<>#0) then
    begin
    //save hash value of password, for next starts
    hash:=CRC32(password,strlen(password));
    WriteSettingInt(PluginLink,0,'ICQ','PasswordCRC32',hash);
    end
  else if password^=#0 then
    begin
    //if no password given ask for password and check if it fits to the hash value
    password:=PChar(querypassword);
    hash:=ReadSettingInt(PluginLink,0,'ICQ','PasswordCRC32',0);
    if hash<>CRC32(password,strlen(password)) then
      begin
      ShowMessage('Wrong password!');
      //hack to close miranda
      PostQuitMessage(0);
      Result:=1;//no unload
      end
    else
      begin
      PluginLink.CallService(MS_DB_CRYPT_ENCODESTRING,strlen(password)+1,dword(password));
      WriteSettingStr(PluginLink,0,'ICQ','Password',password);
      end;
    end;

end;

function Unload:Integer;cdecl;
//unload, removes password from file, just leaves hash value
var
  hash:LongInt;
  password:PChar;
begin
  //check if password fits to hash value
  password:=ReadSettingStr(PluginLink,0,'ICQ','Password','');
  PluginLink.CallService(MS_DB_CRYPT_DECODESTRING,strlen(password)+1,dword(password));
  hash:=ReadSettingInt(PluginLink,0,'ICQ','PasswordCRC32',0);
  if hash<>CRC32(password,strlen(password)) then
    begin
    //if not store new hash value
    hash:=CRC32(password,strlen(password));
    WriteSettingInt(PluginLink,0,'ICQ','PasswordCRC32',hash);
    end;

  //delete password
  WriteSettingStr(PluginLink,0,'ICQ','Password','');

  Result:=0;
end;


exports
  MirandaPluginInfo,
  Load,
  Unload;


begin
end.
 