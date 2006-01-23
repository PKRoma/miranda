#define FILE_VERSION	0, 4, 1, 24
#define PRODUCT_VERSION	0, 4, 1, 24

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, UNICODE build 24"
	#define PRODUCT_VERSION_STR	"0, 4, 1, UNICODE build 24"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 24"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 24"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, build 24"
	#define PRODUCT_VERSION_STR	"0, 4, 1, build 24"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG build 24"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG build 24"
#endif
#endif
