#define FILE_VERSION	0, 4, 1, 15
#define PRODUCT_VERSION	0, 4, 1, 15

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, UNICODE build 15"
	#define PRODUCT_VERSION_STR	"0, 4, 1, UNICODE build 15"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 15"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 15"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, build 15"
	#define PRODUCT_VERSION_STR	"0, 4, 1, build 15"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG build 15"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG build 15"
#endif
#endif
