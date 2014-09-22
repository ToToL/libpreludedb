/*****
*
* Copyright (C) 2003-2012 CS-SI. All Rights Reserved.
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
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <libprelude/prelude.h>


#include "preludedb-error.h"
#include "preludedb-sql-settings.h"
#include "preludedb-sql.h"
#include "preludedb-path-selection.h"

#include "preludedb.h"
#include "preludedb-plugin-format.h"
#include "preludedb-plugin-format-prv.h"


#define PRELUDEDB_PLUGIN_SYMBOL "preludedb_plugin_init"


struct preludedb {
        int refcount;
        char *format_version;
        preludedb_sql_t *sql;
        preludedb_plugin_format_t *plugin;
};

struct preludedb_result_idents {
        preludedb_t *db;
        void *res;
        int refcount;
};

struct preludedb_result_values {
        int refcount;
        preludedb_t *db;
        preludedb_path_selection_t *selection;
        void *res;
};


int _preludedb_sql_transaction_start(preludedb_sql_t *sql);
int _preludedb_sql_transaction_end(preludedb_sql_t *sql);
int _preludedb_sql_transaction_abort(preludedb_sql_t *sql);
void _preludedb_sql_enable_internal_transaction(preludedb_sql_t *sql);
void _preludedb_sql_disable_internal_transaction(preludedb_sql_t *sql);



static int libpreludedb_refcount = 0;
PRELUDE_LIST(_sql_plugin_list);
static PRELUDE_LIST(plugin_format_list);



int preludedb_init(void)
{
        int ret;

        if ( libpreludedb_refcount++ > 0 )
                return 0;

        ret = prelude_init(NULL, NULL);
        if ( ret < 0 )
                return ret;

        ret = access(FORMAT_PLUGIN_DIR, F_OK);
        if ( ret < 0 )
                return preludedb_error_verbose(PRELUDEDB_ERROR_CANNOT_LOAD_FORMAT_PLUGIN,
                                               "could not access format plugin directory '%s'", FORMAT_PLUGIN_DIR);

        ret = prelude_plugin_load_from_dir(&plugin_format_list, FORMAT_PLUGIN_DIR,
                                           PRELUDEDB_PLUGIN_SYMBOL, NULL, NULL, NULL);
        if ( ret < 0 )
                return ret;

        ret = access(SQL_PLUGIN_DIR, F_OK);
        if ( ret < 0 )
                return preludedb_error_verbose(PRELUDEDB_ERROR_CANNOT_LOAD_SQL_PLUGIN,
                                               "could not access sql plugin directory '%s'", SQL_PLUGIN_DIR);

        ret = prelude_plugin_load_from_dir(&_sql_plugin_list, SQL_PLUGIN_DIR,
                                           PRELUDEDB_PLUGIN_SYMBOL, NULL, NULL, NULL);
        if ( ret < 0 )
                return ret;

        return 0;
}



void preludedb_deinit(void)
{
        prelude_list_t *iter;
        prelude_plugin_generic_t *pl;

        if ( --libpreludedb_refcount > 0 )
                return;

        iter = NULL;
        while ( (pl = prelude_plugin_get_next(&_sql_plugin_list, &iter)) ) {
                prelude_plugin_unload(pl);
                free(pl);
        }

        iter = NULL;
        while ( (pl = prelude_plugin_get_next(&plugin_format_list, &iter)) ) {
                prelude_plugin_unload(pl);
                free(pl);
        }

        prelude_deinit();
}



static int preludedb_autodetect_format(preludedb_t *db)
{
        preludedb_sql_table_t *table;
        preludedb_sql_row_t *row;
        preludedb_sql_field_t *format_name;
        preludedb_sql_field_t *format_version;
        int ret;

        ret = preludedb_sql_query(db->sql, "SELECT name, version from _format", &table);
        if ( ret <= 0 )
                return (ret < 0) ? ret : -1;

        ret = preludedb_sql_table_fetch_row(table, &row);
        if ( ret < 0 )
                goto error;

        ret = preludedb_sql_row_get_field(row, 0, &format_name);
        if ( ret < 0 )
                goto error;

        ret = preludedb_set_format(db, preludedb_sql_field_get_value(format_name));
        if ( ret < 0 )
                goto error;

        ret = preludedb_sql_row_get_field(row, 1, &format_version);
        if ( ret < 0 )
                goto error;

        ret = db->plugin->check_schema_version(preludedb_sql_field_get_value(format_version));
        if ( ret < 0 )
                goto error;

        db->format_version = strdup(preludedb_sql_field_get_value(format_version));
        if ( ! db->format_version )
                ret = prelude_error_from_errno(errno);

 error:
        preludedb_sql_table_destroy(table);

        return ret;

}



/**
 * preludedb_new:
 * @db: Pointer to a db object to initialize.
 * @sql: Pointer to a sql object.
 * @format_name: Format name of the underlying database, if NULL the format will be automatically detected
 * @errbuf: Buffer that will be set to an error message if an error occur.
 * @size: size of @errbuf.
 *
 * This function initialize the @db object and detect the format of the underlying database if no format name
 * is given.
 *
 * Returns: 0 on success or a negative value if an error occur.
 */
