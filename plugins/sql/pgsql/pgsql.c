/*****
*
* Copyright (C) 2003-2016 CS-SI. All Rights Reserved.
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

#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#include <libprelude/prelude-error.h>
#include <libprelude/idmef.h>
#include <libpq-fe.h>

#include "preludedb-sql-settings.h"
#include "preludedb-sql.h"
#include "preludedb-error.h"
#include "preludedb-plugin-sql.h"
#include "preludedb-path-selection.h"
#include "preludedb.h"


int pgsql_LTX_prelude_plugin_version(void);
int pgsql_LTX_preludedb_plugin_init(prelude_plugin_entry_t *pe, void *data);


static int handle_error(prelude_error_code_t code, PGconn *conn)
{
        int ret;
        char *tmp;
        size_t len;
        const char *error;

        if ( PQstatus(conn) == CONNECTION_BAD )
                code = PRELUDEDB_ERROR_CONNECTION;

        error = PQerrorMessage(conn);
        if ( ! error )
                return preludedb_error(code);

        /*
         * Pgsql error message are formatted. Remove trailing '\n'.
         */
        tmp = strdup(error);
        if ( ! tmp )
                return preludedb_error_verbose(code, "%s", error);

        len = strlen(tmp) - 1;
        while ( tmp[len] == '\n' || tmp[len] == ' ' )
                tmp[len--] = 0;

        ret = preludedb_error_verbose(code, "%s", tmp);
        free(tmp);

        return ret;
}



static int sql_query(void *session, const char *query, preludedb_sql_table_t **table)
{
        int ret;
        PGresult *result;

        result = PQexec(session, query);
        if ( ! result )
                return handle_error(PRELUDEDB_ERROR_QUERY, session);

        ret = PQresultStatus(result);
        if ( ret == PGRES_TUPLES_OK && PQntuples(result) != 0 ) {
                ret = preludedb_sql_table_new(table, result);
                if ( ret < 0 ) {
                        PQclear(result);
                        return ret;
                }

                return 1;
        }

        PQclear(result);

        if ( ret == PGRES_TUPLES_OK || ret == PGRES_COMMAND_OK )
                return 0;

        return handle_error(PRELUDEDB_ERROR_QUERY, session);
}



static int sql_get_last_insert_ident(void *session, uint64_t *ident)
{
        int ret;
        char *value;
        PGresult *result;

        result = PQexec(session, "SELECT lastval();");
        if ( ! result )
                return handle_error(PRELUDEDB_ERROR_QUERY, session);

        ret = PQresultStatus(result);
        if ( ret != PGRES_TUPLES_OK || PQntuples(result) <= 0 )
                return handle_error(PRELUDEDB_ERROR_QUERY, session);

        value = PQgetvalue(result, 0, 0);
        if ( ! value )
                return preludedb_error(PRELUDEDB_ERROR_INVALID_VALUE);

        ret = sscanf(value, "%" PRELUDE_SCNu64, ident);
        if ( ret <= 0 )
                return preludedb_error(PRELUDEDB_ERROR_INVALID_VALUE);

        PQclear(result);
        return 0;
}



static int sql_open(preludedb_sql_settings_t *settings, void **session)
{
        int ret;
        PGconn *conn;

        conn = PQsetdbLogin(preludedb_sql_settings_get_host(settings),
                            preludedb_sql_settings_get_port(settings),
                            NULL,
                            NULL,
                            preludedb_sql_settings_get_name(settings),
                            preludedb_sql_settings_get_user(settings),
                            preludedb_sql_settings_get_pass(settings));

        if ( PQstatus(conn) == CONNECTION_BAD ) {
                ret = handle_error(PRELUDEDB_ERROR_CONNECTION, conn);
                PQfinish(conn);
                return ret;
        }

        *session = conn;

        ret = sql_query(conn, "SET standard_conforming_strings=on", NULL);
        if ( ret < 0 )
                return ret;

        return sql_query(conn, "SET DATESTYLE TO 'ISO'", NULL);
}



