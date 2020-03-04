#include <assert.h>

#include "xsAll.h"
#include "xs.h"

static xsIntegerValue alreadySeen(xsMachine* the, txSlot* target, xsIntegerValue seen);
static xsIntegerValue append(xsMachine* the, xsIntegerValue offset, void* src, xsIntegerValue qty);
static xsIntegerValue dumpComplex(xsMachine* the, xsIntegerValue offset, txSlot* slot, xsIntegerValue* seen);
static xsIntegerValue dumpScalar(xsMachine* the, xsIntegerValue offset, txSlot* slot);
static xsIntegerValue dumpSlot(xsMachine* the, xsIntegerValue offset, txSlot* slot, xsIntegerValue* seen);
static xsIntegerValue dumpSlotOpt(xsMachine* the, xsIntegerValue offset, txSlot* slot, xsIntegerValue* seen);
static xsIntegerValue dumpSlotValue(xsMachine* the, xsIntegerValue offset, txSlot* slot, xsIntegerValue* seen);
// ISSUE: debug print stuff should go away
static void debug_push(char *format, ...);
static void debug_pop();


#pragma GCC diagnostic ignored "-Wunused-parameter"
void Snapshot_prototype_constructor(xsMachine* the)
{
}

void Snapshot_prototype_destructor(xsMachine* the)
{
}
#pragma GCC diagnostic warning "-Wunused-parameter"


/**

The snapshot format follows the type given in slot.proto;
serialization details differ; see dumpSlot etc. for details.

ISSUE: snapshot format includes slot memory addresses, which are
       non-deterministic. Snapshot object is hence powerful.  Is this
       necessary? Perhaps offets within the snapshot suffice as object
       identities?
*/
void Snapshot_prototype_dump(xsMachine* the)
{
  if (xsToInteger(xsArgc) != 2) {
    mxTypeError("expected 2 arguments");
  }
  xsSlot root = xsArg(0);
  // xsArg(1) = exits
  if (!xsIsInstanceOf(xsArg(1), xsArrayPrototype)) {
    mxTypeError("expected array");
  }

  xsIntegerValue exitQty = xsToInteger(xsGet(xsArg(1), xsID("length")));
  // Initialize seen to 1 less than the lowest ibid.
  xsIntegerValue seen = exitQty - 1;

  // xsVar(0) is snapshot serialization ArrayBuffer
  // xsVar(1) is no longer used
  // IDEA: use xsVars(1) for seen set
  // xsVar(2) thru 5 are for enumerating exits
  xsVars(6);

  xsVar(0) = xsArrayBuffer(NULL, exitQty);

  debug_push(">root");
  xsIntegerValue size = dumpSlot(the, exitQty, (txSlot*)&root, &seen);
  debug_pop();
  fprintf(stderr, "dump: xsSetArrayBufferLength(size=%d)\n", size);
  xsSetArrayBufferLength(xsVar(0), size);
  xsResult = xsVar(0);
}


/** Slot - Ibid, Fresh?

First we append the XS kind. Scalars (< XS_REFERENCE_KIND) are always
Fresh.

Complex values start with a "delta" that distinguishes Ibid from
Fresh.  A negative delta denotes Fresh and the delta points
to the previous fresh compound slot. An exit is denoted by a delta
between 0 and the number of exits; an ibid is given by a positive
delta >= the number of exits.

 */
// a la fxPrintSlot
// https://github.com/Moddable-OpenSource/moddable/blob/public/xs/tools/xslSlot.c#L946
static xsIntegerValue dumpSlot(xsMachine* the, xsIntegerValue offset, txSlot* slot, xsIntegerValue* seen)
{
  debug_push(">@%d K:%d ", offset, slot->kind);
  offset = append(the, offset, &(slot->kind), sizeof(slot->kind));
  xsIntegerValue ibid;
  if (slot->kind >= XS_REFERENCE_KIND
      && (ibid = alreadySeen(the, slot, *seen)) >= 0) {
    // Ibid - < exits.length
    offset = append(the, offset, &ibid, sizeof(ibid));
  } else {
    // Fresh
    offset = dumpSlotValue(the, offset, slot, seen);
    if (slot->kind != XS_ARRAY_KIND) {
      debug_push(">next");
      offset = dumpSlotOpt(the, offset, slot->next, seen);
      debug_pop();
    }
  }
  debug_pop();
  return offset;
}

