/*************************************************************************
	> File Name: atosl.c
	> Author:  TTc
	> Mail: liutianshxkernel@gmail.com
	> Created Time: 一  10/ 10 22:04:19 2016
 ************************************************************************/

#include <stdbool.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include "string.h"
#include "atosl.h"

typedef int dwarf_mach_handle;

struct dwarf_section_s {
    section_32_t section;
    struct dwarf_section_s *next;
};

typedef struct dwarf_section_s dwarf_section_t;

struct dwarf_section_64_s {
    section_64_t section;
    struct dwarf_section_64_s *next;
};

typedef struct dwarf_section_64_s dwarf_section_64_t;

struct section_alloc_data_s {
    Dwarf_Small *section_data;
    int index;
};

typedef struct section_alloc_data_s section_alloc_data_t;

struct dwarf_mach_object_s {
    dwarf_mach_handle handle;
    Dwarf_Small length_size;
    Dwarf_Small pointer_size;
    Dwarf_Endianness endianness;
    Dwarf_Unsigned section_count;
    dwarf_section_t *sections;
    dwarf_section_64_t *sections_64;
    fat_arch_t arch;
    section_alloc_data_t *section_data;
} ;

typedef struct dwarf_mach_object_s dwarf_mach_object_t;

/************************************************
 1  2  3  4  5  6  7  8  9  10
 
 I  n  t  e  r  f  a  c  e  '\0'
 
 -9 -8 -7 -6 -5 -4 -3 -2 -1  0
 **************************************************/
#define idx(i, len) ((i) <= 0 ? (i) + (len) : (i) - 1)


#define convert(s, i, j) \
    do { \
        int len; \
        assert(s); \
        len = (int)strlen(s); \
        i = idx(i, len); \
        j = idx(j, len); \
        if (i > j) { \
            int t = i; \
            i = j; \
            j = t; \
        } \
        assert(i >= 0 && j <= len); \
    } while (0)

/* 返回s[i:j], 它是s中位于置于 i 和 j之间的子串 */
char *
str_sub(const char *s, int i, int j)
{
    char *str, *p;
    convert(s, i, j);
    p = str = malloc(j - i + 1);
    while (i < j)
        *p++ = s[i++];
    *p = '\0';
    return str;
}

/* s[i:j]的最右侧开始搜索字符c*/
int
str_rchr(const char *s, int i, int j, int c)
{
   	convert(s, i, j);
    while (j > i)
        if (s[--j] == c)
            return j + 1;
    return 0;
}

int
atosl_parse_uuid(dwarf_mach_object_t *obj,
                 uint32_t cmdsize,uint8_t *uuid)
{
    if (_read(obj->handle, uuid, UUID_LEN)<0) {
        return DW_DLV_ERROR;
    }
    
    return DW_DLV_OK;
}

int
atosl_parse_section(dwarf_mach_object_t *obj)
{
    dwarf_section_t tmp;
    tmp.next = NULL;
    if (_read(obj->handle, &tmp.section, sizeof(tmp.section))<0) {
        return DW_DLV_ERROR;
    }

    if (strncmp(tmp.section.sectname, "__debug",6) == 0) {
        dwarf_section_t *section;
        section = malloc(sizeof(dwarf_section_t));
        memcpy(section, &tmp, sizeof(dwarf_section_t));
        
        dwarf_section_t *sec = obj->sections;
        if (!sec) {
            obj->sections = section;
        } else {
            while (sec) {
                if (sec->next == NULL) {
                    sec->next = section;
                    break;
                } else {
                    sec = sec->next;
                }
            }
        }
        obj->section_count++;
    }
    
    return DW_DLV_OK;
}

int
atosl_parse_section_64(dwarf_mach_object_t *obj)
{
    dwarf_section_64_t tmp;
    tmp.next = NULL;
    if (_read(obj->handle, &tmp.section, sizeof(tmp.section))<0) {
        return DW_DLV_ERROR;
    }
    if (strncmp(tmp.section.sectname, "__debug",6) == 0) {
        dwarf_section_64_t *section;
        section = malloc(sizeof(dwarf_section_64_t));
        memcpy(section, &tmp, sizeof(dwarf_section_64_t));
        
        dwarf_section_64_t *sec = obj->sections_64;
        if (!sec) {
            obj->sections_64 = section;
        } else {
            while (sec) {
                if (sec->next == NULL) {
                    sec->next = section;
                    break;
                } else {
                    sec = sec->next;
                }
            }
        }
        obj->section_count++;
    }

    return DW_DLV_OK;
}

