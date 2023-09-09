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

/*
  This file contains ports of various, mostly POSIX functions and macros
*/
#pragma once

#include <stdint.h>
#include <time.h>
#include <sys/types.h>

#ifndef __GNUC__
#define ssize_t intptr_t
#endif

/* alarm(), and signal() that understands SIGALRM */
#ifndef HAVE_ALARM
#define HAVE_ALARM
#endif

#ifndef SIGALRM
#define SIGALRM 14
#endif
unsigned int sb_win_alarm(unsigned int seconds);
typedef void (* sb_win_signal_t)(int);
sb_win_signal_t sb_win_signal(int sig, sb_win_signal_t);

/* pread()/write() */
int sb_win_open(const char *name, int oflag);
ssize_t sb_win_pread(int fd, void* buf, size_t count, unsigned long long offset);
ssize_t sb_win_pwrite(int fd, const void* buf, size_t count, unsigned long long offset);
int sb_win_fdatasync(int fd);

/* clock_gettime() */
#ifndef HAVE_CLOCK_GETTIME
int sb_win_clock_gettime(int id,struct timespec* ts);
#endif

/* basename() */
#ifndef HAVE_LIBGEN_H
const char* sb_win_basename(const char* path);
#endif

#ifndef O_SYNC
#define O_SYNC 0x80000
#elif O_SYNC == 0
#error Dummy O_SYNC
#endif

#ifndef O_DIRECT
#define O_DIRECT 0x100000
#elif O_DIRECT == 0
#error Dummy O_DIRECT
#endif

#ifdef SB_WIN_POSIX_NAMES
#include <windows.h>
#include <signal.h>
#include <sys/stat.h>
#include <io.h>
#ifndef S_IRUSR
#define S_IRUSR 0x0100
#endif
#ifndef S_IWUSR
#define S_IWUSR 0x0080
#endif
#define alarm(sec) sb_win_alarm(sec)
#define signal(sig, func) sb_win_signal(sig, func)
#define pread(fd, buf, count, offset) sb_win_pread(fd, buf, count, offset)
#define pwrite(fd, buf, count, offset) sb_win_pwrite(fd, buf, count, offset)
#define fsync(x) _commit(x)
#define stat _stat64
#define fstat _fstat64

#define strcasecmp _stricmp
#define random() rand()
#define srandom(seed) srand(seed)
#ifndef HAVE_CLOCK_GETTIME
#define HAVE_CLOCK_GETTIME
#define CLOCK_MONOTONIC 0
#define clock_gettime(id, ts) sb_win_clock_gettime(id,ts)
#endif
#ifndef HAVE_LIBGEN_H
#define basename(path) sb_win_basename(path)
#endif
#ifndef HAVE_FDATASYNC
#define HAVE_FDATASYNC
#define fdatasync(fd) sb_win_fdatasync(fd)
#endif

#endif
