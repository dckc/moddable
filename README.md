context: https://github.com/Agoric/agoric-sdk/issues/511

example run:

```
-*- mode: compilation; default-directory: "~/projects/moddable/examples/js/snapshots/" -*-
Compilation started at Tue Feb 18 20:02:55

make run
make: Circular build <- build/bin/lin/debug/snapshots dependency dropped.
mcconfig -d -o ./build -m -p x-cli-lin
make[1]: Entering directory '/home/connolly/projects/moddable/examples/js/snapshots'
# xsc snapshot.xsb
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
snapshot: hello from dump!
 lin_xs_cli: main() returned a promise; entering event loop

Compilation finished at Tue Feb 18 20:02:56
```
