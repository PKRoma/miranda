
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the P_STATUSNOTIFY_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// P_STATUSNOTIFY_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef P_STATUSNOTIFY_EXPORTS
#define P_STATUSNOTIFY_API __declspec(dllexport)
#else
#define P_STATUSNOTIFY_API __declspec(dllimport)
#endif

// This class is exported from the P_StatusNotify.dll
class P_STATUSNOTIFY_API CP_StatusNotify {
public:
	CP_StatusNotify(void);
	// TODO: add your methods here.
};

extern P_STATUSNOTIFY_API int nP_StatusNotify;

P_STATUSNOTIFY_API int fnP_StatusNotify(void);

