/*
** Zabbix
** Copyright (C) 2001-2017 Zabbix SIA
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/

#include "sysinc.h"
#include "module.h"
#include "common.h"
#include "log.h"
#include "zbxjson.h"
#include "dbcache.h"

#include "config_load.h"

#define MODULE_NAME "history2json.so"

extern unsigned char program_type ;

/* the variable keeps timeout setting for item processing */
static int	item_timeout = 0;

/* module SHOULD define internal functions as static and use a naming pattern different from Zabbix internal */
/* symbols (zbx_*) and loadable module API functions (zbx_module_*) to avoid conflicts                       */
static int	history2json_enable(AGENT_REQUEST *request, AGENT_RESULT *result);
static int	history2json_path(AGENT_REQUEST *request, AGENT_RESULT *result);

static ZBX_METRIC keys[] =
/*	KEY				FLAG		FUNCTION		TEST PARAMETERS */
{
	{"history2json.enable",		0,		history2json_enable,	NULL},
	{"history2json.path",		0,		history2json_path,	NULL},
	{NULL}
};

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_api_version                                           *
 *                                                                            *
 * Purpose: returns version number of the module interface                    *
 *                                                                            *
 * Return value: ZBX_MODULE_API_VERSION - version of module.h module is       *
 *               compiled with, in order to load module successfully Zabbix   *
 *               MUST be compiled with the same version of this header file   *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_api_version(void)
{
	return ZBX_MODULE_API_VERSION;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_item_timeout                                          *
 *                                                                            *
 * Purpose: set timeout value for processing of items                         *
 *                                                                            *
 * Parameters: timeout - timeout in seconds, 0 - no timeout set               *
 *                                                                            *
 ******************************************************************************/
void	zbx_module_item_timeout(int timeout)
{
	item_timeout = timeout;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_item_list                                             *
 *                                                                            *
 * Purpose: returns list of item keys supported by the module                 *
 *                                                                            *
 * Return value: list of item keys                                            *
 *                                                                            *
 ******************************************************************************/
ZBX_METRIC	*zbx_module_item_list(void)
{
	return keys;
}

static int	history2json_enable(AGENT_REQUEST *request, AGENT_RESULT *result)
{
	SET_UI64_RESULT(result, CONFIG_JSON_OUTPUT_ENABLE);

	return SYSINFO_RET_OK;
}

static int	history2json_path(AGENT_REQUEST *request, AGENT_RESULT *result)
{
	SET_STR_RESULT(result, zbx_strdup(NULL, CONFIG_JSON_OUTPUT_PATH) );

	return SYSINFO_RET_OK;
}


/******************************************************************************
 *                                                                            *
 * Function: zbx_module_init                                                  *
 *                                                                            *
 * Purpose: the function is called on agent startup                           *
 *          It should be used to call any initialization routines             *
 *                                                                            *
 * Return value: ZBX_MODULE_OK - success                                      *
 *               ZBX_MODULE_FAIL - module initialization failed               *
 *                                                                            *
 * Comment: the module won't be loaded in case of ZBX_MODULE_FAIL             *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_init(void)
{
	int ret = ZBX_MODULE_FAIL;

	zabbix_log(LOG_LEVEL_DEBUG, "[%s] module loaded by %s process. [%d]",
	           MODULE_NAME, get_program_type_string(program_type), program_type);

	switch (program_type){
		case ZBX_PROGRAM_TYPE_SERVER:
			ret = ZBX_MODULE_OK;
			break;
		case ZBX_PROGRAM_TYPE_PROXY_ACTIVE:
		case ZBX_PROGRAM_TYPE_PROXY_PASSIVE:
		case ZBX_PROGRAM_TYPE_PROXY:
		case ZBX_PROGRAM_TYPE_AGENTD:
			/* nothing to do */
			break;
		default:
			zabbix_log(LOG_LEVEL_WARNING,
			           "[%s] unknown value program_type [%d]",
			           MODULE_NAME, program_type );
	}

	zbx_module_load_config();

	zabbix_log(LOG_LEVEL_WARNING, "[%s] config parameter enable:[%d], path:[%s]",
	           MODULE_NAME, CONFIG_JSON_OUTPUT_ENABLE, CONFIG_JSON_OUTPUT_PATH);
	return ret;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_uninit                                                *
 *                                                                            *
 * Purpose: the function is called on agent shutdown                          *
 *          It should be used to cleanup used resources if there are any      *
 *                                                                            *
 * Return value: ZBX_MODULE_OK - success                                      *
 *               ZBX_MODULE_FAIL - function failed                            *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_uninit(void)
{
	return ZBX_MODULE_OK;
}


/******************************************************************************
 *                                                                            *
 * Functions: history2json_generarl_cb                                        *
 *                                                                            *
 * Purpose: callback functions for generaral data of types                    *
 *                                                                            *
 * Parameters: item_type   - types of history value                           *
 *             history     - array of historical data                         *
 *             history_num - number of elements in history array              *
 *                                                                            *
 ******************************************************************************/
#define H2J_ITEM_FLOAT 1
#define H2J_ITEM_INTEGER 2
#define H2J_ITEM_STRING 3
#define H2J_ITEM_TEXT 4
#define H2J_ITEM_LOG 5

static void	history2json_general_cb(const int item_type, const void *history, int history_num)
{
	int	i;
	FILE	*f;
	char	*filename = NULL;
	char	*str_value = NULL;
	struct	zbx_json j;

	DC_HOST	host;
	DC_ITEM	items;
	int	err;
	char	*hostname = NULL;
	int	hostid = 0;
	char	*itemkey = NULL;

	time_t	t;
	struct	tm tm_tmp;
	char	filename_suffix_date[16];
	char	*filename_suffix_type = NULL;

	sigset_t	orig_mask;
	sigset_t	mask;

	ZBX_HISTORY_FLOAT	*history_float = NULL;
	ZBX_HISTORY_INTEGER	*history_integer = NULL;
	ZBX_HISTORY_STRING	*history_string = NULL;
	ZBX_HISTORY_TEXT	*history_text = NULL;
	ZBX_HISTORY_LOG 	*history_log = NULL;

	static char*	item_type_str[] =
	{
		"", // this shoud not use.
		"float",
		"integer",
		"string",
		"text",
		"log"
	};

	zabbix_log(LOG_LEVEL_DEBUG, "[%s] In %s() %s:%d",
	           MODULE_NAME, __FUNCTION__, __FILE__, __LINE__);

	// if disabled this module, return.
	if( CONFIG_DISABLE == CONFIG_JSON_OUTPUT_ENABLE ) return;

	switch(item_type){
		case  H2J_ITEM_FLOAT:
			history_float = (ZBX_HISTORY_FLOAT*)history;
			break;
		case  H2J_ITEM_INTEGER:
			history_integer = (ZBX_HISTORY_INTEGER*)history;
			break;
		case  H2J_ITEM_STRING:
			history_string = (ZBX_HISTORY_STRING*)history;
			break;
		case  H2J_ITEM_TEXT:
			history_text = (ZBX_HISTORY_TEXT*)history;
			break;
		case  H2J_ITEM_LOG:
			history_log = (ZBX_HISTORY_LOG*)history;
			break;
		default:
			THIS_SHOULD_NEVER_HAPPEN;
	}


	if( history == NULL || history_num == 0 ){
		zabbix_log(LOG_LEVEL_WARNING, "[%s] In %s() %s:%d history is empty",
		           MODULE_NAME, __FUNCTION__, __FILE__, __LINE__);
		return;
	}

	zabbix_log(LOG_LEVEL_DEBUG, "[%s] In %s() %s:%d item value type[%s]",
	          MODULE_NAME, __FUNCTION__, __FILE__, __LINE__, item_type_str[item_type]);

	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGTERM);      /* block SIGTERM, SIGINT to prevent deadlock on log file mutex */
	sigaddset(&mask, SIGINT);

	if (0 > sigprocmask(SIG_BLOCK, &mask, &orig_mask))
		zbx_error("cannot set sigprocmask to block the user signal");


	/* build JSON output file name */

	// separate the JSON data by date, or not.
	if ( CONFIG_ENABLE == CONFIG_JSON_OUTPUT_SEP_DATE ){
		t = time(NULL);
		localtime_r(&t, &tm_tmp);

		if ( 0 ==  strftime( filename_suffix_date, sizeof(filename_suffix_date), ".%F" , &tm_tmp) ){
			zabbix_log(LOG_LEVEL_WARNING, "[%s] In %s() %s:%d Error in strftime",
			           MODULE_NAME, __FUNCTION__, __FILE__, __LINE__);
			return;
		}
	}else{
		filename_suffix_date[0] = 0x00;
	}

	// separate the JSON data by item type, or not.
	if ( CONFIG_ENABLE == CONFIG_JSON_OUTPUT_SEP_TYPE ){
		filename_suffix_type = zbx_dsprintf(NULL, ".%s", item_type_str[item_type]);
	}else{
		filename_suffix_type = zbx_dsprintf(NULL, "");
	}

	filename = zbx_dsprintf(filename, "%s/%s%s%s",
	               CONFIG_JSON_OUTPUT_PATH, CONFIG_JSON_OUTPUT_FILENAME,
	               filename_suffix_type, filename_suffix_date);

	/* open file */
	errno = 0;
	if ( NULL == (f= fopen(filename, "a")) ){
		zabbix_log(LOG_LEVEL_WARNING, "[%s] In %s() %s:%d Error in open file \"%s\" disable it. [%s]",
		           MODULE_NAME, __FUNCTION__, __FILE__, __LINE__, filename, zbx_strerror(errno) );
		CONFIG_JSON_OUTPUT_ENABLE = CONFIG_DISABLE;
		goto quit;
	}

	if( 0 != flock( fileno(f), LOCK_EX) ){
		zabbix_log(LOG_LEVEL_WARNING, "[%s] In %s() %s:%d Error in flock()",
		           MODULE_NAME, __FUNCTION__, __FILE__, __LINE__ );
		goto quit;
	}

	for (i = 0; i < history_num; i++){

		if( CONFIG_ENABLE == CONFIG_JSON_OUTPUT_HOSTINFO ){
			// Get hostname from item key
			switch(item_type){
				case  H2J_ITEM_FLOAT:
					DCconfig_get_hosts_by_itemids( &host, &(history_float[i].itemid), &err, 1);
					break;
				case  H2J_ITEM_INTEGER:
					DCconfig_get_hosts_by_itemids( &host, &(history_integer[i].itemid), &err, 1);
					break;
				case  H2J_ITEM_STRING:
					DCconfig_get_hosts_by_itemids( &host, &(history_string[i].itemid), &err, 1);
					break;
				case  H2J_ITEM_TEXT:
					DCconfig_get_hosts_by_itemids( &host, &(history_text[i].itemid), &err, 1);
					break;
				case  H2J_ITEM_LOG:
					DCconfig_get_hosts_by_itemids( &host, &(history_log[i].itemid), &err, 1);
					break;
				default:
					THIS_SHOULD_NEVER_HAPPEN;
			}

			if( FAIL == err ){
				zabbix_log(LOG_LEVEL_WARNING, "[%s] In %s() %s:%d failed in  DCconfig_get_hosts_by_itemids()",
				           MODULE_NAME, __FUNCTION__, __FILE__, __LINE__ );
				hostid = 0;
				hostname = NULL;
			}else{
				hostid = host.hostid;
				hostname = zbx_strdup(hostname, host.host );
			}
		}

		if( CONFIG_ENABLE == CONFIG_JSON_OUTPUT_ITEMINFO ){
			// Get item key from item id
			switch(item_type){
				case  H2J_ITEM_FLOAT:
					DCconfig_get_items_by_itemids( &items, &(history_float[i].itemid), &err, 1);
					break;
				case  H2J_ITEM_INTEGER:
					DCconfig_get_items_by_itemids( &items, &(history_integer[i].itemid), &err, 1);
					break;
				case  H2J_ITEM_STRING:
					DCconfig_get_items_by_itemids( &items, &(history_string[i].itemid), &err, 1);
					break;
				case  H2J_ITEM_TEXT:
					DCconfig_get_items_by_itemids( &items, &(history_text[i].itemid), &err, 1);
					break;
				case  H2J_ITEM_LOG:
					DCconfig_get_items_by_itemids( &items, &(history_log[i].itemid), &err, 1);
					break;
				default:
					THIS_SHOULD_NEVER_HAPPEN;
			}

			if( FAIL == err ){
				zabbix_log(LOG_LEVEL_WARNING, "[%s] In %s() %s:%d failed in  DCconfig_get_items_by_itemids()",
				           MODULE_NAME, __FUNCTION__, __FILE__, __LINE__ );
				itemkey = NULL;
			}else{
				itemkey = zbx_strdup(itemkey, items.key_orig );
			}
		}

		// Packing item values to json format
		zbx_json_init(&j, ZBX_JSON_STAT_BUF_LEN);

		if( CONFIG_ENABLE == CONFIG_JSON_OUTPUT_PID ){
			zbx_json_adduint64(&j, "pid", getpid());
		}
		if( CONFIG_ENABLE == CONFIG_JSON_OUTPUT_HOSTINFO ){
			zbx_json_adduint64(&j, ZBX_PROTO_TAG_HOSTID, hostid);
			zbx_json_addstring(&j, ZBX_PROTO_TAG_HOST, hostname, ZBX_JSON_TYPE_STRING);
		}
		if( CONFIG_ENABLE == CONFIG_JSON_OUTPUT_TYPE ){
			zbx_json_addstring(&j, ZBX_PROTO_TAG_TYPE, item_type_str[item_type], ZBX_JSON_TYPE_STRING);
		}
		if( CONFIG_ENABLE == CONFIG_JSON_OUTPUT_ITEMINFO ){
			zbx_json_addstring(&j, ZBX_PROTO_TAG_KEY, itemkey, ZBX_JSON_TYPE_STRING);
		}

		switch(item_type){
			case  H2J_ITEM_FLOAT:
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_ITEMID, history_float[i].itemid);
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_CLOCK, history_float[i].clock);
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_NS, history_float[i].ns);
				str_value = zbx_dsprintf(str_value, "%lf", history_float[i].value);
				zbx_json_addstring(&j, ZBX_PROTO_TAG_VALUE, str_value, ZBX_JSON_TYPE_INT);
				zbx_free( str_value );
				break;
			case  H2J_ITEM_INTEGER:
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_ITEMID, history_integer[i].itemid);
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_CLOCK, history_integer[i].clock);
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_NS, history_integer[i].ns);
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_VALUE, history_integer[i].value);
				break;
			case  H2J_ITEM_STRING:
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_ITEMID, history_string[i].itemid);
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_CLOCK, history_string[i].clock);
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_NS, history_string[i].ns);
				zbx_json_addstring(&j, ZBX_PROTO_TAG_VALUE, history_string[i].value, ZBX_JSON_TYPE_STRING);
				break;
			case  H2J_ITEM_TEXT:
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_ITEMID, history_text[i].itemid);
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_CLOCK, history_text[i].clock);
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_NS, history_text[i].ns);
				zbx_json_addstring(&j, ZBX_PROTO_TAG_VALUE, history_text[i].value, ZBX_JSON_TYPE_STRING);
				break;
			case  H2J_ITEM_LOG:
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_ITEMID, history_log[i].itemid);
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_CLOCK, history_log[i].clock);
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_NS, history_log[i].ns);
				zbx_json_addstring(&j, ZBX_PROTO_TAG_VALUE, history_log[i].value, ZBX_JSON_TYPE_STRING);
				zbx_json_addstring(&j, ZBX_PROTO_TAG_LOGSOURCE, history_log[i].source, ZBX_JSON_TYPE_STRING);
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_LOGTIMESTAMP, history_log[i].timestamp);
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_LOGEVENTID, history_log[i].logeventid);
				zbx_json_adduint64(&j, ZBX_PROTO_TAG_LOGSEVERITY, history_log[i].severity);
				break;
			default:
				THIS_SHOULD_NEVER_HAPPEN;
		}

		zbx_json_close(&j);

		zabbix_log(LOG_LEVEL_DEBUG, "[%s] In %s() %s:%d JSON output \"%s\"",
		           MODULE_NAME, __FUNCTION__, __FILE__, __LINE__,
		           j.buffer );

		fprintf(f, "%s\n", j.buffer );

		zbx_json_free(&j);
		zbx_free(hostname);
		zbx_free(itemkey);
		DCconfig_clean_items( &items, &err, 1);
	}


	zabbix_log(LOG_LEVEL_DEBUG, "[%s] In %s() %s:%d synced %d history",
	           MODULE_NAME, __FUNCTION__, __FILE__, __LINE__,
	           history_num);

	if( 0 != fflush(f) ){
		zabbix_log(LOG_LEVEL_WARNING, "[%s] In %s() %s:%d Error in fflush()",
		           MODULE_NAME, __FUNCTION__, __FILE__, __LINE__ );
	}

	if( 0 != flock( fileno(f), LOCK_UN) ){
		zabbix_log(LOG_LEVEL_WARNING, "[%s] In %s() %s:%d Error in flock()",
		           MODULE_NAME, __FUNCTION__, __FILE__, __LINE__ );
		goto quit;
	}

