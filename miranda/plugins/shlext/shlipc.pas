unit shlIPC;

interface

uses

    m_globaldefs, Windows;

const

    REPLY_FAIL       = $88888888;
    REPLY_OK         = $00000000;

    REQUEST_ICONS    = 1;
    REQUEST_GROUPS   = (REQUEST_ICONS) shl 1;
    REQUEST_CONTACTS = (REQUEST_GROUPS) shl 1;
    REQUEST_XFRFILES = (REQUEST_CONTACTS) shl 1;
	REQUEST_NEWICONS = (REQUEST_XFRFILES) shl 1;

    ICONS_NOTIMPL    = $00000008;
    GROUPS_NOTIMPL   = $00000080;
    CONTACTS_NOTIMPL = $00000800;

    STATUS_PROFILENAME      = 2;

    // there maybe more than one reason why any request type wasn't returned

type

    { this can be a group entry, if it is, hContact = <index>
    the string contains the full group path }

    PSlotIPC = ^TSlotIPC;
    TSlotIPC = packed record
        cbSize: Byte;
        fType: int;  // a REQUEST_* type
        Next: PSlotIPC;
        hContact: THandle;
		hProto: Cardinal; // hash of the protocol the user is on
        hGroup: Cardinal; // hash of the entire path (not defined for REQUEST_GROUPS slots)
        Status: Word; // only used for contacts -- can be STATUS_PROFILENAME -- but that is because returning the profile name is optional
        cbStrSection: int;
    end;

    // if the slot contains a nickname, after the NULL, there is another NULL or a group path string

	PSlotProtoIcons = ^TSlotProtoIcons;
	TSlotProtoIcons = packed record
		pid: Cardinal;		// pid of Miranda this protocol was on
		hProto: Cardinal;	// hash of the protocol
		hIcons: array[0..9] of HICON;
	end;
	TSlotProtoIconsArray=array[0..0] of TSlotProtoIcons;
    // the process space the thread is running in WILL use a different mapping
    // address than the client's process space, addresses need to be adjusted
    // to the client's process space.. this is done by the following means :

    //
    // new_addr := (old_address - serverbase) + client base
    //
    // this isn't the best of solutions, the link list should be a variant array
    // without random access, which would mean each element's different
    // size would need to be computed each time it is accessed or read past

    PHeaderIPC = ^THeaderIPC;
    THeaderIPC = record
        cbSize: Cardinal;
        dwVersion: Cardinal;		
        pServerBaseAddress: Pointer;
        pClientBaseAddress: Pointer;
        fRequests: Cardinal;		
		dwFlags: Cardinal;
        Slots: Cardinal;		
		Param: Cardinal;
        SignalEventName: array[0..63] of Char;
		MirandaName: array[0..63] of Char;
        IconsBegin: PSlotIPC;
        ContactsBegin: PSlotIPC;
        GroupsBegin: PSlotIPC;
		NewIconsBegin: PSlotIPC;
        // start of an flat memory stack, which is referenced as a linked list
        DataSize: int;
        DataPtr: PSlotIPC;
        DataPtrEnd: PSlotIPC;
        DataFramePtr: Pointer;
    end;
	
	const HIPC_NOICONS = 1;

    procedure ipcPrepareRequests(ipcPacketSize: int; pipch: PHeaderIPC; fRequests: Cardinal);
    function ipcSendRequest(hSignal, hWaitFor: THandle; pipch: PHeaderIPC; dwTimeoutMsecs: DWORD): Cardinal;
    function ipcAlloc(pipch: PHeaderIPC; nSize: Integer): PSlotIPC;
    procedure ipcFixupAddresses(FromServer: LongBool; pipch: PHeaderIPC);

type

    TStrTokRec = record
        szStr: PChar;
        szSet: set of Char;
        // need a delimiter after the token too?, e.g. FOO^BAR^ if FOO^BAR
        // is the string then only FOO^ is returned, could cause infinite loops
        // if the condition isn't accounted for thou.
        bSetTerminator: Boolean;
    end;

    function StrTok(var strr: TStrTokRec): PChar;

