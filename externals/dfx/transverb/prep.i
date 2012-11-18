# 1 "transverb.cpp"

 

# 1 "/usr/include/string.h" 1 3
 

















 






# 1 "/usr/include/features.h" 1 3
 




















 






























































 




















 





 



 







 
# 142 "/usr/include/features.h" 3


 









 








 



























# 208 "/usr/include/features.h" 3


































 



 


 








 




 
















 


# 1 "/usr/include/sys/cdefs.h" 1 3
 




















 




 






 





 








 



# 65 "/usr/include/sys/cdefs.h" 3


 





 




 









 







 

















 















 







 






 








 








 








 











 










 







 




 


















# 283 "/usr/include/features.h" 2 3



 








 





 

 








# 1 "/usr/include/gnu/stubs.h" 1 3
 



















# 312 "/usr/include/features.h" 2 3




# 26 "/usr/include/string.h" 2 3


extern "C" { 

 


# 1 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 


































typedef unsigned int size_t;






















 




 

# 271 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


# 283 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3




 





















# 33 "/usr/include/string.h" 2 3



 
extern void *memcpy (void *__restrict __dest,
		     __const void *__restrict __src, size_t __n) throw () ;
 

extern void *memmove (void *__dest, __const void *__src, size_t __n)
     throw () ;

 



extern void *memccpy (void *__restrict __dest, __const void *__restrict __src,
		      int __c, size_t __n)
     throw () ;



 
extern void *memset (void *__s, int __c, size_t __n) throw () ;

 
extern int memcmp (__const void *__s1, __const void *__s2, size_t __n)
     throw ()   ;

 
extern void *memchr (__const void *__s, int __c, size_t __n)
      throw ()   ;

# 73 "/usr/include/string.h" 3



 
extern char *strcpy (char *__restrict __dest, __const char *__restrict __src)
     throw () ;
 
extern char *strncpy (char *__restrict __dest,
		      __const char *__restrict __src, size_t __n) throw () ;

 
extern char *strcat (char *__restrict __dest, __const char *__restrict __src)
     throw () ;
 
extern char *strncat (char *__restrict __dest, __const char *__restrict __src,
		      size_t __n) throw () ;

 
extern int strcmp (__const char *__s1, __const char *__s2)
     throw ()   ;
 
extern int strncmp (__const char *__s1, __const char *__s2, size_t __n)
     throw ()   ;

 
extern int strcoll (__const char *__s1, __const char *__s2)
     throw ()   ;
 
extern size_t strxfrm (char *__restrict __dest,
		       __const char *__restrict __src, size_t __n) throw () ;

# 116 "/usr/include/string.h" 3



 
extern char *strdup (__const char *__s) throw ()   ;


 







# 152 "/usr/include/string.h" 3


 
extern char *strchr (__const char *__s, int __c) throw ()   ;
 
extern char *strrchr (__const char *__s, int __c) throw ()   ;







 

extern size_t strcspn (__const char *__s, __const char *__reject)
     throw ()   ;
 

extern size_t strspn (__const char *__s, __const char *__accept)
     throw ()   ;
 
extern char *strpbrk (__const char *__s, __const char *__accept)
     throw ()   ;
 
extern char *strstr (__const char *__haystack, __const char *__needle)
     throw ()   ;







 
extern char *strtok (char *__restrict __s, __const char *__restrict __delim)
     throw () ;

 

extern char *__strtok_r (char *__restrict __s,
			 __const char *__restrict __delim,
			 char **__restrict __save_ptr) throw () ;

extern char *strtok_r (char *__restrict __s, __const char *__restrict __delim,
		       char **__restrict __save_ptr) throw () ;


# 214 "/usr/include/string.h" 3



 
extern size_t strlen (__const char *__s) throw ()   ;









 
extern char *strerror (int __errnum) throw () ;

 

extern char *strerror_r (int __errnum, char *__buf, size_t __buflen) throw () ;


 

extern void __bzero (void *__s, size_t __n) throw () ;


 
extern void bcopy (__const void *__src, void *__dest, size_t __n) throw () ;

 
extern void bzero (void *__s, size_t __n) throw () ;

 
extern int bcmp (__const void *__s1, __const void *__s2, size_t __n)
     throw ()   ;

 
extern char *index (__const char *__s, int __c) throw ()   ;

 
extern char *rindex (__const char *__s, int __c) throw ()   ;

 

extern int ffs (int __i) throw ()  __attribute__ ((__const__));

 









 
extern int strcasecmp (__const char *__s1, __const char *__s2)
     throw ()   ;

 
extern int strncasecmp (__const char *__s1, __const char *__s2, size_t __n)
     throw ()   ;


# 289 "/usr/include/string.h" 3



 

extern char *strsep (char **__restrict __stringp,
		     __const char *__restrict __delim) throw () ;


# 332 "/usr/include/string.h" 3




# 361 "/usr/include/string.h" 3



} 


# 4 "transverb.cpp" 2

# 1 "/usr/include/time.h" 1 3
 

















 










extern "C" { 




 


# 1 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3





 




 

# 271 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


# 283 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3




 





















# 38 "/usr/include/time.h" 2 3


 

# 1 "/usr/include/bits/time.h" 1 3
 


















 







 


 





 

# 1 "/usr/include/bits/types.h" 1 3
 

















 









# 1 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3





 




 

# 271 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


# 283 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3




 





















# 29 "/usr/include/bits/types.h" 2 3


 
typedef unsigned char __u_char;
typedef unsigned short __u_short;
typedef unsigned int __u_int;
typedef unsigned long __u_long;

__extension__ typedef unsigned long long int __u_quad_t;
__extension__ typedef long long int __quad_t;
# 48 "/usr/include/bits/types.h" 3

typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;

__extension__ typedef signed long long int __int64_t;
__extension__ typedef unsigned long long int __uint64_t;

typedef __quad_t *__qaddr_t;

typedef __u_quad_t __dev_t;		 
typedef __u_int __uid_t;		 
typedef __u_int __gid_t;		 
typedef __u_long __ino_t;		 
typedef __u_int __mode_t;		 
typedef __u_int __nlink_t; 		 
typedef long int __off_t;		 
typedef __quad_t __loff_t;		 
typedef int __pid_t;			 
typedef int __ssize_t;			 
typedef __u_long __rlim_t;		 
typedef __u_quad_t __rlim64_t;		 
typedef __u_int __id_t;			 

typedef struct
  {
    int __val[2];
  } __fsid_t;				 

 
typedef int __daddr_t;			 
typedef char *__caddr_t;
typedef long int __time_t;
typedef unsigned int __useconds_t;
typedef long int __suseconds_t;
typedef long int __swblk_t;		 

typedef long int __clock_t;

 
typedef int __clockid_t;

 
typedef int __timer_t;


 



typedef int __key_t;

 
typedef unsigned short int __ipc_pid_t;


 
typedef long int __blksize_t;

 

 
typedef long int __blkcnt_t;
typedef __quad_t __blkcnt64_t;

 
typedef __u_long __fsblkcnt_t;
typedef __u_quad_t __fsblkcnt64_t;

 
typedef __u_long __fsfilcnt_t;
typedef __u_quad_t __fsfilcnt64_t;

 
typedef __u_quad_t __ino64_t;

 
typedef __loff_t __off64_t;

 
typedef long int __t_scalar_t;
typedef unsigned long int __t_uscalar_t;

 
typedef int __intptr_t;

 
typedef unsigned int __socklen_t;


 

# 1 "/usr/include/bits/pthreadtypes.h" 1 3
 
 
 
 
 
 
 
 
 
 
 
 
 









# 1 "/usr/include/bits/sched.h" 1 3
 



















# 62 "/usr/include/bits/sched.h" 3





 
struct __sched_param
  {
    int __sched_priority;
  };


# 23 "/usr/include/bits/pthreadtypes.h" 2 3


typedef int __atomic_lock_t;

 
struct _pthread_fastlock
{
  long int __status;    
  __atomic_lock_t __spinlock;   

};


 
typedef struct _pthread_descr_struct *_pthread_descr;




 
typedef struct __pthread_attr_s
{
  int __detachstate;
  int __schedpolicy;
  struct __sched_param __schedparam;
  int __inheritsched;
  int __scope;
  size_t __guardsize;
  int __stackaddr_set;
  void *__stackaddr;
  size_t __stacksize;
} pthread_attr_t;


 
typedef struct
{
  struct _pthread_fastlock __c_lock;  
  _pthread_descr __c_waiting;         
} pthread_cond_t;


 
typedef struct
{
  int __dummy;
} pthread_condattr_t;

 
typedef unsigned int pthread_key_t;


 
 

typedef struct
{
  int __m_reserved;                
  int __m_count;                   
  _pthread_descr __m_owner;        
  int __m_kind;                    
  struct _pthread_fastlock __m_lock;  
} pthread_mutex_t;


 
typedef struct
{
  int __mutexkind;
} pthread_mutexattr_t;


 
typedef int pthread_once_t;


# 119 "/usr/include/bits/pthreadtypes.h" 3


# 138 "/usr/include/bits/pthreadtypes.h" 3



 
typedef unsigned long int pthread_t;


# 143 "/usr/include/bits/types.h" 2 3




# 39 "/usr/include/bits/time.h" 2 3

extern long int __sysconf (int);




 

 

 


 






# 73 "/usr/include/bits/time.h" 3

# 42 "/usr/include/time.h" 2 3


 













 
typedef __clock_t clock_t;









 
typedef __time_t time_t;










 
typedef __clockid_t clockid_t;










 
typedef __timer_t timer_t;









 

struct timespec
  {
    __time_t tv_sec;		 
    long int tv_nsec;		 
  };






 
struct tm
{
  int tm_sec;			 
  int tm_min;			 
  int tm_hour;			 
  int tm_mday;			 
  int tm_mon;			 
  int tm_year;			 
  int tm_wday;			 
  int tm_yday;			 
  int tm_isdst;			 


  long int tm_gmtoff;		 
  __const char *tm_zone;	 




};



 
struct itimerspec
  {
    struct timespec it_interval;
    struct timespec it_value;
  };

 
struct sigevent;











 

extern clock_t clock (void) throw () ;

 
extern time_t time (time_t *__timer) throw () ;

 
extern double difftime (time_t __time1, time_t __time0)
     throw ()  __attribute__ ((__const__));

 
extern time_t mktime (struct tm *__tp) throw () ;


 


extern size_t strftime (char *__restrict __s, size_t __maxsize,
			__const char *__restrict __format,
			__const struct tm *__restrict __tp) throw () ;










 

extern struct tm *gmtime (__const time_t *__timer) throw () ;

 

extern struct tm *localtime (__const time_t *__timer) throw () ;


 

extern struct tm *gmtime_r (__const time_t *__restrict __timer,
			    struct tm *__restrict __tp) throw () ;

 

extern struct tm *localtime_r (__const time_t *__restrict __timer,
			       struct tm *__restrict __tp) throw () ;


 

extern char *asctime (__const struct tm *__tp) throw () ;

 
extern char *ctime (__const time_t *__timer) throw () ;


 

 

extern char *asctime_r (__const struct tm *__restrict __tp,
			char *__restrict __buf) throw () ;

 
extern char *ctime_r (__const time_t *__restrict __timer,
		      char *__restrict __buf) throw () ;



 
extern char *__tzname[2];	 
extern int __daylight;		 
extern long int __timezone;	 



 
extern char *tzname[2];

 

extern void tzset (void) throw () ;



extern int daylight;
extern long int timezone;



 

extern int stime (__const time_t *__when) throw () ;



 






 


 
extern time_t timegm (struct tm *__tp) throw () ;

 
extern time_t timelocal (struct tm *__tp) throw () ;

 
extern int dysize (int __year) throw ()   __attribute__ ((__const__));




 
extern int nanosleep (__const struct timespec *__requested_time,
		      struct timespec *__remaining) throw () ;


 
extern int clock_getres (clockid_t __clock_id, struct timespec *__res) throw () ;

 
extern int clock_gettime (clockid_t __clock_id, struct timespec *__tp) throw () ;

 
extern int clock_settime (clockid_t __clock_id, __const struct timespec *__tp)
     throw () ;

# 305 "/usr/include/time.h" 3



 
extern int timer_create (clockid_t __clock_id,
			 struct sigevent *__restrict __evp,
			 timer_t *__restrict __timerid) throw () ;

 
extern int timer_delete (timer_t __timerid) throw () ;

 
extern int timer_settime (timer_t __timerid, int __flags,
			  __const struct itimerspec *__restrict __value,
			  struct itimerspec *__restrict __ovalue) throw () ;

 
extern int timer_gettime (timer_t __timerid, struct itimerspec *__value)
     throw () ;

 
extern int timer_getoverrun (timer_t __timerid) throw () ;



# 349 "/usr/include/time.h" 3


# 359 "/usr/include/time.h" 3



} 




# 5 "transverb.cpp" 2

# 1 "/usr/include/stdlib.h" 1 3
 

















 







 





# 1 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3





 




 





























 












































# 283 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3




 





















# 33 "/usr/include/stdlib.h" 2 3


extern "C" { 




# 91 "/usr/include/stdlib.h" 3


 
typedef struct
  {
    int quot;			 
    int rem;			 
  } div_t;

 

typedef struct
  {
    long int quot;		 
    long int rem;		 
  } ldiv_t;



# 118 "/usr/include/stdlib.h" 3



 



 





 

extern size_t __ctype_get_mb_cur_max (void) throw () ;


 
extern double atof (__const char *__nptr) throw ()   ;
 
extern int atoi (__const char *__nptr) throw ()   ;
 
extern long int atol (__const char *__nptr) throw ()   ;


 
__extension__ extern long long int atoll (__const char *__nptr)
     throw ()   ;


 
extern double strtod (__const char *__restrict __nptr,
		      char **__restrict __endptr) throw () ;










 
extern long int strtol (__const char *__restrict __nptr,
			char **__restrict __endptr, int __base) throw () ;
 
extern unsigned long int strtoul (__const char *__restrict __nptr,
				  char **__restrict __endptr, int __base)
     throw () ;


 
__extension__
extern long long int strtoq (__const char *__restrict __nptr,
			     char **__restrict __endptr, int __base) throw () ;
 
__extension__
extern unsigned long long int strtouq (__const char *__restrict __nptr,
				       char **__restrict __endptr, int __base)
     throw () ;



 

 
__extension__
extern long long int strtoll (__const char *__restrict __nptr,
			      char **__restrict __endptr, int __base) throw () ;
 
__extension__
extern unsigned long long int strtoull (__const char *__restrict __nptr,
					char **__restrict __endptr, int __base)
     throw () ;



# 244 "/usr/include/stdlib.h" 3



 


extern double __strtod_internal (__const char *__restrict __nptr,
				 char **__restrict __endptr, int __group)
     throw () ;
extern float __strtof_internal (__const char *__restrict __nptr,
				char **__restrict __endptr, int __group)
     throw () ;
extern long double __strtold_internal (__const char *__restrict __nptr,
				       char **__restrict __endptr,
				       int __group) throw () ;

extern long int __strtol_internal (__const char *__restrict __nptr,
				   char **__restrict __endptr,
				   int __base, int __group) throw () ;



extern unsigned long int __strtoul_internal (__const char *__restrict __nptr,
					     char **__restrict __endptr,
					     int __base, int __group) throw () ;




__extension__
extern long long int __strtoll_internal (__const char *__restrict __nptr,
					 char **__restrict __endptr,
					 int __base, int __group) throw () ;



__extension__
extern unsigned long long int __strtoull_internal (__const char *
						   __restrict __nptr,
						   char **__restrict __endptr,
						   int __base, int __group)
     throw () ;





 

extern __inline double
strtod (__const char *__restrict __nptr, char **__restrict __endptr) throw () 
{
  return __strtod_internal (__nptr, __endptr, 0);
}
extern __inline long int
strtol (__const char *__restrict __nptr, char **__restrict __endptr,
	int __base) throw () 
{
  return __strtol_internal (__nptr, __endptr, __base, 0);
}
extern __inline unsigned long int
strtoul (__const char *__restrict __nptr, char **__restrict __endptr,
	 int __base) throw () 
{
  return __strtoul_internal (__nptr, __endptr, __base, 0);
}

# 322 "/usr/include/stdlib.h" 3



__extension__ extern __inline long long int
strtoq (__const char *__restrict __nptr, char **__restrict __endptr,
	int __base) throw () 
{
  return __strtoll_internal (__nptr, __endptr, __base, 0);
}
__extension__ extern __inline unsigned long long int
strtouq (__const char *__restrict __nptr, char **__restrict __endptr,
	 int __base) throw () 
{
  return __strtoull_internal (__nptr, __endptr, __base, 0);
}



__extension__ extern __inline long long int
strtoll (__const char *__restrict __nptr, char **__restrict __endptr,
	 int __base) throw () 
{
  return __strtoll_internal (__nptr, __endptr, __base, 0);
}
__extension__ extern __inline unsigned long long int
strtoull (__const char * __restrict __nptr, char **__restrict __endptr,
	  int __base) throw () 
{
  return __strtoull_internal (__nptr, __endptr, __base, 0);
}


extern __inline double
atof (__const char *__nptr) throw () 
{
  return strtod (__nptr, (char **) __null );
}
extern __inline int
atoi (__const char *__nptr) throw () 
{
  return (int) strtol (__nptr, (char **) __null , 10);
}
extern __inline long int
atol (__const char *__nptr) throw () 
{
  return strtol (__nptr, (char **) __null , 10);
}


__extension__ extern __inline long long int
atoll (__const char *__nptr) throw () 
{
  return strtoll (__nptr, (char **) __null , 10);
}





 


extern char *l64a (long int __n) throw () ;

 
extern long int a64l (__const char *__s) throw ()   ;


# 1 "/usr/include/sys/types.h" 1 3
 

















 








extern "C" { 





typedef __u_char u_char;
typedef __u_short u_short;
typedef __u_int u_int;
typedef __u_long u_long;
typedef __quad_t quad_t;
typedef __u_quad_t u_quad_t;
typedef __fsid_t fsid_t;




typedef __loff_t loff_t;



typedef __ino_t ino_t;











typedef __dev_t dev_t;




typedef __gid_t gid_t;




typedef __mode_t mode_t;




typedef __nlink_t nlink_t;




typedef __uid_t uid_t;





typedef __off_t off_t;











typedef __pid_t pid_t;




typedef __id_t id_t;




typedef __ssize_t ssize_t;





typedef __daddr_t daddr_t;
typedef __caddr_t caddr_t;





typedef __key_t key_t;











# 143 "/usr/include/sys/types.h" 3



# 1 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3





 




 

# 271 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


# 283 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3




 





















# 146 "/usr/include/sys/types.h" 2 3



 
typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;


 

# 180 "/usr/include/sys/types.h" 3


 







typedef int int8_t __attribute__ ((__mode__ (  __QI__ ))) ;
typedef int int16_t __attribute__ ((__mode__ (  __HI__ ))) ;
typedef int int32_t __attribute__ ((__mode__ (  __SI__ ))) ;
typedef int int64_t __attribute__ ((__mode__ (  __DI__ ))) ;


typedef unsigned int u_int8_t __attribute__ ((__mode__ (  __QI__ ))) ;
typedef unsigned int u_int16_t __attribute__ ((__mode__ (  __HI__ ))) ;
typedef unsigned int u_int32_t __attribute__ ((__mode__ (  __SI__ ))) ;
typedef unsigned int u_int64_t __attribute__ ((__mode__ (  __DI__ ))) ;

typedef int register_t __attribute__ ((__mode__ (__word__)));


 






 
# 1 "/usr/include/endian.h" 1 3
 






















 











 
# 1 "/usr/include/bits/endian.h" 1 3
 






# 37 "/usr/include/endian.h" 2 3


 



















# 212 "/usr/include/sys/types.h" 2 3


 
# 1 "/usr/include/sys/select.h" 1 3
 


















 






 


 
# 1 "/usr/include/bits/select.h" 1 3
 

























# 36 "/usr/include/bits/select.h" 3












# 56 "/usr/include/bits/select.h" 3

# 72 "/usr/include/bits/select.h" 3

# 31 "/usr/include/sys/select.h" 2 3


 
# 1 "/usr/include/bits/sigset.h" 1 3
 





















typedef int __sig_atomic_t;

 


typedef struct
  {
    unsigned long int __val[(1024 / (8 * sizeof (unsigned long int))) ];
  } __sigset_t;




 





# 125 "/usr/include/bits/sigset.h" 3

# 34 "/usr/include/sys/select.h" 2 3




typedef __sigset_t sigset_t;


 




# 1 "/usr/include/bits/time.h" 1 3
 


















 



# 57 "/usr/include/bits/time.h" 3








 

struct timeval
  {
    __time_t tv_sec;		 
    __suseconds_t tv_usec;	 
  };


# 46 "/usr/include/sys/select.h" 2 3



typedef __suseconds_t suseconds_t;




 
typedef long int __fd_mask;

 




 
typedef struct
  {
     





    __fd_mask __fds_bits[1024  / (8 * sizeof (__fd_mask)) ];


  } fd_set;

 



 
typedef __fd_mask fd_mask;

 




 






extern "C" { 

 




extern int select (int __nfds, fd_set *__restrict __readfds,
		   fd_set *__restrict __writefds,
		   fd_set *__restrict __exceptfds,
		   struct timeval *__restrict __timeout) throw () ;

# 116 "/usr/include/sys/select.h" 3


} 


# 215 "/usr/include/sys/types.h" 2 3


 
# 1 "/usr/include/sys/sysmacros.h" 1 3
 





















 








# 47 "/usr/include/sys/sysmacros.h" 3



# 218 "/usr/include/sys/types.h" 2 3









 


typedef __blkcnt_t blkcnt_t;	  



typedef __fsblkcnt_t fsblkcnt_t;  



typedef __fsfilcnt_t fsfilcnt_t;  


# 254 "/usr/include/sys/types.h" 3








} 


# 390 "/usr/include/stdlib.h" 2 3


 



 
extern long int random (void) throw () ;

 
extern void srandom (unsigned int __seed) throw () ;

 



extern char *initstate (unsigned int __seed, char *__statebuf,
			size_t __statelen) throw () ;

 

extern char *setstate (char *__statebuf) throw () ;



 



struct random_data
  {
    int32_t *fptr;		 
    int32_t *rptr;		 
    int32_t *state;		 
    int rand_type;		 
    int rand_deg;		 
    int rand_sep;		 
    int32_t *end_ptr;		 
  };

extern int random_r (struct random_data *__restrict __buf,
		     int32_t *__restrict __result) throw () ;

extern int srandom_r (unsigned int __seed, struct random_data *__buf) throw () ;

extern int initstate_r (unsigned int __seed, char *__restrict __statebuf,
			size_t __statelen,
			struct random_data *__restrict __buf) throw () ;

extern int setstate_r (char *__restrict __statebuf,
		       struct random_data *__restrict __buf) throw () ;




 
extern int rand (void) throw () ;
 
extern void srand (unsigned int __seed) throw () ;


 
extern int rand_r (unsigned int *__seed) throw () ;




 

 
extern double drand48 (void) throw () ;
extern double erand48 (unsigned short int __xsubi[3]) throw () ;

 
extern long int lrand48 (void) throw () ;
extern long int nrand48 (unsigned short int __xsubi[3]) throw () ;

 
extern long int mrand48 (void) throw () ;
extern long int jrand48 (unsigned short int __xsubi[3]) throw () ;

 
extern void srand48 (long int __seedval) throw () ;
extern unsigned short int *seed48 (unsigned short int __seed16v[3]) throw () ;
extern void lcong48 (unsigned short int __param[7]) throw () ;


 


struct drand48_data
  {
    unsigned short int __x[3];	 
    unsigned short int __old_x[3];  
    unsigned short int __c;	 
    unsigned short int __init;	 
    unsigned long long int __a;	 
  };

 
extern int drand48_r (struct drand48_data *__restrict __buffer,
		      double *__restrict __result) throw () ;
extern int erand48_r (unsigned short int __xsubi[3],
		      struct drand48_data *__restrict __buffer,
		      double *__restrict __result) throw () ;

 
extern int lrand48_r (struct drand48_data *__restrict __buffer,
		      long int *__restrict __result) throw () ;
extern int nrand48_r (unsigned short int __xsubi[3],
		      struct drand48_data *__restrict __buffer,
		      long int *__restrict __result) throw () ;

 
extern int mrand48_r (struct drand48_data *__restrict __buffer,
		      long int *__restrict __result) throw () ;
extern int jrand48_r (unsigned short int __xsubi[3],
		      struct drand48_data *__restrict __buffer,
		      long int *__restrict __result) throw () ;

 
extern int srand48_r (long int __seedval, struct drand48_data *__buffer)
     throw () ;

extern int seed48_r (unsigned short int __seed16v[3],
		     struct drand48_data *__buffer) throw () ;

extern int lcong48_r (unsigned short int __param[7],
		      struct drand48_data *__buffer) throw () ;







 
extern void *malloc (size_t __size) throw ()   ;
 
extern void *calloc (size_t __nmemb, size_t __size)
     throw ()   ;



 

extern void *realloc (void *__ptr, size_t __size) throw ()   ;
 
extern void free (void *__ptr) throw () ;


 
extern void cfree (void *__ptr) throw () ;



# 1 "/usr/include/alloca.h" 1 3
 























# 1 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3





 




 

# 271 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


# 283 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3




 





















# 25 "/usr/include/alloca.h" 2 3


extern "C" { 

 


 
extern void *alloca (size_t __size) throw () ;





} 


# 546 "/usr/include/stdlib.h" 2 3




 
extern void *valloc (size_t __size) throw ()   ;








 
extern void abort (void) throw ()  __attribute__ ((__noreturn__));


 
extern int atexit (void (*__func) (void)) throw () ;


 

extern int on_exit (void (*__func) (int __status, void *__arg), void *__arg)
     throw () ;


 


extern void exit (int __status) throw ()  __attribute__ ((__noreturn__));








 
extern char *getenv (__const char *__name) throw () ;

 

extern char *__secure_getenv (__const char *__name) throw () ;


 
 

extern int putenv (char *__string) throw () ;



 

extern int setenv (__const char *__name, __const char *__value, int __replace)
     throw () ;

 
extern int unsetenv (__const char *__name) throw () ;



 


extern int clearenv (void) throw () ;




 



extern char *mktemp (char *__template) throw () ;

 





extern int mkstemp (char *__template) throw () ;













 




extern char *mkdtemp (char *__template) throw () ;



 
extern int system (__const char *__command) throw () ;










 





extern char *realpath (__const char *__restrict __name,
		       char *__restrict __resolved) throw () ;



 


typedef int (*__compar_fn_t) (__const void *, __const void *);






 

extern void *bsearch (__const void *__key, __const void *__base,
		      size_t __nmemb, size_t __size, __compar_fn_t __compar);

 

extern void qsort (void *__base, size_t __nmemb, size_t __size,
		   __compar_fn_t __compar);


 
extern int abs (int __x) throw ()  __attribute__ ((__const__));
extern long int labs (long int __x) throw ()  __attribute__ ((__const__));






 

 
extern div_t div (int __numer, int __denom)
     throw ()  __attribute__ ((__const__));
extern ldiv_t ldiv (long int __numer, long int __denom)
     throw ()  __attribute__ ((__const__));








 


 


extern char *ecvt (double __value, int __ndigit, int *__restrict __decpt,
		   int *__restrict __sign) throw () ;

 


extern char *fcvt (double __value, int __ndigit, int *__restrict __decpt,
		   int *__restrict __sign) throw () ;

 


extern char *gcvt (double __value, int __ndigit, char *__buf) throw () ;



 
extern char *qecvt (long double __value, int __ndigit,
		    int *__restrict __decpt, int *__restrict __sign) throw () ;
extern char *qfcvt (long double __value, int __ndigit,
		    int *__restrict __decpt, int *__restrict __sign) throw () ;
extern char *qgcvt (long double __value, int __ndigit, char *__buf) throw () ;


 

extern int ecvt_r (double __value, int __ndigit, int *__restrict __decpt,
		   int *__restrict __sign, char *__restrict __buf,
		   size_t __len) throw () ;
extern int fcvt_r (double __value, int __ndigit, int *__restrict __decpt,
		   int *__restrict __sign, char *__restrict __buf,
		   size_t __len) throw () ;

extern int qecvt_r (long double __value, int __ndigit,
		    int *__restrict __decpt, int *__restrict __sign,
		    char *__restrict __buf, size_t __len) throw () ;
extern int qfcvt_r (long double __value, int __ndigit,
		    int *__restrict __decpt, int *__restrict __sign,
		    char *__restrict __buf, size_t __len) throw () ;




 

extern int mblen (__const char *__s, size_t __n) throw () ;
 

extern int mbtowc (wchar_t *__restrict __pwc,
		   __const char *__restrict __s, size_t __n) throw () ;
 

extern int wctomb (char *__s, wchar_t __wchar) throw () ;


 
extern size_t mbstowcs (wchar_t *__restrict  __pwcs,
			__const char *__restrict __s, size_t __n) throw () ;
 
extern size_t wcstombs (char *__restrict __s,
			__const wchar_t *__restrict __pwcs, size_t __n)
     throw () ;



 



extern int rpmatch (__const char *__response) throw () ;



# 811 "/usr/include/stdlib.h" 3









 






# 842 "/usr/include/stdlib.h" 3


# 852 "/usr/include/stdlib.h" 3



 


extern int getloadavg (double __loadavg[], int __nelem) throw () ;





} 


