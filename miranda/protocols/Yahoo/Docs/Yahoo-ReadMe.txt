Protocol for the Miranda IM for communicating with users of
the Yahoo Messenger protocol.

Copyright (C) 2003-5 Gennady Feldman (aka Gena01)
Copyright (C) 2003-4 Laurent Marechal (aka Peorth)

Thanks to Robert Rainwater and George Hazan for their code and support.


Miranda IM: the free icq client for MS Windows
Copyright (C) 2002-3  Martin Oberg, Robert Rainwater, Sam Kothari, Lyon Lim
Copyright (C) 2000-2  Richard Hughes, Roland Rabien & Tristan Van de Vreede

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

=================================================================================


I- Information
    This is a plug-in for Miranda IM. It provides the Yahoo protocol using the 
    Libyahoo2 library.


II- Requirement for compilation
    Mingw32 v3.2 min 
    It might work with other compiler, but you might need to make some change.
    Miranda SDK
    Some features require popup plugin.


III- Installation
    Binary version:
        Unzip and copy the dll into your Miranda plugin directory.
        Restart Miranda.
        Go into the Option/Network/Yahoo menu of Miranda.
        Enter the connection information (username/password).
        If you have the popup plugin:
                Go into the Option/Popups/Yahoo menu of Miranda.
                Set your preference.

    Source version:
        Unzip and change the project to fit you location.
        Compile (see your compiler documentation).
        Copy the dll into your Miranda plugin directory.
        Restart Miranda.
        Go into the Option/Network/Yahoo menu of Miranda.
        Enter the connection information (username/password).
        If you have the popup plugin:
                Go into the Option/Popups/Yahoo menu of Miranda.
                Set your preference.



IV- Limitation
    See Feature.txt
    Some features cannot be implemented at this time in Miranda. This is caused
    by some missing features in the core. Some are planed for Miranda M1, some
    might be implemented before.

    The status matching between Yahoo and Miranda is as follow:
    Yahoo -> Miranda
        Yahoo_Available     - Miranda_Online
        Yahoo_BRB           - Miranda_N/A
        Yahoo_Busy          - Miranda_Occuped
        Yahoo_NotAtHome     - Miranda_Away
        Yahoo_NotAtDesk     - Miranda_Away
        Yahoo_NotInOffice   - Miranda_Away
        Yahoo_OnVacation    - Miranda_Away
        Yahoo_OnThePhone    - Miranda_OnThePhone
        Yahoo_OutToLunch    - Miranda_OutToLunch
        Yahoo_Invisible     - Miranda_Invisible
        Yahoo_CustomeOnline - Miranda_Online
        Yahoo_CustomeBusy   - Miranda_Occuped
        Yahoo_SteppedOut    - Miranda_DND

    Miranda -> Yahoo
        Miranda_Online      - Yahoo_Available
        Miranda_N/A         - Yahoo_BRB
        Miranda_Occuped     - Yahoo_Busy
        Miranda_Away        - Yahoo_NotAtDesk
        Miranda_OnThePhone  - Yahoo_OnThePhone
        Miranda_OutToLunch  - Yahoo_OutToLunch
        Miranda_Invisible   - Yahoo_Invisible
        Miranda_DND         - Yahoo_SteppedOut
        
        
V- Download
    Download can be done on the Miranda-IM site in the plugin section.
    Source are available from CVS in the developement section.


VI- Bug
    When reporting bug, please make sure you also report which plugin/version
    you are using. You can use the 'Version Information' plugin available on the
    Miranda download section for that.
    Bug report should be sent to the bug tracker. You can report the bug here :
	http://bugs.miranda-im.org/


VII- Support
    Support can be done via the community forum at the following address:
	http://forums.miranda-im.org/
