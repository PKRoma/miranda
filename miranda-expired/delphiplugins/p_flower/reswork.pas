unit reswork;

interface

function ExtractSound:Boolean;
function GetSoundName:string;
procedure DeleteSound;

implementation

uses windows,classes,sysutils;

{$R sound.res}

const
  FileName='startup.wav';
  ResourceName='startupsound';

function ExtractSound:Boolean;
//extracts updateas.exe from resources and saves it to the submitta directory
//returns true if successful
Var
  ResFile : TResourceStream;
begin
  Result:=True;
  try
  ResFile := TResourceStream.Create(HInstance , ResourceName, RT_RcData);
  ResFile.SaveToFile(GetSoundName);
  ResFile.Free;
  except Result:=False; end;
end;

function GetSoundName:string;
{returns filename incl. pathname, use this instead of directly updateas.exe
because the filename might change in later versions}
begin
  Result:=ExtractFilePath(ParamStr(0))+FileName;
end;

procedure DeleteSound;
//deletes the file from disk
begin
try
  DeleteFile(GetSoundName);
except end;
end;


end.
