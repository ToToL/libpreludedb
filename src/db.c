/*****
*
* Copyright (C) 2002 Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
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
#include <inttypes.h>
#include <sys/types.h>

#include <libprelude/list.h>
#include <libprelude/prelude-log.h>
#include <libprelude/idmef-tree.h>
#include <libprelude/plugin-common.h>
#include <libprelude/config-engine.h>

#include <libprelude/prelude-io.h>
#include <libprelude/prelude-message.h>
#include <libprelude/prelude-getopt.h>

#include "sql-table.h"
#include "sql-connection-data.h"
#include "plugin-sql.h"
#include "db-type.h"
#include "db-connection.h"
#include "plugin-format.h"
#include "db.h"

static int initialized = 0;



int prelude_db_init(void)
{
	int ret;

	/* FIXME: we should rather return error here */
	if ( initialized++ ) {
		log(LOG_ERR, "attempt to re-initialize db subsystem! pretending to be OK\n");
		return 0;
	}
	
	log(LOG_INFO, "- Starting DB subsystem\n");

#if 0
	ret = db_dispatch_init();
	if (ret < 0)
		return -1;
#endif	

	ret = sql_plugins_init(SQL_PLUGIN_DIR);	
	if ( ret < 0 )
		return -2;
		
	ret = format_plugins_init(FORMAT_PLUGIN_DIR);	
	if ( ret < 0 )
		return -3;
        
#if 0
	ret = filter_plugins_init(FILTER_PLUGIN_DIR, 0, NULL);
	if (ret < 0)
		return -4;
#endif
	
	return 0;
}



int prelude_db_shutdown(void) 
{
	return 0;
}