int preludedb_new(preludedb_t **db, preludedb_sql_t *sql, const char *format_name, char *errbuf, size_t size)
{
        int ret;

        /*
         * FIXME: format_name, errbuf, are deprecated.
         */
        prelude_return_val_if_fail(sql, prelude_error(PRELUDE_ERROR_ASSERTION));

        *db = calloc(1, sizeof (**db));
        if ( ! *db ) {
                ret = preludedb_error_from_errno(errno);
                snprintf(errbuf, size, "%s", preludedb_strerror(ret));
                return ret;
        }

        (*db)->refcount = 1;
        (*db)->sql = preludedb_sql_ref(sql);

        if ( format_name )
                ret = preludedb_set_format(*db, format_name);
        else
                ret = preludedb_autodetect_format(*db);

        if ( ret < 0 ) {
                if ( errbuf )
                        preludedb_get_error(*db, ret, errbuf, size);

                if ( (*db)->format_version )
                        free((*db)->format_version);

                free(*db);
        }

        return ret;
}




preludedb_t *preludedb_ref(preludedb_t *db)
{
        db->refcount++;
        return db;
}




/**
 * preludedb_destroy:
 * @db: Pointer to a db object.
 *
 * Destroy @db object and the underlying @sql object given as argument to preludedb_new.
 */
void preludedb_destroy(preludedb_t *db)
{
        if ( --db->refcount != 0 )
                return;

        preludedb_sql_destroy(db->sql);
        free(db->format_version);
        free(db);
}



/**
 * preludedb_get_format_name:
 * @db: Pointer to a db object.
 *
 * Returns: the format name currently used by the @db object.
 */
const char *preludedb_get_format_name(preludedb_t *db)
{
        return prelude_plugin_get_name(db->plugin);
}



/**
 * preludedb_get_format_version:
 * @db: Pointer to a db object.
 *
 * Returns: the format version currently used by the @db object.
 */
const char *preludedb_get_format_version(preludedb_t *db)
{
        return db->format_version;
}



/**
 * preludedb_set_format:
 * @db: Pointer to a db object.
 * @format_name: New format to use.
 *
 * Change the current format plugin.
 *
 * Returns: 0 on success or negative value if an error occur.
 */
int preludedb_set_format(preludedb_t *db, const char *format_name)
{
        db->plugin = (preludedb_plugin_format_t *) prelude_plugin_search_by_name(&plugin_format_list, format_name);
        if ( ! db->plugin )
                return preludedb_error_verbose(PRELUDEDB_ERROR_CANNOT_LOAD_FORMAT_PLUGIN,
                                               "could not find format plugin '%s'", format_name);

        return 0;
}



/**
 * preludedb_get_sql:
 * @db: Pointer to a db object.
 *
 * Returns: a pointer to the underlying sql object.
 */
preludedb_sql_t *preludedb_get_sql(preludedb_t *db)
{
        return db->sql;
}



/**
 * preludedb_get_error:
 * @db: Pointer to a db object.
 * @error: Error code to build the error string from.
 * @errbuf: Buffer where the error message will be stored,
 * @size: size of this buffer must be PRELUDEDB_ERRBUF_SIZE.
 *
 * Build an error message from the error code given as argument and from
 * the sql plugin error string (if any) if the error code is db related.
 *
 * FIXME: deprecated.
 *
 * Returns: a pointer to the error string or NULL if an error occured.
 */
