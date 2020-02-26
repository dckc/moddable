/**
 */

#include <assert.h>

#include "xsAll.h"
#include "xs.h"

#define KEEP_NEXT 0
#define SKIP_NEXT 1

#pragma GCC diagnostic ignored "-Wunused-parameter"
void Snapshot_prototype_constructor(xsMachine* the)
{
}

void Snapshot_prototype_destructor(xsMachine* the)
{
}
#pragma GCC diagnostic warning "-Wunused-parameter"


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


/** track "seen" slots
 * REQUIRES: xsVar(0) is an ArrayBuffer
 *           xsArg(1) is an Array (exits)
 *           seen >= xsArg(1).length
 *           xsVar(1) is available for storing toString() output
 *           xsVar(2) thru 5 are available (for iterating over exits)

avoid infinite recursion for cyclic structures

Each time we serialize a compound slot, we save either
  - the offset where it was already serialized; or
  - the delta to the previous compound slot and
    the address of this slot
*/
static xsIntegerValue alreadySeen(xsMachine* the, txSlot* target, xsIntegerValue seen) {
  char *display = "[?]";
  if (target->kind == XS_REFERENCE_KIND && xsHas(*target, xsID("toString"))) {
    xsVar(1) = xsCall0(*target, xsID("toString"));
    display = xsToString(xsVar(1));
  }
  fprintf(stderr, "==== alreadySeen(%p) seen=%d %s?\n", target, seen, display);

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
    //         ix, xsVar(5).kind, fxGetInstance(the, &xsVar(5)), target);
    if (fxIsSameSlot(the, &xsVar(5), target)
        || fxGetInstance(the, &xsVar(5)) == target) {
      fprintf(stderr, "===== yes %p is exit %d\n", target, ix);
      return ix;
    }
    ix += 1;
  }

  if (fxIsSameValue(the, target, &mxArrayPrototype, 0)) {
    fprintf(stderr, "!! target:%p == Array.prototype\n", target);
  }

  if (target->kind == XS_INSTANCE_KIND
      && target == mxArrayPrototype.value.reference) {
    fprintf(stderr, "Array.prototype already seen! prototype=%p reference=%p\n",
            &mxArrayPrototype, mxArrayPrototype.value.reference);
    xsSlot e1 = xsGetAt(xsArg(1), xsInteger(0));
    fprintf(stderr, "exits[0] kind:%d reference:%p\n", e1.kind, e1.value.reference);
  }

  xsIntegerValue exitQty = xsToInteger(xsGet(xsArg(1), xsID("length")));
  while (seen >= exitQty) {
    xsIntegerValue delta;
    txSlot* candidate;
    xsGetArrayBufferData(xsVar(0), seen + sizeof(delta), &candidate, sizeof(candidate));
    if (candidate == target) {
      fprintf(stderr, "===== yes seen %p at %d\n", target, seen);
      return seen;
    }
    xsGetArrayBufferData(xsVar(0), seen, &delta, sizeof(delta));
    // fprintf(stderr, "=== seen loop: seen:%d exitQty:%d delta:%d\n", seen, exitQty, delta);
    seen += delta;
  }
  fprintf(stderr, "===== no not seen %p exitQty=%d\n", target, exitQty);
  return -1;
}


/**
 * REQUIRES: xsVar(0) is an ArrayBuffer
 */
static xsIntegerValue pushSelf(xsMachine* the, txSlot* self, xsIntegerValue offset, xsIntegerValue *seen) {
  // xsArg(1) == exits
  xsIntegerValue exitQty = xsToInteger(xsGet(xsArg(1), xsID("length")));
  xsIntegerValue delta = (*seen >= exitQty ? *seen : exitQty) - offset;
  fprintf(stderr, "=== pushSelf %p offset %d\n", self, offset);
  xsIntegerValue found;
  if ((found = alreadySeen(the, self, *seen)) >= 0) {
    fprintf(stderr, "==== pushSelf %p; alreadySeen at %d\n", self, found);
    offset = append(the, offset, &found, sizeof(found));
    return -offset;
  }
  *seen = offset;

  fprintf(stderr, "==== pushSelf(%p) delta=%d (#%ld) *seen=%d offset:%d exitQty:%d\n", self, delta, sizeof(delta), *seen, offset, exitQty);
  offset = append(the, offset, &delta, sizeof(delta));
  offset = append(the, offset, &self, sizeof(self));
  return offset;
}

