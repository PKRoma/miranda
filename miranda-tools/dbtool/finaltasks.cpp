/*
Miranda Database Tool
Copyright (C) 2001-2005  Richard Hughes

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
#include "dbtool.h"
#include <io.h>

extern int errorCount;

int WorkFinalTasks(int firstTime)
{
	AddToStatus(STATUS_MESSAGE,"Processing final tasks");
	dbhdr.slackSpace=0;
	if(WriteSegment(0,&dbhdr,sizeof(dbhdr))==WS_ERROR)
		return ERROR_WRITE_FAULT;
	CloseHandle(opts.hFile);
	if(!opts.bCheckOnly) CloseHandle(opts.hOutFile);
	if(opts.bAggressive) DeleteFile(opts.workingFilename) || AddToStatus(STATUS_WARNING,"Unable to delete aggressive working file");
	if(errorCount && !opts.bBackup && !opts.bCheckOnly) {
		if(IDYES==MessageBox(NULL,"Errors were encountered, however you selected not to backup the original database. It is strongly recommended that you do so in case important data was omitted. Do you wish to keep a backup of the original database?","Miranda Database Tool",MB_YESNO))
			opts.bBackup=1;
	}
	if(opts.bBackup) {
		int i;
		char dbPath[MAX_PATH],dbFile[MAX_PATH];
		char *str2;
		strcpy(dbPath,opts.filename);
		str2=strrchr(dbPath,'\\');
		if(str2!=NULL) {
			strcpy(dbFile,str2+1);
			*str2=0;
		}
		else {
			strcpy(dbFile,dbPath);
			dbPath[0]=0;
		}
		for(i=1;;i++) {
			if(i==1) wsprintf(opts.backupFilename,"%s\\Backup of %s",dbPath,dbFile);
			else wsprintf(opts.backupFilename,"%s\\Backup (%d) of %s",dbPath,i,dbFile);
			if(_access(opts.backupFilename,0)==-1) break;
		}
		MoveFile(opts.filename,opts.backupFilename) || AddToStatus(STATUS_WARNING,"Unable to rename original file");
	}
	else if(!opts.bCheckOnly) DeleteFile(opts.filename) || AddToStatus(STATUS_WARNING,"Unable to delete original file");
	if(!opts.bCheckOnly) MoveFile(opts.outputFilename,opts.filename) || AddToStatus(STATUS_WARNING,"Unable to rename output file");
	return ERROR_NO_MORE_ITEMS;
}