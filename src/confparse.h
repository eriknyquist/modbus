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

void parse_modbus_params(FILE *fp, mbdport *mp, logging *lp, mbdinfo *mip);
int get_next_regparam(FILE *fp, element *p);
void parse_order (FILE *fp, element *v, mbdport *mp);

#endif
