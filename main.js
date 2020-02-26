/* global Compartment, trace, snapshot */

trace(`in main module\n`);

import { Snapshot } from 'snapshot';

function traceError(thunk) {
    try {
        return thunk();
    } catch (err) {
        trace(`Error: ${err.message}\n`);
        throw err;
    }
}

export default function main() {
    const root = [
        1, 2, 3,
        'Hello World',
        'Hello World'.repeat(20),
    ];

    trace(`root type: ${ typeof root }\n`); // JSON.stringify could run into circular structures.
    trace(`root type: ${JSON.stringify(root.map(e => typeof e))}\n`);
    const exits = [Object.prototype, Array.prototype, String.prototype, true, 1];
    trace(`[] prototype in exits? ${exits.indexOf(root.__proto__)}\n`);
    const s1 = new Snapshot();
    const rawbuf = s1.dump(root, exits);
    trace(`snapshot: 0x${s1.tohex(rawbuf, 128)}\n`);
    const info = traceError(() => s1.restore(rawbuf, exits.length));
    const { self, next, kind, flag, id, value } = info;
    trace(`snapshot value: ${JSON.stringify({ self, next, kind, flag, id, value }, null, 2)}\n`);
}
