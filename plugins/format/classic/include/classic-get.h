/*****
*
* Copyright (C) 2003-2015 CS-SI. All Rights Reserved.
* Author: Nicolas Delon <nicolas.delon@prelude-ids.com>
*
* This file is part of the PreludeDB library.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

#ifndef _LIBPRELUDEDB_CLASSIC_GET_H
#define _LIBPRELUDEDB_CLASSIC_GET_H

int classic_get_alert(preludedb_sql_t *sql, uint64_t ident, idmef_message_t **message);

int classic_get_heartbeat(preludedb_sql_t *sql, uint64_t ident, idmef_message_t **message);

#endif /* ! _LIBPRELUDEDB_CLASSIC_GET_H  */
