
PLATFORM=x-cli-lin
DEBUG=-d
MAKE=-m
SOURCES=main.js alice.js bob.js mallet.js
MODULES=../../../modules
MODWORK=$(MODULES)/base/worker
CLI=../../../xs/platforms/lin_xs_cli.c
PROGRAM=./build/bin/lin/debug/xvat

$(PROGRAM): build manifest.json $(SOURCES) $(MODWORK)/gioWorker.c $(CLI)
	mcconfig -p $(PLATFORM) $(DEBUG) $(MAKE) -o ./build

run: $(PROGRAM)
	$(PROGRAM)

./build:
	mkdir -p ./build

clean:
	rm -rf ./build
