/* Copyright (C) 2004 MySQL AB
   Copyright (C) 2004-2018 Alexey Kopytov <akopytov@gmail.com>

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

#ifdef STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
# include <inttypes.h>
#endif

#ifdef HAVE_UNISTD_H 
# include <unistd.h>
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif
#ifdef HAVE_LIBAIO
# include <libaio.h>
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#ifdef _WIN32
# include <io.h>
# include <fcntl.h>
# include <sys/stat.h>
# define S_IRUSR _S_IREAD
# define S_IWUSR _S_IWRITE
# define HAVE_MMAP
#endif

#include "sysbench.h"
#include "crc32.h"
#include "sb_histogram.h"
#include "sb_rand.h"
#include "sb_util.h"

/* Lengths of the checksum and the offset fields in a block */
#define FILE_CHECKSUM_LENGTH sizeof(int)
#define FILE_OFFSET_LENGTH sizeof(long)

#ifdef _WIN32
typedef HANDLE FILE_DESCRIPTOR;
#define VALID_FILE(fd) (fd != INVALID_HANDLE_VALUE)
#define SB_INVALID_FILE INVALID_HANDLE_VALUE
#define FD_FMT "%p"
#define MAP_SHARED 0
#define PROT_READ  1
#define PROT_WRITE 2
#define MAP_FAILED NULL

void *mmap(void *addr, size_t len, int prot, int flags,
            FILE_DESCRIPTOR fd, long long off);
int munmap(void *addr, size_t size);
#else
typedef int FILE_DESCRIPTOR;
#define VALID_FILE(fd) (fd >= 0)
#define SB_INVALID_FILE (-1)
#define FD_FMT "%d"
#endif

/* Supported operations in request */
typedef enum
{
  MODE_READ,
  MODE_WRITE,
  MODE_REWRITE,
  MODE_RND_READ,
  MODE_RND_WRITE,
  MODE_RND_RW,
  MODE_MIXED
} file_test_mode_t;

/* fsync modes */
typedef enum
{
  FSYNC_ALL,
  FSYNC_DATA
} file_fsync_mode_t;

/* File I/O modes */
typedef enum
{
  FILE_IO_MODE_SYNC,
  FILE_IO_MODE_ASYNC,
  FILE_IO_MODE_MMAP
} file_io_mode_t;

typedef enum {
  SB_FILE_FLAG_SYNC = 1,
  SB_FILE_FLAG_DSYNC = 2,
  SB_FILE_FLAG_DIRECTIO = 4
} file_flags_t;

#ifdef HAVE_LIBAIO
/* Per-thread async I/O context */
typedef struct
{
  io_context_t    io_ctxt;      /* AIO context */
  unsigned int    nrequests;    /* Current number of queued I/O requests */
  struct io_event *events;      /* Array of events */
} sb_aio_context_t;

/* Async I/O operation */
typedef struct
{
  struct iocb   iocb; 
  sb_file_op_t  type;
  ssize_t       len;
} sb_aio_oper_t;

static sb_aio_context_t *aio_ctxts;
#endif

typedef struct
{
  void           *buffer;
  unsigned int    buffer_file_id;
  long long       buffer_pos;
} sb_per_thread_t;

static sb_per_thread_t	*per_thread;

/* Test options */
static unsigned int      num_files;
static long long         total_size;
static long long         file_size;
static int               file_block_size;
static file_flags_t      file_extra_flags;
static int               file_fsync_freq;
static int               file_fsync_all;
static int               file_fsync_end;
static file_fsync_mode_t file_fsync_mode;
static float             file_rw_ratio;
static int               file_merged_requests;
static long long         file_request_size;
static file_io_mode_t    file_io_mode;
#ifdef HAVE_LIBAIO
static unsigned int      file_async_backlog;
#endif

/* statistical and other "local" variables */
static long long       position;      /* current position in file */
static unsigned int    current_file;  /* current file */
static unsigned int    fsynced_file;  /* file number to be fsynced (periodic) */

static int is_dirty;               /* any writes after last fsync series ? */
static int read_ops;
static int write_ops;
static int other_ops;
static int last_other_ops;
static unsigned int req_performed; /* number of requests done */
static unsigned long long bytes_read;
static unsigned long long last_bytes_read;
static unsigned long long bytes_written;
static unsigned long long last_bytes_written;

static const double megabyte = 1024.0 * 1024.0;

#ifdef HAVE_MMAP
/* Array of file mappings */
static void          **mmaps;
static unsigned long file_page_mask;
#endif

/* Array of file descriptors */
static FILE_DESCRIPTOR *files;

/* test mode type */
static file_test_mode_t test_mode;

/* Previous request needed for validation */
static sb_file_request_t prev_req;

static sb_arg_t fileio_args[] = {
  SB_OPT("file-num", "number of files to create", "128", INT),
  SB_OPT("file-block-size", "block size to use in all IO operations", "16384",
         INT),
  SB_OPT("file-total-size", "total size of files to create", "2G", SIZE),
  SB_OPT("file-test-mode",
         "test mode {seqwr, seqrewr, seqrd, rndrd, rndwr, rndrw}", NULL,
         STRING),
  SB_OPT("file-io-mode", "file operations mode {sync,async,mmap}", "sync",
         STRING),
#ifdef HAVE_LIBAIO
  SB_OPT("file-async-backlog",
         "number of asynchronous operatons to queue per thread", "128", INT),
#endif
  SB_OPT("file-extra-flags",
         "list of additional flags to use to open files {sync,dsync,direct}",
         "", LIST),
  SB_OPT("file-fsync-freq", "do fsync() after this number of requests "
         "(0 - don't use fsync())", "100", INT),
  SB_OPT("file-fsync-all", "do fsync() after each write operation", "off",
         BOOL),
  SB_OPT("file-fsync-end", "do fsync() at the end of test", "on", BOOL),
  SB_OPT("file-fsync-mode",
         "which method to use for synchronization {fsync, fdatasync}",
         "fsync", STRING),
  SB_OPT("file-merged-requests", "merge at most this number of IO requests "
         "if possible (0 - don't merge)", "0", INT),
  SB_OPT("file-rw-ratio", "reads/writes ratio for combined test", "1.5", DOUBLE),

  SB_OPT_END
};

/* fileio test commands */
static int file_cmd_prepare(void);
static int file_cmd_cleanup(void);

/* fileio test operations */
static int file_init(void);
static void file_print_mode(void);
static int file_prepare(void);
static sb_event_t file_next_event(int thread_id);
static int file_execute_event(sb_event_t *, int);
static int file_thread_done(int);
static int file_done(void);
static void file_report_intermediate(sb_stat_t *);
static void file_report_cumulative(sb_stat_t *);

static sb_test_t fileio_test =
{
  .sname = "fileio",
  .lname = "File I/O test",
  .ops = {
    .init = file_init,
    .prepare = file_prepare,
    .print_mode = file_print_mode,
    .next_event = file_next_event,
    .execute_event = file_execute_event,
    .report_intermediate = file_report_intermediate,
    .report_cumulative = file_report_cumulative,
    .thread_done = file_thread_done,
    .done = file_done
  },
  .builtin_cmds = {
   .prepare = file_cmd_prepare,
   .cleanup = file_cmd_cleanup
  },
  .args = fileio_args
};