# 6 "transverb.cpp" 2

# 1 "/usr/include/stdio.h" 1 3
 


















 









extern "C" { 



# 1 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3





 




 

# 271 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


# 283 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3




 





















# 34 "/usr/include/stdio.h" 2 3










 
typedef struct _IO_FILE FILE;








 
typedef struct _IO_FILE __FILE;









# 1 "/usr/include/libio.h" 1 3
 






























# 1 "/usr/include/_G_config.h" 1 3
 





 






# 1 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3





 




 


# 269 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3











typedef unsigned int  wint_t;




 

 

# 317 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3




 





















# 14 "/usr/include/_G_config.h" 2 3










# 1 "/usr/include/wchar.h" 1 3
 

















 











# 46 "/usr/include/wchar.h" 3


# 1 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 

# 190 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3



 




 

# 271 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3














 

 

# 317 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3




 





















# 48 "/usr/include/wchar.h" 2 3


# 1 "/usr/include/bits/wchar.h" 1 3
 

























# 50 "/usr/include/wchar.h" 2 3


 













 
typedef struct
{
  int __count;
  union
  {
    wint_t __wch;
    char __wchb[4];
  } __value;		 
} __mbstate_t;




 

# 682 "/usr/include/wchar.h" 3



# 24 "/usr/include/_G_config.h" 2 3


typedef struct
{
  __off_t __pos;
  __mbstate_t __state;
} _G_fpos_t;
typedef struct
{
  __off64_t __pos;
  __mbstate_t __state;
} _G_fpos64_t;








# 1 "/usr/include/gconv.h" 1 3
 

















 








# 1 "/usr/include/wchar.h" 1 3
 

















 











# 46 "/usr/include/wchar.h" 3


# 1 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 

# 190 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3



 




 

# 271 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3














 

 

# 317 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3




 





















# 48 "/usr/include/wchar.h" 2 3




 











# 76 "/usr/include/wchar.h" 3




 

# 682 "/usr/include/wchar.h" 3



# 28 "/usr/include/gconv.h" 2 3



# 1 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3





 




 


# 269 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3




# 283 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3




 





















# 31 "/usr/include/gconv.h" 2 3


 


 
enum
{
  __GCONV_OK = 0,
  __GCONV_NOCONV,
  __GCONV_NODB,
  __GCONV_NOMEM,

  __GCONV_EMPTY_INPUT,
  __GCONV_FULL_OUTPUT,
  __GCONV_ILLEGAL_INPUT,
  __GCONV_INCOMPLETE_INPUT,

  __GCONV_ILLEGAL_DESCRIPTOR,
  __GCONV_INTERNAL_ERROR
};


 
enum
{
  __GCONV_IS_LAST = 0x0001,
  __GCONV_IGNORE_ERRORS = 0x0002
};


 
struct __gconv_step;
struct __gconv_step_data;
struct __gconv_loaded_object;
struct __gconv_trans_data;


 
typedef int (*__gconv_fct) (struct __gconv_step *, struct __gconv_step_data *,
			    __const unsigned char **, __const unsigned char *,
			    unsigned char **, size_t *, int, int);

 
typedef int (*__gconv_init_fct) (struct __gconv_step *);
typedef void (*__gconv_end_fct) (struct __gconv_step *);


 
typedef int (*__gconv_trans_fct) (struct __gconv_step *,
				  struct __gconv_step_data *, void *,
				  __const unsigned char *,
				  __const unsigned char **,
				  __const unsigned char *, unsigned char **,
				  size_t *);

 
typedef int (*__gconv_trans_context_fct) (void *, __const unsigned char *,
					  __const unsigned char *,
					  unsigned char *, unsigned char *);

 
typedef int (*__gconv_trans_query_fct) (__const char *, __const char ***,
					size_t *);

 
typedef int (*__gconv_trans_init_fct) (void **, const char *);
typedef void (*__gconv_trans_end_fct) (void *);

struct __gconv_trans_data
{
   
  __gconv_trans_fct __trans_fct;
  __gconv_trans_context_fct __trans_context_fct;
  __gconv_trans_end_fct __trans_end_fct;
  void *__data;
  struct __gconv_trans_data *__next;
};


 
struct __gconv_step
{
  struct __gconv_loaded_object *__shlib_handle;
  __const char *__modname;

  int __counter;

  char *__from_name;
  char *__to_name;

  __gconv_fct __fct;
  __gconv_init_fct __init_fct;
  __gconv_end_fct __end_fct;

   

  int __min_needed_from;
  int __max_needed_from;
  int __min_needed_to;
  int __max_needed_to;

   
  int __stateful;

  void *__data;		 
};

 

struct __gconv_step_data
{
  unsigned char *__outbuf;     
  unsigned char *__outbufend;  


   
  int __flags;

   

  int __invocation_counter;

   

  int __internal_use;

  __mbstate_t *__statep;
  __mbstate_t __state;	 


   
  struct __gconv_trans_data *__trans;
};


 
typedef struct __gconv_info
{
  size_t __nsteps;
  struct __gconv_step *__steps;
  __extension__ struct __gconv_step_data __data [0] ;
} *__gconv_t;


# 44 "/usr/include/_G_config.h" 2 3

typedef union
{
  struct __gconv_info __cd;
  struct
  {
    struct __gconv_info __cd;
    struct __gconv_step_data __data;
  } __combined;
} _G_iconv_t;

typedef int _G_int16_t __attribute__ ((__mode__ (__HI__)));
typedef int _G_int32_t __attribute__ ((__mode__ (__SI__)));
typedef unsigned int _G_uint16_t __attribute__ ((__mode__ (__HI__)));
typedef unsigned int _G_uint32_t __attribute__ ((__mode__ (__SI__)));




 


















 




 














# 32 "/usr/include/libio.h" 2 3

 

















 

# 1 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stdarg.h" 1 3
 
































































 






typedef void *__gnuc_va_list;



 

# 122 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stdarg.h" 3




















# 209 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stdarg.h" 3




# 53 "/usr/include/libio.h" 2 3







# 72 "/usr/include/libio.h" 3


 

















# 103 "/usr/include/libio.h" 3











 

























 



















struct _IO_jump_t;  struct _IO_FILE;

 







typedef void _IO_lock_t;



 

struct _IO_marker {
  struct _IO_marker *_next;
  struct _IO_FILE *_sbuf;
   

   
  int _pos;
# 192 "/usr/include/libio.h" 3

};

 
enum __codecvt_result
{
  __codecvt_ok,
  __codecvt_partial,
  __codecvt_error,
  __codecvt_noconv
};

# 259 "/usr/include/libio.h" 3


struct _IO_FILE {
  int _flags;		 


   
   
  char* _IO_read_ptr;	 
  char* _IO_read_end;	 
  char* _IO_read_base;	 
  char* _IO_write_base;	 
  char* _IO_write_ptr;	 
  char* _IO_write_end;	 
  char* _IO_buf_base;	 
  char* _IO_buf_end;	 
   
  char *_IO_save_base;  
  char *_IO_backup_base;   
  char *_IO_save_end;  

  struct _IO_marker *_markers;

  struct _IO_FILE *_chain;

  int _fileno;
  int _blksize;
  __off_t   _old_offset;  


   
  unsigned short _cur_column;
  signed char _vtable_offset;
  char _shortbuf[1];

   

  _IO_lock_t *_lock;








  __off64_t   _offset;





  void *__pad1;
  void *__pad2;

  int _mode;
   
  char _unused2[15 * sizeof (int) - 2 * sizeof (void *)];

};





struct _IO_FILE_plus;

extern struct _IO_FILE_plus _IO_2_1_stdin_;
extern struct _IO_FILE_plus _IO_2_1_stdout_;
extern struct _IO_FILE_plus _IO_2_1_stderr_;











 

 

typedef __ssize_t __io_read_fn (void *__cookie, char *__buf, size_t __nbytes);

 





typedef __ssize_t __io_write_fn (void *__cookie, __const char *__buf,
				 size_t __n);

 





typedef int __io_seek_fn (void *__cookie, __off64_t   *__pos, int __w);

 
typedef int __io_close_fn (void *__cookie);


# 389 "/usr/include/libio.h" 3




extern "C" {


extern int __underflow (_IO_FILE *) throw () ;
extern int __uflow (_IO_FILE *) throw () ;
extern int __overflow (_IO_FILE *, int) throw () ;
extern wint_t   __wunderflow (_IO_FILE *) throw () ;
extern wint_t   __wuflow (_IO_FILE *) throw () ;
extern wint_t   __woverflow (_IO_FILE *, wint_t  ) throw () ;
























extern int _IO_getc (_IO_FILE *__fp) throw () ;
extern int _IO_putc (int __c, _IO_FILE *__fp) throw () ;
extern int _IO_feof (_IO_FILE *__fp) throw () ;
extern int _IO_ferror (_IO_FILE *__fp) throw () ;

extern int _IO_peekc_locked (_IO_FILE *__fp) throw () ;

 



extern void _IO_flockfile (_IO_FILE *) throw () ;
extern void _IO_funlockfile (_IO_FILE *) throw () ;
extern int _IO_ftrylockfile (_IO_FILE *) throw () ;
















extern int _IO_vfscanf (_IO_FILE * __restrict, const char * __restrict,
			__gnuc_va_list , int *__restrict) throw () ;
extern int _IO_vfprintf (_IO_FILE *__restrict, const char *__restrict,
			 __gnuc_va_list ) throw () ;
extern __ssize_t   _IO_padn (_IO_FILE *, int, __ssize_t  ) throw () ;
extern size_t   _IO_sgetn (_IO_FILE *, void *, size_t  ) throw () ;

extern __off64_t   _IO_seekoff (_IO_FILE *, __off64_t  , int, int) throw () ;
extern __off64_t   _IO_seekpos (_IO_FILE *, __off64_t  , int) throw () ;

extern void _IO_free_backup_area (_IO_FILE *) throw () ;

# 511 "/usr/include/libio.h" 3



}



# 65 "/usr/include/stdio.h" 2 3


# 76 "/usr/include/stdio.h" 3


 

typedef _G_fpos_t fpos_t;







 





 





 






 







 




 








# 1 "/usr/include/bits/stdio_lim.h" 1 3
 








































# 129 "/usr/include/stdio.h" 2 3



 
extern FILE *stdin;		 
extern FILE *stdout;		 
extern FILE *stderr;		 
 




 
extern int remove (__const char *__filename) throw () ;
 
extern int rename (__const char *__old, __const char *__new) throw () ;


 

extern FILE *tmpfile (void) throw () ;










 
extern char *tmpnam (char *__s) throw () ;


 

extern char *tmpnam_r (char *__s) throw () ;




 






extern char *tempnam (__const char *__dir, __const char *__pfx)
     throw ()   ;



 
extern int fclose (FILE *__stream) throw () ;
 
extern int fflush (FILE *__stream) throw () ;


 
extern int fflush_unlocked (FILE *__stream) throw () ;









 
extern FILE *fopen (__const char *__restrict __filename,
		    __const char *__restrict __modes) throw () ;
 
extern FILE *freopen (__const char *__restrict __filename,
		      __const char *__restrict __modes,
		      FILE *__restrict __stream) throw () ;
# 220 "/usr/include/stdio.h" 3










 
extern FILE *fdopen (int __fd, __const char *__modes) throw () ;


# 249 "/usr/include/stdio.h" 3



 

extern void setbuf (FILE *__restrict __stream, char *__restrict __buf) throw () ;
 


extern int setvbuf (FILE *__restrict __stream, char *__restrict __buf,
		    int __modes, size_t __n) throw () ;


 

extern void setbuffer (FILE *__restrict __stream, char *__restrict __buf,
		       size_t __size) throw () ;

 
extern void setlinebuf (FILE *__stream) throw () ;



 
extern int fprintf (FILE *__restrict __stream,
		    __const char *__restrict __format, ...) throw () ;
 
extern int printf (__const char *__restrict __format, ...) throw () ;
 
extern int sprintf (char *__restrict __s,
		    __const char *__restrict __format, ...) throw () ;

 
extern int vfprintf (FILE *__restrict __s, __const char *__restrict __format,
		     __gnuc_va_list  __arg) throw () ;
 
extern int vprintf (__const char *__restrict __format, __gnuc_va_list  __arg)
     throw () ;
 
extern int vsprintf (char *__restrict __s, __const char *__restrict __format,
		     __gnuc_va_list  __arg) throw () ;


 
extern int snprintf (char *__restrict __s, size_t __maxlen,
		     __const char *__restrict __format, ...)
     throw ()  __attribute__ ((__format__ (__printf__, 3, 4)));

extern int vsnprintf (char *__restrict __s, size_t __maxlen,
		      __const char *__restrict __format, __gnuc_va_list  __arg)
     throw ()  __attribute__ ((__format__ (__printf__, 3, 0)));


# 321 "/usr/include/stdio.h" 3



 
extern int fscanf (FILE *__restrict __stream,
		   __const char *__restrict __format, ...) throw () ;
 
extern int scanf (__const char *__restrict __format, ...) throw () ;
 
extern int sscanf (__const char *__restrict __s,
		   __const char *__restrict __format, ...) throw () ;

# 347 "/usr/include/stdio.h" 3



 
extern int fgetc (FILE *__stream) throw () ;
extern int getc (FILE *__stream) throw () ;

 
extern int getchar (void) throw () ;

 




 
extern int getc_unlocked (FILE *__stream) throw () ;
extern int getchar_unlocked (void) throw () ;



 
extern int fgetc_unlocked (FILE *__stream) throw () ;



 
extern int fputc (int __c, FILE *__stream) throw () ;
extern int putc (int __c, FILE *__stream) throw () ;

 
extern int putchar (int __c) throw () ;

 




 
extern int fputc_unlocked (int __c, FILE *__stream) throw () ;



 
extern int putc_unlocked (int __c, FILE *__stream) throw () ;
extern int putchar_unlocked (int __c) throw () ;




 
extern int getw (FILE *__stream) throw () ;

 
extern int putw (int __w, FILE *__stream) throw () ;



 
extern char *fgets (char *__restrict __s, int __n, FILE *__restrict __stream)
     throw () ;







 

extern char *gets (char *__s) throw () ;


# 437 "/usr/include/stdio.h" 3



 
extern int fputs (__const char *__restrict __s, FILE *__restrict __stream)
     throw () ;







 
extern int puts (__const char *__s) throw () ;


 
extern int ungetc (int __c, FILE *__stream) throw () ;


 
extern size_t fread (void *__restrict __ptr, size_t __size,
		     size_t __n, FILE *__restrict __stream) throw () ;
 
extern size_t fwrite (__const void *__restrict __ptr, size_t __size,
		      size_t __n, FILE *__restrict __s) throw () ;


 
extern size_t fread_unlocked (void *__restrict __ptr, size_t __size,
			      size_t __n, FILE *__restrict __stream) throw () ;
extern size_t fwrite_unlocked (__const void *__restrict __ptr, size_t __size,
			       size_t __n, FILE *__restrict __stream) throw () ;



 
extern int fseek (FILE *__stream, long int __off, int __whence) throw () ;
 
extern long int ftell (FILE *__stream) throw () ;
 
extern void rewind (FILE *__stream) throw () ;

 












 
extern int fgetpos (FILE *__restrict __stream, fpos_t *__restrict __pos)
     throw () ;
 
extern int fsetpos (FILE *__stream, __const fpos_t *__pos) throw () ;
# 520 "/usr/include/stdio.h" 3










 
extern void clearerr (FILE *__stream) throw () ;
 
extern int feof (FILE *__stream) throw () ;
 
extern int ferror (FILE *__stream) throw () ;


 
extern void clearerr_unlocked (FILE *__stream) throw () ;
extern int feof_unlocked (FILE *__stream) throw () ;
extern int ferror_unlocked (FILE *__stream) throw () ;



 
extern void perror (__const char *__s) throw () ;

 


extern int sys_nerr;
extern __const char *__const sys_errlist[];








 
extern int fileno (FILE *__stream) throw () ;



 
extern int fileno_unlocked (FILE *__stream) throw () ;





 
extern FILE *popen (__const char *__command, __const char *__modes) throw () ;

 
extern int pclose (FILE *__stream) throw () ;




 
extern char *ctermid (char *__s) throw () ;









# 602 "/usr/include/stdio.h" 3




 

 
extern void flockfile (FILE *__stream) throw () ;

 

extern int ftrylockfile (FILE *__stream) throw () ;

 
extern void funlockfile (FILE *__stream) throw () ;










 


# 1 "/usr/include/bits/stdio.h" 1 3
 






























 
inline  int
vprintf (__const char *__restrict __fmt, __gnuc_va_list  __arg) throw () 
{
  return vfprintf (stdout , __fmt, __arg);
}

 
inline  int
getchar (void) throw () 
{
  return _IO_getc (stdin );
}



 
inline  int
getc_unlocked (FILE *__fp) throw () 
{
  return (( __fp )->_IO_read_ptr >= ( __fp )->_IO_read_end ? __uflow ( __fp ) : *(unsigned char *) ( __fp )->_IO_read_ptr++) ;
}

 
inline  int
getchar_unlocked (void) throw () 
{
  return (( stdin  )->_IO_read_ptr >= ( stdin  )->_IO_read_end ? __uflow ( stdin  ) : *(unsigned char *) ( stdin  )->_IO_read_ptr++) ;
}



 
inline  int
putchar (int __c) throw () 
{
  return _IO_putc (__c, stdout );
}



 
inline  int
fputc_unlocked (int __c, FILE *__stream) throw () 
{
  return (((  __stream )->_IO_write_ptr >= (  __stream )->_IO_write_end) ? __overflow (  __stream , (unsigned char) ( __c )) : (unsigned char) (*(  __stream )->_IO_write_ptr++ = ( __c ))) ;
}




 
inline  int
putc_unlocked (int __c, FILE *__stream) throw () 
{
  return (((  __stream )->_IO_write_ptr >= (  __stream )->_IO_write_end) ? __overflow (  __stream , (unsigned char) ( __c )) : (unsigned char) (*(  __stream )->_IO_write_ptr++ = ( __c ))) ;
}

 
inline  int
putchar_unlocked (int __c) throw () 
{
  return (((  stdout  )->_IO_write_ptr >= (  stdout  )->_IO_write_end) ? __overflow (  stdout  , (unsigned char) ( __c )) : (unsigned char) (*(  stdout  )->_IO_write_ptr++ = ( __c ))) ;
}














 
inline  int
feof_unlocked (FILE *__stream) throw () 
{
  return ((( __stream )->_flags & 0x10 ) != 0) ;
}

 
inline  int
ferror_unlocked (FILE *__stream) throw () 
{
  return ((( __stream )->_flags & 0x20 ) != 0) ;
}






 

# 150 "/usr/include/bits/stdio.h" 3


# 166 "/usr/include/bits/stdio.h" 3


 

# 630 "/usr/include/stdio.h" 2 3



} 




