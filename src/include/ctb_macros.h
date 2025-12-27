#ifndef _CTB_MACROS_H
#define _CTB_MACROS_H


#define CTB_KiB	(1024ULL)
#define CTB_MiB	(1024ULL * CTB_KiB)
#define CTB_GiB	(1024ULL * CTB_MiB)
#define CTB_TiB	(1024ULL * CTB_GiB)
#define CTB_PiB	(1024ULL * CTB_TiB)

#define CTB_KB	(1000ULL)
#define CTB_MB	(1000ULL * CTB_KB)
#define CTB_GB	(1000ULL * CTB_MB)
#define CTB_TB	(1000ULL * CTB_GB)
#define CTB_PB	(1000ULL * CTB_TB)


#define CTB_BIT(n)				(1ULL << (n))
#define CTB_SET_BIT(v, bit)		((v) |= (1ULL << (bit)))
#define CTB_GET_BIT(v, bit)		((v) >> (bit)) & 1ULL)
#define CTB_CLEAR_BIT(v, bit)	((v) &= ~(1ULL << (bit)))
#define CTB_TOGGLE_BIT(v, bit)	((v) ^= (1ULL << (bit)))
#define CTB_IS_BIT_SET(v, bit)	(!!(v) & (1ULL << (bit)))

#define CTB_AND			&&
#define CTB_OR			||
#define CTB_NOT			!
#define CTB_NOTEQUALS	!=
#define CTB_EQUALS		==
#define CTB_IS			=

#define CTB_FOREVER		for (;;)
#define CTB_COUNTOF(arr) (sizeof(arr) / sizeof((arr)[0]))
#include <stddef.h>
#define CTB_CONTAINER_OF(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#define CTB_STRINGIFY_X(x) #x
#define CTB_STRINGIFY(x) CTB_STRINGIFY_X(x)
#define CTB_CONCAT_X(a, b) a##b
#define CTB_CONCAT(a, b) CTB_CONCAT_X(a, b)


#ifdef CTB_MACROS_NOPREFIX
	#define KiB		CTB_KiB
	#define MiB		CTB_MiB
	#define GiB		CTB_GiB
	#define TiB		CTB_TiB
	#define PiB		CTB_PiB

	#define KB		CTB_KB
	#define MB		CTB_MB
	#define GB		CTB_GB
	#define TB		CTB_TB
	#define PB		CTB_PB

	#define BIT			CTB_BIT
	#define SET_BIT		CTB_SET_BIT
	#define GET_BIT		CTB_GET_BIT
	#define CLEAR_BIT	CTB_CLEAR_BIT
	#define TOGGLE_BIT	CTB_TOGGLE_BIT
	#define IS_BIT_SET	CTB_IS_BIT_SET

	#define AND				CTB_AND
	#define OR				CTB_OR
	#define NOT				CTB_NOT
	#define NOTEQUALS		CTB_NOTEQUALS
	#define EQUALS			CTB_EQUALS
	#define IS				CTB_IS
	#define COUNTOF			CTB_COUNTOF
	#define CONTAINER_OF	CTB_CONTAINER_OF
	#define CONCAT			CTB_CONCAT
	#define STRINGIFY		CTB_STRINGIFY

#endif


#endif // !_CTB_MACROS_H
