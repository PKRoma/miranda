/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2008 Miranda ICQ/IM project,
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

#include "commonheaders.h"
#include "FontService.h"

#if defined( _UNICODE )
void ConvertFontSettings( FontSettings* fs, TFontSettings* fsw)
{
	fsw->colour = fs->colour;
	fsw->size = fs->size;
	fsw->style = fs->style;
	fsw->charset = fs->charset;

	MultiByteToWideChar( code_page, 0, fs->szFace, -1, fsw->szFace, LF_FACESIZE);
}

void ConvertFontID( FontID *fid, TFontID* fidw )
{
	memset(fidw, 0, sizeof(TFontID));
	fidw->cbSize = sizeof(TFontID);
	strcpy(fidw->dbSettingsGroup, fid->dbSettingsGroup);
	strcpy(fidw->prefix, fid->prefix);
	fidw->flags = fid->flags;
	fidw->order = fid->order;
	ConvertFontSettings(&fid->deffontsettings, &fidw->deffontsettings);

	MultiByteToWideChar( code_page, 0, fid->group, -1, fidw->group, 64);
	MultiByteToWideChar( code_page, 0, fid->name, -1, fidw->name, 64);
	if (fid->cbSize >= FontID_SIZEOF_V2A) {
		MultiByteToWideChar( code_page, 0, fid->backgroundGroup, -1, fidw->backgroundGroup, 64);
		MultiByteToWideChar( code_page, 0, fid->backgroundName, -1, fidw->backgroundName, 64);
	}
}

void ConvertColourID(ColourID *cid, TColourID* cidw)
{
	cidw->cbSize = sizeof(TColourID);

	strcpy(cidw->dbSettingsGroup, cid->dbSettingsGroup);
	strcpy(cidw->setting, cid->setting);
	cidw->flags = cid->flags;
	cidw->defcolour = cid->defcolour;
	cidw->order = cid->order;

	MultiByteToWideChar( code_page, 0, cid->group, -1, cidw->group, 64);
	MultiByteToWideChar( code_page, 0, cid->name, -1, cidw->name, 64);
}

void ConvertLOGFONT(LOGFONTW *lfw, LOGFONTA *lfa)
{
	lfa->lfHeight = lfw->lfHeight;
	lfa->lfWidth = lfw->lfWidth;
	lfa->lfEscapement = lfw->lfEscapement;
	lfa->lfOrientation = lfw->lfOrientation;
	lfa->lfWeight = lfw->lfWeight;
	lfa->lfItalic = lfw->lfItalic;
	lfa->lfUnderline = lfw->lfUnderline;
	lfa->lfStrikeOut = lfw->lfStrikeOut;
	lfa->lfCharSet = lfw->lfCharSet;
	lfa->lfOutPrecision = lfw->lfOutPrecision;
	lfa->lfClipPrecision = lfw->lfClipPrecision;
	lfa->lfQuality = lfw->lfQuality;
	lfa->lfPitchAndFamily = lfw->lfPitchAndFamily;

	WideCharToMultiByte( code_page, 0, lfw->lfFaceName, -1, lfa->lfFaceName, LF_FACESIZE, 0, 0);
}
#endif

static void GetDefaultFontSetting(LOGFONT* lf, COLORREF* colour)
{
	SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), lf, FALSE);
	if ( colour )
		*colour = GetSysColor(COLOR_WINDOWTEXT);

	lf->lfHeight = 10;
	
	HDC hdc = GetDC(0);
	lf->lfHeight = -MulDiv(lf->lfHeight,GetDeviceCaps(hdc, LOGPIXELSY), 72);
	ReleaseDC(0, hdc);
}

