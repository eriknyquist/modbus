FILES := main.c abb_modbus.c
MODBUS_H = /usr/include/modbus

abb_modbus: ${FILES}
	${CC} -I${MODBUS_H} $^ -o $@ -lmodbus

clean:
	${RM} abb_modbus
