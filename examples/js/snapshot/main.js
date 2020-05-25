/* global Compartment, trace, snapshot, globalThis */
// @flow

trace(`in main module\n`);

import { Snapshot } from './snapshot_restore';

function traceError/*::<T>*/(thunk/*: () => T*/) /*: T */{
    try {
        return thunk();
    } catch (err) {
        if (err instanceof Error) {
            trace(`Error: ${err.message}\n`);
        }
        throw err;
    }
}

/*::
import type { Slot } from './snapshot_restore';

type TestCase = {
    input: mixed,
    note?: string,
    parts?: string[],
    struct?: Slot
}
*/

const cases /*: TestCase[] */ = [
    {
        input: undefined,
        note: 'undefined scalar',
        parts: [
            '00', // kind
            '00', // flag
            '0000', '00000000',  // ID, ID name length
            'FE', // next is NULL ("kind" -2)
        ],
        struct: { kind: 0, flag: 0, id: 0 }
    },
    {
        input: null,
        note: 'null scalar',
        struct: { kind: 1, flag: 0, id: 0, value: null },
    },
    {
        input: true,
        note: 'boolean scalar',
        struct: { kind: 2, flag: 0, id: 0, value: true },
    },
    {
        input: 1,
        note: 'int scalar',
        struct: { kind: 3, flag: 0, id: 0, value: 1 },
    },
    {
        input: 1.5,
        note: 'float scalar',
    },
    {
        input: 2.5,
        note: 'another float scalar',
    },
    {
        input: 'Hello World',
        note: 'string scalar',
        struct: { kind: 5, flag: 0, id: 0, value: "Hello World" },
    },
    {
        input: 'Hello World'.repeat(20),
        note: 'longer string scalar',
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
        note: 'array of 2 ints',
    },
    {
        note: 'object { x: 1 }',
        input: { x: 1 },
    }
];

const fnCases /*: TestCase[] */ = [
    {
        input: (1, eval)(`
        (function makeCounter() {
            let count = 1;
            return harden({
                incr() {  // <-- what does this thing's .closure slot store? [2]?
                    return ++count;
                },
                decr() {  // <-- what does this thing's .closure slot store? [2] ?
                    return ++count;
                }
            });
        })
        `),
        note: 'record of closures with shared var',
    },
    {
        // we use eval() to avoid functions from ROM
        input: (1, eval)(`(function f1(x, y, z) { return 0; })`),
        note: 'function f() { ... }',
    },
    {
        input: (1, eval)(`() => 1`),
        note: 'arrow function',
    },
    {
        input: eval(`(function f1(x, y, z) { return 0; })`),
        note: 'direct eval',
    },
];

function assert(condition) {
    if (!condition) {
        throw new Error('assert!');
    }
}

export function stringifyCycles(v /*: mixed*/) {
  const cache = new Map();
  return JSON.stringify(v, function (key, value) {
    if (typeof value === 'object' && value !== null) {
      if (cache.get(value)) {
        // Circular reference found, discard key
        return'*CYCLE*';
      }
      // Store value in our collection
      cache.set(value, true);
    }
    return value;
  });
}


export default function main() {
    const exits = [Object.prototype, Array.prototype, String.prototype, Function.prototype,
                   Snapshot,
                   traceError,
                   globalThis];
    const s0 = new Snapshot();
    const slot = 123;
    const actual = s0.encodeSlot(slot, exits);

    trace(`typeof actual: ${typeof actual} ArrayBuffer? ${actual instanceof ArrayBuffer}\n`);
    trace(`root: ${slot} encodedSlot: ${s0.tohex(actual)}\n`);


    return;
    // TODO...
    const cycle = [1, 'b', 'c'];
    cycle.push(cycle);
    cases.push({ input: cycle, note: 'cycle' });
    for (const aCase of cases.concat(...fnCases)) {
        const { input: root, parts, note } = aCase;
        const s1 = new Snapshot();
        trace(`\n\n\n======= ${note}\n--calling Snapshot.dump(type ${typeof root})\ncase keys: ${Object.keys(aCase).toString()}...\n`);
        const rawbuf = s1.dump(root, exits);

        const actual = s1.tohex(rawbuf);
        trace(`snapshot: ${rawbuf.byteLength} 0x${actual}\n`);
        if (typeof parts != 'undefined' && parts !== null) {
            const expected = '00'.repeat(exits.length) + parts.join('');
            if (actual !== expected) {
                trace(`FAIL: expected [${expected}].\n`);
            }
        }
        const info = traceError(() => s1.restore(rawbuf, exits.length));
        // $FlowFixMe -- thinks undefined shouldn't be stringified
        trace(`snapshot value: ${JSON.stringify(info, null, 2)}\n`);
        const built = s1.rebuild(info, exits);
        assert(typeof built === typeof root);
        trace(`rebuild ok? ${stringifyCycles(built)} =?= ${stringifyCycles(root)}\n`);
    }
}
