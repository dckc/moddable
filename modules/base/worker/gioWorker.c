#include "mc.xs.h"
#include "screen_gio.h"

typedef struct sxWorker txWorker;
typedef struct sxWorkerMessageJob txWorkerMessageJob;

struct sxWorker {
	xsMachine* machine;
	xsSlot reference;
	xsMachine* ownerMachine;
	xsSlot ownerReference;
	txWorker* nextWorker;
	xsBooleanValue running;
	txCondition runningCondition;
	txMutex runningMutex;
#if mxLinux
	GMainLoop* main_loop;
		#else
                        #error
#endif
	char name[1];
};

struct sxWorkerMessageJob {
	txWorkerJob* next;
	txWorkerCallback callback;
	xsSlot* reference;
	void* argument;
};

#ifdef mxInstrument	
extern void fxDescribeInstrumentation(xsMachine* the, xsIntegerValue count, xsStringValue* names, xsStringValue* units);
extern void fxSampleInstrumentation(xsMachine* the, xsIntegerValue count, xsIntegerValue* values);
#endif

static void fxWorkerInitialize(txWorker* worker);
static void fxWorkerMessage(void* machine, void* job);
static void fxWorkerMessageAlien(void* machine, void* job);
static void fxWorkerTerminate(txWorker* worker);

static void xs_worker_postfromworker(xsMachine *the);

#if mxLinux
static gpointer fxWorkerLoop(gpointer it)
{
	txWorker* worker = it;
	GMainContext* main_context = g_main_context_new();
	g_main_context_push_thread_default(main_context);
	fxWorkerInitialize(worker);
	worker->main_loop = g_main_loop_new(main_context, FALSE);
	g_main_loop_run(worker->main_loop);
	g_main_loop_unref(worker->main_loop);
	worker->main_loop = NULL;
	fxWorkerTerminate(worker);
	g_main_context_pop_thread_default(main_context);
	g_main_context_unref(main_context);
	return NULL;
}
		#else
                        #error
#endif

void fxWorkerInitialize(txWorker* worker)
{
	void* preparation = xsPreparation();
	worker->machine = fxPrepareMachine(NULL, preparation, worker->name, worker, NULL);
    worker->machine->host = worker->ownerMachine->host;
	xsBeginHost(worker->machine);
	{
		xsVars(3);
		xsVar(0) = xsNewHostObject(NULL);
		xsSetHostData(xsVar(0), worker);
		xsVar(1) = xsNewHostFunction(xs_worker_postfromworker, 1);
		xsSet(xsVar(0), xsID_postMessage, xsVar(1));
// 		xsVar(1) = xsNewHostFunction(xs_worker_close, 0);
// 		xsSet(xsVar(0), xsID_close, xsVar(1));
		xsSet(xsGlobal, xsID_self, xsVar(0));
		worker->reference = xsVar(0);
		xsRemember(worker->reference);
		
		xsVar(0) = xsAwaitImport(worker->name, XS_IMPORT_DEFAULT);
		if (xsTest(xsVar(0)) && xsIsInstanceOf(xsVar(0), xsFunctionPrototype))
			xsCallFunction0(xsVar(0), xsGlobal);
	}
	xsEndHost(worker->machine);
#if mxInstrument
	fxDescribeInstrumentation(worker->machine, 0, NULL, NULL);
#endif
	mxLockMutex(&worker->runningMutex);
	worker->running = 1;
	mxSignalCondition(&worker->runningCondition);
	mxUnlockMutex(&worker->runningMutex);
}

void fxWorkerMessage(void* machine, void* it)
{
	txWorkerMessageJob* job = it;
	xsBeginHost(machine);
	{
		xsVars(1);
		xsTry {
			xsCollectGarbage();
			xsResult = xsDemarshall(job->argument);
			xsVar(0) = xsAccess(*(job->reference));
			xsCall1(xsVar(0), xsID_onmessage, xsResult);
		}
		xsCatch {
		}
	}
	xsEndHost(machine);
#if mxInstrument
#endif
	c_free(job->argument);
}

