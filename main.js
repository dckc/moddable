/* global Compartment, trace, snapshot */

trace(`in main module\n`);

import { Snapshot } from 'snapshot';

function buf2hex(buffer /*: Uint8Array */ ) {
  return Array.prototype.map.call(buffer, x => ('00' + x.toString(16)).slice(-2)).join('');
}

export default function main() {
    const s1 = new Snapshot();
    const root = 'Hello World'.repeat(100);
    trace(`${JSON.stringify({ root })}\n`);
    const rawbuf = s1.dump(root, []);
    const data = new Uint8Array(rawbuf);
    trace(`snapshot: 0x${buf2hex(data)}\n`);
    const chars = bytes => [...bytes].map(b => String.fromCharCode(b)).join('');
    const u16 = bytes => bytes[0] + bytes[1] * 0x100;
    const u32 = bytes => bytes[0] + 0x100 * (bytes[1] + 0x100 * (bytes[2] + 0x100 * bytes[3]));
    const kind = data[0];
    const flag = data[1];
    const id = u16(data.slice(2, 4));
    const len = u32(data.slice(4, 8));
    const txt = chars(data.slice(8, 8 + len));
    trace(`${JSON.stringify({ root, snapshot: { kind, flag, id, len, txt }})}\n`);
}
