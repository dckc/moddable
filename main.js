/* global Compartment, trace, snapshot */

trace(`in main module\n`);

import { Snapshot } from 'snapshot';

function buf2hex(buffer /*: Uint8Array */ ) {
  return Array.prototype.map.call(buffer, x => ('00' + x.toString(16)).slice(-2)).join('');
}

export default function main() {
    const s1 = new Snapshot();
    const data = new Uint8Array(s1.dump('root', []));
    trace(`snapshot: 0x${buf2hex(data)}\n`);
    const chars = bytes => [...bytes].map(b => String.fromCharCode(b)).join('');
    const u16 = bytes => bytes[0] + bytes[1] * 0x100;
    const u32 = bytes => bytes[0] + 0x100 * (bytes[1] + 0x100 * (bytes[2] + 0x100 * bytes[3]));
    const op1 = chars(data.slice(0, 2));
    const id = u16(data.slice(2, 4));
    const op2 = chars(data.slice(4, 6));
    const flags = data.slice(6, 7)[0];
    const op3 = chars(data.slice(7, 9));
    const len = u32(data.slice(9, 13));
    const txt = chars(data.slice(13, 13 + len));
    trace(`op0: ${op1} = ${id} ${op2} = ${flags} ${op3} ${len} ${txt}\n`);
}
