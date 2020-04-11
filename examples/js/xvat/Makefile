
PLATFORM=x-cli-lin
DEBUG=-d
MAKE=-m
SOURCES=main.js alice.js bob.js mallet.js
PROGRAM=./build/bin/lin/debug/xvat

$(PROGRAM): build manifest.json $(SOURCES)
	mcconfig -p $(PLATFORM) $(DEBUG) $(MAKE) -o ./build

run: $(PROGRAM)
	$(PROGRAM)

./build:
	mkdir -p ./build

clean:
	rm -rf ./build
