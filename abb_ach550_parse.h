#ifndef ABB_ACH550_PARSE_H_ 
#define ABB_ACH550_PARSE_H_ 

#define CONF_FILE "/etc/abb.conf"
void parse_modbus_params(FILE *fp, modbusport *mp);
element get_next_regparam(FILE *fp);
void parse_order (FILE *fp, element *v, modbusport *mp);

#endif
