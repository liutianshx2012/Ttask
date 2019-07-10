/*************************************************************************
	> File Name: mach.h
	> Author:  TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time:  10/ 10 22:03:15 2016
 ************************************************************************/

#ifndef _MACH_H
#define _MACH_H


#include <stdint.h>


#define FAT_MAGIC	0xcafebabe
#define FAT_CIGAM	0xbebafeca



typedef int cpu_type_t;
typedef int cpu_subtype_t;
typedef int vm_prot_t;


struct fat_header_s {
    uint32_t	magic;
    uint32_t	nfat_arch;
};
typedef struct fat_header_s fat_header_t;

struct fat_arch_s {
    cpu_type_t	cputype;
    cpu_subtype_t	cpusubtype;
    uint32_t	offset;
    uint32_t	size;
    uint32_t	align;
};
typedef struct fat_arch_s fat_arch_t;

/*
 * The 32-bit mach header appears at the very beginning of the object file for
 * 32-bit architectures.
 */
struct mach_header_s {
    cpu_type_t	cputype;
    cpu_subtype_t	cpusubtype;
    uint32_t	filetype;
    uint32_t	ncmds;
    uint32_t	sizeofcmds;
    uint32_t	flags;
};
typedef struct mach_header_s mach_header_t;

/* Constant for the magic field of the mach_header (32-bit architectures) */
#define	MH_MAGIC	0xfeedface
#define MH_CIGAM	0xcefaedfe


/*
 * The 64-bit mach header appears at the very beginning of object files for
 * 64-bit architectures.
 */
struct mach_header_64_s {
    cpu_type_t	cputype;
    cpu_subtype_t	cpusubtype;
    uint32_t	filetype;
    uint32_t	ncmds;
    uint32_t	sizeofcmds;
    uint32_t	flags;
    uint32_t	reserved;
};
typedef struct mach_header_64_s mach_header_64_t;

/* Constant for the magic field of the mach_header_64 (64-bit architectures) */
#define MH_MAGIC_64 0xfeedfacf
#define MH_CIGAM_64 0xcffaedfe


struct load_command_s {
    uint32_t cmd;
    uint32_t cmdsize;
};
typedef struct load_command_s load_command_t;


/* for 32-bit architectures */
struct segment_command_32_s {  /* sizeof(struct segment_command_32_t) = 56*/
    char		segname[16];	/* segment name */
    uint32_t	vmaddr;		/* memory address of this segment */
    uint32_t	vmsize;		/* memory size of this segment */
    uint32_t	fileoff;	/* file offset of this segment */
    uint32_t	filesize;	/* amount to map from the file */
    vm_prot_t	maxprot;	/* maximum VM protection */
    vm_prot_t	initprot;	/* initial VM protection */
    uint32_t	nsects;		/* number of sections in segment */
    uint32_t	flags;		/* flags */
};
typedef struct segment_command_32_s segment_command_32_t;

/*
 * The 64-bit segment load command indicates that a part of this file is to be
 * mapped into a 64-bit task's address space.  If the 64-bit segment has
 * sections then section_64 structures directly follow the 64-bit segment
 * command and their size is reflected in cmdsize.
 */
/* for 64-bit architectures */
struct segment_command_64_s { /* sizeof(struct segment_command_64_t) = 72 */
    char		segname[16];	/* segment name */
    uint64_t	vmaddr;		/* memory address of this segment */
    uint64_t	vmsize;		/* memory size of this segment */
    uint64_t	fileoff;	/* file offset of this segment */
    uint64_t	filesize;	/* amount to map from the file */
    vm_prot_t	maxprot;	/* maximum VM protection */
    vm_prot_t	initprot;	/* initial VM protection */
    uint32_t	nsects;		/* number of sections in segment */
    uint32_t	flags;		/* flags */
};
typedef struct segment_command_64_s segment_command_64_t;


/* for 32-bit architectures */
struct section_32_s {                 /* sizeof(struct section) = 68 */
    char		sectname[16];	/* name of this section */
    char		segname[16];	/* segment this section goes in */
    uint32_t	addr;		/* memory address of this section */
    uint32_t	size;		/* size in bytes of this section */
    uint32_t	offset;		/* file offset of this section */
    uint32_t	align;		/* section alignment (power of 2) */
    uint32_t	reloff;		/* file offset of relocation entries */
    uint32_t	nreloc;		/* number of relocation entries */
    uint32_t	flags;		/* flags (section type and attributes)*/
    uint32_t	reserved1;	/* reserved (for offset or index) */
    uint32_t	reserved2;	/* reserved (for count or sizeof) */
};
typedef struct section_32_s section_32_t;

/* for 64-bit architectures */
struct section_64_s {             /* sizeof(struct section) = 80 */
    char		sectname[16];	/* name of this section */
    char		segname[16];	/* segment this section goes in */
    uint64_t	addr;		/* memory address of this section */
    uint64_t	size;		/* size in bytes of this section */
    uint32_t	offset;		/* file offset of this section */
    uint32_t	align;		/* section alignment (power of 2) */
    uint32_t	reloff;		/* file offset of relocation entries */
    uint32_t	nreloc;		/* number of relocation entries */
    uint32_t	flags;		/* flags (section type and attributes)*/
    uint32_t	reserved1;	/* reserved (for offset or index) */
    uint32_t	reserved2;	/* reserved (for count or sizeof) */
    uint32_t	reserved3;	/* reserved */
};
typedef struct section_64_s section_64_t;

/*
 * The symtab_command contains the offsets and sizes of the link-edit 4.3BSD
 * "stab" style symbol table information as described in the header files
 * <nlist.h> and <stab.h>.
 */
struct symtab_command_s {
    uint32_t	symoff;
    uint32_t	nsyms;
    uint32_t	stroff;
    uint32_t	strsize;
};
typedef struct symtab_command_s symtab_command_t;

/*
 * The linkedit_data_command contains the offsets and sizes of a blob
 * of data in the __LINKEDIT segment.
 */
struct linkedit_data_command_t {
    uint32_t	dataoff;
    uint32_t	datasize;
};


struct nlist_s {
    union {
        uint32_t n_strx;
    } n_un;
    uint8_t n_type;
    uint8_t n_sect;
    int16_t n_desc;
    uint32_t n_value;
};
typedef struct nlist_s nlist_t;

/*
 * This is the symbol table entry structure for 64-bit architectures.
 */
struct nlist_64_s {
    union {
        uint32_t  n_strx;
    } n_un;
    uint8_t n_type;
    uint8_t n_sect;
    uint16_t n_desc;
    uint64_t n_value;
};
typedef struct nlist_64_s nlist_64_t;


#endif
