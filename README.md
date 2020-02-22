context: https://github.com/Agoric/agoric-sdk/issues/511

example run:

```
-*- mode: compilation; default-directory: "~/projects/moddable/examples/js/snapshots/" -*-
Compilation started at Sat Feb 22 10:39:04

make run
mcconfig -d -o ./build -m -p x-cli-lin
make[1]: Entering directory '/home/connolly/projects/moddable/examples/js/snapshots'
# xsc main.xsb
# xsl modules
# xsl modules
# cc mc.xs.c
# cc snapshots
make[1]: Leaving directory '/home/connolly/projects/moddable/examples/js/snapshots'
./build/bin/lin/debug/snapshots
lin_xs_cli: loading top-level main.js
in main module
 lin_xs_cli: loaded
lin_xs_cli: invoking main(argv)
{"root":"Hello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello World"}
ensureSpace: 0x7ffc08ac63a0, old capacity 0; new size: 4; nextQuantum: 256 (kind: 10 ref: 0x556b76a5bbf0 next: 0x556b76a5bbd0 kind: 17)
ensureSpace: after set length: 0x7ffc08ac63a0, (kind: 10 ref: 0x556b76a5bbf0 next: 0x556b76a5bbd0 kind: 17)
ensureSpace: fxSetArrayBufferLength() done. 0x7ffc08ac63a0 length = 256
append: xsSetArrayBufferData(buf=0x7ffc08ac63a0, offset=0, qty=4)
append: xsSetArrayBufferData() done.
ensureSpace: 0x7ffc08ac63a0, capacity 256 sufficient for size: 8
append: xsSetArrayBufferData(buf=0x7ffc08ac63a0, offset=4, qty=4)
append: xsSetArrayBufferData() done.
ensureSpace: 0x7ffc08ac63a0, old capacity 256; new size: 1108; nextQuantum: 1280 (kind: 10 ref: 0x556b76a5bbf0 next: 0x556b76a5bbd0 kind: 17)
ensureSpace: after set length: 0x7ffc08ac63a0, (kind: 10 ref: 0x556b76a5bbf0 next: 0x556b76a5bbd0 kind: 0)
```
