/*
 *
 * Copyright (c) 2011, 2012
 *   University of Houston System and Oak Ridge National Laboratory.
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * o Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * 
 * o Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 
 * o Neither the name of the University of Houston System, Oak Ridge
 *   National Laboratory nor the names of its contributors may be used to
 *   endorse or promote products derived from this software without specific
 *   prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */



#include <stdio.h>
#include <sys/param.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

#define __USE_BSD 1
#include <unistd.h>

#include <sys/file.h>

#include "trace.h"
#include "state.h"

static const char *self = "/proc/self/exe";
static const char *self_fmt = "/proc/%ld/exe";

/*
 * inspect our own executable: find the real program name, and open a
 * file descriptor
 */
void
__shmem_executable_init (void)
{
  ssize_t s;
  int fd;

  /* see if the shortcut works */
  s = readlink (self, GET_STATE (exe_name), MAXPATHLEN);

  /* if not, try finding our PID */
  if (s < 0)
    {
      char buf[MAXPATHLEN];
      snprintf (buf, MAXPATHLEN, self_fmt, getpid ());
      s = readlink (buf, GET_STATE (exe_name), MAXPATHLEN);
    }

  /* dunno who I am, complain */
  if (s < 0)
    {
      __shmem_trace (SHMEM_LOG_FATAL,
		     "can't find my own executable name (%s)",
		     strerror (errno));
      /* NOT REACHED */
    }

  /* bleagh, readlink doesn't null-terminate */
  GET_STATE (exe_name)[s] = '\0';

  /* get a file descriptor */
  fd = open (GET_STATE (exe_name), O_RDONLY, 0);
  if (fd < 0)
    {
      __shmem_trace (SHMEM_LOG_FATAL,
		     "can't open \"%s\" (%s)",
		     GET_STATE (exe_name), strerror (errno));
      /* NOT REACHED */
    }

  SET_STATE (exe_fd, fd);
}

/*
 * shut down the executable inspection: close the opened file
 * descriptor
 */
void
__shmem_executable_finalize (void)
{
  close (GET_STATE (exe_fd));
}
