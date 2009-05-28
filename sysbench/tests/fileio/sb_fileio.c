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

#ifdef STDC_HEADERS
# include <stdio.h>
/* Required for memalign to be declared on Solaris */
#define __EXTENSIONS__
# include <stdlib.h>
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

/* Lengths of the checksum and the offset fields in a block */
#define FILE_CHECKSUM_LENGTH sizeof(int)
#define FILE_OFFSET_LENGTH sizeof(long)

#ifdef _WIN32
typedef HANDLE FILE_DESCRIPTOR;
#define VALID_FILE(fd) (fd != INVALID_HANDLE_VALUE)
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

/* Test options */
static unsigned int      num_files;
static long long         total_size;
static long long         file_size;
static int               file_block_size;
static int               file_extra_flags;
static int               file_fsync_freq;
static int               file_fsync_all;
static int               file_fsync_end;
static file_fsync_mode_t file_fsync_mode;
static float             file_rw_ratio;
static int               file_merged_requests;
static long long         file_max_request_size;
static file_io_mode_t    file_io_mode;
#ifdef HAVE_LIBAIO
static unsigned int      file_async_backlog;
#endif

/* statistical and other "local" variables */
static long long       position;      /* current position in file */
static unsigned int    current_file;  /* current file */
static unsigned int    fsynced_file;  /* file number to be fsynced (periodic) */
static unsigned int    fsynced_file2; /* fsyncing in the end */
static pthread_mutex_t fsync_mutex;   /* used to sync access to counters */

static int is_dirty;               /* any writes after last fsync series ? */
static int read_ops;
static int real_read_ops;
static int write_ops;
static int real_write_ops;
static int other_ops;
static unsigned int req_performed; /* number of requests done */
static unsigned long long bytes_read;
static unsigned long long  bytes_written;

#ifdef HAVE_MMAP
/* Array of file mappings */
static void          **mmaps; 
static unsigned long file_page_mask;
#endif

/* Array of file descriptors */
static FILE_DESCRIPTOR *files;

/* Buffer for all I/O operations */
static void *buffer;

/* test mode type */
static file_test_mode_t test_mode;

/* Previous request needed for validation */
static sb_file_request_t prev_req;

static sb_arg_t fileio_args[] = {
  {"file-num", "number of files to create", SB_ARG_TYPE_INT, "128"},
  {"file-block-size", "block size to use in all IO operations", SB_ARG_TYPE_INT, "16384"},
  {"file-total-size", "total size of files to create", SB_ARG_TYPE_SIZE, "2G"},
  {"file-test-mode", "test mode {seqwr, seqrewr, seqrd, rndrd, rndwr, rndrw}",
   SB_ARG_TYPE_STRING, NULL},
  {"file-io-mode", "file operations mode {sync,async,fastmmap,slowmmap}", SB_ARG_TYPE_STRING, "sync"},
#ifdef HAVE_LIBAIO
  {"file-async-backlog", "number of asynchronous operatons to queue per thread", SB_ARG_TYPE_INT, "128"},
#endif
  {"file-extra-flags", "additional flags to use on opening files {sync,dsync,direct}",
   SB_ARG_TYPE_STRING, ""},
  {"file-fsync-freq", "do fsync() after this number of requests (0 - don't use fsync())",
   SB_ARG_TYPE_INT, "100"},
  {"file-fsync-all", "do fsync() after each write operation", SB_ARG_TYPE_FLAG, "off"},
  {"file-fsync-end", "do fsync() at the end of test", SB_ARG_TYPE_FLAG, "on"},
  {"file-fsync-mode", "which method to use for synchronization {fsync, fdatasync}",
   SB_ARG_TYPE_STRING, "fsync"},
  {"file-merged-requests", "merge at most this number of IO requests if possible (0 - don't merge)",
   SB_ARG_TYPE_INT, "0"},
  {"file-rw-ratio", "reads/writes ratio for combined test", SB_ARG_TYPE_FLOAT, "1.5"},

  {NULL, NULL, SB_ARG_TYPE_NULL, NULL}
};

