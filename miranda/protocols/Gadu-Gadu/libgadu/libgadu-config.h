/* Generated automatically by configure. Do not edit. */
/* Local libgadu configuration. */

#ifndef GG_CONFIG_CONFIG_H
#define GG_CONFIG_CONFIG_H

/* Defined if libgadu was compiled for bigendian machine. */
#undef GG_CONFIG_BIGENDIAN

/* Defined if libgadu was compiled and linked with pthread support. */
#undef GG_CONFIG_HAVE_PTHREAD

/* Defined if this machine has C99-compiliant vsnprintf(). */
#undef GG_CONFIG_HAVE_C99_VSNPRINTF

/* MSC have no va_copy */
#ifdef _MSC_VER
#define _USE_32BIT_TIME_T
#define strcasecmp  stricmp
#else

/* MingW has them but not Visual C++ */
/* Defined if this machine has va_copy(). */
#define GG_CONFIG_HAVE_VA_COPY
/* Defined if this machine has __va_copy(). */
#define GG_CONFIG_HAVE___VA_COPY

#endif

/* Defined if this machine supports long long. */
/* Visual C++ 6.0 has no long long */
#if !defined(_MSC_VER) || (_MSC_VER >= 1300)
#define GG_CONFIG_HAVE_LONG_LONG
#endif

/* Defined if libgadu was compiled and linked with TLS support. */
#define GG_CONFIG_HAVE_OPENSSL

/* Defined if libgadu should be compatible with Miranda win32 sockets. */
#define GG_CONFIG_MIRANDA

/* Include file containing uintXX_t declarations. */
#include <stdint.h>

#endif /* GG_CONFIG_CONFIG_H */
