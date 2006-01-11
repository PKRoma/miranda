#define FILE_VERSION	0, 4, 1, 14
#define PRODUCT_VERSION	0, 4, 1, 14

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, UNICODE build 14"
	#define PRODUCT_VERSION_STR	"0, 4, 1, UNICODE build 14"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 14"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 14"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, build 14"
	#define PRODUCT_VERSION_STR	"0, 4, 1, build 14"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG build 14"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG build 14"
#endif
#endif