/**
Arrays omit the "next" slot for both the elements and the Array itself.
*/
static xsIntegerValue dumpItem(xsMachine* the, xsIntegerValue offset, txSlot* slot, xsIntegerValue* seen)
{
  offset = append(the, offset, &(slot->kind), sizeof(slot->kind));
  xsIntegerValue ibid;
  if (slot->kind >= XS_REFERENCE_KIND
      && (ibid = alreadySeen(the, slot, *seen)) >= 0) {
    // Ibid - < exits.length
    offset = append(the, offset, &ibid, sizeof(ibid));
  } else {
    // Fresh
    offset = dumpSlotValue(the, offset, slot, seen);
  }
  return offset;
}


/**
 * REQUIRES: xsVar(0) is an ArrayBuffer
 *
 * NOTE: conflates null ID name with empty ID name
 */
static xsIntegerValue dumpID(xsMachine* the, xsIntegerValue offset, txFlag flag, txID id)
{
  offset = append(the, offset, &flag, sizeof(flag));
  offset = append(the, offset, &id, sizeof(id));
  txU4 len = 0;
  char* value = fxGetKeyName(the, id);
  if (value) {
    len = strlen(value);
  }
  offset = append(the, offset, &len, sizeof(len));
  if (value) {
    offset = append(the, offset, value, len);
    fprintf(stderr, "@%d .%s [%d]\n", offset, value, id);
    debug_push(">.%s", value);
  } else {
    debug_push(">id:%d", id);
  }
  return offset;
}


/**
 * REQUIRES: xsVar(0) is an ArrayBuffer
 *
 * For scalar values, we append the flag and ID and then kind-specific data.
 *
 * For (fresh) Complex values, we append the delta and the identity /
 * address of the slot followed by flag and ID and then kind-specific data.
 */
static xsIntegerValue dumpSlotValue(xsMachine* the, xsIntegerValue offset, txSlot* slot, xsIntegerValue *seen)
{
  if (slot->kind < XS_REFERENCE_KIND) {
    offset = dumpID(the, offset, slot->flag, slot->ID); // includes debug_push
    offset = dumpScalar(the, offset, slot);
    debug_pop();
  } else {
    debug_push(">fresh %p", slot);
    xsIntegerValue exitQty = xsToInteger(xsGet(xsArg(1), xsID("length")));
    xsIntegerValue delta = (*seen >= exitQty ? *seen : exitQty) - offset;
    *seen = offset;
    offset = append(the, offset, &delta, sizeof(delta));
    offset = append(the, offset, &slot, sizeof(slot));
    offset = dumpID(the, offset, slot->flag, slot->ID); // includes debug_push
    offset = dumpComplex(the, offset, slot, seen);
    debug_pop();
    debug_pop();
  }

  return offset;
}

/**
 * REQUIRES: xsVar(0) is an ArrayBuffer
 * REQUIRES: slot->kind already appended
 */
static xsIntegerValue dumpScalar(xsMachine* the, xsIntegerValue offset, txSlot* slot)
{
  switch(slot->kind) {
  case XS_UNINITIALIZED_KIND:
  case XS_UNDEFINED_KIND:
  case XS_NULL_KIND:
    break;

  case XS_BOOLEAN_KIND: {
    debug_push("> =%s", slot->value.boolean ? "true" : "false");
    // ISSUE: 4 bytes for a boolean? really?
    offset = append(the, offset, &(slot->value.boolean), sizeof(slot->value.boolean));
    debug_pop();
  } break;

  case XS_INTEGER_KIND: {
    debug_push("> =%d", slot->value.integer);
    offset = append(the, offset, &(slot->value.integer), sizeof(slot->value.integer));
    debug_pop();
  } break;

  case XS_NUMBER_KIND: {
    // ISSUE: assume IEEE double format?
    debug_push("> =%f", slot->value.number);
    offset = append(the, offset, &(slot->value.number), sizeof(slot->value.number));
    debug_pop();
  } break;

    // ISSUE: exposes distinction between ROM string and heap string
  case XS_STRING_KIND:
  case XS_STRING_X_KIND: {
    txU4 len = strlen(slot->value.string);
    debug_push("> ='%.16s...' len:%d", slot->value.string, len);
    offset = append(the, offset, &len, sizeof(len));
    offset = append(the, offset, slot->value.string, len);
    debug_pop();
  } break;

  case XS_SYMBOL_KIND: {
    debug_push("> sym:%d", slot->value.symbol);
    offset = append(the, offset, &(slot->value.symbol), sizeof(slot->value.symbol));
    debug_pop();
  } break;
    /*@@@@
	case XS_BIGINT_KIND:
	case XS_BIGINT_X_KIND: {
		fprintf(file, ".kind = XS_BIGINT_X_KIND}, ");
		fprintf(file, ".value = { .bigint = { ");
		fprintf(file, ".data = (txU4*)&gxBigIntData[%d], ", linker->bigintSize);
		fprintf(file, ".size = %d, ", slot->value.bigint.size);
		fprintf(file, ".sign = %d, ", slot->value.bigint.sign);
		fprintf(file, " } } ");
		c_memcpy(linker->bigintData + linker->bigintSize, slot->value.bigint.data, slot->value.bigint.size * sizeof(txU4));
		linker->bigintSize += slot->value.bigint.size;
	} break;
    */
  default:
    fprintf(stderr, "== slot kind not implemented: %d!\n", slot->kind);
    assert(0); //
  }

  return offset;
}