# 7 "transverb.cpp" 2

# 1 "/usr/include/math.h" 1 3
 


















 








extern "C" { 

 

# 1 "/usr/include/bits/huge_val.h" 1 3
 

























 

















 

# 81 "/usr/include/bits/huge_val.h" 3

# 33 "/usr/include/math.h" 2 3


 



 
# 1 "/usr/include/bits/mathdef.h" 1 3
 





















# 41 "/usr/include/bits/mathdef.h" 3

# 40 "/usr/include/math.h" 2 3



 



















# 1 "/usr/include/bits/mathcalls.h" 1 3
 


















 






























 

 
extern   double          acos          (double  __x)    throw ()  ; extern   double         __acos          (double  __x)    throw ()    ;
 
extern   double          asin          (double  __x)    throw ()  ; extern   double         __asin          (double  __x)    throw ()    ;
 
extern   double          atan          (double  __x)    throw ()  ; extern   double         __atan          (double  __x)    throw ()    ;
 
extern   double          atan2          (double  __y, double  __x)    throw ()  ; extern   double         __atan2          (double  __y, double  __x)    throw ()    ;

 
extern   double          cos          (double  __x)    throw ()  ; extern   double         __cos          (double  __x)    throw ()    ;
 
extern   double          sin          (double  __x)    throw ()  ; extern   double         __sin          (double  __x)    throw ()    ;
 
extern   double          tan          (double  __x)    throw ()  ; extern   double         __tan          (double  __x)    throw ()    ;







 

 
extern   double          cosh          (double  __x)    throw ()  ; extern   double         __cosh          (double  __x)    throw ()    ;
 
extern   double          sinh          (double  __x)    throw ()  ; extern   double         __sinh          (double  __x)    throw ()    ;
 
extern   double          tanh          (double  __x)    throw ()  ; extern   double         __tanh          (double  __x)    throw ()    ;


 
extern   double          acosh          (double  __x)    throw ()  ; extern   double         __acosh          (double  __x)    throw ()    ;
 
extern   double          asinh          (double  __x)    throw ()  ; extern   double         __asinh          (double  __x)    throw ()    ;
 
extern   double          atanh          (double  __x)    throw ()  ; extern   double         __atanh          (double  __x)    throw ()    ;


 

 
extern   double          exp          (double  __x)    throw ()  ; extern   double         __exp          (double  __x)    throw ()    ;








 
extern   double          frexp          (double  __x, int *__exponent)    throw ()  ; extern   double         __frexp          (double  __x, int *__exponent)    throw ()    ;

 
extern   double          ldexp          (double  __x, int __exponent)    throw ()  ; extern   double         __ldexp          (double  __x, int __exponent)    throw ()    ;

 
extern   double          log          (double  __x)    throw ()  ; extern   double         __log          (double  __x)    throw ()    ;

 
extern   double          log10          (double  __x)    throw ()  ; extern   double         __log10          (double  __x)    throw ()    ;

 
extern   double          modf          (double  __x, double  *__iptr)    throw ()  ; extern   double         __modf          (double  __x, double  *__iptr)    throw ()    ;


 
extern   double          expm1          (double  __x)    throw ()  ; extern   double         __expm1          (double  __x)    throw ()    ;

 
extern   double          log1p          (double  __x)    throw ()  ; extern   double         __log1p          (double  __x)    throw ()    ;

 
extern   double          logb          (double  __x)    throw ()  ; extern   double         __logb          (double  __x)    throw ()    ;











 

 
extern   double          pow          (double  __x, double  __y)    throw ()  ; extern   double         __pow          (double  __x, double  __y)    throw ()    ;

 
extern   double          sqrt          (double  __x)    throw ()  ; extern   double         __sqrt          (double  __x)    throw ()    ;


 
extern   double          hypot          (double  __x, double  __y)    throw ()  ; extern   double         __hypot          (double  __x, double  __y)    throw ()    ;



 
extern   double          cbrt          (double  __x)    throw ()  ; extern   double         __cbrt          (double  __x)    throw ()    ;



 

 
extern   double          ceil          (double  __x)    throw ()  ; extern   double         __ceil          (double  __x)    throw ()    ;

 
extern   double          fabs          (double  __x)    throw ()   __attribute__ (    (__const__)  ); extern   double         __fabs          (double  __x)    throw ()   __attribute__ (    (__const__)  )  ;

 
extern   double          floor          (double  __x)    throw ()  ; extern   double         __floor          (double  __x)    throw ()    ;

 
extern   double          fmod          (double  __x, double  __y)    throw ()  ; extern   double         __fmod          (double  __x, double  __y)    throw ()    ;


 

extern  int     __isinf      (double  __value)  throw ()   __attribute__ ((__const__));

 
extern  int     __finite      (double  __value)  throw ()   __attribute__ ((__const__));


 

extern  int     isinf      (double  __value)  throw ()   __attribute__ ((__const__));

 
extern  int     finite      (double  __value)  throw ()   __attribute__ ((__const__));

 
extern   double          drem          (double  __x, double  __y)    throw ()  ; extern   double         __drem          (double  __x, double  __y)    throw ()    ;


 
extern   double          significand          (double  __x)    throw ()  ; extern   double         __significand          (double  __x)    throw ()    ;



 
extern   double          copysign          (double  __x, double  __y)    throw ()   __attribute__ (    (__const__)  ); extern   double         __copysign          (double  __x, double  __y)    throw ()   __attribute__ (    (__const__)  )  ;








 
extern  int     __isnan      (double  __value)  throw ()   __attribute__ ((__const__));


 
extern  int     isnan      (double  __value)  throw ()   __attribute__ ((__const__));

 
extern   double          j0          (double )    throw ()  ; extern   double         __j0          (double )    throw ()    ;
extern   double          j1          (double )    throw ()  ; extern   double         __j1          (double )    throw ()    ;
extern   double          jn          (int, double )    throw ()  ; extern   double         __jn          (int, double )    throw ()    ;
extern   double          y0          (double )    throw ()  ; extern   double         __y0          (double )    throw ()    ;
extern   double          y1          (double )    throw ()  ; extern   double         __y1          (double )    throw ()    ;
extern   double          yn          (int, double )    throw ()  ; extern   double         __yn          (int, double )    throw ()    ;




 
extern   double          erf          (double )    throw ()  ; extern   double         __erf          (double )    throw ()    ;
extern   double          erfc          (double )    throw ()  ; extern   double         __erfc          (double )    throw ()    ;
extern   double          lgamma          (double )    throw ()  ; extern   double         __lgamma          (double )    throw ()    ;







 
extern   double          gamma          (double )    throw ()  ; extern   double         __gamma          (double )    throw ()    ;



 


extern   double          lgamma_r              (double , int *__signgamp)    throw ()  ; extern   double         __lgamma_r              (double , int *__signgamp)    throw ()    ;




 

extern   double          rint          (double  __x)    throw ()  ; extern   double         __rint          (double  __x)    throw ()    ;

 
extern   double          nextafter          (double  __x, double  __y)    throw ()   __attribute__ (    (__const__)  ); extern   double         __nextafter          (double  __x, double  __y)    throw ()   __attribute__ (    (__const__)  )  ;




 
extern   double          remainder          (double  __x, double  __y)    throw ()  ; extern   double         __remainder          (double  __x, double  __y)    throw ()    ;


 
extern   double          scalb          (double  __x, double  __n)    throw ()  ; extern   double         __scalb          (double  __x, double  __n)    throw ()    ;



 
extern   double          scalbn          (double  __x, int __n)    throw ()  ; extern   double         __scalbn          (double  __x, int __n)    throw ()    ;


 
extern   int        ilogb        (double  __x)   throw ()  ; extern   int        __ilogb        (double  __x)   throw ()   ;


# 333 "/usr/include/bits/mathcalls.h" 3

# 63 "/usr/include/math.h" 2 3







 











# 1 "/usr/include/bits/mathcalls.h" 1 3
 


















 






























 

 
extern   float          acosf         (float   __x)    throw ()  ; extern   float         __acosf         (float   __x)    throw ()    ;
 
extern   float          asinf         (float   __x)    throw ()  ; extern   float         __asinf         (float   __x)    throw ()    ;
 
extern   float          atanf         (float   __x)    throw ()  ; extern   float         __atanf         (float   __x)    throw ()    ;
 
extern   float          atan2f         (float   __y, float   __x)    throw ()  ; extern   float         __atan2f         (float   __y, float   __x)    throw ()    ;

 
extern   float          cosf         (float   __x)    throw ()  ; extern   float         __cosf         (float   __x)    throw ()    ;
 
extern   float          sinf         (float   __x)    throw ()  ; extern   float         __sinf         (float   __x)    throw ()    ;
 
extern   float          tanf         (float   __x)    throw ()  ; extern   float         __tanf         (float   __x)    throw ()    ;







 

 
extern   float          coshf         (float   __x)    throw ()  ; extern   float         __coshf         (float   __x)    throw ()    ;
 
extern   float          sinhf         (float   __x)    throw ()  ; extern   float         __sinhf         (float   __x)    throw ()    ;
 
extern   float          tanhf         (float   __x)    throw ()  ; extern   float         __tanhf         (float   __x)    throw ()    ;


 
extern   float          acoshf         (float   __x)    throw ()  ; extern   float         __acoshf         (float   __x)    throw ()    ;
 
extern   float          asinhf         (float   __x)    throw ()  ; extern   float         __asinhf         (float   __x)    throw ()    ;
 
extern   float          atanhf         (float   __x)    throw ()  ; extern   float         __atanhf         (float   __x)    throw ()    ;


 

 
extern   float          expf         (float   __x)    throw ()  ; extern   float         __expf         (float   __x)    throw ()    ;








 
extern   float          frexpf         (float   __x, int *__exponent)    throw ()  ; extern   float         __frexpf         (float   __x, int *__exponent)    throw ()    ;

 
extern   float          ldexpf         (float   __x, int __exponent)    throw ()  ; extern   float         __ldexpf         (float   __x, int __exponent)    throw ()    ;

 
extern   float          logf         (float   __x)    throw ()  ; extern   float         __logf         (float   __x)    throw ()    ;

 
extern   float          log10f         (float   __x)    throw ()  ; extern   float         __log10f         (float   __x)    throw ()    ;

 
extern   float          modff         (float   __x, float   *__iptr)    throw ()  ; extern   float         __modff         (float   __x, float   *__iptr)    throw ()    ;


 
extern   float          expm1f         (float   __x)    throw ()  ; extern   float         __expm1f         (float   __x)    throw ()    ;

 
extern   float          log1pf         (float   __x)    throw ()  ; extern   float         __log1pf         (float   __x)    throw ()    ;

 
extern   float          logbf         (float   __x)    throw ()  ; extern   float         __logbf         (float   __x)    throw ()    ;











 

 
extern   float          powf         (float   __x, float   __y)    throw ()  ; extern   float         __powf         (float   __x, float   __y)    throw ()    ;

 
extern   float          sqrtf         (float   __x)    throw ()  ; extern   float         __sqrtf         (float   __x)    throw ()    ;


 
extern   float          hypotf         (float   __x, float   __y)    throw ()  ; extern   float         __hypotf         (float   __x, float   __y)    throw ()    ;



 
extern   float          cbrtf         (float   __x)    throw ()  ; extern   float         __cbrtf         (float   __x)    throw ()    ;



 

 
extern   float          ceilf         (float   __x)    throw ()  ; extern   float         __ceilf         (float   __x)    throw ()    ;

 
extern   float          fabsf         (float   __x)    throw ()   __attribute__ (    (__const__)  ); extern   float         __fabsf         (float   __x)    throw ()   __attribute__ (    (__const__)  )  ;

 
extern   float          floorf         (float   __x)    throw ()  ; extern   float         __floorf         (float   __x)    throw ()    ;

 
extern   float          fmodf         (float   __x, float   __y)    throw ()  ; extern   float         __fmodf         (float   __x, float   __y)    throw ()    ;


 

extern  int    __isinff     (float   __value)  throw ()   __attribute__ ((__const__));

 
extern  int    __finitef     (float   __value)  throw ()   __attribute__ ((__const__));


 

extern  int    isinff     (float   __value)  throw ()   __attribute__ ((__const__));

 
extern  int    finitef     (float   __value)  throw ()   __attribute__ ((__const__));

 
extern   float          dremf         (float   __x, float   __y)    throw ()  ; extern   float         __dremf         (float   __x, float   __y)    throw ()    ;


 
extern   float          significandf         (float   __x)    throw ()  ; extern   float         __significandf         (float   __x)    throw ()    ;



 
extern   float          copysignf         (float   __x, float   __y)    throw ()   __attribute__ (    (__const__)  ); extern   float         __copysignf         (float   __x, float   __y)    throw ()   __attribute__ (    (__const__)  )  ;








 
extern  int    __isnanf     (float   __value)  throw ()   __attribute__ ((__const__));


 
extern  int    isnanf     (float   __value)  throw ()   __attribute__ ((__const__));

 
extern   float          j0f         (float  )    throw ()  ; extern   float         __j0f         (float  )    throw ()    ;
extern   float          j1f         (float  )    throw ()  ; extern   float         __j1f         (float  )    throw ()    ;
extern   float          jnf         (int, float  )    throw ()  ; extern   float         __jnf         (int, float  )    throw ()    ;
extern   float          y0f         (float  )    throw ()  ; extern   float         __y0f         (float  )    throw ()    ;
extern   float          y1f         (float  )    throw ()  ; extern   float         __y1f         (float  )    throw ()    ;
extern   float          ynf         (int, float  )    throw ()  ; extern   float         __ynf         (int, float  )    throw ()    ;




 
extern   float          erff         (float  )    throw ()  ; extern   float         __erff         (float  )    throw ()    ;
extern   float          erfcf         (float  )    throw ()  ; extern   float         __erfcf         (float  )    throw ()    ;
extern   float          lgammaf         (float  )    throw ()  ; extern   float         __lgammaf         (float  )    throw ()    ;







 
extern   float          gammaf         (float  )    throw ()  ; extern   float         __gammaf         (float  )    throw ()    ;



 


extern   float          lgammaf_r            (float  , int *__signgamp)    throw ()  ; extern   float         __lgammaf_r            (float  , int *__signgamp)    throw ()    ;




 

extern   float          rintf         (float   __x)    throw ()  ; extern   float         __rintf         (float   __x)    throw ()    ;

 
extern   float          nextafterf         (float   __x, float   __y)    throw ()   __attribute__ (    (__const__)  ); extern   float         __nextafterf         (float   __x, float   __y)    throw ()   __attribute__ (    (__const__)  )  ;




 
extern   float          remainderf         (float   __x, float   __y)    throw ()  ; extern   float         __remainderf         (float   __x, float   __y)    throw ()    ;


 
extern   float          scalbf         (float   __x, float   __n)    throw ()  ; extern   float         __scalbf         (float   __x, float   __n)    throw ()    ;



 
extern   float          scalbnf         (float   __x, int __n)    throw ()  ; extern   float         __scalbnf         (float   __x, int __n)    throw ()    ;


 
extern   int       ilogbf       (float   __x)   throw ()  ; extern   int       __ilogbf       (float   __x)   throw ()   ;


# 333 "/usr/include/bits/mathcalls.h" 3

# 82 "/usr/include/math.h" 2 3





 











# 1 "/usr/include/bits/mathcalls.h" 1 3
 


















 






























 

 
extern   long double          acosl         (long double   __x)    throw ()  ; extern   long double         __acosl         (long double   __x)    throw ()    ;
 
extern   long double          asinl         (long double   __x)    throw ()  ; extern   long double         __asinl         (long double   __x)    throw ()    ;
 
extern   long double          atanl         (long double   __x)    throw ()  ; extern   long double         __atanl         (long double   __x)    throw ()    ;
 
extern   long double          atan2l         (long double   __y, long double   __x)    throw ()  ; extern   long double         __atan2l         (long double   __y, long double   __x)    throw ()    ;

 
extern   long double          cosl         (long double   __x)    throw ()  ; extern   long double         __cosl         (long double   __x)    throw ()    ;
 
extern   long double          sinl         (long double   __x)    throw ()  ; extern   long double         __sinl         (long double   __x)    throw ()    ;
 
extern   long double          tanl         (long double   __x)    throw ()  ; extern   long double         __tanl         (long double   __x)    throw ()    ;







 

 
extern   long double          coshl         (long double   __x)    throw ()  ; extern   long double         __coshl         (long double   __x)    throw ()    ;
 
extern   long double          sinhl         (long double   __x)    throw ()  ; extern   long double         __sinhl         (long double   __x)    throw ()    ;
 
extern   long double          tanhl         (long double   __x)    throw ()  ; extern   long double         __tanhl         (long double   __x)    throw ()    ;


 
extern   long double          acoshl         (long double   __x)    throw ()  ; extern   long double         __acoshl         (long double   __x)    throw ()    ;
 
extern   long double          asinhl         (long double   __x)    throw ()  ; extern   long double         __asinhl         (long double   __x)    throw ()    ;
 
extern   long double          atanhl         (long double   __x)    throw ()  ; extern   long double         __atanhl         (long double   __x)    throw ()    ;


 

 
extern   long double          expl         (long double   __x)    throw ()  ; extern   long double         __expl         (long double   __x)    throw ()    ;








 
extern   long double          frexpl         (long double   __x, int *__exponent)    throw ()  ; extern   long double         __frexpl         (long double   __x, int *__exponent)    throw ()    ;

 
extern   long double          ldexpl         (long double   __x, int __exponent)    throw ()  ; extern   long double         __ldexpl         (long double   __x, int __exponent)    throw ()    ;

 
extern   long double          logl         (long double   __x)    throw ()  ; extern   long double         __logl         (long double   __x)    throw ()    ;

 
extern   long double          log10l         (long double   __x)    throw ()  ; extern   long double         __log10l         (long double   __x)    throw ()    ;

 
extern   long double          modfl         (long double   __x, long double   *__iptr)    throw ()  ; extern   long double         __modfl         (long double   __x, long double   *__iptr)    throw ()    ;


 
extern   long double          expm1l         (long double   __x)    throw ()  ; extern   long double         __expm1l         (long double   __x)    throw ()    ;

 
extern   long double          log1pl         (long double   __x)    throw ()  ; extern   long double         __log1pl         (long double   __x)    throw ()    ;

 
extern   long double          logbl         (long double   __x)    throw ()  ; extern   long double         __logbl         (long double   __x)    throw ()    ;











 

 
extern   long double          powl         (long double   __x, long double   __y)    throw ()  ; extern   long double         __powl         (long double   __x, long double   __y)    throw ()    ;

 
extern   long double          sqrtl         (long double   __x)    throw ()  ; extern   long double         __sqrtl         (long double   __x)    throw ()    ;


 
extern   long double          hypotl         (long double   __x, long double   __y)    throw ()  ; extern   long double         __hypotl         (long double   __x, long double   __y)    throw ()    ;



 
extern   long double          cbrtl         (long double   __x)    throw ()  ; extern   long double         __cbrtl         (long double   __x)    throw ()    ;



 

 
extern   long double          ceill         (long double   __x)    throw ()  ; extern   long double         __ceill         (long double   __x)    throw ()    ;

 
extern   long double          fabsl         (long double   __x)    throw ()   __attribute__ (    (__const__)  ); extern   long double         __fabsl         (long double   __x)    throw ()   __attribute__ (    (__const__)  )  ;

 
extern   long double          floorl         (long double   __x)    throw ()  ; extern   long double         __floorl         (long double   __x)    throw ()    ;

 
extern   long double          fmodl         (long double   __x, long double   __y)    throw ()  ; extern   long double         __fmodl         (long double   __x, long double   __y)    throw ()    ;


 

extern  int    __isinfl     (long double   __value)  throw ()   __attribute__ ((__const__));

 
extern  int    __finitel     (long double   __value)  throw ()   __attribute__ ((__const__));


 

extern  int    isinfl     (long double   __value)  throw ()   __attribute__ ((__const__));

 
extern  int    finitel     (long double   __value)  throw ()   __attribute__ ((__const__));

 
extern   long double          dreml         (long double   __x, long double   __y)    throw ()  ; extern   long double         __dreml         (long double   __x, long double   __y)    throw ()    ;


 
extern   long double          significandl         (long double   __x)    throw ()  ; extern   long double         __significandl         (long double   __x)    throw ()    ;



 
extern   long double          copysignl         (long double   __x, long double   __y)    throw ()   __attribute__ (    (__const__)  ); extern   long double         __copysignl         (long double   __x, long double   __y)    throw ()   __attribute__ (    (__const__)  )  ;








 
extern  int    __isnanl     (long double   __value)  throw ()   __attribute__ ((__const__));


 
extern  int    isnanl     (long double   __value)  throw ()   __attribute__ ((__const__));

 
extern   long double          j0l         (long double  )    throw ()  ; extern   long double         __j0l         (long double  )    throw ()    ;
extern   long double          j1l         (long double  )    throw ()  ; extern   long double         __j1l         (long double  )    throw ()    ;
extern   long double          jnl         (int, long double  )    throw ()  ; extern   long double         __jnl         (int, long double  )    throw ()    ;
extern   long double          y0l         (long double  )    throw ()  ; extern   long double         __y0l         (long double  )    throw ()    ;
extern   long double          y1l         (long double  )    throw ()  ; extern   long double         __y1l         (long double  )    throw ()    ;
extern   long double          ynl         (int, long double  )    throw ()  ; extern   long double         __ynl         (int, long double  )    throw ()    ;




 
extern   long double          erfl         (long double  )    throw ()  ; extern   long double         __erfl         (long double  )    throw ()    ;
extern   long double          erfcl         (long double  )    throw ()  ; extern   long double         __erfcl         (long double  )    throw ()    ;
extern   long double          lgammal         (long double  )    throw ()  ; extern   long double         __lgammal         (long double  )    throw ()    ;







 
extern   long double          gammal         (long double  )    throw ()  ; extern   long double         __gammal         (long double  )    throw ()    ;



 


extern   long double          lgammal_r            (long double  , int *__signgamp)    throw ()  ; extern   long double         __lgammal_r            (long double  , int *__signgamp)    throw ()    ;




 

extern   long double          rintl         (long double   __x)    throw ()  ; extern   long double         __rintl         (long double   __x)    throw ()    ;

 
extern   long double          nextafterl         (long double   __x, long double   __y)    throw ()   __attribute__ (    (__const__)  ); extern   long double         __nextafterl         (long double   __x, long double   __y)    throw ()   __attribute__ (    (__const__)  )  ;




 
extern   long double          remainderl         (long double   __x, long double   __y)    throw ()  ; extern   long double         __remainderl         (long double   __x, long double   __y)    throw ()    ;


 
extern   long double          scalbl         (long double   __x, long double   __n)    throw ()  ; extern   long double         __scalbl         (long double   __x, long double   __n)    throw ()    ;



 
extern   long double          scalbnl         (long double   __x, int __n)    throw ()  ; extern   long double         __scalbnl         (long double   __x, int __n)    throw ()    ;


 
extern   int       ilogbl       (long double   __x)   throw ()  ; extern   int       __ilogbl       (long double   __x)   throw ()   ;


# 333 "/usr/include/bits/mathcalls.h" 3

# 99 "/usr/include/math.h" 2 3













 
extern int signgam;



 
# 236 "/usr/include/math.h" 3



 
typedef enum
{
  _IEEE_ = -1,	 
  _SVID_,	 
  _XOPEN_,	 
  _POSIX_,
  _ISOC_	 
} _LIB_VERSION_TYPE;

 


extern _LIB_VERSION_TYPE _LIB_VERSION;




 





struct __exception



  {
    int type;
    char *name;
    double arg1;
    double arg2;
    double retval;
  };


extern int matherr (struct __exception *__exc) throw ();






 







 












 
















 


# 338 "/usr/include/math.h" 3



 






 

# 1 "/usr/include/bits/mathinline.h" 1 3
 






























# 146 "/usr/include/bits/mathinline.h" 3



 






 


















































































# 252 "/usr/include/bits/mathinline.h" 3





































 

__inline   double     __sgn   ( double ) throw () ;	__inline    double        __sgn    (  double         __x   ) throw () 	{	        return __x == 0.0 ? 0.0 : (__x > 0.0 ? 1.0 : -1.0)   ;	}  	__inline   float      __sgnf    ( float ) throw () ;	__inline    float         __sgnf     (  float         __x   ) throw () 	{	        return __x == 0.0 ? 0.0 : (__x > 0.0 ? 1.0 : -1.0)   ;	}  	__inline   long double      __sgnl    ( long double ) throw () ;	__inline    long double         __sgnl     (  long double         __x   ) throw () 	{	        return __x == 0.0 ? 0.0 : (__x > 0.0 ? 1.0 : -1.0)   ;	}   


 
# 416 "/usr/include/bits/mathinline.h" 3









__inline   double     atan2   ( double      __y  ,  double      __x  ) throw ()  {	    register long double __value;	__asm __volatile__	("fpatan"	: "=t" (__value) : "0" (__x), "u" (__y) : "st(1)");	return __value   ;	} 	__inline   float      atan2f    ( float      __y  ,  float      __x  ) throw ()  {	    register long double __value;	__asm __volatile__	("fpatan"	: "=t" (__value) : "0" (__x), "u" (__y) : "st(1)");	return __value   ;	} 	__inline   long double      atan2l    ( long double      __y  ,  long double      __x  ) throw ()  {	    register long double __value;	__asm __volatile__	("fpatan"	: "=t" (__value) : "0" (__x), "u" (__y) : "st(1)");	return __value   ;	}  
__inline   long double    __atan2l  ( long double    __y ,  long double    __x ) throw ()  {	  register long double __value;	__asm __volatile__	("fpatan"	: "=t" (__value) : "0" (__x), "u" (__y) : "st(1)");	return __value  ;	} 


__inline   double     fmod   ( double      __x  ,  double      __y  ) throw ()  {	      register long double __value;						        __asm __volatile__							          ("1:	fprem\n\t"						           "fnstsw	%%ax\n\t"						           "sahf\n\t"								           "jp	1b"							           : "=t" (__value) : "0" (__x), "u" (__y) : "ax", "cc");		        return __value  ;	} 	__inline   float      fmodf    ( float      __x  ,  float      __y  ) throw ()  {	      register long double __value;						        __asm __volatile__							          ("1:	fprem\n\t"						           "fnstsw	%%ax\n\t"						           "sahf\n\t"								           "jp	1b"							           : "=t" (__value) : "0" (__x), "u" (__y) : "ax", "cc");		        return __value  ;	} 	__inline   long double      fmodl    ( long double      __x  ,  long double      __y  ) throw ()  {	      register long double __value;						        __asm __volatile__							          ("1:	fprem\n\t"						           "fnstsw	%%ax\n\t"						           "sahf\n\t"								           "jp	1b"							           : "=t" (__value) : "0" (__x), "u" (__y) : "ax", "cc");		        return __value  ;	}  
# 438 "/usr/include/bits/mathinline.h" 3


__inline    double        sqrt    (  double   __x) throw () 	{	register   double   __result;	__asm __volatile__ (      "fsqrt"    : "=t" (__result) :   "0" (__x) );	return __result;	}  	__inline    float         sqrtf     (  float   __x) throw () 	{	register   float   __result;	__asm __volatile__ (      "fsqrt"    : "=t" (__result) :   "0" (__x) );	return __result;	}  	__inline    long double         sqrtl     (  long double   __x) throw () 	{	register   long double   __result;	__asm __volatile__ (      "fsqrt"    : "=t" (__result) :   "0" (__x) );	return __result;	}   
__inline    long double       __sqrtl   (  long double   __x) throw () 	{	register   long double   __result;	__asm __volatile__ (    "fsqrt"   : "=t" (__result) :   "0" (__x) );	return __result;	}  


__inline   double    fabs  ( double    __x ) throw () 	{	  return __builtin_fabs (__x) ;	} 
__inline   float    fabsf  ( float    __x ) throw () 	{	  return __builtin_fabsf (__x) ;	} 
__inline   long double    fabsl  ( long double    __x ) throw () 	{	  return __builtin_fabsl (__x) ;	} 
__inline   long double    __fabsl  ( long double    __x ) throw () 	{	  return __builtin_fabsl (__x) ;	} 





# 464 "/usr/include/bits/mathinline.h" 3


__inline   double     atan   ( double  __x) throw () 	{	register  double  __result;	__asm __volatile__ (    "fld1; fpatan"   : "=t" (__result) :     "0" (__x) : "st(1)"  );	return __result;	} 	__inline   float      atanf    ( float  __x) throw () 	{	register  float  __result;	__asm __volatile__ (    "fld1; fpatan"   : "=t" (__result) :     "0" (__x) : "st(1)"  );	return __result;	} 	__inline   long double      atanl    ( long double  __x) throw () 	{	register  long double  __result;	__asm __volatile__ (    "fld1; fpatan"   : "=t" (__result) :     "0" (__x) : "st(1)"  );	return __result;	}  

__inline   long double    __sgn1l  ( long double ) throw () ;	__inline    long double       __sgn1l   (  long double       __x  ) throw () 	{	      __extension__ union { long double __xld; unsigned int __xi[3]; } __n =          { __xld: __x };							        __n.__xi[2] = (__n.__xi[2] & 0x8000) | 0x3fff;			        __n.__xi[1] = 0x80000000;						        __n.__xi[0] = 0;							        return __n.__xld  ;	}  








# 490 "/usr/include/bits/mathinline.h" 3


__inline   double     floor   ( double      __x  ) throw () 	{	      register long double __value;						        __volatile unsigned short int __cw;					        __volatile unsigned short int __cwtmp;				        __asm __volatile ("fnstcw %0" : "=m" (__cw));				        __cwtmp = (__cw & 0xf3ff) | 0x0400;  		        __asm __volatile ("fldcw %0" : : "m" (__cwtmp));			        __asm __volatile ("frndint" : "=t" (__value) : "0" (__x));		        __asm __volatile ("fldcw %0" : : "m" (__cw));				        return __value  ;	} 	__inline   float      floorf    ( float      __x  ) throw () 	{	      register long double __value;						        __volatile unsigned short int __cw;					        __volatile unsigned short int __cwtmp;				        __asm __volatile ("fnstcw %0" : "=m" (__cw));				        __cwtmp = (__cw & 0xf3ff) | 0x0400;  		        __asm __volatile ("fldcw %0" : : "m" (__cwtmp));			        __asm __volatile ("frndint" : "=t" (__value) : "0" (__x));		        __asm __volatile ("fldcw %0" : : "m" (__cw));				        return __value  ;	} 	__inline   long double      floorl    ( long double      __x  ) throw () 	{	      register long double __value;						        __volatile unsigned short int __cw;					        __volatile unsigned short int __cwtmp;				        __asm __volatile ("fnstcw %0" : "=m" (__cw));				        __cwtmp = (__cw & 0xf3ff) | 0x0400;  		        __asm __volatile ("fldcw %0" : : "m" (__cwtmp));			        __asm __volatile ("frndint" : "=t" (__value) : "0" (__x));		        __asm __volatile ("fldcw %0" : : "m" (__cw));				        return __value  ;	}  
# 502 "/usr/include/bits/mathinline.h" 3

__inline   double     ceil   ( double      __x  ) throw () 	{	      register long double __value;						        __volatile unsigned short int __cw;					        __volatile unsigned short int __cwtmp;				        __asm __volatile ("fnstcw %0" : "=m" (__cw));				        __cwtmp = (__cw & 0xf3ff) | 0x0800;  			        __asm __volatile ("fldcw %0" : : "m" (__cwtmp));			        __asm __volatile ("frndint" : "=t" (__value) : "0" (__x));		        __asm __volatile ("fldcw %0" : : "m" (__cw));				        return __value  ;	} 	__inline   float      ceilf    ( float      __x  ) throw () 	{	      register long double __value;						        __volatile unsigned short int __cw;					        __volatile unsigned short int __cwtmp;				        __asm __volatile ("fnstcw %0" : "=m" (__cw));				        __cwtmp = (__cw & 0xf3ff) | 0x0800;  			        __asm __volatile ("fldcw %0" : : "m" (__cwtmp));			        __asm __volatile ("frndint" : "=t" (__value) : "0" (__x));		        __asm __volatile ("fldcw %0" : : "m" (__cw));				        return __value  ;	} 	__inline   long double      ceill    ( long double      __x  ) throw () 	{	      register long double __value;						        __volatile unsigned short int __cw;					        __volatile unsigned short int __cwtmp;				        __asm __volatile ("fnstcw %0" : "=m" (__cw));				        __cwtmp = (__cw & 0xf3ff) | 0x0800;  			        __asm __volatile ("fldcw %0" : : "m" (__cwtmp));			        __asm __volatile ("frndint" : "=t" (__value) : "0" (__x));		        __asm __volatile ("fldcw %0" : : "m" (__cw));				        return __value  ;	}  
# 513 "/usr/include/bits/mathinline.h" 3








__inline  double
ldexp (double __x, int __y) throw () 
{
  register long double __value;	__asm __volatile__	("fscale"	: "=t" (__value) : "0" (__x), "u" ((long double) __y));	return __value ;
}


 






 



__inline   double     log1p   ( double      __x  ) throw () 	{	      register long double __value;						        if (__fabsl (__x) >= 1.0 - 0.5 * 1.41421356237309504880L )				          __value = logl (1.0 + __x);						        else									          __asm __volatile__							            ("fldln2\n\t"							             "fxch\n\t"							             "fyl2xp1"							             : "=t" (__value) : "0" (__x) : "st(1)");				        return __value  ;	} 	__inline   float      log1pf    ( float      __x  ) throw () 	{	      register long double __value;						        if (__fabsl (__x) >= 1.0 - 0.5 * 1.41421356237309504880L )				          __value = logl (1.0 + __x);						        else									          __asm __volatile__							            ("fldln2\n\t"							             "fxch\n\t"							             "fyl2xp1"							             : "=t" (__value) : "0" (__x) : "st(1)");				        return __value  ;	} 	__inline   long double      log1pl    ( long double      __x  ) throw () 	{	      register long double __value;						        if (__fabsl (__x) >= 1.0 - 0.5 * 1.41421356237309504880L )				          __value = logl (1.0 + __x);						        else									          __asm __volatile__							            ("fldln2\n\t"							             "fxch\n\t"							             "fyl2xp1"							             : "=t" (__value) : "0" (__x) : "st(1)");				        return __value  ;	}  
# 550 "/usr/include/bits/mathinline.h" 3


 
__inline   double     asinh   ( double      __x  ) throw () 	{	      register long double  __y = __fabsl (__x);				        return (log1pl (__y * __y / (__sqrtl (__y * __y + 1.0) + 1.0) + __y)	      	  * __sgn1l (__x))  ;	} 	__inline   float      asinhf    ( float      __x  ) throw () 	{	      register long double  __y = __fabsl (__x);				        return (log1pl (__y * __y / (__sqrtl (__y * __y + 1.0) + 1.0) + __y)	      	  * __sgn1l (__x))  ;	} 	__inline   long double      asinhl    ( long double      __x  ) throw () 	{	      register long double  __y = __fabsl (__x);				        return (log1pl (__y * __y / (__sqrtl (__y * __y + 1.0) + 1.0) + __y)	      	  * __sgn1l (__x))  ;	}  




__inline   double     acosh   ( double      __x  ) throw () 	{	      return logl (__x + __sqrtl (__x - 1.0) * __sqrtl (__x + 1.0))  ;	} 	__inline   float      acoshf    ( float      __x  ) throw () 	{	      return logl (__x + __sqrtl (__x - 1.0) * __sqrtl (__x + 1.0))  ;	} 	__inline   long double      acoshl    ( long double      __x  ) throw () 	{	      return logl (__x + __sqrtl (__x - 1.0) * __sqrtl (__x + 1.0))  ;	}  


__inline   double     atanh   ( double      __x  ) throw () 	{	      register long double __y = __fabsl (__x);				        return -0.5 * log1pl (-(__y + __y) / (1.0 + __y)) * __sgn1l (__x)  ;	} 	__inline   float      atanhf    ( float      __x  ) throw () 	{	      register long double __y = __fabsl (__x);				        return -0.5 * log1pl (-(__y + __y) / (1.0 + __y)) * __sgn1l (__x)  ;	} 	__inline   long double      atanhl    ( long double      __x  ) throw () 	{	      register long double __y = __fabsl (__x);				        return -0.5 * log1pl (-(__y + __y) / (1.0 + __y)) * __sgn1l (__x)  ;	}  



 
__inline   double     hypot   ( double      __x  ,  double      __y  ) throw ()  {	    return __sqrtl (__x * __x + __y * __y)  ;	} 	__inline   float      hypotf    ( float      __x  ,  float      __y  ) throw ()  {	    return __sqrtl (__x * __x + __y * __y)  ;	} 	__inline   long double      hypotl    ( long double      __x  ,  long double      __y  ) throw ()  {	    return __sqrtl (__x * __x + __y * __y)  ;	}  

__inline   double     logb   ( double      __x  ) throw () 	{	      register long double __value;						        register long double __junk;						        __asm __volatile__							          ("fxtract\n\t"							           : "=t" (__junk), "=u" (__value) : "0" (__x));			        return __value  ;	} 	__inline   float      logbf    ( float      __x  ) throw () 	{	      register long double __value;						        register long double __junk;						        __asm __volatile__							          ("fxtract\n\t"							           : "=t" (__junk), "=u" (__value) : "0" (__x));			        return __value  ;	} 	__inline   long double      logbl    ( long double      __x  ) throw () 	{	      register long double __value;						        register long double __junk;						        __asm __volatile__							          ("fxtract\n\t"							           : "=t" (__junk), "=u" (__value) : "0" (__x));			        return __value  ;	}  









# 647 "/usr/include/bits/mathinline.h" 3





__inline   double     drem   ( double      __x  ,  double      __y  ) throw ()  {	      register double __value;						        register int __clobbered;						        __asm __volatile__							          ("1:	fprem1\n\t"						           "fstsw	%%ax\n\t"						           "sahf\n\t"								           "jp	1b"							           : "=t" (__value), "=&a" (__clobbered) : "0" (__x), "u" (__y) : "cc");      return __value  ;	} 	__inline   float      dremf    ( float      __x  ,  float      __y  ) throw ()  {	      register double __value;						        register int __clobbered;						        __asm __volatile__							          ("1:	fprem1\n\t"						           "fstsw	%%ax\n\t"						           "sahf\n\t"								           "jp	1b"							           : "=t" (__value), "=&a" (__clobbered) : "0" (__x), "u" (__y) : "cc");      return __value  ;	} 	__inline   long double      dreml    ( long double      __x  ,  long double      __y  ) throw ()  {	      register double __value;						        register int __clobbered;						        __asm __volatile__							          ("1:	fprem1\n\t"						           "fstsw	%%ax\n\t"						           "sahf\n\t"								           "jp	1b"							           : "=t" (__value), "=&a" (__clobbered) : "0" (__x), "u" (__y) : "cc");      return __value  ;	}  
# 662 "/usr/include/bits/mathinline.h" 3


 
__inline  int
__finite (double __x) throw () 
{
  return (__extension__
	  (((((union { double __d; int __i[2]; }) {__d: __x}).__i[1]
	     | 0x800fffffu) + 1) >> 31));
}

 
# 682 "/usr/include/bits/mathinline.h" 3



 










 
# 705 "/usr/include/bits/mathinline.h" 3



# 350 "/usr/include/math.h" 2 3




# 409 "/usr/include/math.h" 3


} 



