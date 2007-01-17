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

extern DWORD spaceProcessed,sourceFileSize;

int SignatureValid(DWORD ofs,DWORD signature)
{
	DWORD bytesRead;
	DWORD sig;

	if(ofs>=sourceFileSize)	{
		AddToStatus(STATUS_ERROR,TranslateT("Invalid offset found"));
		return 0;
	}
	SetFilePointer(opts.hFile,ofs,NULL,FILE_BEGIN);
	ReadFile(opts.hFile,&sig,sizeof(sig),&bytesRead,NULL);
	if(bytesRead<sizeof(sig)) {
		AddToStatus(STATUS_ERROR,TranslateT("Error reading, database truncated? (%u)"),GetLastError());
		return 0;
	}
	return sig==signature;
}

int PeekSegment(DWORD ofs,PVOID buf,int cbBytes)
{
	DWORD bytesRead;
	if(ofs>=sourceFileSize) {
		AddToStatus(STATUS_ERROR,TranslateT("Invalid offset found"));
		return ERROR_SEEK;
	}
	SetFilePointer(opts.hFile,ofs,NULL,FILE_BEGIN);
	ReadFile(opts.hFile,buf,cbBytes,&bytesRead,NULL);
	if(bytesRead==0) {
		AddToStatus(STATUS_ERROR,TranslateT("Error reading, database truncated? (%u)"),GetLastError());
		return ERROR_READ_FAULT;
	}
	if((int)bytesRead<cbBytes) return ERROR_HANDLE_EOF;
	return ERROR_SUCCESS;
}

int ReadSegment(DWORD ofs,PVOID buf,int cbBytes)
{
	int ret;

	ret=PeekSegment(ofs,buf,cbBytes);
	if(ret!=ERROR_SUCCESS && ret!=ERROR_HANDLE_EOF) return ret;
	if(opts.bAggressive) {
		PBYTE zeros;
		DWORD bytesWritten;
		zeros=(PBYTE)calloc(cbBytes,1);
		SetFilePointer(opts.hFile,ofs,NULL,FILE_BEGIN);
		WriteFile(opts.hFile,zeros,cbBytes,&bytesWritten,NULL);
		if((int)bytesWritten<cbBytes) AddToStatus(STATUS_WARNING,TranslateT("Can't write to working file, aggressive mode may be too aggressive now (%u)"),GetLastError());
		free(zeros);
	}
	spaceProcessed+=cbBytes;
	return ERROR_SUCCESS;
}

DWORD WriteSegment(DWORD ofs,PVOID buf,int cbBytes)
{
	DWORD bytesWritten;
	if(opts.bCheckOnly) return 0xbfbfbfbf;
	if(ofs==WSOFS_END) {
		ofs=dbhdr.ofsFileEnd;
		dbhdr.ofsFileEnd+=cbBytes;
	}
	SetFilePointer(opts.hOutFile,ofs,NULL,FILE_BEGIN);
	WriteFile(opts.hOutFile,buf,cbBytes,&bytesWritten,NULL);
	if((int)bytesWritten<cbBytes) {
		AddToStatus(STATUS_FATAL,TranslateT("Can't write to output file - disk full? (%u)"),GetLastError());
		return WS_ERROR;
	}
	return ofs;
}
