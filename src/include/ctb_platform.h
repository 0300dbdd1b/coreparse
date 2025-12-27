#ifndef _CTB_PLATFORM_H
#define _CTB_PLATFORM_H

#if defined(_WIN32) || defined(_WIN64)
	#define CTB_OS_WINDOWS 1
#elif defined(__APPLE__) || defined(__MACH__)
	#define CTB_OS_MACOS   1
	#define CTB_OS_UNIX    1
#elif defined(__ANDROID__)
	#define CTB_OS_ANDROID 1
	#define CTB_OS_LINUX   1
	#define CTB_OS_UNIX    1
#elif defined(__linux__)
	#define CTB_OS_LINUX   1
	#define CTB_OS_UNIX    1
#elif defined(__unix__) || defined(__unix)
	#define CTB_OS_UNIX    1
#else
	#define CTB_OS_UNKNOWN 1
#endif

#ifndef CTB_OS_WINDOWS
	#define CTB_OS_WINDOWS 0
#endif
#ifndef CTB_OS_MACOS
	#define CTB_OS_MACOS 0
#endif
#ifndef CTB_OS_LINUX
	#define CTB_OS_LINUX 0
#endif
#ifndef CTB_OS_UNIX
	#define CTB_OS_UNIX 0
#endif
#ifndef CTB_OS_ANDROID
	#define CTB_OS_ANDROID 0
#endif

#if defined(__clang__)
	#define CTB_COMPILER_CLANG 1
#elif defined(__GNUC__)
	#define CTB_COMPILER_GCC   1
#elif defined(_MSC_VER)
	#define CTB_COMPILER_MSVC  1
#else
	#define CTB_COMPILER_UNKNOWN 1
#endif

#ifndef CTB_COMPILER_CLANG
	#define CTB_COMPILER_CLANG 0
#endif
#ifndef CTB_COMPILER_GCC
	#define CTB_COMPILER_GCC 0
#endif
#ifndef CTB_COMPILER_MSVC
	#define CTB_COMPILER_MSVC 0
#endif


#if defined(__x86_64__) || defined(_M_X64)
	#define CTB_ARCH_X64 1
#elif defined(__i386__) || defined(_M_IX86)
	#define CTB_ARCH_X86 1
#elif defined(__aarch64__) || defined(_M_ARM64)
	#define CTB_ARCH_ARM64 1
#elif defined(__arm__) || defined(_M_ARM)
	#define CTB_ARCH_ARM32 1
#else
	#define CTB_ARCH_UNKNOWN 1
#endif

#if defined(__STDC_VERSION__)
	#define CTB_C_VERSION __STDC_VERSION__
#elif defined(__cplusplus)
	#define CTB_C_VERSION 0
#elif defined(_MSC_VER)
	#define CTB_C_VERSION 199001L
#else
	#define CTB_C_VERSION 199001L
#endif

#define CTB_C89 199001L
#define CTB_C99 199901L
#define CTB_C11 201112L
#define CTB_C17 201710L
#define CTB_C23 202311L


#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    #define CTB_LITTLE_ENDIAN 1
    #define CTB_BIG_ENDIAN 0
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    #define CTB_LITTLE_ENDIAN 0
    #define CTB_BIG_ENDIAN 1
#elif CTB_OS_WINDOWS
    #define CTB_LITTLE_ENDIAN 1
    #define CTB_BIG_ENDIAN 0
#else
    #define CTB_LITTLE_ENDIAN 0
    #define CTB_BIG_ENDIAN 0
#endif

#if defined(__has_include)
	#define CTB_HAS_INCLUDE(x)	__has_include(x)
#else
	#define CTB_HAS_INCLUDE(x)	0
#endif

#if defined(__has_attribute)
	#define CTB_HAS_ATTRIBUTE(x)	__has_attribute(x)
#else
	#define CTB_HAS_ATTRIBUTE(x)	0
#endif

#if defined(__has_builtin)
	#define CTB_HAS_BUILTIN(x)	__has_builtin(x)
#else
	#define CTB_HAS_BUILTIN(x)	0
#endif

