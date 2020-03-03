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
    {
        input: undefined,
        parts: [
            '00', // kind
            '00', // flag
            '0000', '00000000',  // ID, ID name length
            'FE', // next is NULL ("kind" -2)
        ],
        struct: { next: null, kind: 0, flag: 0, id: 0 }
    },
    {
        input: null,
        struct: { next: null, kind: 1, flag: 0, id: 0, value: null },
    },
    {
        input: true,
        struct: { next: null, kind: 2, flag: 0, id: 0, value: true },
    },
    {
        input: 1,
        struct: { next: null, kind: 3, flag: 0, id: 0, value: 1 },
    },
    {
        input: 1.5,
    },
    {
        input: 'Hello World',
        struct: { next: null, kind: 6, flag: 0, id: 0, value: "Hello World" },
    },
    {
        input: 'Hello World'.repeat(20),
        parts: [
            '05',  // kind
            '00', '0000', '00000000', // flag, id, id name length
            'DC000000', // string value length 220
            '48656C6C6F20576F726C6448656C6C6F20576F726C64', // string value (repeat 0, 1)
            '48656C6C6F20576F726C6448656C6C6F20576F726C64', // string value (cont.)
            '48656C6C6F20576F726C6448656C6C6F20576F726C64',
            '48656C6C6F20576F726C6448656C6C6F20576F726C64',
            '48656C6C6F20576F726C6448656C6C6F20576F726C64',
            '48656C6C6F20576F726C6448656C6C6F20576F726C64',
            '48656C6C6F20576F726C6448656C6C6F20576F726C64',
            '48656C6C6F20576F726C6448656C6C6F20576F726C64',
            '48656C6C6F20576F726C6448656C6C6F20576F726C64',
            '48656C6C6F20576F726C6448656C6C6F20576F726C64',
            'FE',  // NULL
        ]
    },
    {
        input: [1, 2],
    },
    {
        input: { x: 1 },
    }
];

export default function main() {
    const exits = [Object.prototype, Array.prototype, String.prototype, true, 1];
    const cycle = [1];
    cycle.push(cycle);
    cases.push({ input: cycle });
    for (const { input: root, parts } of cases) {
        const s1 = new Snapshot();
        trace(`calling Snapshot.dump(type ${typeof root}) ${cases.length}...\n`);
        const rawbuf = s1.dump(root, exits);

        const actual = s1.tohex(rawbuf);
        trace(`snapshot: ${rawbuf.byteLength} 0x${actual}\n`);
        if (typeof parts != 'undefined') {
            const expected = '00'.repeat(exits.length) + parts.join('');
            if (actual !== expected) {
                trace(`FAIL: expected [${expected}].\n`);
            }
        }
        const info = traceError(() => s1.restore(rawbuf, exits.length));
        const { self, next, kind, flag, id, value } = info;
        trace(`snapshot value: ${JSON.stringify({ self, next, kind, flag, id, value }, null, 2)}\n`);
    }
}
