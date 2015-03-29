FILES := main.c init.c time.c parse.c read.c log.c
OUTPUT=mbd

MODBUS_H = /usr/include/modbus

${OUTPUT}: ${FILES}
	${CC} -I${MODBUS_H} $^ -o $@ -lmodbus -lrt -lpthread

nomodbus: ${FILES}
	${CC} -I${MODBUS_H} $^ -o ${OUTPUT} -lmodbus -lrt -lpthread -D NOMODBUS

clean:
	${RM} ${OUTPUT}