#if defined(__has_feature)
	#define CTB_HAS_FEATURE(x)	__has_feature(x)
#else
	#define CTB_HAS_FEATURE(x)	0
#endif


/* ==========================================================================
 * 1. FUNCTION ATTRIBUTES
 * ========================================================================== */

/* Inlining */
#if CTB_COMPILER_MSVC
	#define CTB_INLINE			__inline
	#define CTB_FORCE_INLINE	__forceinline
	#define CTB_NOINLINE		__declspec(noinline)
    #define CTB_GNU_INLINE
#elif CTB_COMPILER_GCC || CTB_COMPILER_CLANG
	#define CTB_INLINE			inline
	#define CTB_FORCE_INLINE	inline __attribute__((always_inline))
	#define CTB_NOINLINE		__attribute__((noinline))
    #define CTB_GNU_INLINE      __attribute__((gnu_inline))
#else
	#define CTB_INLINE			inline
	#define CTB_FORCE_INLINE	inline
	#define CTB_NOINLINE
    #define CTB_GNU_INLINE
#endif

/* No Return & Unreachable */
#if CTB_COMPILER_MSVC
	#define CTB_NORETURN		__declspec(noreturn)
	#define CTB_UNREACHABLE()	__assume(0)
    #define CTB_RETURNS_TWICE
#elif CTB_COMPILER_GCC || CTB_COMPILER_CLANG
	#define CTB_NORETURN		__attribute__((noreturn))
	#define CTB_UNREACHABLE()	__builtin_unreachable()
    #define CTB_RETURNS_TWICE   __attribute__((returns_twice))
#elif CTB_C_VERSION >= CTB_C11
	#define CTB_NORETURN		_Noreturn
	#define CTB_UNREACHABLE()
    #define CTB_RETURNS_TWICE
#else
	#define CTB_NORETURN
	#define CTB_UNREACHABLE()
    #define CTB_RETURNS_TWICE
#endif

/* Optimization Hints (Function Level) */
#if CTB_COMPILER_GCC || CTB_COMPILER_CLANG
	#define CTB_PURE			__attribute__((pure))
	#define CTB_CONST_FN		__attribute__((const))
	#define CTB_HOT				__attribute__((hot))
	#define CTB_COLD			__attribute__((cold))
	#define CTB_FLATTEN			__attribute__((flatten))
    #define CTB_LEAF            __attribute__((leaf))
    #define CTB_NOCLONE         __attribute__((noclone))
    #define CTB_OPTIMIZE(lvl)   __attribute__((optimize(lvl)))
    #define CTB_ARTIFICIAL		__attribute__((artificial))
    #define CTB_ERROR_ATTR(msg) __attribute__((error(msg)))
    #define CTB_WARNING_ATTR(msg) __attribute__((warning(msg)))
#elif CTB_COMPILER_MSVC
	#define CTB_PURE
	#define CTB_CONST_FN
	#define CTB_HOT
	#define CTB_COLD
	#define CTB_FLATTEN
    #define CTB_LEAF
    #define CTB_NOCLONE
    #define CTB_OPTIMIZE(lvl)
    #define CTB_ARTIFICIAL
    #define CTB_ERROR_ATTR(msg)   __declspec(deprecated(msg))
    #define CTB_WARNING_ATTR(msg) __declspec(deprecated(msg))
#else
	#define CTB_PURE
	#define CTB_CONST_FN
	#define CTB_HOT
	#define CTB_COLD
	#define CTB_FLATTEN
    #define CTB_LEAF
    #define CTB_NOCLONE
    #define CTB_OPTIMIZE(lvl)
    #define CTB_ARTIFICIAL
    #define CTB_ERROR_ATTR(msg)
    #define CTB_WARNING_ATTR(msg)
#endif

/* Memory & Allocators */
#if CTB_COMPILER_GCC || CTB_COMPILER_CLANG
	#define CTB_MALLOC			__attribute__((malloc))
	#define CTB_RETURNS_NONNULL __attribute__((returns_nonnull))
	#define CTB_ALLOC_SIZE(x)	__attribute__((alloc_size(x)))
	#define CTB_ALLOC_SIZE2(x,y) __attribute__((alloc_size(x,y)))
    #define CTB_ALLOC_ALIGN(x)  __attribute__((alloc_align(x)))
    #define CTB_ASSUME_ALIGNED(ptr, align) __builtin_assume_aligned(ptr, align)
