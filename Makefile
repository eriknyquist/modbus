FILES := main.c abb_modbus.c abb_time.c
MODBUS_H = /usr/include/modbus

abb_modbus: ${FILES}
	${CC} -I${MODBUS_H} $^ -o $@ -lmodbus -lrt

clean:
	${RM} abb_modbus