void fxWorkerMessageAlien(void* machine, void* it)
{
	txWorkerMessageJob* job = it;
	xsBeginHost(machine);
	{
		xsVars(1);
		xsTry {
			xsResult = xsDemarshallAlien(job->argument);
			xsVar(0) = xsAccess(*(job->reference));
			xsCall1(xsVar(0), xsID("onmessage"), xsResult); // alien xsID!
		}
		xsCatch {
		}
	}
	xsEndHost(machine);
	c_free(job->argument);
}

void fxWorkerTerminate(txWorker* worker)
{
    xsDeleteMachine(worker->machine);
	mxLockMutex(&worker->runningMutex);
	worker->running = 0;
	mxSignalCondition(&worker->runningCondition);
	mxUnlockMutex(&worker->runningMutex);
}

void xs_worker_destructor(void *data)
{
	if (data) {
		txWorker* worker = data;
		#if mxLinux
			g_main_loop_quit(worker->main_loop);
		#else
                        #error
		#endif
			mxLockMutex(&worker->runningMutex);
			while (worker->running) mxWaitCondition(&worker->runningCondition, &worker->runningMutex);
			mxUnlockMutex(&worker->runningMutex);
			mxDeleteMutex(&worker->runningMutex);
			mxDeleteCondition(&worker->runningCondition);
			c_free(worker);
	}
}

void xs_worker(xsMachine *the)
{
	char *name = xsToString(xsArg(0));
	if (xsTest(xsAwaitImport(name, XS_IMPORT_PREFLIGHT))) {
		txWorker* worker = c_calloc(sizeof(txWorker) + c_strlen(name), 1);
		if (worker == NULL)
			xsUnknownError("not enough memory");
		worker->ownerMachine = the;
		worker->ownerReference = xsThis;
		xsRemember(worker->ownerReference);
		c_strcpy(worker->name, name);
		xsSetHostData(xsThis, worker);
	
		mxCreateCondition(&worker->runningCondition);
		mxCreateMutex(&worker->runningMutex);
		mxLockMutex(&worker->runningMutex);
		#if mxLinux
			g_thread_new(worker->name, fxWorkerLoop, worker);
		#else
                        #error
		#endif
		while (!worker->running) mxWaitCondition(&worker->runningCondition, &worker->runningMutex);
		mxUnlockMutex(&worker->runningMutex);
	}
    else {
			xsUnknownError("worker not found");
    }
}

void xs_worker_postfrominstantiator(xsMachine *the)
{
	txWorker* worker = xsGetHostData(xsThis);
	txWorkerMessageJob* job = c_calloc(sizeof(txWorkerMessageJob), 1);
	if (job == NULL)
		xsUnknownError("not enough memory");
		job->callback = fxWorkerMessage;
		job->argument = xsMarshall(xsArg(0));
	job->reference = &(worker->reference);
	fxQueueWorkerJob(worker->machine, job);
}

void xs_worker_postfromworker(xsMachine *the)
{
	txWorker* worker = xsGetHostData(xsThis);
	txWorkerMessageJob* job = c_calloc(sizeof(txWorkerMessageJob), 1);
	if (job == NULL)
		xsUnknownError("not enough memory");
	job->argument = xsMarshall(xsArg(0));
	job->reference = &(worker->ownerReference);
	job->callback = fxWorkerMessage;
	fxQueueWorkerJob(worker->ownerMachine, job);
}

void xs_worker_terminate(xsMachine *the)
{
	txWorker* worker = xsGetHostData(xsThis);
	xsForget(worker->ownerReference);
	xsSetHostData(xsThis, NULL);
	xs_worker_destructor(worker);
}

void xs_sharedworker(xsMachine *the)
{
	xsDebugger();
}