type

    PGroupNode = ^TGroupNode;
    TGroupNode = record
        Left, Right, _prev, _next: PGroupNode;
        Depth: Cardinal;
        Hash: Cardinal;     // hash of the group name alone
        szGroup: PChar;
        cchGroup: Integer;
        hMenu: THandle;
        hMenuGroupID: Integer;
        dwItems: Cardinal;
    end;

    PGroupNodeList = ^TGroupNodeList;
    TGroupNodeList = record
        First, Last: PGroupNode;
    end;

    function AllocGroupNode(list: PGroupNodeList; Root: PGroupNode; Depth: Integer): PGroupNode;
    function FindGroupNode(P: PGroupNode; const Hash, Depth: Integer): PGroupNode;

type

    // a contact can never be a submenu too.
    TSlotDrawType = (dtEntry, dtGroup, dtSubmenu, dtContact);
    TSlotDrawTypes = set of TSlotDrawType;

    PMenuDrawInfo = ^TMenuDrawInfo;
    TMenuDrawInfo = record
        szText: PChar;
        szProfile: PChar;
        cch: Integer;
        wID: Integer;              // should be the same as the menu item's ID
        fTypes: TSlotDrawTypes;
        hContact: THandle;
		hStatusIcon: THandle;	   // HICON from Self^.ProtoIcons[index].hIcons[status]; Do not DestroyIcon()!
        pid: Integer;
    end;

