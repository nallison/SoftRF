/*
 * GDL90Helper.h
 * Copyright (C) 2016-2021 Linar Yusupov
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

#ifndef GNS5892_H
#define GNS5892_H

void play5892(void);
void gns5892_setup(void);
void gns5892_loop(void);
void save_zone_stats(void);
void show_zone_stats(void);

void gns5892_test_mode(void);

extern bool gns5892_found;
extern uint32_t adsb_packets_counter;

#endif /* GNS5892_H */