static int create_files(void);
static int remove_files(void);
static int parse_arguments(void);
static void clear_stats(void);
static void init_vars(void);
static sb_event_t file_get_seq_request(void);
static sb_event_t file_get_rnd_request(int thread_id);
static void check_seq_req(sb_file_request_t *, sb_file_request_t *);
static const char *get_io_mode_str(file_io_mode_t mode);
static const char *get_test_mode_str(file_test_mode_t mode);
static void file_fill_buffer(unsigned char *, unsigned int, size_t);
static int file_validate_buffer(unsigned char  *, unsigned int, size_t);

/* File operation wrappers */
static int file_do_fsync(unsigned int, int);
static int file_fsync(unsigned int, int);
static ssize_t file_pread(unsigned int, void *, ssize_t, long long, int);
static ssize_t file_pwrite(unsigned int, void *, ssize_t, long long, int);
#ifdef HAVE_LIBAIO
static int file_async_init(void);
static int file_async_done(void);
static int file_submit_or_wait(struct iocb *, sb_file_op_t, ssize_t, int);
static int file_wait(int, long);
#endif
#ifdef HAVE_MMAP
static int file_mmap_prepare(void);
static int file_mmap_done(void);
#endif

/* Portability wrappers */
static unsigned long sb_get_allocation_granularity(void);
static void sb_free_memaligned(void *buf);
static FILE_DESCRIPTOR sb_open(const char *);
static int sb_create(const char *);

int register_test_fileio(sb_list_t *tests)
{
  SB_LIST_ADD_TAIL(&fileio_test.listitem, tests);

  return 0;
}


int file_init(void)
{
  if (parse_arguments())
    return 1;
  
  files = (FILE_DESCRIPTOR *)malloc(num_files * sizeof(FILE_DESCRIPTOR));
  if (files == NULL)
  {
    log_text(LOG_FATAL, "Memory allocation failure.");
    return 1;
  }

#ifdef HAVE_LIBAIO
  if (file_async_init())
    return 1;
#endif

  init_vars();
  clear_stats();

  return 0;
}


int file_prepare(void)
{
  unsigned int  i;
  char          file_name[512];

  for (i=0; i < num_files; i++)
  {
    snprintf(file_name, sizeof(file_name), "test_file.%d",i);
    /* remove test files for creation test if they exist */
    if (test_mode == MODE_WRITE)
    {
      unlink(file_name);
      if (sb_create(file_name))
      {
        log_errno(LOG_FATAL, "Cannot create file '%s'", file_name);
        return 1;
      }
    }

    log_text(LOG_DEBUG, "Opening file: %s", file_name);
    files[i] = sb_open(file_name);
    if (!VALID_FILE(files[i]))
    {
      log_errno(LOG_FATAL, "Cannot open file '%s'", file_name);
      log_text(LOG_WARNING, "Did you forget to run the prepare step?");
      return 1;
    }

    if (test_mode == MODE_WRITE)
      continue;

    /* Validate file size */
    struct stat buf;
    if (fstat(files[i], &buf))
    {
      log_errno(LOG_FATAL, "fstat() failed on file '%s'", file_name);
      return 1;
    }
    if (buf.st_size < file_size)
    {
      char ss1[16], ss2[16];
      log_text(LOG_FATAL,
               "Size of file '%s' is %sB, but at least %sB is expected.",
               file_name,
               sb_print_value_size(ss1, sizeof(ss1), buf.st_size),
               sb_print_value_size(ss2, sizeof(ss2), file_size));
      log_text(LOG_WARNING,
               "Did you run 'prepare' with different --file-total-size or "
               "--file-num values?");
      return 1;
    }
  }

#ifdef HAVE_MMAP
  if (file_mmap_prepare())
    return 1;
#endif

  return 0; 
}


int file_done(void)
{
  unsigned int  i;
  
  for (i = 0; i < num_files; i++)
#ifndef _WIN32
    close(files[i]);
#else
    CloseHandle(files[i]);
#endif

#ifdef HAVE_LIBAIO
  if (file_async_done())
    return 1;
#endif

#ifdef HAVE_MMAP
  if (file_mmap_done())
    return 1;
#endif

  for (i = 0; i < sb_globals.threads; i++)
  {
    if (per_thread[i].buffer != NULL)
      sb_free_memaligned(per_thread[i].buffer);
  }

  free(per_thread);

  return 0;
}

sb_event_t file_next_event(int thread_id)
{
  if (test_mode == MODE_WRITE || test_mode == MODE_REWRITE ||
      test_mode == MODE_READ)
    return file_get_seq_request();
  
  
  return file_get_rnd_request(thread_id);
}


/* Get sequential read or write request */


sb_event_t file_get_seq_request(void)
{
  sb_event_t           sb_req;
  sb_file_request_t    *file_req = &sb_req.u.file_request;

  sb_req.type = SB_REQ_TYPE_FILE;
  SB_THREAD_MUTEX_LOCK();
  
  /* assume function is called with correct mode always */
  if (test_mode == MODE_WRITE || test_mode == MODE_REWRITE)
    file_req->operation = FILE_OP_TYPE_WRITE;
  else     
    file_req->operation = FILE_OP_TYPE_READ;

  /* See whether it's time to fsync file(s) */
  if (file_fsync_freq != 0 && file_req->operation == FILE_OP_TYPE_WRITE &&
      is_dirty && req_performed % file_fsync_freq == 0)
  {
    file_req->operation = FILE_OP_TYPE_FSYNC;
    file_req->file_id = fsynced_file;
    file_req->pos = 0;
    file_req->size = 0;
    fsynced_file++;
    if (fsynced_file == num_files)
    {
      fsynced_file = 0;
      is_dirty = 0;
    }

    SB_THREAD_MUTEX_UNLOCK();
    return sb_req;
  }

  req_performed++;

  if (file_req->operation == FILE_OP_TYPE_WRITE)
    is_dirty = 1;

  /* Rewind to the first file if all files are processed */
  if (current_file == num_files)
  {
    position= 0;
    current_file= 0;
  }

  file_req->file_id = current_file;
  file_req->pos = position;
  file_req->size = SB_MIN(file_request_size, file_size - position);

  position += file_req->size;

  /* scroll to the next file if not already out of bound */
  if (position == file_size)
  {
    current_file++;
    position=0;
  }      
  
  if (sb_globals.validate)
  {
    check_seq_req(&prev_req, file_req);
    prev_req = *file_req;
  }
  
  SB_THREAD_MUTEX_UNLOCK(); 

  return sb_req;    
}


/* Request generatior for random tests */