int
atosl_parse_segment(dwarf_mach_object_t *obj,uint32_t cmdsize,uint8_t *is_dwarf,
                    Dwarf_Addr *seg_addr)
{
    int err = 0;
    int i = 0;
    segment_command_32_t segment;
    if (_read(obj->handle, &segment, sizeof(segment))<0) {
        return DW_DLV_ERROR;
    }
    if (strcmp(segment.segname, "__TEXT") == 0) {
        seg_addr[0] = segment.vmaddr;
    }
    if (strcmp(segment.segname, "__LINKEDIT") ==0) {
        seg_addr[1] = segment.fileoff;
    }
    if (strcmp(segment.segname, "__DWARF") == 0) {
        *is_dwarf = 1;
    }
    for (i =0; i < segment.nsects; i++) {
        err = atosl_parse_section(obj);
        
        if (err) {
            return DW_DLV_ERROR;
        }
    }
    
    return DW_DLV_OK;
}

int
atosl_parse_segment_64(dwarf_mach_object_t *obj,
                uint32_t cmdsize,uint8_t *is_dwarf,Dwarf_Addr *seg_addr)
{
    int err = 0;
    int i = 0;
    segment_command_64_t segment;
    
    if (_read(obj->handle, &segment, sizeof(segment))<0) {
        return DW_DLV_ERROR;
    }
    if (strcmp(segment.segname, "__TEXT") == 0) {
        seg_addr[0] = segment.vmaddr;
    }
    if (strcmp(segment.segname, "__LINKEDIT") ==0) {
        seg_addr[1] = segment.fileoff;
    }
    if (strcmp(segment.segname, "__DWARF") == 0) {
        *is_dwarf = 1;
    }
    for (i =0; i < segment.nsects; i++) {
        err = atosl_parse_section_64(obj);
        if (err) {
            return DW_DLV_ERROR;
        }
    }
    
    return DW_DLV_OK;
}

static int
atosl_compare_symbols(const void *a, const void *b)
{
    symbol_t *sym_a = (symbol_t*)a;
    symbol_t *sym_b = (symbol_t*)b;
    long long comp = (sym_a->addr - sym_b->addr);
    int result = 0;
    if (comp > 0) {
        result = 1;
    } else if (comp < 0) {
        result = -1;
    } else {
        result = 0;
    }
    
    return result;
}

void
atosl_dealloc_symtab(symbol_t *header,char *strtable)
{
    if (header) {
        free(header);
    }
    if (strtable) {
        free(strtable);
    }
}

int
atosl_parse_symtab(dwarf_mach_object_t *obj, uint32_t cmdsize,uint8_t is_64,
                   fat_arch_t arch,symbol_t **header,uint32_t *nsymbols,
                   char **strtab)
{
    int i;
    off_t pos;
    char *strtable = NULL;
    
    union {
        nlist_t nlist32;
        nlist_64_t nlist64;
    } nlist;
    
    symtab_command_t symtab;
    symbol_t *current = NULL;
    symbol_t *symlist = NULL;
    
    if (_read(obj->handle, &symtab, sizeof(symtab))<0) {
        return DW_DLV_ERROR;
    }
    if ((strtable = malloc(symtab.strsize)) == NULL) {
        return DW_DLV_ERROR;
    }
    *strtab = strtable;
    pos = lseek(obj->handle, 0, SEEK_CUR);
    if (pos < 0) {
        free(strtable);
        return DW_DLV_ERROR;
    }
    if (lseek(obj->handle, arch.offset + symtab.stroff, SEEK_SET) < 0) {
        free(strtable);
        return DW_DLV_ERROR;
    }
    if (_read(obj->handle, strtable, symtab.strsize) < 0) {
        free(strtable);
        return DW_DLV_ERROR;
    }
    if (lseek(obj->handle, arch.offset + symtab.symoff, SEEK_SET) < 0) {
        free(strtable);
        return DW_DLV_ERROR;
    }
    if ((symlist = malloc(sizeof(symbol_t) * symtab.nsyms)) == NULL) {
        return DW_DLV_ERROR;
    }
    current = *header = symlist;
    *nsymbols = symtab.nsyms;
    for (i = 0; i < symtab.nsyms; i++) {
        if (_read(obj->handle, is_64 ? (void*)&current->sym.sym64
                  : (void*)&current->sym.sym32, is_64 ?
                  sizeof(current->sym.sym64) : sizeof(current->sym.sym32))<0) {
            return DW_DLV_ERROR;
        }
        
        if (is_64 ? current->sym.sym64.n_un.n_strx :
            current->sym.sym32.n_un.n_strx) {
            if ((is_64 ? current->sym.sym64.n_un.n_strx
                       : current->sym.sym32.n_un.n_strx) > symtab.strsize) {
                return DW_DLV_ERROR;
            }
            current->name = strtable+(is_64 ? current->sym.sym64.n_un.n_strx
                                            : current->sym.sym32.n_un.n_strx);
            
            memcpy(is_64 ? (void*)&nlist.nlist64 : (void*)&nlist.nlist32,
                   is_64 ? (void*)&current->sym.sym64 : (void*)&current->sym.sym32,
                   is_64 ? sizeof(current->sym.sym64) : sizeof(current->sym.sym32));
                   
            current->thumb = ((is_64 ? nlist.nlist64.n_desc : nlist.nlist32.n_desc)
                                & N_ARM_THUMB_DEF) ? 1 : 0;
            
            current->addr = is_64 ? nlist.nlist64.n_value : nlist.nlist32.n_value;
        }
        current++;
    }
    /* 效率提高不明显，因为基本有序的 平均 O(nlogn); 最坏O(n^2)*/
    qsort(symlist, symtab.nsyms, sizeof(symbol_t), atosl_compare_symbols);
    if (lseek(obj->handle, pos, SEEK_SET)<0) {
        return DW_DLV_ERROR;
    }
    return DW_DLV_OK;
}


