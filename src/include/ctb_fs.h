#ifndef _CTB_FS_H
#define _CTB_FS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>

#if defined(CTB_FS_STATIC)
#   define CTB_FS_DEC static
#   define CTB_FS_DEF static
#elif defined(__cplusplus)
#   define CTB_FS_DEC extern "C"
#   define CTB_FS_DEF extern "C"
#else
#   define CTB_FS_DEC extern
#   define CTB_FS_DEF
#endif

/* Platform Detection & Macros */
#if defined(_WIN32) || defined(_WIN64)
    #define CTB_FS_PLATFORM_WINDOWS
    #define CTB_FS_PATH_SEPARATOR   '\\'
    #define _ctb_fs_stat_func       _stat
    #define _ctb_fs_stat_struct     struct _stat
    #define _ctb_fs_s_ifdir         _S_IFDIR
    #define _ctb_fs_s_ifreg         _S_IFREG
    #include <direct.h>
    #include <io.h>
#else
    #define CTB_FS_PLATFORM_POSIX
    #define CTB_FS_PATH_SEPARATOR   '/'
    #define _ctb_fs_stat_func       stat
    #define _ctb_fs_stat_struct     struct stat
    #define _ctb_fs_s_ifdir         S_IFDIR
    #define _ctb_fs_s_ifreg         S_IFREG
    #include <unistd.h>
    #include <string.h>
#endif

/* Flags & Masks */
typedef enum ctb_fs_mask
{
    CTB_FS_NONE         = 0,
    CTB_FS_FILE         = (1 << 0),
    CTB_FS_DIRECTORY    = (1 << 1),
    CTB_FS_SYMLINK      = (1 << 2), 
    CTB_FS_OTHER        = (1 << 3),

    CTB_FS_PERM_READ    = (1 << 4),
    CTB_FS_PERM_WRITE   = (1 << 5),
    CTB_FS_PERM_EXEC    = (1 << 6),
    
    CTB_FS_IS_OPEN      = (1 << 7),
    CTB_FS_OPEN_READ    = (1 << 8),
    CTB_FS_OPEN_WRITE   = (1 << 9),
    CTB_FS_OPEN_APPEND  = (1 << 10),
    CTB_FS_OPEN_TRUNC   = (1 << 11),
    CTB_FS_OPEN_BINARY  = (1 << 12)
} ctb_fs_mask;

typedef struct ctb_file
{
    char* path;
    char* basename;
    uint64_t        size;
    void* handle; /* FILE* */
    ctb_fs_mask     mask;
    int             valid;  
} ctb_file;

/* --- Main Object API (For opening/manipulating files) --- */
CTB_FS_DEC ctb_file ctb_fs_open(const char *path, ctb_fs_mask mode);
CTB_FS_DEC void     ctb_fs_close(ctb_file *file);
CTB_FS_DEC int      ctb_fs_is_open(const ctb_file *file);

/* --- Path Helpers (Use these to check strings directly!) --- */
CTB_FS_DEC int      ctb_fs_path_exists(const char *path);
CTB_FS_DEC int      ctb_fs_path_is_dir(const char *path);
CTB_FS_DEC int      ctb_fs_path_is_file(const char *path);
CTB_FS_DEC int      ctb_fs_delete(const char *path);
CTB_FS_DEC int      ctb_fs_move(const char *oldpath, const char *newpath);

/* --- Object Helpers (Use these if you already have a ctb_file) --- */
CTB_FS_DEC int      ctb_fs_is_file(const ctb_file *file);
CTB_FS_DEC int      ctb_fs_is_dir(const ctb_file *file);
CTB_FS_DEC int      ctb_fs_can_read(const ctb_file *file);
CTB_FS_DEC int      ctb_fs_can_write(const ctb_file *file);


#ifdef CTB_FS_NOPREFIX
    #define fs_open             ctb_fs_open
    #define fs_close            ctb_fs_close
    #define fs_path_exists      ctb_fs_path_exists
    #define fs_path_is_dir      ctb_fs_path_is_dir
    #define fs_path_is_file     ctb_fs_path_is_file
#endif

#endif // !_CTB_FS_H

/* ============================================================================================== */
/* IMPLEMENTATION                                                                                 */
/* ============================================================================================== */

#ifdef CTB_FS_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

