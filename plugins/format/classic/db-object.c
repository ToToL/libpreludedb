/*****
*
* Copyright (C) 2003 Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
* All Rights Reserved
*
* This file is part of the Prelude program.
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
* You should have received a copy of the GNU General Public License
* along with this program; see the file COPYING.  If not, write to
* the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>

#include <libprelude/prelude-log.h>
#include <libprelude/idmef.h>
#include <libprelude/prelude-list.h>

#include "preludedb-sql-settings.h"
#include "preludedb-sql.h"

#include "db-object.h"


struct db_object {
	int listed;
	prelude_list_t list;
	idmef_object_t *object;
	char *table;
	char *field;
	char *function;
	char *top_table;
	char *top_field;
	char *condition;
	char *ident_field;
	/* special fields for idmef_time */
	char *usec_field;
	char *gmtoff_field;
};


PRELUDE_LIST_HEAD(db_objects);

static int db_object_count = 0;

db_object_t **db_object_index = NULL;


#define OBJECT_DUPLICATED_ERROR 1
#define PARSE_ERROR 2
#define OTHER_ERROR 3
#define BUFLEN 127

static void db_object_destroy(db_object_t *obj);

/* 
 * Find char 'c' in first 'n' bytes of string 's'
 */

static const char *strnchr(const char *s, char c, int n)
{
	int i;
	
	for ( i = 0; i < n; i++ )
		if ( s[i] == c )
			return s + i;
		
	return NULL;
}

/*
 * Find the first occurence of any of separator characters from string 
 * 'sep' in string 's'.
 *
 * If a `'' or `"' is found, the search is resumed at the next occurence
 * or `'' or `"' respectively. 
 * If a `\' is found, the following character is skipped. 
 */

static const char *find_separator(const char *s, char *sep)
{
	if ( !s || ! sep )
		return NULL;
	
	while ( s && *s ) {
		
		if ( ( *s == '\'' ) && ( *(s-1) != '\\' ) ) {
			/* allow sigle quote to be escaped with a backslash */
			do {
				s = strchr(s+1, '\'');
			} while ( s && ( *(s-1) == '\\' ) ); 
			s++;
			continue;
		}
		
		if ( ( *s == '"' ) && ( *(s-1) != '\\' ) ) {
			/* allow double quote to be escaped with a backslash */
			do {
				s = strchr(s+1, '"');
			} while ( s && ( *(s-1) == '\\') ); 
			s++;
			continue;
			
		}
		
		if ( *s == '\\' ) {
			/* don't step over trailing \0 */
			if (*++s)
				s++;
				
			continue;
		}
		
		if ( strchr(sep, *s) )
			return s;
			
		s++;
	}
	
	return NULL;
}

/* strcpy(3) says that strings may not overlap, so we reimplement */
static void my_strcpy(char *dst, char *src)
{
	if ( !dst || !src )
		return ;
	
	while (*src) 
		*dst++ = *src++;
		
	*dst = '\0';
}

/* unescape backslash-espaced sequences */
static void unescape(char *s)
{
	if ( !s ) 
		return ;
	
	while ( *s ) {
		if ( *s == '\\' ) 
			my_strcpy(s, s+1);
		
		s++;
	}
}

/* remove quotes surrounding data */
static void unquote(char *s)
{
	int len;
	
	len = strlen(s);

	if ( ( s[0] == '"' ) && ( s[len-1] == '"' ) ) {
		s[len-1] = '\0';
		my_strcpy(s, s+1);
	}


	if ( ( s[0] == '\'' ) && ( s[len-1] == '\'' ) ) {
		s[len-1] = '\0';
		my_strcpy(s, s+1);
	}
			
}

/* 
 * Get value of the parameter 'field' from the configuration string 'string'. 
 *
 * The string must have structure similar to the example below:
 * field1=value1 field2="par11=val11 par12=val12" field3='par22=val22 par23' field4\' 
 *
 * Field name is case insensitive. 
 * If the field is not found, NULL is returned. 
 * If the field value is empty, a pointer to empty string is returned. 
 */
