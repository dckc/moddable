/* global trace */


function tohex(buffer, limit) {
    if (typeof limit === 'number') {
        buffer = buffer.slice(0, limit);
    }
    return Array.prototype.map.call(buffer, x => ('00' + x.toString(16)).slice(-2)).join('');
}

export class Snapshot @ "Snapshot_prototype_destructor" {
    constructor() @ "Snapshot_prototype_constructor";

    dump(root, exits) @ "Snapshot_prototype_dump";

    tohex(rawbuf, limit) {
        return tohex(new Uint8Array(rawbuf), limit);
    }

    restore(rawbuf, exitQty) {
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
        const u16 = (label) => {
            const x = dv.getUint16(offset, true);
            go(2, label);
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

        function flagid() {
            const flag = u8('flag');
            const id = i16('id');
            const idname = chars('id name') || null;
            trace(`flagid ${flag} ${id} ${idname}\n`);
            return { flag, id, idname };
        }

        function slot(skip_next = false) {
            const kind = i8('kind');
            trace(`slot kind: ${kind} skip_next:${skip_next}\n`);
            if (kind == -2) {
                return null; // NULL
            }
            let value;
            if (kind < 10) { // XS_REFERENCE_KIND; i.e. simple
                const { flag, id, idname } = flagid();
                switch (kind) {
                case 0: // XS_UNDEFINED_KIND
                    value = undefined;
                    break;
                case 1: // XS_NULL_KIND
                    value = null;
                    break;
                case 2: // XS_BOOLEAN_KIND
                    value = !!u32('bool');
                    break;
                case 3: // XS_INTEGER_KIND
                    value = i32('int');
                    break;
                case 4: // XS_NUMBER_KIND
                    value = double('number');
                    break;
                case 5: // XS_STRING_KIND
                case 6: // XS_STRING_X_KIND
                    value = chars('string');
                    break;
                default:
                    // TODO: Symbol, BigInt, ...
                    throw new RangeError(kind);
                }
                const next = skip_next ? null : slot();
                return { kind, flag, id, idname, value, next };
            }

            const delta = i32('delta'); // txIntegerValue is 32bits
            let self;
            if (delta >= 0) {
                if (delta <= exitQty) {
                    value = { exit: delta };
                    trace(`exit: ${delta}\n`);
                } else {
                    self = u64(delta + 4);
                    value = { self, delta };
                    trace(`seen slot: ${delta}, ${self.toString(16).toLowerCase()}\n`);
                }
                return value;
            } else {
                self = u64go('self');
                trace(`fresh slot: ${delta}, ${self.toString(16).toLowerCase()}\n`);
            }
            const { flag, id, idname } = flagid();
            switch (kind) {
            case 10: // XS_REFERENCE_KIND
                const reference = slot();
                value = { reference };
                break;
            case 13: // XS_INSTANCE_KIND
                const prototype = slot();
                value = { garbage: null, prototype };
                break;
            case 16: // XS_ARRAY_KIND
                let size = u32('array size');
                value = [];
                while (size > 0) {
                    trace(`array items to do: ${size}\n`);
                    value.push(slot(true));
                    size -= 1;
                }
                skip_next = true;
                break;
            case 20: // XS_CODE_X_KIND
                value = ['TODO: XS_CODE_X_KIND', self];
                // const address = u64go('code.address');
                // const closures = slot();
                // value = { address, closures };
                break;
            case 37: // XS_ACCESSOR_KIND
                const getter = slot();
                const setter = slot();
                value = { getter, setter };
                break;
            case 41: // XS_HOME_KIND
                value = ['TODO: XS_HOME_KIND', self]
                // const object = slot();
                // const module = slot();
                // value = { object, module };
                break;
            case 48: // XS_CALLBACK_X_KIND
                const cb_addr = u64go('callback.address');
                value = { address: cb_addr };
                break;
            default:
                // TODO: lots
                throw new RangeError(kind);
            }
            const next = skip_next ? null : slot();
            // trace(`compound: ${JSON.stringify({ kind, self, flag, id, idname, value, next }, null, 2)}\n`);
            return { kind, self, flag, id, idname, value, next };
        }
        return slot();
    }
}
