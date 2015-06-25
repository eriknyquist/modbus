OUTPUT = mbd
MODBUS_H = /usr/include/modbus
SRC=src
BIN=bin
CFLAGS = -Wall -lmodbus -lrt -lpthread

FILES := main.c init.c time.c confparse.c argparse.c read.c log.c
SRCFILES = $(FILES:%=$(SRC)/%)

# default
$(OUTPUT): pre-build
	$(CC) -I$(MODBUS_H) $(SRCFILES) -o $(BIN)/$@ $(CFLAGS)

# no modbus ioctls
nomodbus: pre-build
	$(CC) -I$(MODBUS_H) $(SRCFILES) -o $(BIN)/$(OUTPUT) $(CFLAGS) -D NOMODBUS

# no modbus ioctls, and compile with debug symbols
dnomodbus: pre-build
	$(CC) -I$(MODBUS_H) $(SRCFILES) -g -o $(BIN)/$(OUTPUT) $(CFLAGS) -D NOMODBUS

pre-build:
	[ -d $(BIN) ] || mkdir $(BIN)

clean:
	rm -rf $(BIN)