/** detect "seen" slots
 * REQUIRES: seen >= xsArg(1).length

avoid infinite recursion for cyclic structures
*/
static xsIntegerValue alreadySeen(xsMachine* the, txSlot* target, xsIntegerValue seen) {
  xsIntegerValue exitQty = xsToInteger(xsGet(xsArg(1), xsID("length")));
  fprintf(stderr, "=== %p in %d exits?\n", target, exitQty);
  xsIntegerValue ix = 0;
  xsVar(2) = xsEnumerate(xsArg(1));
  for (;;) {
    xsVar(3) = xsCall0(xsVar(2), xsID("next"));
    // ISSUE: warning: operation on ‘the->stack’ may be undefined [-Wsequence-point]
    if (xsTest(xsGet(xsVar(3), xsID("done"))))
      break;
    xsVar(4) = xsGet(xsVar(3), xsID("value"));
    xsVar(5) = xsGetAt(xsArg(1), xsVar(4));
    // fprintf(stderr, "exits[%d] kind %d ref %p == target %p?\n",
    //        ix, xsVar(5).kind, fxGetInstance(the, &xsVar(5)), target);
    if (fxIsSameSlot(the, &xsVar(5), target)
        || fxGetInstance(the, &xsVar(5)) == target) {
      fprintf(stderr, "=== %p EXIT %d\n", target, ix);
      return ix;
    }
    ix += 1;
  }

  fprintf(stderr, "=== %p ibid? seen=%d\n", target, seen);
  while (seen >= exitQty) {
    xsIntegerValue delta;
    txSlot* candidate;
    xsGetArrayBufferData(xsVar(0), seen + sizeof(delta), &candidate, sizeof(candidate));
    if (candidate == target) {
      fprintf(stderr, "=== %p IBID %d\n", target, seen);
      return seen;
    }
    xsGetArrayBufferData(xsVar(0), seen, &delta, sizeof(delta));
    // fprintf(stderr, "=== seen loop: seen:%d exitQty:%d delta:%d\n", seen, exitQty, delta);
    seen += delta;
  }
  fprintf(stderr, "=== %p FRESH\n", target);
  return -1;
}


/**
 * REQUIRES: xsVar(0) is an ArrayBuffer
 */
