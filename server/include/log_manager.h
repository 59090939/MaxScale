/*
 * Copyright (c) 2016 MariaDB Corporation Ab
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl.
 *
 * Change Date: 2019-07-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */
#if !defined(LOG_MANAGER_H)
#define LOG_MANAGER_H

#include <stdbool.h>
#include <syslog.h>
#include <unistd.h>
#include <platform.h>
#include <locale.h>
#include <string.h>

#if defined(__cplusplus)
extern "C" {
#endif

/** strerror_r buffer size, deprecated */
#ifndef STRERROR_BUFLEN
#define STRERROR_BUFLEN 512
#endif

/**
 * If MXS_MODULE_NAME is defined before log_manager.h is included, then all
 * logged messages will be prefixed with that string enclosed in square brackets.
 * For instance, the following
 *
 *     #define MXS_MODULE_NAME "xyz"
 *     #include <log_manager.h>
 *
 * will lead to every logged message looking like:
 *
 *     2016-08-12 13:49:11   error : [xyz] The gadget was not ready
 *
 * In general, the value of MXS_MODULE_NAME should be the name of the shared
 * library to which the source file, where MXS_MODULE_NAME is defined, belongs.
 *
 * Note that a file that is compiled into multiple modules should
 * have MXS_MODULE_NAME defined as something else than the name of a real
 * module, or not at all.
 *
 * Any file that is compiled into maxscale-common should *not* have
 * MXS_MODULE_NAME defined.
 */
#if !defined(MXS_MODULE_NAME)
#define MXS_MODULE_NAME NULL
#endif

enum mxs_log_priorities
{
    MXS_LOG_EMERG   = (1 << LOG_EMERG),
    MXS_LOG_ALERT   = (1 << LOG_ALERT),
    MXS_LOG_CRIT    = (1 << LOG_CRIT),
    MXS_LOG_ERR     = (1 << LOG_ERR),
    MXS_LOG_WARNING = (1 << LOG_WARNING),
    MXS_LOG_NOTICE  = (1 << LOG_NOTICE),
    MXS_LOG_INFO    = (1 << LOG_INFO),
    MXS_LOG_DEBUG   = (1 << LOG_DEBUG),

    MXS_LOG_MASK    = (MXS_LOG_EMERG | MXS_LOG_ALERT | MXS_LOG_CRIT | MXS_LOG_ERR |
                       MXS_LOG_WARNING | MXS_LOG_NOTICE | MXS_LOG_INFO | MXS_LOG_DEBUG),
};

typedef enum
{
    MXS_LOG_TARGET_DEFAULT = 0,
    MXS_LOG_TARGET_FS      = 1, // File system
    MXS_LOG_TARGET_SHMEM   = 2, // Shared memory
    MXS_LOG_TARGET_STDOUT  = 3, // Standard output
} mxs_log_target_t;

/**
* Thread-specific logging information.
*/
typedef struct mxs_log_info
{
    size_t li_sesid;
    int    li_enabled_priorities;
} mxs_log_info_t;

extern int mxs_log_enabled_priorities;
extern ssize_t mxs_log_session_count[];
extern __thread mxs_log_info_t mxs_log_tls;

/**
 * Check if specified log type is enabled in general or if it is enabled
 * for the current session.
 */
#define MXS_LOG_PRIORITY_IS_ENABLED(priority) \
    (((mxs_log_enabled_priorities & (1 << priority)) ||      \
      (mxs_log_session_count[priority] > 0 && \
       mxs_log_tls.li_enabled_priorities & (1 << priority))) ? true : false)

/**
 * LOG_AUGMENT_WITH_FUNCTION Each logged line is suffixed with [function-name].
 */
typedef enum
{
    MXS_LOG_AUGMENT_WITH_FUNCTION = 1,
    MXS_LOG_AUGMENTATION_MASK     = (MXS_LOG_AUGMENT_WITH_FUNCTION)
} mxs_log_augmentation_t;

typedef struct mxs_log_throttling
{
    size_t count;       // Maximum number of a specific message...
    size_t window_ms;   // ...during this many milliseconds.
    size_t suppress_ms; // If exceeded, suppress such messages for this many ms.
} MXS_LOG_THROTTLING;

/** Use this instead of calling strerror[_r] directly */
#define mxs_strerror(A) strerror_l(A, uselocale(0))

bool mxs_log_init(const char* ident, const char* logdir, mxs_log_target_t target);
void mxs_log_finish(void);

int mxs_log_flush();
int mxs_log_flush_sync();
int mxs_log_rotate();

int  mxs_log_set_priority_enabled(int priority, bool enabled);
void mxs_log_set_syslog_enabled(bool enabled);
void mxs_log_set_maxlog_enabled(bool enabled);
void mxs_log_set_highprecision_enabled(bool enabled);
void mxs_log_set_augmentation(int bits);
void mxs_log_set_throttling(const MXS_LOG_THROTTLING* throttling);

void mxs_log_get_throttling(MXS_LOG_THROTTLING* throttling);

int mxs_log_message(int priority,
                    const char* modname,
                    const char* file, int line, const char* function,
                    const char* format, ...) __attribute__((format(printf, 6, 7)));
/**
 * Log an error, warning, notice, info, or debug  message.
 *
 * @param priority One of the syslog constants (LOG_ERR, LOG_WARNING, ...)
 * @param format   The printf format of the message.
 * @param ...      Arguments, depending on the format.
 *
 * NOTE: Should typically not be called directly. Use some of the
 *       MXS_ERROR, MXS_WARNING, etc. macros instead.
 */
#define MXS_LOG_MESSAGE(priority, format, ...)\
    mxs_log_message(priority, MXS_MODULE_NAME, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/**
 * Log an error, warning, notice, info, or debug  message.
 *
 * @param format The printf format of the message.
 * @param ...    Arguments, depending on the format.
 */
#define MXS_ERROR(format, ...)   MXS_LOG_MESSAGE(LOG_ERR,     format, ##__VA_ARGS__)
#define MXS_WARNING(format, ...) MXS_LOG_MESSAGE(LOG_WARNING, format, ##__VA_ARGS__)
#define MXS_NOTICE(format, ...)  MXS_LOG_MESSAGE(LOG_NOTICE,  format, ##__VA_ARGS__)
#define MXS_INFO(format, ...)    MXS_LOG_MESSAGE(LOG_INFO,    format, ##__VA_ARGS__)
#define MXS_DEBUG(format, ...)   MXS_LOG_MESSAGE(LOG_DEBUG,   format, ##__VA_ARGS__)

/**
 * Log an out of memory error using custom message.
 *
 * @param message Text to be logged.
 */
// TODO: In an OOM situation, the default logging will (most likely) *not* work,
// TODO: as memory is allocated as part of the process. A custom route, that does
// TODO: not allocate memory, must be created for OOM messages.
// TODO: So, currently these are primarily placeholders.
#define MXS_OOM_MESSAGE(message) MXS_ERROR("OOM: %s", message);

/**
 * Log an out of memory error using custom message, if the
 * provided pointer is NULL.
 *
 * @param p If NULL, an OOM message will be logged.
 * @param message Text to be logged.
 */
#define MXS_OOM_MESSAGE_IFNULL(p, m) do { if (!p) { MXS_OOM_MESSAGE(m); } } while (false)

/**
 * Log an out of memory error using a default message.
 */
#define MXS_OOM() MXS_OOM_MESSAGE(__func__)

/**
 * Log an out of memory error using a default message, if the
 * provided pointer is NULL.
 *
 * @param p If NULL, an OOM message will be logged.
 */
#define MXS_OOM_IFNULL(p) do { if (!p) { MXS_OOM(); } } while (false)

enum
{
    MXS_OOM_MESSAGE_MAXLEN = 80 /** Maximum length of an OOM message, including the
                                    trailing NULL. If longer, it will be cut. */
};

#if defined(__cplusplus)
}
#endif

#endif /** LOG_MANAGER_H */