/* fileio test commands */
static int file_cmd_prepare(void);
static int file_cmd_cleanup(void);

/* fileio test operations */
static int file_init(void);
static void file_print_mode(void);
static int file_prepare(void);
static sb_request_t file_get_request(int);
static int file_execute_request(sb_request_t *, int);
#ifdef HAVE_LIBAIO
static int file_thread_done(int);
#endif
static int file_done(void);
static void file_print_stats(void);

static sb_test_t fileio_test =
{
  "fileio",
  "File I/O test",
  {
    file_init,
    file_prepare,
    NULL,
    file_print_mode,
    file_get_request,
    file_execute_request,
    file_print_stats,
#ifdef HAVE_LIBAIO
    file_thread_done,
#else
    NULL,
#endif
    NULL,
    file_done
  },
  {
    NULL,
    file_cmd_prepare,
    NULL,
    file_cmd_cleanup
  },
  fileio_args,
  {NULL, NULL}
};


static int create_files(void);
static int remove_files(void);
static int parse_arguments(void);
static void clear_stats(void);
static sb_request_t file_get_seq_request(void);
static sb_request_t file_get_rnd_request(void);
static void check_seq_req(sb_file_request_t *, sb_file_request_t *);
static const char *get_io_mode_str(file_io_mode_t mode);
static const char *get_test_mode_str(file_test_mode_t mode);
static void file_fill_buffer(char *, unsigned int, size_t);
static int file_validate_buffer(char  *, unsigned int, size_t);

/* File operation wrappers */
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
static unsigned long sb_getpagesize(void);
static unsigned long sb_get_allocation_granularity(void);
static void *sb_memalign(size_t size);
static void sb_free_memaligned(void *buf);

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
      unlink(file_name);

    log_text(LOG_DEBUG, "Opening file: %s",file_name);
#ifndef _WIN32
    files[i] = open(file_name, O_CREAT | O_RDWR | file_extra_flags,
                    S_IRUSR | S_IWUSR);
#else
    files[i] = CreateFile(file_name, GENERIC_READ|GENERIC_WRITE, 0, NULL,
      OPEN_ALWAYS, file_extra_flags? file_extra_flags : FILE_ATTRIBUTE_NORMAL,
      NULL);
#endif
    if (!VALID_FILE(files[i]))
    {
      log_errno(LOG_FATAL, "Cannot open file");
      return 1; 
    }
  }

#ifdef HAVE_MMAP
  if (file_mmap_prepare())
    return 1;
#endif

  pthread_mutex_init(&fsync_mutex, NULL);
  
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

  if (buffer != NULL)
    sb_free_memaligned(buffer);

  pthread_mutex_destroy(&fsync_mutex);
  
  return 0;
}

sb_request_t file_get_request(int tid)
{
  (void)tid; /* unused */
  
  if (test_mode == MODE_WRITE || test_mode == MODE_REWRITE ||
      test_mode == MODE_READ)
    return file_get_seq_request();
  
  
  return file_get_rnd_request();
}


/* Get sequential read or write request */