# 8 "transverb.cpp" 2


# 1 "transverb.hpp" 1






 



# 1 "/usr/local/lib/pd/externs/flext/flext.h" 1
 









 








 

 


 
# 1 "/usr/local/lib/pd/externs/flext/fldefs.h" 1
 









 







 


 



 
 

 



 



 



 



 



 
 

 


 
 

 




 




 



 




 
 

 




 




 



 




 
 

 




 




 



 




 
 

 




 




 



 




 
 

 




 




 



 




 

 


 


 



 

 



 



 



 



 



 



 



 



 



 

 


 

 


 

 

 


 



 



 



 

 



 



 



 



 



 



 



 



 



 



 

 

 

 

 

 

 

 


 



# 26 "/usr/local/lib/pd/externs/flext/flext.h" 2


 
# 1 "/usr/local/lib/pd/externs/flext/flclass.h" 1
 









 








 
# 1 "/usr/local/lib/pd/externs/flext/flbase.h" 1
 









 

 








# 1 "/usr/local/lib/pd/externs/flext/flstdc.h" 1
 









 









 




 





 



 







extern "C" {	    	    	    	    	    	    	


 



# 1 "/usr/local/include/m_pd.h" 1 3
 




extern "C" {









     










     















     



typedef int t_int;


typedef float t_float;	 
typedef float t_floatarg;   

typedef struct _symbol
{
    char *s_name;
    struct _class **s_thing;
    struct _symbol *s_next;
} t_symbol;

struct  _array;


 








typedef struct _gstub
{
    union
    {
    	struct _glist *gs_glist;     
    	struct _array *gs_array;     
    } gs_un;
    int gs_which;   	    	     
    int gs_refcount;   	    	     
} t_gstub;

typedef struct _gpointer 	    
{
    union
    {	
    	struct _scalar *gp_scalar;   
    	union word *gp_w;  	     
    } gp_un;
    int gp_valid;   	    	     
    t_gstub *gp_stub;	    	     
} t_gpointer;

typedef union word
{
    t_float w_float;
    t_symbol *w_symbol;
    t_gpointer *w_gpointer;
    struct _array  *w_array;
    struct _glist *w_list;
    int w_index;
} t_word;

typedef enum
{
    A_NULL,
    A_FLOAT,
    A_SYMBOL,
    A_POINTER,
    A_SEMI,
    A_COMMA,
    A_DEFFLOAT,
    A_DEFSYM,
    A_DOLLAR, 
    A_DOLLSYM,
    A_GIMME,
    A_CANT
}  t_atomtype;



typedef struct _atom
{
    t_atomtype a_type;
    union word a_w;
} t_atom;

struct  _class;


struct  _outlet;


struct  _inlet;


struct  _binbuf;


struct  _clock;


struct  _outconnect;


struct  _glist;



typedef struct _class  *t_pd;	     

typedef struct _gobj	     
{
    t_pd g_pd;	    	     
    struct _gobj *g_next;    
} t_gobj;

typedef struct _scalar	     
{
    t_gobj sc_gobj;	     
    t_symbol *sc_template;   
    t_word sc_vec[1];	     
} t_scalar;

typedef struct _text	     
{
    t_gobj te_g;	    	 
    struct _binbuf  *te_binbuf;    	 
    struct _outlet  *te_outlet;    	 
    struct _inlet  *te_inlet;	    	 
    short te_xpix;	    	 
    short te_ypix;
    short te_width;	    	 
    unsigned int te_type:2; 	 
} t_text;








    

typedef struct _text t_object;







typedef void (*t_method)(void);
typedef void *(*t_newmethod)( void);
typedef void (*t_gotfn)(void *x, ...);

 
extern  t_pd pd_objectmaker; 	 
extern  t_pd pd_canvasmaker; 	 
extern  t_symbol s_pointer;
extern  t_symbol s_float;
extern  t_symbol s_symbol;
extern  t_symbol s_bang;
extern  t_symbol s_list;
extern  t_symbol s_anything;
extern  t_symbol s_signal;
extern  t_symbol s__N;
extern  t_symbol s__X;
extern  t_symbol s_x;
extern  t_symbol s_y;
extern  t_symbol s_;

 
extern  void pd_typedmess(t_pd *x, t_symbol *s, int argc, t_atom *argv);
extern  void pd_forwardmess(t_pd *x, int argc, t_atom *argv);
extern  t_symbol *gensym(char *s);
extern  t_gotfn getfn(t_pd *x, t_symbol *s);
extern  t_gotfn zgetfn(t_pd *x, t_symbol *s);
extern  void nullfn(void);
extern  void pd_vmess(t_pd *x, t_symbol *s, char *fmt, ...);






void obj_list(t_object *x, t_symbol *s, int argc, t_atom *argv);

 
extern  void *getbytes(size_t nbytes);
extern  void *getzbytes(size_t nbytes);
extern  void *copybytes(void *src, size_t nbytes);
extern  void freebytes(void *x, size_t nbytes);
extern  void *resizebytes(void *x, size_t oldsize, size_t newsize);

 













extern  t_float atom_getfloat(t_atom *a);
extern  t_int atom_getint(t_atom *a);
extern  t_symbol *atom_getsymbol(t_atom *a);
extern  t_symbol *atom_gensym(t_atom *a);
extern  t_float atom_getfloatarg(int which, int argc, t_atom *argv);
extern  t_int atom_getintarg(int which, int argc, t_atom *argv);
extern  t_symbol *atom_getsymbolarg(int which, int argc, t_atom *argv);

extern  void atom_string(t_atom *a, char *buf, unsigned int bufsize);

 

extern  struct _binbuf  *binbuf_new(void);
extern  void binbuf_free(struct _binbuf  *x);

extern  void binbuf_text(struct _binbuf  *x, char *text, size_t size);
extern  void binbuf_gettext(struct _binbuf  *x, char **bufp, int *lengthp);
extern  void binbuf_clear(struct _binbuf  *x);
extern  void binbuf_add(struct _binbuf  *x, int argc, t_atom *argv);
extern  void binbuf_addv(struct _binbuf  *x, char *fmt, ...);
extern  void binbuf_addbinbuf(struct _binbuf  *x, struct _binbuf  *y);
extern  void binbuf_addsemi(struct _binbuf  *x);
extern  void binbuf_restore(struct _binbuf  *x, int argc, t_atom *argv);
extern  void binbuf_print(struct _binbuf  *x);
extern  int binbuf_getnatom(struct _binbuf  *x);
extern  t_atom *binbuf_getvec(struct _binbuf  *x);
extern  void binbuf_eval(struct _binbuf  *x, t_pd *target, int argc, t_atom *argv);
extern  int binbuf_read(struct _binbuf  *b, char *filename, char *dirname,
    int crflag);
extern  int binbuf_read_via_path(struct _binbuf  *b, char *filename, char *dirname,
    int crflag);
extern  int binbuf_write(struct _binbuf  *x, char *filename, char *dir,
    int crflag);
extern  void binbuf_evalfile(t_symbol *name, t_symbol *dir);

 

extern  struct _clock  *clock_new(void *owner, t_method fn);
extern  void clock_set(struct _clock  *x, double systime);
extern  void clock_delay(struct _clock  *x, double delaytime);
extern  void clock_unset(struct _clock  *x);
extern  double clock_getlogicaltime(void);
extern  double clock_getsystime(void);  
extern  double clock_gettimesince(double prevsystime);
extern  double clock_getsystimeafter(double delaytime);
extern  void clock_free(struct _clock  *x);

 
extern  t_pd *pd_new(struct _class  *cls);
extern  void pd_free(t_pd *x);
extern  void pd_bind(t_pd *x, t_symbol *s);
extern  void pd_unbind(t_pd *x, t_symbol *s);
extern  t_pd *pd_findbyclass(t_symbol *s, struct _class  *c);
extern  void pd_pushsym(t_pd *x);
extern  void pd_popsym(t_pd *x);
extern  t_symbol *pd_getfilename(void);
extern  t_symbol *pd_getdirname(void);
extern  void pd_bang(t_pd *x);
extern  void pd_pointer(t_pd *x, t_gpointer *gp);
extern  void pd_float(t_pd *x, t_float f);
extern  void pd_symbol(t_pd *x, t_symbol *s);
extern  void pd_list(t_pd *x, t_symbol *s, int argc, t_atom *argv);
extern  void pd_anything(t_pd *x, t_symbol *s, int argc, t_atom *argv);


 
extern  void gpointer_init(t_gpointer *gp);
extern  void gpointer_copy(const t_gpointer *gpfrom, t_gpointer *gpto);
extern  void gpointer_unset(t_gpointer *gp);
extern  int gpointer_check(const t_gpointer *gp, int headok);

 
struct  _inlet;

struct  _outlet;


extern  struct _inlet  *inlet_new(t_object *owner, t_pd *dest, t_symbol *s1,
    t_symbol *s2);
extern  struct _inlet  *pointerinlet_new(t_object *owner, t_gpointer *gp);
extern  struct _inlet  *floatinlet_new(t_object *owner, t_float *fp);
extern  struct _inlet  *symbolinlet_new(t_object *owner, t_symbol **sp);
extern  void inlet_free(struct _inlet  *x);

extern  struct _outlet  *outlet_new(t_object *owner, t_symbol *s);
extern  void outlet_bang(struct _outlet  *x);
extern  void outlet_pointer(struct _outlet  *x, t_gpointer *gp);
extern  void outlet_float(struct _outlet  *x, t_float f);
extern  void outlet_symbol(struct _outlet  *x, t_symbol *s);
extern  void outlet_list(struct _outlet  *x, t_symbol *s, int argc, t_atom *argv);
extern  void outlet_anything(struct _outlet  *x, t_symbol *s, int argc, t_atom *argv);
extern  void outlet_free(struct _outlet  *x);
extern  t_object *pd_checkobject(t_pd *x);


 

extern  void glob_setfilename(void *dummy, t_symbol *name, t_symbol *dir);

extern  void canvas_setargs(int argc, t_atom *argv);
extern  t_atom *canvas_getarg(int which);
extern  t_symbol *canvas_getcurrentdir(void);
extern  struct _glist  *canvas_getcurrent(void);
extern  void canvas_makefilename(struct _glist  *c, char *file,
    char *result,int resultsize);
extern  t_symbol *canvas_getdir(struct _glist  *x);
extern  int sys_fontwidth(int fontsize);
extern  int sys_fontheight(int fontsize);
extern  void canvas_dataproperties(struct _glist  *x, t_scalar *sc, struct _binbuf  *b);

 

struct  _widgetbehavior;


struct  _parentwidgetbehavior;

extern  struct _parentwidgetbehavior  *pd_getparentwidget(t_pd *x);

 










extern  struct _class  *class_new(t_symbol *name, t_newmethod newmethod,
    t_method freemethod, size_t size, int flags, t_atomtype arg1, ...);
extern  void class_addcreator(t_newmethod newmethod, t_symbol *s, 
    t_atomtype type1, ...);
extern  void class_addmethod(struct _class  *c, t_method fn, t_symbol *sel,
    t_atomtype arg1, ...);
extern  void class_addbang(struct _class  *c, t_method fn);
extern  void class_addpointer(struct _class  *c, t_method fn);
extern  void class_doaddfloat(struct _class  *c, t_method fn);
extern  void class_addsymbol(struct _class  *c, t_method fn);
extern  void class_addlist(struct _class  *c, t_method fn);
extern  void class_addanything(struct _class  *c, t_method fn);
extern  void class_sethelpsymbol(struct _class  *c, t_symbol *s);
extern  void class_setwidget(struct _class  *c, struct _widgetbehavior  *w);
extern  void class_setparentwidget(struct _class  *c, struct _parentwidgetbehavior  *w);
extern  struct _parentwidgetbehavior  *class_parentwidget(struct _class  *c);
extern  char *class_getname(struct _class  *c);
extern  char *class_gethelpname(struct _class  *c);
extern  void class_setdrawcommand(struct _class  *c);
extern  int class_isdrawcommand(struct _class  *c);
extern  void class_domainsignalin(struct _class  *c, int onset);












 
extern  void post(char *fmt, ...);
extern  void startpost(char *fmt, ...);
extern  void poststring(char *s);
extern  void postfloat(float f);
extern  void postatom(int argc, t_atom *argv);
extern  void endpost(void);
extern  void error(char *fmt, ...);
extern  void bug(char *fmt, ...);
extern  void pd_error(void *object, char *fmt, ...);
extern  void sys_logerror(char *object, char *s);
extern  void sys_unixerror(char *object);
extern  void sys_ouch(void);


extern  char* sys_get_path( void);

extern  void sys_addpath(const char* p);


 
extern  int sys_isreadablefile(const char *name);
extern  void sys_bashfilename(const char *from, char *to);
extern  void sys_unbashfilename(const char *from, char *to);
extern  int open_via_path(const char *name, const char *ext, const char *dir,
    char *dirresult, char **nameresult, unsigned int size, int bin);
extern  int sys_geteventno(void);
extern  double sys_getrealtime(void);

 

typedef float t_sample;



typedef struct _signal
{
    int s_n;	    	 
    t_sample *s_vec;	 
    float s_sr;     	 
    int s_refcount; 	 
    int s_isborrowed;	 
    struct _signal *s_borrowedfrom; 	 
    struct _signal *s_nextfree; 	 
    struct _signal *s_nextused;     	 
} t_signal;


typedef t_int *(*t_perfroutine)(t_int *args);

extern  t_int *plus_perform(t_int *args);
extern  t_int *zero_perform(t_int *args);
extern  t_int *copy_perform(t_int *args);

extern  void dsp_add_plus(t_sample *in1, t_sample *in2, t_sample *out, int n);
extern  void dsp_add_copy(t_sample *in, t_sample *out, int n);
extern  void dsp_add_scalarcopy(t_sample *in, t_sample *out, int n);
extern  void dsp_add_zero(t_sample *out, int n);

extern  int sys_getblksize(void);
extern  float sys_getsr(void);
extern  int sys_get_inchannels(void);
extern  int sys_get_outchannels(void);

extern  void dsp_add(t_perfroutine f, int n, ...);
extern  void dsp_addv(t_perfroutine f, int n, t_int *vec);
extern  void pd_fft(float *buf, int npoints, int inverse);
extern  int ilog2(int n);

extern  void mayer_fht(float *fz, int n);
extern  void mayer_fft(int n, float *real, float *imag);
extern  void mayer_ifft(int n, float *real, float *imag);
extern  void mayer_realfft(int n, float *real);
extern  void mayer_realifft(int n, float *real);

extern  float *cos_table;



extern  int canvas_suspend_dsp(void);
extern  void canvas_resume_dsp(int oldstate);
extern  void canvas_update_dsp(void);

 
typedef struct _resample
{
  int method;        

  t_int downsample;  
  t_int upsample;    

  t_float *s_vec;    
  int      s_n;

  t_float *coeffs;   
  int      coefsize;

  t_float *buffer;   
  int      bufsize;
} t_resample;

extern  void resample_init(t_resample *x);
extern  void resample_free(t_resample *x);

extern  void resample_dsp(t_resample *x, t_sample *in, int insize, t_sample *out, int outsize, int method);
extern  void resamplefrom_dsp(t_resample *x, t_sample *in, int insize, int outsize, int method);
extern  void resampleto_dsp(t_resample *x, t_sample *out, int insize, int outsize, int method);
 

 
extern  float mtof(float);
extern  float ftom(float);
extern  float rmstodb(float);
extern  float powtodb(float);
extern  float dbtorms(float);
extern  float dbtopow(float);

extern  float q8_sqrt(float);
extern  float q8_rsqrt(float);

extern  float qsqrt(float);   
extern  float qrsqrt(float);

 

     
struct  _garray;


extern  struct _class  *garray_class;
extern  int garray_getfloatarray(struct _garray  *x, int *size, t_float **vec);
extern  float garray_get(struct _garray  *x, t_symbol *s, t_int indx);
extern  void garray_redraw(struct _garray  *x);
extern  int garray_npoints(struct _garray  *x);
extern  char *garray_vec(struct _garray  *x);
extern  void garray_resize(struct _garray  *x, t_floatarg f);
extern  void garray_usedindsp(struct _garray  *x);
extern  void garray_setsaveit(struct _garray  *x, int saveit);
extern  struct _class  *scalar_class;

extern  t_float *value_get(t_symbol *s);
extern  void value_release(t_symbol *s);
extern  int value_getfloat(t_symbol *s, t_float *f);
extern  int value_setfloat(t_symbol *s, t_float f);

 
extern  void sys_vgui(char *fmt, ...);
extern  void sys_gui(char *s);

extern  void gfxstub_new(t_pd *owner, void *key, const char *cmd);
extern  void gfxstub_deleteforkey(void *key);

 

 

typedef struct _class  *t_externclass;

extern  void c_extern(t_externclass *cls, t_newmethod newroutine,
    t_method freeroutine, t_symbol *name, size_t size, int tiny,     t_atomtype arg1, ...);

extern  void c_addmess(t_method fn, t_symbol *sel, t_atomtype arg1, ...);











 





}

# 51 "/usr/local/lib/pd/externs/flext/flstdc.h" 2




}






