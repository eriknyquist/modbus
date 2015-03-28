FILES := main.c modbus_init.c time.c parse.c read.c
OUTPUT=mbd

MODBUS_H = /usr/include/modbus

${OUTPUT}: ${FILES}
	${CC} -I${MODBUS_H} $^ -o $@ -lmodbus -lrt -lm

nomodbus: ${FILES}
	${CC} -I${MODBUS_H} $^ -o ${OUTPUT} -lmodbus -lrt -lm -D NOMODBUS

clean:
	${RM} ${OUTPUT}
