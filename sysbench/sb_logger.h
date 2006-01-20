/* Copyright (C) 2004 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef SB_LOGGER_H
#define SB_LOGGER_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "sb_options.h"
#include "sb_timer.h"

/* Macros to log per-request execution statistics */

#define LOG_EVENT_START(msg, thread_id) \
  do \
  { \
    sb_timer_start(&sb_globals.op_timers[thread_id]); \
    ((log_msg_oper_t *)(msg).data)->action = LOG_MSG_OPER_START; \
    log_msg(&(msg)); \
  } while (0);

#define LOG_EVENT_STOP(msg, thread_id) \
  do \
  { \
    ((log_msg_oper_t *)(msg).data)->action = LOG_MSG_OPER_STOP; \
    log_msg(&(msg)); \
    sb_timer_stop(&sb_globals.op_timers[thread_id]); \
  } while (0);

/* Message types definition */

typedef enum {
  LOG_MSG_TYPE_MIN,           /* used for internal purposes */
  LOG_MSG_TYPE_TEXT,          /* arbitrary text messages */
  LOG_MSG_TYPE_OPER,          /* operation start/stop messages */
  LOG_MSG_TYPE_MAX            /* used for internal purposes */
} log_msg_type_t;

/* Message priorities definition */

typedef enum {
  LOG_FATAL,     /* system is unusable */
  LOG_ALERT,     /* user actions must be taken */
  LOG_WARNING,   /* warnings */
  LOG_NOTICE,    /* normal but significant messages */
  LOG_INFO,      /* informational messages */   
  LOG_DEBUG,     /* debug-level messages */
  LOG_MAX        /* used for internal purposes */
} log_msg_priority_t;

/* Text message definition */

typedef struct {
  log_msg_priority_t priority;
  char              *text;
} log_msg_text_t;

/* Operation start/stop message definition */

typedef enum {
  LOG_MSG_OPER_START,
  LOG_MSG_OPER_STOP
} log_msg_oper_action_t;

typedef struct {
  log_msg_oper_action_t action;
  sb_timer_t            timer;
} log_msg_oper_t;

/* General log message definition */

typedef struct {
  log_msg_type_t type;
  void           *data;
} log_msg_t;

/* Log handler operations definition */

typedef int log_op_register(void);          /* register handler options */
typedef int log_op_init(void);              /* initialize log handler */
typedef int log_op_process(log_msg_t *msg); /* process message */
typedef int log_op_done(void);              /* uninitialize log handler */

/* Log handler operations structure */

typedef struct {
  log_op_init     *init;
  log_op_process  *process;
  log_op_done     *done;
} log_handler_ops_t;

/* Log handler definition */

typedef struct {
  log_handler_ops_t ops;          /* handler operations */
  sb_arg_t             *args;     /* handler arguments */
  
  sb_list_item_t       listitem;  /* can be linked in a list */
} log_handler_t;

/* Register logger */

int log_register(void);

/* Initialize logger */

int log_init(void);

/* Add handler for a specified type of messages */

int log_add_handler(log_msg_type_t type, log_handler_t *handler);

/* Main function to dispatch log messages */

void log_msg(log_msg_t *);

/* printf-like wrapper to log text messages */

void log_text(log_msg_priority_t priority, const char *fmt, ...);

/* printf-like wrapper to log system error messages */

void log_errno(log_msg_priority_t priority, const char *fmt, ...);

/* Uninitialize logger */

void log_done(void);

#endif /* SB_LOGGER_H */
