unit shlcom;

{$IFDEF FPC}
    {$PACKRECORDS 4}
    {$MODE Delphi}
{$ENDIF}
                                                   
interface

uses

    Windows, m_globaldefs, shlipc;

    {$define COM_STRUCTS}
    {$define SHLCOM}
    {$include shlc.inc}
    {$undef SHLCOM}
    {$undef COM_STRUCTS}

    function DllGetClassObject(const CLSID: TCLSID; const IID: TIID;
             var Obj): HResult; stdcall;
    function DllCanUnloadNow: HResult; stdcall;

    procedure InvokeThreadServer;

    function ExtractIcon(hInst: THandle; pszExe: PChar; nIndex: Integer): HICON; stdcall; external 'shell32.dll' name 'ExtractIconA';

implementation

var

    dllpublic: record
        FactoryCount: Integer;
        ObjectCount: Integer;
    end;

    {$include m_database.inc}
    {$include m_clist.inc}
    {$include m_protocols.inc}
	{$include m_protosvc.inc}
    {$include m_ignore.inc}
    {$include m_skin.inc}
    {$include m_file.inc}
    {$include m_system.inc}
	{$include m_langpack.inc}
	{$include m_skin.inc}
	{$include statusmodes.inc}

    {$define COMAPI}
    {$include shlc.inc}
    {$undef COMAPI}

    {$include m_helpers.inc}

const

    IPC_PACKET_SIZE = $1000 * 32;
    //IPC_PACKET_NAME = 'm.mi.miranda.ipc'; // prior to 1.0.6.6
    IPC_PACKET_NAME = 'mi.miranda.IPCServer';

const

{ Flags returned by IContextMenu*:QueryContextMenu() }

    CMF_NORMAL             = $00000000;
	CMF_DEFAULTONLY        = $00000001;
	CMF_VERBSONLY          = $00000002;
	CMF_EXPLORE            = $00000004;
	CMF_NOVERBS            = $00000008;
	CMF_CANRENAME          = $00000010;
	CMF_NODEFAULT          = $00000020;
	CMF_INCLUDESTATIC      = $00000040;
    CMF_RESERVED           = $FFFF0000;      { view specific }

{ IContextMenu*:GetCommandString() uType flags }

	GCS_VERBA              = $00000000; // canonical verb
	GCS_HELPTEXTA          = $00000001; // help text (for status bar)
	GCS_VALIDATEA          = $00000002; // validate command exists
	GCS_VERBW              = $00000004; // canonical verb (unicode)
	GC_HELPTEXTW           = $00000005; // help text (unicode version)
	GCS_VALIDATEW          = $00000006; // validate command exists (unicode)
	GCS_UNICODE            = $00000004; // for bit testing - Unicode string
	GCS_VERB               = GCS_VERBA; //
	GCS_HELPTEXT           = GCS_HELPTEXTA;
    GCS_VALIDATE           = GCS_VALIDATEA;

type

    { this structure is returned by InvokeCommand() }

    PCMInvokeCommandInfo = ^TCMInvokeCommandInfo;
    TCMInvokeCommandInfo = packed record
    	cbSize: DWORD;
        fMask: DWORD;
        hwnd: HWND;
        lpVerb: PChar;   { maybe index, type cast as Integer }
        lpParams: PChar;
        lpDir: PChar;
        nShow: Integer;
        dwHotkey: DWORD;
        hIcon: THandle;
    end;

{ completely stolen from modules.c: 'NameHashFunction' modified slightly  }

    function StrHash(const szStr: PChar): DWORD; cdecl;
    asm
            // esi content has to be preserved with basm
            push esi
    		xor  edx,edx
    		xor  eax,eax
    		mov  esi,szStr
       		mov  al,[esi]
    		xor  cl,cl
    @@lph_top:	                //only 4 of 9 instructions in here don't use AL, so optimal pipe use is impossible
    		xor  edx,eax
    		inc  esi
    		xor  eax,eax
    		and  cl,31 
    		mov  al,[esi]
    		add  cl,5
            test al,al
    		rol  eax,cl		    //rol is u-pipe only, but pairable
		                        //rol doesn't touch z-flag
	    	jnz  @@lph_top      //5 clock tick loop. not bad.
    		xor  eax,edx
            pop  esi
    end;

    function CreateProcessUID(const pid: Cardinal): string;
    var
        pidrep: string[16];
    begin
        str(pid, pidrep);
        Result := Concat('mim.shlext.', pidrep, '$');
    end;

    function CreateUID: string;
    var
        pidrep, tidrep: string[16];
    begin
        str(GetCurrentProcessId(), pidrep);
        str(GetCurrentThreadId(), tidrep);
        Result := Concat('mim.shlext.caller', pidrep, '$', tidrep);
    end;

    // FPC doesn't support array[0..n] of Char extended syntax with Str()

    function wsprintf(lpOut, lpFmt: PChar; ArgInt: Integer): Integer; cdecl; external 'user32.dll' name 'wsprintfA';

    procedure Str(i: Integer; S: PChar);
    begin
        i := wsprintf(s, '%d', i);
        if i > 2 then PChar(s)[i] := #0;
    end;

{ IShlCom }

