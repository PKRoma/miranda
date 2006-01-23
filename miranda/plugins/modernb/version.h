#define FILE_VERSION	0, 4, 1, 23
#define PRODUCT_VERSION	0, 4, 1, 23

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, UNICODE build 23"
	#define PRODUCT_VERSION_STR	"0, 4, 1, UNICODE build 23"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 23"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 23"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, build 23"
	#define PRODUCT_VERSION_STR	"0, 4, 1, build 23"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG build 23"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG build 23"
#endif
#endif