int GetFontSettingFromDB(char *settings_group, char *prefix, LOGFONT* lf, COLORREF * colour, DWORD flags)
{
	DBVARIANT dbv;
	char idstr[256];
	BYTE style;
	int retval = 0;

	GetDefaultFontSetting(lf, colour);

	if(flags & FIDF_APPENDNAME) sprintf(idstr, "%sName", prefix);
	else sprintf(idstr, "%s", prefix);

	if ( !DBGetContactSettingTString(NULL, settings_group, idstr, &dbv )) {
		_tcscpy(lf->lfFaceName, dbv.ptszVal);
		DBFreeVariant(&dbv);
	}
   else retval = 1;

	if (colour) {
		sprintf(idstr, "%sCol", prefix);
		*colour = DBGetContactSettingDword(NULL, settings_group, idstr, *colour);
	}

	sprintf(idstr, "%sSize", prefix);
	lf->lfHeight = (char)DBGetContactSettingByte(NULL, settings_group, idstr, lf->lfHeight);


	//wsprintf(idstr, "%sFlags", prefix);
	//if(DBGetContactSettingDword(NULL, settings_group, idstr, 0) & FIDF_SAVEACTUALHEIGHT) {
	//	HDC hdc = GetDC(0);
	//	lf->lfHeight = -lf->lfHeight;
	//	ReleaseDC(0, hdc);
	//}

	sprintf(idstr, "%sSty", prefix);
	style = (BYTE) DBGetContactSettingByte(NULL, settings_group, idstr, 
		(lf->lfWeight == FW_NORMAL ? 0 : DBFONTF_BOLD) | (lf->lfItalic ? DBFONTF_ITALIC : 0) | (lf->lfUnderline ? DBFONTF_UNDERLINE : 0) | lf->lfStrikeOut ? DBFONTF_STRIKEOUT : 0);

	lf->lfWidth = lf->lfEscapement = lf->lfOrientation = 0;
	lf->lfWeight = style & DBFONTF_BOLD ? FW_BOLD : FW_NORMAL;
	lf->lfItalic = (style & DBFONTF_ITALIC) != 0;
	lf->lfUnderline = (style & DBFONTF_UNDERLINE) != 0;
	lf->lfStrikeOut = (style & DBFONTF_STRIKEOUT) != 0;

	sprintf(idstr, "%sSet", prefix);
	lf->lfCharSet = DBGetContactSettingByte(NULL, settings_group, idstr, lf->lfCharSet);

	lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
	lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf->lfQuality = DEFAULT_QUALITY;
	lf->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

	if(lf->lfHeight > 0) {
		HDC hdc = GetDC(0);
		if(flags & FIDF_SAVEPOINTSIZE) {
			lf->lfHeight = -MulDiv(lf->lfHeight,GetDeviceCaps(hdc, LOGPIXELSY), 72);
		} else { // assume SAVEACTUALHEIGHT
			TEXTMETRIC tm;
			HFONT hFont = CreateFontIndirect(lf);
			HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

			GetTextMetrics(hdc, &tm);

			lf->lfHeight = -(lf->lfHeight - tm.tmInternalLeading);

			SelectObject(hdc, hOldFont);
			DeleteObject(hFont);
		}
		//lf->lfHeight = -MulDiv(lf->lfHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		ReleaseDC(0, hdc);
	}

	return retval;
}

int CreateFromFontSettings(TFontSettings* fs, LOGFONT* lf, DWORD flags)
{
	GetDefaultFontSetting(lf, 0);

	_tcscpy(lf->lfFaceName, fs->szFace);

	lf->lfWidth = lf->lfEscapement = lf->lfOrientation = 0;
	lf->lfWeight = fs->style & DBFONTF_BOLD ? FW_BOLD : FW_NORMAL;
	lf->lfItalic = (fs->style & DBFONTF_ITALIC) != 0;
	lf->lfUnderline = (fs->style & DBFONTF_UNDERLINE) != 0;
	lf->lfStrikeOut = (fs->style & DBFONTF_STRIKEOUT) != 0;;
	lf->lfCharSet = fs->charset;
	lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
	lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf->lfQuality = DEFAULT_QUALITY;
	lf->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

	lf->lfHeight = fs->size;
	return 0;
}

void UpdateFontSettings(TFontID* font_id, TFontSettings* fontsettings)
{
	LOGFONT lf;
	COLORREF colour;
	if ( GetFontSettingFromDB(font_id->dbSettingsGroup, font_id->prefix, &lf, &colour, font_id->flags) && (font_id->flags & FIDF_DEFAULTVALID)) {
		CreateFromFontSettings(&font_id->deffontsettings, &lf, font_id->flags);// & ~FIDF_SAVEACTUALHEIGHT);
		colour = font_id->deffontsettings.colour;
	}

	fontsettings->style =
		(lf.lfWeight == FW_NORMAL ? 0 : DBFONTF_BOLD) | (lf.lfItalic ? DBFONTF_ITALIC : 0) | (lf.lfUnderline ? DBFONTF_UNDERLINE : 0) | (lf.lfStrikeOut ? DBFONTF_STRIKEOUT : 0);

	fontsettings->size = (char)lf.lfHeight;
	fontsettings->charset = lf.lfCharSet;
	fontsettings->colour = colour;
	_tcscpy(fontsettings->szFace, lf.lfFaceName);
}

/////////////////////////////////////////////////////////////////////////////////////////
// RegisterFont service

static int sttRegisterFontWorker( TFontID* font_id )
{
	for ( int i = 0; i < font_id_list.getCount(); i++ ) {
		TFontID& F = font_id_list[i];
		if ( !lstrcmp( F.group, font_id->group ) && !lstrcmp( F.name, font_id->name ) && !( F.flags & FIDF_ALLOWREREGISTER ))
			return 1;
	}

	char idstr[256];
	sprintf(idstr, "%sFlags", font_id->prefix);
	DBWriteContactSettingDword(0, font_id->dbSettingsGroup, idstr, font_id->flags);
	{	
		TFontID* newItem = new TFontID;
		memset( newItem, 0, sizeof( TFontID ));
		memcpy( newItem, font_id, font_id->cbSize);
		UpdateFontSettings( font_id, &newItem->value );
		font_id_list.insert( newItem );
	}
	return 0;
}

