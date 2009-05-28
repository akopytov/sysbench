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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#ifdef _WIN32
#include "sb_win.h"
#endif

#include "sysbench.h"

#ifdef HAVE_SYS_IPC_H
# include <sys/ipc.h>
#endif

#ifdef HAVE_SYS_SHM_H
# include <sys/shm.h>
#endif

#define LARGE_PAGE_SIZE (4UL * 1024 * 1024)

/* Memory test arguments */
static sb_arg_t memory_args[] =
{
  {"memory-block-size", "size of memory block for test",
   SB_ARG_TYPE_SIZE, "1K"},
  {"memory-total-size", "total size of data to transfer",
   SB_ARG_TYPE_SIZE, "100G"},
  {"memory-scope", "memory access scope {global,local}", SB_ARG_TYPE_STRING,
   "global"},
#ifdef HAVE_LARGE_PAGES
  {"memory-hugetlb", "allocate memory from HugeTLB pool", SB_ARG_TYPE_FLAG, "off"},
#endif
  {"memory-oper", "type of memory operations {read, write, none}",
   SB_ARG_TYPE_STRING, "write"},
  {"memory-access-mode", "memory access mode {seq,rnd}", SB_ARG_TYPE_STRING, "seq"},
  {NULL, NULL, SB_ARG_TYPE_NULL, NULL}
};

/* Memory test operations */
static int memory_init(void);
static void memory_print_mode(void);
static sb_request_t memory_get_request(int);
static int memory_execute_request(sb_request_t *, int);
static void memory_print_stats(void);

static sb_test_t memory_test =
{
  "memory",
  "Memory functions speed test",
  {
    memory_init,
    NULL,
    NULL,
    memory_print_mode,
    memory_get_request,
    memory_execute_request,
    memory_print_stats,
    NULL,
    NULL,
    NULL
  },
  {
    NULL,
    NULL,
    NULL,
    NULL
  },
  memory_args,
  {NULL, NULL}
};

/* Test arguments */

static ssize_t memory_block_size;
static size_t   memory_total_size;
static unsigned int memory_scope;
static unsigned int memory_oper;
static unsigned int memory_access_rnd;
#ifdef HAVE_LARGE_PAGES
static unsigned int memory_hugetlb;
#endif

/* Statistics */
static unsigned int total_ops;
static size_t        total_bytes;

/* Array of per-thread buffers */
static int **buffers;
/* Global buffer */
static int *buffer;

#ifdef HAVE_LARGE_PAGES
static void * hugetlb_alloc(size_t size);
#endif

int register_test_memory(sb_list_t *tests)
{
  SB_LIST_ADD_TAIL(&memory_test.listitem, tests);

  return 0;
}


int memory_init(void)
{
  unsigned int i;
  char         *s;
  
  memory_block_size = sb_get_value_size("memory-block-size");
  if (memory_block_size % sizeof(int) != 0)
  {
    log_text(LOG_FATAL, "memory-block-size must be a multiple of %ld!", (long)sizeof(int));
    return 1;
  }
  memory_total_size = sb_get_value_size("memory-total-size");
  
  s = sb_get_value_string("memory-scope");
  if (!strcmp(s, "global"))
    memory_scope = SB_MEM_SCOPE_GLOBAL;
  else if (!strcmp(s, "local"))
    memory_scope = SB_MEM_SCOPE_LOCAL;
  else
  {
    log_text(LOG_FATAL, "Invalid value for memory-scope: %s", s);
    return 1;
  }

#ifdef HAVE_LARGE_PAGES
    memory_hugetlb = sb_get_value_flag("memory-hugetlb");
#endif  

  s = sb_get_value_string("memory-oper");
  if (!strcmp(s, "write"))
    memory_oper = SB_MEM_OP_WRITE;
  else if (!strcmp(s, "read"))
    memory_oper = SB_MEM_OP_READ;
  else if (!strcmp(s, "none"))
    memory_oper = SB_MEM_OP_NONE;
  else
  {
    log_text(LOG_FATAL, "Invalid value for memory-oper: %s", s);
    return 1;
  }

  s = sb_get_value_string("memory-access-mode");
  if (!strcmp(s, "seq"))
    memory_access_rnd = 0;
  else if (!strcmp(s, "rnd"))
    memory_access_rnd = 1;
  else
  {
    log_text(LOG_FATAL, "Invalid value for memory-access-mode: %s", s);
    return 1;
  }
  
  if (memory_scope == SB_MEM_SCOPE_GLOBAL)
  {
#ifdef HAVE_LARGE_PAGES
    if (memory_hugetlb)
      buffer = (int *)hugetlb_alloc(memory_block_size);
    else
#endif
    buffer = (int *)malloc(memory_block_size);
    if (buffer == NULL)
    {
      log_text(LOG_FATAL, "Failed to allocate buffer!");
      return 1;
    }

    memset(buffer, 0, memory_block_size);
  }
  else
  {
    buffers = (int **)malloc(sb_globals.num_threads * sizeof(char *));
    if (buffers == NULL)
    {
      log_text(LOG_FATAL, "Failed to allocate buffers array!");
      return 1;
    }
    for (i = 0; i < sb_globals.num_threads; i++)
    {
#ifdef HAVE_LARGE_PAGES
      if (memory_hugetlb)
        buffers[i] = (int *)hugetlb_alloc(memory_block_size);
      else
#endif
      buffers[i] = (int *)malloc(memory_block_size);
      if (buffers[i] == NULL)
      {
        log_text(LOG_FATAL, "Failed to allocate buffer for thread #%d!", i);
        return 1;
      }

      memset(buffers[i], 0, memory_block_size);
    }
  }
  
  return 0;
}