int
atosl_parse_command(dwarf_mach_object_t *obj,load_command_t load_command,
        uint8_t is_64,uint8_t *is_dwarf,Dwarf_Addr *seg_addr,fat_arch_t arch,
                  symbol_t **header,uint32_t *nsymbols,char **strtable)
{
    uint8_t uuid[UUID_LEN];
    
    int ret = DW_DLV_OK;
    switch (load_command.cmd) {
        case LC_UUID:
            ret = atosl_parse_uuid(obj, load_command.cmdsize, uuid);
            break;
        case LC_SEGMENT:
            ret = atosl_parse_segment(obj, load_command.cmdsize,is_dwarf,seg_addr);
            break;
        case LC_SEGMENT_64:
            ret = atosl_parse_segment_64(obj, load_command.cmdsize,is_dwarf,seg_addr);
            break;
        case LC_SYMTAB:
            ret = atosl_parse_symtab(obj, load_command.cmdsize,is_64,arch,header,nsymbols,strtable);
            break;
        default:
            atosl_log("Warning: unhandled command: 0x%x\n",
                      load_command.cmd);
        case LC_PREPAGE: {
            int cmdsize = load_command.cmdsize - sizeof(load_command);
            if (lseek(obj->handle, cmdsize, SEEK_CUR)<0) {
                return DW_DLV_ERROR;
            }
            break;
        }
    }
    
    return ret;
}

static int
dwarf_mach_object_get_section_info(void *obj_in,Dwarf_Half section_index,
                Dwarf_Obj_Access_Section *ret_scn,int *error)
{
    int i;
    dwarf_mach_object_t *obj =
    (dwarf_mach_object_t *)obj_in;
    
    if (section_index >= obj->section_count) {
        *error = DW_DLE_MDE;
        return DW_DLV_ERROR;
    }
    
    if (obj->sections_64) {
        dwarf_section_64_t *sec = obj->sections_64;
        for (i = 0; i < section_index; i++) {
            sec = sec->next;
        }
        sec->section.sectname[1] = '.';
        ret_scn->size = sec->section.size;
        ret_scn->addr = sec->section.addr;
        ret_scn->name = sec->section.sectname+1;
    } else {
        dwarf_section_t *sec = obj->sections;
        for (i = 0; i < section_index; i++) {
            sec = sec->next;
        }
        sec->section.sectname[1] = '.';
        ret_scn->size = sec->section.size;
        ret_scn->addr = sec->section.addr;
        ret_scn->name = sec->section.sectname+1;
    }
    
    if (strcmp(ret_scn->name, ".debug_pubnames__DWARF") == 0) {
        ret_scn->name = ".debug_pubnames";
    }
    if (strcmp(ret_scn->name, ".__debug_pubtypes__DWARF") == 0) {
        ret_scn->name = ".debug_pubtypes";
    }
    /* rela section or from symtab to strtab */
    ret_scn->link = 0;
    ret_scn->entrysize = 0;
    
    return DW_DLV_OK;
}

static Dwarf_Endianness
dwarf_mach_object_get_byte_order(void *object)
{
    dwarf_mach_object_t *obj =
    (dwarf_mach_object_t*)object;
    return obj->endianness;
}

static Dwarf_Small
dwarf_mach_object_get_length_size(void *object)
{
    dwarf_mach_object_t *obj =
    (dwarf_mach_object_t*)object;
    return obj->length_size;
}

static Dwarf_Small
dwarf_mach_object_get_pointer_size(void *object)
{
    dwarf_mach_object_t *obj =
    (dwarf_mach_object_t*)object;
    return obj->pointer_size;
}

static Dwarf_Unsigned
dwarf_mach_object_get_section_count(void *object)
{
    dwarf_mach_object_t *obj =
    (dwarf_mach_object_t*)object;
    size_t len = obj->section_count *sizeof(section_alloc_data_t);
    obj->section_data = malloc(len);
    memset(obj->section_data, 0, len);
    return obj->section_count;
}