sb_event_t file_get_rnd_request(int thread_id)
{
  sb_event_t           sb_req;
  sb_file_request_t    *file_req = &sb_req.u.file_request;
  unsigned long long   tmppos;
  int                  mode = test_mode;
  unsigned int         i;

  sb_req.type = SB_REQ_TYPE_FILE;
  SB_THREAD_MUTEX_LOCK(); 
  
  /*
    Convert mode for combined tests. Locking to get consistent values
    We have to use "real" values for mixed test  
  */
  if (test_mode==MODE_RND_RW)
  {
    if ((double)(read_ops + 1) / (write_ops + 1) < file_rw_ratio)
      mode=MODE_RND_READ;
    else
      mode=MODE_RND_WRITE;
  }

  /*
    is_dirty is only set if writes are done and cleared after all files
    are synced
  */
  if (file_fsync_freq != 0 && is_dirty)
  {
    if (req_performed % file_fsync_freq == 0)
    {
      file_req->operation = FILE_OP_TYPE_FSYNC;  
      file_req->file_id = fsynced_file;
      file_req->pos = 0;
      file_req->size = 0;
      fsynced_file++;
      if (fsynced_file == num_files)
      {
        fsynced_file = 0;
        is_dirty = 0;
      }

      SB_THREAD_MUTEX_UNLOCK();
      return sb_req;
    }
  }

  if (mode==MODE_RND_WRITE) /* mode shall be WRITE or RND_WRITE only */
    file_req->operation = FILE_OP_TYPE_WRITE;
  else
    file_req->operation = FILE_OP_TYPE_READ;

retry:
  tmppos = (long long) (sb_rand_uniform_double() * total_size);
  tmppos = tmppos - (tmppos % (long long) file_block_size);
  file_req->file_id = (int) (tmppos / (long long) file_size);
  file_req->pos = (long long) (tmppos % (long long) file_size);
  file_req->size = SB_MIN(file_block_size, file_size - file_req->pos);

  if (sb_globals.validate)
  {
    /*
       For the multi-threaded validation test we have to make sure the block is
       not being used by another thread
    */
    for (i = 0; i < sb_globals.threads; i++)
    {
      if (i != (unsigned) thread_id && per_thread[i].buffer_file_id == file_req->file_id &&
          per_thread[i].buffer_pos == file_req->pos)
        goto retry;
    }
  }

  per_thread[thread_id].buffer_file_id = file_req->file_id;
  per_thread[thread_id].buffer_pos = file_req->pos;

  req_performed++;
  if (file_req->operation == FILE_OP_TYPE_WRITE) 
    is_dirty = 1;

  SB_THREAD_MUTEX_UNLOCK();        
  return sb_req;
}


int file_execute_event(sb_event_t *sb_req, int thread_id)
{
  FILE_DESCRIPTOR    fd;
  sb_file_request_t *file_req = &sb_req->u.file_request;

  if (sb_globals.debug)
  {
    log_text(LOG_DEBUG,
             "Executing request, operation: %d, file_id: %d, pos: %d, "
             "size: %d",
             file_req->operation,
             file_req->file_id,
             (int)file_req->pos,
             (int)file_req->size);
  }
  
  /* Check request parameters */
  if (file_req->file_id > num_files)
  {
    log_text(LOG_FATAL, "Incorrect file id in request: %u", file_req->file_id);
    return 1;
  }
  if (file_req->pos + file_req->size > file_size)
  {
    log_text(LOG_FATAL, "I/O request exceeds file size. "
             "file id: %d file size: %lld req offset: %lld req size: %lld",
             file_req->file_id, (long long) file_size,
             (long long) file_req->pos, (long long) file_req->size);
    return 1;
  }
  fd = files[file_req->file_id];

  switch (file_req->operation) {
    case FILE_OP_TYPE_NULL:
      log_text(LOG_FATAL, "Execute of NULL request called !, aborting");
      return 1;
    case FILE_OP_TYPE_WRITE:

      /* Store checksum and offset in a buffer when in validation mode */
      if (sb_globals.validate)
        file_fill_buffer(per_thread[thread_id].buffer, file_req->size, file_req->pos);
                         
      if(file_pwrite(file_req->file_id, per_thread[thread_id].buffer,
                     file_req->size, file_req->pos, thread_id)
         != (ssize_t)file_req->size)
      {
        log_errno(LOG_FATAL, "Failed to write file! file: " FD_FMT " pos: %lld", 
                  fd, (long long)file_req->pos);
        return 1;
      }

      /* Check if we have to fsync each write operation */
      if (file_fsync_all && file_fsync(file_req->file_id, thread_id))
          return 1;

      /* In async mode stats will me updated on AIO requests completion */
      if (file_io_mode != FILE_IO_MODE_ASYNC)
      {
        SB_THREAD_MUTEX_LOCK();
        write_ops++;
        bytes_written += file_req->size;
        SB_THREAD_MUTEX_UNLOCK();
      }

      break;
    case FILE_OP_TYPE_READ:
      if(file_pread(file_req->file_id, per_thread[thread_id].buffer,
                    file_req->size, file_req->pos, thread_id)
         != (ssize_t)file_req->size)
      {
        log_errno(LOG_FATAL, "Failed to read file! file: " FD_FMT " pos: %lld",
                  fd, (long long)file_req->pos);
        return 1;
      }

      /* Validate block if run with validation enabled */
      if (sb_globals.validate &&
          file_validate_buffer(per_thread[thread_id].buffer, file_req->size, file_req->pos))
      {
        log_text(LOG_FATAL,
          "Validation failed on file " FD_FMT ", block offset %lld, exiting...",
                 file_req->file_id, (long long) file_req->pos);
        return 1;
      }

      /* In async mode stats will me updated on AIO requests completion */
      if(file_io_mode != FILE_IO_MODE_ASYNC)
      {
        SB_THREAD_MUTEX_LOCK();
        read_ops++;
        bytes_read += file_req->size;
        SB_THREAD_MUTEX_UNLOCK();
      }

      break;
    case FILE_OP_TYPE_FSYNC:
      /* Ignore fsync requests if we are already fsync'ing each operation */
      if (file_fsync_all)
        break;
      if(file_fsync(file_req->file_id, thread_id))
        return 1;

      break;
    default:
      log_text(LOG_FATAL, "Execute of UNKNOWN file request type called (%d)!, "
               "aborting", file_req->operation);
      return 1;
  }
  return 0;

}

static void print_file_extra_flags(void)
{
  log_text(LOG_NOTICE, "Extra file open flags: %s%s%s%s",
           file_extra_flags == 0 ? "(none)" : "",
           file_extra_flags & SB_FILE_FLAG_SYNC ? "sync " : "",
           file_extra_flags & SB_FILE_FLAG_DSYNC ? "dsync ": "",
           file_extra_flags & SB_FILE_FLAG_DIRECTIO ? "directio" : ""
           );
}

void file_print_mode(void)
{
  char sizestr[16];

  print_file_extra_flags();
  log_text(LOG_NOTICE, "%d files, %sB each", num_files,
           sb_print_value_size(sizestr, sizeof(sizestr), file_size));
  log_text(LOG_NOTICE, "%sB total file size",
           sb_print_value_size(sizestr, sizeof(sizestr),
                               file_size * num_files));
  log_text(LOG_NOTICE, "Block size %sB",
           sb_print_value_size(sizestr, sizeof(sizestr), file_block_size));
  if (file_merged_requests > 0)
    log_text(LOG_NOTICE, "Merging requests up to %sB for sequential IO.",
             sb_print_value_size(sizestr, sizeof(sizestr),
                                 file_request_size));

  switch (test_mode)
  {
    case MODE_RND_WRITE:
    case MODE_RND_READ:
    case MODE_RND_RW:
      log_text(LOG_NOTICE, "Number of IO requests: %" PRIu64,
               sb_globals.max_events);
      log_text(LOG_NOTICE,
               "Read/Write ratio for combined random IO test: %2.2f",
               file_rw_ratio);
      break;
    default:
      break;
  }

  if (file_fsync_freq > 0)
    log_text(LOG_NOTICE,
             "Periodic FSYNC enabled, calling fsync() each %d requests.",
             file_fsync_freq);

  if (file_fsync_end)
    log_text(LOG_NOTICE, "Calling fsync() at the end of test, Enabled.");

  if (file_fsync_all)
    log_text(LOG_NOTICE, "Calling fsync() after each write operation.");

  log_text(LOG_NOTICE, "Using %s I/O mode", get_io_mode_str(file_io_mode));

  if (sb_globals.validate)
    log_text(LOG_NOTICE, "Using checksums validation.");
  
  log_text(LOG_NOTICE, "Doing %s test", get_test_mode_str(test_mode));
}