static char *parameter_value(const char *string, const char *field)
{
	char buf[BUFLEN+1]; /* +1 for trailing '\0' */
	const char *s;
	const char *sep;
	const char *data;
	char *ret;
	int len;
	int name_len;
	int data_len;
	
	s = string;
	
	do {
		/*sep = strpbrk(s, " ");*/
		sep = find_separator(s, " \t");
		if ( sep ) {
			if ( sep == s ) {
				s++;
				continue;
			}
			
			len = sep - s;
		} else
			len = strlen(s);
		
		data = strnchr(s, '=', len);
		if ( data ) 
			data++;
		else
			data = s + len;

		data_len = s + len - data;
		name_len = len - data_len - 1;
				
		if ( name_len > BUFLEN )
			name_len = BUFLEN;

		strncpy(buf, s, name_len);
		buf[name_len] = '\0';
			
		if ( strncasecmp(buf, field, name_len) == 0 ) {
			
			ret = malloc(data_len + 1);
			if ( ! ret ) {
				log(LOG_ERR, "memory exhausted.\n");
				return NULL;
			}
			
			strncpy(ret, data, data_len);
			ret[data_len] = '\0';
			
			unescape(ret);
			unquote(ret);
			
			return ret;
		}
		
		s = sep + 1;
	} while (sep && *s);
	
	return NULL;
}




static int db_object_new(char *descr)
{
	db_object_t *obj;
	char *val;
	prelude_list_t *tmp;
	db_object_t *entry, *prev;
	int ret;
	
	obj = calloc(1, sizeof(*obj));
	if ( ! obj ) {
		log(LOG_ERR, "out of memory\n");
		return -1;
	}
	
	val = parameter_value(descr, "object");
	if ( ! val )
		goto parse_error;
	
	obj->object = idmef_object_new(val);
	free(val);
	if ( ! obj->object )
		goto error;

	obj->table = parameter_value(descr, "table");
	if ( ! obj->table )
		goto parse_error;
		
	obj->field = parameter_value(descr, "field");
	obj->function = parameter_value(descr, "function");
	
	if ( ! obj->field && ! obj->function )
		goto parse_error;

	/* Following fields are optional */
	obj->top_table = parameter_value(descr, "top_table");
	obj->top_field = parameter_value(descr, "top_field");
	obj->condition = parameter_value(descr, "condition");
	obj->ident_field = parameter_value(descr, "ident_field");
	obj->usec_field = parameter_value(descr, "usec_field");
	obj->gmtoff_field = parameter_value(descr, "gmtoff_field");
	

	/* From idmef-cache.c -- adapted (shamelessly copying my own code :) */
	
	/*
	 * determine if object is in the list, or where it should be 
	 * added to the list, so the list is sorted by idmef_object_t->id
	 */
	entry = NULL;
	prev = NULL;
	prelude_list_for_each(tmp, &db_objects) {
		entry = prelude_list_entry(tmp, db_object_t, list);
		ret = idmef_object_compare(entry->object, obj->object);
		
		if (ret == 0)
			goto error_duplicated; /* object found */
			
		if (ret > 0) {
			entry = prev;
			break;        /* add after previous object */
		}
		
		prev = entry;
	}	
	
	/* 
	 * @entry@ holds the pointer to object we should add our object 
	 * after, or NULL if we should add it before the first object
	 */
	 
	if ( entry )
		prelude_list_add(&obj->list, &entry->list);
	else 
	 	prelude_list_add(&obj->list, &db_objects);
	 	
	obj->listed = 1;

	return 0;  /* Success */

error_duplicated:
	db_object_destroy(obj);

	return -OBJECT_DUPLICATED_ERROR;
			
parse_error:
	db_object_destroy(obj);
		
	return -PARSE_ERROR;
	
error:
	db_object_destroy(obj);
	
	return -OTHER_ERROR;
}






static void db_object_destroy(db_object_t *obj)
{
	if ( obj->listed )
		prelude_list_del(&obj->list);
	
	if ( obj->object )
		idmef_object_destroy(obj->object);
		
	if ( obj->table )
		free(obj->table);
		
	if ( obj->field )
		free(obj->field);
		
	if ( obj->function)
		free(obj->function);
		
	if ( obj->top_table )
		free(obj->top_table);
		
	if ( obj->top_field )
		free(obj->top_field);
		
	if ( obj->condition )
		free(obj->condition);
		
	if ( obj->ident_field )
		free(obj->ident_field);

	if ( obj->usec_field )
		free(obj->usec_field);

	if ( obj->gmtoff_field )
		free(obj->ident_field);

	free(obj);
}




