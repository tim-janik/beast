/* TOYPROF - Poor man's profiling toy
 * Copyright (C) 2001 Tim Janik
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef	__TOYPROF_H__
#define __TOYPROF_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if	defined (__GNUC__) && __GNUC__ >= 2 && __GNUC_MINOR__ >= 95


/* function flagging */
#define	TOYPROF_GNUC_NO_INSTRUMENT	__attribute__((no_instrument_function))

/* print out profiling statistics to filedescriptor */
void toyprof_dump_stats	(int fd) TOYPROF_GNUC_NO_INSTRUMENT;

/* profiling types */
typedef enum {
  TOYPROF_OFF			= 0x0,
  TOYPROF_PROFILE_TIMING	= 0x1,
  TOYPROF_TRACE_FUNCTIONS	= 0x3
} ToyprofBehaviour;

/* enable/disable profiling and optionally trace functions */
void toyprof_set_profiling (ToyprofBehaviour behav) TOYPROF_GNUC_NO_INSTRUMENT;


#endif	/* GCC-2.95 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* __TOYPROF_H__ */