sb_request_t memory_get_request(int tid)
{
  sb_request_t      req;
  sb_mem_request_t  *mem_req = &req.u.mem_request;

  (void)tid; /* unused */
  
  SB_THREAD_MUTEX_LOCK();
  if (total_bytes >= memory_total_size)
  {
    req.type = SB_REQ_TYPE_NULL;
    SB_THREAD_MUTEX_UNLOCK();
    return req;
  }
  total_ops++;
  total_bytes += memory_block_size;
  SB_THREAD_MUTEX_UNLOCK();

  req.type = SB_REQ_TYPE_MEMORY;
  mem_req->block_size = memory_block_size;
  mem_req->scope = memory_scope;
  mem_req->type = memory_oper;

  return req;
}

int memory_execute_request(sb_request_t *sb_req, int thread_id)
{
  sb_mem_request_t    *mem_req = &sb_req->u.mem_request;
  int                 tmp = 0;
  int                 idx; 
  int                 *buf, *end;
  log_msg_t           msg;
  log_msg_oper_t      op_msg;
  long                i;
  unsigned int        rand;
  
  /* Prepare log message */
  msg.type = LOG_MSG_TYPE_OPER;
  msg.data = &op_msg;
  
  if (mem_req->scope == SB_MEM_SCOPE_GLOBAL)
    buf = buffer;
  else
    buf = buffers[thread_id];
  end = (int *)((char *)buf + memory_block_size);

  LOG_EVENT_START(msg, thread_id);

  if (memory_access_rnd)
  {
    rand = sb_rnd();
    switch (mem_req->type) {
      case SB_MEM_OP_WRITE:
        for (i = 0; i < memory_block_size; i++)
        {
          idx = (int)((double)rand / (double)SB_MAX_RND *
                      (double)(memory_block_size / sizeof(int)));
          buf[idx] = tmp;
        }
        break;
      case SB_MEM_OP_READ:
        for (i = 0; i < memory_block_size; i++)
        {
          idx = (int)((double)rand / (double)SB_MAX_RND *
                      (double)(memory_block_size / sizeof(int)));
          tmp = buf[idx];
        }
        break;
      default:
        log_text(LOG_FATAL, "Unknown memory request type:%d. Aborting...\n",
                 mem_req->type);
        return 1;
    }
  }
  else
  {
    switch (mem_req->type) {
      case SB_MEM_OP_NONE:
        for (; buf < end; buf++)
          tmp = end - buf;
        break;
      case SB_MEM_OP_WRITE:
        for (; buf < end; buf++)
          *buf = tmp;
        break;
      case SB_MEM_OP_READ:
        for (; buf < end; buf++)
          tmp = *buf;
        break;
      default:
        log_text(LOG_FATAL, "Unknown memory request type:%d. Aborting...\n",
                 mem_req->type);
        return 1;
    }
  }
  
  LOG_EVENT_STOP(msg, thread_id);

  return 0;
}


void memory_print_mode(void)
{
  char *str;
  
  log_text(LOG_INFO, "Doing memory operations speed test");
  log_text(LOG_INFO, "Memory block size: %ldK\n",
           (long)(memory_block_size / 1024));
  log_text(LOG_INFO, "Memory transfer size: %ldM\n",
           (long)(memory_total_size / 1024 / 1024));

  switch (memory_oper) {
    case SB_MEM_OP_READ:
      str = "read";
      break;
    case SB_MEM_OP_WRITE:
      str = "write";
      break;
    case SB_MEM_OP_NONE:
      str = "none";
      break;
    default:
      str = "(unknown)";
      break;
  }
  log_text(LOG_INFO, "Memory operations type: %s", str);

  switch (memory_scope) {
    case SB_MEM_SCOPE_GLOBAL:
      str = "global";
      break;
    case SB_MEM_SCOPE_LOCAL:
      str = "local";
      break;
    default:
      str = "(unknown)";
      break;
  }
  log_text(LOG_INFO, "Memory scope type: %s", str);
}


void memory_print_stats(void)
{
  double total_time;

  total_time = NS2SEC(sb_timer_value(&sb_globals.exec_timer));
  
  log_text(LOG_NOTICE, "Operations performed: %d (%8.2f ops/sec)\n", total_ops,
           total_ops / total_time);
  if (memory_oper != SB_MEM_OP_NONE)
    log_text(LOG_NOTICE, "%4.2f MB transferred (%4.2f MB/sec)\n",
             (double)total_bytes / (1024 * 1024),
             (double)total_bytes / (1024 * 1024) / total_time);
}

#ifdef HAVE_LARGE_PAGES

/* Allocate memory from HugeTLB pool */

void * hugetlb_alloc(size_t size)
{
  int shmid;
  void *ptr;
  struct shmid_ds buf;

  /* Align block size to my_large_page_size */
  size = ((size - 1) & ~LARGE_PAGE_SIZE) + LARGE_PAGE_SIZE;

  shmid = shmget(IPC_PRIVATE, size, SHM_HUGETLB | SHM_R | SHM_W);
  if (shmid < 0)
  {
      log_errno(LOG_FATAL,
                "Failed to allocate %d bytes from HugeTLB memory.", size);

      return NULL;
  }

  ptr = shmat(shmid, NULL, 0);
  if (ptr == (void *)-1)
  {
    log_errno(LOG_FATAL, "Failed to attach shared memory segment,");
    shmctl(shmid, IPC_RMID, &buf);

    return NULL;
  }

  /*
        Remove the shared memory segment so that it will be automatically freed
            after memory is detached or process exits
  */
  shmctl(shmid, IPC_RMID, &buf);

  return ptr;
}

#endif
