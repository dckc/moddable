/* global trace */
// @flow strict

/*::
interface SlotId {
    flag: number;
    id: number;
    idname?: ?string;
}

interface SlotScalar extends SlotId {
    next?: SlotOpt;
}
interface SlotUndefined extends SlotScalar {
    kind: 0; value?: 'undefined';
}
interface SlotNull extends SlotScalar {
    kind: 1; value: null;
}
interface SlotBoolean extends SlotScalar {
    kind: 2; value: boolean
}
interface SlotInteger extends SlotScalar {
    kind: 3; value: number
}
interface SlotNumber extends SlotScalar {
    kind: 4; value: number
}
interface SlotString extends SlotScalar {
    kind: 5; value: string
}

interface SlotComplex extends SlotId {
    next?: SlotOpt;
    self?: number;
}
interface SlotReference extends SlotComplex {
    kind: 10; value: { reference: SlotOpt }
}
interface SlotClosure extends SlotComplex {
    kind: 11; value: { closure: SlotOpt }
}
interface SlotInstance extends SlotComplex {
    kind: 13; value: { garbage: null, prototype: SlotOpt, properties: Slot[] }
}
interface SlotArray extends SlotComplex {
    kind: 16; value: Slot[]
}
interface SlotCode extends SlotComplex {
    kind: 19; value: {
        code: string, // Hex
        closures: SlotOpt
    }
}
interface SlotAccessor extends SlotComplex {
    kind: 37; value: { getter: SlotOpt, setter: SlotOpt }
}
interface SlotCallback extends SlotComplex {
    kind: 48; value: { address: number }
}

type Exit = {|
    exit: number;
    kind: 101;
|}
type Ibid = Exit | {|
    self: number;
    delta: number;
    kind: 102;
|}

export type Slot = (
    SlotUndefined | SlotNull | SlotBoolean | SlotInteger | SlotNumber | SlotString |
    SlotReference | SlotClosure | SlotInstance | SlotArray | SlotCode | SlotAccessor | SlotCallback |
    Ibid
);
export type SlotOpt = Slot | null;
*/

function tohex(buffer, limit /*:: ?: number */) /*: string */ {
    const buflimit = typeof limit === 'number' ? buffer.slice(0, limit) : buffer;
    return Array.prototype.map.call(buffer, x => ('00' + x.toString(16)).slice(-2)).join('');
}

import { SnapshotFFI } from 'snapshot';

export class Snapshot extends SnapshotFFI {
    constructor() /*: Snapshot */ {
        super();
        return this;
    }
    tohex(rawbuf /*: ArrayBuffer */, limit /*:: ?: number*/) {
        return tohex(new Uint8Array(rawbuf), limit);
    }

