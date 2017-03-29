/* Copyright (C) 2004 MySQL AB
   Copyright (C) 2004-2017 Alexey Kopytov <akopytov@gmail.com>

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#ifdef _WIN32
#include "sb_win.h"
#endif

#include "sysbench.h"
#include "sb_rand.h"

#ifdef HAVE_SYS_IPC_H
# include <sys/ipc.h>
#endif

#ifdef HAVE_SYS_SHM_H
# include <sys/shm.h>
#endif

#include <inttypes.h>

#define LARGE_PAGE_SIZE (4UL * 1024 * 1024)

/* Memory test arguments */
static sb_arg_t memory_args[] =
{
  SB_OPT("memory-block-size", "size of memory block for test", "1K", SIZE),
  SB_OPT("memory-total-size", "total size of data to transfer", "100G", SIZE),
  SB_OPT("memory-scope", "memory access scope {global,local}", "global",
         STRING),
#ifdef HAVE_LARGE_PAGES
  SB_OPT("memory-hugetlb", "allocate memory from HugeTLB pool", "off", BOOL),
#endif
  SB_OPT("memory-oper", "type of memory operations {read, write, none}",
         "write", STRING),
  SB_OPT("memory-access-mode", "memory access mode {seq,rnd}", "seq", STRING),

  SB_OPT_END
};

/* Memory test operations */
static int memory_init(void);
static int memory_thread_init(int);
static void memory_print_mode(void);
static sb_event_t memory_next_event(int);
static int event_rnd_none(sb_event_t *, int);
static int event_rnd_read(sb_event_t *, int);
static int event_rnd_write(sb_event_t *, int);
static int event_seq_none(sb_event_t *, int);
static int event_seq_read(sb_event_t *, int);
static int event_seq_write(sb_event_t *, int);
static void memory_report_intermediate(sb_stat_t *);
static void memory_report_cumulative(sb_stat_t *);

static sb_test_t memory_test =
{
  .sname = "memory",
  .lname = "Memory functions speed test",
  .ops = {
    .init = memory_init,
    .thread_init = memory_thread_init,
    .print_mode = memory_print_mode,
    .next_event = memory_next_event,
    .report_intermediate = memory_report_intermediate,
    .report_cumulative = memory_report_cumulative
  },
  .args = memory_args
};

/* Test arguments */

static ssize_t memory_block_size;
static long long    memory_total_size;
static unsigned int memory_scope;
static unsigned int memory_oper;
static unsigned int memory_access_rnd;
#ifdef HAVE_LARGE_PAGES
static unsigned int memory_hugetlb;
#endif

static TLS uint64_t tls_total_ops CK_CC_CACHELINE;
static TLS size_t *tls_buf;
static TLS size_t *tls_buf_end;

/* Array of per-thread buffers */
static size_t **buffers;
/* Global buffer */
static size_t *buffer;

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
  if (memory_block_size < SIZEOF_SIZE_T ||
      /* Must be a power of 2 */
      (memory_block_size & (memory_block_size - 1)) != 0)
  {
    log_text(LOG_FATAL, "Invalid value for memory-block-size: %s",
             sb_get_value_string("memory-block-size"));
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
      buffer = hugetlb_alloc(memory_block_size);
    else
#endif
      buffer = sb_memalign(memory_block_size, sb_getpagesize());

    if (buffer == NULL)
    {
      log_text(LOG_FATAL, "Failed to allocate buffer!");
      return 1;
    }

    memset(buffer, 0, memory_block_size);
  }
  else
  {
    buffers = malloc(sb_globals.threads * sizeof(void *));
    if (buffers == NULL)
    {
      log_text(LOG_FATAL, "Failed to allocate buffers array!");
      return 1;
    }
    for (i = 0; i < sb_globals.threads; i++)
    {
#ifdef HAVE_LARGE_PAGES
      if (memory_hugetlb)
        buffers[i] = hugetlb_alloc(memory_block_size);
      else
#endif
        buffers[i] = sb_memalign(memory_block_size, sb_getpagesize());

      if (buffers[i] == NULL)
      {
        log_text(LOG_FATAL, "Failed to allocate buffer for thread #%d!", i);
        return 1;
      }

      memset(buffers[i], 0, memory_block_size);
    }
  }

  switch (memory_oper) {
  case SB_MEM_OP_NONE:
    memory_test.ops.execute_event =
      memory_access_rnd ? event_rnd_none : event_seq_none;
    break;

  case SB_MEM_OP_READ:
    memory_test.ops.execute_event =
      memory_access_rnd ? event_rnd_read : event_seq_read;
    break;

  case SB_MEM_OP_WRITE:
    memory_test.ops.execute_event =
      memory_access_rnd ? event_rnd_write : event_seq_write;
    break;

  default:
    log_text(LOG_FATAL, "Unknown memory request type: %d\n", memory_oper);
    return 1;
  }

  /* Use our own limit on the number of events */
  sb_globals.max_events = 0;

  return 0;
}


int memory_thread_init(int thread_id)
{
  (void) thread_id; /* unused */

  /* Initialize thread-local variables for each thread */

  if (memory_total_size > 0)
  {
    tls_total_ops = memory_total_size / memory_block_size / sb_globals.threads;
  }

  switch (memory_scope) {
  case SB_MEM_SCOPE_GLOBAL:
    tls_buf = buffer;
    break;
  case SB_MEM_SCOPE_LOCAL:
    tls_buf = buffers[thread_id];
    break;
  default:
    log_text(LOG_FATAL, "Invalid memory scope");
    return 1;
  }

  tls_buf_end = (size_t *) (void *) ((char *) tls_buf + memory_block_size);

  return 0;
}