sb_request_t file_get_seq_request(void)
{
  sb_request_t         sb_req;
  sb_file_request_t    *file_req = &sb_req.u.file_request;

  sb_req.type = SB_REQ_TYPE_FILE;
  
  /* assume function is called with correct mode always */
  if (test_mode == MODE_WRITE || test_mode == MODE_REWRITE)
    file_req->operation = FILE_OP_TYPE_WRITE;
  else     
    file_req->operation = FILE_OP_TYPE_READ;
  if (file_merged_requests > 0)
  {
    if (position + file_max_request_size <= file_size)
      file_req->size = file_max_request_size;
    else
      file_req->size = file_size - position;    
    file_req->file_id = current_file;
    file_req->pos = position;
  }
  else
  {
    file_req->size = file_block_size;
    file_req->file_id = current_file;
    file_req->pos = position;
  }
  
  /* Advance pointers, if we're not in the fsync phase */ 
  pthread_mutex_lock(&fsync_mutex);
  if(current_file != num_files)
    position += file_req->size;
  /* scroll to the next file if not already out of bound */
  if (position == file_size && current_file != num_files)
  {
    current_file++;
    position=0;

    pthread_mutex_unlock(&fsync_mutex);
    
    if (sb_globals.validate)
    {
      check_seq_req(&prev_req, file_req);
      prev_req = *file_req;
    }
    
    return sb_req; /* This request is valid even for last file */
  }      
  
  /* if we've done with all files stop */
  if (current_file == num_files)
  {
    /* no fsync for reads */
    if (file_fsync_end && file_req->operation == FILE_OP_TYPE_WRITE &&
        fsynced_file < num_files)
    {
      file_req->file_id = fsynced_file;
      file_req->pos = 0;
      file_req->size = 0;
      file_req->operation = FILE_OP_TYPE_FSYNC;
      fsynced_file++;
    }
    else 
      sb_req.type = SB_REQ_TYPE_NULL;
  }  
  pthread_mutex_unlock(&fsync_mutex);

  if (sb_globals.validate)
  {
    check_seq_req(&prev_req, file_req);
    prev_req = *file_req;
  }
  
  return sb_req;    
}


/* Request generatior for random tests */


sb_request_t file_get_rnd_request(void)
{
  sb_request_t         sb_req;
  sb_file_request_t    *file_req = &sb_req.u.file_request;
  unsigned int         randnum;
  unsigned long long   tmppos;
  int                  real_mode = test_mode;
  int                  mode = test_mode;
  
  sb_req.type = SB_REQ_TYPE_FILE;
  
  /*
    Convert mode for combined tests. Locking to get consistent values
    We have to use "real" values for mixed test  
  */
  if (test_mode==MODE_RND_RW)
  {
    SB_THREAD_MUTEX_LOCK();
    if ((double)(real_read_ops + 1) / (real_write_ops + 1) < file_rw_ratio)
      mode=MODE_RND_READ;
    else
      mode=MODE_RND_WRITE;
    SB_THREAD_MUTEX_UNLOCK();
  }

  /* fsync all files (if requested by user) as soon as we are done */
  if (req_performed == sb_globals.max_requests && sb_globals.max_requests > 0)
  {
    if (file_fsync_end != 0 &&
        (real_mode == MODE_RND_WRITE || real_mode == MODE_RND_RW ||
         real_mode == MODE_MIXED))
    {
      pthread_mutex_lock(&fsync_mutex);
      if(fsynced_file2 < num_files)
      {
        file_req->file_id = fsynced_file2;
        file_req->operation = FILE_OP_TYPE_FSYNC;
        file_req->pos = 0;
        file_req->size = 0;
        fsynced_file2++;
        pthread_mutex_unlock(&fsync_mutex);
        
        return sb_req;
      }
      pthread_mutex_unlock(&fsync_mutex);
    }
    sb_req.type = SB_REQ_TYPE_NULL;

    return sb_req;
  }

  /*
    is_dirty is only set if writes are done and cleared after all files
    are synced
  */
  if(file_fsync_freq != 0 && is_dirty)
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
      
      return sb_req;
    }
  }

  randnum=sb_rnd();
  if (mode==MODE_RND_WRITE) /* mode shall be WRITE or RND_WRITE only */
    file_req->operation = FILE_OP_TYPE_WRITE;
  else     
    file_req->operation = FILE_OP_TYPE_READ;

  tmppos = (long long)((double)randnum / (double)SB_MAX_RND * (double)(total_size));
  tmppos = tmppos - (tmppos % (long long)file_block_size);
  file_req->file_id = (int)(tmppos / (long long)file_size);
  file_req->pos = (long long)(tmppos % (long long)file_size);
  file_req->size = file_block_size;

  req_performed++;
  if (file_req->operation == FILE_OP_TYPE_WRITE) 
    is_dirty = 1;

  return sb_req;
}


