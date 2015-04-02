FILES := main.c init.c time.c parse.c read.c log.c
CFLAGS = -Wall -lmodbus -lrt -lpthread
OUTPUT = mbd

MODBUS_H = /usr/include/modbus

$(OUTPUT): $(FILES)
	$(CC) -I$(MODBUS_H) $^ -o $@ $(CFLAGS)

nomodbus: $(FILES)
	$(CC) -I$(MODBUS_H) $^ -o $(OUTPUT) $(CFLAGS) -D NOMODBUS

vnomodbus: $(FILES)
	$(CC) -I$(MODBUS_H) $^ -o $(OUTPUT) $(CFLAGS) -D NOMODBUS -D DEBUG

clean:
	$(RM) $(OUTPUT)