implementation

    {$include m_helpers.inc}

    function FindGroupNode(P: PGroupNode; const Hash, Depth: Integer): PGroupNode;
    begin
        Result := P;
        while Result <> nil do
        begin
            if (Result^.Hash = Hash) and (Result^.Depth = Depth) then Exit;
            If Result^.Left <> nil then
            begin
                P := Result;
                Result := FindGroupNode(Result^.Left, Hash, Depth);
                If Result <> nil then Exit;
                Result := P;
            end;
            Result := Result^.Right;
        end; //while
    end;

    function AllocGroupNode(list: PGroupNodeList; Root: PGroupNode; Depth: Integer): PGroupNode;
    begin
        New(Result);
        Result^.Left := nil;
        Result^.Right := nil;
        Result^.Depth := Depth;
        if Depth > 0 then
        begin
            if root^.left = nil then root^.left := Result
            else begin
                root := root^.left;
                while root^.right <> nil do root := root^.right;
                root^.right := Result;
            end;
        end else
        begin
            if list^.first = nil then list^.first := Result;
            if list^.last <> nil then list^.last^.right := Result;
            list^.last := Result;
        end; //if
    end;

    procedure ipcPrepareRequests(ipcPacketSize: int; pipch: PHeaderIPC; fRequests: Cardinal);
    begin
        // some fields may already have values like the event object name to open
        pipch^.cbSize := sizeof(THeaderIPC);
        pipch^.dwVersion := PLUGIN_MAKE_VERSION(1,0,6,6);
		pipch^.dwFlags := 0;
        pipch^.pServerBaseAddress := nil;
        pipch^.pClientBaseAddress := pipch;
        pipch^.fRequests := fRequests;
        pipch^.Slots := 0;
        pipch^.IconsBegin := nil;
        pipch^.ContactsBegin := nil;
        pipch^.GroupsBegin := nil;
		pipch^.NewIconsBegin := nil;
        pipch^.DataSize := ipcPacketSize - pipch^.cbSize;
        // the server side will adjust these pointers as soon as it opens
        // the mapped file to it's base address, these are set 'ere because ipcAlloc()
        // maybe used on the client side and are translated by the server side.
        // ipcAlloc() is used on the client side when transferring filenames
        // to the ST thread.
        Integer(pipch^.DataPtr) := Integer(pipch) + sizeof(THeaderIPC);
        Integer(pipch^.DataPtrEnd) := Integer(pipch^.DataPtr) + pipch^.DataSize;
        pipch^.DataFramePtr := pipch^.DataPtr;
        // fill the data area
        FillChar(pipch^.DataPtr^, pipch^.DataSize, 0);
    end;

    function ipcSendRequest(hSignal, hWaitFor: THandle; pipch: PHeaderIPC; dwTimeoutMsecs: DWORD): Cardinal;
    begin
        { signal ST to work }
        SetEvent(hSignal);
        { wait for reply, it should open a handle to hWaitFor... }
		while True do
		begin
			Result := WaitForSingleObjectEx(hWaitFor, dwTimeoutMsecs, True);
			if Result = WAIT_OBJECT_0 then
			begin
				Result := pipch^.fRequests;	break;
			end else if Result = WAIT_IO_COMPLETION	then
			begin
				(* APC call... *)
			end else begin
				Result := REPLY_FAIL; break;
			end; //if
		end; //while
    end;

    function ipcAlloc(pipch: PHeaderIPC; nSize: Integer): PSlotIPC;
    var
        PSP: int;
    begin
        Result := nil;
        { nSize maybe zero, in that case there is no string section --- }
        PSP := int( pipch^.DataFramePtr ) + sizeof(TSlotIPC) + nSize;
        { is it past the end? }
        If PSP >= int(pipch^.DataPtrEnd) then Exit;
        { return the pointer }
        Result := pipch^.DataFramePtr;
        { set up the item }
        Result^.cbSize := sizeof(TSlotIPC);
        Result^.cbStrSection := nSize;
        { update the frame ptr }
        pipch^.DataFramePtr := Pointer(PSP);
        { let this item jump to the next yet-to-be-allocated-item which should be null anyway }
        Result^.Next := Pointer(PSP);
    end;

    procedure ipcFixupAddresses(FromServer: LongBool; pipch: PHeaderIPC);
    var
        pct: PSlotIPC;
        q: ^PSlotIPC;
        iServerBase: Integer;
        iClientBase: Integer;
    begin
        if pipch^.pServerBaseAddress = pipch^.pClientBaseAddress then Exit;
        iServerBase := Integer(pipch^.pServerBaseAddress);
        iClientBase := Integer(pipch^.pClientBaseAddress);
        // fix up all the pointers in the header
        if pipch^.iconsBegin <> nil then
        begin
            Integer(pipch^.IconsBegin) := (Integer(pipch^.IconsBegin) - iServerBase) + iClientBase;
        end; //if
        if pipch^.contactsBegin <> nil then
        begin
            Integer(pipch^.ContactsBegin) := (Integer(pipch^.ContactsBegin) - iServerBase) + iClientBase;
        end; //if
        if pipch^.groupsBegin <> nil then
        begin
            Integer(pipch^.GroupsBegin) := (Integer(pipch^.GroupsBegin) - iServerBase) + iClientBase;
        end; //if
		if pipch^.NewIconsBegin <> nil then
		begin
			Integer(pipch^.NewIconsBegin) := (Integer(pipch^.NewIconsBegin) - iServerBase) + iClientBase;
		end;
        Integer(pipch^.DataPtr) := (Integer(pipch^.DataPtr) - iServerBase) + iClientBase;
        Integer(pipch^.DataPtrEnd) := (Integer(pipch^.DataPtrEnd) - iServerBase) + iClientBase;
        Integer(pipch^.DataFramePtr) := (Integer(pipch^.DataFramePtr) - iServerBase) + iClientBase;
        // and the link list
        pct := pipch^.DataPtr;
        while (pct <> nil) do
        begin
            // the first pointer is already fixed up, have to get a pointer
            // to the next pointer and modify where it jumps to
            q := @pct^.Next;
            if q^ <> nil then
            begin
                Integer(q^) := (Integer(q^) - iServerBase) + iClientBase;
            end; //if
            pct := q^;
        end; //while
    end;

    function StrTok(var strr: TStrTokRec): PChar;
    begin
        Result := nil;
        { don't allow #0's in sets or null strings }
        If (strr.szStr = nil) or (#0 in strr.szSet) then Exit;
        { strip any leading delimiters }
        while strr.szStr^ in strr.szSet do Inc(strr.szStr);
        { end on null? full of delimiters }
        If strr.szStr^ = #0 then
        begin
            // wipe out the pointer
            strr.szStr := nil;
            Exit;
        end;
        { store the start of the token }
        Result := strr.szStr;
        { process til start of another delim }
        while not (strr.szStr^ in strr.szSet) do
        begin
            { don't process past the real null, is a delimter required to cap the token? }
            If strr.szStr^ = #0 then Break;
            Inc(strr.szStr);
        end;
        { if we end on a null stop reprocessin' }
        If strr.szStr^ = #0 then
        begin
            // no more tokens can be read
            strr.szStr := nil;
            // is a ending delimiter required?
            If strr.bSetTerminator then
            begin
                // rollback
                strr.szStr := Result;
                Result := nil;
            end;
            //
        end else
        begin
            { mark the end of the token, may AV if a constant pchar is passed }
            strr.szStr^ := #0;
            { skip past this fake null for next time }
            Inc(strr.szStr);
        end;
    end;

end.


