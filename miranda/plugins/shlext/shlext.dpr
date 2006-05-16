{$IFDEF FPC}
    {$PACKRECORDS 4}
    {$MODE Delphi}
    {$ASMMODE intel}
    {$INLINE ON}
    {$MACRO ON}
    {$APPTYPE GUI}
{$ELSE}
    {$IMAGEBASE $49ac0000} // this is ignored with FPC, must be set via the command line
{$ENDIF}

library shlext;
uses

    Windows, shlcom, shlipc, m_globaldefs;

    // use the registry to store the COM information needed by the shell

    function DllRegisterServer: HResult; stdcall;
    var
        szData: PChar;
        hRegKey: HKEY;
    begin

    {$IFDEF INSTALLER_REGISTER}
        Result := S_OK;
    {$ELSE}
        // progID
        szData := 'shlext (1.0.6.6) - shell context menu support for Miranda v0.3.0.0+';
        if ERROR_SUCCESS = RegSetValue(HKEY_CLASSES_ROOT, 'miranda.shlext', REG_SZ, szData, Length(szData)) then
        begin
            // CLSID related to ProgID
            szData := '{72013A26-A94C-11d6-8540-A5E62932711D}';
            if ERROR_SUCCESS = RegSetValue(HKEY_CLASSES_ROOT, 'miranda.shlext\CLSID', REG_SZ, szData, Length(szData)) then
            begin
                // CLSID link back to progID
                szData := 'miranda.shlext';
                if ERROR_SUCCESS = RegSetValue(HKEY_CLASSES_ROOT, 'CLSID\{72013A26-A94C-11d6-8540-A5E62932711D}', REG_SZ, szData, Length(szData)) then
                begin
                    // CLSID link back to ProgID under \ProgID again?
                    szData := 'miranda.shlext';
                    if ERROR_SUCCESS = RegSetValue(HKEY_CLASSES_ROOT, 'CLSID\{72013A26-A94C-11d6-8540-A5E62932711D}\ProgID', REG_SZ, szData, Length(szData)) then
                    begin
                        GetMem(szData, MAX_PATH);
                        GetModuleFileName(hInstance, szData, MAX_PATH-1);
                        Result := RegSetValue(HKEY_CLASSES_ROOT, 'CLSID\{72013A26-A94C-11d6-8540-A5E62932711D}\InprocServer32', REG_SZ, szData, Length(szData));
                        FreeMem(szData);
                        if Result = ERROR_SUCCESS then
                        begin
                            // have to add threading model
                            szData := 'CLSID\{72013A26-A94C-11d6-8540-A5E62932711D}\InprocServer32';
                            Result := RegCreateKeyEx(HKEY_CLASSES_ROOT, szData, 0, nil, 0, KEY_SET_VALUE or KEY_CREATE_SUB_KEY, nil, hRegKey, nil);
                            if Result = ERROR_SUCCESS then
                            begin
                                szData := 'Apartment';
                                RegSetValueEx(hRegKey, 'ThreadingModel', 0, REG_SZ, PByte(szData), Length(szData)+1);
                                RegCloseKey(hRegKey);
                                // write which file types to show under
                                szData := '{72013A26-A94C-11d6-8540-A5E62932711D}';
                                // note that *\ should use AllFilesystemObjects for 4.71+
                                if ERROR_SUCCESS = RegSetValue(HKEY_CLASSES_ROOT, '*\shellex\ContextMenuHandlers\miranda.shlext', REG_SZ, szData, Length(szData)) then
                                begin
                                    // don't support directories
                                    if ERROR_SUCCESS = RegSetValue(HKEY_CLASSES_ROOT, 'Directory\shellex\ContextMenuHandlers\miranda.shlext', REG_SZ, szData, Length(szData)) then
                                    begin
                                        Result := S_OK;
                                        // have to add to the approved list under NT/2000/XP with {CLSID}="<description>"
                                        szData := 'SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved';
                                        Result := RegCreateKeyEx(HKEY_LOCAL_MACHINE, szData, 0, nil, 0, KEY_SET_VALUE or KEY_CREATE_SUB_KEY, nil, hRegKey, nil);
                                        if Result = ERROR_SUCCESS then
                                        begin
                                            szData := 'shlext (1.0.6.6) - context menu support for Miranda v0.3.0.0+';
                                            RegSetValueEx(hRegKey, '{72013A26-A94C-11d6-8540-A5E62932711D}', 0, REG_SZ, PByte(szData), Length(szData)+1);
                                            RegCloseKey(hRegKey);
                                        end; //if
                                    end else Result := E_FAIL;
                                end else Result := E_FAIL;
                            end else Result := E_FAIL;
                        end else Result := E_FAIL;
                    end else Result := E_FAIL;
                end else Result := E_FAIL;
            end else Result := E_FAIL;
        end else Result := E_FAIL;
        //
    {$ENDIF}
    end;

    function DllUnregisterServer: HResult; stdcall;
    var
        hRootKey: HKEY;
    begin
        if RegOpenKeyEx(HKEY_CLASSES_ROOT, 'miranda.shlext', 0, KEY_READ, hRootKey) = ERROR_SUCCESS then
        begin
            (* need to delete the subkey before the parent key is deleted under NT/2000/XP *)
            RegDeleteKey(hRootKey, 'CLSID');
            (* close the key *)
            RegCloseKey(hRootKey);
            (* delete it *)
            if RegDeleteKey(HKEY_CLASSES_ROOT, 'miranda.shlext') <> ERROR_SUCCESS then
            begin
                MessageBox(0, 'Unable to delete registry key for "shlext COM", this key may already be deleted or you may need admin rights.', 'Problem', MB_ICONERROR);
            end; //if
        end; //if
        if RegOpenKeyEx(HKEY_CLASSES_ROOT, '\*\shellex\ContextMenuHandlers', 0, KEY_ALL_ACCESS, hRootKey) = ERROR_SUCCESS then
        begin
            if RegDeleteKey(hRootKey, 'miranda.shlext') <> ERROR_SUCCESS then
            begin
                MessageBox(0, 'Unable to delete registry key for "File context menu handlers", this key may already be deleted or you may need admin rights.', 'Problem', MB_ICONERROR);
            end; //if
            RegCloseKey(hRootKey);
        end; //if
        if RegOpenKeyEx(HKEY_CLASSES_ROOT, 'Directory\shellex\ContextMenuHandlers', 0, KEY_ALL_ACCESS, hRootKey) = ERROR_SUCCESS then
        begin
            if RegDeleteKey(hRootKey, 'miranda.shlext') <> ERROR_SUCCESS then
            begin
                MessageBox(0, 'Unable to delete registry key for "Directory context menu handlers", this key may already be deleted or you may need admin rights.', 'Problem', MB_ICONERROR);
            end; //if
            RegCloseKey(hRootKey);
        end; //if
        if ERROR_SUCCESS = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 'Software\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved', 0, KEY_ALL_ACCESS, hRootKey) then
        begin
            if RegDeleteValue(hRootKey, '{72013A26-A94C-11d6-8540-A5E62932711D}') <> ERROR_SUCCESS then
            begin
                MessageBox(0, 'Unable to delete registry entry for "Approved context menu handlers", this key may already be deleted or you may need admin rights.', 'Problem', MB_ICONERROR);
            end; //if
            RegCloseKey(hRootKey);
        end; //if
        Result := S_OK;
    end;


    // - miranda section ----

    {$include m_options.inc}
    {$include m_system.inc}
    {$include m_database.inc}
    {$include m_file.inc}
    {$include m_langpack.inc}
    {$include m_helpers.inc}

