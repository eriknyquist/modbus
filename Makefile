OUTPUT = mbd
MODBUS_H = /usr/include/modbus
SRC=src
BIN=bin
CFLAGS = -Wall -lmodbus -lrt -lpthread

FILES := main.c init.c time.c parse.c read.c log.c
SRCFILES = $(FILES:%=$(SRC)/%)

$(OUTPUT): pre-build
	$(CC) -I$(MODBUS_H) $(SRCFILES) -o $(BIN)/$@ $(CFLAGS)

nomodbus: pre-build
	$(CC) -I$(MODBUS_H) $(SRCFILES) -o $(BIN)/$(OUTPUT) $(CFLAGS) -D NOMODBUS

vnomodbus: pre-build
	$(CC) -I$(MODBUS_H) $(SRCFILES) -o $(BIN)/$(OUTPUT) $(CFLAGS) -D NOMODBUS -D DEBUG

pre-build:
	[ -d $(BIN) ] || mkdir $(BIN)

clean:
	rm -rf $(BIN)