#elif CTB_COMPILER_MSVC
	#define CTB_MALLOC			__declspec(restrict)
	#define CTB_RETURNS_NONNULL
	#define CTB_ALLOC_SIZE(x)
	#define CTB_ALLOC_SIZE2(x,y)
    #define CTB_ALLOC_ALIGN(x)
    #define CTB_ASSUME_ALIGNED(ptr, align) (ptr)
#else
	#define CTB_MALLOC
	#define CTB_RETURNS_NONNULL
	#define CTB_ALLOC_SIZE(x)
	#define CTB_ALLOC_SIZE2(x,y)
    #define CTB_ALLOC_ALIGN(x)
    #define CTB_ASSUME_ALIGNED(ptr, align) (ptr)
#endif

/* Calling Conventions */
#if CTB_COMPILER_MSVC
	#define CTB_STDCALL			__stdcall
	#define CTB_CDECL			__cdecl
	#define CTB_FASTCALL		__fastcall
	#define CTB_VECTORCALL		__vectorcall
    #define CTB_NAKED           __declspec(naked)
#elif CTB_COMPILER_GCC || CTB_COMPILER_CLANG
	#define CTB_STDCALL			__attribute__((stdcall))
	#define CTB_CDECL			__attribute__((cdecl))
	#define CTB_FASTCALL		__attribute__((fastcall))
	#define CTB_VECTORCALL		__attribute__((vectorcall))
    #define CTB_NAKED           __attribute__((naked))
#else
	#define CTB_STDCALL
	#define CTB_CDECL
	#define CTB_FASTCALL
	#define CTB_VECTORCALL
    #define CTB_NAKED
#endif

/* Safety & Checks */
#if CTB_COMPILER_GCC || CTB_COMPILER_CLANG
	#define CTB_FORMAT(archetype, fmt, first) __attribute__((format(archetype, fmt, first)))
    #define CTB_FORMAT_ARG(idx) __attribute__((format_arg(idx)))
	#define CTB_NONNULL(...)    __attribute__((nonnull(__VA_ARGS__)))
	#define CTB_NONNULL_ALL     __attribute__((nonnull))
	#define CTB_SENTINEL        __attribute__((sentinel))
    #define CTB_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
    #define CTB_ACCESS(mode, ref_index) __attribute__((access(mode, ref_index))) /*WARN: GCC 10+ */
    #define CTB_ACCESS_SIZE(mode, ref_index, size_index) __attribute__((access(mode, ref_index, size_index)))
#else
	#define CTB_FORMAT(archetype, fmt, first)
    #define CTB_FORMAT_ARG(idx)
	#define CTB_NONNULL(...)
	#define CTB_NONNULL_ALL
	#define CTB_SENTINEL
    #define CTB_WARN_UNUSED_RESULT
    #define CTB_ACCESS(mode, ref_index)
    #define CTB_ACCESS_SIZE(mode, ref_index, size_index)
#endif

/* Function Code Section */
#if CTB_COMPILER_MSVC
    #define CTB_SECTION_FUN(name) __declspec(code_seg(name))
#elif CTB_COMPILER_GCC || CTB_COMPILER_CLANG
    #define CTB_SECTION_FUN(name) __attribute__((section(name)))
#else
    #define CTB_SECTION_FUN(name)
#endif

