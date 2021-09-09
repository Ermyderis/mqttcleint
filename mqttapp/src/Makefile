BIN:=mqttapp
LDFLAGS:=-lmosquitto -lsqlite3 -luci -lcurl -ljson-c
CPPFLAGS:=-I$(CURDIR)
SRC:=$(wildcard *.c)
OBJS:=$(SRC:.c=.o)

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) 
clean:
	rm -f $(BIN) $(OBJS)