/*
  Print intermediate test statistics.

  TODO: remove the mutex, use sb_stat_t and sb_counter_t.
*/

void file_report_intermediate(sb_stat_t *stat)
{
  unsigned long long diff_read;
  unsigned long long diff_written;
  unsigned long long diff_other_ops;

  SB_THREAD_MUTEX_LOCK();

  diff_read = bytes_read - last_bytes_read;
  diff_written = bytes_written - last_bytes_written;
  diff_other_ops = other_ops - last_other_ops;

  last_bytes_read = bytes_read;
  last_bytes_written = bytes_written;
  last_other_ops = other_ops;

  SB_THREAD_MUTEX_UNLOCK();

  log_timestamp(LOG_NOTICE, stat->time_total,
                "reads: %4.2f MiB/s writes: %4.2f MiB/s fsyncs: %4.2f/s "
                "latency (ms,%u%%): %4.3f",
                diff_read / megabyte / stat->time_interval,
                diff_written / megabyte / stat->time_interval,
                diff_other_ops / stat->time_interval,
                sb_globals.percentile,
                SEC2MS(stat->latency_pct));
}

/*
  Print cumulative test statistics.

  TODO: remove the mutex, use sb_stat_t and sb_counter_t.
*/

void file_report_cumulative(sb_stat_t *stat)
{
  const double seconds = stat->time_interval;

  SB_THREAD_MUTEX_LOCK();

  log_text(LOG_NOTICE, "\n"
           "File operations:\n"
           "    reads/s:                      %4.2f\n"
           "    writes/s:                     %4.2f\n"
           "    fsyncs/s:                     %4.2f\n"
           "\n"
           "Throughput:\n"
           "    read, MiB/s:                  %4.2f\n"
           "    written, MiB/s:               %4.2f",
           read_ops / seconds, write_ops / seconds, other_ops / seconds,
           bytes_read / megabyte / seconds,
           bytes_written / megabyte / seconds);

  clear_stats();

  SB_THREAD_MUTEX_UNLOCK();

  sb_report_cumulative(stat);
}

/* Return name for I/O mode */

const char *get_io_mode_str(file_io_mode_t mode)
{
  switch (mode) {
    case FILE_IO_MODE_SYNC:
      return "synchronous";
    case FILE_IO_MODE_ASYNC:
      return "asynchronous";
    case FILE_IO_MODE_MMAP:
#if SIZEOF_SIZE_T == 4
      return "slow mmaped";
#else
      return "fast mmaped";
#endif
    default:
      break;
  }

  return "(unknown)";
}


/* Return name for test mode */


const char *get_test_mode_str(file_test_mode_t mode)
{
  switch (mode) {
    case MODE_WRITE:
      return "sequential write (creation)";
    case MODE_REWRITE:
      return "sequential rewrite";
    case MODE_READ:
      return "sequential read";
    case MODE_RND_READ:
      return "random read"; 
    case MODE_RND_WRITE:
      return "random write";
    case MODE_RND_RW:
      return "random r/w";
    case MODE_MIXED:
      return "mixed";
    default:
      break;
  }
  
  return "(unknown)";
}


/*
  Converts the argument of --file-extra-flags to platform-specific open() flags.
  Returns 1 on error, 0 on success.
*/

static int convert_extra_flags(file_flags_t extra_flags, int *open_flags)
{
  if (extra_flags == 0)
  {
#ifdef _WIN32
    *open_flags = FILE_ATTRIBUTE_NORMAL;
#endif
  }
  else
  {
    *open_flags = 0;

    if (extra_flags & SB_FILE_FLAG_SYNC)
    {
#ifdef _WIN32
      *open_flags |= FILE_FLAG_WRITE_THROUGH;
#else
      *open_flags |= O_SYNC;
#endif
    }

    if (extra_flags & SB_FILE_FLAG_DSYNC)
    {
#ifdef O_DSYNC
      *open_flags |= O_DSYNC;
#else
      log_text(LOG_FATAL,
               "--file-extra-flags=dsync is not supported on this platform.");
      return 1;
#endif
    }

    if (extra_flags & SB_FILE_FLAG_DIRECTIO)
    {
#ifdef HAVE_DIRECTIO
      /* Will call directio(3) later */
#elif defined(O_DIRECT)
      *open_flags |= O_DIRECT;
#elif defined _WIN32
      *open_flags |= FILE_FLAG_NO_BUFFERING;
#else
      log_text(LOG_FATAL,
               "--file-extra-flags=direct is not supported on this platform.");
      return 1;
#endif
    }

    if (extra_flags > SB_FILE_FLAG_DIRECTIO)
    {
      log_text(LOG_FATAL, "Unknown extra flags value: %d", (int) extra_flags);
      return 1;
    }
  }

  return 0;
}

/* Create files of necessary size for test */

int create_files(void)
{
  unsigned int       i;
  int                fd;
  char               file_name[512];
  long long          offset;
  long long          written = 0;
  sb_timer_t         t;
  double             seconds;
  int                flags = 0;

  log_text(LOG_NOTICE, "%d files, %ldKb each, %ldMb total", num_files,
           (long)(file_size / 1024),
           (long)((file_size * num_files) / (1024 * 1024)));
  log_text(LOG_NOTICE, "Creating files for the test...");
  print_file_extra_flags();

  if (convert_extra_flags(file_extra_flags, &flags))
    return 1;

  sb_timer_init(&t);
  sb_timer_start(&t);

  for (i=0; i < num_files; i++)
  {
    snprintf(file_name, sizeof(file_name), "test_file.%d",i);

    fd = open(file_name, O_CREAT | O_WRONLY | flags, S_IRUSR | S_IWUSR);
    if (fd < 0)
    {
      log_errno(LOG_FATAL, "Can't open file");
      return 1; 
    }

#ifndef _WIN32
    offset = (long long) lseek(fd, 0, SEEK_END);
#else
    offset = (long long) _lseeki64(fd, 0, SEEK_END);
#endif

    if (offset >= file_size)
      log_text(LOG_NOTICE, "Reusing existing file %s", file_name);
    else if (offset > 0)
      log_text(LOG_NOTICE, "Extending existing file %s", file_name);
    else
      log_text(LOG_NOTICE, "Creating file %s", file_name);

    for (; offset < file_size;
         written += file_block_size, offset += file_block_size)
    {
      /*
        If in validation mode, fill buffer with random values
        and write checksum. Not called in parallel, so use per_thread[0].
      */
      if (sb_globals.validate)
        file_fill_buffer(per_thread[0].buffer, file_block_size, offset);
                         
      if (write(fd, per_thread[0].buffer, file_block_size) < 0)
        goto error;
    }
    
    /* fsync files to prevent cache flush from affecting test results */
#ifndef _WIN32
    fsync(fd);
#else
    _commit(fd);
#endif
    close(fd);
  }

  seconds = NS2SEC(sb_timer_stop(&t));

  if (written > 0)
    log_text(LOG_NOTICE, "%llu bytes written in %.2f seconds (%.2f MiB/sec).",
             written, seconds,
             (double) (written / megabyte) / seconds);
  else
    log_text(LOG_NOTICE, "No bytes written.");

  return 0;

 error:
  log_errno(LOG_FATAL, "Failed to write file!");
  close(fd);
  return 1;
}


