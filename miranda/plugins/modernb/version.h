#define FILE_VERSION	0, 4, 1, 17
#define PRODUCT_VERSION	0, 4, 1, 17

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, UNICODE build 17"
	#define PRODUCT_VERSION_STR	"0, 4, 1, UNICODE build 17"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 17"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 17"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, build 17"
	#define PRODUCT_VERSION_STR	"0, 4, 1, build 17"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG build 17"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG build 17"
#endif
#endif