/* Misc Function Attributes */
#if CTB_COMPILER_GCC || CTB_COMPILER_CLANG
    #define CTB_CONSTRUCTOR(fn)			__attribute__((constructor)) void fn(void)
	#define CTB_CONSTRUCTOR_PRIO(fn, p)	__attribute__((constructor(101 + p))) void fn(void)
	#define CTB_DESTRUCTOR(fn)			__attribute__((destructor)) void fn(void)
	#define CTB_DESTRUCTOR_PRIO(fn, p)	__attribute__((destructor(101 + p))) void fn(void)
    #define CTB_WEAK                    __attribute__((weak))
    #define CTB_ALIAS(symbol)           __attribute__((alias(#symbol)))
    #define CTB_WEAKREF(symbol)         __attribute__((weakref(#symbol)))
    #define CTB_INTERRUPT               __attribute__((interrupt))
    #define CTB_TARGET_CLONES(...)      __attribute__((target_clones(__VA_ARGS__)))
    #define CTB_TARGET(arch)            __attribute__((target(arch)))
    #define CTB_SIMD                    __attribute__((simd))
#elif CTB_COMPILER_MSVC
    #define CTB_CONSTRUCTOR(fn)         void fn(void)
    #define CTB_CONSTRUCTOR_PRIO(fn, p) void fn(void)
    #define CTB_DESTRUCTOR(fn)          void fn(void)
    #define CTB_DESTRUCTOR_PRIO(fn, p)  void fn(void)
    #define CTB_WEAK                    __declspec(selectany)
    #define CTB_ALIAS(symbol)
    #define CTB_WEAKREF(symbol)
    #define CTB_INTERRUPT
    #define CTB_TARGET_CLONES(...)
    #define CTB_TARGET(arch)
    #define CTB_SIMD
#else
    #define CTB_CONSTRUCTOR(fn)         void fn(void)
    #define CTB_CONSTRUCTOR_PRIO(fn, p) void fn(void)
    #define CTB_DESTRUCTOR(fn)          void fn(void)
    #define CTB_DESTRUCTOR_PRIO(fn, p)  void fn(void)
    #define CTB_WEAK
    #define CTB_ALIAS(symbol)
    #define CTB_WEAKREF(symbol)
    #define CTB_INTERRUPT
    #define CTB_TARGET_CLONES(...)
    #define CTB_TARGET(arch)
    #define CTB_SIMD
#endif


/* ==========================================================================
 * 2. VARIABLE ATTRIBUTES
 * ========================================================================== */

/* Alignment */
#if CTB_COMPILER_MSVC
	#define CTB_ALIGNED(x) __declspec(align(x))
#elif CTB_COMPILER_GCC || CTB_COMPILER_CLANG
	#define CTB_ALIGNED(x) __attribute__((aligned(x)))
#else
	#define CTB_ALIGNED(x)
#endif

/* Variable Cleanup (RAII) */
#if CTB_COMPILER_GCC || CTB_COMPILER_CLANG
	#define CTB_CLEANUP(fn) __attribute__((cleanup(fn)))
#else
	#define CTB_CLEANUP(fn)
#endif

/* Variable Section / Placement */
#if CTB_COMPILER_MSVC
	#define CTB_SECTION_VAR(name) __declspec(allocate(name))
    #define CTB_SELECTANY         __declspec(selectany)
#elif CTB_COMPILER_GCC || CTB_COMPILER_CLANG
	#define CTB_SECTION_VAR(name) __attribute__((section(name)))
    #define CTB_SELECTANY         __attribute__((weak))
#else
	#define CTB_SECTION_VAR(name)
    #define CTB_SELECTANY
#endif

/* Variable Usage & Optimization */
#if CTB_COMPILER_GCC || CTB_COMPILER_CLANG
	#define CTB_USED        __attribute__((used))
    #define CTB_UNUSED_VAR  __attribute__((unused))
    #define CTB_COMMON      __attribute__((common))
    #define CTB_NNO_COMMON  __attribute__((nocommon))
    #define CTB_TLS_MODEL(m) __attribute__((tls_model(m)))
    #define CTB_VECTOR_SIZE(bytes) __attribute__((vector_size(bytes)))
#elif CTB_COMPILER_MSVC
	#define CTB_USED
    #define CTB_UNUSED_VAR  __pragma(warning(suppress: 4100 4101))
    #define CTB_COMMON
    #define CTB_NNO_COMMON
    #define CTB_TLS_MODEL(m)
    #define CTB_VECTOR_SIZE(bytes)
