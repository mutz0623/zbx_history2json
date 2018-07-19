#include "config_load.h"

int  CONFIG_JSON_OUTPUT_ENABLE = 0;
char *CONFIG_JSON_OUTPUT_PATH = NULL;
char *CONFIG_JSON_OUTPUT_FILENAME = NULL;
int CONFIG_JSON_OUTPUT_PID = 0;
int CONFIG_JSON_OUTPUT_HOSTINFO = 0;
int CONFIG_JSON_OUTPUT_ITEMINFO = 0;
int CONFIG_JSON_OUTPUT_TYPE = 0;
int CONFIG_JSON_OUTPUT_SEP_DATE = 0;
int CONFIG_JSON_OUTPUT_SEP_TYPE = 0;


/*********************************************************************
 * zbx_module_load_config                                            *
 *********************************************************************/
void     zbx_module_load_config(void)
{

	char *MODULE_CONFIG_FILE = NULL;

	static struct cfg_line  module_cfg[] =
	{
		/* PARAMETER,			VAR,				TYPE,
				MANDATORY,		MIN,		MAX */
		{"JSONOutputEnable",		&CONFIG_JSON_OUTPUT_ENABLE,	TYPE_INT,
				PARM_OPT,		0,		1},
		{"JSONOutputPath",		&CONFIG_JSON_OUTPUT_PATH,	TYPE_STRING,
				PARM_OPT,		0,		0},
		{"JSONOutputFilenameBase",	&CONFIG_JSON_OUTPUT_FILENAME,	TYPE_STRING,
				PARM_OPT,		0,		0},
		{"JSONOutputPID",		&CONFIG_JSON_OUTPUT_PID,	TYPE_INT,
				PARM_OPT,		0,		1},
		{"JSONOutputHostinfo",		&CONFIG_JSON_OUTPUT_HOSTINFO,	TYPE_INT,
				PARM_OPT,		0,		1},
		{"JSONOutputIteminfo",		&CONFIG_JSON_OUTPUT_ITEMINFO,	TYPE_INT,
				PARM_OPT,		0,		1},
		{"JSONOutputItemType",		&CONFIG_JSON_OUTPUT_TYPE,	TYPE_INT,
				PARM_OPT,		0,		1},
		{"JSONOutputSeparateDate",	&CONFIG_JSON_OUTPUT_SEP_DATE,	TYPE_INT,
				PARM_OPT,		0,		1},
		{"JSONOutputSeparateType",	&CONFIG_JSON_OUTPUT_SEP_TYPE,	TYPE_INT,
				PARM_OPT,		0,		1},
		{NULL}
	};


	MODULE_CONFIG_FILE = zbx_dsprintf(MODULE_CONFIG_FILE, "%s/%s",
	                                  CONFIG_LOAD_MODULE_PATH, MODULE_CONFIG_FILE_NAME);

	zabbix_log(LOG_LEVEL_WARNING, "[%s] load conf:%s",
	           MODULE_NAME, MODULE_CONFIG_FILE);

	parse_cfg_file(MODULE_CONFIG_FILE, module_cfg,
	               ZBX_CFG_FILE_REQUIRED, ZBX_CFG_STRICT);

	zbx_free(MODULE_CONFIG_FILE);

	zbx_module_set_defaults();

}


void     zbx_module_set_defaults(void)
{

	if (NULL == CONFIG_JSON_OUTPUT_PATH){
		CONFIG_JSON_OUTPUT_PATH = zbx_strdup(CONFIG_JSON_OUTPUT_PATH, "/var/log/zabbix/");
	}

	if (NULL == CONFIG_JSON_OUTPUT_FILENAME){
		CONFIG_JSON_OUTPUT_FILENAME = zbx_strdup(CONFIG_JSON_OUTPUT_FILENAME, "history");
	}

}

