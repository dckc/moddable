context: https://github.com/Agoric/agoric-sdk/issues/511

example run:

```
mkdir -p build
mcconfig -d -o ./build -m -p x-cli-lin
make[1]: Entering directory '/home/connolly/projects/moddable/examples/js/snapshots'
# xsc Resource.xsb
# xsc instrumentation.xsb
# xsc main.xsb
# xsc mc/config.xsb
# xsid modInstrumentation.c.xsi
# xsc snapshot.xsb
# xsc vat1.xsb
# xsid Resource.c.xsi
# xsid modInstrumentation.h.xsi
# xsid snapshot.c.xsi
# mcrez resources
# xsl modules
# xsl modules
Total resource size: 0 bytes
# cc mc.resources.c
# cc mc.xs.c
# cc modInstrumentation.c.o
# cc Resource.c.o
# cc snapshot.c.o
In file included from /home/connolly/projects/moddable/examples/js/snapshots/snapshot.c:7:0:
/home/connolly/projects/moddable/examples/js/snapshots/snapshot.c: In function ‘alreadySeen’:
/home/connolly/projects/moddable/xs/includes/xs.h:157:26: warning: operation on ‘the->stack’ may be undefined [-Wsequence-point]
 #define fxPush(_SLOT) (*(--the->stack) = (_SLOT))
                         ~^~~~~~~~~~~~~
/home/connolly/projects/moddable/xs/includes/xs.h:842:2: note: in expansion of macro ‘fxPush’
  fxPush(_SLOT), \
  ^~~~~~
/home/connolly/projects/moddable/examples/js/snapshots/snapshot.c:99:9: note: in expansion of macro ‘xsTest’
     if (xsTest(xsGet(xsVar(3), xsID("done"))))
         ^~~~~~
# copy lin_xs_cli.c
In file included from /home/connolly/projects/moddable/examples/js/snapshots/build/tmp/lin/debug/snapshots/lin_xs_cli.c:3:0:
/home/connolly/projects/moddable/xs/platforms/xsPlatform.h:45:0: warning: "mxLinux" redefined
 #define mxLinux 0
 
In file included from /home/connolly/projects/moddable/examples/js/snapshots/build/tmp/lin/debug/snapshots/lin_xs_cli.c:1:0:
/home/connolly/projects/moddable/xs/platforms/lin_xs.h:42:0: note: this is the location of the previous definition
 #define mxLinux 1
 
# cc snapshots
make[1]: Leaving directory '/home/connolly/projects/moddable/examples/js/snapshots'
./build/bin/lin/debug/snapshots
exits[0] type:10 kind:10 reference:0x55f5c26f0e60
>>root
>>root>slot: 0x7ffc5ff52530 kind:10 offset:5
=== pushSelf 0x7ffc5ff52530 offset 6
==== alreadySeen(0x7ffc5ff52530) seen=4 1,2,3,Hello World,Hello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello World?
===== no not seen 0x7ffc5ff52530 exitQty=5
==== pushSelf(0x7ffc5ff52530) delta=-1 (#4) *seen=6 offset:6 exitQty:5
found Array! slot=0x7ffc5ff52530 reference=0x55f5c3b76ec0 prototype=0x55f5c26f45e0
>>root>slot: 0x7ffc5ff52530 kind:10 offset:5>reference
>>root>slot: 0x7ffc5ff52530 kind:10 offset:5>reference>slot: 0x55f5c3b76ec0 kind:13 offset:21
=== pushSelf 0x55f5c3b76ec0 offset 22
==== alreadySeen(0x55f5c3b76ec0) seen=6 [?]?
===== no not seen 0x55f5c3b76ec0 exitQty=5
==== pushSelf(0x55f5c3b76ec0) delta=-16 (#4) *seen=22 offset:22 exitQty:5
>>root>slot: 0x7ffc5ff52530 kind:10 offset:5>reference>slot: 0x55f5c3b76ec0 kind:13 offset:21>prototype
>>root>slot: 0x7ffc5ff52530 kind:10 offset:5>reference>slot: 0x55f5c3b76ec0 kind:13 offset:21>prototype>slot: 0x55f5c26f45e0 kind:13 offset:37
=== pushSelf 0x55f5c26f45e0 offset 38
==== alreadySeen(0x55f5c26f45e0) seen=22 [?]?
===== yes 0x55f5c26f45e0 is exit 1

...

snapshot value: {
  "self": 140721918387504,
  "next": null,
  "kind": 10,
  "flag": 0,
  "id": 0,
  "value": {
    "reference": {
      "kind": 13,
      "self": 94514038927040,
      "flag": 1,
      "id": -1,
      "idname": null,
      "value": {
        "garbage": null,
        "prototype": {
          "exit": 1
        }
      },
      "next": {
        "kind": 16,
        "self": 94514038927072,
        "flag": 6,
        "id": 3,
        "idname": null,
        "value": [
          {
            "kind": 3,
            "flag": 0,
            "id": -1,
            "idname": null,
            "value": 1,
 main() returned immediate value (not a promise). exiting
           "next": null
          },
          {
            "kind": 3,
            "flag": 0,
            "id": -1,
            "idname": null,
            "value": 2,
            "next": null
          },
          {
            "kind": 3,
            "flag": 0,
            "id": -1,
            "idname": null,
            "value": 3,
            "next": null
          },
          {
            "kind": 6,
            "flag": 0,
            "id": -1,
            "idname": null,
            "value": "Hello World",
            "next": null
          },
          {
            "kind": 5,
            "flag": 0,
            "id": -1,
            "idname": null,
            "value": "Hello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello WorldHello World",
            "next": null
          }
        ],
        "next": null
      }
    }
  }
}
```
