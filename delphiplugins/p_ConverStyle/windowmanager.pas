{***************************************************************
 * Project     : convers
 * Unit Name   : windowmanager
 * Description : Manages the windows: creates new ones, gives
 *               handle from existing ones
 * Author      : Christian Kästner
 * Date        : 18.03.2001
 *
 * Copyright © 2001 by Christian Kästner
 ****************************************************************}

unit windowmanager;

interface

uses
  classes, windows, forms, misc, msgform;

type
  TWindowManager=class
  protected
    WindowList:TList;
  public
    function GetMsgWindow(hContact:THandle):TMsgWindow;
    function CreateMsgWindow(hContact:THandle):TMsgWindow;
    procedure DeleteMsgWindow(MsgWindow:TMsgWindow);
  public
    constructor Create;
    destructor Destroy;override;
  public
    procedure ReloadOptions;
  end;

implementation

{ TWindowManager }

constructor TWindowManager.Create;
begin
  inherited;
  windowlist:=tlist.Create;
end;

function TWindowManager.CreateMsgWindow(hContact: THandle): TMsgWindow;
{Creates window and returns handle}
begin
  result:=TMsgWindow.CreateUID(nil,hContact);
  Result.WindowManager:=Self;
  WindowList.Add(result);
end;

procedure TWindowManager.DeleteMsgWindow(MsgWindow: TMsgWindow);
{removes window from list. called by closing windows}
var
  widx:integer;
begin
  for widx:=windowlist.Count-1 downto 0 do
    if tmsgwindow(windowlist[widx])=msgwindow then
      windowlist.Delete(widx);
end;

destructor TWindowManager.Destroy;
{ondestroy close and free all windows}
var
  widx:integer;
begin
  for widx:=0 to windowlist.Count-1 do
    tmsgwindow(windowlist[widx]).Free;
  windowlist.Free;
end;

function TWindowManager.GetMsgWindow(hContact: THandle): TMsgWindow;
{If there is already a window for the user this function returns it's handle
otherwise it returns nil and the window must be created via CreateMsgWindow}
var
  widx:integer;
begin
  Result:=nil;
  for widx:=0 to windowlist.Count-1 do
    if tmsgwindow(windowlist[widx]).hContact=hContact then
      Result:=tmsgwindow(windowlist[widx]);
end;

procedure TWindowManager.ReloadOptions;
{Makes all windows reload there options}
var
  widx:integer;
begin
  for widx:=0 to windowlist.count-1 do
    if Assigned(windowlist[widx]) then
      tmsgwindow(windowlist[widx]).reloadoptions;
end;

end.