#else
	#define CTB_USED
    #define CTB_UNUSED_VAR
    #define CTB_COMMON
    #define CTB_NNO_COMMON
    #define CTB_TLS_MODEL(m)
    #define CTB_VECTOR_SIZE(bytes)
#endif

/* Deprecation (Variable) */
#if CTB_COMPILER_MSVC
	#define CTB_DEPRECATED_VAR			__declspec(deprecated)
	#define CTB_DEPRECATED_MSG_VAR(m)	__declspec(deprecated(m))
#elif CTB_COMPILER_GCC || CTB_COMPILER_CLANG
	#define CTB_DEPRECATED_VAR			__attribute__((deprecated))
	#define CTB_DEPRECATED_MSG_VAR(m)	__attribute__((deprecated(m)))
#else
	#define CTB_DEPRECATED_VAR
	#define CTB_DEPRECATED_MSG_VAR(m)
#endif


/* ==========================================================================
 * 3. TYPE ATTRIBUTES (Structs, Unions, Enums, Typedefs)
 * ========================================================================== */

/* Packing */
#if CTB_COMPILER_GCC || CTB_COMPILER_CLANG
	#define CTB_PACKED_BEGIN
	#define CTB_PACKED_END      __attribute__((__packed__))
    #define CTB_ATTR_PACKED     __attribute__((packed))
#elif CTB_COMPILER_MSVC
	#define CTB_PACKED_BEGIN    __pragma(pack(push, 1))
	#define CTB_PACKED_END      __pragma(pack(pop))
    #define CTB_ATTR_PACKED
#else
	#define CTB_PACKED_BEGIN
	#define CTB_PACKED_END
    #define CTB_ATTR_PACKED
#endif

/* Type Optimization */
#if CTB_COMPILER_GCC || CTB_COMPILER_CLANG
	#define CTB_MAY_ALIAS           __attribute__((may_alias))
    #define CTB_TRANSPARENT_UNION   __attribute__((transparent_union))
    #define CTB_DESIGNATED_INIT     __attribute__((designated_init))
    #define CTB_MODE(m)             __attribute__((mode(m)))
#else
	#define CTB_MAY_ALIAS
    #define CTB_TRANSPARENT_UNION
    #define CTB_DESIGNATED_INIT
    #define CTB_MODE(m)
#endif

/* Endianness (Type) */
#if CTB_COMPILER_GCC || CTB_COMPILER_CLANG
    #define CTB_SCALAR_STORAGE_ORDER(order) __attribute__((scalar_storage_order(order)))
#else
    #define CTB_SCALAR_STORAGE_ORDER(order)
#endif

/* Deprecation (Type/Enum) */
#if CTB_COMPILER_GCC || CTB_COMPILER_CLANG
    #define CTB_DEPRECATED_TYPE __attribute__((deprecated))
#elif CTB_COMPILER_MSVC
    #define CTB_DEPRECATED_TYPE __declspec(deprecated)
#else
    #define CTB_DEPRECATED_TYPE
#endif

/* No VTable (C++ Class/Struct) */
#if CTB_COMPILER_MSVC
    #define CTB_NOVTABLE __declspec(novtable)
#else
    #define CTB_NOVTABLE
#endif


/* ==========================================================================
 * 4. LABEL & STATEMENT ATTRIBUTES
 * ========================================================================== */

/* Label Attributes (placed before label name: CTB_LABEL_HOT my_label:) */
#if CTB_COMPILER_GCC || CTB_COMPILER_CLANG
    #define CTB_LABEL_HOT       __attribute__((hot))
    #define CTB_LABEL_COLD      __attribute__((cold))
    #define CTB_LABEL_UNUSED    __attribute__((unused))
#else
    #define CTB_LABEL_HOT
    #define CTB_LABEL_COLD
    #define CTB_LABEL_UNUSED
#endif

/* Fallthrough (Statement) */
#if defined(__cplusplus) && __cplusplus >= 201703L
	#define CTB_FALLTHROUGH [[fallthrough]]
#elif CTB_C_VERSION >= CTB_C23
	#define CTB_FALLTHROUGH [[fallthrough]]
