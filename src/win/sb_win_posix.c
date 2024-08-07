/*
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

#include <windows.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <io.h>
#include <string.h>
#include <winternl.h>
#include <fcntl.h>
#include "sb_win_posix.h"

/*
  alarm(2) on Windows, implemented with timer queue timers
*/
static HANDLE timer_handle;
static sb_win_signal_t sigalrm_handler;
static time_t timer_deadline;

static void CALLBACK timer_func(void* param, BOOLEAN fired)
{
  (void)param;
  (void)fired;
  sigalrm_handler(SIGALRM);
}

unsigned int sb_win_alarm(unsigned int seconds)
{
  unsigned int ret = 0;
  if (timer_handle)
  {
    (void)DeleteTimerQueueTimer(NULL, timer_handle, INVALID_HANDLE_VALUE);
    timer_handle = NULL;
    ret = (unsigned int)(timer_deadline - time(NULL));
  }
  if (!seconds)
  {
    return ret;
  }

  timer_deadline = time(NULL) + seconds;
  if (!CreateTimerQueueTimer(&timer_handle, NULL, timer_func, NULL,
      seconds * 1000, 0, WT_EXECUTEONLYONCE))
  {
    abort(); /* alarm does can't return errors, have to abort */
  }
  return ret;
}

/*
  signal(), capable of handling SIGALRM.

  CRT signal() does runtime checks, and disallows unknown
  signals, so we have to implement our own.
*/
sb_win_signal_t sb_win_signal(int sig, sb_win_signal_t func)
{
  if (sig == SIGALRM)
  {
    sb_win_signal_t old = sigalrm_handler;
    sigalrm_handler = func;
    return old;
  }
  return signal(sig, func);
}

static inline HANDLE get_handle(int fd)
{
  intptr_t h = _get_osfhandle(fd);
  return (HANDLE)h;
}


/* pread() and pwrite()*/
ssize_t sb_win_pread(int fd, void* buf, size_t count, unsigned long long offset)
{
  HANDLE hFile = get_handle(fd);
  if (hFile == INVALID_HANDLE_VALUE)
  {
    return -1;
  }

  OVERLAPPED overlapped;
  memset(&overlapped, 0, sizeof(overlapped));
  overlapped.Offset = (DWORD)offset;
  overlapped.OffsetHigh = (DWORD)(offset >> 32);

  DWORD bytes_read;
  if (!ReadFile(hFile, buf, (DWORD)count, &bytes_read, &overlapped))
  {
    return -1;
  }

  return (intptr_t)bytes_read;
}

ssize_t sb_win_pwrite(int fd, const void* buf, size_t count, unsigned long long offset)
{
  HANDLE hFile = get_handle(fd);
  if (hFile == INVALID_HANDLE_VALUE)
  {
    return -1;
  }
  OVERLAPPED overlapped;
  memset(&overlapped, 0, sizeof(overlapped));
  overlapped.Offset = (DWORD)offset;
  overlapped.OffsetHigh = (DWORD)(offset >> 32);

  DWORD bytes_written;
  if (!WriteFile(hFile, buf, (DWORD)count, &bytes_written, &overlapped))
  {
    return -1;
  }
  return (intptr_t)bytes_written;
}

/* Get cached query performance frequency value. */
static inline long long get_qpf(void)
{
  static LARGE_INTEGER f;
  static int initialized;
  if (initialized)
  {
    return f.QuadPart;
  }
  QueryPerformanceFrequency(&f);
  initialized = 1;
  return f.QuadPart;
}

/* clock_gettime() */
int sb_win_clock_gettime(int clock_id, struct timespec* tv)
{
  (void)clock_id;
  long long freq = get_qpf();
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  long long now = counter.QuadPart;
  tv->tv_sec = (time_t)(now / freq);
  tv->tv_nsec = (long)((now % freq) * 1000000000LL / freq);
  return 0;
}

const char* sb_win_basename(const char* path)
{
  const char* p1 = strrchr(path, '\\');
  const char* p2 = strrchr(path, '/');
  if (p1 > p2)
    return p1 + 1;
  if (p1 < p2)
    return p2 + 1;
  return path;
}


/*
  Windows' "fdatasync", NtFlushBuffersFileEx.
  Needs to be loaded dynamically, since not in public headers.
  Available since Windows 10 1607.
*/
typedef NTSTATUS(WINAPI *NtFlushBuffersFileEx_func)(
    HANDLE FileHandle, ULONG Flags, PVOID Parameters, ULONG ParametersSize,
    PIO_STATUS_BLOCK IoStatusBlock);