static int
dwarf_mach_object_load_section(void *object,Dwarf_Half section_index,
                Dwarf_Small **section_data,int *error)
{
    int i = 0;
    void *addr;
    dwarf_mach_object_t *obj =
    (dwarf_mach_object_t*)object;
    if (section_index >= obj->section_count) {
        *error = DW_DLE_MDE;
        return DW_DLV_ERROR;
    }
    fat_arch_t arch = obj->arch;
    
    if (obj->sections_64) {
        dwarf_section_64_t *sec = obj->sections_64;
        for (i=0; i<section_index; i++) {
            sec = sec->next;
        }
        if ((addr = malloc(sec->section.size)) == NULL) {
            return DW_DLV_NO_ENTRY;
        }
        if (lseek(obj->handle,arch.offset+sec->section.offset,SEEK_SET)<0) {
            free(addr);
            return DW_DLV_NO_ENTRY;
        }
        if (_read(obj->handle,addr,sec->section.size)<0) {
            free(addr);
            return DW_DLV_NO_ENTRY;
        }
    } else {
        dwarf_section_t *sec = obj->sections;
        for (i = 0; i < section_index; i++) {
            sec = sec->next;
        }
        if ((addr = malloc(sec->section.size)) == NULL) {
            return DW_DLV_NO_ENTRY;
        }
        if (lseek(obj->handle,arch.offset+sec->section.offset,SEEK_SET)<0) {
            free(addr);
            return DW_DLV_NO_ENTRY;
        }
        if (_read(obj->handle,addr,sec->section.size)<0) {
            free(addr);
            return DW_DLV_NO_ENTRY;
        }
    }
    section_alloc_data_t *cur = obj->section_data+section_index;
    cur->section_data = addr;
    *section_data = addr;
    return DW_DLV_OK;
}

static int
dwarf_mach_object_relocate_a_section(void *obj_in,Dwarf_Half section_index,
                                     Dwarf_Debug dbg,int *error)
{
    return DW_DLV_NO_ENTRY;
}

static const struct Dwarf_Obj_Access_Methods_s
dwarf_mach_object_methods =
{
    dwarf_mach_object_get_section_info,
    dwarf_mach_object_get_byte_order,
    dwarf_mach_object_get_length_size,
    dwarf_mach_object_get_pointer_size,
    dwarf_mach_object_get_section_count,
    dwarf_mach_object_load_section,
    dwarf_mach_object_relocate_a_section
};

static int
atosl_dwarf_mach_object_init(dwarf_mach_handle handle,
            void *obj_in, int *error,uint8_t *is_64,uint8_t *is_dwarf,
            Dwarf_Addr *seg_addr,fat_arch_t arch,symbol_t **header,
                            uint32_t *nsymbols,char **strtable)
{
    int i;
    mach_header_t m_header;
    load_command_t load_command;
    
    dwarf_mach_object_t *obj =
    (dwarf_mach_object_t *)obj_in;
    
    obj->handle = handle;
    obj->length_size = 4;
    obj->pointer_size = 4;
    obj->endianness = DW_OBJECT_LSB;
    obj->sections = NULL;
    obj->sections_64 = NULL;
    obj->section_count = 0;
    obj->arch = arch;
    
    if (_read(obj->handle,&m_header,sizeof(mach_header_t))<0) {
        return DW_DLV_ERROR;
    }
    /* Need to skip 4 bytes of the reserved field of mach_header_64  */
    if (m_header.cputype==CPU_TYPE_ARM64 &&
        m_header.cpusubtype==CPU_SUBTYPE_ARM64_ALL) {
        *is_64 = 1;
        if (lseek(obj->handle,sizeof(uint32_t),SEEK_CUR)<0) {
            return DW_DLV_ERROR;
        }
    } else {
        *is_64 = 0;
    }
    
    assert(arch.cputype==m_header.cputype
           && arch.cpusubtype == m_header.cpusubtype);
    
    for (i=0; i<m_header.ncmds; i++) {
        if (_read(obj->handle,&load_command,sizeof(load_command))<0) {
            return DW_DLV_ERROR;
        }
        if (atosl_parse_command(obj,load_command,*is_64,is_dwarf,
                seg_addr,arch,header,nsymbols,strtable) != DW_DLV_OK) {
            return DW_DLV_ERROR;
        }
    }
    
    return DW_DLV_OK;
}
void
dwarf_dealloc_secion(dwarf_mach_object_t *obj)
{
    dwarf_section_t *sec32 = obj->sections;
    while (sec32) {
        dwarf_section_t *next = sec32->next;
        free(sec32);
        sec32 = next;
    }
    
    dwarf_section_64_t *sec64 = obj->sections_64;
    while (sec64) {
        dwarf_section_64_t *next = sec64->next;
        free(sec64);
        sec64 = next;
    }
    
    for (int i=0; i<obj->section_count; i++) {
        section_alloc_data_t *section_data = obj->section_data+i;
        if (section_data->section_data) {
            free(section_data->section_data);
        }
    }
    free(obj->section_data);
}

int
dwarf_mach_object_init(dwarf_mach_object_t *internals,dwarf_mach_handle handle,
        Dwarf_Obj_Access_Interface **ret_obj,int *err,uint8_t *is_64,
        uint8_t *is_dwarf,Dwarf_Addr *seg_addr,fat_arch_t arch,
        symbol_t **header,uint32_t *nsymbols,char **strtable)
{
    
    Dwarf_Obj_Access_Interface *intfc = NULL;
    if (atosl_dwarf_mach_object_init(handle, internals, err,is_64,
        is_dwarf,seg_addr,arch,header,nsymbols,strtable) != DW_DLV_OK) {
        return DW_DLV_ERROR;
    }
    if ((intfc = malloc(sizeof(Dwarf_Obj_Access_Interface)))==NULL) {
        return DW_DLV_ERROR;
    }
    intfc->object = internals;
    intfc->methods = &dwarf_mach_object_methods;
    
    *ret_obj = intfc;
    
    return DW_DLV_OK;
}

