FILES := main.c init.c time.c parse.c read.c log.c
OUTPUT=mbd

MODBUS_H = /usr/include/modbus

${OUTPUT}: ${FILES}
	${CC} -I${MODBUS_H} $^ -o $@ -Wall -lmodbus -lrt -lpthread

nomodbus: ${FILES}
	${CC} -I${MODBUS_H} $^ -o ${OUTPUT} -Wall -lmodbus -lrt -lpthread -D NOMODBUS

vnomodbus: ${FILES}
	${CC} -I${MODBUS_H} $^ -o ${OUTPUT} -Wall -lmodbus -lrt -lpthread -D NOMODBUS -D DEBUG

clean:
	${RM} ${OUTPUT}
