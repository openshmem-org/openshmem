#include <stdio.h>               /* NULL                           */
#include <stdlib.h>              /* atexit()                       */
#include <sys/utsname.h>         /* uname()                        */
#include <string.h>              /* strdup()                       */
#include <sys/types.h>           /* size_t                         */

#include "comms.h"
#include "state.h"
#include "trace.h"
#include "atomic.h"
#include "barrier.h"
#include "broadcast.h"
#include "ping.h"
#include "utils.h"
#include "service.h"
#include "clock.h"

#include "shmem.h"

/* ----------------------------------------------------------------- */

/*
 * shut down shmem, and then hand off to the comms layer to shut
 * itself down
 *
 */

void
__shmem_exit(int status)
{
  __shmem_service_thread_finalize();

  __shmem_atomic_finalize();

  __shmem_symmetric_memory_finalize();

  SET_STATE(pe_status, PE_SHUTDOWN);

  __shmem_trace(SHMEM_LOG_INIT,
		"finalizing shutdown, handing off to communications layer"
		);

  /*
   * strictly speaking should free alloc'ed things,
   * but exit is immediately next, so everything gets reaped anyway...
   */
  __shmem_comms_finalize(status);
}

/*
 * registered by start_pes() to trigger shut down at exit
 *
 */

static void
__shmem_exit_handler(void)
{
  __shmem_exit(0);
}

/*
 * find the short & (potentially) long host/node name
 *
 */
static void
__shmem_place_init(void)
{
  int s;

  s = uname(& GET_STATE(loc));
  if (s != 0) {
    __shmem_trace(SHMEM_LOG_FATAL,
		  "can't find any node information"
		  );
  }
}

/*
 * this is where we get everything up and running
 *
 */

/* @api@ */
void
pstart_pes(int npes)
{
  /* start the tracking clock */
  __shmem_elapsed_clock_init();

  /* has to happen early to enable messages */
  __shmem_tracers_init();

  /* I shouldn't really call this more than once */
  if (GET_STATE(pe_status) != PE_UNINITIALIZED) {
    __shmem_trace(SHMEM_LOG_FATAL,
		  "shmem has already been initialized (%s)",
		  __shmem_state_as_string(GET_STATE(pe_status))
		  );
    /* NOT REACHED */
  }

  /* set up communications layer */
  __shmem_comms_init();

  /* start network service thread */
  __shmem_service_thread_init();

  /* see if we want to say which message/trace levels are active */
  __shmem_maybe_tracers_show_info();
  __shmem_tracers_show();

  /* set up any locality information */
  __shmem_place_init();

  /* set up PE memory management */
  __shmem_symmetric_memory_init();

  /* set up the atomic ops handling */
  __shmem_atomic_init();

  /* set up timeouts */
  __shmem_ping_init();

  /* set up barrier selection */
  __shmem_barrier_dispatch_init();
  __shmem_broadcast_dispatch_init();

  if (atexit(__shmem_exit_handler) != 0) {
    __shmem_trace(SHMEM_LOG_FATAL,
		  "internal error: cannot register shutdown handler"
		  );
    /* NOT REACHED */
  }

  /*
   * SGI man page says npes *should* be zero, so just make
   * this check informational
   */
  if (npes != 0) {
    __shmem_trace(SHMEM_LOG_INFO,
		  "start_pes() was passed %d, should be 0",
		  npes
		  );
  }

  /*
   * and we're up and running
   */

  SET_STATE(pe_status, PE_RUNNING);

  {
    int ma, mi;
    if (shmem_version(&ma, &mi) == 0) {
      __shmem_trace(SHMEM_LOG_VERSION,
		    "version %d.%d running on %d PE%s",
		    ma, mi,
		    GET_STATE(numpes),
		    GET_STATE(numpes) == 1 ? "" : "s"
		    );
    }
  }
}

/*
 * same as shmem_init()
 */
/* @api@ */
void
pshmem_init(void)
{
  pstart_pes(0);
}

/*
 * does nothing here (just for compatibility with other shmems)
 */
/* @api@ */
void
pshmem_finalize(void)
{
  INIT_CHECK();
}

#pragma weak start_pes = pstart_pes
#pragma weak shmem_init = pshmem_init
#pragma weak shmem_finalize = pshmem_finalize