    restore(rawbuf /*: ArrayBuffer */, exitQty /*: number */) /*: Slot */ {
        const dv = new DataView(rawbuf);
        const alldata = new Uint8Array(rawbuf);
        let data = alldata.slice(exitQty);
        let offset = exitQty;

        function go(qty, label) {
            const used = data.slice(0, qty);
            data = data.slice(qty);
            offset += qty;
            trace(`go(${qty}, ${label}) ${tohex(used)} => @${offset} ${tohex(data, 16)}\n`);
        }

        // IDEA/TODO: use DataView.getUint8 etc.
        const u8 = (label) => {
            const x = dv.getUint8(offset);
            go(1, label);
            return x;
        };
        const i8 = (label) => {
            const x = dv.getInt8(offset);
            go(1, label);
            return x;
        };
        const i16 = (label) => {
            const x = dv.getInt16(offset, true);
            go(2, label);
            return x;
        };
        const u32 = (label) => {
            const x = dv.getUint32(offset, true);
            go(4, label);
            return x;
        };
        const i32 = (label) => {
            const x = dv.getInt32(offset, true);
            go(4, label);
            // trace(`i32: ${u >= 0x80000000} ? ${(~u - 1)} : ${u}\n`);
            return x;
        };
        const chars = (label) => {
            const len = u32('len');
            const s = [...data.slice(0, len)].map(b => String.fromCharCode(b)).join('');
            go(len, label);
            return s;
        };
        // ISSUE: overflow
        const u64 = (offset) => dv.getUint32(offset, true) + 0x100000000 * dv.getUint32(offset + 4, true);
        const u64go = (label) => {
            const x = u64(offset);
            go(8, label);
            return x;
        };
        const double = (label) => {
            const x = dv.getFloat64(offset, true);
            go(8, label);
            return x;
        };

        function flagid() /*: SlotId */ {
            const flag = u8('flag');
            const id = i16('id');
            const idname = chars('id name') || null;
            trace(`flagid ${flag} ${id} ${String(idname)}\n`);
            return { flag, id, idname };
        }

        function slot() /*: Slot */ {
            const kind = i8('kind');
            trace(`slot kind: ${kind}\n`);
            if (kind < -1) {
                throw new TypeError(`bad slot kind: ${kind}`);
            }
            return slotValue(kind);
        }
        function slotOpt() /*: SlotOpt */ {
            const kind = i8('kind');
            trace(`slotOpt kind: ${kind}\n`);
            if (kind < -1) {
                return null; // NULL
            }
            return slotValue(kind);
        }
        function slotValue(kind) /*: Slot */ {
            trace(`slot kind: ${kind}\n`);
            if (kind < 10) { // XS_REFERENCE_KIND; i.e. simple
                const { flag, id, idname: idname_ } = flagid();
                const idname = idname_ || null;
                switch (kind) {
                case 0: // XS_UNDEFINED_KIND
                    return { kind, flag, id, idname };
                case 1: // XS_NULL_KIND
                    return { kind, flag, id, idname, value: null };
                case 2: // XS_BOOLEAN_KIND
                    return { kind: 2, flag, id, idname, value: !!u32('bool') };
                case 3: // XS_INTEGER_KIND
                    return { kind: 3, flag, id, idname, value: i32('int') };
                case 4: // XS_NUMBER_KIND
                    return { kind: 4, flag, id, idname, value: double('number') };
                case 5: // XS_STRING_KIND
                case 6: // XS_STRING_X_KIND
                    return { kind: 5, flag, id, idname, value: chars('string') };
                default:
                    // TODO: Symbol, BigInt, ...
                    throw new RangeError(kind);
                }
            }

            const delta = i32('delta'); // txIntegerValue is 32bits
            let self;
            if (delta >= 0) {
                let ibid /*: Ibid */;
                if (delta <= exitQty) {
                    ibid = { exit: delta, kind: 101 };
                    trace(`exit: ${delta}\n`);
                } else {
                    self = u64(delta + 4);
                    ibid = { self, delta, kind: 102 };
                    trace(`seen slot: ${delta}, ${self.toString(16).toLowerCase()}\n`);
                }
                return ibid;
            } else {
                self = u64go('self');
                trace(`fresh slot: ${delta}, ${self.toString(16).toLowerCase()}\n`);
            }
            const { flag, id, idname: idname_ } = flagid();
            const idname = idname_ || null;
            switch (kind) {
            case 10: // XS_REFERENCE_KIND
                const reference = slotOpt();
                return { kind: 10, self, flag, id, idname, value: { reference } };
            case 11: // XS_CLOSURE_KIND
                const closure = slotOpt();
                return { kind: 11, self, flag, id, idname, value: { closure } };
            case 13: // XS_INSTANCE_KIND
                const prototype = slotOpt();
                const properties = slotList();
                return { kind: 13, self, flag, id, idname, value: { garbage: null, prototype, properties } };
            case 16: // XS_ARRAY_KIND
                let size = u32('array size');
                const items = [];
                while (size > 0) {
                    trace(`array items to do: ${size}\n`);
                    items.push(slot());
                    size -= 1;
                }
                return { kind: 16, self, flag, id, idname, value: items };
            case 19: // XS_CODE_KIND
                const codeSize = u32('code size');
                const code = tohex(data.slice(0, codeSize));
                go(codeSize, 'code');
                const closures = slotOpt();
                return { kind: 19, self, flag, id, idname, value: { code, closures } };
            case 20: // XS_CODE_X_KIND
                // const address = u64go('code.address');
                // const closures = slot();
                // value = { address, closures };
                return { kind: 19, self, flag, id, idname, value: ['TODO: XS_CODE_X_KIND', self] };
            case 37: // XS_ACCESSOR_KIND
                const getter = slotOpt();
                const setter = slotOpt();
                return { kind: 37, self, flag, id, idname, value: { getter, setter } };
            case 41: // XS_HOME_KIND
                // const object = slot();
                // const module = slot();
                // value = { object, module };
                return { kind: 19, self, flag, id, idname, value: ['TODO: XS_HOME_KIND', self] };
            case 48: // XS_CALLBACK_X_KIND
                const cb_addr = u64go('callback.address');
                return { kind: 48, self, flag, id, idname, value: { address: cb_addr } };
            default:
                // TODO: lots
                throw new RangeError(kind);
            }
        }

        function slotList() /*: Slot[] */ {
            const items = [];
            while (1) {
                const next = slotOpt();
                if (next === null) {
                    break;
                }
                items.push(next);
            }
            return items;
        }

        return slot();
    }
}