/* Remove test files */


int remove_files(void)
{
  unsigned int i;
  char         file_name[512];
  
  log_text(LOG_NOTICE, "Removing test files...");
  
  for (i = 0; i < num_files; i++)
  {
    snprintf(file_name, sizeof(file_name), "test_file.%d",i);
    unlink(file_name);
  }

  return 0;
}


/* 'prepare' command for fileio test */


int file_cmd_prepare(void)
{
  if (parse_arguments())
    return 1;

  /*
    Make sure that files do not exist for 'sequential write' test mode,
    create test files for other test modes
  */
  if (test_mode == MODE_WRITE)
    return remove_files();

  return create_files();
}


/* 'cleanup' command for fileio test */


int file_cmd_cleanup(void)
{
  if (parse_arguments())
    return 1;

  return remove_files();
}

void init_vars(void)
{
  position = 0; /* position in file */
  current_file = 0;
  fsynced_file = 0; /* for counting file to be fsynced */
  req_performed = 0;
  is_dirty = 0;
  if (sb_globals.validate)
  {
    prev_req.size = 0;
    prev_req.operation = FILE_OP_TYPE_NULL;
    prev_req.file_id = 0;
    prev_req.pos = 0;
  }
}

void clear_stats(void)
{
  read_ops = 0;
  write_ops = 0;
  other_ops = 0;
  last_other_ops = 0;
  bytes_read = 0;
  last_bytes_read = 0;
  bytes_written = 0;
  last_bytes_written = 0;
}

/*
  Before the benchmark is stopped, issue fsync() if --file-fsync-end is used,
  and wait for all async operations to complete.
*/

int file_thread_done(int thread_id)
{
  if (file_fsync_end && test_mode != MODE_READ && test_mode != MODE_RND_READ)
  {
    for (unsigned i = 0; i < num_files; i++)
    {
      if(file_fsync(i, thread_id))
        return 1;
    }
  }

#ifdef HAVE_LIBAIO
  if (file_io_mode == FILE_IO_MODE_ASYNC && aio_ctxts[thread_id].nrequests > 0)
    return file_wait(thread_id, aio_ctxts[thread_id].nrequests);
#endif

  return 0;
}

#ifdef HAVE_LIBAIO
/* Allocate async contexts pool */


int file_async_init(void)
{
  unsigned int i;

  if (file_io_mode != FILE_IO_MODE_ASYNC)
    return 0;
  
  file_async_backlog = sb_get_value_int("file-async-backlog");
  if (file_async_backlog <= 0) {
    log_text(LOG_FATAL, "Invalid value of file-async-backlog: %d",
             file_async_backlog);
    return 1;
  }

  aio_ctxts = (sb_aio_context_t *)calloc(sb_globals.threads,
                                         sizeof(sb_aio_context_t));
  for (i = 0; i < sb_globals.threads; i++)
  {
    if (io_queue_init(file_async_backlog, &aio_ctxts[i].io_ctxt))
    {
      log_errno(LOG_FATAL, "io_queue_init() failed!");
      return 1;
    }
      
    aio_ctxts[i].events = (struct io_event *)malloc(file_async_backlog *
                                                    sizeof(struct io_event));
    if (aio_ctxts[i].events == NULL)
    {
      log_errno(LOG_FATAL, "Failed to allocate async I/O context!");
      return 1;
    }
  }

  return 0;
}


/* Destroy async contexts pool */


int file_async_done(void)
{
  unsigned int i;
  
  if (file_io_mode != FILE_IO_MODE_ASYNC)
    return 0;

  for (i = 0; i < sb_globals.threads; i++)
  {
    io_queue_release(aio_ctxts[i].io_ctxt);
    free(aio_ctxts[i].events);
  }
  
  free(aio_ctxts);

  return 0;
}  

/*
  Submit async I/O requests until the length of request queue exceeds
  the limit. Then wait for at least one request to complete and proceed.
*/


int file_submit_or_wait(struct iocb *iocb, sb_file_op_t type, ssize_t len,
                        int thread_id)
{
  sb_aio_oper_t   *oper;
  struct iocb     *iocbp;

  oper = (sb_aio_oper_t *)malloc(sizeof(sb_aio_oper_t));
  if (oper == NULL)
  {
    log_text(LOG_FATAL, "Failed to allocate AIO operation!");
    return 1;
  }

  memcpy(&oper->iocb, iocb, sizeof(*iocb));
  oper->type = type;
  oper->len = len;
  iocbp = &oper->iocb;

  if (io_submit(aio_ctxts[thread_id].io_ctxt, 1, &iocbp) < 1)
  {
    log_errno(LOG_FATAL, "io_submit() failed!");
    return 1;
  }
  
  aio_ctxts[thread_id].nrequests++;
  if (aio_ctxts[thread_id].nrequests < file_async_backlog)
    return 0;
  
  return file_wait(thread_id, 1);
}


/*
  Wait for at least nreq I/O requests to complete
*/


int file_wait(int thread_id, long nreq)
{ 
  long            i;
  long            nr;
  struct io_event *event;
  sb_aio_oper_t   *oper;
  struct iocb     *iocbp;

  /* Try to read some events */
#ifdef HAVE_OLD_GETEVENTS
  (void)nreq; /* unused */
  nr = io_getevents(aio_ctxts[thread_id].io_ctxt, file_async_backlog,
                    aio_ctxts[thread_id].events, NULL);
#else
  nr = io_getevents(aio_ctxts[thread_id].io_ctxt, nreq, file_async_backlog,
                    aio_ctxts[thread_id].events, NULL);
#endif
  if (nr < 1)
  {
    log_errno(LOG_FATAL, "io_getevents() failed!");
    return 1;
  }

  /* Verify results */
  for (i = 0; i < nr; i++)
  {
    event = (struct io_event *)aio_ctxts[thread_id].events + i;
    iocbp = (struct iocb *)(unsigned long)event->obj;
    oper = (sb_aio_oper_t *)iocbp;
    switch (oper->type) {
    case FILE_OP_TYPE_FSYNC:
        if (event->res != 0)
        {
          log_text(LOG_FATAL, "Asynchronous fsync failed!\n");
          return 1;
        }

        SB_THREAD_MUTEX_LOCK();
        other_ops++;
        SB_THREAD_MUTEX_UNLOCK();

        break;

    case FILE_OP_TYPE_READ:
        if ((ssize_t)event->res != oper->len)
        {
          log_text(LOG_FATAL, "Asynchronous read failed!\n");
          return 1;
        }

        SB_THREAD_MUTEX_LOCK();
        read_ops++;
        bytes_read += oper->len;
        SB_THREAD_MUTEX_UNLOCK();

        break;

    case FILE_OP_TYPE_WRITE:
        if ((ssize_t)event->res != oper->len)
        {
          log_text(LOG_FATAL, "Asynchronous write failed!\n");
          return 1;
        }

        SB_THREAD_MUTEX_LOCK();
        write_ops++;
        bytes_written += oper->len;
        SB_THREAD_MUTEX_UNLOCK();

        break;

    default:
        break;
    }
    free(oper);
    aio_ctxts[thread_id].nrequests--;
  }
  
  return 0;
}
#endif /* HAVE_LIBAIO */

                        
#ifdef HAVE_MMAP
/* Initialize data structures required for mmap'ed I/O operations */