char *preludedb_get_error(preludedb_t *db, preludedb_error_t error, char *errbuf, size_t size)
{
        int ret;
        preludedb_error_t tmp;

        tmp = preludedb_error(prelude_error_get_code(error));

        ret = snprintf(errbuf, size, "%s: %s", preludedb_strerror(tmp), preludedb_strerror(error));
        if ( ret < 0 || (size_t) ret >= size )
                return NULL;

        return errbuf;
}



/**
 * preludedb_insert_message:
 * @db: Pointer to a db object.
 * @message: Pointer to an IDMEF message.
 *
 * Insert an IDMEF message into the database.
 *
 * Returns: 0 on success, or a negative value if an error occur.
 */
int preludedb_insert_message(preludedb_t *db, idmef_message_t *message)
{
        return db->plugin->insert_message(db->sql, message);
}



preludedb_result_idents_t *preludedb_result_idents_ref(preludedb_result_idents_t *results)
{
        results->refcount++;
        return results;
}


/**
 * preludedb_result_idents_destroy:
 * @result: Pointer to an idents result object.
 *
 * Destroy the @result object.
 */
void preludedb_result_idents_destroy(preludedb_result_idents_t *result)
{
        if ( --result->refcount != 0 )
                return;

        result->db->plugin->destroy_message_idents_resource(result->res);
        preludedb_destroy(result->db);

        free(result);
}


/**
 * preludedb_result_idents_get_next:
 * @result: Pointer to an idents result object.
 * @ident: Pointer to an ident where the next ident will be stored.
 *
 * Retrieve the next ident from the idents result object.
 *
 * Returns: 1 if an ident is available, 0 if there is no more idents available or
 * a negative value if an error occur.
 */
int preludedb_result_idents_get_next(preludedb_result_idents_t *result, uint64_t *ident)
{
        return result->db->plugin->get_next_message_ident(result->res, ident);
}



/**
 * preludedb_result_idents_get:
 * @result: Pointer to an idents result object.
 * @row_index: Row index to retrieve the ident from.
 * @ident: Pointer to an ident where the next ident will be stored.
 *
 * Retrieve the ident located at @row_index from the idents result object.
 *
 * Returns: 1 if an ident is available, 0 if there is no more idents available or
 * a negative value if an error occur.
 */
int preludedb_result_idents_get(preludedb_result_idents_t *result, unsigned int row_index, uint64_t *ident)
{
        if ( ! result->db->plugin->get_message_ident )
                return preludedb_error_verbose(PRELUDEDB_ERROR_GENERIC, "format plugin doesn't implement ident retrieval by index");

        return result->db->plugin->get_message_ident(result->res, row_index, ident);
}


unsigned int preludedb_result_idents_get_count(preludedb_result_idents_t *result)
{
        if ( ! result->db->plugin->get_message_ident_count )
                return preludedb_error_verbose(PRELUDEDB_ERROR_GENERIC, "format plugin doesn't implement ident count retrieval");

        return result->db->plugin->get_message_ident_count(result->res);
}



/**
 * preludedb_result_values_destroy:
 * @result: Pointer to a result values object.
 *
 * Destroy the @result object.
 */
void preludedb_result_values_destroy(preludedb_result_values_t *result)
{
        if ( --result->refcount != 0 )
                return;

        result->db->plugin->destroy_values_resource(result->res);
        preludedb_path_selection_destroy(result->selection);
        preludedb_destroy(result->db);

        free(result);
}



preludedb_path_selection_t *preludedb_result_values_get_selection(preludedb_result_values_t *result)
{
        return result->selection;
}


/**
 * preludedb_result_values_get_next:
 * @result: Pointer to a values result object.
 * @values: Pointer to a values array where the next row of values will be stored.
 *
 * Retrieve the next values row from the values result object.
 *
 * Returns: the number of returned values, 0 if there are no values or a negative value if an
 * error occur.
 */
int preludedb_result_values_get_next(preludedb_result_values_t *result, idmef_value_t ***values)
{
        if ( ! result->db->plugin->get_next_values )
                return preludedb_error_verbose(PRELUDEDB_ERROR_GENERIC, "format plugin doesn't implement value iteration");

        return result->db->plugin->get_next_values(result->res, result->selection, values);
}