int file_execute_request(sb_request_t *sb_req, int thread_id)
{
  FILE_DESCRIPTOR    fd;
  sb_file_request_t *file_req = &sb_req->u.file_request;
  log_msg_t          msg;
  log_msg_oper_t     op_msg;
  
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
    log_text(LOG_FATAL, "Incorrect file discovered in request");
    return 1;
  }
  if (file_req->pos + file_req->size > file_size)
  {
    log_text(LOG_FATAL, "Too large position discovered in request!");
    return 1;
  }
  
  fd = files[file_req->file_id];

  /* Prepare log message */
  msg.type = LOG_MSG_TYPE_OPER;
  msg.data = &op_msg;

  switch (file_req->operation) {
    case FILE_OP_TYPE_NULL:
      log_text(LOG_FATAL, "Execute of NULL request called !, aborting");
      return 1;
    case FILE_OP_TYPE_WRITE:

      /* Store checksum and offset in a buffer when in validation mode */
      if (sb_globals.validate)
        file_fill_buffer(buffer, file_req->size, file_req->pos);
                         
      LOG_EVENT_START(msg, thread_id);
      if(file_pwrite(file_req->file_id, buffer, file_req->size, file_req->pos,
                     thread_id)
         != (ssize_t)file_req->size)
      {
        log_errno(LOG_FATAL, "Failed to write file! file: " FD_FMT " pos: %lld", 
                  fd, (long long)file_req->pos);
        return 1;
      }
      /* Check if we have to fsync each write operation */
      if (file_fsync_all)
      {
        if (file_fsync(file_req->file_id, thread_id))
        {
          log_errno(LOG_FATAL, "Failed to fsync file! file: " FD_FMT, fd);
          return 1;
        }
      }
      LOG_EVENT_STOP(msg, thread_id);

      SB_THREAD_MUTEX_LOCK();
      write_ops++;
      real_write_ops++;
      bytes_written += file_req->size;
      if (file_fsync_all)
        other_ops++;
      SB_THREAD_MUTEX_UNLOCK();

      break;
    case FILE_OP_TYPE_READ:
      LOG_EVENT_START(msg, thread_id);
      if(file_pread(file_req->file_id, buffer, file_req->size, file_req->pos,
                    thread_id)
         != (ssize_t)file_req->size)
      {
        log_errno(LOG_FATAL, "Failed to read file! file: " FD_FMT " pos: %lld",
                  fd, (long long)file_req->pos);
        return 1;
      }
      LOG_EVENT_STOP(msg, thread_id);

      /* Validate block if run with validation enabled */
      if (sb_globals.validate &&
          file_validate_buffer(buffer, file_req->size, file_req->pos))
      {
        log_text(LOG_FATAL,
          "Validation failed on file " FD_FMT ", block offset 0x%x, exiting...",
           file_req->file_id, file_req->pos);
        return 1;
      }
      
      SB_THREAD_MUTEX_LOCK();
      read_ops++;
      real_read_ops++;
      bytes_read += file_req->size;

      SB_THREAD_MUTEX_UNLOCK();

      break;
    case FILE_OP_TYPE_FSYNC:
      /* Ignore fsync requests if we are already fsync'ing each operation */
      if (file_fsync_all)
        break;
      if(file_fsync(file_req->file_id, thread_id))
      {
        log_errno(LOG_FATAL, "Failed to fsync file! file: " FD_FMT, fd);
        return 1;
      }
    
      SB_THREAD_MUTEX_LOCK();
      other_ops++;
      SB_THREAD_MUTEX_UNLOCK();
    
      break;         
    default:
      log_text(LOG_FATAL, "Execute of UNKNOWN file request type called (%d)!, "
               "aborting", file_req->operation);
      return 1;
  }
  return 0;

}