static xsIntegerValue dumpComplex(xsMachine* the, xsIntegerValue offset, txSlot* slot, xsIntegerValue* seen)
{
  switch(slot->kind) {
  case XS_REFERENCE_KIND: {
    debug_push(">reference");
    offset = dumpSlotOpt(the, offset, slot->value.reference, seen);
    debug_pop();
  } break;
  case XS_CLOSURE_KIND: {
    offset = dumpSlotOpt(the, offset, slot->value.closure, seen);
  } break;
  case XS_INSTANCE_KIND: {
    // ISSUE: fprintf(file, ".value = { .instance = { NULL, ");
    debug_push(">prototype");
    offset = dumpSlotOpt(the, offset, slot->value.instance.prototype, seen);
    debug_pop();
  } break;
  case XS_ARRAY_KIND: {
    txSlot *item = slot->value.array.address;
    txInteger size = (txInteger)fxGetIndexSize(the, slot);
    offset = append(the, offset, &size, sizeof(size));
    while (size) {
      // ISSUE: XS_MARK_FLAG??
      debug_push(">@%d item[%d] K:%d", offset, size, item->kind);
      offset = dumpItem(the, offset, item, seen);
      debug_pop();
      item++;
      size--;
    }
  } break;
          /*
	case XS_ARRAY_BUFFER_KIND: {
		fprintf(file, ".kind = XS_ARRAY_BUFFER_KIND}, ");
		fprintf(file, ".value = { .arrayBuffer = { (txByte*)");
		fxWriteCData(file, slot->value.arrayBuffer.address, slot->value.arrayBuffer.length);
		fprintf(file, ", %d } } ", (int)slot->value.arrayBuffer.length);
	} break;
          */
	case XS_CALLBACK_X_KIND:
	case XS_CALLBACK_KIND: {
          offset = append(the, offset, slot->value.callback.address, sizeof(slot->value.callback.address));
	} break;
	case XS_CODE_KIND:  {
          // .code =
          {
            txChunk* chunk = (txChunk*)(slot->value.code.address - sizeof(txChunk));
            int size = chunk->size - sizeof(txChunk);
            offset = append(the, offset, &size, sizeof(size));
            offset = append(the, offset, slot->value.code.address, chunk->size - sizeof(txChunk));
          }
          offset = dumpSlotOpt(the, offset, slot->value.code.closures, seen);
	} break;
	case XS_CODE_X_KIND: {
          fprintf(stderr, "TODO! XS_CODE_X_KIND: %p\n", slot->value.code.address);
          // offset = append(the, offset, &slot->value.code.address, sizeof(slot->value.code.address));
          // offset = dumpSlotOpt(the, offset, slot->value.code.closures, seen);
	} break;
          /*
	case XS_DATE_KIND: {
		fprintf(file, ".kind = XS_DATE_KIND}, ");
		fprintf(file, ".value = { .number = ");
		fxPrintNumber(the, file, slot->value.number);
		fprintf(file, " } ");
	} break;
	case XS_DATA_VIEW_KIND: {
		fprintf(file, ".kind = XS_DATA_VIEW_KIND}, ");
		fprintf(file, ".value = { .dataView = { %d, %d } }", slot->value.dataView.offset, slot->value.dataView.size);
	} break;
	case XS_FINALIZATION_CELL_KIND: {
		fprintf(file, ".kind = XS_FINALIZATION_CELL_KIND}, ");
		fprintf(file, ".value = { .finalizationCell = { ");
		fxPrintAddress(the, file, slot->value.finalizationCell.target);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.finalizationCell.token);
		fprintf(file, " } }");
	} break;
	case XS_FINALIZATION_GROUP_KIND: {
		fprintf(file, ".kind = XS_FINALIZATION_GROUP_KIND}, ");
		fprintf(file, ".value = { .finalizationGroup = { ");
		fxPrintAddress(the, file, slot->value.finalizationGroup.callback);
		fprintf(file, ", %d } }", slot->value.finalizationGroup.flags);
	} break;
	case XS_GLOBAL_KIND: {
		fprintf(file, ".kind = XS_GLOBAL_KIND}, ");
		fprintf(file, ".value = { .table = { NULL, 0 } }");
	} break;
	case XS_HOST_KIND: {
		fprintf(file, ".kind = XS_HOST_KIND}, ");
		fprintf(file, ".value = { .host = { NULL, { .destructor = %s } } }", fxGetCallbackName(the, (txCallback)slot->value.host.variant.destructor ));
	} break;
	case XS_MAP_KIND: {
		fprintf(file, ".kind = XS_MAP_KIND}, ");
		fprintf(file, ".value = { .table = { (txSlot**)&gxSlotData[%d], %d } }", linker->slotSize, slot->value.table.length);
		c_memcpy(linker->slotData + linker->slotSize, slot->value.table.address, slot->value.table.length * sizeof(txSlot*));
		linker->slotSize += slot->value.table.length;
	} break;
          */
	case XS_MODULE_KIND: {
          fprintf(stderr, "@@TODO! XS_MODULE_KIND %p %d\n", slot->value.module.realm, slot->value.module.id);
          // offset = dumpSlotOpt(the, offset, slot->value.module.realm, seen);
          // offset = append(the, offset, &(slot->value.module.id), sizeof(slot->value.module.id));
	} break;
          /*
	case XS_PROMISE_KIND: {
		fprintf(file, ".kind = XS_PROMISE_KIND}, ");
		fprintf(file, ".value = { .integer = %d } ", slot->value.integer);
	} break;
	case XS_PROXY_KIND: {
		fprintf(file, ".kind = XS_PROXY_KIND}, ");
		fprintf(file, ".value = { .instance = { ");
		fxPrintAddress(the, file, slot->value.proxy.handler);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.proxy.target);
		fprintf(file, " } } ");
	} break;
	case XS_REGEXP_KIND: {
		fprintf(file, ".kind = XS_REGEXP_KIND}, ");
		fprintf(file, ".value = { .regexp = { (txInteger*)NULL, (txInteger*)NULL } } ");
	} break;
	case XS_SET_KIND: {
		fprintf(file, ".kind = XS_SET_KIND}, ");
		fprintf(file, ".value = { .table = { (txSlot**)&gxSlotData[%d], %d } }", linker->slotSize, slot->value.table.length);
		c_memcpy(linker->slotData + linker->slotSize, slot->value.table.address, slot->value.table.length * sizeof(txSlot*));
		linker->slotSize += slot->value.table.length;
	} break;
	case XS_TYPED_ARRAY_KIND: {
		fprintf(file, ".kind = XS_TYPED_ARRAY_KIND}, ");
		fprintf(file, ".value = { .typedArray = { (txTypeDispatch*)(&gxTypeDispatches[%d]), (txTypeAtomics*)(&gxTypeAtomics[%d]) } }", fxGetTypeDispatchIndex(slot->value.typedArray.dispatch), fxGetTypeAtomicsIndex(slot->value.typedArray.atomics));
	} break;
	case XS_WEAK_MAP_KIND: {
		fprintf(file, ".kind = XS_WEAK_MAP_KIND}, ");
		fprintf(file, ".value = { .table = { (txSlot**)&gxSlotData[%d], %d } }", linker->slotSize, slot->value.table.length);
		c_memcpy(linker->slotData + linker->slotSize, slot->value.table.address, (slot->value.table.length + 1) * sizeof(txSlot*));
		linker->slotSize += slot->value.table.length + 1;
	} break;
	case XS_WEAK_REF_KIND: {
		fprintf(file, ".kind = XS_WEAK_REF_KIND}, ");
		fprintf(file, ".value = { .weakRef = { ");
		fxPrintAddress(the, file, slot->value.weakRef.target);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.weakRef.link);
		fprintf(file, " } }");
	} break;
	case XS_WEAK_SET_KIND: {
		fprintf(file, ".kind = XS_WEAK_SET_KIND}, ");
		fprintf(file, ".value = { .table = { (txSlot**)&gxSlotData[%d], %d } }", linker->slotSize, slot->value.table.length);
		c_memcpy(linker->slotData + linker->slotSize, slot->value.table.address, (slot->value.table.length + 1) * sizeof(txSlot*));
		linker->slotSize += slot->value.table.length + 1;
	} break;
          */
  case XS_ACCESSOR_KIND: {
    debug_push(">getter");
    offset = dumpSlotOpt(the, offset, slot->value.accessor.getter, seen);
    debug_pop();
    debug_push(">setter");
    offset = dumpSlotOpt(the, offset, slot->value.accessor.setter, seen);
    debug_pop();
  } break;
          /*
	case XS_AT_KIND: {
		fprintf(file, ".kind = XS_AT_KIND}, ");
		fprintf(file, ".value = { .at = { 0x%x, %d } }", slot->value.at.index, slot->value.at.id);
	} break;
	case XS_ENTRY_KIND: {
		fprintf(file, ".kind = XS_ENTRY_KIND}, ");
		fprintf(file, ".value = { .entry = { ");
		fxPrintAddress(the, file, slot->value.entry.slot);
		fprintf(file, ", 0x%x } }", slot->value.entry.sum);
	} break;
	case XS_ERROR_KIND: {
		fprintf(file, ".kind = XS_ERROR_KIND}, ");
		fprintf(file, ".value = { .number = 0 } ");
	} break;
          */
  case XS_HOME_KIND: {
    fprintf(stderr, "@@TODO! XS_HOME_KIND %p %p\n", slot->value.home.object, slot->value.home.module);
    /*
    debug_push(">home-object");
    offset = dumpSlotOpt(the, offset, slot->value.home.object, seen);
    debug_pop();
    debug_push(">home-module");
    offset = dumpSlotOpt(the, offset, slot->value.home.module, seen);
    debug_pop();
    */
  } break;
	case XS_EXPORT_KIND: {
          offset = dumpSlotOpt(the, offset, slot->value.export.closure, seen);
          offset = dumpSlotOpt(the, offset, slot->value.export.module, seen);
	} break;
          /*
	case XS_KEY_KIND:
	case XS_KEY_X_KIND: {
		fprintf(file, ".kind = XS_KEY_X_KIND}, ");
		fprintf(file, ".value = { .key = { ");
		fxWriteCString(file, slot->value.key.string);
		fprintf(file, ", 0x%x } }", slot->value.key.sum);
	} break;
	case XS_LIST_KIND: {
		fprintf(file, ".kind = XS_LIST_KIND}, ");
		fprintf(file, ".value = { .list = { ");
		fxPrintAddress(the, file, slot->value.list.first);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.list.last);
		fprintf(file, " } }");
	} break;
	case XS_PRIVATE_KIND: {
		fprintf(file, ".kind = XS_PRIVATE_KIND}, ");
		fprintf(file, ".value = { .private = { ");
		fxPrintAddress(the, file, slot->value.private.check);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.private.first);
		fprintf(file, " } }");
	} break;
	case XS_STACK_KIND: {
		fprintf(file, ".kind = XS_STACK_KIND}, ");
	} break;
#ifdef mxHostFunctionPrimitive
	case XS_HOST_FUNCTION_KIND: {
		fprintf(file, ".kind = XS_HOST_FUNCTION_KIND}, ");
		fprintf(file, ".value = { .hostFunction = { %s, NULL } }", fxGetBuilderName(the, slot->value.hostFunction.builder));
	} break;
#endif
     */

  default:
    fprintf(stderr, "== slot kind not implemented: %d!\n", slot->kind);
    assert(0); //
  }

  return offset;
}

