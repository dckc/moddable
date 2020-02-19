/* global Compartment, trace, snapshot */

trace(`in main module\n`);

import { Snapshot } from 'snapshot';

export default async function main() {
    const s1 = new Snapshot();
    const data = s1.dump('root', []);
    trace(`snapshot: ${data}\n`);
}
