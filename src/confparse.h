/* Modbus logging daemon
 * Copyright (C) 2015 Erik Nyquist <eknyquist@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CONFPARSE_H_ 
#define CONFPARSE_H_ 

int parse_modbus_params(FILE *fp, mbdport *mp, logging *lp, mbdinfo *mip);
int get_next_regparam(FILE *fp, element *p);
void parse_order (FILE *fp, element *v, mbdport *mp);
int only_has_digits (char *s);

/* For unit testing */
int only_has_digits(char *s);
void convert_assign_ul(unsigned long *dest, char *source, const char *conf_id,
	unsigned long min);
void convert_assign_uint(unsigned int *dest, char *source, const char *conf_id,
	unsigned int min);
void convert_assign_retries(int *dest, char *source);

#endif
