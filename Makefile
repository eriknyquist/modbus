OUTPUT =         mbd
MODBUS_H =       /usr/local/include/modbus
INSTALLDIR =     /usr/bin

UUIDPATH =       /uuid
SRC=             src
BIN=             bin
CONF=            conf
TEST_SH =        $(CONF)/test/test.sh
UNITTEST_SRC =   $(SRC)/unit_test.c
UNITTEST_BIN =   $(BIN)/unit_test
CONFFILE =       $(OUTPUT).conf
CONFPATH =       /etc/$(CONFFILE)
SENSORLOGDIR =   /home/sensordata
SAMPLECONF=      $(CONF)/$(CONFFILE)

CFLAGS = -Wall -lrt -lpthread /usr/local/lib/libmodbus.so.5.1.0

FILES :=           main.c init.c time.c confparse.c argparse.c read.c log.c control.c
FILES_TEST :=      init.c time.c confparse.c argparse.c read.c log.c
SRCFILES =         $(FILES:%=$(SRC)/%)
SRCFILES_TEST =    $(FILES_TEST:%=$(SRC)/%)

# default
$(OUTPUT): pre-build
	$(CC) -I$(MODBUS_H) $(SRCFILES) -o $(BIN)/$@ $(CFLAGS)

# no modbus ioctls
nomodbus: pre-build
	$(CC) -I$(MODBUS_H) $(SRCFILES) -o $(BIN)/$(OUTPUT) $(CFLAGS) -D NOMODBUS

# no modbus ioctls, and compile with debug symbols
dnomodbus: pre-build
	$(CC) -I$(MODBUS_H) $(SRCFILES) -g -o $(BIN)/$(OUTPUT) $(CFLAGS) -D NOMODBUS

test: pre-build 
	$(CC) -I$(MODBUS_H) $(UNITTEST_SRC) $(SRCFILES_TEST) -g -o $(UNITTEST_BIN) $(CFLAGS)
	./$(TEST_SH)

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
