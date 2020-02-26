
SRCS=main.js vat1.js snapshot.js snapshot.c
BIN=./build/bin/lin/debug/snapshots

$(BIN): ./build $(SRCS)
	mcconfig -d -o ./build -m -p x-cli-lin

run: $(BIN)
	$(BIN)

./build:
	mkdir -p build