int db_objects_init(const char *file)
{
	FILE *f;
	char buf[1024];
	int ret;
	int i,line;
	prelude_list_t *tmp;
	db_object_t *entry;
	char *ptr;

	f = fopen(file, "r");
	if ( ! f ) {
		log(LOG_ERR, "could not open file %s\n", file);
		return -1;
	}

#ifdef DEBUG
	log(LOG_INFO, "- Loading database specification from file %s\n", file);
#endif /* DEBUG */
	
	line = 0;
	while ( fgets(buf, sizeof(buf), f) ) {
		line++;

		/* skip comments */
		if ( buf[0] == '#' )
			continue;
			
		/* skip empty line */
		ptr = buf;
		while ( ( *ptr == ' ' ) || ( *ptr == '\t' ) ) ptr++;
		if ( ( *ptr == '\n' ) || ( *ptr == '\0' ) )
			continue;
		
		/* remove trailing \n */
		buf[strlen(buf)-1] = '\0';
		
		ret = db_object_new(buf);
		switch ( ret ) {
		case -OBJECT_DUPLICATED_ERROR:
			log(LOG_ERR, "error: duplicated object on line %d\n", line);
			fclose(f);
			return -1;
		
		case -PARSE_ERROR:
			log(LOG_ERR, "parse error: on line %d\n", line);
			fclose(f);
			return -1;
		
		case -OTHER_ERROR:
			log(LOG_ERR, "error on line %d\n", line);
			fclose(f);
			return -1;
		
		default:
			break;
		}
		
		db_object_count++;
	}
	
	if ( ferror(f) ) {
		log(LOG_ERR, "file read error.\n");
		return -1;
	}
		
	fclose(f);

	db_object_index = calloc(db_object_count, sizeof(*db_object_index));
	if ( ! db_object_index ) {
		log(LOG_ERR, "out of memory\n");
		return -1;
	}
	
	i = 0;
	prelude_list_for_each(tmp, &db_objects) {
		entry = prelude_list_entry(tmp, db_object_t, list);
		db_object_index[i++] = entry;
	}
#ifdef DEBUG
	log(LOG_INFO, "- %d objects loaded and indexed\n", db_object_count);
#endif /* DEBUG */
	
	return 0;
}




/* From idmef-cache.c -- adapted (shamelessly copying my own code :) */
db_object_t *db_object_find(idmef_object_t *object)
{
	int low, high, mid, cmp;
		
	/* Mostly from A.Drozdek, D.L.Simon "Data structures in C" */
	low = 0;
	high = db_object_count-1;
	
	while ( low <= high ) {
		mid = (low + high) / 2;
		cmp = idmef_object_compare(object, db_object_index[mid]->object);
		if ( cmp > 0 )
			low = mid+1;
		else if ( cmp < 0 )
			high = mid-1;
		else
			return db_object_index[mid];
	}
	
	return NULL;
}


char *db_object_get_table(db_object_t *object)
{
	return object->table;
}


char *db_object_get_field(db_object_t *object)
{
	return object->field;
}



char *db_object_get_function(db_object_t *object)
{
	return object->function;
}




char *db_object_get_top_table(db_object_t *object)
{
	return object->top_table;
}



char *db_object_get_top_field(db_object_t *object)
{
	return object->top_field;
}



char *db_object_get_condition(db_object_t *object)
{
	return object->condition;
}



char *db_object_get_ident_field(db_object_t *object)
{
	return object->ident_field;
}



char *db_object_get_usec_field(db_object_t *object)
{
	return object->usec_field;
}



char *db_object_get_gmtoff_field(db_object_t *object)
{
	return object->gmtoff_field;
}




void db_objects_destroy(void)
{
	prelude_list_t *n, *tmp;
	db_object_t *entry;
	
	prelude_list_for_each_safe(tmp, n, &db_objects) {
		entry = prelude_list_entry(tmp, db_object_t, list);
		db_object_destroy(entry);
	}
	
	free(db_object_index);
}
