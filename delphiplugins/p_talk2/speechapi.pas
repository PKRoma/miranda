//Speech Unit file. Text to Speech only
//Converted by Glenn Stephens
//glenn@spes.wow.aust.com

unit speechapi;

interface

uses
 SysUtils, Windows, ActiveX{, ComObj};

type
 PUnknown = ^IUnknown;

const
 SVFN_LEN = 262;
 LANG_LEN = 64;
 EI_TITLESIZE = 128;
 EI_DESCSIZE = 512;
 EI_FIXSIZE = 512;
 SVPI_MFGLEN = 64;
 SVPI_PRODLEN = 64;
 SVPI_COMPLEN = 64;
 SVPI_COPYRIGHTLEN = 128;
 SVI_MFGLEN = SVPI_MFGLEN;

function SetBit(x: byte): DWORD;

// Error Macros
{#define  FACILITY_SPEECH   (FACILITY_ITF)
#define  SPEECHERROR(x)    MAKE_SCODE(SEVERITY_ERROR,   FACILITY_SPEECH, (x)+0x200)
#define  AUDERROR(x)       MAKE_SCODE(SEVERITY_ERROR,   FACILITY_SPEECH, (x)+0x300)
#define  SRWARNING(x)      MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_SPEECH, (x)+0x400)
#define  SRERROR(x)        MAKE_SCODE(SEVERITY_ERROR,   FACILITY_SPEECH, (x)+0x400)
#define  TTSERROR(x)       MAKE_SCODE(SEVERITY_ERROR,   FACILITY_SPEECH, (x)+0x500)
#define  VCMDERROR(x)      MAKE_SCODE(SEVERITY_ERROR,   FACILITY_SPEECH, (x)+0x600)
#define  VTXTERROR(x)      MAKE_SCODE(SEVERITY_ERROR,   FACILITY_SPEECH, (x)+0x700)
#define  LEXERROR(x)       MAKE_SCODE(SEVERITY_ERROR,   FACILITY_SPEECH, (x)+0x800)}

// Audio Errors
const
 S_OK = 0;
 AUDERR_NONE = S_OK; 
 AUDERR_BADDEVICEID = $80040301;
 AUDERR_NEEDWAVEFORMAT = $80040302;
 AUDERR_NOTSUPPORTED = $80004001;
 AUDERR_NOTENOUGHDATA = $80040201;
 AUDERR_NOTPLAYING = $80040306;
 AUDERR_INVALIDPARAM = $80070057;
 AUDERR_WAVEFORMATNOTSUPPORTED = $80040202;
 AUDERR_WAVEDEVICEBUSY = $80040203;
 AUDERR_WAVEDEVNOTSUPPORTED = $80040312;
 AUDERR_NOTRECORDING = $80040313;
 AUDERR_INVALIDFLAG = $80040204;
 AUDERR_INVALIDHANDLE = $80070006;
 AUDERR_NODRIVER = $80040317;
 AUDERR_HANDLEBUSY = $80040318;
 AUDERR_INVALIDNOTIFYSINK = $80040319;
 AUDERR_WAVENOTENABLED = $8004031A;
 AUDERR_ALREADYCLAIMED = $8004031D;
 AUDERR_NOTCLAIMED = $8004031E;
 AUDERR_STILLPLAYING = $8004031F;
 AUDERR_ALREADYSTARTED = $80040320;
 AUDERR_SYNCNOTALLOWED = $80040321;

// Speech Recognition Warnings
//#define  SRWARN_BAD_LIST_PRONUNCIATION    SRWARNING(1)

// Speech Recognition Errors
 SRERR_NONE = $0;
 SRERR_OUTOFDISK = $80040205;
 SRERR_NOTSUPPORTED = $80004001;
 SRERR_NOTENOUGHDATA = $80040201;
 SRERR_VALUEOUTOFRANGE = $8000FFFF;
 SRERR_GRAMMARTOOCOMPLEX = $80040406;
 SRERR_GRAMMARWRONGTYPE = $80040407;
 SRERR_INVALIDWINDOW = $8004000F;
 SRERR_INVALIDPARAM = $80070057;
 SRERR_INVALIDMODE = $80040206;
 SRERR_TOOMANYGRAMMARS = $8004040B;
 SRERR_INVALIDLIST = $80040207;
 SRERR_WAVEDEVICEBUSY = $80040203;
 SRERR_WAVEFORMATNOTSUPPORTED = $80040202;
 SRERR_INVALIDCHAR = $80040208;
 SRERR_GRAMTOOCOMPLEX = $80040406;
 SRERR_GRAMTOOLARGE = $80040411;
 SRERR_INVALIDINTERFACE = $80004002;
 SRERR_INVALIDKEY = $80040209;
 SRERR_INVALIDFLAG = $80040204;
 SRERR_GRAMMARERROR = $80040416;
 SRERR_INVALIDRULE = $80040417;
 SRERR_RULEALREADYACTIVE = $80040418;
 SRERR_RULENOTACTIVE = $80040419;
 SRERR_NOUSERSELECTED = $8004041A;
 SRERR_BAD_PRONUNCIATION = $8004041B;
 SRERR_DATAFILEERROR = $8004041C;
 SRERR_GRAMMARALREADYACTIVE = $8004041D;
 SRERR_GRAMMARNOTACTIVE = $8004041E;
 SRERR_GLOBALGRAMMARALREADYACTIVE = $8004041F;
 SRERR_LANGUAGEMISMATCH = $80040420;
 SRERR_MULTIPLELANG = $80040421;
 SRERR_LDGRAMMARNOWORDS = $80040422;
 SRERR_NOLEXICON = $80040423;
 SRERR_SPEAKEREXISTS = $80040424;
 SRERR_GRAMMARENGINEMISMATCH = $80040425;
 SRERR_BOOKMARKEXISTS = $80040426;
 SRERR_BOOKMARKDOESNOTEXIST = $80040427;
 SRERR_MICWIZARDCANCELED = $80040428;


// Text to Speech Errors
 TTSERR_NONE = $00000000;
 TTSERR_INVALIDINTERFACE = $80004002;
 TTSERR_OUTOFDISK = $80040205;
 TTSERR_NOTSUPPORTED = $80004001;
 TTSERR_VALUEOUTOFRANGE = $8000FFFF;
 TTSERR_INVALIDWINDOW = $8004000F;
 TTSERR_INVALIDPARAM = $80070057;
 TTSERR_INVALIDMODE = $80040206;
 TTSERR_INVALIDKEY = $80040209;
 TTSERR_WAVEFORMATNOTSUPPORTED = $80040202;
 TTSERR_INVALIDCHAR = $80040208;
 TTSERR_QUEUEFULL = $8004020A;
 TTSERR_WAVEDEVICEBUSY = $80040203;
 TTSERR_NOTPAUSED = $80040501;
 TTSERR_ALREADYPAUSED = $80040502;

 // Voice Command Errors

 // Everything worked
 VCMDERR_NONE = $00000000;

 // Voice Commands could not allocate memory
 VCMDERR_OUTOFMEM = $8007000E;

 // Voice Commands could not store/retrieve a command set from the database
 VCMDERR_OUTOFDISK = $80040205;

 // Function not implemented
 VCMDERR_NOTSUPPORTED = $80004001;

 // A parameter was passed that was out of the ranged of accepted values
 VCMDERR_VALUEOUTOFRANGE = $8000FFFF;

 // A menu was too complex to compile a context-free grammar
 VCMDERR_MENUTOOCOMPLEX = $80040606;

 // Language mismatch between the speech recognition mode and menu trying to create
 VCMDERR_MENUWRONGLANGUAGE = $80040607;

 // An invalid window handle was passed to Voice Commands
 VCMDERR_INVALIDWINDOW = $8004000F;

 // Voice Commands detected a bad function parameter
 VCMDERR_INVALIDPARAM = $80070057;

 // This function cannot be completed right now, usually when trying to do
 // some operation while no speech recognition site is established
 VCMDERR_INVALIDMODE = $80040206;

 // There are too many Voice Commands menu
 VCMDERR_TOOMANYMENUS = $8004060B;

 // Invalid list passed to ListSet/ListGet
 VCMDERR_INVALIDLIST = $80040207;

 // Trying to open an existing menu that is not in the Voice Commands database
 VCMDERR_MENUDOESNOTEXIST = $8004060D;

 // The function could not be completed because the menu is actively 
 // listening for commands
 VCMDERR_MENUACTIVE = $8004060E;

 // No speech recognition engine is started
 VCMDERR_NOENGINE = $8004060F;

 // Voice Commands could not acquire a Grammar interface from the speech
 // recognition engine
 VCMDERR_NOGRAMMARINTERFACE = $80040610;

 // Voice Commands could not acquire a Find interface from the speech
 // recognition engine
 VCMDERR_NOFINDINTERFACE = $80040611;

 // Voice Commands could not create a speech recognition enumerator
 VCMDERR_CANTCREATESRENUM = $80040612;

 // Voice Commands could get the appropriate site information to start a
 // speech recognition engine
 VCMDERR_NOSITEINFO = $80040613;

 // Voice Commands could not find a speech recognition engine
 VCMDERR_SRFINDFAILED = $80040614;

 // Voice Commands could not create an audio source object
 VCMDERR_CANTCREATEAUDIODEVICE = $80040615;

 // Voice Commands could not set the appropriate device number in the
 // audio source object
 VCMDERR_CANTSETDEVICE = $80040616;

 // Voice Commands could not select a speech recognition engine. Usually the
 // error will occur when Voice Commands has enumerated and found an
 // appropriate speech recognition engine, then it is not able to actually
 // select/start the engine. There are different reasons that the engine won't
 // start, but the most common is that there is no wave in device.
 
 VCMDERR_CANTSELECTENGINE = $80040617;

 // Voice Commands could not create a notfication sink for engine
 // notifications
 VCMDERR_CANTCREATENOTIFY = $80040618;

 // Voice Commands could not create internal data structures.
 VCMDERR_CANTCREATEDATASTRUCTURES = $80040619;

 // Voice Commands could not initialize internal data structures
 VCMDERR_CANTINITDATASTRUCTURES = $8004061A;

 // The menu does not have an entry in the Voice Commands cache
 VCMDERR_NOCACHEDATA = $8004061B;

 // The menu does not have commands
 VCMDERR_NOCOMMANDS = $8004061C;
 
 // Voice Commands cannot extract unique words needed for the engine grammar
 VCMDERR_CANTXTRACTWORDS = $8004061D;

 // Voice Commands could not get the command set database name
 VCMDERR_CANTGETDBNAME = $8004061E;

 // Voice Commands could not create a registry key
 VCMDERR_CANTCREATEKEY = $8004061F;

 // Voice Commands could not create a new database name
 VCMDERR_CANTCREATEDBNAME = $80040620;
 
 // Voice Commands could not update the registry
 VCMDERR_CANTUPDATEREGISTRY = $80040621;

 // Voice Commands could not open the registry
 VCMDERR_CANTOPENREGISTRY = $80040622;

 // Voice Commands could not open the command set database
 VCMDERR_CANTOPENDATABASE = $80040623;

 // Voice Commands could not create a database storage object
 VCMDERR_CANTCREATESTORAGE = $80040624;

 // Voice Commands could not do CmdMimic
 VCMDERR_CANNOTMIMIC = $80040625;

 // A menu of this name already exist
 VCMDERR_MENUEXIST = $80040626;

 // A menu of this name is open and cannot be deleted right now
 VCMDERR_MENUOPEN = $80040627;

 // Voice Text Errors
 VTXTERR_NONE = $00000000;

 // Voice Text failed to allocate memory it needed
 VTXTERR_OUTOFMEM = $8007000E;

 // An empty string ("") was passed to the Speak function
 VTXTERR_EMPTYSPEAKSTRING = $8004020B;

 // An invalid parameter was passed to a Voice Text function
 VTXTERR_INVALIDPARAM = $80070057;

 // The called function cannot be done at this time. This usually occurs
 // when trying to call a function that needs a site, but no site has been
 // registered.
 VTXTERR_INVALIDMODE = $80040206;

 // No text-to-speech engine is started
 VTXTERR_NOENGINE = $8004070F;

 // Voice Text could not acquire a Find interface from the text-to-speech engine
 VTXTERR_NOFINDINTERFACE = $80040711;

 // Voice Text could not create a text-to-speech enumerator
 VTXTERR_CANTCREATETTSENUM = $80040712;

 // Voice Text could get the appropriate site information to start a
 // text-to-speech engine
 VTXTERR_NOSITEINFO = $80040713;

 // Voice Text could not find a text-to-speech engine
 VTXTERR_TTSFINDFAILED = $80040714;

 // Voice Text could not create an audio destination object
 VTXTERR_CANTCREATEAUDIODEVICE = $80040715;

 // Voice Text could not set the appropriate device number in the
 // audio destination object
 VTXTERR_CANTSETDEVICE = $80040716;

 // Voice Text could not select a text-to-speech engine. Usually the
 // error will occur when Voice Text has enumerated and found an
 // appropriate text-to-speech engine, then it is not able to actually
 // select/start the engine.
 VTXTERR_CANTSELECTENGINE = $80040717;

 // Voice Text could not create a notfication sink for engine
 // notifications
 VTXTERR_CANTCREATENOTIFY = $80040718;

 // Voice Text is disabled at this time
 VTXTERR_NOTENABLED = $80040719;
 VTXTERR_OUTOFDISK = $80040205;
 VTXTERR_NOTSUPPORTED = $80004001;
 VTXTERR_NOTENOUGHDATA = $80040201;
 VTXTERR_QUEUEFULL = $8004020A;
 VTXTERR_VALUEOUTOFRANGE = $8000FFFF;
 VTXTERR_INVALIDWINDOW = $8004000F;
 VTXTERR_WAVEDEVICEBUSY = $80040203;
 VTXTERR_WAVEFORMATNOTSUPPORTED = $80040202;
 VTXTERR_INVALIDCHAR = $80040208;

 // ILexPronounce errors
 LEXERR_INVALIDTEXTCHAR = $80040801;
 LEXERR_INVALIDSENSE = $80040802;
 LEXERR_NOTINLEX = $80040803;
 LEXERR_OUTOFDISK = $80040804;
 LEXERR_INVALIDPRONCHAR = $80040805;
 LEXERR_ALREADYINLEX = $80040806;
 LEXERR_PRNBUFTOOSMALL = $80040807;
 LEXERR_ENGBUFTOOSMALL = $80040808;

//typedefs

type
 PSData = ^TSData;
 TSData = record
   pData: Pointer;
   dwSize: DWORD;
 end;

 PLanguage = ^TLanguage;
 TLanguage = record
   LaunguageID: LangID;
   szDialect:array[0..LANG_LEN-1] of char;
 end;

 PVoid = Pointer;
 PQWord = ^QWord;
 QWord = array[0..1] of DWORD;

const
 CHARSET_TEXT           = 0;
 CHARSET_IPAPHONETIC    = 1;
 CHARSET_ENGINEPHONETIC = 2;

type
 TVoiceCharSet = (vcText, vcIpaPhonetic, vcEnginePhonetic);

 TVoicePartOfSpeech = (vpUnknown, vpNoun, vpVerb, vpAdverb, vpAdjective,
  vpProperNoun, vpPronoun, vpConjunction, vpCardinal,
  vpOrdinal, vpDeterminer, vpQuantifier, vpPunctuation, vpContraction,
  vpInterjection, vpAbbreviation, vpPreposition);

 PSPResPhonemenode = ^TSPResPhonemenode;
 TSPResPhonemenode = record
   dwNextPhonemeNode:DWORD;
   dwUpAlternatePhonemeNode:DWORD;
   dwDownAlternatePhonemeNode:DWORD;
   dwPreviousPhonemeNode:DWORD;
   dwWordNode:DWORD;
   qwStartTime:QWORD;
   qwEndTime:QWORD;
   dwPhonemeScore:DWORD;
   wVolume:WORD;
   wPitch:WORD;
 end;

 PSRResWordNode = ^TSRResWordNode;
 TSRResWordNode = record
   dwNextWordNode:DWORD;
   dwUpAlternateWordNode:DWORD;
   dwDownAlternateWordNode:DWORD;
   dwPreviousWordNode:DWORD;
   dwPhonemeNode:DWORD;
   qwStartTime:QWORD;
   qwEndTime:QWORD;
   dwWordScore:DWORD;
   wVolume:WORD;
   wPitch:WORD;
   pos:TVoicePartOfSpeech;
   dwCFGParse:DWORD;
   dwCue:DWORD;
 end;

//Set Bit constatnts

const
 SETBIT0 = 1;
 SETBIT1 = 2;
 SETBIT2 = 4;
 SETBIT3 = 8;
 SETBIT4 = 16;
 SETBIT5 = 32;
 SETBIT6 = 64;
 SETBIT7 = 128;
 SETBIT8 = 256;
 SETBIT9 = 512;
 SETBIT10 = 1024;
 SETBIT11 = 2048;
 SETBIT12 = 4096;
 SETBIT13 = 8192;
 SETBIT14 = 16384;

// Low-Level text-to-speech API

const
 TTSI_NAMELEN = SVFN_LEN;
 TTSI_STYLELEN = SVFN_LEN;

 GENDER_NEUTRAL = 0;
 GENDER_FEMALE  = 1;
 GENDER_MALE    = 2;

 TTSFEATURE_ANYWORD = SETBIT0;
 TTSFEATURE_VOLUME = SETBIT1;
 TTSFEATURE_SPEED = SETBIT2;
 TTSFEATURE_PITCH = SETBIT3;
 TTSFEATURE_TAGGED = SETBIT4;
 TTSFEATURE_IPAUNICODE = SETBIT5;
 TTSFEATURE_VISUAL = SETBIT6;
 TTSFEATURE_WORDPOSITION = SETBIT7;
 TTSFEATURE_PCOPTIMIZED = SETBIT8;
 TTSFEATURE_PHONEOPTIMIZED = SETBIT9;

 TTSI_ILEXPRONOUNCE  = SETBIT0;
 TTSI_ITTSATTRIBUTES = SETBIT1;
 TTSI_ITTSCENTRAL    = SETBIT2;
 TTSI_ITTSDIALOGS    = SETBIT3;

 TTSDATAFLAG_TAGGED  = SETBIT0;

 TTSBNS_ABORTED      = SETBIT0;

// ITTSNotifySink
 TTSNSAC_REALTIME = 0;
 TTSNSAC_PITCH    = 1;
 TTSNSAC_SPEED    = 2;
 TTSNSAC_VOLUME   = 3;

 TTSNSHINT_QUESTION     = SETBIT0;
 TTSNSHINT_STATEMENT    = SETBIT1;
 TTSNSHINT_COMMAND      = SETBIT2;
 TTSNSHINT_EXCLAMATION  = SETBIT3;
 TTSNSHINT_EMPHASIS     = SETBIT4;

// Ages
 TTSAGE_BABY            = 1;
 TTSAGE_TODDLER         = 3;
 TTSAGE_CHILD           = 6;
 TTSAGE_ADOLESCENT      = 14;
 TTSAGE_ADULT           = 30;
 TTSAGE_ELDERLY         = 70;

// Attribute minimums and maximums
 TTSATTR_MINPITCH       = 0;
 TTSATTR_MAXPITCH       = $ffff;
 TTSATTR_MINREALTIME    = 0;
 TTSATTR_MAXREALTIME    = $ffffffff;
 TTSATTR_MINSPEED       = 0;
 TTSATTR_MAXSPEED       = $ffffffff;
 TTSATTR_MINVOLUME      = 0;
 TTSATTR_MAXVOLUME      = $ffffffff;

type
 PTTSMouth = ^TTTSMouth;
 TTTSMouth = record
  bMouthHeight,
  bMouthWidth,
  bMouthUpturn,
  bJawOpen,
  bTeethUpperVisible,
  bTeethLowerVisible,
  bTonguePosn,
  bLipTension:byte;
 end;

 PTTSModeInfo = ^TTTSModeInfo;
 TTTSModeInfo = record
   gEngineID:TGUID;
   szMfgName:array[0..TTSI_NAMELEN-1] of char;
   szProductName:array[0..TTSI_NAMELEN-1] of char;
   gModeID:TGUID;
   szModeName:array[0..TTSI_NAMELEN-1] of char;
   language:TLanguage;
   szSpeaker:array[0..TTSI_NAMELEN-1] of char;
   szStyle:array[0..TTSI_STYLELEN-1] of char;
   wGender:word;
   wAge:word;
   dwFeatures:dword;
   dwInterfaces:dword;
   dwEngineFeatures:dword;
 end;

 PTTSModeInfoRank = ^TTTSModeInfoRank;
 TTTSModeInfoRank = record
   dwEngineID,
   dwMfgName,
   dwProductName,
   dwModeID,
   dwModeName,
   dwLanguage,
   dwDialect,
   dwSpeaker,
   dwStyle,
   dwGender,
   dwAge,
   dwFeatures,
   dwInterfaces,
   dwEngineFeatures:DWORD;
 end;

const
 CLSID_TTSEnumerator: TGUID = '{D67C0280-C743-11cd-80E5-00AA003E4B50}';

{ ITTSAttributes }

const
 IID_ITTSAttributesA: TGUID = '{0FD6E2A1-E77D-11CD-B3CA-00AA0047BA4F}';
 IID_ITTSAttributes: TGUID = '{0FD6E2A1-E77D-11CD-B3CA-00AA0047BA4F}'; //IID_ITTSAttributesA;

type
 PITTSAttributesA = ^ITTSAttributesA;
 ITTSAttributesA = interface(IUnknown)
  ['{0FD6E2A1-E77D-11CD-B3CA-00AA0047BA4F}']
   function PitchGet(var pwPitch: word): HRESULT; stdcall;
   function PitchSet(wPitch: word): HRESULT; stdcall;
   function RealTimeGet(var pdwRealTime: dword): HRESULT; stdcall;
   function RealTimeSet(dwRealTime: dword): HRESULT; stdcall;
   function SpeedGet(var pdwSpeed: dword): HRESULT; stdcall;
   function SpeedSet(dwSpeed: DWORD): HRESULT; stdcall;
   function VolumeGet(var pdwVolume: DWORD): HRESULT; stdcall;
   function VolumeSet(dwVolume: DWORD): HRESULT; stdcall;
 end;

 ITTSAttributes = ITTSAttributesA;
 PITTSATTRIBUTES = PITTSATTRIBUTESA;

{ ITTSBufNotifySink }

const IID_ITTSBufNotifySink:TGUID = '{E4963D40-C743-11cd-80E5-00AA003E4B50}';

type
 PITTSBufNotifySink = ^ITTSBufNotifySink;
 ITTSBufNotifySink = interface(IUnknown)
  ['{E4963D40-C743-11cd-80E5-00AA003E4B50}']
   function TextDataDone(qTimeStamp:QWORD; dwFlags:DWORD): HRESULT; stdcall;
   function TextDataStarted(qTimeStamp: QWORD): HRESULT; stdcall;
   function BookMark(qTimeStamp:QWord; dwMarkNum:DWORD): HRESULT; stdcall;
   function WordPosition(qTimeStamp: QWORD; dwByteOffset: DWORD): HRESULT; stdcall;
 end;

{ ITTSNotifySink }

const
 IID_ITTSNotifySinkA: TGUID = '{05EB6C6F-DBAB-11CD-B3CA-00AA0047BA4F}';
 IID_ITTSNotifySink: TGUID = '{05EB6C6F-DBAB-11CD-B3CA-00AA0047BA4F}';

type
 ITTSNotifySinkA = interface(IUnknown)
  ['{05EB6C6F-DBAB-11CD-B3CA-00AA0047BA4F}']
  function AttribChanged(dwAttribute: DWORD): HRESULT; stdcall;
  function AudioStart(qTimeStamp: QWORD): HRESULT; stdcall;
  function AudioStop(qTimeStamp: QWORD): HRESULT; stdcall;
  function Visual(qTimeStamp: QWORD; cIPAPhoneme, cEnginePhoneme: Char;
    dwHints: DWORD; pTTSMouth: PTTSMOUTH): HRESULT; stdcall;
 end;

 ITTSNotifySink = ITTSNotifySinkA;

{ ITTSCentral }

const
 IID_ITTSCentralA: TGUID = '{05EB6C6A-DBAB-11CD-B3CA-00AA0047BA4F}';
 IID_ITTSCentral: TGUID = '{05EB6C6A-DBAB-11CD-B3CA-00AA0047BA4F}'; //IID_ITTSCentralA

type
 PITTSCentralA = ^ITTSCentralA;
 ITTSCentralA = interface(IUnknown)
  ['{05EB6C6A-DBAB-11CD-B3CA-00AA0047BA4F}']
   function Inject(pszTag: WideString): HRESULT; stdcall;
   function ModeGet(pttsInfo: PTTSModeInfo): HRESULT; stdcall;
   function Phoneme(eCharacterSet: TVoiceCharSet; dwFlags: DWORD;
     dText: TSDATA; pdPhoneme: PSDATA): HRESULT; stdcall;
   function PosnGet(var pqwTimeStamp: QWORD): HRESULT; stdcall;
   function TextData(eCharacterSet: TVoiceCharSet; dwFlags: DWORD; dText: TSDATA;
     pNotifyInterface: IUnknown; IIDNotifyInterface: TGUID): HRESULT; stdcall;
   function ToFileTime(pqTimeStamp: PQWORD; var pFT:TFILETIME): HRESULT; stdcall;
   function AudioPause: HRESULT; stdcall;
   function AudioResume: HRESULT; stdcall;
   function AudioReset: HRESULT; stdcall;
   function Register(pNotifyInterface: ITTSNotifySink; IIDNotifyInterface: TGUID;
     var dwKey: DWORD): HRESULT; stdcall;
   function UnRegister(dwKey: DWORD): HRESULT; stdcall;
 end;

 ITTSCentral = ITTSCentralA;
 PITTSCENTRAL = PITTSCENTRALA;

const
 IID_ITTSDialogsA: TGUID = '{05EB6C6B-DBAB-11CD-B3CA-00AA0047BA4F}';
 IID_ITTSDialogs: TGUID = '{05EB6C6B-DBAB-11CD-B3CA-00AA0047BA4F}'; //IID_ITTSDialogsA
type
 PITTSDialogsA = ^ITTSDialogsA;
 ITTSDialogsA = interface(IUnknown)
  ['{05EB6C6B-DBAB-11CD-B3CA-00AA0047BA4F}']
   function AboutDlg(hWndParent:HWND; pszTitle:WideString): HRESULT; stdcall;
   function LexiconDlg(hWndParent:HWND; pszTitle:PChar): HRESULT; stdcall;
   function GeneralDlg(hWndParent:HWND; pszTitle:PChar): HRESULT; stdcall;
   function TranslateDlg(hWndParent:HWND; pszTitle:PChar): HRESULT; stdcall;
 end;

 ITTSDialogs = ITTSDialogsA;
 PITTSDIALOGS = PITTSDIALOGSA;

{ ITTSEnum }

const
 IID_ITTSEnumA: TGUID = '{05EB6C6D-DBAB-11CD-B3CA-00AA0047BA4F}';
 IID_ITTSEnum: TGUID = '{05EB6C6D-DBAB-11CD-B3CA-00AA0047BA4F}'; //IID_ITTSEnumA

type
 PITTSEnumA = ^ITTSEnumA;
 ITTSEnumA = interface(IUnknown)
  ['{05EB6C6D-DBAB-11CD-B3CA-00AA0047BA4F}']
   function Next(celt: ULONG; out rgelt: PTTSMODEINFO;
     var pceltFetched: ULONG): HRESULT; stdcall;
   function Skip(celt: ULONG): HRESULT; stdcall;
   function Reset: HRESULT; stdcall;
   function Clone(out ppenum: ITTSEnumA): HRESULT;
   function Select(gModeID: TGUID; ppiTTSCentral: PITTSCENTRAL;
     pIUnknownForAudio: PUnknown): HRESULT; stdcall;
  end;

 ITTSEnum = ITTSEnumA;
 PITTSEnum = PITTSEnumA;

{ ITTSFind }

const
 IID_ITTSFindA: TGUID = '{05EB6C6E-DBAB-11CD-B3CA-00AA0047BA4F}';
 IID_ITTSFind: TGUID = '{05EB6C6E-DBAB-11CD-B3CA-00AA0047BA4F}';

type
 PITTSFindA = ^ITTSFindA;
 ITTSFindA = interface(IUnknown)
  function Find(pttsInfo: PTTSMODEINFO; pttsInfoRank: PTTSMODEINFORANK;
    var pttsInfoFound: TTTSMODEINFO): HRESULT; stdcall;
  function Select(gModeID: TGUID; var ppiTTSCentral: ITTSCENTRAL;
    pIUnknownForAudio: PUNKNOWN): HRESULT; stdcall;
  end;

  ITTSFind = ITTSFindA;
  PITTSFind = PITTSFindA;

const ONE = 1;

// dwFlags parameter of IVoiceText::Register
//#define  VTXTF_ALLMESSAGES      (ONE<<0)


//types of speech
  vtxtst_STATEMENT = 1;
  vtxtst_QUESTION = 2;
  vtxtst_COMMAND = 4;
  vtxtst_WARNING = 8;
  vtxtst_READING = 16;
  vtxtst_NUMBERS = 32;
  vtxtst_SPREADSHEET = 64;
  
  vtxtsp_VERYHIGH = 128;
  vtxtsp_HIGH = 256;
  vtxtsp_NORMAL = 512;


// possible parameter to IVoiceText::Register

type
 PVTSiteInfo = ^TVTSiteInfo;
 TVTSiteInfo = record
    dwDevice,
    dwEnable,
    dwSpeed:DWORD;
    gModeID:TGUID;
 end;

{ IVTxtAttributes }

const
 IID_IVTxtAttributesA: TGUID = '{8BE9CC30-E095-11cd-A166-00AA004CD65C}';
 IID_IVTxtAttributes: TGUID = '{8BE9CC30-E095-11cd-A166-00AA004CD65C}';

type
 IVTxtAttributesA = interface(IUnknown)
  ['{8BE9CC30-E095-11cd-A166-00AA004CD65C}']
   function DeviceGet(var dwDeviceID: dword): HRESULT; stdcall;
   function DeviceSet(dwDeviceID: dword): HRESULT; stdcall;
   function EnabledGet(var dwEnabled: dword): HRESULT; stdcall;
   function EnabledSet(dwEnabled: dword): HRESULT; stdcall;
   function IsSpeaking(var bIsSpeak: BOOL): HRESULT; stdcall;
   function SpeedGet(var dwSpeed: dword): HRESULT; stdcall;
   function SpeedSet(dwSpeed: dword): HRESULT; stdcall;
   function TTSModeGet(var g: TGUID): HRESULT; stdcall;
   function TTSModeSet(g: TGUID): HRESULT; stdcall;
 end;

 IVTxtAttributes = IVTxtAttributesA;

{ IVTxtDialogsA }

const
 IID_IVTxtDialogsA: TGUID = '{E8F6FA20-E095-11cd-A166-00AA004CD65C}';
 IID_IVTxtDialogs: TGUID = '{E8F6FA20-E095-11cd-A166-00AA004CD65C}';

type
 IVTxtDialogsA = interface(IUnknown)
  ['{E8F6FA20-E095-11cd-A166-00AA004CD65C}']
   function AboutDlg(hWndParent:HWND; pszTitle: PChar): HRESULT; stdcall;
   function LexiconDlg(hWndParent:HWND; pszTitle: PChar): HRESULT; stdcall;
   function GeneralDlg(hWndParent:HWND; pszTitle: PChar): HRESULT; stdcall;
   function TranslateDlg(hWndParent:HWND; pszTitle: PChar): HRESULT; stdcall;
 end;

 IVTxtDialogs = IVTxtDialogsA;

const
 IID_IVTxtNotifySink: TGUID = '{D2C840E0-E092-11cd-A166-00AA004CD65C}';

type
 IVTxtNotifySink = interface(IUnknown)
  ['{D2C840E0-E092-11cd-A166-00AA004CD65C}']
  function AttribChanged(dwAttribute: DWORD): HRESULT; stdcall;
  function Visual(cIPAPhoneme: WideChar; cEnginePhoneme: char; dwHints: dword;
    pTTSMouth: PTTSMOUTH): HRESULT; stdcall;
  function Speak(pszText, pszApplication: WideString; dwType: dword): HRESULT; stdcall;
  function SpeakingStarted: HRESULT; stdcall;
  function SpeakingDone: HRESULT; stdcall;
 end;

const
 IID_IVoiceText: TGUID = '{E1B7A180-E093-11cd-A166-00AA004CD65C}';

type
 IVoiceText = interface(IUnknown)
    function Register(Site, Application :WideString; sink: IVTxtNotifySink; IID: TGUID;
      dwflags: dword; SiteInfo: PVTSiteInfo): HRESULT; stdcall;
    function Speak(wText: PChar; dwAttributes: DWORD; pszTags: WideString): HRESULT; stdcall;
    function StopSpeaking: HRESULT; stdcall;
    function AudioFastForward: HRESULT; stdcall;
    function AudioPause: HRESULT; stdcall;
    function AudioResume: HRESULT; stdcall;
    function AudioRewind: HRESULT; stdcall;
 end;

const
 CLSID_VTxt: TGUID = '{F1DC95A0-0BA7-11ce-A166-00AA004CD65C}';

// Multimedia Interface Object

 CLSID_MMAudioDest: TGUID = '{cb96b400-c743-11cd-80e5-00aa003e4b50}';

{ IAudioMultiMediaDevice }

const
 IID_IAudioMultiMediaDevice: TGUID = '{B68AD320-C743-11cd-80E5-00AA003E4B50}';

type
 PIAudioMultiMediaDevice = ^IAudioMultiMediaDevice;
 IAudioMultiMediaDevice = interface(IUnknown)
  ['{B68AD320-C743-11cd-80E5-00AA003E4B50}']
   function CustomMessage(uMsg: integer; dData: TSData): HRESULT; stdcall;
   function DeviceNumGet(var pdwDeviceID: DWORD): HRESULT; stdcall;
   function DeviceNumSet(dwDeviceID: DWORD): HRESULT; stdcall;
 end;

{ Speech Recognition Routines }
 
type
 PVCmdName = ^TVCmdName;
 TVCmdName = record
  szApplication:array[0..31] of char;
  szState:array[0..31] of char;
 end;

type
 PVCSiteInfo = ^TVCSiteInfo;
 TVCSiteInfo = record
  dwAutoGainEnable: dword;
  dwAwakeState: dword;
  dwThreshold: dword;
  dwDevice: dword;
  dwEnable: dword;
  szMicrophone: array[0..31] of char;
  szSpeaker: array[0..31] of char;
  gModeID: TGUID;
 end;

const
 VCMDMC_CREATE_TEMP     = $1;
 VCMDMC_CREATE_NEW      = $2;
 VCMDMC_CREATE_ALWAYS   = $4;
 VCMDMC_OPEN_ALWAYS     = $8;
 VCMDMC_OPEN_EXISTING   = $10;

{ IVCmdNotifySink }

const
 IID_IVCmdNotifySink: TGUID = '{80B25CC0-5540-11b9-C000-5611722E1D15}';

type
 IVCmdNotifySink = interface(IUnknown)
  ['{80B25CC0-5540-11b9-C000-5611722E1D15}']
   function CommandRecognize(dwID: dword; CmdName: PVCMDNAME;  dwFlags, dwActionSize: DWORD;
     pAction: PVOID; pszListValues, pszCommand: PChar): HRESULT; stdcall;
   function CommandOther(pName: PVCMDNAME; pszCommand: PChar): HRESULT; stdcall;
   function CommandStart: HRESULT; stdcall;
   function MenuActivate(pName:PVCMDNAME; bActive: BOOL): HRESULT; stdcall;
   function UtteranceBegin: HRESULT; stdcall;
   function UtteranceEnd: HRESULT; stdcall;
   function VUMeter(wLevel: word): HRESULT; stdcall;
   function AttribChanged(dwAttribute: dword): HRESULT; stdcall;
   function Interference(dwType: DWORD): HRESULT; stdcall;
 end;

{ IVCmdEnum }

const
 IID_IVCmdEnum: TGUID = '{E86F9540-DCA2-11CD-A166-00AA004CD65C}';

type
 IVCmdEnum = interface(IUnknown)
  ['{E86F9540-DCA2-11CD-A166-00AA004CD65C}']
   function Next(celt: ULONG; out rgelt: PVCMDNAME;
     var celtFetched: ULONG): HRESULT; stdcall;
   function Skip(celt: ULONG): HRESULT; stdcall;
   function Reset: HRESULT; stdcall;
   function Clone(out vcenum: IVCmdEnum): HRESULT; stdcall;
 end;

{ IEnumSRShare }

//const
// IID_IEnumSRShare: TGUID = '{E97F05C1-81B3-11ce-B763-00AA004CD65C}';

{type
 IEnumSRShare = interface(IUnknown)}
//  ['{E97F05C1-81B3-11ce-B763-00AA004CD65C}']
{   STDMETHOD (Next)           (THIS_ ULONG, PSRSHAREA, ULONG *) PURE;
   STDMETHOD (Skip)           (THIS_ ULONG) PURE;
   function Reset: HRESULT; stdcall;
   STDMETHOD (Clone)          (THIS_ IEnumSRShareA * FAR *) PURE;
   STDMETHOD (New)            (THIS_ DWORD, GUID, PISRCENTRALA *, QWORD *) PURE;
   STDMETHOD (Share)          (THIS_ QWORD, PISRCENTRALA *) PURE;
   STDMETHOD (Detach)         (THIS_ QWORD) PURE;
 end;}

{ IVCmdMenu }

const
 IID_IVCmdMenu: TGUID = '{746141E0-5543-11b9-C000-5611722E1D15}';

type
 IVCmdMenu = interface(IUnknown)
  ['{746141E0-5543-11b9-C000-5611722E1D15}']
   function Activate(hWndListening: THandle; dwFlags: dword): HRESULT; stdcall;
   function Deactivate: HRESULT; stdcall;
   function TrainMenuDlg(hWndParent: THandle; pTitle: PChar): HRESULT; stdcall;
   function Num(var numCmd: dword): HRESULT; stdcall;
   function Get(dwCmdStart, dwCmdNum, dwFlag: DWORD; pdData:PSDATA;
     var dwCmdNumCompleted: DWORD): HRESULT; stdcall;
   function _Set(dwCmdStart, dwCmdNum, dwFlag: DWORD; dData: TSData): HRESULT; stdcall;
   function Add(dwCmdNum: DWORD; dData: TSDATA; var dwCmdStart:DWORD): HRESULT; stdcall;
   function Remove(dwCmdStart, dwCmdNum, dwFlag: DWORD): HRESULT; stdcall;
   function ListGet(pszList: PChar; pList: PSDATA; var dwListNum: DWORD): HRESULT; stdcall;
   function ListSet(pszList: PChar; dwListNum: DWORD; dList: TSDATA): HRESULT; stdcall;
   function EnableItem(dwEnable, dwCmdNum, dwFlag: DWORD): HRESULT; stdcall;
   function SetItem(dwEnable, dwCmdNum, dwFlag: DWORD): HRESULT; stdcall; 
 end;

{ IVoiceCmd }

const
 IID_IVoiceCmd: TGUID = '{C63A2B30-5543-11b9-C000-5611722E1D15}';

type
 IVoiceCmd = interface(IUnknown)
  ['{C63A2B30-5543-11b9-C000-5611722E1D15}']
   function Register(pszSite: PChar; NotifyInterface: IVCmdNotifySink;
     IIDNotifyInterface: TGUID; dwFlags: dword; SiteInfo: PVCSiteInfo): HRESULT; stdcall;
   function MenuEnum(dwFlags: DWORD; pszApplicationFilter, pszStateFilter: PChar;
     var VCmdEnum:IVCmdEnum): HRESULT; stdcall;
   function MenuCreate(pName: TVCMDNAME; pLang: PLanguage; dwFlags: dword;
     var CmdMenu: IVCmdMenu): HRESULT; stdcall;
   function MenuDelete(pName: PVCMDNAME): HRESULT; stdcall;
   function CmdMimic(pMenu: PVCMDNAME; pszCommand: PChar): HRESULT; stdcall;
 end;

{ IVCmdAttributes }

const
 IID_IVCmdAttributes: TGUID = '{FFF5DF80-5544-11b9-C000-5611722E1D15}';

type
 IVCmdAttributes = interface(IUnknown)
  ['{FFF5DF80-5544-11b9-C000-5611722E1D15}']
   function AutoGainEnableGet(var dwAutoGain: dword): HRESULT; stdcall;
   function AutoGainEnableSet(dwAutoGain: dword): HRESULT; stdcall;
   function AwakeStateGet(var dwAwake: dword): HRESULT; stdcall;
   function AwakeStateSet(dwAwake: dword): HRESULT; stdcall;
   function ThresholdGet(var dwThreshold: dword): HRESULT; stdcall;
   function ThresholdSet(dwThreshold: dword): HRESULT; stdcall;
   function DeviceGet(var dwDeviceID: dword): HRESULT; stdcall;
   function DeviceSet(dwDeviceID: dword): HRESULT; stdcall;
   function EnabledGet(var dwEnabled: dword): HRESULT; stdcall;
   function EnabledSet(dwEnabled: dword): HRESULT; stdcall;
   function MicrophoneGet(pszMicrophone: PChar; dwSize: DWORD;
     var dwNeeded:DWORD): HRESULT; stdcall;
   function MicrophoneSet(pszMicrophone: PChar): HRESULT; stdcall;
   function SpeakerGet(pszSpeaker: PChar; dwSize: dword; var dwNeeded: dword): HRESULT; stdcall;
   function SpeakerSet(pszSpeaker: PChar): HRESULT; stdcall;
   function SRModeGet(var gMode: TGUID): HRESULT; stdcall;
   function SRModeSet(gMode: TGUID): HRESULT; stdcall;
 end;

{ IVCmdDialog }

const
 IID_IVCmdDialogs: TGUID = '{AA8FE260-5545-11b9-C000-5611722E1D15}';

type
 IVCmdDialogs = interface(IUnknown)
  ['{AA8FE260-5545-11b9-C000-5611722E1D15}']
   function AboutDlg(hOwner: THandle; pCaption: PChar): HRESULT; stdcall;
   function GeneralDlg(hOwner: THandle; pCaption: PChar): HRESULT; stdcall;
   function LexiconDlg(hOwner: THandle; pCaption: PChar): HRESULT; stdcall;
   function TrainGeneralDlg(hOwner: THandle; pCaption: PChar): HRESULT; stdcall;
   function TrainMicDlg(hOwner: THandle; pCaption: PChar): HRESULT; stdcall;
 end;

const
 CLSID_VCmd: TGUID = '{6D40D820-0BA7-11ce-A166-00AA004CD65C}';
 CLSID_SRShare: TGUID = '{89F70C30-8636-11ce-B763-00AA004CD65C}';

implementation

function SetBit(x: byte): DWORD;
begin
Result := DWORD(1) shl x;
end;

end.
