#define FILE_VERSION	0, 4, 1, 6
#define PRODUCT_VERSION	0, 4, 1, 6

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, UNICODE build 6"
	#define PRODUCT_VERSION_STR	"0, 4, 1, UNICODE build 6"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 6"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 6"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, build 6"
	#define PRODUCT_VERSION_STR	"0, 4, 1, build 6"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG build 6"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG build 6"
#endif
#endif
