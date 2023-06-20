#ifndef OS_H
#define OS_H

#define OS_WINDOWS 1
#define OS_UNKNOWN 2

#ifdef WIN32
#define OS OS_WINDOWS
#else
#define OS OS_UNKNOWN
#endif

#endif