static NtFlushBuffersFileEx_func sb_NtFlushBuffersFileEx;


#ifndef FLUSH_FLAGS_FILE_DATA_SYNC_ONLY
#define FLUSH_FLAGS_FILE_DATA_SYNC_ONLY 0x00000004
#endif

static BOOL CALLBACK load_NtFlushBuffersFileEx(
  PINIT_ONCE initOnce,  PVOID Parameter,  PVOID *Context)
{
  (void) initOnce;
  (void) Parameter;
  (void) Context;
  sb_NtFlushBuffersFileEx = (NtFlushBuffersFileEx_func) (void *)
    GetProcAddress( GetModuleHandle("ntdll"), "NtFlushBuffersFileEx");
  return TRUE;
}

int sb_win_fdatasync(int fd)
{
  static INIT_ONCE init_once= INIT_ONCE_STATIC_INIT;
  static BOOL disable_datasync;
  InitOnceExecuteOnce(&init_once, load_NtFlushBuffersFileEx, NULL, NULL);

  HANDLE h = get_handle(fd);
  if (h == NULL || h == INVALID_HANDLE_VALUE)
    return -1;

  if (!disable_datasync && sb_NtFlushBuffersFileEx)
  {
    IO_STATUS_BLOCK iosb;
    memset(&iosb, 0, sizeof(iosb));
    NTSTATUS status = sb_NtFlushBuffersFileEx( h, FLUSH_FLAGS_FILE_DATA_SYNC_ONLY,
                                               NULL, 0, &iosb);
    if (!status)
    {
      return 0;
    }
  }

  /* Fallback to full sync. */
  if (FlushFileBuffers(h))
  {
    if (!disable_datasync)
      disable_datasync = TRUE;
    return 0;
  }
  errno = EINVAL;
  return -1;
}

/*
  Variation of open() that additionally understands O_SYNC and O_DIRECT
*/
int sb_win_open(const char *path, int oflag)
{
  DWORD access_flag;
  switch (oflag & (O_RDONLY | O_WRONLY | O_RDWR))
  {
  case O_RDONLY:
    access_flag = GENERIC_READ;
    break;
  case O_WRONLY:
    access_flag = GENERIC_WRITE;
    break;
  case O_RDWR:
    access_flag = GENERIC_READ | GENERIC_WRITE;
    break;
  default:
    errno = EINVAL;
    return -1;
  }

  DWORD disposition;
  switch (oflag & (O_CREAT | O_EXCL | O_TRUNC))
  {
  case 0:
  case O_EXCL:
    disposition = OPEN_EXISTING;
    break;
  case O_CREAT:
    disposition = OPEN_ALWAYS;
    break;
  case O_CREAT | O_EXCL:
  case O_CREAT | O_TRUNC | O_EXCL:
    disposition = CREATE_NEW;
    break;
  case O_TRUNC:
  case O_TRUNC | O_EXCL:
    disposition = TRUNCATE_EXISTING;
    break;
  case O_CREAT | O_TRUNC:
    disposition = CREATE_ALWAYS;
    break;
  default:
    errno = EINVAL;
    return -1;
  }

  DWORD attr = FILE_ATTRIBUTE_NORMAL;
  if (oflag & O_DIRECT)
  {
    attr |= FILE_FLAG_NO_BUFFERING;
  }
  if (oflag & O_SYNC)
  {
    attr |= FILE_FLAG_WRITE_THROUGH;
  }

  HANDLE h = CreateFile(path, access_flag, FILE_SHARE_READ, NULL,
                       disposition, attr, NULL);
  if (h == INVALID_HANDLE_VALUE)
  {
    DWORD last_error = GetLastError();
    switch (last_error)
    {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
      errno = ENOENT;
      break;
    case ERROR_ACCESS_DENIED:
    case ERROR_SHARING_VIOLATION:
      errno = EACCES;
      break;
    case ERROR_FILE_EXISTS:
      errno = EEXIST;
      break;
    default:
      errno = EINVAL;
      break;
    }
    return -1;
  }
  int fd = _open_osfhandle((intptr_t)h, 0);
  if (fd < 0)
    CloseHandle(h);
  return fd;
}