int preludedb_result_values_get_row(preludedb_result_values_t *result, unsigned int rownum, void **row)
{
        if ( ! result->db->plugin->get_result_values_row )
                return preludedb_error_verbose(PRELUDEDB_ERROR_GENERIC, "format plugin doesn't implement value selection");

        return result->db->plugin->get_result_values_row(result, rownum, row);
}


int preludedb_result_values_get_field(preludedb_result_values_t *result, void *row, preludedb_selected_path_t *selected, idmef_value_t **field)
{
        if ( ! result->db->plugin->get_result_values_field )
                return preludedb_error_verbose(PRELUDEDB_ERROR_GENERIC, "format plugin doesn't implement value selection");

        return result->db->plugin->get_result_values_field(result, row, selected, field);
}



static int
preludedb_get_message_idents(preludedb_t *db,
                             idmef_criteria_t *criteria,
                             int (*get_idents)(preludedb_sql_t *sql, idmef_criteria_t *criteria,
                                               int limit, int offset,
                                               preludedb_result_idents_order_t order,
                                               void **res),
                             int limit, int offset,
                             preludedb_result_idents_order_t order,
                             preludedb_result_idents_t **result)
{
        int ret;

        *result = calloc(1, sizeof(**result));
        if ( ! *result )
                return preludedb_error_from_errno(errno);

        ret = get_idents(db->sql, criteria, limit, offset, order, &(*result)->res);
        if ( ret <= 0 ) {
                free(*result);
                return ret;
        }

        (*result)->refcount++;
        (*result)->db = preludedb_ref(db);

        return ret;
}



/**
 * preludedb_get_alert_idents:
 * @db: Pointer to a db object.
 * @criteria: Pointer to an idmef criteria.
 * @limit: Limit of results or -1 if no limit.
 * @offset: Offset in results or -1 if no offset.
 * @order: Result order.
 * @result: Idents result.
 *
 * Returns: the number of result or a negative value if an error occured.
 */
int preludedb_get_alert_idents(preludedb_t *db,
                               idmef_criteria_t *criteria, int limit, int offset,
                               preludedb_result_idents_order_t order,
                               preludedb_result_idents_t **result)
{
        return preludedb_get_message_idents(db, criteria, db->plugin->get_alert_idents, limit, offset, order, result);
}



/**
 * preludedb_get_heartbeat_idents:
 * @db: Pointer to a db object.
 * @criteria: Pointer to an idmef criteria.
 * @limit: Limit of results or -1 if no limit.
 * @offset: Offset in results or -1 if no offset.
 * @order: Result order.
 * @result: Idents result.
 *
 * Returns: the number of result or a negative value if an error occured.
 */
int preludedb_get_heartbeat_idents(preludedb_t *db,
                                   idmef_criteria_t *criteria, int limit, int offset,
                                   preludedb_result_idents_order_t order,
                                   preludedb_result_idents_t **result)
{
        return preludedb_get_message_idents(db, criteria, db->plugin->get_heartbeat_idents, limit, offset, order, result);
}



/**
 * preludedb_get_alert:
 * @db: Pointer to a db object.
 * @ident: Internal database ident of the alert.
 * @message: Pointer to an idmef message object where the retrieved message will be stored.
 *
 * Returns: 0 on success or a negative value if an error occur.
 */
int preludedb_get_alert(preludedb_t *db, uint64_t ident, idmef_message_t **message)
{
        return db->plugin->get_alert(db->sql, ident, message);
}



/**
 * preludedb_get_heartbeat:
 * @db: Pointer to a db object.
 * @ident: Internal database ident of the heartbeat.
 * @message: Pointer to an idmef message object where the retrieved message will be stored.
 *
 * Returns: 0 on success or a negative value if an error occur.
 */
int preludedb_get_heartbeat(preludedb_t *db, uint64_t ident, idmef_message_t **message)
{
        return db->plugin->get_heartbeat(db->sql, ident, message);
}