/* Internal: Populates mask/size without opening a FILE* handle */
static int _ctb_fs_stat_internal(const char *path, ctb_fs_mask *mask, uint64_t *size)
{
    _ctb_fs_stat_struct s;
    if (_ctb_fs_stat_func(path, &s) != 0) return 0; // Does not exist

    if (size) *size = (uint64_t)s.st_size;
    if (mask)
    {
        *mask = CTB_FS_NONE;
        if (s.st_mode & _ctb_fs_s_ifdir) *mask |= CTB_FS_DIRECTORY;
        else if (s.st_mode & _ctb_fs_s_ifreg) *mask |= CTB_FS_FILE;
        else *mask |= CTB_FS_OTHER;

        if (s.st_mode & 0400) *mask |= CTB_FS_PERM_READ;
        if (s.st_mode & 0200) *mask |= CTB_FS_PERM_WRITE;
    }
    return 1;
}

static char* _ctb_fs_basename(char *path)
{
    if (!path) return NULL;
    char *base = strrchr(path, CTB_FS_PATH_SEPARATOR);
    return (base ? base + 1 : path);
}

static char* _ctb_fs_strdup(const char* s)
{
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* new_s = (char*)malloc(len);
    if (new_s) memcpy(new_s, s, len);
    return new_s;
}

static void _ctb_fs_mode_str(ctb_fs_mask mask, char *s)
{
    s[0] = '\0';
    if (mask & CTB_FS_OPEN_WRITE)
    {
        if (mask & CTB_FS_OPEN_READ) strcat(s, "w+");
        else if (mask & CTB_FS_OPEN_APPEND) strcat(s, "a");
        else strcat(s, "w");
    }
    else if (mask & CTB_FS_OPEN_READ) strcat(s, "r");
    if (mask & CTB_FS_OPEN_BINARY) strcat(s, "b");
}


CTB_FS_DEF int ctb_fs_path_exists(const char *path)
{
    ctb_fs_mask m; return _ctb_fs_stat_internal(path, &m, NULL);
}

CTB_FS_DEF int ctb_fs_path_is_dir(const char *path)
{
    ctb_fs_mask m;
    if (_ctb_fs_stat_internal(path, &m, NULL)) return (m & CTB_FS_DIRECTORY);
    return 0;
}

CTB_FS_DEF int ctb_fs_path_is_file(const char *path)
{
    ctb_fs_mask m;
    if (_ctb_fs_stat_internal(path, &m, NULL)) return (m & CTB_FS_FILE);
    return 0;
}

CTB_FS_DEF int ctb_fs_delete(const char *path) { return (remove(path) == 0); }
CTB_FS_DEF int ctb_fs_move(const char *old, const char *newp) { return (rename(old, newp) == 0); }

CTB_FS_DEF int ctb_fs_is_file(const ctb_file *f) { return (f && f->valid && (f->mask & CTB_FS_FILE)); }
CTB_FS_DEF int ctb_fs_is_dir(const ctb_file *f)  { return (f && f->valid && (f->mask & CTB_FS_DIRECTORY)); }
CTB_FS_DEF int ctb_fs_is_open(const ctb_file *f) { return (f && f->valid && (f->mask & CTB_FS_IS_OPEN)); }
CTB_FS_DEF int ctb_fs_can_read(const ctb_file *f) { return (f && f->valid && (f->mask & CTB_FS_PERM_READ)); }
CTB_FS_DEF int ctb_fs_can_write(const ctb_file *f){ return (f && f->valid && (f->mask & CTB_FS_PERM_WRITE)); }

CTB_FS_DEF ctb_file ctb_fs_open(const char *path, ctb_fs_mask mode)
{
    ctb_file f;
    memset(&f, 0, sizeof(ctb_file));
    if (!path) return f;

    f.path = _ctb_fs_strdup(path);
    f.basename = _ctb_fs_basename(f.path);
    f.mask = mode;
    ctb_fs_mask fs_stats = 0;
    if (_ctb_fs_stat_internal(path, &fs_stats, &f.size))
    {
        f.mask |= fs_stats;
        f.valid = 1;
    }

    if (mode & (CTB_FS_OPEN_READ | CTB_FS_OPEN_WRITE | CTB_FS_OPEN_APPEND))
    {
        char s[8]; _ctb_fs_mode_str(mode, s);
        FILE *fp = fopen(path, s);
        if (fp)
        {
            f.handle = (void*)fp;
            f.mask |= CTB_FS_IS_OPEN;
            f.valid = 1; 
            if (f.size == 0) _ctb_fs_stat_internal(path, NULL, &f.size);
        }
        else
        {
            f.mask &= ~CTB_FS_IS_OPEN; // Open failed
        }
    }
    return f;
}

CTB_FS_DEF void ctb_fs_close(ctb_file *file)
{
    if (!file) return;
    if ((file->mask & CTB_FS_IS_OPEN) && file->handle)
    {
        fclose((FILE*)file->handle);
    }
    free(file->path);
    memset(file, 0, sizeof(ctb_file));
}

#endif /* CTB_FS_IMPLEMENTATION */
