#ifndef __ZABBIX_CONFIG_LOAD_H
#define __ZABBIX_CONFIG_LOAD_H


#include "sysinc.h"
#include "module.h"
#include "common.h"
#include "log.h"
#include "cfg.h"

#define MODULE_NAME "history2json.so"
#define MODULE_CONFIG_FILE_NAME "history2json.conf"

#define CONFIG_DISABLE 0
#define CONFIG_ENABLE  1

extern char *CONFIG_LOAD_MODULE_PATH ;


extern void zbx_module_load_config(void);
extern void zbx_module_set_defaults(void);

extern int CONFIG_JSON_OUTPUT_ENABLE;
extern char *CONFIG_JSON_OUTPUT_PATH;
extern char *CONFIG_JSON_OUTPUT_FILENAME;
extern int CONFIG_JSON_OUTPUT_PID;
extern int CONFIG_JSON_OUTPUT_HOSTINFO;
extern int CONFIG_JSON_OUTPUT_ITEMINFO;
extern int CONFIG_JSON_OUTPUT_SEP_DATE;
extern int CONFIG_JSON_OUTPUT_SEP_TYPE;
extern int CONFIG_JSON_OUTPUT_TYPE;


#endif /* __ZABBIX_CONFIG_LOAD_H */

