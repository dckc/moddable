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
        'Hello World',
        'Hello World'.repeat(100),
    ];
    trace(`${JSON.stringify({ root })}\n`);

    const s1 = new Snapshot();
    const rawbuf = s1.dump(root, []);
    trace(`snapshot: 0x${s1.tohex(rawbuf, 128)}\n`);
    const info = traceError(() => s1.load(rawbuf));
    const { self, next, kind, flag, id, value } = info;
    trace(`snapshot value: ${JSON.stringify({ self, next, kind, flag, id, value })}\n`);
}