int file_mmap_prepare(void)
{
  unsigned int i;
  
  if (file_io_mode != FILE_IO_MODE_MMAP)
    return 0;

  file_page_mask = ~(sb_get_allocation_granularity() - 1);
  
  /* Extend file sizes for sequential write test */
  if (test_mode == MODE_WRITE)
    for (i = 0; i < num_files; i++)
    {
#ifdef _WIN32
      HANDLE hFile = files[i];
      LARGE_INTEGER offset;
      offset.QuadPart = file_size;
      if (!SetFilePointerEx(hFile ,offset ,NULL, FILE_BEGIN))
      {
        log_errno(LOG_FATAL, "SetFilePointerEx() failed on file %d", i);
        return 1;
      }
      if (!SetEndOfFile(hFile))
      {
        log_errno(LOG_FATAL, "SetEndOfFile() failed on file %d", i);
        return 1;
      }
      offset.QuadPart = 0;
      SetFilePointerEx(hFile ,offset ,NULL, FILE_BEGIN);
#else
      if (ftruncate(files[i], file_size))
      {
        log_errno(LOG_FATAL, "ftruncate() failed on file %d", i);
        return 1;
      }
#endif 
    }

#if SIZEOF_SIZE_T > 4
  mmaps = (void **)malloc(num_files * sizeof(void *));
  for (i = 0; i < num_files; i++)
  {
    mmaps[i] = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                    files[i], 0);
    if (mmaps[i] == MAP_FAILED)
    {
      log_errno(LOG_FATAL, "mmap() failed on file %d", i);
      return 1;
    }
  }
#else
  (void)i; /* unused */
#endif
  
  return 0;
}


/* Destroy data structure used by mmap'ed I/O operations */


int file_mmap_done(void)
{
  unsigned int i;

  if (file_io_mode != FILE_IO_MODE_MMAP)
    return 0;

#if SIZEOF_SIZE_T > 4
  for (i = 0; i < num_files; i++)
    munmap(mmaps[i], file_size);

  free(mmaps);
#else
   (void)i; /* unused */
#endif
  
  return 0;
}
#endif /* HAVE_MMAP */

int file_do_fsync(unsigned int id, int thread_id)
{
  FILE_DESCRIPTOR fd = files[id];
#ifdef HAVE_LIBAIO
  struct iocb iocb;
#else
  (void)thread_id; /* unused */
#endif

  /*
    FIXME: asynchronous fsync support is missing
    in Linux kernel at the moment
  */
  if (file_io_mode == FILE_IO_MODE_SYNC
      || file_io_mode == FILE_IO_MODE_ASYNC
#if defined(HAVE_MMAP) && SIZEOF_SIZE_T == 4
      /* Use fsync in mmaped mode on 32-bit architectures */
      || file_io_mode == FILE_IO_MODE_MMAP
#endif
      )
  {
    if (file_fsync_mode == FSYNC_ALL)
#ifndef _WIN32
      return fsync(fd);
#else
      return !FlushFileBuffers(fd);
#endif

#ifdef F_FULLFSYNC
      return fcntl(fd, F_FULLFSYNC) != -1;
#elif defined(HAVE_FDATASYNC)
    return fdatasync(fd);
#else
    log_text(LOG_ALERT, "Unknown fsync mode: %d", file_fsync_mode);
    return -1;
#endif

  }
#ifdef HAVE_LIBAIO
  else if (file_io_mode == FILE_IO_MODE_ASYNC)
  {
    /* Use asynchronous fsync */
    if (file_fsync_mode == FSYNC_ALL)
      io_prep_fsync(&iocb, fd);
    else
      io_prep_fdsync(&iocb, fd);

    return file_submit_or_wait(&iocb, FILE_OP_TYPE_FSYNC, 0, thread_id);
  }
#endif
#ifdef HAVE_MMAP
  /* Use msync on file on 64-bit architectures */
  else if (file_io_mode == FILE_IO_MODE_MMAP)
  {
#ifndef _WIN32
    return msync(mmaps[id], file_size, MS_SYNC | MS_INVALIDATE);
#else
    return !FlushViewOfFile(mmaps[id], (size_t) file_size);
#endif
  }
#endif

  return 1; /* Unknown I/O mode */
}


int file_fsync(unsigned int id, int thread_id)
{
  if (file_do_fsync(id, thread_id))
  {
    log_errno(LOG_FATAL, "Failed to fsync file! file: " FD_FMT, files[id]);
    return 1;
  }

  SB_THREAD_MUTEX_LOCK();
  other_ops++;
  SB_THREAD_MUTEX_UNLOCK();

  return 0;
}


#ifdef _WIN32
ssize_t pread(HANDLE hFile, void *buf, ssize_t count, long long offset)
{
  DWORD         nBytesRead;
  OVERLAPPED    ov = {0};
  LARGE_INTEGER li;

  if(!count)
	  return 0;
#ifdef _WIN64
  if(count > UINT_MAX)
    count= UINT_MAX;
#endif

  li.QuadPart   = offset;
  ov.Offset     = li.LowPart;
  ov.OffsetHigh = li.HighPart;

  if(!ReadFile(hFile, buf, (DWORD)count, &nBytesRead, &ov))
  {
    DWORD lastError = GetLastError();
    if(lastError == ERROR_HANDLE_EOF)
     return 0;
    return -1;
  }
  return nBytesRead;
}
ssize_t pwrite(HANDLE hFile, const void *buf, size_t count, 
                     long long  offset)
{
  DWORD         nBytesWritten;
  OVERLAPPED    ov = {0};
  LARGE_INTEGER li;

  if(!count)
    return 0;

#ifdef _WIN64
  if(count > UINT_MAX)
    count= UINT_MAX;
#endif

  li.QuadPart  = offset;
  ov.Offset    = li.LowPart;
  ov.OffsetHigh= li.HighPart;

  if(!WriteFile(hFile, buf, (DWORD)count, &nBytesWritten, &ov))
  {
    return -1;
  }
  else
    return nBytesWritten;
}

#define MAP_SHARED 0
#define PROT_READ  1
#define PROT_WRITE 2
#define MAP_FAILED NULL

void *mmap(void *addr, size_t len, int prot, int flags,
            FILE_DESCRIPTOR fd, long long off)
{
  DWORD flProtect;
  DWORD flMap;
  void *retval;
  LARGE_INTEGER li;
  HANDLE hMap;

  switch(prot)
  {
  case PROT_READ:
    flProtect = PAGE_READONLY;
    flMap     = FILE_MAP_READ;
    break;
  case PROT_READ|PROT_WRITE:
    flProtect = PAGE_READWRITE;
    flMap     = FILE_MAP_ALL_ACCESS;
    break;
  default:
    return MAP_FAILED;
  }
  hMap = CreateFileMapping(fd, NULL, flProtect, 0 , 0, NULL);

  if(hMap == INVALID_HANDLE_VALUE)
    return MAP_FAILED;

  li.QuadPart = off;
  retval = MapViewOfFileEx(hMap, flMap, li.HighPart, li.LowPart, len, NULL);

  CloseHandle(hMap);
  return retval;
}

