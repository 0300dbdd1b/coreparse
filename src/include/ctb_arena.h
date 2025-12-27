#ifndef _CTB_ARENA_H
#define _CTB_ARENA_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#if defined(CTB_ARENA_STATIC)
#	define CTB_ARENA_DEC static
#	define CTB_ARENA_DEF static
#elif defined(__cplusplus)
#	define CTB_ARENA_DEC extern "C"
#	define CTB_ARENA_DEF extern "C"
#else
#	define CTB_ARENA_DEC extern
#	define CTB_ARENA_DEF
#endif

/* Flags */
enum ctb_arena_flags
{
    CTB_ARENA_FLAG_NONE             = 0,
    CTB_ARENA_FLAG_MEMSET_ON_SET    = 1u << 0, /* zero memory on arena creation */
    CTB_ARENA_FLAG_MEMSET_ON_RESET  = 1u << 1, /* zero memory on reset */
    CTB_ARENA_FLAG_USE_HEADER       = 1u << 2  /* store size header (required for realloc) */
};

typedef struct ctb_arena ctb_arena;

struct ctb_arena
{
    uint32_t        flags;
    ctb_arena*      prev;
    ctb_arena*      next;
    unsigned char*  memory;   /* Points to the start of the data buffer */
    size_t          capacity; /* Total capacity of this block */
    size_t          offset;   /* Current allocation offset */
};

/* Public API */
CTB_ARENA_DEC void*         ctb_arena_alloc_aligned(ctb_arena* arena, size_t size, size_t alignment);
CTB_ARENA_DEC void*         ctb_arena_alloc(ctb_arena* arena, size_t size);
CTB_ARENA_DEC void*         ctb_arena_realloc(ctb_arena* arena, void* old_ptr, size_t new_size);
CTB_ARENA_DEC ctb_arena*    ctb_arena_create(size_t capacity);
CTB_ARENA_DEC ctb_arena*    ctb_arena_extend(ctb_arena* arena, size_t capacity);
CTB_ARENA_DEC ctb_arena*    ctb_arena_reset(ctb_arena* arena);
CTB_ARENA_DEC void          ctb_arena_destroy(ctb_arena* arena);
CTB_ARENA_DEC ctb_arena*    ctb_arena_copy(const ctb_arena* source);
CTB_ARENA_DEC char*         ctb_arena_strdup(ctb_arena* arena, const char* cstr);

#ifdef CTB_ARENA_NOPREFIX
#define arena_alloc_aligned     ctb_arena_alloc_aligned
#define arena_alloc             ctb_arena_alloc
#define arena_realloc           ctb_arena_realloc
#define arena_create            ctb_arena_create
#define arena_extend            ctb_arena_extend
#define arena_reset             ctb_arena_reset
#define arena_destroy           ctb_arena_destroy
#define arena_copy              ctb_arena_copy
#define arena_strdup            ctb_arena_strdup
#endif

#endif /* _CTB_ARENA_H */

/* ============================================================================================== */
/* IMPLEMENTATION                                                                                 */
/* ============================================================================================== */

#ifdef CTB_ARENA_IMPLEMENTATION

#ifndef CTB_ARENA_DEFAULT_ALIGNMENT
#define CTB_ARENA_DEFAULT_ALIGNMENT (sizeof(void*))
#endif

#ifndef CTB_ARENA_DEFAULT_FLAGS
#define CTB_ARENA_DEFAULT_FLAGS (CTB_ARENA_FLAG_USE_HEADER | CTB_ARENA_FLAG_MEMSET_ON_SET | CTB_ARENA_FLAG_MEMSET_ON_RESET)
#endif

#if defined(CTB_ARENA_DEBUG)
#define CTB_ARENA_DLOG(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define CTB_ARENA_DLOG(...) ((void)0)
#endif

typedef struct
{
    size_t size;
} _ctb_arena_allocation_header;


static inline int _ctb_arena_is_pow2(size_t a) 
{
    return a && ((a & (a - 1u)) == 0u);
}

static inline uintptr_t _ctb_arena_align_forward(uintptr_t ptr, size_t align)
{
    if (!_ctb_arena_is_pow2(align))
    {
        size_t r = ptr % align;
        return r ? (ptr + (align - r)) : ptr;
    }
    uintptr_t m = align - 1;
    return (ptr + m) & ~m;
}

CTB_ARENA_DEF ctb_arena* ctb_arena_create(size_t capacity)
{
    size_t total_size = sizeof(ctb_arena) + capacity;
    unsigned char* block = (unsigned char*)malloc(total_size);
    if (!block) return NULL;

    ctb_arena* arena = (ctb_arena*)block;
    arena->flags    = CTB_ARENA_DEFAULT_FLAGS;
    arena->prev     = NULL;
    arena->next     = NULL;
    arena->offset   = 0;
    arena->capacity = capacity;
    arena->memory   = block + sizeof(ctb_arena);

    if (arena->flags & CTB_ARENA_FLAG_MEMSET_ON_SET)
    {
        memset(arena->memory, 0, arena->capacity);
    }

    CTB_ARENA_DLOG("[arena] create cap=%zu @%p\n", capacity, (void*)arena);
    return arena;
}

CTB_ARENA_DEF ctb_arena* ctb_arena_extend(ctb_arena* arena, size_t capacity)
{
    if (!arena) return NULL;
    while (arena->next) arena = arena->next;

    ctb_arena* seg = ctb_arena_create(capacity);
    if (!seg)
    {
        CTB_ARENA_DLOG("[arena] extend failed\n");
        return NULL;
    }
    seg->flags  = arena->flags;
    arena->next = seg;
    seg->prev   = arena;

    CTB_ARENA_DLOG("[arena] extend base=%p new=%p cap=%zu\n", (void*)arena, (void*)seg, capacity);
    return seg;
}