void
dwarf_mach_object_finish(Dwarf_Obj_Access_Interface *obj)
{
    if (!obj) {
        return;
    }
    free(obj);
}

bool
atosl_found_cpu_type(cpu_type_t *cpu_type,cpu_subtype_t *cpu_subtype,
                     const char *cpu_name)
{
    int i;
    bool found = false;
    for (i=0;i<NUMOF(arch_str_to_type);i++) {
        if (strcmp(arch_str_to_type[i].name,cpu_name)==0) {
            *cpu_type = arch_str_to_type[i].type;
            *cpu_subtype = arch_str_to_type[i].subtype;
            found = true;break;
        }
    }
    return found;
}

bool
atosl_found_arch(int fd, uint32_t *magic,const char *cpu_name,fat_arch_t *arch)
{
    int i;
    cpu_type_t cpu_type = -1;
    cpu_subtype_t cpu_subtype = -1;
    
    if (!atosl_found_cpu_type(&cpu_type, &cpu_subtype,cpu_name)) {
        return false;
    }
    
    if (_read(fd, magic, sizeof(*magic))<0) {
        return false;
    }
    
    if (*magic == FAT_CIGAM) {
        uint32_t nfat_arch = 0;
        if (_read(fd, &nfat_arch, sizeof(nfat_arch))<0) {
            return false;
        }
        nfat_arch = ntohl(nfat_arch);
        for (i=0; i<nfat_arch; i++) {
            if (_read(fd, arch, sizeof(fat_arch_t))<0) {
                return false;
            }
            arch->cputype = ntohl(arch->cputype);
            arch->cpusubtype = ntohl(arch->cpusubtype);
            arch->offset = ntohl(arch->offset);
            if ((arch->cputype == cpu_type) &&
                (arch->cpusubtype == cpu_subtype)) {
                if (lseek(fd, arch->offset, SEEK_SET)<0) {
                    return false;
                }
                if (_read(fd, magic, sizeof(*magic))<0) {
                    return false;
                }
                break;
            }
        }
    } else {
        arch->cputype = cpu_type;
        arch->cpusubtype = cpu_subtype;
    }
    return true;
}

static int
atosl_find_symbol_symtab(Dwarf_Addr slide,Dwarf_Addr search_address,
                        uint8_t is_64,char *func_name,Dwarf_Unsigned *lineno,
                         symbol_t *header,uint32_t nsymbols)
{
    int i = 0;
    int found = 0;
    symbol_t *current = NULL;
    Dwarf_Addr orig_seach_addr = search_address;
    search_address -= slide;
    current = header;
    
    for (i=0; i<nsymbols; i++) {
        if (current->addr > search_address) {
            if (i < 1) {
                break;
            }
            symbol_t *prev = (current -1);
            if (prev != NULL) {
                char *tmp = prev->name;
                size_t len = strlen(tmp);
                strncpy(func_name,tmp,len+1);
                *lineno = orig_seach_addr - prev->addr;
            }
            found =1;
            break;
        }
        current++;
    }
    
    return found ? DW_DLV_OK : DW_DLV_NO_ENTRY;
}

