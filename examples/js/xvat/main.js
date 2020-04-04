/* global trace */

import Worker from "worker";

trace("main module hello\n");

function mkPromise() {
    let resolve, reject;
    const promise = new Promise((win, lose) => { resolve = win; reject = lose; });
    return { promise, resolve, reject };
}

export default function main() {
    let index = 0;

    // note: allocation and stackCount are very small - most real workers will require a larger allocation and stackCount
    let aWorker = new Worker("alice", { allocation: 6 * 1024, stackCount: 64, slotCount: 32 });
    let bWorker = new Worker("bob", { allocation: 6 * 1024, stackCount: 64, slotCount: 32 });
    let mWorker = new Worker("mallet", { allocation: 6 * 1024, stackCount: 64, slotCount: 32 });

    const aliceP = mkPromise();
    const bobP = mkPromise();

    aWorker.postMessage({ hello: "world", index: ++index });
    bWorker.postMessage("hello, again");
    mWorker.postMessage([1, 2, 3]);

    aWorker.onmessage = function (message) {
        trace(`alice says: ${message}\n`);
        aliceP.resolve(message);
    };

    bWorker.onmessage = function (message) {
        trace(`bob says: ${message}\n`);
        bobP.resolve(message);
    };

    return Promise.all([aliceP.promise, bobP.promise]);
}
