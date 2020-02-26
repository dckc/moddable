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

    load(rawbuf, exitQty) {
        const alldata = new Uint8Array(rawbuf);
        let data = alldata.slice(exitQty);

        function go(qty, label) {
            const used = data.slice(0, qty);
            data = data.slice(qty);
            trace(`go(${qty}, ${label}) ${tohex(used)} => ${tohex(data, 16)}\n`);
        }

        const u8 = (label) => {
            const x = data[0];
            go(1, label);
            return x;
        };
        const i8 = (label) => {
            const u = u8(label);
            return u >= 0x80 ? (-1 - ((~u) & 0x7f)) : u;
        };
        const u16 = (label) => {
            const x = data[0] + data[1] * 0x100;
            go(2, label);
            return x;
        };
        const i16 = (label) => {
            const u = u16(label);
            return u >= 0x8000 ? (-1 - ((~u) & 0x7fff)) : u;
        };
        const u32 = (label) => {
            const x = data[0] + 0x100 * (data[1] + 0x100 * (data[2] + 0x100 * data[3]));
            go(4, label);
            return x;
        };
        const i32 = (label) => {
            const u = u32(label);
            // trace(`i32: ${u >= 0x80000000} ? ${(~u - 1)} : ${u}\n`);
            return u >= 0x80000000 ? (-1 - ((~u) & 0x7fffffff)) : u;
        };
        const chars = (label) => {
            const len = u32('len');
            const s = [...data.slice(0, len)].map(b => String.fromCharCode(b)).join('');
            go(len, label);
            return s;
        };
        const u64 = data => data[0] + 0x100 * (data[1] + 0x100 *
                                               (data[2] + 0x100 * (data[3] + 0x100 *
                                                 (data[4] + 0x100 * (data[5] + 0x100 *
                                                   (data[6] + 0x100 * data[7]))))));
        const u64go = (label) => {
            const x = u64(data);
            go(8, label);
            return x;
        };

        function flagid() {
            const flag = u8('flag');
            const id = i16('id');
            let idname = null;
            if (id != 0 && id != -1 && id != 3) { // ISSUE: 3???
                idname = chars('id name');
            }
            trace(`flagid ${flag} ${id} ${idname}\n`);
            return { flag, id, idname };
        }

        function slot() {
            const kind = i8('kind');
            trace(`slot kind: ${kind}\n`);
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
                case 2: // XS_BOOLEAN_KIND
                    value = !!u8('bool');
                    break;
                case 3: // XS_INTEGER_KIND
                    value = i32('int');
                    break;
                case 5: // XS_STRING_KIND
                case 6: // XS_STRING_X_KIND
                    value = chars('string');
                    break;
                default:
                    throw new RangeError(kind);
                }
                const next = slot();
                return { kind, flag, id, idname, value, next };
            }

            const delta = i32('delta'); // txIntegerValue is 32bits
            let self;
            if (delta > 0) {
                self = u64(alldata.slice(delta, delta + 8));
                trace(`seen slot: ${delta}, ${self.toString(16).toLowerCase()}\n`);
                return { self };
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
                value = [ null, 0 ]; //??
                break;
            case 20: // XS_CODE_X_KIND
                const address = u64go('code.address');
                const closures = slot();
                value = { address, closures };
                break;
            case 37: // XS_ACCESSOR_KIND
                const getter = slot();
                const setter = slot();
                value = { getter, setter };
                break;
            case 41: // XS_HOME_KIND
                const object = slot();
                const module = slot();
                value = { object, module };
                break;
            case 48: // XS_CALLBACK_X_KIND
                const cb_addr = u64go('callback.address');
                value = { address: cb_addr };
                break;
            default:
                throw new RangeError(kind);
            }
            const next = slot();
            trace(`compound: ${JSON.stringify({ kind, self, flag, id, idname, value, next }, null, 2)}\n`);
            return { kind, self, flag, id, idname, value, next };
        }
        return slot();
    }
}
