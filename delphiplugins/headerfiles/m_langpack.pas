{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : m_langpack
 * Description : Converted Headerfile from m_langpack.h
 *
 * Author      : Christian Kästner
 * Date        : 19.06.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_langpack;

interface

uses windows;
//translates a single string into the user's local language
//wParam=0
//lParam=(LPARAM)(const char*)szEnglish
//returns a pointer to the localised string. If there is no known translation
//it will return szEnglish. The return value does not need to be freed in any
//way
const
  MS_LANGPACK_TRANSLATESTRING ='LangPack/TranslateString';

//translates a dialog into the user's local language
//wParam=0
//lParam=(LPARAM)(LANGPACKTRANSLATEDIALOG*)&lptd
//returns 0 on success, nonzero on failure
//This service only knows about the following controls:
//Window titles, STATIC, EDIT, Hyperlink, BUTTON
type
  TLANGPACKTRANSLATEDIALOG=record
    cbSize:integer;
    flags:dword;
    hwndDlg:thandle;
    ignoreControls:pointer;   //zero-terminated list of control IDs *not* to
                              //translate
  end;
const
  LPTDF_NOIGNOREEDIT =  1;   //translate all edit controls. By default
                             //non-read-only edit controls are not translated
  LPTDF_NOTITLE      =  2;   //do not translate the title of the dialog

  MS_LANGPACK_TRANSLATEDIALOG  ='LangPack/TranslateDialog';

//translates a menu into the user's local language
//wParam=(WPARAM)(HMENU)hMenu
//lParam=0
//returns 0 on success, nonzero on failure
const
  MS_LANGPACK_TRANSLATEMENU    ='LangPack/TranslateMenu';

implementation

end.
