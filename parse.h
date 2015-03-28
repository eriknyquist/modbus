#ifndef PARSE_H_ 
#define PARSE_H_ 

void parse_modbus_params(FILE *fp, modbusport *mp);
element get_next_regparam(FILE *fp);
void parse_order (FILE *fp, element *v, modbusport *mp);

#endif