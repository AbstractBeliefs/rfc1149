CXX = clang
CFLAGS = -std=c99
EXECUTABLE = server.out

$(EXECUTABLE): server.c
	$(CXX) $< $(CFLAGS) -o $@

clean:
	rm server.out

debug: CFLAGS += -ggdb -pg
debug: clean $(EXECUTABLE)
	gdb -tui --args $(EXECUTABLE) 10.200.200.1 10.200.200.2

mktun: /dev/net/tun
/dev/net/tun:
	echo "Making tun device"
	sudo mkdir /dev/net
	sudo mknod /dev/net/tun c 10 200

.PHONY: clean mktun