/**
 * preludedb_delete_alert:
 * @db: Pointer to a db object.
 * @ident: Internal database ident of the alert.
 *
 * Delete an alert.
 *
 * Returns: 0 on success, or a negative value if an error occur.
 */
int preludedb_delete_alert(preludedb_t *db, uint64_t ident)
{
        return db->plugin->delete_alert(db->sql, ident);
}


/**
 * preludedb_delete_alert_from_list:
 * @db: Pointer to a db object.
 * @idents: Pointer to an array of idents.
 * @size: Size of @idents.
 *
 * Delete all alerts from ident stored within @idents.
 *
 * Returns: the number of alert deleted on success, or a negative value if an error occur.
 */
ssize_t preludedb_delete_alert_from_list(preludedb_t *db, uint64_t *idents, size_t isize)
{
        if ( isize == 0 )
                return 0;

        return _preludedb_plugin_format_delete_alert_from_list(db->plugin, db->sql, idents, isize);
}


/**
 * preludedb_delete_alert_from_result_idents:
 * @db: Pointer to a db object.
 * @result: Pointer to an idents result object.
 *
 * Delete all alert from ident stored within @result.
 *
 * Returns: the number of alert deleted on success, or a negative value if an error occur.
 */
ssize_t preludedb_delete_alert_from_result_idents(preludedb_t *db, preludedb_result_idents_t *result)
{
        return _preludedb_plugin_format_delete_alert_from_result_idents(db->plugin, db->sql, result);
}



/**
 * preludedb_delete_heartbeat:
 * @db: Pointer to a db object.
 * @ident: Internal database ident of the heartbeat.
 *
 * Delete an heartbeat.
 *
 * Returns: 0 on success, or a negative value if an error occur.
 */
int preludedb_delete_heartbeat(preludedb_t *db, uint64_t ident)
{
        return db->plugin->delete_heartbeat(db->sql, ident);
}


/**
 * preludedb_delete_heartbeat_from_list:
 * @db: Pointer to a db object.
 * @idents: Pointer to an array of idents.
 * @size: Size of @idents.
 *
 * Delete all heartbeat from ident stored within @idents.
 *
 * Returns: the number of heartbeat deleted on success, or a negative value if an error occur.
 */
ssize_t preludedb_delete_heartbeat_from_list(preludedb_t *db, uint64_t *idents, size_t isize)
{
        if ( isize == 0 )
                return 0;

        return _preludedb_plugin_format_delete_heartbeat_from_list(db->plugin, db->sql, idents, isize);
}



/**
 * preludedb_delete_heartbeat_from_result_idents:
 * @db: Pointer to a db object.
 * @result: Pointer to an idents result object.
 *
 * Delete all heartbeat from ident stored within @result.
 *
 * Returns: the number of heartbeat deleted on success, or a negative value if an error occur.
 */
ssize_t preludedb_delete_heartbeat_from_result_idents(preludedb_t *db, preludedb_result_idents_t *result)
{
        return _preludedb_plugin_format_delete_heartbeat_from_result_idents(db->plugin, db->sql, result);
}



/**
 * preludedb_get_values:
 * @db: Pointer to a db object.
 * @path_selection: Pointer to a path selection.
 * @criteria: Pointer to a criteria object.
 * @distinct: Get distinct or not distinct result rows.
 * @limit: Limit of results or -1 if no limit.
 * @offset: Offset in results or -1 if no offset.
 * @result: Values result.
 *
 * Returns: the number of result or a negative value if an error occured.
 */
int preludedb_get_values(preludedb_t *db,
                         preludedb_path_selection_t *path_selection,
                         idmef_criteria_t *criteria,
                         prelude_bool_t distinct,
                         int limit, int offset,
                         preludedb_result_values_t **result)
{
        int ret;

        *result = calloc(1, sizeof (**result));
        if ( ! *result )
                return preludedb_error_from_errno(errno);

        ret = db->plugin->get_values(db->sql, path_selection, criteria, distinct, limit, offset, &(*result)->res);
        if ( ret <= 0 ) {
                free(*result);
                return ret;
        }

        (*result)->refcount = 1;
        (*result)->db = preludedb_ref(db);
        (*result)->selection = preludedb_path_selection_ref(path_selection);

        return ret;
}



void *preludedb_result_values_get_data(preludedb_result_values_t *results)
{
        return results->res;
}



