
PLATFORM=x-cli-lin
DEBUG=-d
MAKE=-m
SOURCES=main.js alice.js bob.js mallet.js

bin: build manifest.json $(SOURCES)
	mcconfig -p $(PLATFORM) $(DEBUG) $(MAKE) -o ./build

./build:
	mkdir -p ./build

clean:
	rm -rf ./build