#if defined( _UNICODE )
int RegisterFontW(WPARAM wParam, LPARAM lParam)
{
	return sttRegisterFontWorker(( TFontID* )wParam );
}
#endif

int RegisterFont(WPARAM wParam, LPARAM lParam)
{
	#if defined( _UNICODE )
		TFontID temp;
		ConvertFontID( ( FontID* )wParam, &temp );
		return sttRegisterFontWorker( &temp );
	#else
		return sttRegisterFontWorker(( TFontID* )wParam );
	#endif
}

/////////////////////////////////////////////////////////////////////////////////////////
// GetFont service

static int sttGetFontWorker( TFontID* font_id, LOGFONT* lf )
{
	COLORREF colour;

	for ( int i = 0; i < font_id_list.getCount(); i++ ) {
		TFontID& F = font_id_list[i];
		if ( !_tcsncmp( F.name, font_id->name, SIZEOF(F.name)) && !_tcsncmp( F.group, font_id->group, SIZEOF(F.group))) {
			if ( GetFontSettingFromDB( F.dbSettingsGroup, F.prefix, lf, &colour, F.flags) && ( F.flags & FIDF_DEFAULTVALID )) {
				CreateFromFontSettings( &F.deffontsettings, lf, F.flags);
				colour = F.deffontsettings.colour;
			}

			return (int)colour;
	}	}

	GetDefaultFontSetting( lf, &colour );
	return (int)colour;
}

#if defined( _UNICODE )
int GetFontW(WPARAM wParam, LPARAM lParam)
{
	return sttGetFontWorker(( TFontID* )wParam, ( LOGFONT* )lParam );
}
#endif

int GetFont(WPARAM wParam, LPARAM lParam)
{
	#if defined( _UNICODE )
		TFontID temp;
		LOGFONT lftemp;
		ConvertFontID((FontID *)wParam, &temp);
		{	int ret = sttGetFontWorker( &temp, &lftemp );
			ConvertLOGFONT( &lftemp, ( LOGFONTA* )lParam );
			return ret;
		}
	#else
		return sttGetFontWorker(( TFontID* )wParam, ( LOGFONT* )lParam );
	#endif
}

/////////////////////////////////////////////////////////////////////////////////////////
// RegisterColour service

void UpdateColourSettings( TColourID* colour_id, COLORREF *colour)
{
	*colour = ( COLORREF )DBGetContactSettingDword(NULL, colour_id->dbSettingsGroup, colour_id->setting, colour_id->defcolour );
}

static int sttRegisterColourWorker( TColourID* colour_id )
{
	for ( int i = 0; i < colour_id_list.getCount(); i++ ) {
		TColourID& C = colour_id_list[i];
		if ( !_tcscmp( C.group, colour_id->group ) && !_tcscmp( C.name, colour_id->name ))
			return 1;
	}

	TColourID* newItem = new TColourID;
	memcpy( newItem, colour_id, sizeof( TColourID ));
	UpdateColourSettings( colour_id, &newItem->value );
	colour_id_list.insert( newItem );
	return 0;
}

#if defined( _UNICODE )
int RegisterColourW(WPARAM wParam, LPARAM lParam)
{
	return sttRegisterColourWorker(( TColourID* )wParam );
}
#endif

int RegisterColour(WPARAM wParam, LPARAM lParam)
{
	#if defined( _UNICODE )
		TColourID temp;
		ConvertColourID(( ColourID* )wParam, &temp );
		return sttRegisterColourWorker( &temp );
	#else
		return sttRegisterColourWorker(( TColourID* )wParam );
	#endif
}

/////////////////////////////////////////////////////////////////////////////////////////
// GetColour service

static int sttGetColourWorker( TColourID* colour_id )
{
	int i;

	for ( i = 0; i < colour_id_list.getCount(); i++ ) {
		TColourID& C = colour_id_list[i];
		if ( !_tcscmp( C.group, colour_id->group ) && !_tcscmp( C.name, colour_id->name ))
			return (int)DBGetContactSettingDword(NULL, C.dbSettingsGroup, C.setting, C.defcolour);
	}

	return -1;
}

#if defined( _UNICODE )
int GetColourW(WPARAM wParam, LPARAM lParam)
{
	return sttGetColourWorker(( TColourID* )wParam );
}
#endif

int GetColour(WPARAM wParam, LPARAM lParam)
{
	#if defined( _UNICODE )
		TColourID temp;
		ConvertColourID(( ColourID* )wParam, &temp );
		return sttGetColourWorker( &temp );
	#else
		return sttGetColourWorker(( TColourID* )wParam );
	#endif
}