static void sql_close(void *session)
{
        PQfinish(session);
}


static int sql_escape(void *session, const char *input, size_t input_size, char **output)
{
#ifdef HAVE_PQESCAPESTRINGCONN
        int error;
#endif
        size_t rsize;

        rsize = input_size * 2 + 3;
        if ( rsize <= input_size )
                return preludedb_error(PRELUDEDB_ERROR_GENERIC);

        *output = malloc(rsize);
        if ( ! *output )
                return preludedb_error_from_errno(errno);

        (*output)[0] = '\'';

#ifdef HAVE_PQESCAPESTRINGCONN
        rsize = PQescapeStringConn(session, (*output) + 1, input, input_size, &error);
        if ( error )
                return handle_error(PRELUDEDB_ERROR_GENERIC, session);
#else
        rsize = PQescapeString((*output) + 1, input, input_size);
#endif
        (*output)[rsize + 1] = '\'';
        (*output)[rsize + 2] = '\0';

        return 0;
}


static int sql_escape_binary(void *session, const unsigned char *input, size_t input_size, char **output)
{
        int ret;
        size_t dummy;
        unsigned char *ptr;
        prelude_string_t *string;

        ret = prelude_string_new(&string);
        if ( ret < 0 )
                return ret;

#ifdef HAVE_PQESCAPEBYTEACONN
        ptr = PQescapeByteaConn(session, input, input_size, &dummy);
#else
        ptr = PQescapeBytea(input, input_size, &dummy);
#endif
        if ( ! ptr ) {
                prelude_string_destroy(string);
                return -1;
        }

        ret = prelude_string_sprintf(string, "'%s'", ptr);
        free(ptr);

        if ( ret < 0 ) {
                prelude_string_destroy(string);
                return ret;
        }

        ret = prelude_string_get_string_released(string, output);
        prelude_string_destroy(string);

        return 0;
}



static int sql_unescape_binary(void *session, const char *input, unsigned char **output, size_t *output_size)
{
        *output = PQunescapeBytea((const unsigned char *) input, output_size);
        if ( ! *output )
                return preludedb_error_from_errno(errno);

        return 0;
}



static int sql_build_limit_offset_string(void *session, int limit, int offset, prelude_string_t *output)
{
        if ( limit >= 0 ) {
                if ( offset >= 0 )
                        return prelude_string_sprintf(output, " LIMIT %d OFFSET %d", limit, offset);

                return prelude_string_sprintf(output, " LIMIT %d", limit);
        }

        else if ( offset >= 0 )
                return prelude_string_sprintf(output, " LIMIT ALL OFFSET %d", offset);

        return 0;
}



static void sql_table_destroy(void *session, preludedb_sql_table_t *table)
{
        PQclear(preludedb_sql_table_get_data(table));
}



static const char *sql_get_column_name(void *session, preludedb_sql_table_t *table, unsigned int column_num)
{
        return PQfname(preludedb_sql_table_get_data(table), column_num);
}



static int sql_get_column_num(void *session, preludedb_sql_table_t *table, const char *column_name)
{
        int ret;

        ret = PQfnumber(preludedb_sql_table_get_data(table), column_name);
        if ( ret < 0 )
                return prelude_error_verbose(PRELUDEDB_ERROR_GENERIC, "unknown column '%s'", column_name);

        return ret;
}



static unsigned int sql_get_column_count(void *session, preludedb_sql_table_t *table)
{
        return PQnfields(preludedb_sql_table_get_data(table));
}



static unsigned int sql_get_row_count(void *session, preludedb_sql_table_t *table)
{
        return PQntuples(preludedb_sql_table_get_data(table));
}



