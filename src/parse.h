#ifndef PARSE_H_ 
#define PARSE_H_ 

void parse_modbus_params(FILE *fp, modbusport *mp, logging *lp);
int get_next_regparam(FILE *fp, element *p);
void parse_order (FILE *fp, element *v, modbusport *mp);

#endif
