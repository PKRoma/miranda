{***************************************************************
 * Project     : convers
 * Unit Name   : misc
 * Description : Misc constants and types for the convers plugin
 *               eg. the defaults for the settings
 * Author      : Christian K�stner
 * Date        : 24.05.2001
 *
 * Copyright � 2001 by Christian K�stner
 ****************************************************************}

unit misc;

interface

uses
  windows, messages, graphics,Controls,forms;



const
  text_chard='Char: %d';
  text_TextTooLong='Text too long. Enable splitting messages, send direct or shorten message.';
const
  DEFAULT_TIMEOUT_MSGSEND=40000;//8sec timeout for message sending (doesnt matter if direct or through server)
  MaxMessageLength=450;

const
  PluginService_ReadBlinkMessage='SRMsg/ReadBlinkingMessage';
  PluginService_LaunchMessageWindow='SRMsg/LaunchMessageWindow';

const
  HM_EVENTSENT =        (WM_USER+10);

type
  //Internal Message Type
  TIcqMessage=record
    MSGTYPE:integer;
    hContact:THandle;//if 0 than outgoing message
    Time:DWord;
    Msg:string;
    Recent:Boolean;//true if just showing a recent message from history
  end;

type
  TDisplayMode = (dmMemo,dmRich,dmGrid);
const
  DefaultDisplayMode=dmRich;


type
  PRichEditSettings=^TRichEditSettings;
  TRichEditSettings=packed record
    RichStyles:array [0..6] of packed record //0=send, 1=inname usw
      Color:TColor;
      Bold:Boolean;
      Italic:Boolean;
    end;
    RecentGray:Boolean;
  end;
  PGridEditSettings=^TGridEditSettings;
  TGridEditSettings=packed record
    BGColorIn:TColor;
    BGColorOut:TColor;
    GrayRecent:Boolean;
    InclTime:Boolean;
    InclDate:Boolean;
  end;

const
  DefaultRichEditSettings:TRichEditSettings=(
    RichStyles:(
      (Color:clBlack;Bold:False;Italic:False), //send
      (Color:clBlack;Bold:False;Italic:False), //inname
      (Color:clBlack;Bold:False;Italic:False), //intime
      (Color:clBlack;Bold:False;Italic:False), //intext
      (Color:clMaroon;Bold:False;Italic:true), //outname
      (Color:clMaroon;Bold:False;Italic:true), //outtime
      (Color:clMaroon;Bold:False;Italic:true));//outtext
    RecentGray:True
    );

  DefaultGridEditSettings:TGridEditSettings=(
    BGColorIn:clWindow;
    BGColorOut:$00E8E8FF;
    GrayRecent:True;
    InclTime:True;
    InclDate:False
    );


var
  blinkid:integer;//id uses for message blinking on contactlist


procedure StartWait;
procedure StopWait;


implementation

const
  WaitCursor: TCursor = crHourGlass;
  WaitCount: Integer = 0;
  SaveCursor: TCursor = crDefault;

procedure StartWait;
begin
  if WaitCount = 0 then begin
    SaveCursor := Screen.Cursor;
    Screen.Cursor := WaitCursor;
  end;
  Inc(WaitCount);
end;

procedure StopWait;
begin
  if WaitCount > 0 then begin
    Dec(WaitCount);
    if WaitCount = 0 then Screen.Cursor := SaveCursor;
  end;
end;

end.
