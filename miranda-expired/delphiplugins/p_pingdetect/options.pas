unit options;

interface

uses classes,windows,messages,globals,clisttools,sysutils,commctrl;

function DlgProcOptions(Dialog: HWnd; Message, wParam, lParam: DWord): Boolean; cdecl;

implementation

{$R optdlgs.res}
{copy from optionsd.rh}
const
  IDC_HOST	=108;
  IDC_TIMEOUT	=103;
  IDC_INTERVAL	=104;
{/copy from optionsd.rh}

function DlgProcOptions(Dialog: HWnd; Message, wParam, lParam: DWord): Boolean; cdecl;
var
//  str:array [0..255] of char;
  str:string;
  pc:PChar;
  val:integer;
begin
  Result:=False;

  case message of
    WM_INITDIALOG:
      begin
      //load options
      pc:=ReadSettingStr(PluginLink,0,'PingDetect','PingTarget','heise.de');
      SetDlgItemText(dialog,IDC_HOST,pc);
//      SendDlgItemMessage(dialog,IDC_RECENTCOUNT,CB_ADDSTRING,0,integer(pc));

      pc:=PChar(IntToStr(ReadSettingInt(PluginLink,0,'PingDetect','Timeout',1000)));
      SetDlgItemText(dialog,IDC_TIMEOUT,pc);

      pc:=PChar(IntToStr(ReadSettingInt(PluginLink,0,'PingDetect','Interval',30)));
      SetDlgItemText(dialog,IDC_INTERVAL,pc);

      //make sure we can save the dialog...
      SendMessage(GetParent(dialog), PSM_CHANGED, 0, 0);

      Result:=True;
      end;
    WM_NOTIFY:
      begin
      if PNMHdr(lParam)^.code = PSN_APPLY then//save settings
        begin
        //save new host
        setlength(str,256);
        pc:=PChar(str);
        GetDlgItemText(dialog,IDC_HOST,pc,256);
        WriteSettingStr(PluginLink,0,'PingDetect','PingTarget',pc);

        //save timeout
        try
        setlength(str,256);
        pc:=PChar(str);
        GetDlgItemText(dialog,IDC_Timeout,pc,256);
        val:=StrToInt(str);
        WriteSettingInt(PluginLink,0,'PingDetect','Timeout',val);
        except end;

        //save interval
        try
        setlength(str,256);
        pc:=PChar(str);
        GetDlgItemText(dialog,IDC_INTERVAL,pc,256);
        val:=StrToInt(str);
        WriteSettingInt(PluginLink,0,'PingDetect','Interval',val);
        except end;

        Result:=True;
        end;
      end;
   end;
end;


end.
