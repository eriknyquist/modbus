OUTPUT =         mbd
MODBUS_H =       /usr/include/modbus
INSTALLDIR =     /usr/bin

UUIDPATH =       /uuid
SRC=             src
BIN=             bin
CONF=            conf
TEST=            test.sh
CONFFILE =       $(OUTPUT).conf
CONFPATH =       /etc/$(CONFFILE)
SENSORLOGDIR =   /home/sensordata
SAMPLECONF=      $(CONF)/$(CONFFILE)

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

test: 
	./$(TEST)

install:
	[ -f $(BIN)/$(OUTPUT) ] || exit 1
	cp $(BIN)/$(OUTPUT) $(INSTALLDIR)
	[ -f $(CONFPATH) ] || cp $(SAMPLECONF) $(CONFPATH)
	[ -f $(UUIDPATH) ] || uuidgen > $(UUIDPATH)
	[ -d $(SENSORLOGDIR) ] || mkdir $(SENSORLOGDIR)


pre-build:
	[ -d $(BIN) ] || mkdir $(BIN)

clean:
	rm -rf $(BIN)