int
atosl_find_symbol_dwarf(Dwarf_Debug dbg, Dwarf_Addr slide, Dwarf_Addr addr,
                        char *dwarf_name,Dwarf_Unsigned *lineno)
{
    Dwarf_Line   *linebuf = NULL;
    Dwarf_Signed linecount = 0;
    Dwarf_Off    cu_die_offset = 0;
    Dwarf_Die    cu_die = NULL;
    Dwarf_Unsigned segment = 0;
    Dwarf_Unsigned segment_entry_size = 0;
    Dwarf_Addr      start = 0;
    Dwarf_Unsigned  length = 0;
    int i = 0;
    int ret = 0;
    int found = 0;
    
    addr -= slide;
    
    Dwarf_Arange *arange_buf = NULL;
    Dwarf_Signed count = 0;
    Dwarf_Error err;
    Dwarf_Arange arange;
    
    if (!arange_buf) {
        ret = dwarf_get_aranges(dbg, &arange_buf, &count, &err);
        if (ret != DW_DLV_OK) {
            goto dwarferror;
        }
    }
    
    ret = dwarf_get_arange(arange_buf, count, addr, &arange, &err);
    if (ret != DW_DLV_OK) {
        goto dwarferror;
    }
    
    ret = dwarf_get_arange_info_b(arange,
                                  &segment,
                                  &segment_entry_size,
                                  &start,
                                  &length,
                                  &cu_die_offset,
                                  &err);
    if (ret != DW_DLV_OK) {
        goto dwarferror;
    }
    ret = dwarf_offdie(dbg, cu_die_offset, &cu_die, &err);
    if (ret != DW_DLV_OK) {
        goto dwarferror;
    }
    
    ret = dwarf_srclines(cu_die, &linebuf, &linecount, &err);
    if (ret != DW_DLV_OK) {
        goto dwarferror;
    }
    
    for (i=0; i<linecount; i++) {
        Dwarf_Line prevline;
        Dwarf_Line nextline;
        Dwarf_Line line = linebuf[i];
        
        Dwarf_Addr lineaddr;
        Dwarf_Addr lowaddr;
        Dwarf_Addr highaddr;
        
        ret = dwarf_lineaddr(line, &lineaddr, &err);
        if (ret != DW_DLV_OK) {
            goto dwarferror;
        }
        
        if (i > 0) {
            prevline = linebuf[i-1];
            ret = dwarf_lineaddr(prevline, &lowaddr, &err);
            if (ret != DW_DLV_OK) {
                goto dwarferror;
            }
            lowaddr += 1;
        } else {
            lowaddr = lineaddr;
        }
        
        if (i < linecount - 1) {
            nextline = linebuf[i+1];
            ret = dwarf_lineaddr(nextline, &highaddr, &err);
            if (ret != DW_DLV_OK) {
                goto dwarferror;
            }
            highaddr -= 1;
        } else {
            highaddr = lineaddr;
        }
        
        if ((addr >= lowaddr) && (addr <= highaddr)) {
            char *filename;
            ret = dwarf_linesrc(line, &filename, &err);
            if (ret != DW_DLV_OK) {
                goto dwarferror;
            }
            
            ret = dwarf_lineno(line, lineno, &err);
            if (ret != DW_DLV_OK) {
                goto dwarferror;
            }
            
            char *tmp = basename(filename);
            size_t len = strlen(tmp);
            strncpy(dwarf_name,tmp,len+1);
            dwarf_dealloc(dbg, filename, DW_DLA_STRING);
            found = 1;
            break;
        }
    }
    
    dwarf_dealloc(dbg, arange, DW_DLA_ARANGE);
    dwarf_srclines_dealloc(dbg, linebuf, linecount);
    
    return found ? DW_DLV_OK : DW_DLV_NO_ENTRY;
    
dwarferror:
    atosl_log("dwarf_errmsg: %s", dwarf_errmsg(err));
    return ret;
}

void
dwarf_error_handler(Dwarf_Error err, Dwarf_Ptr ptr)
{
    atosl_log("dwarf error: %s", dwarf_errmsg(err));
}

extern char * cplus_demangle_v3 (const char *mangled, int options);

void
atosl_format_symbol(uint8_t is_dwarf, const char *symbol, char *format,
                    char *dwarf_name,Dwarf_Unsigned lineno)
{
    char *name = NULL;
    if (strncmp(symbol, "_Z", 2) == 0) {
        name = cplus_demangle_v3(symbol, 1 << 0);
    } else if (strncmp(symbol, "__Z", 3) == 0) {
        name = cplus_demangle_v3(symbol+1, 1 << 0);
    }
    
    char *demangled = name ? name : (char*)symbol;
    if (is_dwarf) {
        sprintf(format,"%s (%s:%llu)",
                demangled,
                dwarf_name,
                lineno);
    } else {
        sprintf(format,"%s + %llu\n",
                demangled,
                lineno);
    }
}

bool
atosl_start_parser_origin_dsym(char result[][128],const char *cpu_name,
                   const char *path,Dwarf_Addr load_address,char **addrs,int count)
{
    int i;
    int  derr = 0;
    uint32_t magic = 0;
    uint8_t is_dwarf = 1;
    uint8_t is_64 = 0;
    
    Dwarf_Debug dbg = NULL;
    Dwarf_Error err;
    Dwarf_Ptr errarg = NULL;
    Dwarf_Addr seg_addr[2]; //intended_addr , linkedit_addr;
    
    fat_arch_t arch;
    symbol_t *header;
    uint32_t nsymbols;
    char *strtable;
    
    int fd = open(path,O_RDONLY);
    if (fd < 0) {
        return false;
    }
    if (!atosl_found_arch(fd, &magic,cpu_name,&arch)) {
        return false;
    }
    
    if (magic != MH_MAGIC && magic != MH_MAGIC_64) {
        return false;
    }
    
    dwarf_mach_object_t obj;
    
    Dwarf_Obj_Access_Interface *bin = NULL;
    dwarf_mach_object_init(&obj,fd,&bin,&derr,&is_64,&is_dwarf,
                    seg_addr,arch,&header,&nsymbols,&strtable);
    assert(bin);
    
    Dwarf_Addr intended_addr = seg_addr[0];
    //Dwarf_Addr linkedit_addr = seg_addr[1];
    
    if (load_address == LONG_MAX) {
        load_address = intended_addr;
    }
    int ret = dwarf_object_init(bin,dwarf_error_handler,
                                errarg, &dbg, &err);
    if (ret != DW_DLV_OK) {
        goto dwarferror;
    }
    char func_name[128];
    char dwarf_name[128];
    
    for (i=0;i<count;i++) {
        Dwarf_Addr search_address = strtol(addrs[i], (char **)NULL, 16);
        Dwarf_Addr slide = load_address - intended_addr;
        
        Dwarf_Unsigned lineno = 0;
        ret = atosl_find_symbol_symtab(slide,search_address,is_64,
                                       func_name,&lineno,header,nsymbols);
        
        if (ret != DW_DLV_OK) {
            goto dwarferror;
        }
        
        
        if (is_dwarf) {
            ret = atosl_find_symbol_dwarf(dbg,slide,search_address,
                                          dwarf_name,&lineno);
            if (ret != DW_DLV_OK) {
                is_dwarf = 0;
            }
        }
        char format[128];
        atosl_format_symbol(is_dwarf,func_name,format,dwarf_name,lineno);
        memcpy(result[i], format, strlen(format)+1);
    }
    close(fd);
    atosl_dealloc_symtab(header,strtable);
    dwarf_dealloc_secion(&obj);
    dwarf_mach_object_finish(bin);
    dwarf_object_finish(dbg, &err);

    return true;
dwarferror:
    atosl_log("dwarf_errmsg: %s", dwarf_errmsg(err));
    return false;
}


