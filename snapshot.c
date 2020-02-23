#include <assert.h>

#include "xsAll.h"
#include "xs.h"

void Snapshot_prototype_constructor(xsMachine* the)
{
}

void Snapshot_prototype_destructor(xsMachine* the)
{
}


static void ensureSpace(xsMachine* the, txSlot* buf, xsIntegerValue size)
{
  txInteger capacity = xsGetArrayBufferLength(*buf);
  if (capacity < size) {
    xsIntegerValue nextQuantum = (size + 0x100) - (size % 0x100);
    fprintf(stderr, "ensureSpace: %p, old capacity %d; new size: %d; nextQuantum: %d (kind: %d ref: %p next: %p kind: %d)\n",
            buf, capacity, size, nextQuantum,
            buf->kind, buf->value.reference, buf->value.reference->next, buf->value.reference->next->kind);
    xsSetArrayBufferLength(*buf, nextQuantum);
    fprintf(stderr, "ensureSpace: after set length: %p, (kind: %d ref: %p next: %p kind: %d)\n",
            buf,
            buf->kind, buf->value.reference, buf->value.reference->next, buf->value.reference->next->kind);
    fprintf(stderr, "ensureSpace: fxSetArrayBufferLength() done. %p length = %d\n",
            buf, xsGetArrayBufferLength(*buf));
  } else {
    fprintf(stderr, "ensureSpace: %p, capacity %d sufficient for size: %d\n", buf, capacity, size);
  }
}


static xsIntegerValue append(xsMachine* the, txSlot* buf, xsIntegerValue offset, void* src, xsIntegerValue qty)
{
  ensureSpace(the, buf, offset + qty);
  fprintf(stderr, "append: xsSetArrayBufferData(buf=%p, offset=%d, qty=%d)\n", buf, offset, qty);
  xsSetArrayBufferData(*buf, offset, src, qty);
  fprintf(stderr, "append: xsSetArrayBufferData() done.\n");
  return offset + qty;
}

// a la fxPrintSlot
// https://github.com/Moddable-OpenSource/moddable/blob/public/xs/tools/xslSlot.c#L946
// IDEA: consider protobuf? capnproto?
static xsIntegerValue dumpSlot(xsMachine* the, txSlot* buf, xsIntegerValue offset, txSlot* slot)
{
  // ISSUE: assumes little-endian
  offset = append(the, buf, offset, &(slot->KIND_FLAG_ID), sizeof(slot->KIND_FLAG_ID));

  switch(slot->kind) {
  case XS_STRING_KIND:
  case XS_STRING_X_KIND:
    {
      xsStringValue value = slot->value.string;
      txU4 len = strlen(value);
      offset = append(the, buf, offset, &len, sizeof(len));
      offset = append(the, buf, offset, value, len);
    }
    break;
  default:
    fprintf(stderr, "slot kind not implemented: %d!\n", slot->kind);
    assert(0); //
  }

  return offset;
}


void Snapshot_prototype_dump(xsMachine* the)
{
  // ?? xsIntegerValue c = xsToInteger(xsArgc);
  xsSlot root = xsArg(0);
  // TODO: exits = xsArg(1)
  xsVars(1);
  xsVar(0) = xsArrayBuffer(NULL, 256);
  xsIntegerValue size = dumpSlot(the, (txSlot*)&(xsVar(0)), 0, (txSlot*)&root); // ISSUE: xsSlot -> txSlot???
  fprintf(stderr, "dump: xsSetArrayBufferLength(size=%d)\n", size);
  xsSetArrayBufferLength(xsVar(0), size);
  fprintf(stderr, "dump: xsSetArrayBufferLength() done.\n");
  xsResult = xsVar(0);
}