/**
 * REQUIRES: xsVar(0) is an ArrayBuffer
 *
 * NOTE: conflates null ID name with empty ID name
 */
static xsIntegerValue dumpID(xsMachine* the, xsIntegerValue offset, txID id)
{
  offset = append(the, offset, &id, sizeof(id));
  txU4 len = 0;
  char* value = fxGetKeyName(the, id);
  if (value) {
    len = strlen(value);
  }
  offset = append(the, offset, &len, sizeof(len));
  if (value) {
    offset = append(the, offset, value, len);
  }
  fprintf(stderr, "==== dumpID=%d %s\n", id, value);
  return offset;
}

/**
 * REQUIRES: xsVar(0) is an ArrayBuffer
 */
static xsIntegerValue dumpSimpleValue(xsMachine* the, xsIntegerValue offset, txSlot* slot)
{
  fprintf(stderr, "== dumpSimpleValue %p kind = %d\n", slot, slot->kind);

  switch(slot->kind) {
  case XS_UNINITIALIZED_KIND:
  case XS_UNDEFINED_KIND:
  case XS_NULL_KIND:
    // ISSUE: .value = { .number = 0 }?
    break;

  case XS_BOOLEAN_KIND: {
    debug_push(">=%s", slot->value.boolean ? "true" : "false");
    offset = append(the, offset, &(slot->value.boolean), sizeof(slot->value.boolean));
    debug_pop();
  } break;

  case XS_INTEGER_KIND: {
    debug_push(">=%d", slot->value.integer);
    offset = append(the, offset, &(slot->value.integer), sizeof(slot->value.integer));
    debug_pop();
  } break;

  case XS_NUMBER_KIND: {
    // ISSUE: assume IEEE double format?
    debug_push(">=%f", slot->value.number);
    offset = append(the, offset, &(slot->value.number), sizeof(slot->value.number));
    debug_pop();
  } break;

  case XS_STRING_KIND:
  case XS_STRING_X_KIND: {
    txU4 len = strlen(slot->value.string);
    debug_push(">= #%d '%.4s'", len, slot->value.string);
    offset = append(the, offset, &len, sizeof(len));
    offset = append(the, offset, slot->value.string, len);
    debug_pop();
  } break;

  case XS_SYMBOL_KIND: {
    debug_push("> sym:%d", slot->value.symbol);
    offset = append(the, offset, &(slot->value.symbol), sizeof(slot->value.symbol));
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


/**
 * REQUIRES: xsVar(0) is an ArrayBuffer
 */
// a la fxPrintSlot
// https://github.com/Moddable-OpenSource/moddable/blob/public/xs/tools/xslSlot.c#L946
// ISSUE: addresses are non-deterministic; exposes runtime details. Snapshot object is hence powerful.
// IDEA: consider protobuf? capnproto?
static xsIntegerValue dumpSlot(xsMachine* the, xsIntegerValue offset, txSlot* slot,
                               xsIntegerValue* seen, txBoolean skip_next)
{
  if (!slot) {
    txKind null_slot_kind = XS_UNINITIALIZED_KIND - 1;
    fprintf(stderr, "= dumpSlot %p\n", slot);
    offset = append(the, offset, &null_slot_kind, sizeof(txKind));
    return offset;
  }

  debug_push(">slot: %p kind:%d offset:%d", slot, slot->kind, offset);
  offset = append(the, offset, &(slot->kind), sizeof(slot->kind));

  if (slot->kind < XS_REFERENCE_KIND) {
    offset = append(the, offset, &(slot->flag), sizeof(slot->flag));
    offset = dumpID(the, offset, slot->ID);
    offset = dumpSimpleValue(the, offset, slot);
    if (!skip_next) {
      debug_push(">next");
      offset = dumpSlot(the, offset, slot->next, seen, KEEP_NEXT);
      debug_pop();
    }
    debug_pop();
    return offset;
  }

  if ((offset = pushSelf(the, slot, offset, seen)) < 0) {
    debug_pop();
    return -offset;
  }
  offset = append(the, offset, &(slot->flag), sizeof(slot->flag));
  offset = dumpID(the, offset, slot->ID);

  switch(slot->kind) {
  case XS_REFERENCE_KIND: {
    if (slot->value.reference
        && slot->value.reference == mxArrayPrototype.value.reference) {
      fprintf(stderr, "found Array.prototype! slot=%p reference=%p\n",
              slot, slot->value.reference);
    }
    if (slot->value.reference
        && slot->value.reference->value.instance.prototype == mxArrayPrototype.value.reference) {
      fprintf(stderr, "found Array! slot=%p reference=%p prototype=%p\n",
              slot, slot->value.reference, slot->value.reference->value.instance.prototype);
    }
    debug_push(">reference");
    offset = dumpSlot(the, offset, slot->value.reference, seen, KEEP_NEXT);
    debug_pop();
  } break;
          /*
	case XS_CLOSURE_KIND: {
		fprintf(file, ".kind = XS_CLOSURE_KIND}, ");
		fprintf(file, ".value = { .closure = ");
		fxPrintAddress(the, file, slot->value.closure);
		fprintf(file, " } ");
	} break;
          */
  case XS_INSTANCE_KIND: {
    // ISSUE: fprintf(file, ".value = { .instance = { NULL, ");
    debug_push(">prototype");
    offset = dumpSlot(the, offset, slot->value.instance.prototype, seen, KEEP_NEXT);
    debug_pop();
  } break;
  case XS_ARRAY_KIND: {
    txSlot *item = slot->value.array.address;
    txInteger size = (txInteger)fxGetIndexSize(the, slot);
    offset = append(the, offset, &size, sizeof(size));
    while (size) {
      // ISSUE: XS_MARK_FLAG??
      debug_push(">item[%d]", size);
      offset = dumpSlot(the, offset, item, seen, SKIP_NEXT);
      debug_pop();
      item++;
      size--;
    }
    skip_next = 1;
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
            offset = append(the, offset, slot->value.code.address, chunk->size - sizeof(txChunk));
          }
          offset = dumpSlot(the, offset, slot->value.code.closures, seen, KEEP_NEXT);
	} break;
	case XS_CODE_X_KIND: {
          offset = append(the, offset, &slot->value.code.address, sizeof(slot->value.code.address));
          offset = dumpSlot(the, offset, slot->value.code.closures, seen, KEEP_NEXT);
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
	case XS_MODULE_KIND: {
		fprintf(file, ".kind = XS_MODULE_KIND}, ");
		fprintf(file, ".value = { .module = { ");
		fxPrintAddress(the, file, slot->value.module.realm);
		fprintf(file, ", %d } }", slot->value.module.id);
	} break;
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
    offset = dumpSlot(the, offset, slot->value.accessor.getter, seen, KEEP_NEXT);
    debug_pop();
    debug_push(">setter");
    offset = dumpSlot(the, offset, slot->value.accessor.setter, seen, KEEP_NEXT);
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
    debug_push(">home-object");
    offset = dumpSlot(the, offset, slot->value.home.object, seen, KEEP_NEXT);
    debug_pop();
    debug_push(">home-module");
    offset = dumpSlot(the, offset, slot->value.home.module, seen, KEEP_NEXT);
    debug_pop();
  } break;
          /*
	case XS_EXPORT_KIND: {
		fprintf(file, ".kind = XS_EXPORT_KIND}, ");
		fprintf(file, ".value = { .export = { ");
		fxPrintAddress(the, file, slot->value.export.closure);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.export.module);
		fprintf(file, " } }");
	} break;
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

  if (!skip_next) {
    debug_push(">next");
    offset = dumpSlot(the, offset, slot->next, seen, KEEP_NEXT);
    debug_pop();
  }
  debug_pop();
  return offset;
}


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

  xsSlot exit0 = xsGetAt(xsArg(1), xsInteger(0));
  fprintf(stderr, "exits[0] type:%d kind:%d reference:%p\n", xsTypeOf(exit0), exit0.kind, exit0.value.reference);

  xsIntegerValue exitQty = xsToInteger(xsGet(xsArg(1), xsID("length")));
  xsIntegerValue seen = exitQty - 1;  // IDEA: use xsVars() for seen too
  xsVars(6);
  // xsVar(0) is snapshot serialization
  xsVar(0) = xsArrayBuffer(NULL, exitQty);
  // xsVar(1) is for seen.toString() temp space
  // xsVar(2) thru 5 are for enumerating exits

  debug_push(">root");
  xsIntegerValue size = dumpSlot(the, exitQty, (txSlot*)&root, // ISSUE: xsSlot -> txSlot???
                                 &seen, KEEP_NEXT);
  debug_pop();
  fprintf(stderr, "dump: xsSetArrayBufferLength(size=%d)\n", size);
  xsSetArrayBufferLength(xsVar(0), size);
  xsResult = xsVar(0);
}
