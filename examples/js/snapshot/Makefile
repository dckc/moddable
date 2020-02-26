
MODULES=../../../modules
SRCS=main.js vat1.js $(MODULES)/data/snapshot/snapshot.js $(MODULES)/data/snapshot/snapshot.c
BIN=./build/bin/lin/debug/snapshot

$(BIN): ./build $(SRCS)
	mcconfig -d -o ./build -m -p x-cli-lin

run: $(BIN)
	$(BIN)

./build:
	mkdir -p build
