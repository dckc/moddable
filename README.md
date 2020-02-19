context: https://github.com/Agoric/agoric-sdk/issues/511

example run:

```
-*- mode: compilation; default-directory: "~/projects/moddable/examples/js/snapshots/" -*-
Compilation started at Tue Feb 18 23:24:25

make run
make: Circular build <- build/bin/lin/debug/snapshots dependency dropped.
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
snapshot: 0x49440000464C00537404000000726F6F74
op0: ID = 0 FL = 0 St 4 root
main() returned immediate value (not a promise). exiting

Compilation finished at Tue Feb 18 23:24:26
```