int munmap(void *start, size_t len)
{
  (void) len; /* unused */
  if(UnmapViewOfFile(start))
    return 0;
  return -1;
}
#endif


ssize_t file_pread(unsigned int file_id, void *buf, ssize_t count,
                   long long offset, int thread_id)
{
  FILE_DESCRIPTOR fd = files[file_id];
#ifdef HAVE_MMAP
  void        *start;
  long long    page_addr;
  long long    page_offset;
#endif
#ifdef HAVE_LIBAIO
  struct iocb iocb;
#else
  (void)thread_id; /* unused */
#endif
    
  if (file_io_mode == FILE_IO_MODE_SYNC)
    return pread(fd, buf, count, offset);
#ifdef HAVE_LIBAIO
  else if (file_io_mode == FILE_IO_MODE_ASYNC)
  {
    /* Use asynchronous read */
    io_prep_pread(&iocb, fd, buf, count, offset);

    if (file_submit_or_wait(&iocb, FILE_OP_TYPE_READ, count, thread_id))
      return 0;

    return count;
  }
#endif
#ifdef HAVE_MMAP
  else if (file_io_mode == FILE_IO_MODE_MMAP)
  {
# if SIZEOF_SIZE_T == 4
    /* Create file mapping for each I/O operation on 32-bit platforms */
    page_addr = offset & file_page_mask;
    page_offset = offset - page_addr;
    start = mmap(NULL, count + page_offset, PROT_READ, MAP_SHARED,
                 fd, page_addr);
    if (start == MAP_FAILED)
      return 0;
    memcpy(buf, (char *)start + page_offset, count);
    munmap(start, count + page_offset);
    return count;
# else
    (void)start; /* unused */
    (void)page_addr; /* unused */
    (void)page_offset; /* unused */
    
    /* We already have all files mapped on 64-bit platforms */
    memcpy(buf, (char *)mmaps[file_id] + offset, count);

    return count;
# endif
  }
#endif /* HAVE_MMAP */
  
  return 1; /* Unknown I/O mode */
}


ssize_t file_pwrite(unsigned int file_id, void *buf, ssize_t count,
                    long long offset, int thread_id)
{
  FILE_DESCRIPTOR fd = files[file_id];
#ifdef HAVE_MMAP
  void        *start;
  size_t       page_addr;
  size_t       page_offset;
#endif  
#ifdef HAVE_LIBAIO
  struct iocb iocb;
#else  
  (void)thread_id; /* unused */
#endif
  
  if (file_io_mode == FILE_IO_MODE_SYNC)
    return pwrite(fd, buf, count, offset);
#ifdef HAVE_LIBAIO
  else if (file_io_mode == FILE_IO_MODE_ASYNC)
  {
    /* Use asynchronous write */
    io_prep_pwrite(&iocb, fd, buf, count, offset);

    if (file_submit_or_wait(&iocb, FILE_OP_TYPE_WRITE, count, thread_id))
      return 0;

    return count;
  }
#endif
#ifdef HAVE_MMAP
  else if (file_io_mode == FILE_IO_MODE_MMAP)
  {
# if SIZEOF_SIZE_T == 4
    /* Create file mapping for each I/O operation on 32-bit platforms */
    page_addr = offset & file_page_mask;
    page_offset = offset - page_addr;
    start = mmap(NULL, count + page_offset, PROT_READ | PROT_WRITE, MAP_SHARED,
                 fd, page_addr);

    if (start == MAP_FAILED)
      return 0;
    memcpy((char *)start + page_offset, buf, count);
    munmap(start, count + page_offset);

    return count;
# else
    (void)start; /* unused */
    (void)page_addr; /* unused */
    (void)page_offset; /* unused */

    /* We already have all files mapped on 64-bit platforms */
    memcpy((char *)mmaps[file_id] + offset, buf, count);

    return count;
# endif    
  }
#endif /* HAVE_MMAP */

  return 0; /* Unknown I/O mode */
}


/* Parse the command line arguments */


int parse_arguments(void)
{
  char         *mode;
  unsigned int  i;
  
  num_files = sb_get_value_int("file-num");

  if (num_files <= 0)
  {
    log_text(LOG_FATAL, "Invalid value for file-num: %d", num_files);
    return 1;
  }
  total_size = sb_get_value_size("file-total-size");
  if (total_size <= 0)
  {
    log_text(LOG_FATAL, "Invalid value for file-total-size: %lld",
             (long long)total_size);
    return 1;
  }
  file_size = total_size / num_files;
  
  mode = sb_get_value_string("file-test-mode");

  /* File test mode is necessary only for 'run' command */
  if (!strcmp(sb_globals.cmdname, "run"))
  {
    if (mode == NULL)
    {
      log_text(LOG_FATAL, "Missing required argument: --file-test-mode\n");

      log_text(LOG_NOTICE, "fileio options:");
      sb_print_options(fileio_args);

      return 1;
    }
    if (!strcmp(mode, "seqwr"))
      test_mode = MODE_WRITE;
    else if (!strcmp(mode, "seqrewr"))
      test_mode = MODE_REWRITE;
    else if (!strcmp(mode, "seqrd"))
      test_mode = MODE_READ;
    else if (!strcmp(mode, "rndrd"))
      test_mode = MODE_RND_READ;
    else if (!strcmp(mode, "rndwr"))
      test_mode = MODE_RND_WRITE;
    else if (!strcmp(mode, "rndrw"))
      test_mode = MODE_RND_RW;
    else
    {
      log_text(LOG_FATAL, "Invalid IO operations mode: %s.", mode);
      return 1;
    }
  }
  
  mode  = sb_get_value_string("file-io-mode");
  if (mode == NULL)
    mode = "sync";
  if (!strcmp(mode, "sync"))
    file_io_mode = FILE_IO_MODE_SYNC;
  else if (!strcmp(mode, "async"))
  {
#ifdef HAVE_LIBAIO
    file_io_mode = FILE_IO_MODE_ASYNC;
#else
    log_text(LOG_FATAL,
             "asynchronous I/O mode is unsupported on this platform.");
    return 1;
#endif
  }
  else if (!strcmp(mode, "mmap"))
  {
#ifdef HAVE_MMAP
    file_io_mode = FILE_IO_MODE_MMAP;
#else
    log_text(LOG_FATAL,
             "mmap'ed I/O mode is unsupported on this platform.");
    return 1;
#endif
  }
  else
  {
    log_text(LOG_FATAL, "unknown I/O mode: %s", mode);
    return 1;
  }
  
  file_merged_requests = sb_get_value_int("file-merged-requests");
  if (file_merged_requests < 0)
  {
    log_text(LOG_FATAL, "Invalid value for file-merged-requests: %d.",
             file_merged_requests);
    return 1;
  }
  
  file_block_size = sb_get_value_size("file-block-size");
  if (file_block_size <= 0)
  {
    log_text(LOG_FATAL, "Invalid value for file-block-size: %d.",
             file_block_size);
    return 1;
  }

  if (file_merged_requests > 0)
    file_request_size = file_block_size * file_merged_requests;
  else
    file_request_size = file_block_size;

  mode = sb_get_value_string("file-extra-flags");

  sb_list_item_t *pos;
  SB_LIST_FOR_EACH(pos, sb_get_value_list("file-extra-flags"))
  {
    const char *val = SB_LIST_ENTRY(pos, value_t, listitem)->data;

    if (!strcmp(val, "sync"))
      file_extra_flags |= SB_FILE_FLAG_SYNC;
    else if (!strcmp(val, "dsync"))
      file_extra_flags |= SB_FILE_FLAG_DSYNC;
    else if (!strcmp(val, "direct"))
      file_extra_flags |= SB_FILE_FLAG_DIRECTIO;
    else
    {
      log_text(LOG_FATAL, "Invalid value for file-extra-flags: %s", mode);
      return 1;
    }
  }

  file_fsync_freq = sb_get_value_int("file-fsync-freq");
  if (file_fsync_freq < 0)
  {
    log_text(LOG_FATAL, "Invalid value for file-fsync-freq: %d.",
             file_fsync_freq);
    return 1;
  }

  file_fsync_end = sb_get_value_flag("file-fsync-end");
  file_fsync_all = sb_get_value_flag("file-fsync-all");
  /* file-fsync-all overrides file-fsync-end and file-fsync-freq */
  if (file_fsync_all) {
    file_fsync_end = 0;
    file_fsync_freq = 0;
  }
  
  mode = sb_get_value_string("file-fsync-mode");
  if (!strcmp(mode, "fsync"))
    file_fsync_mode = FSYNC_ALL;
  else if (!strcmp(mode, "fdatasync"))
  {
#ifdef HAVE_FDATASYNC
    file_fsync_mode = FSYNC_DATA;
#else
    log_text(LOG_FATAL, "fdatasync() is unavailable on this platform");
    return 1;
#endif
  }
  else
  {
    log_text(LOG_FATAL, "Invalid fsync mode: %s.", mode);
    return 1;
  }

  file_rw_ratio = sb_get_value_double("file-rw-ratio");
  if (file_rw_ratio < 0)
  {
    log_text(LOG_FATAL, "Invalid value for --file-rw-ratio: %f.", file_rw_ratio);
    return 1;
  }

  per_thread = malloc(sizeof(*per_thread) * sb_globals.threads);
  for (i = 0; i < sb_globals.threads; i++)
  {
    per_thread[i].buffer = sb_memalign(file_request_size, sb_getpagesize());
    if (per_thread[i].buffer == NULL)
    {
      log_text(LOG_FATAL, "Failed to allocate a memory buffer");
      return 1;
    }
    memset(per_thread[i].buffer, 0, file_request_size);
  }

  return 0;
}