#elif CTB_COMPILER_GCC || CTB_COMPILER_CLANG
	#define CTB_FALLTHROUGH __attribute__((fallthrough))
#else
	#define CTB_FALLTHROUGH
#endif


/* ==========================================================================
 * 5. SANITIZERS & INSTRUMENTATION
 * ========================================================================== */

#if CTB_COMPILER_CLANG
    #define CTB_NO_ASAN     __attribute__((no_sanitize("address")))
    #define CTB_NO_TSAN     __attribute__((no_sanitize("thread")))
    #define CTB_NO_UBSAN    __attribute__((no_sanitize("undefined")))
#elif CTB_COMPILER_GCC
    #define CTB_NO_ASAN     __attribute__((no_sanitize_address))
    #define CTB_NO_TSAN     __attribute__((no_sanitize_thread))
    #define CTB_NO_UBSAN    __attribute__((no_sanitize_undefined))
#elif CTB_COMPILER_MSVC
    #define CTB_NO_ASAN     __declspec(no_sanitize_address)
    #define CTB_NO_TSAN 
    #define CTB_NO_UBSAN
#else
    #define CTB_NO_ASAN
    #define CTB_NO_TSAN
    #define CTB_NO_UBSAN
#endif

#if CTB_COMPILER_GCC || CTB_COMPILER_CLANG
    #define CTB_NO_INSTRUMENT       __attribute__((no_instrument_function))
    #define CTB_NO_STACK_PROTECTOR  __attribute__((no_stack_protector))
#else
    #define CTB_NO_INSTRUMENT
    #define CTB_NO_STACK_PROTECTOR
#endif


/* ==========================================================================
 * 6. LANGUAGE EXTENSIONS & KEYWORDS
 * ========================================================================== */

/* Restrict */
#if CTB_COMPILER_MSVC
	#define CTB_RESTRICT __restrict
#elif CTB_COMPILER_GCC || CTB_COMPILER_CLANG
	#define CTB_RESTRICT __restrict__
#else
	#define CTB_RESTRICT restrict
#endif

/* Thread Local Storage */
#if CTB_COMPILER_MSVC
	#define CTB_THREAD_LOCAL	__declspec(thread)
#elif CTB_C_VERSION >= CTB_C11
	#define CTB_THREAD_LOCAL	_Thread_local
#elif CTB_COMPILER_GCC || CTB_COMPILER_CLANG
	#define CTB_THREAD_LOCAL	__thread
#else
	#define CTB_THREAD_LOCAL
#endif

/* Static Assert */
#if CTB_C_VERSION >= CTB_C11
	#define CTB_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#elif defined(__cplusplus) && (__cplusplus >= 201103L)
	#define CTB_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
	#define _CTB_SA_CAT(a, b) a##b
	#define _CTB_SA_XCAT(a, b) _CTB_SA_CAT(a, b)
	#define CTB_STATIC_ASSERT(cond, msg) \
	typedef char _CTB_SA_XCAT(static_assertion_, __LINE__)[(cond)?1:-1]
#endif

/* Branch Prediction */
#if CTB_COMPILER_GCC || CTB_COMPILER_CLANG
	#define CTB_LIKELY(x)		__builtin_expect(!!(x), 1)
	#define CTB_UNLIKELY(x)		__builtin_expect(!!(x), 0)
#else
	#define CTB_LIKELY(x)		(x)
	#define CTB_UNLIKELY(x)		(x)
#endif

/* Assumptions */
#if CTB_COMPILER_MSVC
	#define CTB_ASSUME(x)		__assume(x)
#elif CTB_COMPILER_CLANG
	#define CTB_ASSUME(x)		__builtin_assume(x)
#elif CTB_COMPILER_GCC
	#define CTB_ASSUME(x)		do { if (!(x)) __builtin_unreachable(); } while(0)
#else
	#define CTB_ASSUME(x)
#endif

/* Debug Trap */
#if CTB_COMPILER_MSVC
	#define CTB_DEBUG_TRAP()	__debugbreak()
