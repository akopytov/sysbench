#ifndef SB_WINPORT_H
#define SB_WINPORT_H
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <time.h>
#include <string.h>
#include <math.h>

#if (_MSC_VER < 1400)
#error "need Visual Studio 2005 or higher"
#endif

#ifndef PACKAGE
#define PACKAGE "sysbench"
#endif
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "0.4"
#endif

#define snprintf(buffer, count,  format,...)  \
  _snprintf_s(buffer,count, _TRUNCATE,format, __VA_ARGS__)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define srandom(seed) srand(seed)
typedef intptr_t ssize_t;

#ifdef _WIN64
#define SIZEOF_SIZE_T 8
#else
#define SIZEOF_SIZE_T 4
#endif

#ifndef __cplusplus
#ifndef inline
#define inline __inline
#endif
#endif

struct  timespec
{
  time_t     tv_sec;
  long long  tv_nsec;
};
typedef HANDLE pthread_t;
typedef CRITICAL_SECTION pthread_mutex_t;

#define SIGNAL     0
#define BROADCAST  1
#define MAX_EVENTS 2
typedef struct _pthread_cond_t
{
  int waiting;
  CRITICAL_SECTION lock_waiting;
 
  HANDLE events[MAX_EVENTS];
  HANDLE broadcast_block_event;
}pthread_cond_t;


typedef struct 
{
	char unused;
}pthread_condattr_t;

typedef struct 
{
	DWORD stacksize;
}pthread_attr_t;

typedef struct
{
	char unused;
}pthread_mutexattr_t;


typedef volatile LONG pthread_once_t;
extern int pthread_once(pthread_once_t *once_control, 
    void (*init_routine)(void));

#define PTHREAD_ONCE_INIT  0
#define PTHREAD_ONCE_INPROGRESS 1
#define PTHREAD_ONCE_DONE 2


extern int pthread_attr_init(pthread_attr_t *attr);
extern int pthread_cond_destroy(pthread_cond_t *cond);
extern int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                           struct timespec *abstime);
extern int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
extern int pthread_cond_signal(pthread_cond_t *cond);
extern int pthread_mutex_lock(pthread_mutex_t *mutex);
extern int pthread_mutex_unlock(pthread_mutex_t *mutex);
extern int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
extern int pthread_mutex_destroy(pthread_mutex_t *mutex);
extern int pthread_create(pthread_t *thread, const pthread_attr_t *attr,  void *(*start_routine)(void*), void *arg);
extern int pthread_attr_setstacksize( pthread_attr_t *attr, size_t stacksize);
extern int pthread_join(pthread_t thread, void **value_ptr);
extern int gettimeofday(struct timeval * tp, void * tzp);
extern int random();

#define ETIMEDOUT 2204

static  __inline int usleep(int micros)
{
	Sleep(micros/1000);
	return 0;
}



#define gmtime_r(a,b) gmtime_s(b,a)

#endif

