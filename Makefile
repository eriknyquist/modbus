FILES := \
main.c abb_pch550_modbus.c \
abb_pch550_time.c

MODBUS_H = /usr/include/modbus

abb_pch550_modbus: ${FILES}
	${CC} -I${MODBUS_H} $^ -o $@ -lmodbus -lrt -lm

clean:
	${RM} abb_pch550_modbus