CTB_ARENA_DEF void* ctb_arena_alloc_aligned(ctb_arena* arena, size_t size, size_t alignment)
{
    if (!arena) return NULL;
    if (alignment == 0) alignment = 1;

    size_t header_size = (arena->flags & CTB_ARENA_FLAG_USE_HEADER) ? sizeof(_ctb_arena_allocation_header) : 0;
    for (; arena; arena = arena->next)
    {
        uintptr_t base_addr = (uintptr_t)(arena->memory + arena->offset);
        uintptr_t user_addr = _ctb_arena_align_forward(base_addr + header_size, alignment);

        size_t padding = (size_t)(user_addr - base_addr);
        size_t total_required = padding + size;

        if (total_required >= size && total_required <= (arena->capacity - arena->offset))
        {
            unsigned char* user_ptr = arena->memory + arena->offset + padding;

            if (header_size)
            {
                _ctb_arena_allocation_header h;
                h.size = size;
                memcpy(user_ptr - sizeof(_ctb_arena_allocation_header), &h, sizeof(h));
            }

            arena->offset += total_required;

            CTB_ARENA_DLOG("[arena] alloc size=%zu align=%zu used=%zu/%zu @%p\n",
                           size, alignment, arena->offset, arena->capacity, (void*)user_ptr);
            return (void*)user_ptr;
        }

        if (!arena->next)
        {
            size_t grow = header_size + size + alignment;
            size_t new_cap = (grow > arena->capacity) ? grow : arena->capacity;
            if (!ctb_arena_extend(arena, new_cap)) return NULL;
        }
    }
    return NULL;
}

CTB_ARENA_DEF void* ctb_arena_alloc(ctb_arena* arena, size_t size)
{
    return ctb_arena_alloc_aligned(arena, size, CTB_ARENA_DEFAULT_ALIGNMENT);
}

CTB_ARENA_DEF void* ctb_arena_realloc(ctb_arena* arena, void* old_ptr, size_t new_size)
{
    if (!arena) return NULL;
    if (!old_ptr) return ctb_arena_alloc(arena, new_size);
    if (new_size == 0) return NULL;

    if (!(arena->flags & CTB_ARENA_FLAG_USE_HEADER))
    {
        return ctb_arena_alloc(arena, new_size);
    }

    unsigned char* raw_ptr = (unsigned char*)old_ptr;
    _ctb_arena_allocation_header hdr;
    memcpy(&hdr, raw_ptr - sizeof(_ctb_arena_allocation_header), sizeof(hdr));

    size_t copy_n = (hdr.size < new_size) ? hdr.size : new_size;
    void* new_ptr = ctb_arena_alloc(arena, new_size);

    if (new_ptr)
    {
        memcpy(new_ptr, old_ptr, copy_n);
        if (arena->flags & CTB_ARENA_FLAG_MEMSET_ON_RESET)
        {
            memset(raw_ptr - sizeof(hdr), 0, hdr.size + sizeof(hdr));
        }
    }
    return new_ptr;
}

CTB_ARENA_DEF ctb_arena* ctb_arena_reset(ctb_arena* arena)
{
    if (!arena) return NULL;
    while (arena->prev) arena = arena->prev;

    ctb_arena* it = arena;
    for (; it; it = it->next)
    {
        if (it->flags & CTB_ARENA_FLAG_MEMSET_ON_RESET)
        {
            memset(it->memory, 0, it->capacity);
        }
        it->offset = 0;
    }
    return arena;
}

CTB_ARENA_DEF void ctb_arena_destroy(ctb_arena* arena)
{
    if (!arena) return;
    while (arena->prev) arena = arena->prev;

    ctb_arena* it = arena;
    while (it)
    {
        ctb_arena* next = it->next;
        CTB_ARENA_DLOG("[arena] destroy @%p\n", (void*)it);
        free(it); 
        it = next;
    }
}

CTB_ARENA_DEF ctb_arena* ctb_arena_copy(const ctb_arena* source)
{
    if (!source) return NULL;
    while (source->prev) source = source->prev;

    ctb_arena* head = ctb_arena_create(source->capacity);
    if (!head) return NULL;

    head->flags = source->flags;
    head->offset = source->offset;
    memcpy(head->memory, source->memory, source->offset);

    const ctb_arena* s = source->next;
    ctb_arena* d = head;

    while (s)
    {
        ctb_arena* seg = ctb_arena_create(s->capacity);
        if (!seg)
        {
            ctb_arena_destroy(head);
            return NULL; 
        }
        seg->flags = s->flags;
        seg->offset = s->offset;
        memcpy(seg->memory, s->memory, s->offset);

        d->next = seg; 
        seg->prev = d;
        d = seg; 
        s = s->next;
    }
    CTB_ARENA_DLOG("[arena] copy done\n");
    return head;
}

CTB_ARENA_DEF char* ctb_arena_strdup(ctb_arena* arena, const char* cstr)
{
    if (!arena || !cstr) return NULL;
    size_t len = strlen(cstr);
    char* out = (char*)ctb_arena_alloc(arena, len + 1);
    if (out)
    {
        memcpy(out, cstr, len);
        out[len] = '\0';
    }
    return out;
}

#endif /* CTB_ARENA_IMPLEMENTATION */
