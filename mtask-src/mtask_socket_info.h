#ifndef mtask_socket_info_h
#define mtask_socket_info_h

#define SOCKET_INFO_UNKNOWN     0
#define SOCKET_INFO_LISTEN      1
#define SOCKET_INFO_TCP         2
#define SOCKET_INFO_UDP         3
#define SOCKET_INFO_BIND        4

#include <stdint.h>

struct mtask_socket_info
{
    int id;
    int type;
    uint64_t opaque;
    uint64_t read;
    uint64_t write;
    uint64_t rtime;
    uint64_t wtime;
    int64_t wbuffer;
    char name[128];
    struct mtask_socket_info *next;
};

struct mtask_socket_info * mtask_socket_info_create(struct mtask_socket_info *last);

void mtask_socket_info_release(struct mtask_socket_info *);

#endif