static int sql_fetch_row(void *s, preludedb_sql_table_t *table, unsigned int row_index, preludedb_sql_row_t **row)
{
        int ret;
        unsigned int row_count;

        row_count = PQntuples(preludedb_sql_table_get_data(table));
        if ( row_index < row_count ) {
                ret = preludedb_sql_table_new_row(table, row, row_index);
                if ( ret < 0 )
                        return ret;

                preludedb_sql_row_set_data(*row, (void *) (unsigned long) row_index);
                return 1;
        }

        return 0;
}



static int sql_fetch_field(void *session, preludedb_sql_table_t *table, preludedb_sql_row_t *row,
                           unsigned int column_num, preludedb_sql_field_t **field)
{
        char *value;
        void *valaddr = preludedb_sql_row_get_data(row);
        PGresult *result = preludedb_sql_table_get_data(table);
        int nfields, len;
        unsigned int row_index;

        row_index = (unsigned int) (unsigned long) valaddr;

        nfields = PQnfields(result);
        if ( nfields < 0 || column_num >= (unsigned int) nfields )
                return preludedb_error(PRELUDEDB_ERROR_INVALID_COLUMN_NUM);

        if ( PQgetisnull(result, row_index, column_num) ) {
                value = NULL;
                len = 0;
        } else {
                value = PQgetvalue(result, row_index, column_num);
                len = PQgetlength(result, row_index, column_num);
        }

        return preludedb_sql_row_new_field(row, field, column_num, value, len);
}




static const char *get_operator_string(idmef_criterion_operator_t operator)
{
        int i;
        const struct tbl {
                idmef_criterion_operator_t operator;
                const char *name;
        } tbl[] = {
                { IDMEF_CRITERION_OPERATOR_EQUAL,             "="           },
                { IDMEF_CRITERION_OPERATOR_NOT_EQUAL,         "!="          },

                { IDMEF_CRITERION_OPERATOR_GREATER,           ">"           },
                { IDMEF_CRITERION_OPERATOR_GREATER_OR_EQUAL,  ">="          },
                { IDMEF_CRITERION_OPERATOR_LESSER,            "<"           },
                { IDMEF_CRITERION_OPERATOR_LESSER_OR_EQUAL,   "<="          },

                { IDMEF_CRITERION_OPERATOR_SUBSTR,            "LIKE"        },
                { IDMEF_CRITERION_OPERATOR_SUBSTR_NOCASE,     "ILIKE"       },
                { IDMEF_CRITERION_OPERATOR_NOT_SUBSTR,        "NOT LIKE"    },
                { IDMEF_CRITERION_OPERATOR_NOT_SUBSTR_NOCASE, "NOT ILIKE"   },

                { IDMEF_CRITERION_OPERATOR_REGEX,             "~"           },
                { IDMEF_CRITERION_OPERATOR_REGEX_NOCASE,      "~*"          },
                { IDMEF_CRITERION_OPERATOR_NOT_REGEX,         "!~"          },
                { IDMEF_CRITERION_OPERATOR_NOT_REGEX_NOCASE,  "!~*"         },

                { IDMEF_CRITERION_OPERATOR_NULL,              "IS NULL"     },
                { IDMEF_CRITERION_OPERATOR_NOT_NULL,          "IS NOT NULL" },
                { 0, NULL },
        };

        for ( i = 0; tbl[i].operator != 0; i++ )
                if ( operator == tbl[i].operator )
                        return tbl[i].name;

        return NULL;
}




static int sql_build_constraint_string(prelude_string_t *out, const char *field,
                                       idmef_criterion_operator_t operator, const char *value)
{
        const char *op_str;

        if ( ! value )
                value = "";

        op_str = get_operator_string(operator);
        if ( op_str )
                return prelude_string_sprintf(out, "%s %s %s", field, op_str, value);

        else if ( operator == IDMEF_CRITERION_OPERATOR_EQUAL_NOCASE )
                return prelude_string_sprintf(out, "lower(%s) = lower(%s)", field, value);

        else if ( operator == IDMEF_CRITERION_OPERATOR_NOT_EQUAL_NOCASE )
                return prelude_string_sprintf(out, "lower(%s) != lower(%s)", field, value);

        return -1;
}



