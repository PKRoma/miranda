#ifndef __M_VARS
#define __M_VARS

typedef struct {
	int cbSize;
	char *szFormat;
	char *szSource;
	HANDLE hContact;
	int pCount;  // number of succesful parses
	int eCount;	 // number of failures
} FORMATINFO;
#define MS_VARS_FORMATSTRING			"Vars/FormatString"

__inline static char *variables_parse(char *src, char *extra, HANDLE hContact) {

	FORMATINFO fi;

	ZeroMemory(&fi, sizeof(fi));
	fi.cbSize = sizeof(fi);
	fi.szFormat = src;
	fi.szSource = extra;
	fi.hContact = hContact;
	return (char *)CallService(MS_VARS_FORMATSTRING, (WPARAM)&fi, 0);
}

#define CI_PROTOID		0x00000001
#define CI_NICK			0x00000002
#define CI_LISTNAME		0x00000004
#define CI_FIRSTNAME	0x00000008
#define CI_LASTNAME		0x00000010
#define CI_EMAIL		0x00000020
#define CI_UNIQUEID		0x00000040
typedef struct {
	int cbSize;
	char *szContact;
	HANDLE *hContacts;
	DWORD flags;
} CONTACTSINFO;

// wparam = (CONTACTSINFO *)
// lparam = 0
// returns number of contacts found, hContacts array contains these hContacts
#define MS_VARS_GETCONTACTFROMSTRING	"Vars/GetContactFromString"

#define AIF_DONTPARSE		0x01	// don't parse the result of this function
#define AIF_FALSE			0x02	// (logical) false
//#define AIF_ERR_ARGS		0x04	// invalid arguments

typedef struct {
	int cbSize;			// in
	FORMATINFO *fi;		// in
	unsigned int argc;	// in
	char **argv;		// in  (argv[0] will be the tokenstring)
	int flags;			// out (AIF_*)
} ARGUMENTSINFO;

#define TR_MEM_VARIABLES	1		// the memory will be allocated in Variable's memory space (not implemented)
#define TR_MEM_MIRANDA		2		// the memory will be allocated in Miranda's memory space, 
									// if VRF_FREEMEM is set, the memory will be freed by Variables.
#define TR_MEM_OWNER		3		// the memory is in the caller's memory space, Variables won't touch it,
									// even if VRF_FREEMEM is set

#define TRF_FREEMEM			0x01	// free the memory if possible
#define TRF_CALLSVC			0x02	// cal szCleanupService when Variables doesn't need the result anymore
#define TRF_UNPARSEDARGS	0x04	// give the arguments unparsed (if available)
#define TRF_FIELD			0x08	// the token can be used as a %field%
#define TRF_FUNCTION		0x10	// the token can be used as a ?function()

typedef struct {
	int cbSize;
	char *szTokenString;	// non-built-in variable WITHOUT '%'
	char *szService;		// service to call, must return a 0 terminating string or NULL on error, will be called
							// with wparam = 0 and lparam = ARGUMENTSINFO *ai
	char *szCleanupService;	// only if flag VRF_CALLSVC is set, will be called when variable copied the result 
							// in it's own memory. wParam = 0, lParam = result from szService call.
	char *szHelpText;		// shown in help dialog, maybe NULL or in format 
							// "subject\targuments\tdescription" ("math\t(x, y ,...)\tx + y + ...") or 
							// "subject\tdescription" ("miranda\tpath to the Miranda-IM executable")
							// subject and description are translated automatically
	int memType;			// set to VR_MEM_* if you use the flag VRF_FREEMEM
	int flags;				// one of VRF_*
} TOKENREGISTER;

// wparam = 0
// lparam = (LPARAM)&TOKENREGISTER
// returns 0 on success
#define MS_VARS_REGISTERTOKEN		"Vars/RegisterToken"

// wparam = (void *)pnt
// lparam = 0
// free the memory from variables' memory space pointed by pnt
#define MS_VARS_FREEMEMORY				"Vars/FreeMemory"

/*
Returns copy of src if Variables is not installed, or a copy of the parsed string if it is installed.
If the returned value is not NULL, it must be free using your own free().
*/
__inline static char *variables_parsedup(char *src, char *extra, HANDLE hContact) {

	FORMATINFO fi;
	char *parsed, *res;

	if (!ServiceExists(MS_VARS_FORMATSTRING)) {
		return _strdup(src);
	}
	res = NULL;
	ZeroMemory(&fi, sizeof(fi));
	fi.cbSize = sizeof(fi);
	fi.szFormat = src;
	fi.szSource = extra;
	fi.hContact = hContact;
	parsed = (char *)CallService(MS_VARS_FORMATSTRING, (WPARAM)&fi, 0);
	if (parsed != NULL) {
		res = _strdup(parsed);
		CallService(MS_VARS_FREEMEMORY, (WPARAM)parsed, 0);
	}
	return res;
}

// Returns Variable's RTL/CRT function poiners to malloc() free() realloc()
// wParam=0, lParam = (LPARAM) &MM_INTERFACE (see m_system.h)
// copied from Miranda's core (miranda.c)
#define MS_VARS_GET_MMI			"Vars/GetMMI"

__inline static void variables_free(void *ptr) {

	if (ptr) {
		struct MM_INTERFACE mm;
		mm.cbSize=sizeof(struct MM_INTERFACE);
		CallService(MS_VARS_GET_MMI,0,(LPARAM)&mm);
		mm.mmi_free(ptr);
	}
}

// wparam = (HWND)hwnd (may be NULL)
// lparam = (char *)string (may be NULL)
// when [ok] is pressed in the help box, hwnd's text will be set to the text in the help box.
// the text of hwnd is set as initial text if string is NULL
// string is set as initial value of the text box and EN_CHANGE will be send
// returns the handle to the help dialog. Only one can be opened at a time.
#define MS_VARS_SHOWHELP		"Vars/ShowHelp"


#endif