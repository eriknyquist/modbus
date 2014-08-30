FILES := main.c abb_modbus.c
OBJECTS := main.o abb_modbus.o
MODBUS_H = /usr/include/modbus

abb_modbus: ${FILES}
	${CC} -I${MODBUS_H} $^ -o $@ -lmodbus

clean:
	${RM} ${OBJECTS} abb_modbus
