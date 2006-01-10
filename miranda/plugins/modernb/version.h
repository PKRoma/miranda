#define FILE_VERSION	0, 4, 1, 10
#define PRODUCT_VERSION	0, 4, 1, 10

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, UNICODE build 10"
	#define PRODUCT_VERSION_STR	"0, 4, 1, UNICODE build 10"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 10"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 10"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, build 10"
	#define PRODUCT_VERSION_STR	"0, 4, 1, build 10"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG build 10"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG build 10"
#endif
#endif