/* check if two requests are sequential */


void check_seq_req(sb_file_request_t *prev_req, sb_file_request_t *r)
{
  /* Do not check fsync operation at the moment */
  if (r->operation == FILE_OP_TYPE_FSYNC || r->operation == FILE_OP_TYPE_NULL)
    return; 
  /* if old request is NULL do not check against it */
  if (prev_req->operation == FILE_OP_TYPE_NULL)
    return;
  /* check files */
  if (r->file_id - prev_req->file_id>1 &&
      !(r->file_id == 0 && prev_req->file_id == num_files-1))
  {
    log_text(LOG_WARNING,
             "Discovered too large file difference in seq requests!");
    return;
  }
  if (r->file_id == prev_req->file_id)
  {
    if(r->pos - prev_req->pos != prev_req->size)
    {
      log_text(LOG_WARNING,
               "Discovered too large position difference in seq request!");
      return;
    }
  }    
  else /* if file changed last request has to complete file and new start */
  {
    if ((prev_req->pos + prev_req->size != file_size) || (r->pos != 0))
    {
      log_text(LOG_WARNING, "Invalid file switch found!");  
      log_text(LOG_WARNING, "Old: file_id: %d, pos: %d  size: %d",
               prev_req->file_id, (int)prev_req->pos, (int)prev_req->size);
      log_text(LOG_WARNING, "New: file_id: %d, pos: %d  size: %d",
               r->file_id, (int)r->pos, (int)r->size);
      
      return;
    }  
  }    
} 


/*
  Alignment requirement for mmap(). The same as page size, except on Windows
  (on Windows it has to be 64KB, even if pagesize is only 4 or 8KB)
*/
unsigned long sb_get_allocation_granularity(void)
{
#ifdef _WIN32
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return info.dwAllocationGranularity;
#else
  return sb_getpagesize();
#endif
}

static void sb_free_memaligned(void *buf)
{
#ifdef _WIN32
  VirtualFree(buf,0,MEM_FREE);
#else
  free(buf);
#endif
}

static FILE_DESCRIPTOR sb_open(const char *name)
{
  FILE_DESCRIPTOR file;
  int flags = 0;

  if (convert_extra_flags(file_extra_flags, &flags))
    return SB_INVALID_FILE;

#ifndef _WIN32
  file = open(name, O_RDWR | flags, S_IRUSR | S_IWUSR);
#else
  file = CreateFile(name, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                    flags, NULL);
#endif

#ifdef HAVE_DIRECTIO
  if (VALID_FILE(file) && file_extra_flags & SB_FILE_FLAG_DIRECTIO &&
      directio(file, DIRECTIO_ON))
  {
    log_errno(LOG_FATAL, "directio() failed");
    return SB_INVALID_FILE;
  }
#endif

  return file;
}

/*
  Create a file with a given path. Signal an error if the file already
  exists. Return a non-zero value on error.
*/

static int sb_create(const char *path)
{
  FILE_DESCRIPTOR file;
  int res;

#ifndef _WIN32
  file = open(path, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  res = !VALID_FILE(file);
  close(file);
#else
  file = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_NEW,
                    0, NULL);
  res = !VALID_FILE(file);
  CloseHandle(file);
#endif

  return res;
}


/* Fill buffer with random values and write checksum */


void file_fill_buffer(unsigned char *buf, unsigned int len,
                      size_t offset)
{
  unsigned int i;

  for (i = 0; i < len - (FILE_CHECKSUM_LENGTH + FILE_OFFSET_LENGTH); i++)
    buf[i] = sb_rand_uniform_uint64() & 0xFF;

  /* Store the checksum */
  *(int *)(void *)(buf + i) = (int)crc32(0, (unsigned char *)buf, len -
                                 (FILE_CHECKSUM_LENGTH + FILE_OFFSET_LENGTH));
  /* Store the offset */
  *(long *)(void *)(buf + i + FILE_CHECKSUM_LENGTH) = offset;
}


/* Validate checksum and offset of block read from disk */


int file_validate_buffer(unsigned char  *buf, unsigned int len, size_t offset)
{
  unsigned int checksum;
  unsigned int cs_offset;

  cs_offset = len - (FILE_CHECKSUM_LENGTH + FILE_OFFSET_LENGTH);
  
  checksum = (unsigned int)crc32(0, (unsigned char *)buf, cs_offset);

  if (checksum != *(unsigned int *)(void *)(buf + cs_offset))
  {
    log_text(LOG_FATAL, "Checksum mismatch in block with offset: %lld",
             (long long) offset);
    log_text(LOG_FATAL, "    Calculated value: 0x%x    Stored value: 0x%x",
             checksum, *(unsigned int *)(void *)(buf + cs_offset));
    return 1;
  }

  if (offset != *(size_t *)(void *)(buf + cs_offset + FILE_CHECKSUM_LENGTH))
  {
    log_text(LOG_FATAL, "Offset mismatch in block:");
    log_text(LOG_FATAL, "   Actual offset: %zu    Stored offset: %zu",
             offset, *(size_t *)(void *)(buf + cs_offset + FILE_CHECKSUM_LENGTH));
    return 1;
  }

  return 0;
}
