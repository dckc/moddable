/* global Compartment, trace */

trace(`in vat1 module\n`);

self.onmessage = (msg) => {
    self.postMessage("pong");
};
