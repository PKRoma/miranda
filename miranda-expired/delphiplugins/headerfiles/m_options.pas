unit m_options;

interface

uses windows;

{ Opt/Initialise
The user opened the options dialog. Modules should do whatever initialisation
they need and call opt/addpage one or more times if they want pages displayed
in the options dialog
wParam=addInfo
lParam=0
addInfo should be passed straight to the wParam of opt/addpage
}
const
  ME_OPT_INITIALISE   ='Opt/Initialise';

{ Opt/AddPage
Must only be called during an opt/initialise hook
Adds a page to the options dialog
wParam=addInfo
lParam=(LPARAM)(OPTIONSDIALOGPAGE*)odp
addInfo must have come straight from the wParam of opt/initialise
Pages in the options dialog operate just like pages in property sheets. See the
Microsoft documentation for details on how they operate.
Strings in the structure can be released as soon as the service returns, but
icons must be kept around. This is not a problem if you're loading them from a
resource
}
const
  OPTIONSDIALOGPAGE_V0100_SIZE =  $18 ;
type
  POPTIONSDIALOGPAGE=^TOPTIONSDIALOGPAGE;
  TOPTIONSDIALOGPAGE=record
    cbSize:Integer;
    Position:Integer;        //a position number, lower numbers are to the left
    pszTitle:PChar;
    pfnDlgProc:Pointer;//DLGPROC
    pszTemplate:PChar;
    hInstance:LongWord;
    hIcon:hIcon;		 //v0.1.0.1+
    pszGroup:PChar;		 //v0.1.0.1+
    groupPosition:Integer;	 //v0.1.0.1+
    hGroupIcon:hIcon;	         //v0.1.0.1+
  end;
const
  MS_OPT_ADDPAGE='Opt/AddPage';


implementation

end.
