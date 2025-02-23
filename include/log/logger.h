/*
**
** Copyright 2007-2014, The Android Open Source Project
**
** This file is dual licensed.  It may be redistributed and/or modified
** under the terms of the Apache 2.0 License OR version 2 of the GNU
** General Public License.
*/

#ifndef _LIBS_LOG_LOGGER_H
#define _LIBS_LOG_LOGGER_H

#include <stdint.h>
#ifdef __linux__
#include <time.h> /* clockid_t definition */
#endif

#include <log/log.h>
#include <log/log_read.h>
#include <log/align.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The userspace structure for version 1 of the logger_entry ABI.
 * This structure is returned to userspace by the kernel logger
 * driver unless an upgrade to a newer ABI version is requested.
 */
struct logger_entry {
#pragma pack(push, 1)
    uint16_t    len;    /* length of the payload */
    uint16_t    __pad;  /* no matter what, we get 2 bytes of padding */
    int32_t     pid;    /* generating process's pid */
    int32_t     tid;    /* generating process's tid */
    int32_t     sec;    /* seconds since Epoch */
    int32_t     nsec;   /* nanoseconds */
    char        msg[0]; /* the entry's payload */
#pragma pack(pop)
};

/*
 * The userspace structure for version 2 of the logger_entry ABI.
 * This structure is returned to userspace if ioctl(LOGGER_SET_VERSION)
 * is called with version==2; or used with the user space log daemon.
 */
struct logger_entry_v2 {
#pragma pack(push, 1)
    uint16_t    len;       /* length of the payload */
    uint16_t    hdr_size;  /* sizeof(struct logger_entry_v2) */
    int32_t     pid;       /* generating process's pid */
    int32_t     tid;       /* generating process's tid */
    int32_t     sec;       /* seconds since Epoch */
    int32_t     nsec;      /* nanoseconds */
    uint32_t    euid;      /* effective UID of logger */
    char        msg[0];    /* the entry's payload */
#pragma pack(pop)
};

struct logger_entry_v3 {
#pragma pack(push, 1)
    uint16_t    len;       /* length of the payload */
    uint16_t    hdr_size;  /* sizeof(struct logger_entry_v3) */
    int32_t     pid;       /* generating process's pid */
    int32_t     tid;       /* generating process's tid */
    int32_t     sec;       /* seconds since Epoch */
    int32_t     nsec;      /* nanoseconds */
    uint32_t    lid;       /* log id of the payload */
    char        msg[0];    /* the entry's payload */
#pragma pack(pop)
};

struct logger_entry_v4 {
#pragma pack(push, 1)
    uint16_t    len;       /* length of the payload */
    uint16_t    hdr_size;  /* sizeof(struct logger_entry_v4) */
    int32_t     pid;       /* generating process's pid */
    uint32_t    tid;       /* generating process's tid */
    uint32_t    sec;       /* seconds since Epoch */
    uint32_t    nsec;      /* nanoseconds */
    uint32_t    lid;       /* log id of the payload, bottom 4 bits currently */
    uint32_t    uid;       /* generating process's uid */
    char        msg[0];    /* the entry's payload */
#pragma pack(pop)
};

/*
 * The maximum size of the log entry payload that can be
 * written to the logger. An attempt to write more than
 * this amount will result in a truncated log entry.
 */
#define LOGGER_ENTRY_MAX_PAYLOAD	4068

/*
 * The maximum size of a log entry which can be read from the
 * kernel logger driver. An attempt to read less than this amount
 * may result in read() returning EINVAL.
 */
#define LOGGER_ENTRY_MAX_LEN		(5*1024)

#define NS_PER_SEC 1000000000ULL

