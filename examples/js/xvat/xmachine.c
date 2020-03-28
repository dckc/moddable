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

void VM_prototype_call(xsMachine *the)
{
	xsVM vm = xsGetHostChunk(xsThis);
	xsStringValue module = xsToString(xsArg(0));
	xsStringValue arg = xsToString(xsArg(1));
	xsStringValue result;
	xsVars(1);
	xsBeginHost(vm->machine);
	{
		xsVars(3);
		{
			xsTry
			{
				xsVar(0) = xsString(arg);
				// TODO: non-default export? xsStringValue export = xsToString(xsArg(1));
				xsVar(1) = xsAwaitImport(module, XS_IMPORT_DEFAULT);
				xsVar(2) = xsCallFunction1(xsVar(1), xsUndefined, xsVar(0));
				result = xsToString(xsVar(2));
			}
			xsCatch
			{
				fprintf(stderr, "@@@AAAARG!");
			}
		}
	}
	xsEndHost(vm->machine);
	xsVar(0) = xsString(result);
}

void VM_prototype_destructor(xsMachine *the)
{
	xsVM vm = xsGetHostData(xsThis);
	xsDeleteMachine(vm->machine);
}