void file_print_mode(void)
{
  char sizestr[16];
  
  log_text(LOG_INFO, "Extra file open flags: %d", file_extra_flags);
  log_text(LOG_INFO, "%d files, %sb each", num_files,
           sb_print_value_size(sizestr, sizeof(sizestr), file_size));
  log_text(LOG_INFO, "%sb total file size",
           sb_print_value_size(sizestr, sizeof(sizestr),
                               file_size * num_files));
  log_text(LOG_INFO, "Block size %sb",
           sb_print_value_size(sizestr, sizeof(sizestr), file_block_size));
  if (file_merged_requests > 0)
    log_text(LOG_INFO, "Merging requests  up to %sb for sequential IO.",
             sb_print_value_size(sizestr, sizeof(sizestr),
                                 file_max_request_size));

  switch (test_mode)
  {
    case MODE_RND_WRITE:
    case MODE_RND_READ:
    case MODE_RND_RW:
      log_text(LOG_INFO, "Number of random requests for random IO: %d",
               sb_globals.max_requests);
      log_text(LOG_INFO,
               "Read/Write ratio for combined random IO test: %2.2f",
               file_rw_ratio);
      break;
    default:
      break;
  }

  if (file_fsync_freq > 0)
    log_text(LOG_INFO,
             "Periodic FSYNC enabled, calling fsync() each %d requests.",
             file_fsync_freq);

  if (file_fsync_end)
    log_text(LOG_INFO, "Calling fsync() at the end of test, Enabled.");

  if (file_fsync_all)
    log_text(LOG_INFO, "Calling fsync() after each write operation.");

  log_text(LOG_INFO, "Using %s I/O mode", get_io_mode_str(file_io_mode));

  if (sb_globals.validate)
    log_text(LOG_INFO, "Using checksums validation.");
  
  log_text(LOG_INFO, "Doing %s test", get_test_mode_str(test_mode));
}


/* Print test statistics */


