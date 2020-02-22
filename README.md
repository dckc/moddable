context: https://github.com/Agoric/agoric-sdk/issues/511

example run:

```
-*- mode: compilation; default-directory: "~/projects/moddable/examples/js/snapshots/" -*-
Compilation started at Sat Feb 22 10:38:12

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
{"root":"Hello World"}
ensureSpace: 0x7ffe90564240, old capacity 0; new size: 4; nextQuantum: 256 (kind: 10 ref: 0x55fe48880b50 next: 0x55fe48880b70 kind: 17)
ensureSpace: after set length: 0x7ffe90564240, (kind: 10 ref: 0x55fe48880b50 next: 0x55fe48880b70 kind: 17)
ensureSpace: fxSetArrayBufferLength() done. 0x7ffe90564240 length = 256
append: xsSetArrayBufferData(buf=0x7ffe90564240, offset=0, qty=4)
append: xsSetArrayBufferData() done.
ensureSpace: 0x7ffe90564240, capacity 256 sufficient for size: 8
append: xsSetArrayBufferData(buf=0x7ffe90564240, offset=4, qty=4)
append: xsSetArrayBufferData() done.
ensureSpace: 0x7ffe90564240, capacity 256 sufficient for size: 19
append: xsSetArrayBufferData(buf=0x7ffe90564240, offset=8, qty=11)
append: xsSetArrayBufferData() done.
dump: xsSetArrayBufferLength(size=19)
dump: xsSetArrayBufferLength() done.
snapshot: 0x060000000B00000048656C6C6F20576F726C64
{"root":"Hello World","snapshot":{"kind":6,"flag":0,"id":0,"len":11,"txt":"Hello World"}}
main() returned immediate value (not a promise). exiting

Compilation finished at Sat Feb 22 10:38:12
```
