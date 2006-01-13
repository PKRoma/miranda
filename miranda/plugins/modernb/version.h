#define FILE_VERSION	0, 4, 1, 19
#define PRODUCT_VERSION	0, 4, 1, 19

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, UNICODE build 19"
	#define PRODUCT_VERSION_STR	"0, 4, 1, UNICODE build 19"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 19"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 19"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, build 19"
	#define PRODUCT_VERSION_STR	"0, 4, 1, build 19"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG build 19"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG build 19"
#endif
#endif