typedef t_object t_sigobj;
typedef t_gpointer *t_ptrtype;

typedef t_float t_flint;
typedef t_symbol *t_symptr;





 
 
# 102 "/usr/local/lib/pd/externs/flext/flstdc.h"



# 118 "/usr/local/lib/pd/externs/flext/flstdc.h"


 











 













# 22 "/usr/local/lib/pd/externs/flext/flbase.h" 2



class flext_obj;

 
 






 

struct   flext_hdr
{
    	 



    	t_sigobj    	    obj;  


		 
		float defsig;			







    	 



        flext_obj           *data;
};


 
 



















 

class   flext_obj
{
    public:

		 
		 

		void *operator new(size_t bytes);
		void operator delete(void *blk);

		
		void *operator new[](size_t bytes) { return operator new(bytes); }
		void operator delete[](void *blk) { operator delete(blk); }
		

		 
		static void *NewAligned(size_t bytes,int bitalign = 128);
		static void FreeAligned(void *blk);
		
		 

         
    	flext_obj();

    	 
    	virtual ~flext_obj() = 0;
    	
         
        struct _glist             *thisCanvas()        { return(m_canvas); }

		t_sigobj *thisHdr() { return &x_obj->obj; }
		const char *thisName() const { return m_name; } 


		struct _class  *thisClass() { return (struct _class  *)((t_object *)(x_obj))->te_g.g_pd; }



 
    protected:    	
		
         
        flext_hdr          *x_obj;        	

    private:

         
        struct _glist             *m_canvas;

	public:

    	 
    	static void callb_setup(struct _class  *) {}	

    	 
         
        static flext_hdr     *m_holder;
        static const char *m_holdname;   

         
		const char *m_name;
		





# 166 "/usr/local/lib/pd/externs/flext/flbase.h"

};

 
inline void *operator new(size_t, void *location, void *) { return location; }

 
 
 



# 187 "/usr/local/lib/pd/externs/flext/flbase.h"



# 200 "/usr/local/lib/pd/externs/flext/flbase.h"


























 


 
 
 

















# 258 "/usr/local/lib/pd/externs/flext/flbase.h"


 
 
 

 







 





















 

 
 
 
 



 

# 320 "/usr/local/lib/pd/externs/flext/flbase.h"







 
 
 








 
 
 
 
 

 
 
 

# 369 "/usr/local/lib/pd/externs/flext/flbase.h"


# 386 "/usr/local/lib/pd/externs/flext/flbase.h"


 
 
 

# 414 "/usr/local/lib/pd/externs/flext/flbase.h"


# 431 "/usr/local/lib/pd/externs/flext/flbase.h"


 
 
 

# 459 "/usr/local/lib/pd/externs/flext/flbase.h"


# 476 "/usr/local/lib/pd/externs/flext/flbase.h"


 
 
 

# 504 "/usr/local/lib/pd/externs/flext/flbase.h"


# 521 "/usr/local/lib/pd/externs/flext/flbase.h"


 
 
 

# 549 "/usr/local/lib/pd/externs/flext/flbase.h"


# 566 "/usr/local/lib/pd/externs/flext/flbase.h"

 
 
 

# 593 "/usr/local/lib/pd/externs/flext/flbase.h"


# 610 "/usr/local/lib/pd/externs/flext/flbase.h"



 















# 21 "/usr/local/lib/pd/externs/flext/flclass.h" 2












 

 