bool
atosl_load_custom_header(int fd, atosl_header_t *header)
{
    if (_read(fd, header, sizeof(atosl_header_t)) <0) {
        return false;
    }
    if (header->magic != CUSTOM_MAGIC) {
        atosl_log("wrong magic: %x, %x",CUSTOM_MAGIC,header->magic);
        return false;
    }
    if (header->version != CUSTOM_VERSION) {
        atosl_log("Unable to handleversion %d",header->version);
        return false;
    }
    if (header->n_entries <= 0) {
        atosl_log("custom dsym file parse error");
        return false;
    }
    
    return true;
}

bool
atosl_get_custom_sym_buff(int fd,atosl_header_t *header,void *buff)
{
    if (_read(fd,buff,header->symsize)<0) {
        atosl_log("unable to read sym data from file");
        return false;
    }
    return true;
}

bool
atosl_find_custom_symbol_symtab(const int fd, atosl_header_t *header,
                Dwarf_Addr search_addr,char *func_name, Dwarf_Unsigned *lineno,
                                void *sym_buf,off_t file_offset)
{
    int i;
    atosl_symtab_entry_t entry;
    atosl_symtab_entry_t prev;
    for (i=0; i<header->n_entries; i++) {
        memcpy(&entry,sym_buf+(sizeof(entry)*i),sizeof(entry));
        /* line search sym name */
        if (entry.addr > search_addr) {
            if (i<1) {
                continue;
            }
            memcpy(&prev,sym_buf+(sizeof(entry)*(i-1)),sizeof(entry));
            file_offset = file_offset - prev.namelen;
            lseek(fd, file_offset, SEEK_SET);
            char name[128];
            if (_read(fd, &name, prev.namelen)<0) {
                atosl_log("unable to read data from file");
                return false;
            }
            size_t len = strlen(name);
            strncpy(func_name,name,len+1);
            *lineno = search_addr - prev.addr;
            
            return true;
        }
        /* the offset of next entry */
        file_offset += entry.namelen;
    }
    
    return false;
}

bool
atosl_find_custom_symbol_dwarf(int fd, atosl_header_t *header,
            char *dwarf_name,Dwarf_Addr search_addr,Dwarf_Unsigned *lineno)
{
    int i,j;
    char buff[header->cudsize];
    void *cudie_buf = buff;
    if (lseek(fd,header->offset,SEEK_SET)<0) {
        return false;
    }
    if (_read(fd,cudie_buf,header->cudsize)<0) {
        atosl_log("unable to read data from file");
        return false;
    }
    for (i=0; i<header->n_cudies; i++) {
        atosl_cudie_entry_t cudie_entry;
        memcpy(&cudie_entry,cudie_buf,sizeof(cudie_entry));
        char name[cudie_entry.namelen];
        cudie_buf += sizeof(cudie_entry);
        memcpy(name,cudie_buf, cudie_entry.namelen);
        cudie_buf += cudie_entry.namelen;
        
        if ((search_addr >= cudie_entry.lowpc) &&
            (search_addr <= cudie_entry.highpc)) {
            if (lseek(fd,cudie_entry.offset,SEEK_SET)<0) {
                return false;
            }
            size_t line_lens = sizeof(line_t)*cudie_entry.nculines;
            char line_buf[line_lens];
            if (_read(fd,line_buf,line_lens)<0) {
                return false;
            }
            for (j =0; j<cudie_entry.nculines; j++) {
                line_t currline;
                line_t prevline;
                line_t nextline;
                
                Dwarf_Addr lineaddr;
                Dwarf_Addr lowaddr;
                Dwarf_Addr highaddr;
                memcpy(&currline,line_buf+(sizeof(line_t)*j),sizeof(line_t));
                lineaddr = currline.addr;
                if (j > 0) {
                    memcpy(&prevline,line_buf+(sizeof(line_t)*(j-1)),sizeof(line_t));
                    lowaddr = prevline.addr + 1;
                } else {
                    lowaddr = lineaddr;
                }
                
                if (j < cudie_entry.nculines - 1) {
                    memcpy(&nextline,line_buf+(sizeof(line_t)*(j+1)),sizeof(line_t));
                    highaddr = nextline.addr - 1;
                } else {
                    highaddr = lineaddr;
                }
                
                if ((search_addr >= lowaddr) &&
                    (search_addr <= highaddr)) {
                    *lineno = currline.lineno;
                    
                    char *tmp = basename(name);
                    size_t len = strlen(tmp);
                    strncpy(dwarf_name,tmp,len+1);
                    return true;
                }
            }
            break;
        }
    }
    return false;
}

