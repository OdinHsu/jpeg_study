#ifndef MM_TYPE_H
#define MM_TYPE_H

#if (defined WIN32 || defined WIN64)
#include <windows.h>
#else
#include <inttypes.h>
typedef unsigned char BYTE;
typedef unsigned short int WORD;
typedef unsigned int DWORD;
typedef int64_t  INT64;
typedef uint64_t  UINT64;
typedef unsigned char UCHAR;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
#define BOOL bool
#endif

typedef signed char  SBYTE;
typedef signed short SWORD;
typedef signed int   SDWORD;
typedef signed short INT16;
typedef unsigned short UINT16;
typedef signed int   INT32;
typedef unsigned int UINT32;


#endif
