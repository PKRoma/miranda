/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
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
*/
#include "../../core/commonheaders.h"
#include "profilemanager.h"


// from the plugin loader, hate extern but the db frontend is pretty much tied
extern PLUGINLINK pluginCoreLink;
// contains the location of mirandaboot.ini
char mirandabootini[MAX_PATH];

// returns 1 if the profile path was returned, without trailing slash
int getProfilePath(char * buf, size_t cch)
{
	char profiledir[MAX_PATH];
	char exprofiledir[MAX_PATH];
	char * p = 0;
	// grab the base location now
	GetModuleFileName(NULL, buf, cch);
	p = strrchr(buf, '\\');
	if ( p != 0 ) *p=0;
	// change to this location, or "." wont expand properly
	_chdir(buf);
	// get the string containing envars and maybe relative paths
	GetPrivateProfileString("Database", "ProfileDir", ".", profiledir, sizeof(profiledir), mirandabootini);
	// get rid of the vars 
	ExpandEnvironmentStrings(profiledir, exprofiledir, sizeof(exprofiledir));
	if ( _fullpath(profiledir, exprofiledir, sizeof(profiledir)) != 0 ) {
		/* XXX: really use CreateDirectory()? it only creates the last dir given a\b\c, SHCreateDirectory() 
		does what we want however thats 2000+ only  */
		DWORD dw = INVALID_FILE_ATTRIBUTES;
		CreateDirectory(profiledir, NULL);
		dw=GetFileAttributes(profiledir);
		if ( dw != INVALID_FILE_ATTRIBUTES && dw&FILE_ATTRIBUTE_DIRECTORY )  {
			strncpy(buf, profiledir, cch);
			p = strrchr(buf, '\\');
			// if the char after '\' is null then change '\' to null
			if ( p != 0 && *(++p)==0 ) *(--p)=0;			
			return 1;
		}
	}
	// this never happens, usually C:\ is always returned	
	return 1;
}

// fills mirandabootini, called from Load
void getMirandaBootIni(void)
{
	char exe[MAX_PATH];
	char * p = 0;
	GetModuleFileName(NULL, exe, sizeof(exe));
	p = strrchr(exe, '\\');
	if ( p != 0 ) *p=0;
	_snprintf(mirandabootini, sizeof(mirandabootini), "%s\\mirandaboot.ini", exe);
}

// returns 1 if *.dat spec is matched
int isValidProfileName(char * name)
{
	char * p = strrchr(name, '.');	
	if ( p ) {
		p++;
		if ( lstrcmpi( p, "dat" ) == 0 ) { 
			if ( p[3] == 0 ) return 1; 
		}
	}
	return 0;
}

