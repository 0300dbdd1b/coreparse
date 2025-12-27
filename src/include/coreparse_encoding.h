#ifndef COREPARSE_ENCODING_H
#define COREPARSE_ENCODING_H

#define CTB_PLATFORM_NOPREFIX
#include "ctb_platform.h"
#include "ctb_types.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

/* =========================================================================
   1. Standard LEB128 (Google Protobuf style)
   ========================================================================= */

static inline u64 varint_encode(u32 value, u8 *output)
{
    u64 i = 0;
    while (value >= 0x80)
    {
        output[i++] = (u8)((value & 0x7F) | 0x80);
        value >>= 7;
    }
    output[i++] = (u8)value;
    return i;
}

static inline u32 varint_decode(u8 **pp, const u8 *end)
{
    u8 *p = *pp;
    u32 result = 0;
    u32 shift = 0;

    while (p < end)
    {
        u8 byte = *p++;
        result |= (u32)(byte & 0x7F) << shift;

        if ((byte & 0x80) == 0)
        {
            *pp = p;
            return result;
        }

        shift += 7;
    }

    /* Buffer overrun */
    *pp = (u8*)end; 
    return 0;
}

/* =========================================================================
   2. Bitcoin "CompactSize"
   Used for: Transaction inputs/outputs count, P2P messages.
   ========================================================================= */

static inline u64 compact_size_encode(u64 value, u8 *output)
{
    if (value < 0xFD)
    {
        output[0] = (u8)value;
        return 1;
    } 
    else if (value <= 0xFFFF)
    {
        output[0] = 0xFD;
        output[1] = (u8)(value);
        output[2] = (u8)(value >> 8);
        return 3;
    } 
    else if (value <= 0xFFFFFFFF)
    {
        output[0] = 0xFE;
        output[1] = (u8)(value);
        output[2] = (u8)(value >> 8);
        output[3] = (u8)(value >> 16);
        output[4] = (u8)(value >> 24);
        return 5;
    } 
    else
    {
        output[0] = 0xFF;
        output[1] = (u8)(value);
        output[2] = (u8)(value >> 8);
        output[3] = (u8)(value >> 16);
        output[4] = (u8)(value >> 24);
        output[5] = (u8)(value >> 32);
        output[6] = (u8)(value >> 40);
        output[7] = (u8)(value >> 48);
        output[8] = (u8)(value >> 56);
        return 9;
    }
}

/* * OPTIMIZED: Uses branch prediction hints for the 1-byte hot path.
 */
static inline u64 compact_size_decode(u8 **pp, const u8 *end)
{
    u8 *p = *pp;

    if (UNLIKELY(p >= end)) return 0;

    u8 first = *p++;

    if (LIKELY(first < 0xFD))
    {
        *pp = p;
        return first;
    } 
    else if (first == 0xFD)
    {
        if (UNLIKELY((size_t)(end - p) < 2))
        {
            *pp = (u8*)end;
            return 0;
        }
        u64 val = (u64)p[0] | ((u64)p[1] << 8);
        *pp = p + 2;
        return val;
    } 
    else if (first == 0xFE)
    {
        if (UNLIKELY((size_t)(end - p) < 4))
        {
            *pp = (u8*)end;
            return 0;
        }
        u64 val = (u64)p[0] | ((u64)p[1] << 8) | ((u64)p[2] << 16) | ((u64)p[3] << 24);
        *pp = p + 4;
        return val;
    } 
    else
    { 
        /* 0xFF */
        if (UNLIKELY((size_t)(end - p) < 8))
        {
            *pp = (u8*)end;
            return 0;
        }
        u64 val = (u64)p[0] | ((u64)p[1] << 8)  | ((u64)p[2] << 16) | ((u64)p[3] << 24) |
                  ((u64)p[4] << 32) | ((u64)p[5] << 40) | ((u64)p[6] << 48) | ((u64)p[7] << 56);
        *pp = p + 8;
        return val;
    }
}

/* =========================================================================
   3. Bitcoin "VarInt" (Base-128)
   Used for: Block file offsets (blk*.dat).
   ========================================================================= */

static inline u64 varint128_encode(u64 value, u8 *output)
{
    u8 tmp[10];
    int len = 0;
    int i;

    tmp[len++] = (value & 0x7F);
    value >>= 7;

    while (value)
    {
        value--; 
        tmp[len++] = (u8)((value & 0x7F) | 0x80);
        value >>= 7;
    }

    for (i = 0; i < len; i++)
    {
        output[i] = tmp[len - i - 1];
    }
    return len;
}

static inline u64 varint128_decode(u8 **pp, const u8 *end)
{
    u8 *p = *pp;
    u64 n = 0;

    while (p < end)
    {
        u8 byte = *p++;

        n = (n << 7) | (byte & 0x7F);

        if (byte & 0x80)
        {
            n++; 
        }
        else
        {
            *pp = p;
            return n;
        }
    }

    *pp = (u8*)end;
    return 0;
}

/* =========================================================================
   4. Helpers
   ========================================================================= */

static inline void print_byte_string(const u8 *bytes, u64 size, FILE *output)
{
    u64 i;
    if (!bytes || !output) return;

    for (i = 0; i < size; ++i)
    {
        fprintf(output, "%02x", bytes[i]);
    }
}

static inline void convert_u32_to_uint8_array(u32 value, u8 *output_array)
{
    if (!output_array) return;
    output_array[0] = (u8)((value >> 24) & 0xFF);
    output_array[1] = (u8)((value >> 16) & 0xFF);
    output_array[2] = (u8)((value >> 8)  & 0xFF);
    output_array[3] = (u8)((value)       & 0xFF);
}

static inline u32 change_endianness_uint32(u32 value)
{
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8)  & 0x0000FF00) |
           ((value << 8)  & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}

static inline void reverse_bytes(u8 *p, u64 len)
{
    u8 temp;
    for (u64 i = 0; i < len / 2; i++)
    {
        temp = p[i];
        p[i] = p[len - 1 - i];
        p[len - 1 - i] = temp;
    }
}

#endif /* COREPARSE_ENCODING_H */
