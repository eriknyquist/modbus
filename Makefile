FILES := \
main.c abb_ach550_modbus.c \
abb_ach550_time.c

MODBUS_H = /usr/include/modbus

abb_ach550_modbus: ${FILES}
	${CC} -I${MODBUS_H} $^ -o $@ -lmodbus -lrt -lm

clean:
	${RM} abb_ach550_modbus
