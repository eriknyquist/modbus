FILES := \
main.c abb_ach550_modbus.c \
abb_ach550_time.c abb_ach550_parse.c
OUTPUT=abb_ach550_modbus

MODBUS_H = /usr/include/modbus

${OUTPUT}: ${FILES}
	${CC} -I${MODBUS_H} $^ -o $@ -lmodbus -lrt -lm

nomodbus: ${FILES}
	${CC} -I${MODBUS_H} $^ -o ${OUTPUT} -lmodbus -lrt -lm -D NOMODBUS

clean:
	${RM} ${OUTPUT}