const

    COMREG_UNKNOWN  = $00000000;
    COMREG_OK       = $80000000;
    COMREG_APPROVED = $08000000;

    function IsCOMRegistered: Integer;
    var
        hRegKey: HKEY;
        lpType: Integer;
    begin
        Result := 0;
        // these arent the BEST checks in the world
        if ERROR_SUCCESS = RegOpenKeyEx(HKEY_CLASSES_ROOT, 'miranda.shlext', 0, KEY_READ, hRegKey) then
        begin
            Inc(Result, COMREG_OK);
            RegCloseKey(hRegKey);
        end; //if
        lpType := REG_SZ;
        if ERROR_SUCCESS = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 'Software\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved', 0, KEY_READ, hRegKey) then
        begin
            if ERROR_SUCCESS = RegQueryValueEx(hRegKey, '{72013A26-A94C-11d6-8540-A5E62932711D}', nil, @lpType, nil, nil) then
            begin
                Inc(Result, COMREG_APPROVED);
            end; //if
            RegCloseKey(hRegKey);
        end; // if
    end;

    procedure AutoSize(hwnd: THandle);
    var
        szBuf: array[0..MAX_PATH] of Char;
        DC: HDC;
        tS: TSize;
        i: Integer;
        hFont, hOldFont: THandle;
    begin
        DC := GetDC(hwnd);
        hFont := GetStockObject(DEFAULT_GUI_FONT);
        hOldFont := SelectObject(DC, hFont);
        i := GetWindowText(hwnd, szBuf, MAX_PATH);
        GetTextExtentPoint32(DC, szBuf, i, tS);
        SelectObject(DC, hOldFont);
        DeleteObject(hFont);
        ReleaseDC(hwnd, DC);
        SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, tS.cx + 10, tS.cy, SWP_NOMOVE or SWP_FRAMECHANGED);
    end;

    function OptDialogProc(hwndDlg: THandle; wMsg: Integer; wParam: WPARAM; lParam: LPARAM): BOOL; stdcall;
    // don't wanna bring in CommCtrl just for a few constants
    const
        {$IFNDEF FPC}
        WM_INITDIALOG            = $0110;
        WM_COMMAND               = $0111;
        WM_USER                  = $0400;
        WM_NOTIFY                = $004E;
        {$ENDIF}
        { propsheet notifications/msessages }
        PSN_APPLY                = (-200) - 2;
        PSM_CHANGED              = WM_USER + 104;
        { hotkey }
        // bring in the IDC's and storage key names
        {$define SHL_IDC}
        {$define SHL_KEYS}
            {$include shlc.inc}
        {$undef SHL_KEYS}
        {$undef SHL_IDC}
    const
        COM_OKSTR: array[Boolean] of PChar = (
            'Problem, registration missing/deleted.',
            'Successfully created shell registration.'
        );
        COM_APPROVEDSTR: array[Boolean] of PChar = (
            'Not Approved',
            'Approved'
        );
    var
        comReg: Integer;
        iCheck: Integer;
        szBuf: array[0..MAX_PATH] of Char;
        cgs: TDBCONTACTGETSETTING;
    begin
        Result := wMsg = WM_INITDIALOG;
        case wMsg of
            WM_INITDIALOG:
            begin
                TranslateDialogDefault(hwndDlg);
                comReg := IsCOMRegistered();
                FillChar(szBuf, MAX_PATH, 0);
                lstrcat(szBuf, Translate(COM_OKSTR[comReg and COMREG_OK = COMREG_OK]));
                lstrcat(szBuf, ' (');
                lstrcat(szBuf, Translate(COM_APPROVEDSTR[comReg and COMREG_APPROVED = COMREG_APPROVED]));
                lstrcat(szBuf, ')');
                SetWindowText(GetDlgItem(hwndDlg, IDC_STATUS), szBuf);
                // auto size the static windows to fit their text
                // they're rendering in a font not selected into the DC.
                AutoSize(GetDlgItem(hwndDlg, IDC_CAPMENUS));
                AutoSize(GetDlgItem(hwndDlg, IDC_CAPSTATUS));
                AutoSize(GetDlgItem(hwndDlg, IDC_CAPSHLSTATUS));
                // show all the options
                iCheck := DBGetContactSettingByte(0, SHLExt_Name, SHLExt_UseGroups, BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_USEGROUPS, iCheck);
                EnableWindow(GetDlgItem(hwndDlg, IDC_CLISTGROUPS), iCheck = BST_CHECKED);
                CheckDlgButton(hwndDlg, IDC_CLISTGROUPS, DBGetContactSettingByte(0, SHLExt_Name, SHLExt_UseCListSetting, BST_UNCHECKED));
                CheckDlgButton(hwndDlg, IDC_NOPROF, DBGetContactSettingByte(0, SHLExt_Name, SHLExt_ShowNoProfile, BST_UNCHECKED));
                CheckDlgButton(hwndDlg, IDC_SHOWFULL, DBGetContactSettingByte(0, SHLExt_Name, SHLExt_UseHITContacts, BST_UNCHECKED));
                CheckDlgButton(hwndDlg, IDC_SHOWINVISIBLES, DBGetContactSettingByte(0, SHLExt_Name, SHLExt_UseHIT2Contacts, BST_UNCHECKED));
				CheckDlgButton(hwndDlg, IDC_USEOWNERDRAW, DBGetContactSettingByte(0, SHLExt_Name, SHLExt_ShowNoIcons, BST_UNCHECKED));
				CheckDlgButton(hwndDlg, IDC_HIDEOFFLINE, DBGetContactSettingByte(0, SHLExt_Name, SHLExt_ShowNoOffline, BST_UNCHECKED));				
            end;
            WM_COMMAND:
            begin
                // don't send the changed message if remove is clicked
                if LOWORD(wParam) <> IDC_REMOVE then
                begin
                    SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
                end; //if
                case LOWORD(wParam) of
                    IDC_USEGROUPS:
                    begin
                        EnableWindow(GetDlgItem(hwndDlg, IDC_CLISTGROUPS), BST_CHECKED = IsDlgButtonChecked(hwndDlg, IDC_USEGROUPS));
                    end; //if
                    IDC_REMOVE:
                    begin
                        if IDYES = MessageBox(0, Translate('Are you sure? this will remove all the settings stored in your database and all registry entries created for shlext to work with Explorer'), Translate('Disable/Remove shlext'), MB_YESNO or MB_ICONQUESTION) then
                        begin
                            cgs.szModule := SHLExt_Name;

                            cgs.szSetting := SHLExt_UseGroups;
                            CallService(MS_DB_CONTACT_DELETESETTING, 0, Integer(@cgs));

                            cgs.szSetting := SHLExt_UseCListSetting;
                            CallService(MS_DB_CONTACT_DELETESETTING, 0, Integer(@cgs));

                            cgs.szSetting := SHLExt_UseHITContacts;
                            CallService(MS_DB_CONTACT_DELETESETTING, 0, Integer(@cgs));

                            cgs.szSetting := SHLExt_UseHIT2Contacts;
                            CallService(MS_DB_CONTACT_DELETESETTING, 0, Integer(@cgs));

                            cgs.szSetting := SHLExt_ShowNoProfile;
                            CallService(MS_DB_CONTACT_DELETESETTING, 0, Integer(@cgs));
							
							cgs.szSetting := SHLExt_ShowNoIcons;
							CallService(MS_DB_CONTACT_DELETESETTING, 0, Integer(@cgs));
						
							cgs.szSetting := SHLExt_ShowNoOffline;
							CallService(MS_DB_CONTACT_DELETESETTING, 0, Integer(@cgs));						

                            (* remove from Explorer *)
                            DllUnregisterServer();
                            (* show all the settings have gone... *)
                            SendMessage(hwndDlg, WM_INITDIALOG, 0, 0);
                        end; //if
                    end; //if
                end; //case
                // LOWORD(wParam) == IDC_*
            end;{outercase}
            WM_NOTIFY:
            begin
                if pNMHDR(lParam)^.code = PSN_APPLY then
                begin
                    DBWriteContactSettingByte(0, SHLExt_Name, SHLExt_UseGroups, IsDlgButtonChecked(hwndDlg, IDC_USEGROUPS));
                    DBWriteContactSettingByte(0, SHLExt_Name, SHLExt_UseCListSetting, IsDlgButtonChecked(hwndDlg, IDC_CLISTGROUPS));
                    DBWriteContactSettingByte(0, SHLExt_Name, SHLExt_ShowNoProfile, IsDlgButtonChecked(hwndDlg, IDC_NOPROF));
                    DBWriteContactSettingByte(0, SHLExt_Name, SHLExt_UseHITContacts, IsDlgButtonChecked(hwndDlg, IDC_SHOWFULL));
                    DBWriteContactSettingByte(0, SHLExt_Name, SHLExt_UseHIT2Contacts, IsDlgButtonChecked(hwndDlg, IDC_SHOWINVISIBLES));
					DBWriteContactSettingByte(0, SHLExt_Name, SHLExt_ShowNoIcons, IsDlgButtonChecked(hwndDlg, IDC_USEOWNERDRAW));
					DBWriteContactSettingByte(0, SHLExt_Name, SHLExt_ShowNoOffline, IsDlgButtonChecked(hwndDlg, IDC_HIDEOFFLINE));
                end; //if
            end;
        end; //case
    end;

    function InitialiseOptionPages(wParam: WPARAM; lParam: LPARAM): int; cdecl;
    const
        IDD_SHLOPTS = 101;
    var
        optDialog: TOPTIONSDIALOGPAGE;
    begin
        Result := 0;
        FillChar(optDialog, sizeof(TOPTIONSDIALOGPAGE), 0);
        optDialog.cbSize := sizeof(TOPTIONSDIALOGPAGE);
        optDialog.flags := ODPF_BOLDGROUPS;
        optDialog.groupPosition := 0;
        optDialog.pszGroup := 'Plugins';
        optDialog.position := -1066;
        optDialog.pszTitle := Translate('Shell context menus');
        optDialog.pszTemplate := MAKEINTRESOURCE(IDD_SHLOPTS);
        {$ifdef VER140}
        optDialog.hInstance := HInstance;
        {$else}
        optDialog.hInstance := System.hInstance;
        {$endif}
        optDialog.pfnDlgProc := @OptDialogProc;
        PluginLink^.CallService(MS_OPT_ADDPAGE, wParam, Integer(@optDialog));
    end;

    function MirandaPluginInfo(mirandaVersion: DWORD): PPLUGININFO; cdecl;
    begin
        Result := nil;
        { only support v0.3.0.0+ }
        if PLUGIN_MAKE_VERSION(0,3,0,0) > MirandaVersion then Exit;
        { fill in plugininfo }
        PLUGININFO.cbSize := sizeof(PLUGININFO);
        PLUGININFO.shortName := 'Shell context menus for transfers';
        PLUGININFO.version := PLUGIN_MAKE_VERSION(1,0,6,6);
        {$ifdef FPC}
        PLUGININFO.description := 'Click ''n'' send support from Explorer/Common dialogs/Desktop, Right click on a file/folder to be presented with all your Miranda contact lists and then select the profile/contact you want to send to. Built on ' + {$I %DATE%} + ' at ' + {$I %TIME%};
        {$else}
        PLUGININFO.description := '';
        {$endif}
        PLUGININFO.author := 'egoDust';
        PLUGININFO.authorEmail := 'egodust@users.sourceforge.net';
        PLUGININFO.copyright := '';
        PLUGININFO.homePage := 'http://miranda-icq.sourceforge.net/';
        PLUGININFO.isTransient := 0;
        PLUGININFO.replacesDefaultModule := 0;
        { return info }
        Result := @PLUGININFO;
    end;

    function Load(link: PPLUGINLINK): int; cdecl;
    begin
        Result := 0;
        PLUGINLINK := Pointer(link);
		InvokeThreadServer;
        PluginLink^.HookEvent(ME_OPT_INITIALISE, InitialiseOptionPages);        
        DllRegisterServer();
        //DisableThreadLibraryCalls(System.hInstance);
    end;

    function Unload: int; cdecl;
    begin
        Result := 0;
    end;

    {$R shldlgs.res}

    exports

        MirandaPluginInfo, Load, Unload;

    exports

        DllGetClassObject, DllCanUnloadNow, DllRegisterServer, DllUnregisterServer;

begin
end.
