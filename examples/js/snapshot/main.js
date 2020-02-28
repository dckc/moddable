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

const cases = [
    { x: 1 },
    undefined,
    null,
    true,
    1,
    // TODO: 1.5,
    'Hello World',
    'Hello World'.repeat(20),
    [1, 2],
];

export default function main() {
    const exits = [Object.prototype, Array.prototype, String.prototype, true, 1];
    const cycle = [1];
    cycle.push(cycle);
    cases.push(cycle);
    for (const root of cases) {
        const s1 = new Snapshot();
        trace(`calling Snapshot.dump(type ${typeof root})...\n`);
        const rawbuf = s1.dump(root, exits);

        trace(`snapshot: 0x${s1.tohex(rawbuf, 128)}\n`);
        const info = traceError(() => s1.restore(rawbuf, exits.length));
        const { self, next, kind, flag, id, value } = info;
        trace(`snapshot value: ${JSON.stringify({ self, next, kind, flag, id, value }, null, 2)}\n`);
    }
}
