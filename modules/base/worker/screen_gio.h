#include <stdint.h>
#if mxLinux
#include <glib.h>
#endif

#if mxLinux
	typedef GCond txCondition;
	typedef GMutex txMutex;
	typedef GThread* txThread;
	#define mxCreateCondition(CONDITION) g_cond_init(CONDITION)
	#define mxCreateMutex(MUTEX) g_mutex_init(MUTEX)
	#define mxCurrentThread() g_thread_self()
	#define mxDeleteCondition(CONDITION) g_cond_clear(CONDITION)
	#define mxDeleteMutex(MUTEX) g_mutex_clear(MUTEX)
	#define mxLockMutex(MUTEX) g_mutex_lock(MUTEX)
	#define mxSignalCondition(CONDITION) g_cond_signal(CONDITION)
	#define mxUnlockMutex(MUTEX) g_mutex_unlock(MUTEX)
	#define mxWaitCondition(CONDITION,MUTEX) g_cond_wait(CONDITION,MUTEX)
#endif