static int sql_build_time_extract_string(prelude_string_t *output, const char *field,
                                         preludedb_sql_time_constraint_type_t type, int gmt_offset)
{
        int ret;
        char buf[128];

        if ( ! gmt_offset )
                ret = snprintf(buf, sizeof (buf), "%s", field);
        else
                ret = snprintf(buf, sizeof (buf), "%s + INTERVAL '%d HOUR'", field, gmt_offset / 3600);

        if ( ret < 0 || (size_t) ret >= sizeof (buf) )
                return preludedb_error(PRELUDEDB_ERROR_GENERIC);

        switch ( type ) {
        case PRELUDEDB_SQL_TIME_CONSTRAINT_YEAR:
                return prelude_string_sprintf(output, "EXTRACT(YEAR FROM %s)", buf);

        case PRELUDEDB_SQL_TIME_CONSTRAINT_QUARTER:
                return prelude_string_sprintf(output, "EXTRACT(QUARTER FROM %s)", buf);

        case PRELUDEDB_SQL_TIME_CONSTRAINT_MONTH:
                return  prelude_string_sprintf(output, "EXTRACT(MONTH FROM %s)", buf);

        case PRELUDEDB_SQL_TIME_CONSTRAINT_YDAY:
                return prelude_string_sprintf(output, "EXTRACT(DOY FROM %s)", buf);

        case PRELUDEDB_SQL_TIME_CONSTRAINT_MDAY:
                return prelude_string_sprintf(output, "EXTRACT(DAY FROM %s)", buf);

        case PRELUDEDB_SQL_TIME_CONSTRAINT_WDAY:
                return prelude_string_sprintf(output, "EXTRACT(DOW FROM %s)", buf);

        case PRELUDEDB_SQL_TIME_CONSTRAINT_HOUR:
                return prelude_string_sprintf(output, "EXTRACT(HOUR FROM %s)", buf);

        case PRELUDEDB_SQL_TIME_CONSTRAINT_MIN:
                return prelude_string_sprintf(output, "EXTRACT(MINUTE FROM %s)", buf);

        case PRELUDEDB_SQL_TIME_CONSTRAINT_SEC:
                return prelude_string_sprintf(output, "EXTRACT(SECOND FROM %s)", buf);

        case PRELUDEDB_SQL_TIME_CONSTRAINT_MSEC:
                return prelude_string_sprintf(output, "EXTRACT(MILLISECOND FROM %s)", buf);

        case PRELUDEDB_SQL_TIME_CONSTRAINT_USEC:
                return prelude_string_sprintf(output, "EXTRACT(MICROSECOND FROM %s)", buf);
        }

        /* not reached */

        return preludedb_error(PRELUDEDB_ERROR_GENERIC);
}

static int sql_build_time_constraint_string(prelude_string_t *output, const char *field,
                                            preludedb_sql_time_constraint_type_t type,
                                            idmef_criterion_operator_t operator, int value, int gmt_offset)
{
        const char *sql_operator;
        int ret;

        ret = sql_build_time_extract_string(output, field, type, gmt_offset);
        if ( ret < 0 )
                return ret;

        sql_operator = get_operator_string(operator);
        if ( ! sql_operator )
                return preludedb_error(PRELUDEDB_ERROR_GENERIC);

        if ( type == PRELUDEDB_SQL_TIME_CONSTRAINT_WDAY )
                value = value % 7 + 1;

        return prelude_string_sprintf(output, " %s %d", sql_operator, value);
}


