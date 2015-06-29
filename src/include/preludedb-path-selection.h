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

#ifndef _LIBPRELUDEDB_OBJECT_SELECTION_H
#define _LIBPRELUDEDB_OBJECT_SELECTION_H

#include "preludedb-sql.h"

#ifdef __cplusplus
 extern "C" {
#endif

typedef enum {
        PRELUDEDB_SELECTED_OBJECT_FUNCTION_MIN = 0x01,
        PRELUDEDB_SELECTED_OBJECT_FUNCTION_MAX = 0x02,
        PRELUDEDB_SELECTED_OBJECT_FUNCTION_AVG = 0x04,
        PRELUDEDB_SELECTED_OBJECT_FUNCTION_STD = 0x08,
        PRELUDEDB_SELECTED_OBJECT_FUNCTION_COUNT = 0x10,
        PRELUDEDB_SELECTED_OBJECT_GROUP_BY = 0x20,

        PRELUDEDB_SELECTED_OBJECT_ORDER_ASC = 0x40,
        PRELUDEDB_SELECTED_OBJECT_ORDER_DESC = 0x80
} preludedb_selected_path_flags_t;


typedef struct preludedb_path_selection preludedb_path_selection_t;
typedef struct preludedb_selected_path preludedb_selected_path_t;

#include "preludedb.h"

int preludedb_path_selection_get_selected(preludedb_path_selection_t *selection, preludedb_selected_path_t **selected, unsigned int index);
int preludedb_selected_path_get_column_index(preludedb_selected_path_t *selected);

int preludedb_selected_path_new(preludedb_selected_path_t **selected_path, idmef_path_t *path, int flags);
int preludedb_selected_path_new_string(preludedb_selected_path_t **selected_path, const char *str);

void preludedb_selected_path_destroy(preludedb_selected_path_t *selected_path);

preludedb_path_selection_t *preludedb_path_selection_ref(preludedb_path_selection_t *path_selection);

idmef_path_t *preludedb_selected_path_get_path(preludedb_selected_path_t *selected_path);
preludedb_selected_path_flags_t preludedb_selected_path_get_flags(preludedb_selected_path_t *selected_path);
preludedb_sql_time_constraint_type_t preludedb_selected_path_get_time_constraint(preludedb_selected_path_t *selected_path);

int preludedb_path_selection_new(preludedb_t *db, preludedb_path_selection_t **path_selection);
void preludedb_path_selection_destroy(preludedb_path_selection_t *path_selection);
int preludedb_path_selection_add(preludedb_path_selection_t *path_selection,
                                 preludedb_selected_path_t *selected_path);

preludedb_selected_path_t *preludedb_path_selection_get_next(preludedb_path_selection_t *path_selection,
                                                                 preludedb_selected_path_t *selected_path);
unsigned int preludedb_path_selection_get_count(preludedb_path_selection_t *path_selection);

#ifdef __cplusplus
  }
#endif

#endif /* ! _LIBPRELUDEDB_OBJECT_SELECTION_H */