class flext_base:
	public flext_obj
{
	public: typedef   flext_base   thisType; static void callb_free(flext_hdr *hdr) { flext_obj *mydata = ((flext_hdr *)hdr)->data; delete mydata; ((flext_hdr *)hdr)->flext_hdr::~flext_hdr(); } static void callb_setup(struct _class  *classPtr) {    flext_obj  ::callb_setup(classPtr);   flext_base  ::   Setup  (classPtr); } protected: static   flext_base   *thisObject(void *c) { return (  flext_base   *)((flext_hdr *)c)->data; }  
	
public:

 

	 



	static bool compatibility;  


 

	 
	virtual void m_help();
	
	 
	virtual void m_loadbang() {}

	 
	virtual void m_assist(long  ,long  ,char *  ) {}

	 



	virtual bool m_methodmain(int inlet,const t_symbol *s,int argc,t_atom *argv);

	 
	virtual bool m_method_(int inlet,const t_symbol *s,int argc,t_atom *argv);


 

	 
	class buffer
	{
	public:
		 



		buffer(t_symbol *s = __null ,bool delayed = false);
		
		 
		~buffer();

		 

		bool Ok() const { return sym != __null  && data != __null ; }
		
		 


		int Set(t_symbol *s = __null ,bool nameonly = false);
		
		 


		void Dirty(bool refr = false);
		
		 
		t_symbol *Symbol() const { return sym; }
		 
		const char *Name() const { return sym?sym->s_name:""; }
		
		 


		t_sample *Data() { return data; }
		 
		int Channels() const { return chns; }
		 
		int Frames() const { return frames; }
		
		 
		void SetRefrIntv(float intv);

	protected:
		t_symbol *sym;
		t_sample *data;
		int chns,frames;

		float interval;
		bool isdirty,ticking;
		struct _clock  *tick;

	private:
		static void cb_tick(buffer *b);

	};


 

	 
	 
	
 

	 
	void AddInAnything(int m = 1) { AddInlet(xlet::tp_any,m); }  
	 
	void AddInFloat(int m = 1) { AddInlet(xlet::tp_float,m); }
	 
	void AddInInt(int m = 1) { AddInlet(xlet::tp_int,m); }
	 
	void AddInSymbol(int m = 1) { AddInlet(xlet::tp_sym,m); }
	 
	void AddInBang(int m = 1) { AddInlet(xlet::tp_sym,m); }
	 
	void AddInList(int m = 1) { AddInlet(xlet::tp_list,m); }   
	
	 
	void AddOutAnything(int m = 1) { AddOutlet(xlet::tp_any,m); }
	 
	void AddOutFloat(int m = 1) { AddOutlet(xlet::tp_float,m); }
	 
	void AddOutInt(int m = 1) { AddOutlet(xlet::tp_int,m); }
	 
	void AddOutSymbol(int m = 1) { AddOutlet(xlet::tp_sym,m); }
	 
	void AddOutBang(int m = 1) { AddOutlet(xlet::tp_sym,m); }
	 
	void AddOutList(int m = 1) { AddOutlet(xlet::tp_list,m); }
	
	 



	bool SetupInOut(); 

	 
	int CntIn() const { return incnt; }
	 
	int CntOut() const { return outcnt; }
	 
	int CntInSig() const { return insigs; }
	 
	int CntOutSig() const { return outsigs; }


	class outlet;

	 
	outlet *GetOut(int ix) { return (outlets && ix < outcnt)?outlets[ix]: __null ; }

	 

	void ToOutBang(outlet *o); 
	 
	void ToOutBang(int n) { outlet *o = GetOut(n); if(o) ToOutBang(o); }

	void ToOutFloat(outlet *o,float f); 
	 
	void ToOutFloat(int n,float f) { outlet *o = GetOut(n); if(o) ToOutFloat(o,f); }

 
	 
 
	
	void ToOutInt(outlet *o,int f); 
	 
	void ToOutInt(int n,int f) { outlet *o = GetOut(n); if(o) ToOutInt(o,f); }
	
	void ToOutSymbol(outlet *o,const t_symbol *s); 
	 
	void ToOutSymbol(int n,const t_symbol *s) { outlet *o = GetOut(n); if(o) ToOutSymbol(o,const_cast<t_symbol *>(s)); }

	void ToOutList(outlet *o,int argc,t_atom *argv); 
	 
	void ToOutList(int n,int argc,t_atom *argv)  { outlet *o = GetOut(n); if(o) ToOutList(o,argc,argv); }
	
	void ToOutAnything(outlet *o,const t_symbol *s,int argc,t_atom *argv); 
	 
	void ToOutAnything(int n,const t_symbol *s,int argc,t_atom *argv)  { outlet *o = GetOut(n); if(o) ToOutAnything(o,const_cast<t_symbol *>(s),argc,argv); }
		
		
 

	enum metharg {
		a_null = 0,
		a_float,a_int, 
		a_symbol,a_pointer,
		a_gimme,a_xgimme
	};

	typedef void (*methfun)(struct _class  *c);

	void AddMethodDef(int inlet);  
	void AddMethodDef(int inlet,const char *tag);  
	void AddMethod(int inlet,const char *tag,methfun fun,metharg tp,...); 

	void AddMethod(int inlet,void (*m)(flext_base *,int argc,t_atom *argv)) { AddMethod(inlet,"list",(methfun)m,a_gimme,a_null); }
	void AddMethod(int inlet,const char *tag,void (*m)(flext_base *)) { AddMethod(inlet,tag,(methfun)m,a_null); }   
	void AddMethod(int inlet,void (*m)(flext_base *,t_symbol *s,int argc,t_atom *argv)) { AddMethod(inlet,"anything",(methfun)m,a_xgimme,a_null); }  
	void AddMethod(int inlet,void (*m)(flext_base *,t_symbol *s)) { AddMethod(inlet,"symbol",(methfun)m,a_symbol,a_null); }  
	void AddMethod(int inlet,void (*m)(flext_base *,float &)) { AddMethod(inlet,"float",(methfun)m,a_float,a_null); }   
	void AddMethod(int inlet,void (*m)(flext_base *,float &,float &)) { AddMethod(inlet,"list",(methfun)m,a_float,a_float,a_null); }  
	void AddMethod(int inlet,void (*m)(flext_base *,float &,float &,float &)) { AddMethod(inlet,"list",(methfun)m,a_float,a_float,a_float,a_null); }  

	void AddMethod(int inlet,void (*m)(flext_base *,int &)) { AddMethod(inlet,"float",(methfun)m,a_int,a_null); }   



	void AddMethod(int inlet,void (*m)(flext_base *,int &,int &)) { AddMethod(inlet,"list",(methfun)m,a_int,a_int,a_null); }  
	void AddMethod(int inlet,void (*m)(flext_base *,int &,int &,int &)) { AddMethod(inlet,"list",(methfun)m,a_int,a_int,a_int,a_null); }  
	void AddMethod(int inlet,const char *tag,void (*m)(flext_base *,int argc,t_atom *argv)) { AddMethod(inlet,tag,(methfun)m,a_gimme,a_null); }  
	void AddMethod(int inlet,const char *tag,void (*m)(flext_base *,t_symbol *s,int argc,t_atom *argv)) { AddMethod(inlet,tag,(methfun)m,a_xgimme,a_null); }  
	void AddMethod(int inlet,const char *tag,void (*m)(flext_base *,t_symbol *s)) { AddMethod(inlet,tag,(methfun)m,a_symbol,a_null); }  
	void AddMethod(int inlet,const char *tag,void (*m)(flext_base *,float &)) { AddMethod(inlet,tag,(methfun)m,a_float,a_null); }   
	void AddMethod(int inlet,const char *tag,void (*m)(flext_base *,int &)) { AddMethod(inlet,tag,(methfun)m,a_int,a_null); }  

	 
	void SetDist(bool d = true) { distmsgs = d; }

 

	static const t_symbol *sym_float;
	static const t_symbol *sym_symbol;
	static const t_symbol *sym_bang;
	static const t_symbol *sym_list;
	static const t_symbol *sym_anything;
	static const t_symbol *sym_int;
	static const t_symbol *sym_pointer;


	static const t_symbol *sym_signal;


	static const t_symbol *MakeSymbol(const char *s);

	 
	static const char *GetString(const t_symbol *s) { return s->s_name; }  
	static const char *GetAString(const t_symbol *s) { return s?s->s_name:""; }  
		
 
		
	static bool IsFloat(const t_atom &a) { return a.a_type == A_FLOAT; }
	static bool CanbeFloat(const t_atom &a) { return IsFloat(a) || IsInt(a); }
	static float GetFloat(const t_atom &a) { return a.a_w.w_float; }
 
	static float GetAFloat(const t_atom &a) { return GetAFlint(a); }
	static void SetFloat(t_atom &a,float v) { a.a_type = A_FLOAT; a.a_w.w_float = v; }

	static bool IsSymbol(const t_atom &a) { return a.a_type == A_SYMBOL; }
	static t_symbol *GetSymbol(const t_atom &a) { return a.a_w.w_symbol; }
	static t_symbol *GetASymbol(const t_atom &a) { return IsSymbol(a)?GetSymbol(a): __null ; }   
	static void SetSymbol(t_atom &a,const t_symbol *s) { a.a_type = A_SYMBOL; a.a_w.w_symbol = const_cast<t_symbol *>(s); }

	static bool IsString(const t_atom &a) { return IsSymbol(a); }
	static const char *GetString(const t_atom &a) { t_symbol *s = GetSymbol(a); return s?GetString(s): __null ; }  
	static void GetAString(const t_atom &a,char *buf,int szbuf);
	static void SetString(t_atom &a,const char *c) { SetSymbol(a,gensym(const_cast<char *>(c))); }

	static bool CanbeInt(const t_atom &a) { return IsFloat(a) || IsInt(a); }
	static float GetAInt(const t_atom &a) { return GetAFlint(a); }

	static bool IsFlint(const t_atom &a) { return IsFloat(a) || IsInt(a); }
	static float GetAFlint(const t_atom &a) { return IsFloat(a)?GetFloat(a):(IsInt(a)?GetInt(a):0); }

	static bool IsInt(const t_atom &) { return false; }
	static int GetInt(const t_atom &) { return 0; }
 

	static void SetFlint(t_atom &a,int v) { a.a_type = A_FLOAT; a.a_w.w_float = (float)v; }

	static bool IsPointer(const t_atom &a) { return a.a_type == A_POINTER; }
	static t_gpointer *GetPointer(const t_atom &a) { return a.a_w.w_gpointer; }
	static t_gpointer *GetAPointer(const t_atom &a) { return IsPointer(a)?GetPointer(a): __null ; }
	static void SetPointer(t_atom &a,t_gpointer *p) { a.a_type = A_POINTER; a.a_w.w_gpointer = p; }

# 327 "/usr/local/lib/pd/externs/flext/flclass.h"


 


 


 

protected:

	flext_base();
	virtual ~flext_base();
		
 
		
	struct xlet {	
		enum type {
			tp_none = 0,
			tp_float,  tp_int,tp_sym,tp_list,tp_sig,tp_any
		};

		xlet(type t): tp(t),nxt(__null ) {}
		~xlet();
		
		type tp;
		xlet *nxt;
	};

	unsigned long XletCode(xlet::type tp = xlet::tp_none,...);  

	void AddInlets(unsigned long code);  
	void AddInlet(xlet::type tp,int mult = 1) { AddXlet(tp,mult,inlist); }
	void AddOutlets(unsigned long code);  
	void AddOutlet(xlet::type tp,int mult = 1) { AddXlet(tp,mult,outlist); }

 

	class methitem { 
	public:
		methitem(int inlet,t_symbol *t);
		~methitem();

		void SetArgs(methfun fun,int argc,metharg *args);

		t_symbol *tag;
		int inlet;
		int argc;
		metharg *args;
		methfun fun;
		
		methitem *nxt;
	};
	
	void AddMethItem(methitem *m);
	
private:

	static void Setup(struct _class  *c);

	xlet *inlist,*outlist;
	int incnt,outcnt,insigs,outsigs;
	outlet **outlets;
	bool distmsgs;

	void AddXlet(xlet::type tp,int mult,xlet *&root);	

	methitem *mlst;


	 
	struct px_object;
	friend struct px_object;
# 416 "/usr/local/lib/pd/externs/flext/flclass.h"

	static void cb_px_anything(struct _class  *c,const t_symbol *s,int argc,t_atom *argv);

	static void cb_px_ft1(struct _class  *c,float f);
	static void cb_px_ft2(struct _class  *c,float f);
	static void cb_px_ft3(struct _class  *c,float f);
	static void cb_px_ft4(struct _class  *c,float f);
	static void cb_px_ft5(struct _class  *c,float f);
	static void cb_px_ft6(struct _class  *c,float f);
	static void cb_px_ft7(struct _class  *c,float f);
	static void cb_px_ft8(struct _class  *c,float f);
	static void cb_px_ft9(struct _class  *c,float f);

	px_object **inlets;

	 

	static void cb_help(struct _class  *c);

	static void cb_loadbang(struct _class  *c);



};



 

 



class flext_dsp:
	public flext_base
{
	public: typedef   flext_dsp   thisType; static void callb_free(flext_hdr *hdr) { flext_obj *mydata = ((flext_hdr *)hdr)->data; delete mydata; ((flext_hdr *)hdr)->flext_hdr::~flext_hdr(); } static void callb_setup(struct _class  *classPtr) {    flext_base  ::callb_setup(classPtr);   flext_dsp  ::   Setup  (classPtr); } protected: static   flext_dsp   *thisObject(void *c) { return (  flext_dsp   *)((flext_hdr *)c)->data; }  
	
public:

	 
	float Samplerate() const { return srate; }
	
	 
	int Blocksize() const { return blksz; }
	
	 
	int ChannelsIn() const { return chnsin; }
	 
	int ChannelsOut() const { return chnsout; }
	

 

	 





	virtual void m_dsp(int n,t_sample *const *insigs,t_sample *const *outsigs);

	 





	virtual void m_signal(int n,t_sample *const *insigs,t_sample *const *outsigs) = 0;


	 
	virtual void m_enable(bool on);


 

	 
	void AddInSignal(int m = 1) { AddInlet(xlet::tp_sig,m); }

	 
	void AddOutSignal(int m = 1) { AddOutlet(xlet::tp_sig,m); }


protected:
	
	flext_dsp();
	~flext_dsp();
	
private:

	 
	float srate; 
	int blksz;
	int chnsin,chnsout;

	 
	static void Setup(struct _class  *c);

	 

	static void cb_dsp(struct _class  *c,t_signal **s);

	static void cb_enable(struct _class  *c,t_flint on);
	bool dspon;


	 

	static t_int *dspmeth(t_int *w); 
	t_sample **invecs,**outvecs;
};


# 29 "/usr/local/lib/pd/externs/flext/flext.h" 2





# 11 "transverb.hpp" 2

# 1 "/usr/include/errno.h" 1 3
 

















 





 






extern "C" { 

 

# 1 "/usr/include/bits/errno.h" 1 3
 























# 1 "/usr/include/linux/errno.h" 1 3



# 1 "/usr/include/asm/errno.h" 1 3




































































































































# 4 "/usr/include/linux/errno.h" 2 3


# 24 "/usr/include/linux/errno.h" 3



# 25 "/usr/include/bits/errno.h" 2 3


 


 




 
extern int errno;

 
extern int *__errno_location (void) throw ()  __attribute__ ((__const__));







 













# 36 "/usr/include/errno.h" 2 3





 

















} 



 










# 12 "transverb.hpp" 2

# 1 "/usr/include/unistd.h" 1 3
 

















 








extern "C" { 

 


 



 


 


 


 



 



 



 



 






 


 




 


 


 



 



 



















































































# 1 "/usr/include/bits/posix_opt.h" 1 3
 





















 


 


 


 


 


 


 


 


 


 


 


 


 



 


 


 


 


 


 



 


 


 


 


 


 


 


 


 




 


 


 


 


 


 


 


 


 


 


 


 



# 175 "/usr/include/unistd.h" 2 3


 




 





 










# 1 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 1 3






 


# 19 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3



 


 





 


# 61 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 





 


















 





 

 

# 131 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 


# 188 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3





 




 

# 271 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


# 283 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3


 

 

# 317 "/usr/lib/gcc-lib/i386-linux/2.95.4/include/stddef.h" 3




 





















# 199 "/usr/include/unistd.h" 2 3


# 236 "/usr/include/unistd.h" 3




typedef __intptr_t intptr_t;






typedef __socklen_t socklen_t;




 






 
extern int access (__const char *__name, int __type) throw () ;








 







 






 





extern __off_t lseek (int __fd, __off_t __offset, int __whence) throw () ;
# 300 "/usr/include/unistd.h" 3





 
extern int close (int __fd) throw () ;

 

extern ssize_t read (int __fd, void *__buf, size_t __nbytes) throw () ;

 
extern ssize_t write (int __fd, __const void *__buf, size_t __n) throw () ;

# 347 "/usr/include/unistd.h" 3


 



extern int pipe (int __pipedes[2]) throw () ;

 






extern unsigned int alarm (unsigned int __seconds) throw () ;

 






extern unsigned int sleep (unsigned int __seconds) throw () ;


 



extern __useconds_t ualarm (__useconds_t __value, __useconds_t __interval)
     throw () ;

 

extern int usleep (__useconds_t __useconds) throw () ;



 

extern int pause (void) throw () ;


 
extern int chown (__const char *__file, __uid_t __owner, __gid_t __group)
     throw () ;


 
extern int fchown (int __fd, __uid_t __owner, __gid_t __group) throw () ;


 

extern int lchown (__const char *__file, __uid_t __owner, __gid_t __group)
     throw () ;



 
extern int chdir (__const char *__path) throw () ;


 
extern int fchdir (int __fd) throw () ;


 






extern char *getcwd (char *__buf, size_t __size) throw () ;









 


extern char *getwd (char *__buf) throw () ;



 
extern int dup (int __fd) throw () ;

 
extern int dup2 (int __fd, int __fd2) throw () ;

 
extern char **__environ;





 

extern int execve (__const char *__path, char *__const __argv[],
		   char *__const __envp[]) throw () ;









 
extern int execv (__const char *__path, char *__const __argv[]) throw () ;

 

extern int execle (__const char *__path, __const char *__arg, ...) throw () ;

 

extern int execl (__const char *__path, __const char *__arg, ...) throw () ;

 

extern int execvp (__const char *__file, char *__const __argv[]) throw () ;

 


extern int execlp (__const char *__file, __const char *__arg, ...) throw () ;



 
extern int nice (int __inc) throw () ;



 
extern void _exit (int __status) __attribute__ ((__noreturn__));


 


# 1 "/usr/include/bits/confname.h" 1 3
 






















 
enum
  {
    _PC_LINK_MAX,

    _PC_MAX_CANON,

    _PC_MAX_INPUT,

    _PC_NAME_MAX,

    _PC_PATH_MAX,

    _PC_PIPE_BUF,

    _PC_CHOWN_RESTRICTED,

    _PC_NO_TRUNC,

    _PC_VDISABLE,

    _PC_SYNC_IO,

    _PC_ASYNC_IO,

    _PC_PRIO_IO,

    _PC_SOCK_MAXBUF,

    _PC_FILESIZEBITS,

    _PC_REC_INCR_XFER_SIZE,

    _PC_REC_MAX_XFER_SIZE,

    _PC_REC_MIN_XFER_SIZE,

    _PC_REC_XFER_ALIGN,

    _PC_ALLOC_SIZE_MIN,

    _PC_SYMLINK_MAX

  };

 
enum
  {
    _SC_ARG_MAX,

    _SC_CHILD_MAX,

    _SC_CLK_TCK,

    _SC_NGROUPS_MAX,

    _SC_OPEN_MAX,

    _SC_STREAM_MAX,

    _SC_TZNAME_MAX,

    _SC_JOB_CONTROL,

    _SC_SAVED_IDS,

    _SC_REALTIME_SIGNALS,

    _SC_PRIORITY_SCHEDULING,

    _SC_TIMERS,

    _SC_ASYNCHRONOUS_IO,

    _SC_PRIORITIZED_IO,

    _SC_SYNCHRONIZED_IO,

    _SC_FSYNC,

    _SC_MAPPED_FILES,

    _SC_MEMLOCK,

    _SC_MEMLOCK_RANGE,

    _SC_MEMORY_PROTECTION,

    _SC_MESSAGE_PASSING,

    _SC_SEMAPHORES,

    _SC_SHARED_MEMORY_OBJECTS,

    _SC_AIO_LISTIO_MAX,

    _SC_AIO_MAX,

    _SC_AIO_PRIO_DELTA_MAX,

    _SC_DELAYTIMER_MAX,

    _SC_MQ_OPEN_MAX,

    _SC_MQ_PRIO_MAX,

    _SC_VERSION,

    _SC_PAGESIZE,


    _SC_RTSIG_MAX,

    _SC_SEM_NSEMS_MAX,

    _SC_SEM_VALUE_MAX,

    _SC_SIGQUEUE_MAX,

    _SC_TIMER_MAX,


     

    _SC_BC_BASE_MAX,

    _SC_BC_DIM_MAX,

    _SC_BC_SCALE_MAX,

    _SC_BC_STRING_MAX,

    _SC_COLL_WEIGHTS_MAX,

    _SC_EQUIV_CLASS_MAX,

    _SC_EXPR_NEST_MAX,

    _SC_LINE_MAX,

    _SC_RE_DUP_MAX,

    _SC_CHARCLASS_NAME_MAX,


    _SC_2_VERSION,

    _SC_2_C_BIND,

    _SC_2_C_DEV,

    _SC_2_FORT_DEV,

    _SC_2_FORT_RUN,

    _SC_2_SW_DEV,

    _SC_2_LOCALEDEF,


    _SC_PII,

    _SC_PII_XTI,

    _SC_PII_SOCKET,

    _SC_PII_INTERNET,

    _SC_PII_OSI,

    _SC_POLL,

    _SC_SELECT,

    _SC_UIO_MAXIOV,

    _SC_IOV_MAX = _SC_UIO_MAXIOV ,

    _SC_PII_INTERNET_STREAM,

    _SC_PII_INTERNET_DGRAM,

    _SC_PII_OSI_COTS,

    _SC_PII_OSI_CLTS,

    _SC_PII_OSI_M,

    _SC_T_IOV_MAX,


     
    _SC_THREADS,

    _SC_THREAD_SAFE_FUNCTIONS,

    _SC_GETGR_R_SIZE_MAX,

    _SC_GETPW_R_SIZE_MAX,

    _SC_LOGIN_NAME_MAX,

    _SC_TTY_NAME_MAX,

    _SC_THREAD_DESTRUCTOR_ITERATIONS,

    _SC_THREAD_KEYS_MAX,

    _SC_THREAD_STACK_MIN,

    _SC_THREAD_THREADS_MAX,

    _SC_THREAD_ATTR_STACKADDR,

    _SC_THREAD_ATTR_STACKSIZE,

    _SC_THREAD_PRIORITY_SCHEDULING,

    _SC_THREAD_PRIO_INHERIT,

    _SC_THREAD_PRIO_PROTECT,

    _SC_THREAD_PROCESS_SHARED,


    _SC_NPROCESSORS_CONF,

    _SC_NPROCESSORS_ONLN,

    _SC_PHYS_PAGES,

    _SC_AVPHYS_PAGES,

    _SC_ATEXIT_MAX,

    _SC_PASS_MAX,


    _SC_XOPEN_VERSION,

    _SC_XOPEN_XCU_VERSION,

    _SC_XOPEN_UNIX,

    _SC_XOPEN_CRYPT,

    _SC_XOPEN_ENH_I18N,

    _SC_XOPEN_SHM,


    _SC_2_CHAR_TERM,

    _SC_2_C_VERSION,

    _SC_2_UPE,


    _SC_XOPEN_XPG2,

    _SC_XOPEN_XPG3,

    _SC_XOPEN_XPG4,


    _SC_CHAR_BIT,

    _SC_CHAR_MAX,

    _SC_CHAR_MIN,

    _SC_INT_MAX,

    _SC_INT_MIN,

    _SC_LONG_BIT,

    _SC_WORD_BIT,

    _SC_MB_LEN_MAX,

    _SC_NZERO,

    _SC_SSIZE_MAX,

    _SC_SCHAR_MAX,

    _SC_SCHAR_MIN,

    _SC_SHRT_MAX,

    _SC_SHRT_MIN,

    _SC_UCHAR_MAX,

    _SC_UINT_MAX,

    _SC_ULONG_MAX,

    _SC_USHRT_MAX,


    _SC_NL_ARGMAX,

    _SC_NL_LANGMAX,

    _SC_NL_MSGMAX,

    _SC_NL_NMAX,

    _SC_NL_SETMAX,

    _SC_NL_TEXTMAX,


    _SC_XBS5_ILP32_OFF32,

    _SC_XBS5_ILP32_OFFBIG,

    _SC_XBS5_LP64_OFF64,

    _SC_XBS5_LPBIG_OFFBIG,


    _SC_XOPEN_LEGACY,

    _SC_XOPEN_REALTIME,

    _SC_XOPEN_REALTIME_THREADS,


    _SC_ADVISORY_INFO,

    _SC_BARRIERS,

    _SC_BASE,

    _SC_C_LANG_SUPPORT,

    _SC_C_LANG_SUPPORT_R,

    _SC_CLOCK_SELECTION,

    _SC_CPUTIME,

    _SC_THREAD_CPUTIME,

    _SC_DEVICE_IO,

    _SC_DEVICE_SPECIFIC,

    _SC_DEVICE_SPECIFIC_R,

    _SC_FD_MGMT,

    _SC_FIFO,

    _SC_PIPE,

    _SC_FILE_ATTRIBUTES,

    _SC_FILE_LOCKING,

    _SC_FILE_SYSTEM,

    _SC_MONOTONIC_CLOCK,

    _SC_MULTI_PROCESS,

    _SC_SINGLE_PROCESS,

    _SC_NETWORKING,

    _SC_READER_WRITER_LOCKS,

    _SC_SPIN_LOCKS,

    _SC_REGEXP,

    _SC_REGEX_VERSION,

    _SC_SHELL,

    _SC_SIGNALS,

    _SC_SPAWN,

    _SC_SPORADIC_SERVER,

    _SC_THREAD_SPORADIC_SERVER,

    _SC_SYSTEM_DATABASE,

    _SC_SYSTEM_DATABASE_R,

    _SC_TIMEOUTS,

    _SC_TYPED_MEMORY_OBJECTS,

    _SC_USER_GROUPS,

    _SC_USER_GROUPS_R,

    _SC_2_PBS,

    _SC_2_PBS_ACCOUNTING,

    _SC_2_PBS_LOCATE,

    _SC_2_PBS_MESSAGE,

    _SC_2_PBS_TRACK,

    _SC_SYMLOOP_MAX,

    _SC_STREAMS,

    _SC_2_PBS_CHECKPOINT,


    _SC_V6_ILP32_OFF32,

    _SC_V6_ILP32_OFFBIG,

    _SC_V6_LP64_OFF64,

    _SC_V6_LPBIG_OFFBIG,


    _SC_HOST_NAME_MAX,

    _SC_TRACE,

    _SC_TRACE_EVENT_FILTER,

    _SC_TRACE_INHERIT,

    _SC_TRACE_LOG

  };




 
enum
  {
    _CS_PATH,			 


# 492 "/usr/include/bits/confname.h" 3


# 527 "/usr/include/bits/confname.h" 3

# 561 "/usr/include/bits/confname.h" 3


    _CS_V6_WIDTH_RESTRICTED_ENVS

  };

# 500 "/usr/include/unistd.h" 2 3


 
extern long int pathconf (__const char *__path, int __name) throw () ;

 
extern long int fpathconf (int __fd, int __name) throw () ;

 
extern long int sysconf (int __name) throw ()  __attribute__ ((__const__));


 
extern size_t confstr (int __name, char *__buf, size_t __len) throw () ;



 
extern __pid_t getpid (void) throw () ;

 
extern __pid_t getppid (void) throw () ;

 


extern __pid_t getpgrp (void) throw () ;








 
extern __pid_t __getpgid (__pid_t __pid) throw () ;





 


extern int setpgid (__pid_t __pid, __pid_t __pgid) throw () ;


 











 

extern int setpgrp (void) throw () ;

# 574 "/usr/include/unistd.h" 3



 


extern __pid_t setsid (void) throw () ;






 
extern __uid_t getuid (void) throw () ;

 
extern __uid_t geteuid (void) throw () ;

 
extern __gid_t getgid (void) throw () ;

 
extern __gid_t getegid (void) throw () ;

 


extern int getgroups (int __size, __gid_t __list[]) throw () ;






 



extern int setuid (__uid_t __uid) throw () ;


 

extern int setreuid (__uid_t __ruid, __uid_t __euid) throw () ;



 
extern int seteuid (__uid_t __uid) throw () ;


 



extern int setgid (__gid_t __gid) throw () ;


 

extern int setregid (__gid_t __rgid, __gid_t __egid) throw () ;



 
extern int setegid (__gid_t __gid) throw () ;



 


extern __pid_t fork (void) throw () ;


 



extern __pid_t vfork (void) throw () ;



 

extern char *ttyname (int __fd) throw () ;

 

extern int ttyname_r (int __fd, char *__buf, size_t __buflen) throw () ;

 

extern int isatty (int __fd) throw () ;



 

extern int ttyslot (void) throw () ;



 
extern int link (__const char *__from, __const char *__to) throw () ;


 
extern int symlink (__const char *__from, __const char *__to) throw () ;

 


extern int readlink (__const char *__restrict __path, char *__restrict __buf,
		     size_t __len) throw () ;


 
extern int unlink (__const char *__name) throw () ;

 
extern int rmdir (__const char *__path) throw () ;


 
extern __pid_t tcgetpgrp (int __fd) throw () ;

 
extern int tcsetpgrp (int __fd, __pid_t __pgrp_id) throw () ;


 
extern char *getlogin (void) throw () ;








 
extern int setlogin (__const char *__name) throw () ;




 



# 1 "/usr/include/getopt.h" 1 3
 
























 











extern "C" {


 





extern char *optarg;

 











extern int optind;

 


extern int opterr;

 

extern int optopt;

# 113 "/usr/include/getopt.h" 3



 

























 


extern int getopt (int ___argc, char *const *___argv, const char *__shortopts);




# 163 "/usr/include/getopt.h" 3

# 172 "/usr/include/getopt.h" 3



}


 



# 726 "/usr/include/unistd.h" 2 3





 


extern int gethostname (char *__name, size_t __len) throw () ;




 

extern int sethostname (__const char *__name, size_t __len) throw () ;

 

extern int sethostid (long int __id) throw () ;


 


extern int getdomainname (char *__name, size_t __len) throw () ;
extern int setdomainname (__const char *__name, size_t __len) throw () ;


 


extern int vhangup (void) throw () ;

 
extern int revoke (__const char *__file) throw () ;


 




extern int profil (unsigned short int *__sample_buffer, size_t __size,
		   size_t __offset, unsigned int __scale) throw () ;


 


extern int acct (__const char *__name) throw () ;


 
extern char *getusershell (void) throw () ;
extern void endusershell (void) throw () ;  
extern void setusershell (void) throw () ;  


 


extern int daemon (int __nochdir, int __noclose) throw () ;




 

extern int chroot (__const char *__path) throw () ;

 

extern char *getpass (__const char *__prompt) throw () ;




 
extern int fsync (int __fd) throw () ;





 
extern long int gethostid (void) throw () ;

 
extern void sync (void) throw () ;


 

extern int getpagesize (void)  throw ()  __attribute__ ((__const__));


 

extern int truncate (__const char *__file, __off_t __length) throw () ;
# 834 "/usr/include/unistd.h" 3





 

extern int ftruncate (int __fd, __off_t __length) throw () ;













 

extern int getdtablesize (void) throw () ;






 

extern int brk (void *__addr) throw () ;

 



extern void *sbrk (intptr_t __delta) throw () ;




 









extern long int syscall (long int __sysno, ...) throw () ;





 



 









extern int lockf (int __fd, int __cmd, __off_t __len) throw () ;














# 933 "/usr/include/unistd.h" 3



 

extern int fdatasync (int __fildes) throw () ;



 

# 959 "/usr/include/unistd.h" 3



 








 

 









extern int pthread_atfork (void (*__prepare) (void),
			   void (*__parent) (void),
			   void (*__child) (void)) throw () ;


} 


