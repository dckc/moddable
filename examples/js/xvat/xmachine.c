/*
 * cross-machine calling.
 */
#include <stdio.h>

#include "xs.h"

typedef struct
{
	xsMachine *machine;
} xsVMRecord, *xsVM;

void VM_prototype_constructor(xsMachine *the)
{
	xsCreation _creation = {
		16 * 1024 * 1024, /* initialChunkSize */
		16 * 1024 * 1024, /* incrementalChunkSize */
		1 * 1024 * 1024,  /* initialHeapCount */
		1 * 1024 * 1024,  /* incrementalHeapCount */
		4096,			  /* stackCount */
		4096 * 3,		  /* keyCount */
		1993,			  /* nameModulo */
		127,			  /* symbolModulo */
		32 * 1024,		  /* parserBufferSize */
		1993,			  /* parserTableModulo */
	};
	xsCreation *creation = &_creation;

	xsVMRecord r;
	// TODO: consider letting the caller set the name
	r.machine = fxCreateMachine(creation, "VM", NULL);
	xsSetHostChunk(xsThis, &r, sizeof(r));
}

void VM_prototype_evaluate(xsMachine *the)
{
	xsVM vm = xsGetHostChunk(xsThis);
	xsStringValue expr = xsToString(xsArg(0));
	fprintf(stderr, "the: %p vm %p eval: %s\n", the, &xsThis, expr);
	xsStringValue result;
	xsStringValue message = NULL;
	xsVars(1);
	xsCollectGarbage(); // ISSUE: good idea to collect garbage before and after x-machine calls?
	xsBeginHost(vm->machine);
	{
		xsVars(3);
		{
			xsTry
			{
				xsCollectGarbage();
				xsVar(0) = xsString(expr);
				xsVar(1) = xsCall1(xsGlobal, xsID("eval"), xsVar(0));
				result = xsToString(xsVar(1));
			}
			xsCatch
			{
				message = xsToString(xsException);
				fprintf(stderr, "### %s\n", message);
			}
		}
	}
	xsEndHost(vm->machine);
	xsCollectGarbage();
	if (message)
	{
		xsUnknownError("%s", message);
	}
	else
	{
		xsResult = xsString(result);
	}
}

void VM_prototype_destructor(void *data)
{
	fprintf(stderr, "del VM data=%p\n", data);
	if (!data) {
		return;
	}
	xsVM vm = data;
	if (vm->machine)
	{
		xsDeleteMachine(vm->machine);
		vm->machine = NULL;
	}
}
