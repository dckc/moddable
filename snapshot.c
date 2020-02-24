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
    // fprintf(stderr, "ensureSpace(%d) grow capacity %d -> %d\n", size, capacity, nextQuantum);
    xsSetArrayBufferLength(*buf, nextQuantum);
  } else {
    // fprintf(stderr, "ensureSpace(%d) capacity %d ok\n", size, capacity);
  }
}


static xsIntegerValue append(xsMachine* the, txSlot* buf, xsIntegerValue offset, void* src, xsIntegerValue qty)
{
  // fprintf(stderr, "append(%d += %d)\n", offset, qty);
  ensureSpace(the, buf, offset + qty);
  xsSetArrayBufferData(*buf, offset, src, qty);
  return offset + qty;
}


/** track "seen" slots

avoid infinite recursion for cyclic structures

Each time we serialize a compound slot, we save either
  - the offset where it was already serialized; or
  - the delta to the previous compound slot and
    the address of this slot
*/
static xsIntegerValue alreadySeen(xsMachine* the, txSlot* target, txSlot* buf, xsIntegerValue seen) {
  fprintf(stderr, "alreadySeen(%p)?\n", target);
  while (seen >= 0) {
    xsIntegerValue delta;
    txSlot* candidate;
    xsGetArrayBufferData(*buf, seen + sizeof(delta), &candidate, sizeof(candidate));
    if (candidate == target) {
      fprintf(stderr, "yes seen %p at %d\n", target, seen);
      return seen;
    }
    xsGetArrayBufferData(*buf, seen, &delta, sizeof(delta));
    seen += delta;
  }
  fprintf(stderr, "no not seen %p\n", target);
  return -1;
}


static xsIntegerValue pushSelf(xsMachine* the, txSlot* self, txSlot* buf, xsIntegerValue offset, xsIntegerValue *seen) {
  xsIntegerValue delta = (*seen >= 0 ? *seen : 0) - offset;
  fprintf(stderr, "pushSelf %p; sizeof(delta) = %ld\n", self, sizeof(delta));
  xsIntegerValue found;
  if ((found = alreadySeen(the, self, buf, *seen)) >= 0) {
    fprintf(stderr, "pushSelf %p; alreadySeen at %d\n", self, found);
    offset = append(the, buf, offset, &found, sizeof(found));
    return -offset;
  }
  *seen = offset;
  fprintf(stderr, "pushSelf(%p) delta=%d *seen=%d\n", self, delta, *seen);
  offset = append(the, buf, offset, &delta, sizeof(delta));
  offset = append(the, buf, offset, &self, sizeof(self));
  return offset;
}

static xsIntegerValue dumpID(xsMachine* the, txSlot* buf, xsIntegerValue offset, txID id)
{
  offset = append(the, buf, offset, &id, sizeof(id));
  if (id != 0 && id != -1) {
    char* value = fxGetKeyName(the, id);
    if (!value) {
      fprintf(stderr, "??? id=%d no name?\n", id);
      return offset;
    }
    txU4 len = strlen(value);
    offset = append(the, buf, offset, &len, sizeof(len));
    offset = append(the, buf, offset, value, len);
  }
  return offset;
}

