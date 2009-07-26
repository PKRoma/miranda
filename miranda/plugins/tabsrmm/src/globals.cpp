/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2009 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

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

$Id:$

*/

#include "commonheaders.h"
extern PLUGININFOEX pluginInfo;
static char *relnotes[] = {
	"{\\rtf1\\ansi\\deff0\\pard\\li%u\\fi-%u\\ri%u\\tx%u}",
	"\\par\t\\b\\ul1 Release notes for version 3.0.0.0\\b0\\ul0\\par ",
	"*\tDevel branch for Miranda 0.9. Does NOT work in Miranda 0.8 or earlier.\\par ",
	"\t\\b View all release notes and history online:\\b0 \\par \thttp://wiki.miranda.or.at/TabSrmm:ChangeLog\\par ",
	NULL
};

/*
 * display release notes dialog
 */

void _Globals::ViewReleaseNotes(bool fForced = 0, bool fTerminated = false)
{
	if(fTerminated) {
		m_showRelnotes = false;
		return;
	}

	if(!fForced) {
		if(m_showRelnotes)
			return;
		if (M->GetDword("last_relnotes", 0) < pluginInfo.version)
			M->WriteDword(SRMSGMOD_T, "last_relnotes", pluginInfo.version);
		else
			return;
	}
	else if(m_showRelnotes)
		return;

	m_showRelnotes = true;
	::CreateDialogParam(::g_hInst, MAKEINTRESOURCE(IDD_VARIABLEHELP), 0, ::DlgProcTemplateHelp, (LPARAM)relnotes);
}

_Globals _Plugin;
_Globals *pConfig = &_Plugin;