static int sql_build_time_interval_string(prelude_string_t *output, const char *field, const char *value, preludedb_selected_object_interval_t unit)
{
        char *end;
        const char *sunit;

        switch ( unit ) {
        case PRELUDEDB_SELECTED_OBJECT_INTERVAL_YEAR:
                sunit = "YEAR";
                break;

        case PRELUDEDB_SELECTED_OBJECT_INTERVAL_QUARTER:
                sunit = "QUARTER";
                break;

        case PRELUDEDB_SELECTED_OBJECT_INTERVAL_MONTH:
                sunit = "MONTH";
                break;

        case PRELUDEDB_SELECTED_OBJECT_INTERVAL_DAY:
                sunit = "DAY";
                break;

        case PRELUDEDB_SELECTED_OBJECT_INTERVAL_HOUR:
                sunit = "HOUR";
                break;

        case PRELUDEDB_SELECTED_OBJECT_INTERVAL_MIN:
                sunit = "MINUTE";
                break;

        case PRELUDEDB_SELECTED_OBJECT_INTERVAL_SEC:
                sunit = "SECOND";
                break;

        default:
                return preludedb_error(PRELUDEDB_ERROR_GENERIC);
        }

        strtol(value, &end, 10);
        if ( end )
                return prelude_string_sprintf(output, "(%s + (%s * INTERVAL '1 %s'))", field, value, sunit);
        else
                return prelude_string_sprintf(output, "(%s + INTERVAL '%s %s')", field, value, sunit);
}



static long sql_get_server_version(void *session)
{
        return PQserverVersion(session);
}



int pgsql_LTX_preludedb_plugin_init(prelude_plugin_entry_t *pe, void *data)
{
        int ret;
        preludedb_plugin_sql_t *plugin;

        ret = preludedb_plugin_sql_new(&plugin);
        if ( ret < 0 )
                return ret;

        prelude_plugin_set_name((prelude_plugin_generic_t *) plugin, "PgSQL");
        prelude_plugin_entry_set_plugin(pe, (void *) plugin);

        preludedb_plugin_sql_set_open_func(plugin, sql_open);
        preludedb_plugin_sql_set_close_func(plugin, sql_close);
        preludedb_plugin_sql_set_escape_func(plugin, sql_escape);
        preludedb_plugin_sql_set_escape_binary_func(plugin, sql_escape_binary);
        preludedb_plugin_sql_set_unescape_binary_func(plugin, sql_unescape_binary);
        preludedb_plugin_sql_set_query_func(plugin, sql_query);
        preludedb_plugin_sql_set_get_server_version_func(plugin, sql_get_server_version);
        preludedb_plugin_sql_set_table_destroy_func(plugin, sql_table_destroy);
        preludedb_plugin_sql_set_get_column_count_func(plugin, sql_get_column_count);
        preludedb_plugin_sql_set_get_row_count_func(plugin, sql_get_row_count);
        preludedb_plugin_sql_set_get_column_name_func(plugin, sql_get_column_name);
        preludedb_plugin_sql_set_get_column_num_func(plugin, sql_get_column_num);
        preludedb_plugin_sql_set_get_operator_string_func(plugin, get_operator_string);
        preludedb_plugin_sql_set_fetch_row_func(plugin, sql_fetch_row);
        preludedb_plugin_sql_set_fetch_field_func(plugin, sql_fetch_field);
        preludedb_plugin_sql_set_build_constraint_string_func(plugin, sql_build_constraint_string);
        preludedb_plugin_sql_set_build_time_extract_string_func(plugin, sql_build_time_extract_string);
        preludedb_plugin_sql_set_build_time_constraint_string_func(plugin, sql_build_time_constraint_string);
        preludedb_plugin_sql_set_build_time_interval_string_func(plugin, sql_build_time_interval_string);
        preludedb_plugin_sql_set_build_limit_offset_string_func(plugin, sql_build_limit_offset_string);
        preludedb_plugin_sql_set_get_last_insert_ident_func(plugin, sql_get_last_insert_ident);

        return 0;
}



int pgsql_LTX_prelude_plugin_version(void)
{
        return PRELUDE_PLUGIN_API_VERSION;
}