type

    PLResult = ^LResult;

    // bare minimum interface of IDataObject, since GetData() is only required.

    PVTable_IDataObject = ^TVTable_IDataObject;
    TVTable_IDataObject = record
        { IUnknown }
        QueryInterface: Pointer;
        AddRef: function(Self: Pointer): Cardinal; stdcall;
        Release: function(Self: Pointer): Cardinal; stdcall;
        { IDataObject }
        GetData: function(Self: Pointer; var formatetcIn: TFormatEtc; var medium: TStgMedium): HResult; stdcall;
        GetDataHere: Pointer;
        QueryGetData: Pointer;
        GetCanonicalFormatEtc: Pointer;
        SetData: Pointer;
        EnumFormatEtc: Pointer;
        DAdvise: Pointer;
        DUnadvise: Pointer;
        EnumDAdvise: Pointer;
    end;

    PDataObject_Interface = ^TDataObject_Interface;
    TDataObject_Interface = record
        ptrVTable: PVTable_IDataObject;
    end;

    { TShlComRec inherits from different interfaces with different function tables
    all "compiler magic" is lost in this case, but it's pretty easy to return
    a different function table for each interface, IContextMenu is returned
    as IContextMenu'3' since it inherits from '2' and '1' }

    PVTable_IShellExtInit = ^TVTable_IShellExtInit;
    TVTable_IShellExtInit = record
        { IUnknown }
        QueryInterface: Pointer;
        AddRef: Pointer;
        Release: Pointer;
        { IShellExtInit }
        Initialise: Pointer;
    end;

    PShlComRec = ^TShlComRec;
    PShellExtInit_Interface = ^TShellExtInit_Interface;
    TShellExtInit_Interface = record
        { pointer to function table }
        ptrVTable: PVTable_IShellExtInit;
        { instance data }
        ptrInstance: PShlComRec;
        { function table itself }
        vTable: TVTable_IShellExtInit;
    end;

    PVTable_IContextMenu3 = ^TVTable_IContextMenu3;
    TVTable_IContextMenu3 = record
        { IUnknown }
        QueryInterface: Pointer;
        AddRef: Pointer;
        Release: Pointer;
        { IContextMenu }
        QueryContextMenu: Pointer;
        InvokeCommand: Pointer;
        GetCommandString: Pointer;
        { IContextMenu2 }
        HandleMenuMsg: Pointer;
        { IContextMenu3 }
        HandleMenuMsg2: Pointer;
    end;

    PContextMenu3_Interface = ^TContextMenu3_Interface;
    TContextMenu3_Interface = record
        ptrVTable: PVTable_IContextMenu3;
        ptrInstance: PShlComRec;
        vTable: TVTable_IContextMenu3;
    end;

    PCommon_Interface = ^TCommon_Interface;
    TCommon_Interface = record
        ptrVTable: Pointer;
        ptrInstance: PShlComRec;
    end;

    TShlComRec = record
        ShellExtInit_Interface: TShellExtInit_Interface;
        ContextMenu3_Interface: TContextMenu3_Interface;
    {fields}
        RefCount: LongInt;
        // this is owned by the shell after items are added 'n' is used to
        // grab menu information directly via id rather than array indexin'
        hRootMenu: THandle;
        idCmdFirst: Integer;
        // most of the memory allocated is on this heap object so HeapDestroy()
        // can do most of the cleanup, extremely lazy I know.
        hDllHeap: THandle;
		// array of all the protocol icons, for every running instance!
		ProtoIcons: ^TSlotProtoIconsArray;
		ProtoIconsCount: Cardinal;
        // maybe null, taken from IShellExtInit_Initalise() and AddRef()'d
        // only used if a Miranda instance is actually running and a user
        // is selected
        pDataObject: PDataObject_Interface;
		// DC is used for font metrics and saves on creating and destroying lots of DC handles
		// during WM_MEASUREITEM
		hMemDC: HDC;
    end;

    { this is passed to the enumeration callback so it can process PID's with
    main windows by the class name MIRANDANAME loaded with the plugin
    and use the IPC stuff between enumerations -- }

    PEnumData = ^TEnumData;
    TEnumData = record
        Self: PShlComRec;
        // autodetected, don't hard code since shells that don't support it
        // won't send WM_MEASUREITETM/WM_DRAWITEM at all.
        bOwnerDrawSupported: LongBool;
		// as per user setting (maybe of multiple Mirandas) 
		bShouldOwnerDraw: LongBool;
        idCmdFirst: Integer;
        ipch: PHeaderIPC;
        // OpenEvent()'d handle to give each IPC server an object to set signalled
        hWaitFor: THandle;
        pid: DWORD;                 // sub-unique value used to make work object name
    end;
    
    procedure FreeGroupTreeAndEmptyGroups(hParentMenu: THandle; pp, p: PGroupNode);
    var
        q: PGroupNode;
    begin
        while p <> nil do
        begin
            q := p^.Right;         
            if p^.Left <> nil then
            begin
                FreeGroupTreeAndEmptyGroups(p^.Left^.hMenu, p, p^.Left);
            end; //if
            if p^.dwItems = 0 then
            begin
                if pp <> nil then 
                begin
                    DeleteMenu(pp^.hMenu, p^.hMenuGroupID, MF_BYCOMMAND)
                end else begin
                    DeleteMenu(hParentMenu, p^.hMenuGroupID, MF_BYCOMMAND);
                end; //if
            end else begin
                // make sure this node's parent know's it exists
                if pp <> nil then inc (pp^.dwItems);
            end;
            Dispose(p);
            p := q;
        end;
    end;

    procedure DecideMenuItemInfo(pct: PSlotIPC; pg: PGroupNode; var mii: TMenuItemInfo; lParam: PEnumData);
    var
        psd: PMenuDrawInfo;
        hDllHeap: THandle;
		j, c: Cardinal;
		pp: ^TSlotProtoIconsArray;
    begin
        mii.wID := lParam^.idCmdFirst;
        Inc(lParam^.idCmdFirst);
        // get the heap object
        hDllHeap := lParam^.Self^.hDllHeap;
        psd := HeapAlloc(hDllHeap, 0, sizeof(TMenuDrawInfo));
        if pct <> nil then
        begin
            psd^.cch := pct^.cbStrSection-1; // no null;
            psd^.szText := HeapAlloc(hDllHeap, 0, pct^.cbStrSection);
            strcpy(psd^.szText, PChar(Integer(pct)+sizeof(TSlotIPC)));
            psd^.hContact := pct^.hContact;			
            psd^.fTypes := [dtContact];
			// find the protocol icon array to use and which status
			c := lParam^.Self^.protoIconsCount;
			pp := lParam^.Self^.protoIcons;
			psd^.hStatusIcon := 0;
			while c > 0 do
			begin
				dec(c);
				if (pp[c].hProto = pct^.hProto) and (pp[c].pid = lParam^.pid) then
				begin
					psd^.hStatusIcon := pp[c].hIcons[pct^.Status - ID_STATUS_OFFLINE];
					break;
				end;
			end; //while
            psd^.pid := lParam^.pid;
        end else if pg <> nil then
        begin
            // store the given ID
            pg^.hMenuGroupID := mii.wID;
            // steal the pointer from the group node it should be on the heap
            psd^.cch := pg^.cchGroup;
            psd^.szText := pg^.szGroup;
            psd^.fTypes := [dtGroup]; 
        end; //if
        psd^.wID := mii.wID;
        psd^.szProfile := nil;
        // store
        mii.dwItemData := Integer(psd);
        if ((lParam^.bOwnerDrawSupported) and (lParam^.bShouldOwnerDraw)) then
        begin
            mii.fType := MFT_OWNERDRAW;
            Pointer(mii.dwTypeData) := psd;
        end else begin
            // normal menu
            mii.fType := MFT_STRING;
            if pct <> nil then
            begin
                Integer(mii.dwTypeData) := Integer(pct) + sizeof(TSlotIPC);
            end else begin
                mii.dwTypeData := pg^.szGroup;
            end;
        end; //if
    end;
            
    procedure BuildContactTree(group: PGroupNode; lParam: PEnumData);
    label
        grouploop;
    var            
        pct: PSlotIPC;
        pg, px: PGroupNode;
        str: TStrTokRec;
        sz: PChar;
        Hash: Cardinal;
        Depth: Cardinal;
        mii: TMenuItemInfo;
    begin
        // set up the menu item
        mii.cbSize := sizeof(TMenuItemInfo);
        mii.fMask := MIIM_ID or MIIM_TYPE or MIIM_DATA;            
        // set up the scanner
        str.szSet := ['\'];
        str.bSetTerminator := False;
        // go thru all the contacts
        pct := lParam^.ipch^.ContactsBegin;
        while (pct <> nil) and (pct^.cbSize=sizeof(TSlotIPC)) and (pct^.fType=REQUEST_CONTACTS) do
        begin
            if pct^.hGroup <> 0 then
            begin
                // at the end of the slot header is the contact's display name
                // and after a double NULL char there is the group string, which has the full path of the group
                // this must be tokenised at '\' and we must walk the in memory group tree til we find our group
                // this is faster than the old version since we only ever walk one or at most two levels of the tree
                // per tokenised section, and it doesn't matter if two levels use the same group name (which is valid)
                // as the tokens processed is equatable to depth of the tree
                str.szStr := PChar(Integer(pct)+sizeof(TSlotIPC)+pct^.cbStrSection+1); 
                sz := StrTok(str);
                // restore the root
                pg := group;
                Depth := 0;
                while sz <> nil do
                begin                    
                    Hash := StrHash(sz);                    
                    // find this node within 
                    while pg <> nil do
                    begin
                        // does this node have the right hash and the right depth?
                        if (Hash = pg^.Hash) and (Depth = pg^.Depth) then Break;
                        // each node may have a left pointer going to a sub tree
                        // the path syntax doesn't know if a group is a group at the same level
                        // or a nested one, which means the search node can be anywhere
                        px := pg^.Left;
                        if px <> nil then
                        begin
                            // keep searching this level
                            while px <> nil do
                            begin   
                                if (hash = px^.Hash) and (Depth = px^.Depth) then
                                begin
                                    // found the node we're looking for at the next level to pg, px is now pq for next time
                                    pg := px;
                                    goto grouploop;
                                end; //if
                                px := px^.Right;
                            end; //if
                        end; //if
                        pg := pg^.Right;
                    end; //while
                grouploop:
                    inc (Depth);
                    // process next token
                    sz := StrTok(str);
                end; //while
                // tokenisation finished, if pg <> nil then the group is found
                if pg <> nil then
                begin
                    DecideMenuItemInfo(pct, nil, mii, lParam);
                    InsertMenuitem(pg^.hMenu, $FFFFFFFF,True, mii);
                    Inc(pg^.dwItems);
                end;
            end; //if
            pct := pct^.Next;
        end; //while
    end;  
                
    procedure BuildMenuGroupTree(p: PGroupNode; lParam: PEnumData; hLastMenu: HMENU);
    var
        mii: TMenuItemInfo;
    begin
        mii.cbSize := sizeof(TMenuItemInfo);
        mii.fMask := MIIM_ID or MIIM_DATA or MIIM_TYPE or MIIM_SUBMENU;
        // go thru each group and create a menu for it adding submenus too.
        while p <> nil do
        begin
            mii.hSubMenu := CreatePopupMenu();
            if p^.Left <> nil then BuildMenuGroupTree(p^.Left, lParam, mii.hSubMenu);
            p^.hMenu := mii.hSubMenu;
            DecideMenuItemInfo(nil, P, mii, lParam);
            InsertMenuItem(hLastMenu, $FFFFFFFF, True, mii);
            p := p^.Right;
        end; //while
    end;
    
    procedure BuildMenus(lParam: PEnumData);
    {$define SHL_IDC}
    {$define SHL_KEYS}
        {$include shlc.inc}
    {$undef SHL_KEYS}
    {$undef SHL_IDC}
    var
        hBaseMenu: HMENU;
        hGroupMenu: HMENU;
        pg: PSlotIPC;
        szProf: PChar;
        mii: TMenuItemInfo;
        j: TGroupNodeList;
        p, q: PGroupNode;
        Depth, Hash: Cardinal;
        Token: PChar;
        tk: TStrTokRec;
        szBuf: PChar;
        hDllHeap: THandle;
        psd: PMenuDrawInfo;
        c: Cardinal;
        pp: ^TSlotProtoIconsArray;
    begin
        hDllHeap := lParam^.Self^.hDllHeap;
        hBaseMenu := lParam^.Self^.hRootMenu;
        // build an in memory tree of the groups
        pg := lParam^.ipch^.GroupsBegin;
        tk.szSet := ['\'];
        tk.bSetTerminator := False;
        j.First := nil;
        j.Last := nil;
        while pg <> nil do
        begin
            if (pg^.cbSize <> sizeof(TSlotIPC)) or (pg^.fType <> REQUEST_GROUPS) then Break;
            Depth := 0;            
            p := j.First; // start at root again
            // get the group
            Integer(tk.szStr) := (Integer(pg) + sizeof(TSlotIPC));
            // find each word between \ and create sub groups if needed.
            Token := StrTok(tk);
            while Token <> nil do
            begin
                Hash := StrHash(Token);
                // if the (sub)group doesn't exist, create it.
                q := FindGroupNode(p, Hash, Depth);
                if q = nil then
                begin
                    q := AllocGroupNode(@j, p, Depth);
                    q^.Depth := Depth;
                    // this is the hash of this group node, but it can be anywhere
                    // i.e. Foo\Foo this is because each node has a different depth
                    // trouble is contacts don't come with depths!                    
                    q^.Hash := Hash;
                    // don't assume that pg^.hGroup's hash is valid for this token 
                    // since it maybe Miranda\Blah\Blah and we have created the first node
                    // which maybe Miranda, thus giving the wrong hash
                    // since "Miranda" can be a group of it's own and a full path
                    q^.cchGroup := lstrlen(Token);
                    q^.szGroup := HeapAlloc(hDllHeap, 0, q^.cchGroup+1);
                    strcpy(q^.szGroup, Token);
                    q^.dwItems := 0;                                       
                end;
                p := q;
                Inc(Depth);
                Token := StrTok(tk);
            end; //while
            pg := pg^.Next;
        end; // while
        // build the menus inserting into hGroupMenu which will be a submenu of
        // the instance menu item. e.g. Miranda -> [Groups ->] contacts
        hGroupMenu := CreatePopupMenu();
        // create group menus only if they exist!
        if lParam^.ipch^.GroupsBegin <> nil then
        begin
            BuildMenuGroupTree(j.First, lParam, hGroupMenu);
            // add contacts that have a group somewhere
            BuildContactTree(j.First, lParam);
        end;
        //
        mii.cbSize := sizeof(TMenuItemInfo);
        mii.fMask := MIIM_ID or MIIM_TYPE or MIIM_DATA;
        // add all the contacts that have no group (which maybe all of them)
        pg := lParam^.ipch^.ContactsBegin;
        while pg <> nil do
        begin
            if (pg^.cbSize <> sizeof(TSlotIPC)) or (pg^.fType <> REQUEST_CONTACTS) then Break;
            if pg^.hGroup = 0 then
            begin
                DecideMenuItemInfo(pg, nil, mii, lParam);
                InsertMenuItem(hGroupMenu, $FFFFFFFF, True, mii);
            end; //if
            pg := pg^.Next;
        end; //while

        mii.cbSize := sizeof(TMenuItemInfo);
        mii.fMask := MIIM_ID or MIIM_DATA or MIIM_TYPE or MIIM_SUBMENU;
        mii.hSubMenu := hGroupMenu;
			
        psd := HeapAlloc(hDllHeap, 0, sizeof(TMenuDrawInfo));
        psd^.cch := strlen(lParam^.ipch^.MirandaName);
        psd^.szText := HeapAlloc(hDllHeap, 0, psd^.cch+1);
        lstrcpyn(psd^.szText, lParam^.ipch^.MirandaName, sizeof(lParam^.ipch^.MirandaName)-1);
        // there may not be a profile name
        pg := lParam^.ipch^.DataPtr;
        psd^.szProfile := nil;
        if ((pg <> nil) and (pg^.Status = STATUS_PROFILENAME)) then
        begin
            psd^.szProfile := HeapAlloc(hDllHeap, 0, pg^.cbStrSection);
            strcpy(psd^.szProfile, PChar(Integer(pg) + sizeof(TSlotIPC)));
        end; //if
        // owner draw menus need ID's
        mii.wID := lParam^.idCmdFirst;
        Inc(lParam^.idCmdFirst);
        psd^.fTypes := [dtEntry];
        psd^.wID := mii.wID;
        psd^.hContact := 0;
        // get Miranda's icon
        c := lParam^.Self^.protoIconsCount;
        pp := lParam^.Self^.protoIcons;
        while c > 0 do
        begin
            dec(c);
            if (pp[c].pid = lParam^.pid) and (pp[c].hProto = 0) then
            begin
                psd^.hStatusIcon := pp[c].hIcons[0]; break;
            end; //if
        end; //while
        mii.dwItemData := Integer(psd);
        if ((lParam^.bOwnerDrawSupported) and (lParam^.bShouldOwnerDraw)) then
        begin
            mii.fType := MFT_OWNERDRAW;
            Pointer(mii.dwTypeData) := psd;
        end else begin
            mii.fType := MFT_STRING;               
            mii.dwTypeData := lParam^.ipch^.MirandaName;
			mii.cch := sizeof(lParam^.ipch^.MirandaName)-1;
        end;
        // add it all
        InsertMenuItem(hBaseMenu, 0, True, mii);
        // free the group tree
        FreeGroupTreeAndEmptyGroups(hGroupMenu, nil, j.First);
    end;
			
	procedure BuildSkinIcons(lParam: PEnumData);
	var
		pct: PSlotIPC;
		p, d: PSlotProtoIcons;
		Self: PShlComRec;
		j: Cardinal;
	begin
		pct := lParam^.ipch^.NewIconsBegin;
		self := lParam^.Self;
		while (pct <> nil) do
		begin
			if (pct^.cbSize <> sizeof(TSlotIPC)) or (pct^.fType <> REQUEST_NEWICONS) then Break;
			Integer(p) := Integer(pct) + sizeof(TSlotIPC);
			ReAllocMem(self^.protoIcons, (self^.protoIconsCount+1)*sizeof(TSlotProtoIcons));
			d := @self^.protoIcons[self^.protoIconsCount];
			CopyMemory(d,p,sizeof(TSlotProtoIcons));
			for j := 0 to 9 do
			begin
				d^.hIcons[j] := CopyIcon(p^.hIcons[j]);
			end;
			inc(self^.protoIconsCount);
			pct := pct^.Next;
		end;
	end;

    function ProcessRequest(hwnd: HWND; lParam: PEnumData): BOOL; stdcall;
    var
        pid: Integer;
        hMirandaWorkEvent: THandle;
        replyBits: Integer;
        hScreenDC: THandle;
        szBuf: array[0..MAX_PATH] of Char;
    begin
        Result := True;
        pid := 0;
        GetWindowThreadProcessId(hwnd, @pid);
        If pid <> 0 then
        begin
            // old system would get a window's pid and the module handle that created it
            // and try to OpenEvent() a event object name to it (prefixed with a string)
            // this was fine for most Oses (not the best way) but now actually compares
            // the class string (a bit slower) but should get rid of those bugs finally.
            hMirandaWorkEvent := OpenEvent(EVENT_ALL_ACCESS, False, PChar(CreateProcessUID(pid)));
            if (hMirandaWorkEvent <> 0) then
            begin
                GetClassName(hwnd, szBuf, sizeof(szBuf));
                if lstrcmp(szBuf, MIRANDANAME) <> 0 then
                begin
                    // opened but not valid.
                    CloseHandle(hMirandaWorkEvent); Exit;
                end; //if
            end; //if
            If hMirandaWorkEvent <> 0 then
            begin
                { prep the request }
                ipcPrepareRequests(IPC_PACKET_SIZE, lParam^.ipch, REQUEST_ICONS or REQUEST_GROUPS or REQUEST_CONTACTS or REQUEST_NEWICONS);
                // slots will be in the order of icon data, groups then contacts, the first
                // slot will contain the profile name
                replyBits := ipcSendRequest(hMirandaWorkEvent, lParam^.hWaitFor, lParam^.ipch, 1000);
                { replyBits will be REPLY_FAIL if the wait timed out, or it'll be the request
                bits as sent or a series of *_NOTIMPL bits where the request bit were, if there are no
				contacts to speak of, then don't bother showing this instance of Miranda }
                if (replyBits <> REPLY_FAIL) and (lParam^.ipch^.ContactsBegin <> nil) then
                begin
                    // load the address again, the server side will always overwrite it
                    lParam^.ipch^.pClientBaseAddress := lParam^.ipch;
                    // fixup all the pointers to be relative to the memory map
                    // the base pointer of the client side version of the mapped file
                    ipcFixupAddresses(False, lParam^.ipch);
                    // store the PID used to create the work event object
                    // that got replied to -- this is needed since each contact
                    // on the final menu maybe on a different instance and another OpenEvent() will be needed.
                    lParam^.pid := pid;
					// check out the user options from the server
					lParam^.bShouldOwnerDraw := (lParam^.ipch^.dwFlags and HIPC_NOICONS) = 0;
					// process the icons
					BuildSkinIcons(lParam);
                    // process other replies
                    BuildMenus(lParam);
                end;
                { close the work object }
                CloseHandle(hMirandaWorkEvent);
            end; //if
        end; //if
    end;

    function TShlComRec_QueryInterface(Self: PCommon_Interface;
        const IID: TIID; var Obj): HResult; stdcall;
    begin
        Pointer(Obj) := nil;
        { IShellExtInit is given when the TShlRec is created }
        if IsEqualIID(IID, IID_IContextMenu)
            or IsEqualIID(IID, IID_IContextMenu2)
                or IsEqualIID(IID, IID_IContextMenu3) then
        begin
            with Self^.ptrInstance^ do
            begin
                Pointer(Obj) := @ContextMenu3_Interface;
                Inc(RefCount);
            end; {with}
            Result := S_OK;
        end else begin
            // under XP, it may ask for IShellExtInit again, this fixes the -double- click to see menus issue
            // which was really just the object not being created
            if IsEqualIID(IID, IID_IShellExtInit) then
            begin
                with Self^.ptrInstance^ do
                begin
                    Pointer(Obj) := @ShellExtInit_Interface;
                    Inc(RefCount);
                end; //if
                Result := S_OK;
            end else begin
                Result := CLASS_E_CLASSNOTAVAILABLE;
            end; //if
        end; //if
    end;

    function TShlComRec_AddRef(Self: PCommon_Interface): LongInt; stdcall;
    begin
        with Self^.ptrInstance^ do
        begin
            Inc(RefCount);
            Result := RefCount;
        end; {with}
    end;

    function TShlComRec_Release(Self: PCommon_Interface): LongInt; stdcall;
    var
		j, c: Cardinal;
    begin
        with Self^.ptrInstance^ do
        begin
            Dec(RefCount);
            Result := RefCount;
            If RefCount = 0 then
            begin
                // time to go byebye.
                with Self^.ptrInstance^ do
                begin
					// free icons!
					if protoIcons <> nil then
					begin						
						c := protoIconsCount;
						while c > 0 do
						begin
							dec(c);
							for j := 0 to 9 do DestroyIcon( protoIcons[c].hIcons[j] );
						end;
						FreeMem(protoIcons); protoIcons := nil;						
					end; //if
                    // free IDataObject reference if pointer exists
                    if pDataObject <> nil then
                    begin
                        pDataObject^.ptrvTable^.Release(pDataObject);
                    end; //if
                    pDataObject := nil;
                    // free the heap and any memory allocated on it
                    HeapDestroy(hDllHeap);
					// destroy the DC
					if hMemDC <> 0 then DeleteDC(hMemDC);
                end; //with
                // free the instance (class record) created
                Dispose(Self^.ptrInstance);
                Dec(dllpublic.ObjectCount);
            end; {if}
        end; {with}
    end;

    function TShlComRec_Initialise(Self: PContextMenu3_Interface;
        pidLFolder: Pointer; DObj: PDataObject_Interface; hKeyProdID: HKEY): HResult; stdcall;
    begin
        // DObj is a pointer to an instance of IDataObject which is a pointer itself
        // it contains a pointer to a function table containing the function pointer
        // address of GetData() - the instance data has to be passed explicitly since
        // all compiler magic has gone.
        with Self^.ptrInstance^ do
        begin
            if DObj <> nil then
            begin
                Result := S_OK;
                // if an instance already exists, free it.
                if pDataObject <> nil then pDataObject^.ptrVTable^.Release(pDataObject);
                // store the new one and AddRef() it
                pDataObject := DObj;
                pDataObject^.ptrVTable^.AddRef(pDataObject);
            end else begin
                Result := E_INVALIDARG;
            end; //if
        end; //if
    end;

    function MAKE_HRESULT(Severity, Facility, Code: Integer): HResult;
    {$ifdef FPC}
    inline;
    {$endif}
    begin
        Result := (Severity shl 31) or (Facility shl 16) or Code;
    end;

    function TShlComRec_QueryContextMenu(Self: PContextMenu3_Interface;
        Menu: HMENU; indexMenu, idCmdFirst, idCmdLast, uFlags: UINT): HResult; stdcall;
    type
        TDllVersionInfo = record
            cbSize: DWORD;
            dwMajorVersion: DWORD;
            dwMinorVersion: DWORD;
            dwBuildNumber: DWORD;
            dwPlatformID: DWORD;
        end;
        TDllGetVersionProc = function(var dv: TDllVersionInfo): HResult; stdcall;
    var
        hShellInst: THandle;
        bMF_OWNERDRAW: Boolean;
        DllGetVersionProc: TDllGetVersionProc;
        dvi: TDllVersionInfo;
        ed: TEnumData;
        hMap: THandle;
        pipch: PHeaderIPC;
    begin
        Result := 0;
        if ((LOWORD(uFlags) and CMF_VERBSONLY) <> CMF_VERBSONLY) and ((LOWORD(uFlags) and CMF_DEFAULTONLY) <> CMF_DEFAULTONLY) then
        begin
            bMF_OWNERDRAW := False;
            // get the shell version
            hShellInst := LoadLibrary('shell32.dll');
            if hShellInst <> 0 then
            begin
                DllGetVersionProc := GetProcAddress(hShellInst, 'DllGetVersion');
                if @DllGetVersionProc <> nil then
                begin
                    dvi.cbSize := sizeof(TDllVersionInfo);
                    if DllGetVersionProc(dvi) >= 0 then
                    begin
                        // it's at least 4.00
                        bMF_OWNERDRAW := (dvi.dwMajorVersion > 4) or (dvi.dwMinorVersion >= 71);
                    end; //if
                end; //if
                FreeLibrary(hShellInst);
            end; //if

            hMap := CreateFileMapping(INVALID_HANDLE_VALUE, nil, PAGE_READWRITE, 0, IPC_PACKET_SIZE, IPC_PACKET_NAME);
            If (hMap <> 0) and (GetLastError <> ERROR_ALREADY_EXISTS) then
            begin
                { map the memory to this address space }
                pipch := MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
                If pipch <> nil then
                begin
                    { let the callback have instance vars }
                    ed.Self := Self^.ptrInstance;
                    // not used 'ere
                    ed.Self^.hRootMenu := Menu;
                    // store the first ID to offset with index for InvokeCommand()
                    Self^.ptrInstance^.idCmdFirst := idCmdFirst;
                    // store the starting index to offset
                    Result := idCmdFirst;
                    ed.bOwnerDrawSupported := bMF_OWNERDRAW;
					ed.bShouldOwnerDraw := True;
                    ed.idCmdFirst := idCmdFirst;
                    ed.ipch := pipch;
                    { allocate a wait object so the ST can signal us, it can't be anon
                    since it has to used by OpenEvent() }
                    strcpy(@pipch^.SignalEventName, PChar(CreateUID()));
                    { create the wait wait-for-wait object }
                    ed.hWaitFor := CreateEvent(nil, False, False, pipch^.SignalEventName);
                    If ed.hWaitFor <> 0 then
                    begin
                        { enumerate all the top level windows to find all loaded MIRANDANAME
                        classes -- }
                        EnumWindows(@ProcessRequest, lParam(@ed));
                        { close the wait-for-reply object }
                        CloseHandle(ed.hWaitFor);
                    end;
                    { unmap the memory from this address space }
                    UnmapViewOfFile(pipch);
                end; {if}
                { close the mapping }
                CloseHandle(hMap);
                // use the MSDN recommended way, thou there ain't much difference
                Result := MAKE_HRESULT(0, 0, (ed.idCmdFirst - Result) + 1);
            end else begin
                // the mapping file already exists, which is not good!
            end;
        end else begin
            // same as giving a SEVERITY_SUCCESS, FACILITY_NULL, since that
            // just clears the higher bits, which is done anyway
            Result := MAKE_HRESULT(0, 0, 1);
        end; //if
    end;

    function TShlComRec_GetCommandString(Self: PContextMenu3_Interface;
        idCmd, uType: UINT; pwReserved: PUINT; pszName: PChar; cchMax: UINT): HResult; stdcall;
    begin
        Result := E_NOTIMPL;
    end;

    function ipcGetFiles(pipch: PHeaderIPC; pDataObject: PDataObject_Interface; const hContact: THandle): Integer;
    type
        TDragQueryFile = function(hDrop: THandle; fileIndex: Integer; FileName: PChar; cbSize: Integer): Integer; stdcall;
    var
        fet: TFormatEtc;
        stgm: TStgMedium;
        pct: PSlotIPC;
        iFile: Cardinal;
        iFileMax: Cardinal;
        hShell: THandle;
        DragQueryFile: TDragQueryFile;
        cbSize: Integer;
        hDrop: THandle;
    begin
        Result := E_INVALIDARG;
        hShell := LoadLibrary('shell32.dll');
        if hShell <> 0 then
        begin
            DragQueryFile := GetProcAddress(hShell, 'DragQueryFileA');
            if @DragQueryFile <> nil then
            begin
                fet.cfFormat := CF_HDROP;
                fet.ptd := nil;
                fet.dwAspect := DVASPECT_CONTENT;
                fet.lindex := -1;
                fet.tymed := TYMED_HGLOBAL;
                Result := pDataObject^.ptrVTable^.GetData(pDataObject, fet, stgm);
                if Result = S_OK then
                begin
                    // FIX, actually lock the global object and get a pointer
                    Pointer(hDrop) := GlobalLock(stgm.hGlobal);
                    if hDrop <> 0 then
                    begin
                        // get the maximum number of files
                        iFileMax := DragQueryFile(stgm.hGlobal, $FFFFFFFF, nil, 0);
                        iFile := 0;
                        while iFile < iFileMax do
                        begin
                            // get the size of the file path
                            cbSize := DragQueryFile(stgm.hGlobal, iFile, nil, 0);
                            // get the buffer
                            pct := ipcAlloc(pipch, cbSize + 1); // including null term
                            // allocated?
                            if pct = nil then Break;
                            // store the hContact
                            pct^.hContact := hContact;
                            // copy it to the buffer
                            DragQueryFile(stgm.hGlobal, iFile, PChar(Integer(pct)+sizeof(TSlotIPC)), pct^.cbStrSection);
                            // next file
                            Inc(iFile);
                        end; //while
                        // store the number of files
                        pipch^.Slots := iFile;
                        GlobalUnlock(stgm.hGlobal);
                    end; //if hDrop check
                    // release the mediumn the lock may of failed
                    ReleaseStgMedium(stgm);
                end; //if
            end; //if
            // free the dll
            FreeLibrary(hShell);
        end; //if
    end;

    function RequestTransfer(Self: PShlComRec; idxCmd: Integer): Integer;
    var
        hMap: THandle;
        pipch: PHeaderIPC;
        mii: TMenuItemInfo;
        hTransfer: THandle;
        psd: PMenuDrawInfo;
        hReply: THandle;
        replyBits: Integer;
    begin
        // get the contact information
        mii.cbSize := sizeof(TMenuItemInfo);
        mii.fMask := MIIM_ID or MIIM_DATA;
        if GetMenuItemInfo(Self^.hRootMenu, Self^.idCmdFirst + idxCmd, False, mii) then
        begin
            // get the pointer
            Integer(psd) := mii.dwItemData;
            // the ID stored in the item pointer and the ID for the menu must match
            if (psd = nil) or (psd^.wID <> mii.wID) then
            begin
                Result := E_INVALIDARG; Exit;
            end; //if
        end else begin
            // couldn't get the info, can't start the transfer
            Result := E_INVALIDARG; Exit;
        end; //if
        // is there an IDataObject instance?
        if Self^.pDataObject <> nil then
        begin
            // OpenEvent() the work object to see if the instance is still around
            hTransfer := OpenEvent(EVENT_ALL_ACCESS, False, PChar(CreateProcessUID(psd^.pid)));
            if hTransfer <> 0 then
            begin
                // map the ipc file again
                hMap := CreateFileMapping(INVALID_HANDLE_VALUE, nil, PAGE_READWRITE, 0, IPC_PACKET_SIZE, IPC_PACKET_NAME);
                if (hMap <> 0) and (GetLastError <> ERROR_ALREADY_EXISTS) then
                begin
                    // map it to process
                    pipch := MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
                    if pipch <> nil then
                    begin
                        // create the name of the object to be signalled by the ST
                        strcpy(pipch^.SignalEventName, PChar(CreateUID()));
                        // create it
                        hReply := CreateEvent(nil, False, False, pipch^.SignalEventName);
                        if hReply <> 0 then
                        begin
                            // prepare the buffer
                            ipcPrepareRequests(IPC_PACKET_SIZE, pipch, REQUEST_XFRFILES);
                            // get all the files into the packet
                            if ipcGetFiles(pipch, Self^.pDataObject, psd^.hContact) = S_OK then
                            begin
                                // need to wait for the ST to open the mapping object
                                // since if we close it before it's opened it the data it
                                // has will be undefined
                                replyBits := ipcSendRequest(hTransfer, hReply, pipch, 200);
                                if replyBits <> REPLY_FAIL then
                                begin
                                    // they got the files!
                                    Result := S_OK;
                                end; //if
                            end;
                            // close the work object name
                            CloseHandle(hReply);
                        end; //if
                        // unmap it from this process
                        UnmapViewOfFile(pipch);
                    end; //if
                    // close the map
                    CloseHandle(hMap);
                end; //if
                // close the handle to the ST object name
                CloseHandle(hTransfer);
            end; //if
        end //if;
    end;

    function TShlComRec_InvokeCommand(Self: PContextMenu3_Interface;
        var lpici: TCMInvokeCommandInfo): HResult; stdcall;
    begin
        Result := RequestTransfer(Self^.ptrInstance, LOWORD(Integer(lpici.lpVerb)));
    end;

    function TShlComRec_HandleMenuMsgs(Self: PContextMenu3_Interface;
        uMsg: UINT; wParam: WPARAM; lParam: LPARAM; pResult: PLResult): HResult;
    const
        WM_DRAWITEM    = $002B;
        WM_MEASUREITEM = $002C;
    var
        dwi: PDrawItemStruct;
        msi: PMeasureItemStruct;
        psd: PMenuDrawInfo;
        ncm: TNonClientMetrics;
        hOldFont: THandle;
        hFont: THandle;
        tS: TSize;
        dx: Integer;
        hBr: HBRUSH;
        icorc: TRect;
		hMemDC: HDC;
    begin
        pResult^ := Integer(True);
        if (uMsg = WM_DRAWITEM) and (wParam = 0) then
        begin
            // either a main sub menu, a group menu or a contact
            dwi := PDrawItemStruct(lParam);
            Integer(psd) := dwi^.itemData;
            // don't fill
            SetBkMode(dwi^.hDC, TRANSPARENT);
            // where to draw the icon?
            icorc.Left := 0;
            // center it
            with dwi^ do
            icorc.Top := rcItem.Top + ((rcItem.Bottom - rcItem.Top) div 2) - (16 div 2);
            icorc.Right := icorc.Left + 16;
            icorc.Bottom := icorc.Top + 16;
            // draw for groups
            if (dtGroup in psd^.fTypes) or (dtEntry in psd^.fTypes) then
            begin
                hBr := GetSysColorBrush(COLOR_MENU);
                FillRect(dwi^.hDC, dwi^.rcItem, hBr);
                DeleteObject(hBr);
                //
                if (ODS_SELECTED and dwi^.itemState = ODS_SELECTED) then
                begin
                    // only do this for entry menu types otherwise a black mask
                    // is drawn under groups
                    hBr := GetSysColorBrush(COLOR_HIGHLIGHT);
                    FillRect(dwi^.hDC, dwi^.rcItem, hBr);
                    DeleteObject(hBr);
                    SetTextColor(dwi^.hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                end; //if
                // draw icon
                with dwi^, icorc do
                begin
					if (ODS_SELECTED and dwi^.itemState) = ODS_SELECTED then
					begin
						hBr := GetSysColorBrush(COLOR_HIGHLIGHT); 
					end else begin
						hBr := GetSysColorBrush(COLOR_MENU);
					end; //if
					DrawIconEx(hDC, Left+2, Top, psd^.hStatusIcon, 
						16, 16, // width, height
						0, 	  // step
						hBr,  // brush
						DI_NORMAL);
					DeleteObject(hBr);
                end; //with
                // draw the text
                with dwi^ do
                begin
                    Inc(rcItem.Left, ((rcItem.Bottom-rcItem.Top)-2));
                    DrawText(hDC, psd^.szText, psd^.cch, rcItem, DT_NOCLIP or DT_NOPREFIX or DT_SINGLELINE or DT_VCENTER);
                    // draw the name of the database text if it's there
                    if psd^.szProfile <> nil then
                    begin
                        GetTextExtentPoint32(dwi^.hDC, psd^.szText, psd^.cch, tS);
                        Inc(rcItem.Left, tS.cx+8);
                        SetTextColor(hDC, GetSysColor(COLOR_GRAYTEXT));
                        DrawText(hDC, psd^.szProfile, lstrlen(psd^.szProfile), rcItem, DT_NOCLIP or DT_NOPREFIX or DT_SINGLELINE or DT_VCENTER);
                    end; //if
                end; //with
            end else
            begin
                // it's a contact!
                hBr := GetSysColorBrush(COLOR_MENU);
                FillRect(dwi^.hDC, dwi^.rcItem, hBr);
                DeleteObject(hBr);
                if ODS_SELECTED and dwi^.itemState = ODS_SELECTED then
                begin
                    hBr := GetSysColorBrush(COLOR_HIGHLIGHT);
                    FillRect(dwi^.hDC, dwi^.rcItem, hBr);
                    DeleteObject(hBr);
                    SetTextColor(dwi^.hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                end;
                // draw icon
                with dwi^, icorc do
                begin
					if (ODS_SELECTED and dwi^.itemState) = ODS_SELECTED then
					begin
						hBr := GetSysColorBrush(COLOR_HIGHLIGHT); 
					end else begin
						hBr := GetSysColorBrush(COLOR_MENU);
					end; //if
					DrawIconEx(hDC, Left+2, Top, psd^.hStatusIcon, 
						16, 16, // width, height
						0, 	  // step
						hBr,  // brush
						DI_NORMAL);
					DeleteObject(hBr);
                end; //with
                // draw the text
                with dwi^ do
                begin
                    Inc(rcItem.Left, (rcItem.Bottom-rcItem.Top) + 1);
                    DrawText(hDC, psd^.szText, psd^.cch, rcItem, DT_NOCLIP or DT_NOPREFIX or DT_SINGLELINE or DT_VCENTER);
                end; //with
            end; //if
        end
        else if (uMsg = WM_MEASUREITEM) then
        begin
            // don't check if it's really a menu
            msi := PMeasureItemStruct(lParam);
            Integer(psd) := msi^.itemData;
            ncm.cbSize := sizeof(TNonClientMetrics);
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, @ncm, 0);
            // create the font used in menus, this font should be cached somewhere really
            {$IFDEF FPC}
            hFont := CreateFontIndirect(@ncm.lfMenuFont);
            {$ELSE}
            hFont := CreateFontIndirect(ncm.lfMenuFont);
            {$ENDIF}
			hMemDC := Self^.ptrInstance^.hMemDC;
			// select in the font
            hOldFont := SelectObject(hMemDC, hFont);
            // default to an icon
            dx := 16;
            // get the size 'n' account for the icon
            GetTextExtentPoint32(hMemDC, psd^.szText, psd^.cch, tS);
            Inc(dx, tS.cx);
            // main menu item?
            if psd^.szProfile <> nil then
            begin
                GetTextExtentPoint32(hMemDC, psd^.szProfile, lstrlen(psd^.szProfile), tS);
                Inc(dx, tS.cx);
            end;
            // store it
            msi^.itemWidth := dx + Integer(ncm.iMenuWidth);
            msi^.itemHeight := Integer(ncm.iMenuHeight)+2;
            if tS.cy > msi^.itemHeight then Inc(msi^.itemHeight, tS.cy - msi^.itemHeight);
            // clean up
            SelectObject(hMemDC, hOldFont);
            DeleteObject(hFont);
        end;
        Result := S_OK;
    end;

    function TShlComRec_HandleMenuMsg(Self: PContextMenu3_Interface;
        uMsg: UINT; wParam: WPARAM; lParam: LPARAM): HResult; stdcall;
    var
        Dummy: HResult;
    begin
        Result := TShlComRec_HandleMenuMsgs(Self, uMsg, wParam, lParam, @Dummy);
    end;

    function TShlComRec_HandleMenuMsg2(Self: PContextMenu3_Interface;
        uMsg: UINT; wParam: WPARAM; lParam: LPARAM; plResult: Pointer{^LResult}): HResult; stdcall;
    var
        Dummy: HResult;
    begin
        // this will be null if a return value isn't needed.
        if plResult = nil then plResult := @Dummy;
        Result := TShlComRec_HandleMenuMsgs(Self, uMsg, wParam, lParam, plResult);
    end;

    function TShlComRec_Create: PShlComRec;
	var
		DC: HDC;
    begin
        New(Result);
        { build all the function tables for interfaces }
        with Result^.ShellExtInit_Interface do
        begin
            { this is only owned by us... }
            ptrVTable := @vTable;
            { IUnknown }
            vTable.QueryInterface := @TShlComRec_QueryInterface;
            vTable.AddRef := @TShlComRec_AddRef;
            vTable.Release := @TShlComRec_Release;
            { IShellExtInit }
            vTable.Initialise := @TShlComRec_Initialise;
            { instance of a TShlComRec }
            ptrInstance := Result;
        end;
        with Result^.ContextMenu3_Interface do
        begin
            ptrVTable := @vTable;
            { IUnknown }
            vTable.QueryInterface := @TShlComRec_QueryInterface;
            vTable.AddRef := @TShlComRec_AddRef;
            vTable.Release := @TShlComRec_Release;
            { IContextMenu }
            vTable.QueryContextMenu := @TShlComRec_QueryContextMenu;
            vTable.InvokeCommand := @TShlComRec_InvokeCommand;
            vTable.GetCommandString := @TShlComRec_GetCommandString;
            { IContextMenu2 }
            vTable.HandleMenuMsg := @TShlComRec_HandleMenuMsg;
            { IContextMenu3 }
            vTable.HandleMenuMsg2 := @TShlComRec_HandleMenuMsg2;
            { instance data }
            ptrInstance := Result;
        end;
        { initalise variables }
        Result^.RefCount := 1;
        Result^.hDllHeap := HeapCreate(0, 0, 0);
        Result^.hRootMenu := 0;
        Result^.idCmdFirst := 0;
        Result^.pDataObject := nil;
		Result^.ProtoIcons := nil;
		Result^.ProtoIconsCount := 0;
		// create an inmemory DC
		DC := GetDC(0);
		Result^.hMemDC := CreateCompatibleDC(DC);
		ReleaseDC(0,DC);
        { keep count on the number of objects }
        Inc(dllpublic.ObjectCount);
    end;

{ IClassFactory }

type

    PVTable_IClassFactory = ^TVTable_IClassFactory;
    TVTable_IClassFactory = record
        { IUnknown }
        QueryInterface: Pointer;
        AddRef: Pointer;
        Release: Pointer;
        { IClassFactory }
        CreateInstance: Pointer;
        LockServer: Pointer;
    end;
    PClassFactoryRec = ^TClassFactoryRec;
    TClassFactoryRec = record
        ptrVTable: PVTable_IClassFactory;
        vTable: TVTable_IClassFactory;
    {fields}
        RefCount: LongInt;
    end;

    function TClassFactoryRec_QueryInterface(Self: PClassFactoryRec;
        const IID: TIID; var Obj): HResult; stdcall;
    begin
        Pointer(Obj) := nil;
        Result := E_NOTIMPL;
    end;

    function TClassFactoryRec_AddRef(Self: PClassFactoryRec): LongInt; stdcall;
    begin
        Inc(Self^.RefCount);
        Result := Self^.RefCount;
    end;

    function TClassFactoryRec_Release(Self: PClassFactoryRec): LongInt; stdcall;
    begin
        Dec(Self^.RefCount);
        Result := Self^.RefCount;
        if Result = 0 then
        begin
            Dispose(Self);
            Dec(dllpublic.FactoryCount);
        end; {if}
    end;

    function TClassFactoryRec_CreateInstance(Self: PClassFactoryRec;
        unkOuter: Pointer; const IID: TIID; var Obj): HResult; stdcall;
    var
        ShlComRec: PShlComRec;
    begin
        Pointer(Obj) := nil;
        Result := CLASS_E_NOAGGREGATION;
        if unkOuter = nil then
        begin
            if IsEqualIID(IID, IID_IShellExtInit) then
            begin
                Result := S_OK;
                ShlComRec := TShlComRec_Create;
                Pointer(Obj) := @ShlComRec^.ShellExtInit_Interface;
            end; //if
        end; //if
    end;
    function TClassFactoryRec_LockServer(Self: PClassFactoryRec; fLock: BOOL): HResult; stdcall;
    begin
        Result := E_NOTIMPL;
    end;

    function TClassFactoryRec_Create: PClassFactoryRec;
    begin
        New(Result);
        Result^.ptrVTable := @Result^.vTable;
        { IUnknown }
        Result^.vTable.QueryInterface := @TClassFactoryRec_QueryInterface;
        Result^.vTable.AddRef := @TClassFactoryRec_AddRef;
        Result^.vTable.Release := @TClassFactoryRec_Release;
        { IClassFactory }
        Result^.vTable.CreateInstance := @TClassFactoryRec_CreateInstance;
        Result^.vTable.LockServer := @TClassFactoryRec_LockServer;
        { inital the variables }
        Result^.RefCount := 1;
        { count the number of factories }
        Inc(dllpublic.FactoryCount);
    end;
	
	//
	// IPC part
	// 

				
	type 
	PFileList = ^TFileList;
	TFileList = array[0..0] of PChar;
	PAddArgList = ^TAddArgList;
	TAddArgList = record
		szFile: PChar;		// file being processed
		cch: Cardinal;		// it's length (with space for NULL char)
		count: Cardinal;	// number we have so far
		files: PFileList;
		hContact: THandle;
		hEvent: THandle;
	end;
				
	function AddToList(var args: TAddArgList): LongBool;
	var
		attr: Cardinal;
		p: Pointer;
		hFind: THandle;
		fd: TWIN32FINDDATA;
		szBuf: array[0..MAX_PATH] of Char;
		szThis: PChar;
		cchThis: Cardinal;
	begin
		Result := False;
		attr := GetFileAttributes(args.szFile);
		if (attr <> $FFFFFFFF) and ((attr and FILE_ATTRIBUTE_HIDDEN) = 0) then
		begin
			if args.count mod 10 = 5 then
			begin
				if CallService(MS_SYSTEM_TERMINATED,0,0) <> 0 then
				begin
					Result := True; Exit;
				end; //if
			end;
			if attr and FILE_ATTRIBUTE_DIRECTORY <> 0 then
			begin	
				// add the directory
				strcpy(szBuf,args.szFile);
				ReallocMem(args.files,(args.count+1)*sizeof(PChar));
				GetMem(p,strlen(szBuf)+1);
				strcpy(p,szBuf);
				args.files^[args.count] := p;
				inc(args.count);
				// tack on ending search token
				strcat(szBuf,'\*');					
				hFind := FindFirstFile(szBuf,fd);
				while True do
				begin
					if fd.cFileName[0] <> '.' then
					begin
						strcpy(szBuf,args.szFile);
						strcat(szBuf,'\');
						strcat(szBuf,fd.cFileName);							
						// keep a copy of the current thing being processed
						szThis := args.szFile; args.szFile := szBuf;
						cchThis := args.cch; args.cch := strlen(szBuf)+1;
						// recurse
						Result := AddToList(args);
						// restore
						args.szFile := szThis;
						args.cch := cchThis;
						if Result then Break;
					end; //if
					if not FindNextFile(hFind,fd) then Break;
				end; //while
				FindClose(hFind);
			end else begin
				// add the file
				ReallocMem(args.files,(args.count+1)*sizeof(PChar));
				GetMem(p,args.cch);
				strcpy(p,args.szFile);
				args.files^[args.count] := p;
				inc (args.count);
			end; //if
		end;
	end;
			
	procedure MainThreadIssueTransfer(p: PAddArgList); stdcall;
	begin
		CallService(MS_FILE_SENDSPECIFICFILES,p^.hContact,lParam(p^.files));
		SetEvent(p^.hEvent);
	end;
				
    function IssueTransferThread(pipch: PHeaderIPC): Cardinal; stdcall;
	var
		szBuf: array[0..MAX_PATH] of Char;
		pct: PSlotIPC;
		args: TAddArgList;
		bQuit: LongBool;
		j,c: Cardinal;
		p: Pointer;
		hMainThread: THandle;
	begin
		CallService(MS_SYSTEM_THREAD_PUSH,0,0);
		hMainThread := THandle(pipch^.Param);
		GetCurrentDirectory(sizeof(szBuf),szBuf);
		args.count := 0;
		args.files := nil;		        
        pct := pipch^.DataPtr;
		bQuit := False;
        while pct <> nil do
        begin
            if (pct^.cbSize <> sizeof(TSlotIPC)) then break;
            args.szFile := PChar(Integer(pct) + sizeof(TSlotIPC));
			args.hContact := pct^.hContact;
			args.cch := pct^.cbStrSection+1;
			bQuit := AddToList(args);
			if bQuit then Break;
            pct := pct^.next;
        end; //while
		if args.files <> nil then
		begin
			ReallocMem(args.files,(args.count+1)*sizeof(PChar));
			args.files^[args.count] := nil;
			inc (args.count);
			if (not bQuit) then
			begin
				args.hEvent := CreateEvent(nil, True, False, nil);
				QueueUserAPC(@MainThreadIssueTransfer,hMainThread,DWORD(@args));
				while True do				
				begin
					if WaitForSingleObjectEx(args.hEvent,INFINITE,True) <> WAIT_IO_COMPLETION then Break;
				end;			
				CloseHandle(args.hEvent);
			end; //if
			c := args.count-1;
			for j := 0 to c do
			begin
				p := args.files^[j];
				if p <> nil then FreeMem(p);
			end;
			FreeMem(args.files);
		end;
        SetCurrentDirectory(szBuf);
		FreeMem(pipch);
		CloseHandle(hMainThread);
		CallService(MS_SYSTEM_THREAD_POP,0,0);
		ExitThread(0);
    end;

type

    PSlotInfo = ^TSlotInfo;
    TSlotInfo = record
        hContact: THandle;
		hProto: Cardinal;
        dwStatus: Integer;         // will be aligned anyway
    end;
    TSlotArray = array[0..$FFFFFF] of TSlotInfo;
    PSlotArray = ^TSlotArray;

    function SortContact(var Item1, Item2: TSlotInfo): Integer; stdcall;
    begin
        Result := PluginLink^.CallService(MS_CLIST_CONTACTSCOMPARE, item1.hContact, item2.hContact);
    end;

    // from FP FCL

    procedure QuickSort (FList: PSlotArray; L, R: LongInt);
    var
        I, J: LongInt;
        P, Q: TSlotInfo;
    begin
        repeat
            I := L;
            J := R;
            P := FList^[ (L+R) div 2 ];
            repeat
                while SortContact(P, FList^[i]) > 0 do Inc(i);
                while SortContact(P, FList^[j]) < 0 do Dec(j);
                if I <= J then
                begin
                    Q := FList^[I];
                    FList^[I] := FList^[J];
                    FList^[J] := Q;
                    Inc(I);
                    Dec(J);
                end; //if
            until I > J;
            if L < J then QuickSort (FList, L, J);
            L := I;
        until I >= R;
    end;

    {$define SHL_KEYS}
    {$include shlc.inc}
    {$undef SHL_KEYS}

	function ipcGetSkinIcons(ipch: PHeaderIPC);
	var
		protoCount: Integer;
		pp: ^PPROTOCOLDESCRIPTOR;
		spi: TSlotProtoIcons;
		j: Cardinal;
		pct: PSlotIPC;
		szTmp: array[0..63] of Char;
		dwCaps: Cardinal;
	begin
		if (CallService(MS_PROTO_ENUMPROTOCOLS,wParam(@protoCount),lParam(@pp)) = 0) and (protoCount <> 0) then
		begin
			spi.pid := GetCurrentProcessId();
			while protoCount > 0 do
			begin
				if (pp^.type_ = PROTOTYPE_PROTOCOL) then
				begin
					strcpy(szTmp,pp^.szName);
					strcat(szTmp,PS_GETCAPS);
					dwCaps := CallService(szTmp,PFLAGNUM_1,0);
					if (dwCaps and PF1_FILESEND) <> 0 then
					begin
						pct := ipcAlloc(ipch,sizeof(TSlotProtoIcons));
						if pct <> nil then
						begin
							// capture all the icons!
							spi.hProto := StrHash(pp^.szName);
							for j := 0 to 9 do
							begin
								spi.hIcons[j] := LoadSkinnedProtoIcon(pp^.szName,ID_STATUS_OFFLINE+j);
							end; //for
							pct^.fType := REQUEST_NEWICONS;
							CopyMemory(Pointer(Integer(pct)+sizeof(TSlotIPC)),@spi,sizeof(TSlotProtoIcons));
							if ipch^.NewIconsBegin = nil then ipch^.NewIconsBegin := pct;
						end; //if
					end; //if
				end; //if
				inc (pp);
				dec (protoCount);
			end; //while
		end; //if
		// add Miranda icon
		pct := ipcAlloc(ipch, sizeof(TSlotProtoIcons));
		if pct <> nil then
		begin
			ZeroMemory(@spi.hIcons, sizeof(spi.hIcons));
			spi.hProto := 0; // no protocol
			spi.hIcons[0] := LoadSkinnedIcon(SKINICON_OTHER_MIRANDA);
			pct^.fType := REQUEST_NEWICONS;
			CopyMemory(Pointer(Integer(pct)+sizeof(TSlotIPC)),@spi,sizeof(TSlotProtoIcons));
			if ipch^.NewIconsBegin = nil then ipch^.NewIconsBegin := pct;
		end; //if
	end;

    function ipcGetSortedContacts(ipch: PHeaderIPC; pSlot: pint; bGroupMode: Boolean): Boolean;
    var
        dwContacts: Cardinal;
        pContacts: PSlotArray;
        hContact: THandle;
        I: Integer;
        dwOnline: Cardinal;
        szProto: PChar;
        dwStatus: Integer;
        pct: PSlotIPC;
        szContact: PChar;
        dbv: TDBVariant;
        bHideOffline: Boolean;
		szTmp: array[0..63] of Char;
		dwCaps: Cardinal;
        szSlot: PChar;
        n, rc, cch: Cardinal;
    begin
        Result := False;
        // hide offliners?
        bHideOffline := DBGetContactSettingByte(0, 'CList', 'HideOffline', 0) = 1;
		// do they wanna hide the offline people anyway?
		if DBGetContactSettingByte(0, SHLExt_Name, SHLExt_ShowNoOffline, 0) = 1 then
		begin
			// hide offline people
			bHideOffline := True;
		end;
        // get the number of contacts
        dwContacts := PluginLink^.CallService(MS_DB_CONTACT_GETCOUNT, 0, 0);
        if dwContacts = 0 then Exit;
        // get the contacts in the array to be sorted by status, trim out anyone
        // who doesn't wanna be seen.
        GetMem(pContacts, (dwContacts+2) * sizeof(TSlotInfo));
        i := 0;
        dwOnline := 0;
        hContact := PluginLink^.CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
        while (hContact <> 0) do
        begin
            if i >= dwContacts then Break;
			(* do they have a running protocol? *)
            Integer(szProto) := PluginLink^.CallService(MS_PROTO_GETCONTACTBASEPROTO, hContact, 0);
            if szProto <> nil then
            begin
				(* does it support file sends? *)
				strcpy(szTmp, szProto);
				strcat(szTmp, PS_GETCAPS);				
				dwCaps := CallService(szTmp,PFLAGNUM_1,0);
				if (dwCaps and PF1_FILESEND) = 0 then
				begin
					hContact := CallService(MS_DB_CONTACT_FINDNEXT,hContact,0);
					continue;
				end;
                dwStatus := DBGetContactSettingWord(hContact, szProto, 'Status', ID_STATUS_OFFLINE);
                if dwStatus <> ID_STATUS_OFFLINE then Inc(dwOnline)
                else if bHideOffline then
                begin
                    hContact := PluginLink^.CallService(MS_DB_CONTACT_FINDNEXT, hContact, 0);
                    continue;
                end; //if
                // is HIT on?
                if BST_UNCHECKED = DBGetContactSettingByte(0, SHLExt_Name, SHLExt_UseHITContacts, BST_UNCHECKED) then
                begin
                    // don't show people who are "Hidden" "NotOnList" or Ignored
                    if (DBGetContactSettingByte(hContact, 'CList', 'Hidden', 0) = 1)
                        or (DBGetContactSettingByte(hContact, 'CList', 'NotOnList', 0) = 1)
                        or (PluginLink^.CallService(MS_IGNORE_ISIGNORED, hContact, IGNOREEVENT_MESSAGE or IGNOREEVENT_URL or IGNOREEVENT_FILE) <> 0) then
                    begin
                        hContact := PluginLink^.CallService(MS_DB_CONTACT_FINDNEXT, hContact, 0);
                        continue;
                    end; //if
                end; //if
                // is HIT2 off?
                if BST_UNCHECKED = DBGetContactSettingByte(0, SHLExt_Name, SHLExt_UseHIT2Contacts, BST_UNCHECKED) then
                begin
                    if DBGetContactSettingWord(hContact, szProto, 'ApparentMode', 0) = ID_STATUS_OFFLINE then
                    begin
                        hContact := PluginLink^.CallService(MS_DB_CONTACT_FINDNEXT, hContact, 0);
                        continue;
                    end; //if
                end; //if
				// store
				pContacts^[i].hContact := hContact;
				pContacts^[i].dwStatus := dwStatus;
				pContacts^[i].hProto := StrHash(szProto);
				Inc(i);					
            end else begin
				// contact has no protocol!
            end; //if
            hContact := PluginLink^.CallService(MS_DB_CONTACT_FINDNEXT, hContact, 0);
        end; //while
        // if no one is online and the CList isn't showing offliners, quit
        if (dwOnline = 0) and (bHideOffline) then
        begin
            FreeMem(pContacts); Exit;
        end; //if
        dwContacts := i; i := 0;
        // sort the array
        QuickSort(pContacts, 0, dwContacts-1);
        // create an IPC slot for each contact and store display name, etc
        while i < dwContacts do
        begin
            Integer(szContact) := PluginLink^.CallService(MS_CLIST_GETCONTACTDISPLAYNAME, pContacts^[i].hContact, 0);
            if (szContact <> nil) then
            begin   
                n := 0;
                rc := 1;
                if bGroupMode then
                begin
                    rc := DBGetContactSetting(pContacts^[i].hContact,'CList','Group',@dbv);
                    if rc = 0 then
                    begin
                        n := lstrlen(dbv.pszVal)+1;
                    end;
                end; //if
                cch := lstrlen(szContact)+1;
                pct := ipcAlloc(ipch, cch + 1 + n);
                if pct = nil then
                begin
                    DBFreeVariant(@dbv);
                    break;
                end;
                // lie about the actual size of the TSlotIPC
                pct^.cbStrSection := cch;
                szSlot := PChar(Integer(pct) + sizeof(TSlotIPC));
                strcpy(szSlot, szContact);                
                pct^.fType := REQUEST_CONTACTS;
                pct^.hContact := pContacts^[i].hContact;
                pct^.Status := pContacts^[i].dwStatus;
				pct^.hProto := pContacts^[i].hProto;                
                if ipch^.ContactsBegin = nil then ipch^.ContactsBegin := pct;
                inc(szSlot,cch+1);
                if rc=0 then
                begin
                    pct^.hGroup := StrHash(dbv.pszVal);
                    strcpy(szSlot,dbv.pszVal);
                    DBFreeVariant(@dbv);
                end else begin
                    pct^.hGroup := 0;
                    szSlot^ := #0;
                end;
                inc(pSlot^);
            end; //if
            Inc(i);
        end; //while
        FreeMem(pContacts);
        //
        Result := True;
    end;
	
	procedure ipcService(dwParam: DWORD); stdcall;
    label
        Reply;
    var
        hMap: THandle;
        pMMT: PHeaderIPC;
        hSignal: THandle;
        pct: PSlotIPC;
        hContact: THandle;
        szContact: PChar;
        Status: int;
        szBuf: PChar;
        iSlot: Integer;
        szGroupStr: array[0..31] of Char;
        dbv: TDBVARIANT;
        bits: pint;
        hIcon: THandle;
        I: Integer;
        bGroupMode: Boolean;
		tid: Cardinal;
		cloned: PHeaderIPC;
		szMiranda: PChar;
    begin		
        { try to open the file mapping object the caller must make sure no other
        running instance is using this file }
        hMap := OpenFileMapping(FILE_MAP_ALL_ACCESS, False, IPC_PACKET_NAME);
        If hMap <> 0 then
        begin
            { map the file to this process }
            pMMT := MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
            { if it fails the caller should of had some timeout in wait }
            if (pMMT <> nil) and (pMMT^.cbSize = sizeof(THeaderIPC)) and (pMMT^.dwVersion = PLUGIN_MAKE_VERSION(1,0,6,6)) then
            begin
				// toggle the right bits
				bits := @pMMT^.fRequests;	
				// jump right to a worker thread for file processing?
				if (bits^ and REQUEST_XFRFILES) = REQUEST_XFRFILES then
				begin
					GetMem(cloned, IPC_PACKET_SIZE);
					// translate from client space to cloned heap memory
					pMMT^.pServerBaseAddress := pMMT^.pClientBaseAddress;
					pMMT^.pClientBaseAddress := cloned;
					CopyMemory(cloned, pMMT, IPC_PACKET_SIZE);
					ipcFixupAddresses(True, cloned);					
					DuplicateHandle(GetCurrentProcess(),GetCurrentThread(),GetCurrentProcess(),@cloned^.Param,THREAD_SET_CONTEXT,FALSE,0);
					CloseHandle(CreateThread(nil,0,@IssueTransferThread,cloned,0,tid));
					goto Reply;
				end;
                // the IPC header may have pointers that need to be translated
                // in either case the supplied data area pointers has to be
                // translated to this address space.
                // the server base address is always removed to get an offset
                // to which the client base is added, this is what ipcFixupAddresses() does
                pMMT^.pServerBaseAddress := pMMT^.pClientBaseAddress;
                pMMT^.pClientBaseAddress := pMMT;
                // translate to the server space map
                ipcFixupAddresses(True, pMMT);
                // store the address map offset so the caller can retranslate
                pMMT^.pServerBaseAddress := pMMT;
				// return some options to the client 
				if DBGetContactSettingByte(0,SHLExt_Name,SHLExt_ShowNoIcons,0) <> 0 then 
				begin
					pMMT^.dwFlags := HIPC_NOICONS;
				end;
				// see if we have a custom string for 'Miranda'
				szMiranda := Translate('Miranda');
				lstrcpyn(pMMT^.MirandaName,szMiranda,sizeof(pMMT^.MirandaName)-1);
                // if the group mode is on, check if they want the CList setting
                bGroupMode := BST_CHECKED = DBGetContactSettingByte(0, SHLExt_Name, SHLExt_UseGroups, BST_UNCHECKED);
                if bGroupMode and (BST_CHECKED = DBGetContactSettingByte(0, SHLExt_Name, SHLExt_UseCListSetting, BST_UNCHECKED)) then
                begin
                    bGroupMode := 1 = DBGetContactSettingByte(0, 'CList', 'UseGroups', 0);
                end;
                iSlot := 0;
                // return profile if set
                if BST_UNCHECKED = DBGetContactSettingByte(0, SHLExt_Name, SHLExt_ShowNoProfile, BST_UNCHECKED) then
                begin
                    pct := ipcAlloc(pMMT, 50);
                    if pct <> nil then
                    begin
                        // will actually return with .dat if there's space for it, not what the docs say
                        pct^.Status := STATUS_PROFILENAME;
                        PluginLink^.CallService(MS_DB_GETPROFILENAME, 49, Integer(pct)+sizeof(TSlotIPC));
                    end; //if
                end; //if
				if (bits^ and REQUEST_NEWICONS) = REQUEST_NEWICONS then
				begin
					ipcGetSkinIcons(pMMT);
				end;
                if (bits^ and REQUEST_GROUPS = REQUEST_GROUPS) then
                begin
                    // return contact's grouping if it's present
                    while bGroupMode do
                    begin
                        Str(iSlot, szGroupStr);
                        if DBGetContactSetting(0, 'CListGroups', szGroupStr, @dbv) <> 0 then Break;
                        pct := ipcAlloc(pMMT, lstrlen(dbv.pszVal+1)+1); // first byte has flags, need null term
                        if pct <> nil then
                        begin
                            if pMMT^.GroupsBegin = nil then pMMT^.GroupsBegin := pct;
                            pct^.fType := REQUEST_GROUPS;
                            pct^.hContact := 0;
                            Integer (szBuf) := Integer(pct) + sizeof(TSlotIPC); // get the end of the slot
                            strcpy(szBuf, dbv.pszVal+1);
                            pct^.hGroup := 0;
                            DBFreeVariant(@dbv); // free the string
                        end else begin
                            // outta space
                            DBFreeVariant(@dbv);
                            Break;
                        end; //if
                        Inc(iSlot);
                    end; {while}
                    // if there was no space left, it'll end on null
                    if pct = nil then bits^ := (bits^ or GROUPS_NOTIMPL) and not REQUEST_GROUPS;
                end; {if: group request}
                // SHOULD check slot space.
                if (bits^ and REQUEST_CONTACTS = REQUEST_CONTACTS) then
                begin
                    if not ipcGetSortedContacts(pMMT, @iSlot, bGroupMode) then
                    begin
                        // fail if there were no contacts AT ALL
                        bits^ := (bits^ or CONTACTS_NOTIMPL) and not REQUEST_CONTACTS;
                    end; //if
                end; // if:contact request
                // store the number of slots allocated
                pMMT^.Slots := iSlot;
        Reply:
                { get the handle the caller wants to be signalled on }
                hSignal := OpenEvent(EVENT_ALL_ACCESS, False, pMMT^.SignalEventName);
                { did it open? }
                If hSignal <> 0 then
                begin
                    { signal and close }
                    SetEvent(hSignal); CloseHandle(hSignal);
                end;
                { unmap the shared memory from this process }
                UnmapViewOfFile(pMMT);
            end;
            { close the map file }
            CloseHandle(hMap);
        end; {if}
        //
    end;

    function ThreadServer(hMainThread: Pointer): Cardinal;
    {$ifdef FPC}
    stdcall;
    {$endif}
	var
		hEvent: THandle;
    begin
		CallService(MS_SYSTEM_THREAD_PUSH,0,0);
        hEvent := CreateEvent(nil, False, False, PChar(CreateProcessUID(GetCurrentProcessId())));        
        while True do
        begin
            Result := WaitForSingleObjectEx(hEvent,INFINITE,True);
            if Result = WAIT_OBJECT_0 then
            begin				
				QueueUserAPC(@ipcService,THandle(hMainThread),0);
            end; //if
            if CallService(MS_SYSTEM_TERMINATED,0,0)=1 then Break;
        end; //while
        CloseHandle(hEvent);
		CloseHandle(THandle(hMainThread));
        CallService(MS_SYSTEM_THREAD_POP,0,0);
		ExitThread(0);
    end;

    procedure InvokeThreadServer;
    var
        {$ifdef FPC}
        TID: DWORD;
        {$else}
        TID: Cardinal;
        {$endif}
	var
		hMainThread: THandle;
    begin
		hMainThread := 0;
		DuplicateHandle(GetCurrentProcess(),GetCurrentThread(),GetCurrentProcess(),@hMainThread,THREAD_SET_CONTEXT,FALSE,0);
        if hMainThread <> 0 then
        begin
            {$ifdef FPC}
            CloseHandle(CreateThread(nil,0,@ThreadServer,Pointer(hMainThread),0,TID));
            {$else}
            CloseHandle(BeginThread(nil,0,@ThreadServer,Pointer(hMainThread),0,TID));
            {$endif}
        end; //if
    end;

{ exported functions }

    function DllGetClassObject(const CLSID: TCLSID; const IID: TIID; var Obj): HResult; stdcall;
    begin
        Pointer(Obj) := nil;
        Result := CLASS_E_CLASSNOTAVAILABLE;
        if (IsEqualCLSID(CLSID,CLSID_ISHLCOM))
            and (IsEqualIID(IID,IID_IClassFactory))
                and (FindWindow(MIRANDANAME,nil) <> 0) then
        begin
            Pointer(Obj) := TClassFactoryRec_Create;
            Result := S_OK;
        end; //if
    end;

    function DllCanUnloadNow: HResult;
    begin
        if ((dllpublic.FactoryCount = 0) and (dllpublic.ObjectCount = 0)) then
        begin
            Result := S_OK;
        end else begin
            Result := S_FALSE;
        end; //if
    end;

initialization
begin
    FillChar(dllpublic, sizeof(dllpublic), 0);
    IsMultiThread := True;
end;

end.