quit:
	zbx_free(filename);
	zbx_free(filename_suffix_type);
	zbx_fclose(f);

	if (0 > sigprocmask(SIG_SETMASK, &orig_mask, NULL))
		zbx_error("cannot restore sigprocmask");

	return;
}

/******************************************************************************
 *                                                                            *
 * Functions: history2json_float_cb                                           *
 *            history2json_integer_cb                                         *
 *            history2json_string_cb                                          *
 *            history2json_text_cb                                            *
 *            history2json_log_cb                                             *
 *                                                                            *
 * Purpose: callback functions for storing historical data of types float,    *
 *          integer, string, text and log respectively in external storage    *
 *                                                                            *
 * Parameters: history     - array of historical data                         *
 *             history_num - number of elements in history array              *
 *                                                                            *
 ******************************************************************************/

static void	history2json_float_cb(const ZBX_HISTORY_FLOAT *history, int history_num)
{
	history2json_general_cb(H2J_ITEM_FLOAT, (const void *)history, history_num);
}

static void	history2json_integer_cb(const ZBX_HISTORY_INTEGER *history, int history_num)
{
	history2json_general_cb(H2J_ITEM_INTEGER, (const void *)history, history_num);
}

static void	history2json_string_cb(const ZBX_HISTORY_STRING *history, int history_num)
{
	history2json_general_cb(H2J_ITEM_STRING, (const void *)history, history_num);
}

static void	history2json_text_cb(const ZBX_HISTORY_TEXT *history, int history_num)
{
	history2json_general_cb(H2J_ITEM_TEXT, (const void *)history, history_num);
}

static void	history2json_log_cb(const ZBX_HISTORY_LOG *history, int history_num)
{
	history2json_general_cb(H2J_ITEM_LOG, (const void *)history, history_num);
}


/******************************************************************************
 *                                                                            *
 * Function: zbx_module_history_write_cbs                                     *
 *                                                                            *
 * Purpose: returns a set of module functions Zabbix will call to export      *
 *          different types of historical data                                *
 *                                                                            *
 * Return value: structure with callback function pointers (can be NULL if    *
 *               module is not interested in data of certain types)           *
 *                                                                            *
 ******************************************************************************/
ZBX_HISTORY_WRITE_CBS	zbx_module_history_write_cbs(void)
{
	static ZBX_HISTORY_WRITE_CBS	history2json_callbacks =
	{
		history2json_float_cb,
		history2json_integer_cb,
		history2json_string_cb,
		history2json_text_cb,
		history2json_log_cb,
	};

	return history2json_callbacks;
}