# 13 "transverb.hpp" 2






# 1 "../dfx-library/dfxmisc.h" 1













 
 



















 












 
const float ONE_DIV_RAND_MAX = 1.0f / (float)2147483647 ;








 


 


 
inline float magmax(float a, float b) {
  if (fabs(a) > fabs(b)) return a;
  else return b;
}


 
 

 
 

 
 

double LambertW(double input);


 
 

 
inline float interpolateHermite(float *data, double address, long arraysize)
{
	long pos = (long)address;
	float posFract = (float) (address - (double)pos);

	long posMinus1 = (pos == 0) ? arraysize-1 : pos-1;
	long posPlus1 = (pos+1) % arraysize;
	long posPlus2 = (pos+2) % arraysize;

	float a = ( (3.0f*(data[pos]-data[posPlus1])) - data[posMinus1] + data[posPlus2] ) * 0.5f;
	float b = (2.0f*data[posPlus1]) + data[posMinus1] - (2.5f*data[pos]) - (data[posPlus2]*0.5f);
	float c = (data[posPlus1] - data[posMinus1]) * 0.5f;

	return (( ((a*posFract)+b) * posFract + c ) * posFract) + data[pos];
}

inline float interpolateLinear(float *data, double address, long arraysize)
{
	long pos = (long)address;
	float posFract = (float) (address - (double)pos);
	return (data[pos] * (1.0f-posFract)) + (data[(pos+1)%arraysize] * posFract);
}

inline float interpolateRandom(float randMin, float randMax)
{
	float randy = (float)rand() * ONE_DIV_RAND_MAX;
	return ((randMax-randMin) * randy) + randMin;
}

inline float interpolateLinear2values(float point1, float point2, double address)
{
	float posFract = (float) (address - (double)((long)address));
	return (point1 * (1.0f-posFract)) + (point2 * posFract);
}

 
 
 













 
 

# 182 "../dfx-library/dfxmisc.h"


struct dfxmutex {
	dfxmutex() {}
	~dfxmutex() {}
	void grab () {}
	void release () {}
};





# 19 "transverb.hpp" 2

# 1 "../dfx-library/IIRfilter.h" 1







class IIRfilter
{
public:
	IIRfilter();
	~IIRfilter();

	void calculateLowpassCoefficients(float cutoff, float samplerate);
	void calculateHighpassCoefficients(float cutoff, float samplerate);
	void copyCoefficients(IIRfilter *source);

	void reset()
	{
		prevIn = prevprevIn = prevOut = prevprevOut = prevprevprevOut = currentOut = 0.0f;
	}

	float prevIn, prevprevIn, prevOut, prevprevOut, prevprevprevOut, currentOut;
	float pOutCoeff, ppOutCoeff, pInCoeff, ppInCoeff, inCoeff;



	void process(float currentIn)



	{
	
		 
		prevprevprevOut = prevprevOut;
	
		prevprevOut = prevOut;
		prevOut = currentOut;

 
 
		currentOut = ((currentIn+prevprevIn)*inCoeff) + (prevIn*pInCoeff) 
					- (prevOut*pOutCoeff) - (prevprevOut*ppOutCoeff);

		prevprevIn = prevIn;
		prevIn = currentIn;

	


	}


 
 

	void processH1(float currentIn)
	{
		prevprevprevOut = prevprevOut;
		prevprevOut = prevOut;
		prevOut = currentOut;
		 
		currentOut = ( (currentIn+prevprevIn) * inCoeff ) + (prevIn  * pInCoeff)
					- (prevOut * pOutCoeff) - (prevprevOut * ppOutCoeff);
		 
		prevprevIn = prevIn;
		prevIn = currentIn;
	}

	void processH2(float *in, long inPos, long arraySize)
	{
	  float in0 = in[inPos];
	  float in1 = in[(inPos+1) % arraySize];

		prevprevprevOut = prevprevOut;
		prevprevOut = prevOut;
		prevOut = currentOut;
		currentOut = ( (in0+prevprevIn) * inCoeff ) + (prevIn * pInCoeff)
					- (prevOut * pOutCoeff) - (prevprevOut * ppOutCoeff);
		 
		prevprevprevOut = prevprevOut;
		prevprevOut = prevOut;
		prevOut = currentOut;
		currentOut = ( (in1+prevIn) * inCoeff ) + (in0 * pInCoeff)
					- (prevOut * pOutCoeff) - (prevprevOut * ppOutCoeff);
		 
		prevprevIn = in0;
		prevIn = in1;
	}

	void processH3(float *in, long inPos, long arraySize)
	{
	  float in0 = in[inPos];
	  float in1 = in[(inPos+1) % arraySize];
	  float in2 = in[(inPos+2) % arraySize];

		prevprevprevOut = ( (in0+prevprevIn) * inCoeff ) + (prevIn * pInCoeff)
						- (currentOut * pOutCoeff) - (prevOut * ppOutCoeff);
		prevprevOut = ((in1+prevIn) * inCoeff) + (in0 * pInCoeff)
						- (prevprevprevOut * pOutCoeff) - (currentOut * ppOutCoeff);
		prevOut = ((in2+in0) * inCoeff) + (in1 * pInCoeff)
						- (prevprevOut * pOutCoeff) - (prevprevprevOut * ppOutCoeff);
		 
		currentOut = prevOut;
		prevOut = prevprevOut;
		prevprevOut = prevprevprevOut;
		prevprevprevOut = currentOut;
		 
		prevprevIn = in1;
		prevIn = in2;
	}

	void processH4(float *in, long inPos, long arraySize)
	{
	  float in0 = in[inPos];
	  float in1 = in[(inPos+1) % arraySize];
	  float in2 = in[(inPos+2) % arraySize];
	  float in3 = in[(inPos+3) % arraySize];

		prevprevprevOut = ( (in0+prevprevIn) * inCoeff ) + (prevIn * pInCoeff)
						- (currentOut * pOutCoeff) - (prevOut * ppOutCoeff);
		prevprevOut = ((in1+prevIn) * inCoeff) + (in0 * pInCoeff)
						- (prevprevprevOut * pOutCoeff) - (currentOut * ppOutCoeff);
		prevOut = ((in2+in0) * inCoeff) + (in1 * pInCoeff)
						- (prevprevOut * pOutCoeff) - (prevprevprevOut * ppOutCoeff);
		currentOut = ((in3+in1) * inCoeff) + (in2 * pInCoeff)
						- (prevOut * pOutCoeff) - (prevprevOut * ppOutCoeff);
		 
		prevprevIn = in2;
		prevIn = in3;
	}


 

};	 



 
inline float interpolateHermitePostFilter(IIRfilter *filter, double address)
{
	long pos = (long)address;
	float posFract = (float) (address - (double)pos);

	float a = ( (3.0f*(filter->prevprevOut-filter->prevOut)) - 
				filter->prevprevprevOut + filter->currentOut ) * 0.5f;
	float b = (2.0f*filter->prevOut) + filter->prevprevprevOut - 
				(2.5f*filter->prevprevOut) - (filter->currentOut*0.5f);
	float c = (filter->prevOut - filter->prevprevprevOut) * 0.5f;

	return (( ((a*posFract)+b) * posFract + c ) * posFract) + filter->prevprevOut;
}



# 20 "transverb.hpp" 2




 
 
enum
{
	kBsize,
	kSpeed1,
	kFeed1,
	kDist1,
	kSpeed2,
	kFeed2,
	kDist2,
	kDrymix,
	kMix1,
	kMix2,
	kQuality,
	kTomsound,
	kSpeed1mode,
	kSpeed2mode,

	NUM_PARAMS
};










  
  
















 









 
 


 
enum { kFineMode, kSemitoneMode, kOctaveMode, numSpeedModes };




const float RAND_MAX_FLOAT = (float) 2147483647 ;	 


enum { dirtfi, hifi, ultrahifi, numQualities };

enum { useNothing, useHighpass, useLowpassIIR, useLowpassFIR, numFilterModes };

struct param {
  float * ptr;
  const char * name;
  const char * units;
};


class Transverb  : public flext_dsp {

  public: typedef   Transverb    thisType; static void callb_free(flext_hdr *hdr) { flext_obj *mydata = ((flext_hdr *)hdr)->data; delete mydata; ((flext_hdr *)hdr)->flext_hdr::~flext_hdr(); } static void callb_setup(struct _class  *classPtr) {     flext_dsp  ::callb_setup(classPtr); } protected: static   Transverb    *thisObject(void *c) { return (  Transverb    *)((flext_hdr *)c)->data; }  
  
  virtual void processX(float **inputs, float **outputs, long sampleFrames,
			int replacing);
  virtual void process(float **inputs, float **outputs, long sampleFrames);
  virtual void processReplacing(float **inputs, float **outputs, 
				long sampleFrames);

 
 
 
 
 

 

  virtual void suspend();
  virtual void resume();

 
  void randomizeParameters(bool writeAutomation = false);

  float fBsize;	 

public:
  Transverb (int argc, t_atom *argv);
  ~Transverb ();

	virtual void m_help()
	{
		post("");
		post("_ _____transverb~ help___ _");
		post(" f : mix         1");
		post(" f : speed       1");
		post(" f : feed        1");
		post(" f : distortion  1");
		post(" f : mix         2");
		post(" f : speed       2");
		post(" f : feed        2");
		post(" f : distortion  2");
		post(" f : dry/wet ratio");
		post(" f : quality");
		post("");
	}



protected:

  void initPresets();
  void createAudioBuffers();
  void clearBuffers();

  virtual void m_signal(int n, float *const *in, float *const *out);

  float drymix;
  int bsize;
  float mix1, speed1, feed1, dist1;
  float mix2, speed2, feed2, dist2;
  float fQuality, fTomsound;
  long quality;
  bool tomsound;

  int writer;
  double read1, read2;
  
  int sr; int blocksize;

  float * buf1[2];
  float * buf2[2];
  int MAXBUF;	 

  IIRfilter *filter1, *filter2;
  bool speed1hasChanged, speed2hasChanged;
  float fSpeed1mode, fSpeed2mode;

  int smoothcount1[2], smoothcount2[2], smoothdur1[2], smoothdur2[2];
  float smoothstep1[2], smoothstep2[2], lastr1val[2], lastr2val[2];

  float SAMPLERATE;

  float *firCoefficients1, *firCoefficients2;


	 
	static void cb_setMix  (flext_base *c, float  &arg1) { static_cast<thisType *>(c)->  setMix  (arg1); }  
	void setMix(float f) {
		drymix = (( f )*( f )) ;
	}

	static void cb_setBsize  (flext_base *c, float  &arg1) { static_cast<thisType *>(c)->  setBsize  (arg1); }  
	void setBsize(float f) {
		bsize = ( ((int)(( ( (( (  (int)f  ) ) * ((  3000.0f  )-(  1.0f  ))) + (  1.0f  ) )  ) *SAMPLERATE*0.001f) > MAXBUF) ? MAXBUF : (int)(( ( (( (  (int)f  ) ) * ((  3000.0f  )-(  1.0f  ))) + (  1.0f  ) )  ) *SAMPLERATE*0.001f) ) ;
		writer %= bsize;
		read1 = fmod(fabs(read1), (double)bsize);
		read2 = fmod(fabs(read2), (double)bsize);
	}

	static void cb_setMix1  (flext_base *c, float  &arg1) { static_cast<thisType *>(c)->  setMix1  (arg1); }  
	void setMix1(float f) {
		mix1 = (( f )*( f )) ;
	}

	static void cb_setSpeed1  (flext_base *c, float  &arg1) { static_cast<thisType *>(c)->  setSpeed1  (arg1); }  
	void setSpeed1(float f) {
		speed1 = powf(2.0f, ( ( (( ( f ) ) * ((  6.0f  )-(  (-3.0f)  ))) + (  (-3.0f)  ) )  ) );
		speed1hasChanged = true;
	}

	static void cb_setFeed1  (flext_base *c, float  &arg1) { static_cast<thisType *>(c)->  setFeed1  (arg1); }  
	void setFeed1(float f) {
		feed1 = f;
	}

	static void cb_setDist1  (flext_base *c, float  &arg1) { static_cast<thisType *>(c)->  setDist1  (arg1); }  
	void setDist1(float f) {
		dist1 = f;
		read1 = fmod(fabs((double)writer + (double)dist1 *
				  (double)MAXBUF), (double)bsize);
	}

	static void cb_setMix2  (flext_base *c, float  &arg1) { static_cast<thisType *>(c)->  setMix2  (arg1); }  
	void setMix2(float f) {
		mix2 = (( f )*( f )) ;
	}

	static void cb_setSpeed2  (flext_base *c, float  &arg1) { static_cast<thisType *>(c)->  setSpeed2  (arg1); }  
	void setSpeed2(float f) {
		speed2 = powf(2.0f, ( ( (( ( f ) ) * ((  6.0f  )-(  (-3.0f)  ))) + (  (-3.0f)  ) )  ) );
		speed2hasChanged = true;
	}

	static void cb_setFeed2  (flext_base *c, float  &arg1) { static_cast<thisType *>(c)->  setFeed2  (arg1); }  
	void setFeed2(float f) {
		feed2 = f;
	}

	static void cb_setDist2  (flext_base *c, float  &arg1) { static_cast<thisType *>(c)->  setDist2  (arg1); }  
	void setDist2(float f) {
		dist2 = f;
		read2 = fmod(fabs((double)writer + (double)dist2 *
				  (double)MAXBUF), (double)bsize);
	}
	

	static void cb_setQuality  (flext_base *c, float  &arg1) { static_cast<thisType *>(c)->  setQuality  (arg1); }  
	void setQuality(float f) {
		fQuality = ( ( (long)(( ( f ) ) * ((float)(  numQualities )-0.01f)) )  ) ;
	}

	static void cb_setTom  (flext_base *c, float  &arg1) { static_cast<thisType *>(c)->  setTom  (arg1); }  
	void setTom(float f) {
		fTomsound = (f > 0.5f);
	}

};





inline float interpolateHermite (float *data, double address, 
				 int arraysize, int danger) {
  int pos, posMinus1, posPlus1, posPlus2;
  float posFract, a, b, c;

  pos = (long)address;
  posFract = (float) (address - (double)pos);

   
   
   
  switch (danger) {
    case 0:		 
      posMinus1 = pos;
      posPlus1 = (pos+1) % arraysize;
      posPlus2 = (pos+2) % arraysize;
      break;
    case 1:		 
      posMinus1 = (pos == 0) ? arraysize-1 : pos-1;
      posPlus1 = posPlus2 = pos;
      break;
    case 2:		 
      posMinus1 = (pos == 0) ? arraysize-1 : pos-1;
      posPlus1 = posPlus2 = (pos+1) % arraysize;
      break;
    default:	 
      posMinus1 = (pos == 0) ? arraysize-1 : pos-1;
      posPlus1 = (pos+1) % arraysize;
      posPlus2 = (pos+2) % arraysize;
      break;
    }

  a = ( (3.0f*(data[pos]-data[posPlus1])) - 
	 data[posMinus1] + data[posPlus2] ) * 0.5f;
  b = (2.0f*data[posPlus1]) + data[posMinus1] - 
         (2.5f*data[pos]) - (data[posPlus2]*0.5f);
  c = (data[posPlus1] - data[posMinus1]) * 0.5f;

  return ( ((a*posFract)+b) * posFract + c ) * posFract + data[pos];
}
 

