struct log_msg {
    union ALIGN(4) {
        unsigned char buf[LOGGER_ENTRY_MAX_LEN + 1];
        struct logger_entry_v4 entry;
        struct logger_entry_v4 entry_v4;
        struct logger_entry_v3 entry_v3;
        struct logger_entry_v2 entry_v2;
        struct logger_entry    entry_v1;
    };
#ifdef __cplusplus
    /* Matching log_time operators */
    bool operator== (const log_msg &T) const
    {
        return (entry.sec == T.entry.sec) && (entry.nsec == T.entry.nsec);
    }
    bool operator!= (const log_msg &T) const
    {
        return !(*this == T);
    }
    bool operator< (const log_msg &T) const
    {
        return (entry.sec < T.entry.sec)
            || ((entry.sec == T.entry.sec)
             && (entry.nsec < T.entry.nsec));
    }
    bool operator>= (const log_msg &T) const
    {
        return !(*this < T);
    }
    bool operator> (const log_msg &T) const
    {
        return (entry.sec > T.entry.sec)
            || ((entry.sec == T.entry.sec)
             && (entry.nsec > T.entry.nsec));
    }
    bool operator<= (const log_msg &T) const
    {
        return !(*this > T);
    }
    uint64_t nsec() const
    {
        return static_cast<uint64_t>(entry.sec) * NS_PER_SEC + entry.nsec;
    }

    /* packet methods */
    log_id_t id()
    {
        return (log_id_t) entry.lid;
    }
    char *msg()
    {
        return entry.hdr_size ? (char *) buf + entry.hdr_size : entry_v1.msg;
    }
    unsigned int len()
    {
        return (entry.hdr_size ? entry.hdr_size : sizeof(entry_v1)) + entry.len;
    }
#endif
};

struct logger;

log_id_t android_logger_get_id(struct logger *logger);

int android_logger_clear(struct logger *logger);
long android_logger_get_log_size(struct logger *logger);
int android_logger_set_log_size(struct logger *logger, unsigned long size);
long android_logger_get_log_readable_size(struct logger *logger);
int android_logger_get_log_version(struct logger *logger);

struct logger_list;

#ifdef _WIN32
#include <windows.h>
typedef SSIZE_T ssize_t;
#endif

ssize_t android_logger_get_statistics(struct logger_list *logger_list,
                                      char *buf, size_t len);
ssize_t android_logger_get_prune_list(struct logger_list *logger_list,
                                      char *buf, size_t len);
int android_logger_set_prune_list(struct logger_list *logger_list,
                                  char *buf, size_t len);

#define ANDROID_LOG_RDONLY   O_RDONLY
#define ANDROID_LOG_WRONLY   O_WRONLY
#define ANDROID_LOG_RDWR     O_RDWR
#define ANDROID_LOG_ACCMODE  O_ACCMODE
#define ANDROID_LOG_NONBLOCK O_NONBLOCK
#define ANDROID_LOG_WRAP     0x40000000 /* Block until buffer about to wrap */
#define ANDROID_LOG_WRAP_DEFAULT_TIMEOUT 7200 /* 2 hour default */
#define ANDROID_LOG_PSTORE   0x80000000

#ifdef _WIN32
typedef DWORD pid_t;
#endif

struct logger_list *android_logger_list_alloc(int mode,
                                              unsigned int tail,
                                              pid_t pid);
struct logger_list *android_logger_list_alloc_time(int mode,
                                                   log_time start,
                                                   pid_t pid);
void android_logger_list_free(struct logger_list *logger_list);
/* In the purest sense, the following two are orthogonal interfaces */
int android_logger_list_read(struct logger_list *logger_list,
                             struct log_msg *log_msg);

/* Multiple log_id_t opens */
struct logger *android_logger_open(struct logger_list *logger_list,
                                   log_id_t id);
#define android_logger_close android_logger_free
/* Single log_id_t open */
struct logger_list *android_logger_list_open(log_id_t id,
                                             int mode,
                                             unsigned int tail,
                                             pid_t pid);
#define android_logger_list_close android_logger_list_free

#ifdef __linux__
clockid_t android_log_clockid();
#endif

/*
 * log_id_t helpers
 */
log_id_t android_name_to_log_id(const char *logName);
const char *android_log_id_to_name(log_id_t log_id);

#ifdef __cplusplus
}
#endif

#endif /* _LIBS_LOG_LOGGER_H */