/**
 * REQUIRES: xsVar(0) is an ArrayBuffer
 * REQUIRES: seen <= offset
 * RETURNS: new offset after serializing None or Some(slot)
 */
static xsIntegerValue dumpSlotOpt(xsMachine* the, xsIntegerValue offset, txSlot* slot, xsIntegerValue* seen)
{
  if (!slot) {
    // distinct from all XS slot kinds in xsAll.h
    txKind null_slot_kind = XS_UNINITIALIZED_KIND - 1;
    fprintf(stderr, "= dumpSlot %p\n", slot);
    offset = append(the, offset, &null_slot_kind, sizeof(txKind));
  } else {
    offset = dumpSlot(the, offset, slot, seen);
  }
  return offset;
}

static char debug_buf[1024] = { 0 };

static void debug_push(char *format, ...) {
  char *start = debug_buf + strlen(debug_buf);
  va_list spread;
  va_start(spread, format);
  vsprintf(start, format, spread);
  fprintf(stderr, ">%s\n", debug_buf);
}

static void debug_pop() {
  char* last_dot = strrchr(debug_buf, '>');
  fprintf(stderr, "<%s\n", debug_buf);
  if (last_dot) {
    *last_dot = 0;
  }
}


/**
 * REQUIRES: xsVar(0) is an ArrayBuffer
 * ENSURES: buf.length is at least size
 */
static void ensureSpace(xsMachine* the, xsIntegerValue size)
{
  txInteger capacity = xsGetArrayBufferLength(xsVar(0));
  if (capacity < size) {
    xsIntegerValue nextQuantum = (size + 0x100) - (size % 0x100);
    // fprintf(stderr, "ensureSpace(%d) grow capacity %d -> %d\n", size, capacity, nextQuantum);
    xsSetArrayBufferLength(xsVar(0), nextQuantum);
  } else {
    // fprintf(stderr, "ensureSpace(%d) capacity %d ok\n", size, capacity);
  }
}


/**
 * REQUIRES: xsVar(0) is an ArrayBuffer
 * ENSURES: buf.slice(offset, qty) == qty bytes at src
 */
static xsIntegerValue append(xsMachine* the, xsIntegerValue offset, void* src, xsIntegerValue qty)
{
  // fprintf(stderr, "append(%d += %d)\n", offset, qty);
  ensureSpace(the, offset + qty);
  xsSetArrayBufferData(xsVar(0), offset, src, qty);
  return offset + qty;
}
