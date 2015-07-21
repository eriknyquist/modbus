/*
 * int modbus_mask_write_register(modbus_t *'ctx', int 'addr', uint16_t 'and', uint16_t 'or');
 *
 *
 * DESCRIPTION
 * -----------
 * The *modbus_mask_write_register()* function shall modify the value of the
 * holding register at the address 'addr' of the remote device using the algorithm:
 * 
 * new value = (current value AND 'and') OR ('or' AND (NOT 'and'))
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <inttypes.h>
#include <unistd.h>

#define BIT1                      0x0001
#define BIT2                      0x0002
#define BIT3                      0x0004
#define BIT4                      0x0008
#define BIT5                      0x0010
#define BIT6                      0x0020
#define BIT7                      0x0040
#define BIT8                      0x0080
#define BIT9     0x0400
#define BIT11    0x0800

uint16_t current =   0x0000;
uint16_t mask =       0x0001;

void print_uint16_binary (uint16_t num)
{
	uint16_t i;

	for (i = 0; i < 16; i++) {
		printf("%d", (num & 0x8000) >> 15);
		num <<= 1;
	}
  
	printf("\n");
}

void mask_and_print (uint16_t curr, uint16_t and, uint16_t or)
{
	uint16_t old = 0x0001;
	uint16_t new;

	new = old | BIT3 | BIT5 | BIT7 | BIT9;

	//new = (curr & and) | (or & (~and));

	printf("old value  : ");
	print_uint16_binary(old);
	printf("new value  : ");
	print_uint16_binary(new);
}

void main (void)
{
	mask_and_print(current, ~mask, mask);	
}
