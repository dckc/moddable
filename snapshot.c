#include <assert.h>

#include "xsAll.h"
#include "xs.h"

void Snapshot_prototype_constructor(xsMachine* the)
{
}

void Snapshot_prototype_destructor(xsMachine* the)
{
}

// a la fxPrintSlot
// https://github.com/Moddable-OpenSource/moddable/blob/public/xs/tools/xslSlot.c#L946
// IDEA: consider protobuf? capnproto?
static void dumpSlot(xsMachine* the, xsSlot buf, xsIntegerValue offset, txSlot* slot)
{
  //? XS_DEBUG_FLAG
  xsSetArrayBufferData(buf, offset, &"ID", 2); // opcode ID
  offset += 2;
  xsSetArrayBufferData(buf, offset, &slot->ID, 2); // ISSUE: which endian?
  offset += 2;
  xsSetArrayBufferData(buf, offset, &"FL", 2); // opcode FL = flag
  offset += 2;
  xsSetArrayBufferData(buf, offset, &slot->flag, 1);
  offset += 1;

  switch(slot->kind) {
  case XS_STRING_X_KIND:
    xsSetArrayBufferData(buf, offset, &"St", 2); // opcode St = String
    offset += 2;
    xsStringValue value = slot->value.string;
    txU4 len = strlen(value);
    xsSetArrayBufferData(buf, offset, &len, 4);
    offset += 4;
    xsSetArrayBufferData(buf, offset, value, len);
    offset += len;
    break;
  default:
    fprintf(stderr, "slot kind not implemented: %d!\n", slot->kind);
    assert(0); //
  }

  xsSetArrayBufferLength(buf, offset);
}

void Snapshot_prototype_dump(xsMachine* the)
{
  // ?? xsIntegerValue c = xsToInteger(xsArgc);
  xsSlot root = xsArg(0);
  // TODO: exits = xsArg(1)
  xsIntegerValue bigEnough = 1024;  // ISSUE: resizeable data structure?
  xsSlot buf = xsArrayBuffer(NULL, bigEnough);
  dumpSlot(the, buf, 0, (txSlot*)&root); // ISSUE: xsSlot -> txSlot???
  xsResult = buf;
}