void file_print_stats(void)
{
  double total_time;
  char   s1[16], s2[16], s3[16], s4[16];

  total_time = NS2SEC(sb_timer_value(&sb_globals.exec_timer));
  
  log_text(LOG_NOTICE,
           "Operations performed:  %d Read, %d Write, %d Other = %d Total",
           read_ops, write_ops, other_ops, read_ops + write_ops + other_ops); 
  log_text(LOG_NOTICE, "Read %sb  Written %sb  Total transferred %sb  "
           "(%sb/sec)",
           sb_print_value_size(s1, sizeof(s1), bytes_read),
           sb_print_value_size(s2, sizeof(s2), bytes_written),
           sb_print_value_size(s3, sizeof(s3), bytes_read + bytes_written),
           sb_print_value_size(s4, sizeof(s4),
                               (bytes_read + bytes_written) / total_time));  
  log_text(LOG_NOTICE, "%8.2f Requests/sec executed",
           (read_ops + write_ops) / total_time);  
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


/* Create files of necessary size for test */


int create_files(void)
{
  unsigned int       i;
  int                fd;
  char               file_name[512];
  long long          offset;

  log_text(LOG_INFO, "%d files, %ldKb each, %ldMb total", num_files,
           (long)(file_size / 1024),
           (long)((file_size * num_files) / (1024 * 1024)));
  log_text(LOG_INFO, "Creating files for the test...");
  for (i=0; i < num_files; i++) {
    snprintf(file_name, sizeof(file_name), "test_file.%d",i);
    unlink(file_name);

    fd = open(file_name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd < 0)
    {
      log_errno(LOG_FATAL, "Can't open file");
      return 1; 
    }

    for (offset = 0; offset < file_size; offset += file_block_size)
    {
      /* If in validation mode, fill buffer with random values
         and write checksum
      */
      if (sb_globals.validate)
        file_fill_buffer(buffer, file_block_size, offset);
                         
      if (write(fd, buffer, file_block_size) < 0)
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
  
  log_text(LOG_INFO, "Removing test files...");
  
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

void clear_stats(void)
{
  position = 0; /* position in file */
  current_file = 0;
  fsynced_file = 0; /* for counting file to be fsynced */
  fsynced_file2 = 0;
  read_ops = 0;
  real_read_ops = 0;
  write_ops = 0;
  real_write_ops = 0;
  other_ops = 0;
  bytes_read = 0;
  bytes_written = 0;
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

  aio_ctxts = (sb_aio_context_t *)calloc(sb_globals.num_threads,
                                         sizeof(sb_aio_context_t));
  for (i = 0; i < sb_globals.num_threads; i++)
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

  for (i = 0; i < sb_globals.num_threads; i++)
  {
    io_queue_release(aio_ctxts[i].io_ctxt);
    free(aio_ctxts[i].events);
  }
  
  free(aio_ctxts);

  return 0;
}  


/* Wait for all async operations to complete before the end of the test */


int file_thread_done(int thread_id)
{
  if (file_io_mode == FILE_IO_MODE_ASYNC && aio_ctxts[thread_id].nrequests > 0)
    return file_wait(thread_id, aio_ctxts[thread_id].nrequests);

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
        break;
      case FILE_OP_TYPE_READ:
        if ((ssize_t)event->res != oper->len)
        {
          log_text(LOG_FATAL, "Asynchronous read failed!\n");
          return 1;
        }
        break;
      case FILE_OP_TYPE_WRITE:
        if ((ssize_t)event->res != oper->len)
        {
          log_text(LOG_FATAL, "Asynchronous write failed!\n");
          return 1;
        }
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

int file_fsync(unsigned int file_id, int thread_id)
{
  FILE_DESCRIPTOR fd = files[file_id];
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

#ifdef HAVE_FDATASYNC    
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
    msync(mmaps[file_id], file_size, MS_SYNC | MS_INVALIDATE);
#else
    FlushViewOfFile(mmaps[file_id], (size_t)file_size);
#endif
  }
#endif
  
  return 1; /* Unknown I/O mode */
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
    memcpy(buffer, (char *)start + page_offset, count);
    munmap(start, count + page_offset);

    return count;
# else
    (void)start; /* unused */
    (void)page_addr; /* unused */
    (void)page_offset; /* unused */
    
    /* We already have all files mapped on 64-bit platforms */
    memcpy(buffer, (char *)mmaps[file_id] + offset, count);

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
    memcpy((char *)start + page_offset, buffer, count);
    munmap(start, count + page_offset);

    return count;
# else
    (void)start; /* unused */
    (void)page_addr; /* unused */
    (void)page_offset; /* unused */

    /* We already have all files mapped on 64-bit platforms */
    memcpy((char *)mmaps[file_id] + offset, buffer, count);

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
  if (sb_globals.command == SB_COMMAND_RUN)
  {
    if (mode == NULL)
    {
      log_text(LOG_FATAL, "Missing required argument: --file-test-mode");
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
    file_max_request_size = file_block_size * file_merged_requests;
  else
    file_max_request_size = file_block_size;

  mode = sb_get_value_string("file-extra-flags");
  if (mode == NULL || !strlen(mode))
    file_extra_flags = 0;
  else if (!strcmp(mode, "sync"))
  {
#ifdef _WIN32
    file_extra_flags = FILE_FLAG_WRITE_THROUGH;
#else
    file_extra_flags = O_SYNC;
#endif
  }
  else if (!strcmp(mode, "dsync"))
  {
#ifdef O_DSYNC
    file_extra_flags = O_DSYNC;
#else
    log_text(LOG_FATAL, "O_DSYNC is not supported on this platform.");
    return 1;
#endif
  }
  else if (!strcmp(mode, "direct"))
  {
#ifdef _WIN32
    file_extra_flags = FILE_FLAG_NO_BUFFERING;
#elif defined(O_DIRECT)
    file_extra_flags = O_DIRECT;
#else
    log_text(LOG_FATAL, "O_DIRECT is not supported on this platform.");
    return 1;
#endif
  }
  else
  {
    log_text(LOG_FATAL, "Invalid value for file-extra-flags: %s", mode);
    return 1;
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

  file_rw_ratio = sb_get_value_float("file-rw-ratio");
  if (file_rw_ratio < 0)
  {
    log_text(LOG_FATAL, "Invalid value file file-rw-ratio: %f.", file_rw_ratio);
    return 1;
  }

  buffer = sb_memalign(file_max_request_size);

  return 0;
}


/* check if two request given out are really sequential) */


void check_seq_req(sb_file_request_t *prev_req, sb_file_request_t *r)
{
  /* Do not check fsync operation at the moment */
  if (r->operation == FILE_OP_TYPE_FSYNC || r->operation == FILE_OP_TYPE_NULL)
    return; 
  /* if old request is NULL do not check against it */
  if (prev_req->operation == FILE_OP_TYPE_NULL)
    return;
  /* check files */
  if (r->file_id - prev_req->file_id>1)
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


unsigned long sb_getpagesize(void)
{
#ifdef _SC_PAGESIZE
  return sysconf(_SC_PAGESIZE);
#elif defined _WIN32
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return info.dwPageSize;
#else
  return getpagesize();
#endif
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

/* Allocate a buffer of a specified size and align it to the page size */


void *sb_memalign(size_t size)
{
  unsigned long page_size;
  void *buffer;
  
#ifdef HAVE_POSIX_MEMALIGN
  page_size = sb_getpagesize();
  posix_memalign((void **)&buffer, page_size, size);
#elif defined(HAVE_MEMALIGN)
  page_size = sb_getpagesize();
  buffer = memalign(page_size, size);
#elif defined(HAVE_VALLOC)
  (void)page_size; /* unused */
  buffer = valloc(size);
#elif defined (_WIN32)
  (void)page_size; /* unused */
  buffer = VirtualAlloc(NULL, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
#else
  (void)page_size; /* unused */
  log_text(LOG_WARNING, "None of valloc(), memalign and posix_memalign() is available, doing unaligned IO!");
  buffer = malloc(size);
#endif

  return buffer;
}

static void sb_free_memaligned(void *buf)
{
#ifdef _WIN32
  VirtualFree(buf,0,MEM_FREE);
#else
  free(buf);
#endif
}

/* Fill buffer with random values and write checksum */


void file_fill_buffer(char *buf, unsigned int len, size_t offset)
{
  unsigned int i;

  for (i = 0; i < len - (FILE_CHECKSUM_LENGTH + FILE_OFFSET_LENGTH); i++)
    buf[i] = sb_rnd() & 0xFF;

  /* Store the checksum */
  *(int *)(buf + i) = (int)crc32(0, buf, len -
                                 (FILE_CHECKSUM_LENGTH + FILE_OFFSET_LENGTH));
  /* Store the offset */
  *(long *)(buf + i + FILE_CHECKSUM_LENGTH) = offset;
}


/* Validate checksum and offset of block read from disk */


int file_validate_buffer(char  *buf, unsigned int len, size_t offset)
{
  unsigned int checksum;
  unsigned int cs_offset;

  cs_offset = len - (FILE_CHECKSUM_LENGTH + FILE_OFFSET_LENGTH);
  
  checksum = (unsigned int)crc32(0, buf, cs_offset);

  if (checksum != *(unsigned int *)(buf + cs_offset))
  {
    log_text(LOG_FATAL, "Checksum mismatch in block: ", offset);
    log_text(LOG_FATAL, "    Calculated value: 0x%x    Stored value: 0x%x",
             checksum, *(unsigned int *)(buf + cs_offset));
    return 1;
  }

  if (offset != *(size_t *)(buf + cs_offset + FILE_CHECKSUM_LENGTH))
  {
    log_text(LOG_FATAL, "Offset mismatch in block:");
    log_text(LOG_FATAL, "   Actual offset: %ld    Stored offset: %ld",
             offset, *(size_t *)(buf + cs_offset + FILE_CHECKSUM_LENGTH));
    return 1;
  }

  return 0;
}
