// Copyright Scott Ellis (mail@scottellis.com.au) 2005
// This software is licenced under the GPL (General Public Licence)
// available at http://www.gnu.org/copyleft/gpl.html

#ifndef _FONT_SERVICE_API_INC
#define _FONT_SERVICE_API_INC

// style flags
#define DBFONTF_BOLD       1
#define DBFONTF_ITALIC     2
#define DBFONTF_UNDERLINE  4
#define DBFONTF_STRIKEOUT  8

// flags for compatibility
#define FIDF_APPENDNAME			1		// append 'Name' to the setting used to store font face (as CLC settings require)
#define FIDF_NOAS				2		// disable the <prefix>As setting to prevent 'same as' behaviour
#define FIDF_SAVEACTUALHEIGHT	4		// write the actual height of a test string to the db
#define FIDF_SAVEPOINTSIZE		8		// write the font point size to the db

// additional flags
#define FIDF_DEFAULTVALID		32		// the default font settings are valid - else, just use generic default
#define FIDF_NEEDRESTART		64		// setting changes will not take effect until miranda is restarted
#define FIDF_ALLOWREREGISTER	128		// allow plugins to register this font again (i.e. override already registered settings such as flags)
#define FIDF_ALLOWEFFECTS		256		// allow setting of font effects (i.e. underline and strikeout)

// settings to be used for the value of 'deffontsettings' in the FontID structure below - i.e. defaults
typedef struct FontSettings_tag
{
    COLORREF colour;
    char size;
    BYTE style;					// see the DBFONTF_* flags above
    BYTE charset;
    char szFace[LF_FACESIZE];
} FontSettings;

// a font identifier structire - used for registering a font, and getting one out again
typedef struct FontID_tag {
	int cbSize;
	char group[64];				// group the font belongs to - this is the 'Font Group' list in the options page
	char name[64];				// this is the name of the font setting - e.g. 'contacts' in the 'contact list' group
	char dbSettingsGroup[32];	// the 'module' in the database where the font data is stored
	char prefix[32];			// this is prepended to the settings used to store this font's data in the db
	DWORD flags;				// bitwise OR of the FIDF_* flags above
	FontSettings deffontsettings; // defaults, valid if flags & FIDF_DEFAULTVALID
	int order;					// controls the order in the font group in which the fonts are listed in the UI (if order fields are equal,
								// they will be ordered alphabetically by name)
} FontID;

typedef struct ColourID_tag {
	int cbSize;
	char group[64];
	char name[64];
	char dbSettingsGroup[32];
	char setting[32];
	DWORD flags;		// not used
	COLORREF defcolour; // default value
	int order;
} ColourID;

// register a font
// wparam = (FontID *)&font_id
// lparam = 0
#define MS_FONT_REGISTER		"Font/Register"

// get a font
// will fill the logfont structure passed in with the user's choices, or the default if it was set and the user has not chosen a font yet,
// or the global default font settings otherwise (i.e. no user choice and default set, or font not registered)
// global default font is gotten using SPI_GETICONTITLELOGFONT, color COLOR_WINDOWTEXT, size 8.
// wparam = (FontID *)&font_id (only name and group matter)
// lParam = (LOGFONT *)&logfont
// returns the font's colour
#define MS_FONT_GET				"Font/Get"

// fired when a user modifies font settings, so reload your fonts
// wparam = lparam = 0
#define ME_FONT_RELOAD			"Font/Reload"

// register a colour (this should be used for everything except actual text colour for registered fonts)
// [note - a colour with name 'Background' [translated!] has special meaning and will be used as the background colour of 
// the font list box in the options, for the given group]
// wparam = (ColourID *)&colour_id
// lparam = 0
#define MS_COLOUR_REGISTER		"Colour/Register"

// get a colour
// wparam = (ColourID *)&colour_id (only name and group matter)
// lParam = (LOGFONT *)&logfont
#define MS_COLOUR_GET				"Colour/Get"

// fired when a user modifies font settings, so reget your fonts and colours
// wparam = lparam = 0
#define ME_COLOUR_RELOAD			"Colour/Reload"



//////////////////// Example ///////////////
#ifdef I_AM_A_CONSTANT_THAT_IS_NEVER_DEFINED_BUT_ALLOWS_THE_CODE_BELOW_NOT_TO_BE_COMMENTED


// In the modules loaded event handler, register your fonts
int testOnModulesLoaded(WPARAM wParam, LPARAM lParam) {
	FontID fid = {0};
	fid.cbSize = sizeof(fid);
	strncpy(fid.name, "Test Font", 64);
	strncpy(fid.group, "My Group", 64);
	strncpy(fid.dbSettingsGroup, "MyPlugin", 32);
	strncpy(fid.prefix, "testFont", 32);
	fid.order = 0;

	// you could register the font at this point - getting it will get either the global default or what the user has set it 
	// to - but we'll set a default font:

	fid.flags = FIDF_DEFAULTVALID;

	fid.deffontsettings.charset = DEFAULT_CHARSET;
	fid.deffontsettings.colour = RGB(255, 0, 0);
	fid.deffontsettings.size = 8;
	fid.deffontsettings.style = DBFONTF_BOLD;
	strncpy(fid.deffontsettings.szFace, "Arial", LF_FACESIZE);

	CallService(MS_FONT_REGISTER, (WPARAM)&fid, 0);

	// if you add more fonts, and leave the 'order' field at 0, they will be ordered alphabetically
	
	// .....

	return 0;
}

// Later, when you need the LOGFONT structure for drawing with a font, do this
void InSomeCodeSomewhere() {
	//.....
	LOGFONT lf;
	COLORREF col;
	FontID fid;
	strncpy(fid.name, "Test Font", 64);
	strncpy(fid.group, "My Group", 64);

	col = (COLORREF)CallService(MS_FONT_GET, (WPARAM)&fid, (LPARAM)&lf);

	// then procede to use the font
	//....
}

#endif // example code

#endif // _FONT_SERVICE_API_INC