// returns 1 if a single profile (full path) is found within the profile dir
static int getProfile1(char * szProfile, size_t cch, char * profiledir, BOOL * noProfiles)
{
	int rc = 1;
	char searchspec[MAX_PATH];
	WIN32_FIND_DATA ffd;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	unsigned int found=0;
	_snprintf(searchspec,sizeof(searchspec),"%s\\*.dat", profiledir);
	hFind = FindFirstFile(searchspec, &ffd);
	if ( hFind != INVALID_HANDLE_VALUE ) 
	{
		// make sure the first hit is actually a *.dat file
		if ( !(ffd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && isValidProfileName(ffd.cFileName) ) 
		{
			// copy the profile name early cos it might be the only one
			_snprintf(szProfile, cch, "%s\\%s", profiledir, ffd.cFileName);
			found++;
			// this might be the only dat but there might be a few wrong things returned before another *.dat
			while ( FindNextFile(hFind,&ffd) ) {
				// found another *.dat, but valid?
				if ( !(ffd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && isValidProfileName(ffd.cFileName) ) {
					rc=0;
					found++;
					break;
				} //if
			} // while
		} //if
		FindClose(hFind);
	}
	if ( found == 0 && noProfiles != 0 ) { 
		*noProfiles=TRUE;
		rc=0;
	}
	return rc;
}

// returns 1 if a profile was selected
static int getProfile(char * szProfile, size_t cch)
{
	char profiledir[MAX_PATH];
	PROFILEMANAGERDATA pd;
	ZeroMemory(&pd,sizeof(pd));
	getProfilePath(profiledir,sizeof(profiledir));
	if ( getProfile1(szProfile, cch, profiledir, &pd.noProfiles) ) return 1;
	else {		
		pd.szProfile=szProfile;
		pd.szProfileDir=profiledir;
		return getProfileManager(&pd);
	}
}

// called by the UI, return 1 on success, use link to create profile, set error if any
int makeDatabase(char * profile, DATABASELINK * link, HWND hwndDlg)
{
	char buf[256];
	int err=0;	
	// check if the file already exists
	HANDLE hFile=CreateFile(profile, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	char * file = strrchr(profile,'\\');
	file++;
	if ( hFile != INVALID_HANDLE_VALUE ) {		
		CloseHandle(hFile);		
		_snprintf(buf,sizeof(buf),Translate("The profile '%s' already exists. Do you want to move it to the "
			"Recycle Bin? \n\nWARNING: The profile will be deleted if Recycle Bin is disabled.\nWARNING: A profile may contain confidential information and should be properly deleted."),file);
		// file already exists!
		if ( MessageBox(hwndDlg, buf, Translate("The profile already exists"), MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2) != IDYES ) return 0;
		// move the file
		{		
			char szName[MAX_PATH]; // SHFileOperation needs a "double null" 
			SHFILEOPSTRUCT sf;
			ZeroMemory(&sf,sizeof(sf));
			sf.wFunc=FO_DELETE;
			sf.pFrom=szName;
			sf.fFlags=FOF_NOCONFIRMATION|FOF_NOERRORUI|FOF_SILENT;
			_snprintf(szName,sizeof(szName),"%s\0",profile);
			if ( SHFileOperation(&sf) != 0 ) {
				_snprintf(buf,sizeof(buf),Translate("Couldn't move '%s' to the Recycle Bin, Please select another profile name."),file);
				MessageBox(0,buf,Translate("Problem moving profile"),MB_ICONINFORMATION|MB_OK);
				return 0;
			}
		}
		// now the file should be gone!
	}
	// ask the database to create the profile
	if ( link->makeDatabase(profile,&err) ) { 
		_snprintf(buf,sizeof(buf),Translate("Unable to create the profile '%s', the error was %x"),file, err);
		MessageBox(hwndDlg,buf,Translate("Problem creating profile"),MB_ICONERROR|MB_OK);
		return 0;
	}
	// the profile has been created! woot
	return 1;
}

void UnloadDatabaseModule(void)
{
}

// enumerate all plugins that had valid DatabasePluginInfo()
static int FindDbPluginForProfile(char * pluginname, DATABASELINK * dblink, LPARAM lParam)
{
	char * szProfile = (char *) lParam;
	if ( dblink && dblink->cbSize==sizeof(DATABASELINK) ) {
		int err=0;
		int rc=0;
		// liked the profile?
		rc=dblink->grokHeader(szProfile,&err);
		if ( rc == 0 ) { 			
			// added APIs?
			if ( dblink->Load(szProfile, &pluginCoreLink) == 0 ) return DBPE_DONE;
			return DBPE_HALT;
		} else {
			switch ( err ) {				 
				case EGROKPRF_CANTREAD:
				case EGROKPRF_UNKHEADER:
				{
					// just not supported.
					return DBPE_CONT;
				}
				case EGROKPRF_VERNEWER:
				case EGROKPRF_DAMAGED:
				{
					MessageBox(0,"The profile is damaged or newer than the current db driver supports.","",MB_ICONERROR);
					break;
				}
			}
			return DBPE_HALT;
		} //if
	}
	return DBPE_CONT;
}

int LoadDatabaseModule(void)
{
	char szProfile[MAX_PATH];
	// store the full location of any expected mirandaboot.ini
	getMirandaBootIni();
	// find out which profile to load
	if ( getProfile(szProfile, sizeof(szProfile)) ) {
		PLUGIN_DB_ENUM dbe;
		dbe.cbSize=sizeof(PLUGIN_DB_ENUM);
		dbe.pfnEnumCallback=FindDbPluginForProfile;
		dbe.lParam=(LPARAM)szProfile;
		// find a driver to support the given profile
		if ( CallService(MS_PLUGINS_ENUMDBPLUGINS, 0, (LPARAM)&dbe) != 0 ) {
			// no enumeration took place
			MessageBox(0,Translate("Could not load the profile, no driver found to understand it."),szProfile,MB_ICONERROR);
			return 1;
		}
	} else {
		return 1;
	}
	return 0;
}

