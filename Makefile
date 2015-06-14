OUTPUT = mbd
MODBUS_H = /usr/include/modbus
SRC=src
BIN=bin
CFLAGS = -Wall -lmodbus -lrt -lpthread

FILES := main.c init.c time.c parse.c read.c log.c
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

# no modbus ioctls, and print to stdout
vnomodbus: pre-build
	$(CC) -I$(MODBUS_H) $(SRCFILES) -o $(BIN)/$(OUTPUT) $(CFLAGS) -D NOMODBUS -D DEBUG

pre-build:
	[ -d $(BIN) ] || mkdir $(BIN)

clean:
	rm -rf $(BIN)