#elif CTB_COMPILER_CLANG
	#define CTB_DEBUG_TRAP()	__builtin_debugtrap()
#elif CTB_COMPILER_GCC
	#define CTB_DEBUG_TRAP()	__builtin_trap()
#else
	#define CTB_DEBUG_TRAP()	(*(volatile int *)0 = 0)
#endif

/* Nodiscard (Return Value) */
#if defined(__cplusplus) && __cplusplus >= 201703L
	#define CTB_NODISCARD   [[nodiscard]]
#elif CTB_C_VERSION >= CTB_C23
	#define CTB_NODISCARD   [[nodiscard]]
#elif CTB_COMPILER_GCC || CTB_COMPILER_CLANG
	#define CTB_NODISCARD   __attribute__((warn_unused_result))
#elif CTB_COMPILER_MSVC
	#define CTB_NODISCARD   _Check_return_
#else
	#define CTB_NODISCARD
#endif


/* ==========================================================================
 * 7. SHORT NAMES (OPTIONAL)
 * ========================================================================== */
#ifdef CTB_PLATFORM_NOPREFIX
    /* OS & Arch */
	#define OS_WINDOWS		CTB_OS_WINDOWS
	#define OS_MACOS		CTB_OS_MACOS
	#define OS_LINUX		CTB_OS_LINUX
	#define OS_UNIX			CTB_OS_UNIX
	#define OS_ANDROID		CTB_OS_ANDROID
	#define ARCH_X64		CTB_ARCH_X64
	#define ARCH_X86		CTB_ARCH_X86
	#define ARCH_ARM64		CTB_ARCH_ARM64
	#define ARCH_ARM32		CTB_ARCH_ARM32
    #define LITTLE_ENDIAN	CTB_LITTLE_ENDIAN
	#define BIG_ENDIAN		CTB_BIG_ENDIAN

    /* Feature Checks */
	#define HAS_INCLUDE		CTB_HAS_INCLUDE
	#define HAS_ATTRIBUTE	CTB_HAS_ATTRIBUTE
	#define HAS_BUILTIN		CTB_HAS_BUILTIN
	#define HAS_FEATURE		CTB_HAS_FEATURE

    /* Standard Keywords */
	#define INLINE			CTB_INLINE
	#define FORCE_INLINE	CTB_FORCE_INLINE
	#define NOINLINE		CTB_NOINLINE
    #define NORETURN		CTB_NORETURN
	#define THREAD_LOCAL	CTB_THREAD_LOCAL
	#define STATIC_ASSERT	CTB_STATIC_ASSERT
	#define RESTRICT		CTB_RESTRICT
    #define NODISCARD		CTB_NODISCARD
    #define FALLTHROUGH		CTB_FALLTHROUGH

    /* Execution Control */
	#define UNREACHABLE		CTB_UNREACHABLE
	#define DEBUG_TRAP		CTB_DEBUG_TRAP
	#define LIKELY			CTB_LIKELY
	#define UNLIKELY		CTB_UNLIKELY
	#define ASSUME			CTB_ASSUME
    #define RETURNS_TWICE   CTB_RETURNS_TWICE

    /* Calling Conventions */
	#define STDCALL			CTB_STDCALL
	#define CDECL			CTB_CDECL
	#define FASTCALL		CTB_FASTCALL
	#define VECTORCALL		CTB_VECTORCALL
    #define NAKED			CTB_NAKED

    /* Attributes: Function */
	#define PURE			CTB_PURE
	#define CONST_FN		CTB_CONST_FN
	#define HOT				CTB_HOT
	#define COLD			CTB_COLD
	#define FLATTEN			CTB_FLATTEN
    #define LEAF            CTB_LEAF
    #define NOCLONE         CTB_NOCLONE
    #define OPTIMIZE        CTB_OPTIMIZE
    #define ARTIFICIAL		CTB_ARTIFICIAL
    #define WARN_UNUSED     CTB_WARN_UNUSED_RESULT
    #define INTERRUPT       CTB_INTERRUPT
    #define TARGET_CLONES	CTB_TARGET_CLONES
    #define TARGET          CTB_TARGET
    #define SIMD            CTB_SIMD
    #define ERROR_ATTR      CTB_ERROR_ATTR
    #define WARNING_ATTR    CTB_WARNING_ATTR
    #define CONSTRUCTOR		CTB_CONSTRUCTOR
	#define CONSTRUCTOR_P	CTB_CONSTRUCTOR_PRIO
	#define DESTRUCTOR		CTB_DESTRUCTOR
	#define DESTRUCTOR_P	CTB_DESTRUCTOR_PRIO
    
    /* Attributes: Memory/Alloc */
	#define MALLOC_ATTR		CTB_MALLOC
    #define RETURNS_NONNULL	CTB_RETURNS_NONNULL
	#define ALLOC_SIZE		CTB_ALLOC_SIZE
	#define ALLOC_SIZE2		CTB_ALLOC_SIZE2
    #define ALLOC_ALIGN     CTB_ALLOC_ALIGN
    #define ASSUME_ALIGNED  CTB_ASSUME_ALIGNED

    /* Attributes: Safety/Format */
	#define FORMAT			CTB_FORMAT
    #define FORMAT_ARG      CTB_FORMAT_ARG
	#define NONNULL			CTB_NONNULL
	#define NONNULL_ALL		CTB_NONNULL_ALL
	#define SENTINEL		CTB_SENTINEL
    #define ACCESS          CTB_ACCESS
    #define ACCESS_SIZE     CTB_ACCESS_SIZE

    /* Attributes: Variable */
	#define ALIGNED			CTB_ALIGNED
	#define CLEANUP			CTB_CLEANUP
	#define SECTION_VAR     CTB_SECTION_VAR
    #define SECTION_FUN     CTB_SECTION_FUN
    #define SELECTANY		CTB_SELECTANY
	#define USED            CTB_USED
	#define UNUSED          CTB_UNUSED_VAR /* Variable specific */
    #define COMMON          CTB_COMMON
    #define NO_COMMON       CTB_NNO_COMMON
    #define TLS_MODEL       CTB_TLS_MODEL
    #define VECTOR_SIZE     CTB_VECTOR_SIZE

    /* Attributes: Type */
	#define PACKED_BEGIN	CTB_PACKED_BEGIN
	#define PACKED_END		CTB_PACKED_END
    #define ATTR_PACKED     CTB_ATTR_PACKED
	#define MAY_ALIAS		CTB_MAY_ALIAS
    #define TRANSPARENT_UNION CTB_TRANSPARENT_UNION
    #define DESIGNATED_INIT CTB_DESIGNATED_INIT
    #define MODE            CTB_MODE
    #define SCALAR_STORAGE_ORDER CTB_SCALAR_STORAGE_ORDER
    #define NOVTABLE        CTB_NOVTABLE

    /* Attributes: Label */
    #define LABEL_HOT       CTB_LABEL_HOT
    #define LABEL_COLD      CTB_LABEL_COLD
    #define LABEL_UNUSED    CTB_LABEL_UNUSED

    /* Deprecation */
	#define DEPRECATED		    CTB_DEPRECATED
    #define DEPRECATED_MSG      CTB_DEPRECATED_MSG
    #define DEPRECATED_VAR      CTB_DEPRECATED_VAR
    #define DEPRECATED_TYPE     CTB_DEPRECATED_TYPE

    /* Visibility */
	#define EXPORT			CTB_EXPORT
	#define IMPORT			CTB_IMPORT
	#define HIDDEN			CTB_HIDDEN
	#define WEAK			CTB_WEAK
	#define ALIAS			CTB_ALIAS
	#define WEAKREF			CTB_WEAKREF

    /* Sanitizers */
	#define NO_ASAN			CTB_NO_ASAN
	#define NO_TSAN			CTB_NO_TSAN
	#define NO_UBSAN		CTB_NO_UBSAN
	#define NO_INSTRUMENT   CTB_NO_INSTRUMENT
	#define NO_STACK_PROT   CTB_NO_STACK_PROTECTOR
	
    /* Helpers */

#endif

#endif /* _CTB_PLATFORM_H */