bool
atosl_start_parser_custom_dsym(char result[][128],const char *cpu_name,int fd,
                               Dwarf_Addr load_address,char **addrs,int count)
{
    int i;
    char func_name[128];
    char dwarf_name[64];
    Dwarf_Unsigned lineno;
    atosl_header_t header;
    if (!atosl_load_custom_header(fd, &header)) {
        return false;
    }
    char filename[header.dsym_len];
    if (_read(fd, filename, header.dsym_len)<0) {
        return false;
    }
    uint8_t is_dwarf = header.is_dwarf;
    Dwarf_Addr intended_addr = header.intended_addr;
    
    if (load_address == LONG_MAX) {
        load_address = intended_addr;
    }
    
    char buff[header.symsize];
    if (!atosl_get_custom_sym_buff(fd, &header, buff)) {
        return false;
    }
    off_t file_offset = lseek(fd,0,SEEK_CUR);
 
    for (i=0; i<count; i++) {
        Dwarf_Addr search_address = strtol(addrs[i], (char **)NULL, 16);
        if (!atosl_find_custom_symbol_symtab(fd,&header,search_address,
                        func_name,&lineno,buff,file_offset)) {
            return false;
        }
        
        if (is_dwarf) {
            if (!atosl_find_custom_symbol_dwarf(fd,&header,dwarf_name,
                                                search_address,&lineno)) {
                is_dwarf = 0;
            }
        }
        char format[128];
        atosl_format_symbol(is_dwarf,func_name,format,dwarf_name,lineno);
        memcpy(result[i], format, strlen(format)+1);
    }
    
    return true;
}


/* 9314_91126_0_EDC1A393-0DF2-3603-9ACA-5401CB671692.dsymb
 * 原始dSYM的命名格式(有后缀区别)
 * 9314_91126_0_EDC1A393-0DF2-3603-9ACA-5401CB671692.arm64|armv7s.dsymb
 * covert 以后的 dSYM 命名格式（插入 arch）
 */
void
atosl_convert_dsymb_path(const char *archname,const char *dsym_path,char *conv_dsym)
{
    int rchr = str_rchr(dsym_path, 1, (int)strlen(dsym_path), '.');
    char *tmp = str_sub(dsym_path, 1, rchr);
    sprintf(conv_dsym, "%s.%s.%s",tmp,archname,"dsymb");
    free(tmp);
}

int
atosl_access_custom_file(char *filename)
{
    int fd = -1;
    if (access(filename, R_OK) == 0) {
        fd = open(filename, O_RDONLY);
    }
    return fd;
}

void
atosl_start_parser(char format[][128],const char *cpu_name,const char *dsym_path,
    Dwarf_Addr load_address,char **addrs,int count)
{
    char conv_path[128];

    atosl_convert_dsymb_path(cpu_name,dsym_path,conv_path);
    int fd = atosl_access_custom_file(conv_path);
    if (fd < 0) {
        if(!atosl_start_parser_origin_dsym(format,cpu_name,dsym_path,load_address,addrs,count)) {
            return;
        }
    } else {
        if(!atosl_start_parser_custom_dsym(format,cpu_name,fd,load_address,addrs,count)) {
            return ;
        }
    }
}

#ifdef ATOSL_BIN
int
tt_main(int argc, char *argv[])
{
#ifdef DEBUG
    clock_t start , finish;
    start = clock();
#endif
    const char *dsym_path;
    Dwarf_Addr load_address = LONG_MAX;
    const char *cpu_name;
    int count = argc - 5;
    char **addrs = argv + 5;
    atosl_getopt_cmd(argc,argv,&dsym_path,&load_address,&cpu_name);
    atosl_start_parser(cpu_name,dsym_path,load_address,addrs,count);

#ifdef DEBUG
    finish = clock();
    double duration = ((double)(finish - start) / CLOCKS_PER_SEC);
    fprintf(stderr, "duration  %f seconds \n",duration);
#endif
    return 0;
}
#endif
