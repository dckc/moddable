
MODULES=../../../modules
SRCS=main.js snapshot_restore.js vat1.js $(MODULES)/data/snapshot/snapshot.js $(MODULES)/data/snapshot/snapshot.c
BIN=./build/bin/lin/debug/snapshot

$(BIN): ./build $(SRCS)
	mcconfig -d -o ./build -m -p x-cli-lin

run: $(BIN)
	$(BIN)

./build:
	mkdir -p build


# protobuf-c cheat-sheet

apt-install:
	echo Addendum to Getting Started for Protobuf serialization
	sudo apt install protobuf-c-compiler libprotobuf-c-dev

protogen: amessage.pb-c.h amessage.pb-c.c

amessage.pb-c.h amessage.pb-c.c: amessage.proto
	protoc-c --c_out=. amessage.proto

protoex.o: protoex.c amessage.pb-c.h

amessage.pb-c.o: amessage.pb-c.c amessage.pb-c.h

protoex: protoex.o amessage.pb-c.o
	$(CC) -o $@ protoex.o amessage.pb-c.o $(shell pkg-config --libs libprotobuf-c)

protoex1.proto: protoex
	./protoex 100 200 >$@

protoex1.txt: protoex1.proto
	protoc --decode=AMessage amessage.proto < $< > $@

clean:
	rm -rf protoex1.txt protoex1.proto protoex *.o amessage.pb-c.h amessage.pb-c.c