static xsIntegerValue dumpSimpleValue(xsMachine* the, txSlot* buf, xsIntegerValue offset, txSlot* slot)
{
  fprintf(stderr, "dumpSimpleValue %p kind = %d\n", slot, slot->kind);

  switch(slot->kind) {
  case XS_UNINITIALIZED_KIND:
  case XS_UNDEFINED_KIND:
  case XS_NULL_KIND:
    // ISSUE: .value = { .number = 0 }?
    break;

  case XS_BOOLEAN_KIND: {
    fprintf(stderr, " BOOLEAN = %d\n", slot->value.boolean);
    offset = append(the, buf, offset, &(slot->value.boolean), sizeof(slot->value.boolean));
  } break;

  case XS_INTEGER_KIND: {
    fprintf(stderr, " INTEGER = %d\n", slot->value.integer);
    offset = append(the, buf, offset, &(slot->value.integer), sizeof(slot->value.integer));
  } break;

  case XS_NUMBER_KIND: {
    // ISSUE: assume IEEE double format?
    fprintf(stderr, " INTEGER = %f\n", slot->value.number);
    offset = append(the, buf, offset, &(slot->value.number), sizeof(slot->value.number));
  } break;

  case XS_STRING_KIND:
  case XS_STRING_X_KIND: {
    fprintf(stderr, " STRING = %s\n", slot->value.string);
    txU4 len = strlen(slot->value.string);
    offset = append(the, buf, offset, &len, sizeof(len));
    offset = append(the, buf, offset, slot->value.string, len);
  } break;

  case XS_SYMBOL_KIND: {
    fprintf(stderr, " SYMBOL = %d\n", slot->value.symbol);
    offset = append(the, buf, offset, &(slot->value.symbol), sizeof(slot->value.symbol));
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
    fprintf(stderr, "slot kind not implemented: %d!\n", slot->kind);
    assert(0); //
  }

  return offset;
}


// a la fxPrintSlot
// https://github.com/Moddable-OpenSource/moddable/blob/public/xs/tools/xslSlot.c#L946
// ISSUE: addresses are non-deterministic; exposes runtime details. Snapshot object is hence powerful.
// IDEA: consider protobuf? capnproto?
static xsIntegerValue dumpSlot(xsMachine* the, txSlot* buf, xsIntegerValue offset, txSlot* slot, xsIntegerValue* seen)
{
  if (!slot) {
    txKind null_slot_kind = XS_UNINITIALIZED_KIND - 1;
    fprintf(stderr, "dumpSlot %p\n", slot);
    offset = append(the, buf, offset, &null_slot_kind, sizeof(txKind));
    return offset;
  }

  fprintf(stderr, "dumpSlot %p kind = %d\n", slot, slot->kind);
  offset = append(the, buf, offset, &(slot->kind), sizeof(slot->kind));

  if (slot->kind < XS_REFERENCE_KIND) {
    offset = append(the, buf, offset, &(slot->flag), sizeof(slot->flag));
    offset = dumpID(the, buf, offset, slot->ID);
    offset = dumpSimpleValue(the, buf, offset, slot);
    offset = dumpSlot(the, buf, offset, slot->next, seen);
    return offset;
  }

  if ((offset = pushSelf(the, slot, buf, offset, seen)) < 0) {
    return -offset;
  }
  offset = append(the, buf, offset, &(slot->flag), sizeof(slot->flag));
  offset = dumpID(the, buf, offset, slot->ID);

  switch(slot->kind) {
  case XS_REFERENCE_KIND: {
    fprintf(stderr, "REFERENCE: Array? %p\n", &mxArrayPrototype);
    offset = dumpSlot(the, buf, offset, slot->value.reference, seen);
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
    fprintf(stderr, "INSTANCE: Array? %p\n", &mxArrayPrototype);
    offset = dumpSlot(the, buf, offset, slot->value.instance.prototype, seen);
  } break;
  case XS_ARRAY_KIND: {
    // ISSUE: fprintf(file, ".value = { .array = { NULL, 0 } }");
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
          offset = append(the, buf, offset, slot->value.callback.address, sizeof(slot->value.callback.address));
	} break;
	case XS_CODE_KIND:  {
          // .code =
          {
            txChunk* chunk = (txChunk*)(slot->value.code.address - sizeof(txChunk));
            offset = append(the, buf, offset, slot->value.code.address, chunk->size - sizeof(txChunk));
          }
          offset = dumpSlot(the, buf, offset, slot->value.code.closures, seen);
	} break;
	case XS_CODE_X_KIND: {
          offset = append(the, buf, offset, &slot->value.code.address, sizeof(slot->value.code.address));
          offset = dumpSlot(the, buf, offset, slot->value.code.closures, seen);
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
    offset = dumpSlot(the, buf, offset, slot->value.accessor.getter, seen);
    offset = dumpSlot(the, buf, offset, slot->value.accessor.setter, seen);
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
    offset = dumpSlot(the, buf, offset, slot->value.home.object, seen);
    offset = dumpSlot(the, buf, offset, slot->value.home.module, seen);
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
    fprintf(stderr, "slot kind not implemented: %d!\n", slot->kind);
    assert(0); //
  }

  offset = dumpSlot(the, buf, offset, slot->next, seen);
  return offset;
}


void Snapshot_prototype_dump(xsMachine* the)
{
  // ?? xsIntegerValue c = xsToInteger(xsArgc);
  xsSlot root = xsArg(0);
  // TODO: exits = xsArg(1)
  xsVars(2);
  xsVar(0) = xsArrayBuffer(NULL, 256);  // snapshot serialization
  xsIntegerValue seen = -1;
  xsIntegerValue size = dumpSlot(the, (txSlot*)&(xsVar(0)), 0, (txSlot*)&root, // ISSUE: xsSlot -> txSlot???
                                 &seen);
  fprintf(stderr, "dump: xsSetArrayBufferLength(size=%d)\n", size);
  xsSetArrayBufferLength(xsVar(0), size);
  fprintf(stderr, "dump: xsSetArrayBufferLength() done.\n");
  xsResult = xsVar(0);
}