inline float interpolateLinear(float *data, double address, 
				int arraysize, int danger) {
	int posPlus1, pos = (long)address;
	float posFract = (float) (address - (double)pos);

	if (danger == 1) {
	   

	  posPlus1 = pos;
	} else {
	   
	  posPlus1 = (pos + 1) % arraysize;
	}
	return (data[pos] * (1.0f-posFract)) + 
	       (data[posPlus1] * posFract);
}




# 10 "transverb.cpp" 2

# 1 "../dfx-library/FIRfilter.h" 1










 
void calculateFIRidealLowpassCoefficients(float cutoff, float samplerate, 
											int numTaps, float *coefficients);
void applyKaiserWindow(int numTaps, float *coefficients, float attenuation);
float besselIzero(float in);
float besselIzero2(float in);

 
inline float processFIRfilter(float *in, int numTaps, float *coefficients, 
								long inPos, long arraySize)
{
	float out = 0.0f;
	if ( (inPos+numTaps) > arraySize )
	{
		for (long i=0; i < numTaps; i++)
			out += in[(inPos+i)%arraySize] * coefficients[i];
	}
	else
	{
		for (long i=0; i < numTaps; i++)
			out += in[inPos+i] * coefficients[i];
	}
	return out;
} 



# 11 "transverb.cpp" 2



 
static struct _class  *     Transverb_class; flext_hdr* class_Transverb     (t_symbol *s,int argc,t_atom *argv) { flext_hdr *obj = new (pd_new(     Transverb_class ) ,(void *)__null ) flext_hdr; flext_obj::m_holder = obj; flext_obj::m_holdname =    "transverb~"   ; obj->data = new     Transverb    (argc,argv); flext_obj::m_holder = __null ; return(obj); } extern "C"       void     Transverb_tilde_setup  () { ((void)0) ;     Transverb_class = ::class_new ( gensym(    "transverb~"    ) , (t_newmethod)class_Transverb    ,	(t_method)&    Transverb    ::callb_free, sizeof(flext_hdr), 0 , A_GIMME, A_NULL);     Transverb    ::callb_setup(    Transverb_class); }     

 

Transverb :: Transverb (int argc, t_atom *argv)  {

  do {  fBsize  = (  (2700.0f- 1.0f )/(3000.0f - 1.0f ) ); } while (0) ;
  do {  drymix  = (  1.0f ); } while (0) ;
  do {  mix1  = (  1.0f ); } while (0) ;
  do {  dist1  = (  0.90009f ); } while (0) ;
  do {  speed1  = (  (0.0f- (-3.0f) )/(6.0f - (-3.0f) ) ); } while (0) ;
  do {  feed1  = (  0.0f ); } while (0) ;
  do {  mix2  = (  0.0f ); } while (0) ;
  do {  dist2  = (  0.1f ); } while (0) ;
  do {  speed2  = (  (1.0f- (-3.0f) )/(6.0f - (-3.0f) ) ); } while (0) ;
  do {  feed2  = (  0.0f ); } while (0) ;
  do {  fQuality  = (  1.0f ); } while (0) ;
  do {  fTomsound  = (  0.0f ); } while (0) ;
  do {  fSpeed1mode  = (  0.0f ); } while (0) ;
  do {  fSpeed2mode  = (  0.0f ); } while (0) ;

   
  MAXBUF = (int) (3000.0f  * 44.1f);
  buf1[0] = __null ;
  buf2[0] = __null ;

  buf1[1] = __null ;
  buf2[1] = __null ;

  filter1 = new IIRfilter[2 ];
  filter2 = new IIRfilter[2 ];
  firCoefficients1 = new float[23 ];
  firCoefficients2 = new float[23 ];
  suspend();
  srand((unsigned int)time(__null ));	 

	post("_ ____transverb~ with flext");

	sr = (int) Samplerate();
	blocksize = Blocksize(); 
	
	
	 
	 
	 
	
	AddInSignal(2);
 
	AddInFloat(12);
	AddOutSignal(2);          
	
	SetupInOut();            
				 

	 
	 
	AddMethod(  0 ,cb_setMix ) ;
	AddMethod(  1 ,cb_setBsize ) ;
	AddMethod(  2 ,cb_setMix1 ) ;
	AddMethod(  3 ,cb_setSpeed1 ) ;
	AddMethod(  4 ,cb_setFeed1 ) ;
	AddMethod(  5 ,cb_setDist1 ) ;
	AddMethod(  6 ,cb_setMix2 ) ;
	AddMethod(  7 ,cb_setSpeed2 ) ;
	AddMethod(  8 ,cb_setFeed2 ) ;
	AddMethod(  9 ,cb_setDist2 ) ;
	AddMethod( 10 ,cb_setQuality ) ;
	AddMethod( 11 ,cb_setTom ) ;
	
	 
	post("_ ____ ____ _");
	
}  
		

Transverb ::~Transverb () {

  if (buf1[0])
    free(buf1[0]);
  if (buf2[0])
    free(buf2[0]);

  if (buf1[1])
    free(buf1[1]);
  if (buf2[1])
    free(buf2[1]);

  if (filter1)
    delete[] filter1;
  if (filter2)
    delete[] filter2;
  if (firCoefficients1)
    delete[] firCoefficients1;
  if (firCoefficients2)
    delete[] firCoefficients2;

}

void Transverb ::suspend () {

  clearBuffers();
  writer = 0;
  read1 = read2 = 0.0;
  smoothcount1[0] = smoothcount2[0] = 0;
  lastr1val[0] = lastr2val[0] = 0.0f;
  filter1[0].reset();
  filter2[0].reset();

  smoothcount1[1] = smoothcount2[1] = 0;
  lastr1val[1] = lastr2val[1] = 0.0f;
  filter1[1].reset();
  filter2[1].reset();

  SAMPLERATE = 44100.0f;
  bsize = ( ((int)(( ( (( (  fBsize  ) ) * ((  3000.0f  )-(  1.0f  ))) + (  1.0f  ) )  ) *SAMPLERATE*0.001f) > MAXBUF) ? MAXBUF : (int)(( ( (( (  fBsize  ) ) * ((  3000.0f  )-(  1.0f  ))) + (  1.0f  ) )  ) *SAMPLERATE*0.001f) ) ;
  speed1hasChanged = speed2hasChanged = true;
}

void Transverb ::resume() {

  createAudioBuffers();
  bsize = ( ((int)(( ( (( (  fBsize  ) ) * ((  3000.0f  )-(  1.0f  ))) + (  1.0f  ) )  ) *SAMPLERATE*0.001f) > MAXBUF) ? MAXBUF : (int)(( ( (( (  fBsize  ) ) * ((  3000.0f  )-(  1.0f  ))) + (  1.0f  ) )  ) *SAMPLERATE*0.001f) ) ;
}

void Transverb ::createAudioBuffers() {

  SAMPLERATE = 44100.0f;

  long oldmax = MAXBUF;
  MAXBUF = (int) (3000.0f  * 0.001f * SAMPLERATE);

   
   
  if (MAXBUF != oldmax)
  {
    if (buf1[0] != __null )
      free(buf1[0]);
    buf1[0] = __null ;
    if (buf2[0] != __null )
      free(buf2[0]);
    buf2[0] = __null ;

    if (buf1[1] != __null )
      free(buf1[1]);
    buf1[1] = __null ;
    if (buf2[1] != __null )
      free(buf2[1]);
    buf2[1] = __null ;

  }
  if (buf1[0] == __null )
    buf1[0] = (float*)malloc(MAXBUF * sizeof (float));
  if (buf2[0] == __null )
    buf2[0] = (float*)malloc(MAXBUF * sizeof (float));

  if (buf1[1] == __null )
    buf1[1] = (float*)malloc(MAXBUF * sizeof (float));
  if (buf2[1] == __null )
    buf2[1] = (float*)malloc(MAXBUF * sizeof (float));

}

void Transverb ::clearBuffers() {
  if ( (buf1[0] != __null ) && (buf2[0] != __null ) ) {
    for (int j=0; j < MAXBUF; j++) buf1[0][j] = buf2[0][j] = 0.0f;
  }

  if ( (buf1[1] != __null ) && (buf2[1] != __null ) ) {
    for (int k=0; k < MAXBUF; k++) buf1[1][k] = buf2[1][k] = 0.0f;
  }

}


 
void Transverb ::randomizeParameters(bool writeAutomation)
{
 
































































}

void Transverb ::m_signal(int n, float *const *in, float *const *out)
{
	float *outs          = out[0];	
	
	 
	 
	
	int i = 0;
	while (n--) {
		processX((float **)in,(float **)out,(long int)n,0);
 
	}
}   


void Transverb ::processReplacing(float **inputs, float **outputs, long samples) {
  processX(inputs,outputs,samples, 1);
}

void Transverb ::process(float **inputs, float **outputs, long samples) {
  processX(inputs,outputs,samples, 0);
}















void Transverb ::processX(float **in, float **outputs, long samples, 
		      int replacing) {
   
  int writertemp;
  double read1temp, read2temp;
   
  int speed1int, speed2int, read1int, read2int;
  int lowpass1pos, lowpass2pos;	 
  float r1val, r2val;	 
  double bsize_float = (double)bsize;	 
  int filterMode1, filterMode2;	 
  float mug1, mug2;	 
  float quietNoise = 1.0e-15f;	 


   
   
  if ( ((buf1[0] == __null ) || (buf2[0] == __null )) 
  
       || ((buf1[1] == __null ) || (buf2[1] == __null )) 
  
     ) createAudioBuffers();
    
   
  if ( (buf1[0] == __null ) || (buf2[0] == __null ) )
    return;

  if ( (buf1[1] == __null ) || (buf2[1] == __null ) )
    return;


  SAMPLERATE = 44100.0f;

  filterMode1 = filterMode2 = useNothing;	 
  if (quality == ultrahifi)
  {
     
    if (speed1 > 1.0f)
    {
      filterMode1 = useLowpassIIR;
      speed1int = (int)speed1;
       
      if (speed1int >= 5)
      {
        filterMode1 = useLowpassFIR;
        mug1 = powf( (speed1*0.2f), 0.78f );	 
         
        if (speed1hasChanged)
        {
          calculateFIRidealLowpassCoefficients((SAMPLERATE/speed1)* 0.333f , SAMPLERATE, 23 , firCoefficients1);
          applyKaiserWindow(23 , firCoefficients1, 60.0f);
          speed1hasChanged = false;
        }
      }
      else if (speed1hasChanged)
      {
        filter1[0].calculateLowpassCoefficients((SAMPLERATE/speed1)* 0.333f , SAMPLERATE);
      
        filter1[1].copyCoefficients(filter1);
      
        speed1hasChanged = false;
      }
    }
     
    else
    {
      filterMode1 = useHighpass;
      if (speed1hasChanged)
      {
        filter1[0].calculateHighpassCoefficients(33.3f/speed1, SAMPLERATE);
      
        filter1[1].copyCoefficients(filter1);
      
        speed1hasChanged = false;
      }
    }

     
    if (speed2 > 1.0f)
    {
      filterMode2 = useLowpassIIR;
      speed2int = (int)speed2;
      if (speed2int >= 5)
      {
        filterMode2 = useLowpassFIR;
        mug2 = powf( (speed2*0.2f), 0.78f );	 
        if (speed2hasChanged)
        {
          calculateFIRidealLowpassCoefficients((SAMPLERATE/speed2)* 0.333f , SAMPLERATE, 23 , firCoefficients2);
          applyKaiserWindow(23 , firCoefficients2, 60.0f);
          speed2hasChanged = false;
        }
      }
      else if (speed2hasChanged)
      {
        filter2[0].calculateLowpassCoefficients((SAMPLERATE/speed2)* 0.333f , SAMPLERATE);
      
        filter2[1].copyCoefficients(filter2);
      
        speed2hasChanged = false;
      }
    }
     
    else
    {
      filterMode2 = useHighpass;
      if (speed2hasChanged)
      {
        filter2[0].calculateHighpassCoefficients(33.3f/speed2, SAMPLERATE);
      
        filter2[1].copyCoefficients(filter2);
      
        speed2hasChanged = false;
      }
    }
  }

   
   
  if (!tomsound) {
     
    read1temp = read1;
    read2temp = read2;
    writertemp = writer;


    for(int i=0; i < 2 ; i++) {	 
      lowpass1pos = (int)read1;
      lowpass2pos = (int)read2;

      for(long j=0; j < samples; j++) {	 

        read1int = (int)read1;
        read2int = (int)read2;

         
        switch(quality)
        {
           
          case dirtfi:
            r1val = buf1[i][read1int];
            r2val = buf2[i][read2int];
            break;
           
          case hifi:
 
            r1val = interpolateHermite(buf1[i], read1, bsize, writer-read1int);
            r2val = interpolateHermite(buf2[i], read2, bsize, writer-read2int);
            break;
           
           
          case ultrahifi:
            float lp1, lp2;
            switch (filterMode1)
            {
              case useHighpass:
              case useLowpassIIR:
                 
                r1val = interpolateHermitePostFilter(&filter1[i], read1);
                break;
              case useLowpassFIR:
                 
                lp1 = processFIRfilter(buf1[i], 23 , firCoefficients1, 
                                       (read1int- 23 +bsize)%bsize, bsize);
                lp2 = processFIRfilter(buf1[i], 23 , firCoefficients1, 
                                       (read1int- 23 +1+bsize)%bsize, bsize);
                 
                r1val = interpolateLinear2values(lp1, lp2, read1) * mug1;
                break;
              default:
                r1val = interpolateHermite(buf1[i], read1, bsize, writer-read1int);
                break;
            }
            switch (filterMode2)
            {
              case useHighpass:
              case useLowpassIIR:
                 
                r2val = interpolateHermitePostFilter(&filter2[i], read2);
                break;
              case useLowpassFIR:
                 
                lp1 = processFIRfilter(buf2[i], 23 , firCoefficients2, 
                                       (read2int- 23 +bsize)%bsize, bsize);
                lp2 = processFIRfilter(buf2[i], 23 , firCoefficients2, 
                                       (read2int- 23 +1+bsize)%bsize, bsize);
                 
                r2val = interpolateLinear2values(lp1, lp2, read2) * mug2;
                break;
              default:
                r2val = interpolateHermite(buf2[i], read2, bsize, writer-read2int);
                break;
            }
            break;
           
          default:
            r1val = buf1[i][read1int];
            r2val = buf2[i][read2int];
            break;
        }	 

         
         
        if (smoothcount1[i]) {
          r1val = ( r1val * (1.0f - (smoothstep1[i]*(float)smoothcount1[i])) ) 
                  + (lastr1val[i] * smoothstep1[i]*(float)smoothcount1[i]);
          (smoothcount1[i])--;
        }
        if (smoothcount2[i]) {
          r2val = ( r2val * (1.0f - (smoothstep2[i]*(float)smoothcount2[i])) ) 
                  + (lastr2val[i] * smoothstep2[i]*(float)smoothcount2[i]);
          (smoothcount2[i])--;
        }
        
         

         
         
 



        buf1[i][writer] = in[i][j] + (feed1 * r1val * mix1);
        buf2[i][writer] = in[i][j] + (feed2 * r2val * mix2);
        if (fabs( buf1[i][writer] ) < 1.0e-15)    buf1[i][writer]  = 0.0 ;
        if (fabs( buf2[i][writer] ) < 1.0e-15)    buf2[i][writer]  = 0.0 ;

         
        if (replacing) 
          outputs[i][j] = (in[i][j]*drymix) + (r1val*mix1) + (r2val*mix2);
        else
          outputs[i][j] += (in[i][j]*drymix) + (r1val*mix1) + (r2val*mix2);

         




        if ( ( (read1int < writer) && 
               (((int)(read1+(double)speed1)) >= (writer+1)) ) || 
             ( (read1int >= writer) && 
               (((int)(read1+(double)speed1)) <= (writer+1)) ) ) {
         

          if (smoothcount1[i] <= 0) {
             
            lastr1val[i] = r1val;
             
            smoothdur1[i] = 
              (42  > (int)(bsize_float/(double)speed1)) ? 
              (int)(bsize_float/(double)speed1) : 42 ;
            smoothstep1[i] = 1.0f / (float)smoothdur1[i];	 
            smoothcount1[i] = smoothdur1[i];	 
          }
        }

         
        if ( ( (read2int < writer) && 
               (((int)(read2+(double)speed2)) >= (writer+1)) ) || 
             ( (read2int >= writer) && 
               (((int)(read2+(double)speed2)) <= (writer+1)) ) ) {
          if (smoothcount2[i] <= 0) {
             
            lastr2val[i] = r2val;
             
            smoothdur2[i] = 
              (42  > (int)(bsize_float/(double)speed2)) ? 
              (int)(bsize_float/(double)speed2) : 42 ;
            smoothstep2[i] = 1.0f / (float)smoothdur2[i];	 
            smoothcount2[i] = smoothdur2[i];	 
          }
        }

         
        writer++;
        read1 += (double)speed1;
        read2 += (double)speed2;

         
        writer %= bsize;
        if (read1 >= bsize_float)
          read1 = fmod(fabs(read1), bsize_float);
        if (read2 >= bsize_float)
          read2 = fmod(fabs(read2), bsize_float);

         
         
         
         
        if (filterMode1 == useLowpassIIR)
        {
          int lowpasscount = 0;
          while (lowpasscount < speed1int)
          {
            switch (speed1int - lowpasscount)
            {
              case 1:
                filter1[i].processH1(buf1[i][lowpass1pos]);
                lowpass1pos = (lowpass1pos + 1) % bsize;
                lowpasscount++;
                break;
              case 2:
                filter1[i].processH2(buf1[i], lowpass1pos, bsize);
                lowpass1pos = (lowpass1pos + 2) % bsize;
                lowpasscount += 2;
                break;
              case 3:
                filter1[i].processH3(buf1[i], lowpass1pos, bsize);
                lowpass1pos = (lowpass1pos + 3) % bsize;
                lowpasscount += 3;
                break;
              default:
                filter1[i].processH4(buf1[i], lowpass1pos, bsize);
                lowpass1pos = (lowpass1pos + 4) % bsize;
                lowpasscount += 4;
                break;
            }
          }
          read1int = (int)read1;
           
          if ( ((lowpass1pos < read1int) && ((lowpass1pos+1) == read1int)) ||
               ((lowpass1pos == (bsize-1)) && (read1int == 0)) )
          {
            filter1[i].processH1(buf1[i][lowpass1pos]);
            lowpass1pos = (lowpass1pos+1) % bsize;
          }
        }
         
         
        else if (filterMode1 == useHighpass)
        {
           
          if ((int)read1 != read1int)
            filter1[i].process(buf1[i][read1int]);
        }

         
        if (filterMode2 == useLowpassIIR)
        {
          int lowpasscount = 0;
          while (lowpasscount < speed2int)
          {
            switch (speed2int - lowpasscount)
            {
              case 1:
                filter2[i].processH1(buf2[i][lowpass2pos]);
                lowpass2pos = (lowpass2pos + 1) % bsize;
                lowpasscount++;
                break;
              case 2:
                filter2[i].processH2(buf2[i], lowpass2pos, bsize);
                lowpass2pos = (lowpass2pos + 2) % bsize;
                lowpasscount += 2;
                break;
              case 3:
                filter2[i].processH3(buf2[i], lowpass2pos, bsize);
                lowpass2pos = (lowpass2pos + 3) % bsize;
                lowpasscount += 3;
                break;
              default:
                filter2[i].processH4(buf2[i], lowpass2pos, bsize);
                lowpass2pos = (lowpass2pos + 4) % bsize;
                lowpasscount += 4;
                break;
            }
          }
          read2int = (int)read2;
          if ( ((lowpass2pos < read2int) && ((lowpass2pos+1) == read2int)) ||
               ((lowpass2pos == (bsize-1)) && (read2int == 0)) )
          {
            filter2[i].processH1(buf2[i][lowpass2pos]);
            lowpass2pos = (lowpass2pos+1) % bsize;
          }
        }
        else if (filterMode2 == useHighpass)
        {
          if ((int)read2 != read2int)
            filter2[i].process(buf2[i][read2int]);
        }
      }	 

    
      if (i == 0) {
         
        read1 = read1temp;
        read2 = read2temp;
        writer = writertemp;
      }
    
    }	 
  }	 




   
  else {
      for(long j=0; j < samples; j++) {
        for(int i=0; i < 2 ; i++) {

	   

	  switch(quality) {
	    case dirtfi:
	      r1val = mix1 * buf1[i][(int)read1];
	      r2val = mix2 * buf1[i][(int)read2];
	      break;
	    case hifi:
	    case ultrahifi:
	      r1val = mix1 * interpolateHermite(buf1[i], read1, bsize, 333);
	      r2val = mix2 * interpolateHermite(buf1[i], read2, bsize, 333);
	      break;
	    default:
	      r1val = mix1 * buf1[i][(int)read1];
	      r2val = mix2 * buf1[i][(int)read2];
	      break;
	    }

	   

	  buf1[i][writer] = 
	    in[i][j] + 
	    feed1 * r1val + 
	    feed2 * r2val;
      
	   
	  writer++;
	  writer %= bsize;

	  read1 += (double)speed1;
	  read2 += (double)speed2;

	  if (read1 >= bsize_float)
	    read1 = fmod(fabs(read1), bsize_float);
	  if (read2 >= bsize_float)
	    read2 = fmod(fabs(read2), bsize_float);

	   
	  if (replacing) 
	    outputs[i][j] = in[i][j] * drymix + r1val + r2val;
	  else
	    outputs[i][j] += in[i][j] * drymix + r1val + r2val;
        }
      }
    }

}
