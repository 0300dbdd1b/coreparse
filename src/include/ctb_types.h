#ifndef _CTB_TYPES_H
#define _CTB_TYPES_H

#ifndef __has_include
	#define __has_include(x) 0
#endif

#if __has_include(<stdint.h>)
	#include <stdint.h>
	typedef	int8_t		i8;
	typedef	int16_t		i16;
	typedef	int32_t		i32;
	typedef	int64_t		i64;

	typedef	uint8_t		u8;
	typedef	uint16_t	u16;
	typedef	uint32_t	u32;
	typedef	uint64_t	u64;
#else
	typedef	signed char			i8;
	typedef	signed short		i16;
	typedef	signed int			i32;
	typedef	signed long long	i64;

	typedef	unsigned char		u8;
	typedef	unsigned short		u16;
	typedef	unsigned int		u32;
	typedef	unsigned long long	u64;
#endif

typedef	float				f32;
typedef	double				f64;

#define _CTB_VEC2_BODY(T) struct {T x, y;};			T v[2];
#define _CTB_VEC3_BODY(T) struct {T x, y, z;};		T v[3];
#define _CTB_VEC4_BODY(T) struct {T x, y, z, w;};	T v[4];

#define _CTB_GENERATE_VECTOR_TYPE(T)						\
	typedef union vec2##T { _CTB_VEC2_BODY(T) } vec2##T;	\
	typedef union vec3##T { _CTB_VEC3_BODY(T) } vec3##T;	\
	typedef union vec4##T { _CTB_VEC4_BODY(T) } vec4##T

_CTB_GENERATE_VECTOR_TYPE(i8);
_CTB_GENERATE_VECTOR_TYPE(i16);
_CTB_GENERATE_VECTOR_TYPE(i32);
_CTB_GENERATE_VECTOR_TYPE(i64);
_CTB_GENERATE_VECTOR_TYPE(u8);
_CTB_GENERATE_VECTOR_TYPE(u16);
_CTB_GENERATE_VECTOR_TYPE(u32);
_CTB_GENERATE_VECTOR_TYPE(u64);
_CTB_GENERATE_VECTOR_TYPE(f32);
_CTB_GENERATE_VECTOR_TYPE(f64);

#undef _CTB_VEC2_BODY
#undef _CTB_VEC3_BODY
#undef _CTB_VEC4_BODY
#undef _CTB_GENERATE_VECTOR_TYPE

#endif