sb_event_t memory_next_event(int thread_id)
{
  sb_event_t      req;

  (void) thread_id; /* unused */

  if (memory_total_size > 0 && !tls_total_ops--)
  {
    req.type = SB_REQ_TYPE_NULL;
    return req;
  }

  req.type = SB_REQ_TYPE_MEMORY;

  return req;
}

/*
  Use either 32- or 64-bit primitives depending on the native word
  size. ConcurrencyKit ensures the corresponding loads/stores are not optimized
  away by the compiler.
*/
#if SIZEOF_SIZE_T == 4
# define SIZE_T_LOAD(ptr) ck_pr_load_32((uint32_t *)(ptr))
# define SIZE_T_STORE(ptr,val) ck_pr_store_32((uint32_t *)(ptr),(uint32_t)(val))
#elif SIZEOF_SIZE_T == 8
# define SIZE_T_LOAD(ptr) ck_pr_load_64((uint64_t *)(ptr))
# define SIZE_T_STORE(ptr,val) ck_pr_store_64((uint64_t *)(ptr),(uint64_t)(val))
#else
# error Unsupported platform.
#endif

int event_rnd_none(sb_event_t *req, int thread_id)
{
  (void) req; /* unused */
  (void) thread_id; /* unused */

  for (ssize_t i = 0; i < memory_block_size; i += SIZEOF_SIZE_T)
  {
    size_t offset = (volatile size_t) (sb_rand_uniform_double() *
                                       (memory_block_size / SIZEOF_SIZE_T));
    (void) offset; /* unused */
    /* nop */
  }

  return 0;
}


int event_rnd_read(sb_event_t *req, int thread_id)
{
  (void) req; /* unused */
  (void) thread_id; /* unused */

  for (ssize_t i = 0; i < memory_block_size; i += SIZEOF_SIZE_T)
  {
    size_t offset = (size_t) (sb_rand_uniform_double() *
                              (memory_block_size / SIZEOF_SIZE_T));
    size_t val = SIZE_T_LOAD(tls_buf + offset);
    (void) val; /* unused */
  }

  return 0;
}


int event_rnd_write(sb_event_t *req, int thread_id)
{
  (void) req; /* unused */
  (void) thread_id; /* unused */

  for (ssize_t i = 0; i < memory_block_size; i += SIZEOF_SIZE_T)
  {
    size_t offset = (size_t) (sb_rand_uniform_double() *
                              (memory_block_size / SIZEOF_SIZE_T));
    SIZE_T_STORE(tls_buf + offset, i);
  }

  return 0;
}


int event_seq_none(sb_event_t *req, int thread_id)
{
  (void) req; /* unused */
  (void) thread_id; /* unused */

  for (size_t *buf = tls_buf, *end = buf + memory_block_size / SIZEOF_SIZE_T;
       buf < end; buf++)
  {
    ck_pr_barrier();
    /* nop */
  }

  return 0;
}


int event_seq_read(sb_event_t *req, int thread_id)
{
  (void) req; /* unused */
  (void) thread_id; /* unused */

  for (size_t *buf = tls_buf, *end = buf + memory_block_size / SIZEOF_SIZE_T;
       buf < end; buf++)
  {
    size_t val = SIZE_T_LOAD(buf);
    (void) val; /* unused */
  }

  return 0;
}


int event_seq_write(sb_event_t *req, int thread_id)
{
  (void) req; /* unused */
  (void) thread_id; /* unused */

  for (size_t *buf = tls_buf, *end = buf + memory_block_size / SIZEOF_SIZE_T;
       buf < end; buf++)
  {
    SIZE_T_STORE(buf, end - buf);
  }

  return 0;
}


void memory_print_mode(void)
{
  char *str;

  log_text(LOG_NOTICE, "Running memory speed test with the following options:");
  log_text(LOG_NOTICE, "  block size: %ldKiB",
           (long)(memory_block_size / 1024));
  log_text(LOG_NOTICE, "  total size: %ldMiB",
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
  log_text(LOG_NOTICE, "  operation: %s", str);

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
  log_text(LOG_NOTICE, "  scope: %s", str);

  log_text(LOG_NOTICE, "");
}

/*
  Print intermediate test statistics.
*/

void memory_report_intermediate(sb_stat_t *stat)
{
  const double megabyte = 1024.0 * 1024.0;

  log_timestamp(LOG_NOTICE, stat->time_total, "%4.2f MiB/sec",
                stat->events * memory_block_size / megabyte /
                stat->time_interval);
}

/*
  Print cumulative test statistics.
*/

void memory_report_cumulative(sb_stat_t *stat)
{
  const double megabyte = 1024.0 * 1024.0;

  log_text(LOG_NOTICE, "Total operations: %" PRIu64 " (%8.2f per second)\n",
           stat->events, stat->events / stat->time_interval);

  if (memory_oper != SB_MEM_OP_NONE)
  {
    const double mb = stat->events * memory_block_size / megabyte;
    log_text(LOG_NOTICE, "%4.2f MiB transferred (%4.2f MiB/sec)\n",
             mb, mb / stat->time_interval);
  }

  sb_report_cumulative(stat);
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
                "Failed to allocate %zd bytes from HugeTLB memory.", size);

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