int preludedb_result_values_get_count(preludedb_result_values_t *results)
{
        if ( ! results->db->plugin->get_result_values_count )
                return preludedb_error_verbose(PRELUDEDB_ERROR_GENERIC, "format plugin doesn't implement value count retrieval");

        return results->db->plugin->get_result_values_count(results);
}



preludedb_result_values_t *preludedb_result_values_ref(preludedb_result_values_t *results)
{
        results->refcount++;
        return results;
}


unsigned int preludedb_result_values_get_field_count(preludedb_result_values_t *results)
{
        return preludedb_path_selection_get_count(results->selection);
}


ssize_t preludedb_update_from_list(preludedb_t *db,
                                   const idmef_path_t **paths, const idmef_value_t **values, size_t pvsize,
                                   uint64_t *idents, size_t isize)
{
       if ( ! db->plugin->update_from_list )
                return preludedb_error_from_errno(ENOTSUP);

        return db->plugin->update_from_list(db->sql, paths, values, pvsize, idents, isize);
}


ssize_t preludedb_update_from_result_idents(preludedb_t *db,
                                            const idmef_path_t **paths, const idmef_value_t **values, size_t pvsize,
                                            preludedb_result_idents_t *result)
{
        if ( ! db->plugin->update_from_result_idents )
                return preludedb_error_from_errno(ENOTSUP);

        return db->plugin->update_from_result_idents(db->sql, paths, values, pvsize, result);
}



/**
 * preludedb_update:
 * @db: Pointer to a db object.
 * @paths: Array of path to update
 * @values: Array of value for their respective @paths.
 * @pvsize: Number of element in the @paths and @values array.
 * @criteria: Criteria updated event should match.
 * @order: Optional list of path used to order the update command.
 * @limit: Limit of results or -1 if no limit.
 * @offset: Offset in results or -1 if no offset.
 *
 *
 * Returns: the number of result or a negative value if an error occured.
 */

int preludedb_update(preludedb_t *db,
                     const idmef_path_t **paths, const idmef_value_t **values, size_t pvsize,
                     idmef_criteria_t *criteria,
                     preludedb_path_selection_t *order,
                     int limit, int offset)
{
        if ( ! db->plugin->update )
                return preludedb_error_from_errno(ENOTSUP);

        return db->plugin->update(db->sql, paths, values, pvsize, criteria, order, limit, offset);
}



/**
 * preludedb_transaction_start:
 * @db: Pointer to a #preludedb_t object.
 *
 * Begin a transaction using @db object. Internal transaction
 * handling will be disabled until preludedb_transaction_end()
 * or preludedb_transaction_abort() is called.
 *
 * Returns: 0 on success or a negative value if an error occur.
 */
int preludedb_transaction_start(preludedb_t *db)
{
        int ret;

        ret = _preludedb_sql_transaction_start(db->sql);
        if ( ret < 0 )
                return ret;

        _preludedb_sql_disable_internal_transaction(db->sql);

        return ret;
}



/**
 * preludedb_transaction_end:
 * @db: Pointer to a #preludedb_t object.
 *
 * Terminate a sql transaction (SQL COMMIT command) initiated
 * with preludedb_transaction_start(). Internal transaction
 * handling will be enabled again once this function return.
 *
 * Returns: 0 on success or a negative value if an error occur.
 */
int preludedb_transaction_end(preludedb_t *db)
{
        int ret;

        ret = _preludedb_sql_transaction_end(db->sql);
        _preludedb_sql_enable_internal_transaction(db->sql);

        if ( ret < 0 )
                return ret;

        return ret;
}



/**
 * preludedb_transaction_abort:
 * @db: Pointer to a #preludedb_t object.
 *
 * Abort a sql transaction (SQL ROLLBACK command) initiated
 * with preludedb_transaction_start(). Internal transaction
 * handling will be enabled again once this function return.
 *
 * Returns: 0 on success or a negative value if an error occur.
 */
int preludedb_transaction_abort(preludedb_t *db)
{
        int ret;

        ret = _preludedb_sql_transaction_abort(db->sql);
        _preludedb_sql_enable_internal_transaction(db->sql);

        if ( ret < 0 )
                return ret;

        return ret;
}
