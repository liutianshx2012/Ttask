/*************************************************************************
	> File Name: atosl.h
	> Author:  TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 一  10/ 10 22:04:16 2016
 ************************************************************************/

#ifndef _ATOSL_H
#define _ATOSL_H

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <getopt.h>
#include <libgen.h>

#include <limits.h>
#include <libdwarf.h>
#include "mach.h"

#define VERSION 2.1

#define UUID_LEN 16


//#define DEBUG
#ifdef DEBUG
#define atosl_log(format,...) fprintf(stderr,format"\n", ##__VA_ARGS__)
#else
#define atosl_log(format,...)
#endif


/* Wrapper to call write() in a loop until all data is written */
static inline ssize_t
_write(int fd, const void *buf, size_t count)
{
    ssize_t n_writted = 0;
    ssize_t ret = 0;
    while (n_writted < count) {
        ret = write(fd, buf + n_writted, count - n_writted);
        if (ret == 0) return n_writted;
        else if (ret < 0) return ret;
        n_writted += ret;
    }
    return  n_writted;
}
/* Wrapper to call read() in a loop until all data is read */
static inline ssize_t
_read(int fd, void *buf, size_t count)
{
    ssize_t n_read =0;
    ssize_t ret =0;
    while (n_read < count) {
        ret = read(fd,buf + n_read, count-n_read);
        if (ret == 0) return n_read;
        else if (ret < 0) return ret;
        n_read += ret;
    }
    return n_read;
}


#define MH_DYLIB 0x6
#define MH_DYLIB_STUB 0x9
#define MH_DSYM 0xa
#define MH_EXECUTE 0x2


#define LC_SEGMENT 0x1
#define LC_SYMTAB 0x2
#define LC_PREPAGE 0xa
#define LC_UUID 0x1b
#define LC_SEGMENT_64 0x19
#define LC_FUNCTION_STARTS 0x26

#define N_STAB 0xe0
#define N_PEXT 0x10
#define N_TYPE 0x0e
#define N_EXT 0x01

#define N_UNDF 0x0
#define N_ABS 0x2
#define N_SECT 0xe
#define N_PBUD 0xc
#define N_INDR 0xa

#define N_FUN 0x24

#define CPU_TYPE_ARM ((cpu_type_t)12)
#define CPU_SUBTYPE_ARM_V6 ((cpu_subtype_t)6)
#define CPU_SUBTYPE_ARM_V7 ((cpu_subtype_t)9)
#define CPU_SUBTYPE_ARM_V7S ((cpu_subtype_t)11)
#define CPU_TYPE_ARM64 ((cpu_type_t)16777228)
#define CPU_SUBTYPE_ARM64_ALL ((cpu_subtype_t)0)
#define CPU_TYPE_I386 ((cpu_type_t)7)
#define CPU_SUBTYPE_X86_ALL ((cpu_subtype_t)3)
#define N_ARM_THUMB_DEF 0x0008

#define NUMOF(x) (sizeof((x))/sizeof((x)[0]))

static struct {
    const char *name;
    cpu_type_t type;
    cpu_subtype_t subtype;
} arch_str_to_type[] = {
    {"i386", CPU_TYPE_I386, CPU_SUBTYPE_X86_ALL},
    {"armv6",  CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V6},
    {"armv7",  CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7},
    {"armv7s", CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7S},
    {"arm64",  CPU_TYPE_ARM64, CPU_SUBTYPE_ARM64_ALL}
};

struct symbol_s {
    char *name;
    union {
        nlist_t sym32;
        nlist_64_t sym64;
    }sym;
    Dwarf_Addr addr;
    int thumb:1;
};

typedef struct symbol_s symbol_t;


void
atosl_start_parser(char format[][128],const char *cpu_name,const char *path,Dwarf_Addr load_address,char **addrs,int count);
/*
void
atosl_getopt_cmd(int argc, char *argv[],const char **dsym_path,Dwarf_Addr *load_address,
           const char **cpu_name);
*/

#define CUSTOM_MAGIC   0xcaceecac
#define CUSTOM_VERSION 1

struct atosl_header_s {
    unsigned int magic;
    unsigned int version;
    //uint8_t uuid[UUID_LEN]; //16byte
    Dwarf_Addr intended_addr; /* __TEXT segment  .vmaddr */
    Dwarf_Addr linkedit_addr; /* __LINKEDIT  segment  .fileoff */
    unsigned int n_entries;   /* numbers of symbols in __Symtab  segment */
    unsigned int n_cudies;    /* numbers of cu_die_t in __Debugline section */
    unsigned int symsize;     /* sym name size of file */
    unsigned int cudsize;     /* cudie size of file */
    unsigned int offset;      /* line cu_die offset */
    unsigned int cksum;
    uint8_t is_64;            /* 64bit flag */
    uint8_t is_dwarf;         /* __DWARF segment flag */
    uint8_t dsym_len;         /* the length of dsym file name */
};

typedef struct atosl_header_s atosl_header_t;

struct atosl_symtab_entry_s { /* 16byte*/
    Dwarf_Addr addr;
    size_t namelen;
};

typedef struct atosl_symtab_entry_s atosl_symtab_entry_t;

struct atosl_cudie_entry_s {  /* 40 byte*/
    Dwarf_Addr lowpc;
    Dwarf_Addr highpc;
    Dwarf_Signed nculines;
    off_t offset;
    size_t namelen;
};

typedef struct atosl_cudie_entry_s atosl_cudie_entry_t;

struct line_s {        /*16 byte*/
    Dwarf_Addr addr;
    Dwarf_Unsigned lineno;
};

typedef struct line_s line_t;

struct cu_die_s{
    Dwarf_Addr lowpc;
    Dwarf_Addr highpc;
    char *name;
    line_t *linelist;
    Dwarf_Signed nculines;
};

typedef struct cu_die_s cu_die_t;
#endif
