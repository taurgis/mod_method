/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * mod_method.c: Currently only supports replacing a method in a request with a
 * new value.
 *
 *     Written by Thomas Theunen, thomas.theunen@gmail.com, 07/08/2015
 *
 * The Method directive can be used to replace the request method.
 * Valid in both per-server and per-dir configurations.
 *
 * Syntax is:
 *
 *   Method action method1 method2
 *
 * Where action is one of:
 *     replace    - Replace the method with the new value.
 *
 * Examples:
 *
 *  To set all OPTIONS requests to a GET request, use
 *     Method replace OPTIONS GET
 *
 */

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "http_request.h"
#include "util_script.h"
#include "http_connection.h"
#include "apr_strings.h"
#include <stdio.h>

typedef struct method_cfg {
        int cmode;
    #define CONFIG_MODE_SERVER 1
    #define CONFIG_MODE_DIRECTORY 2
    #define CONFIG_MODE_COMBO 3
        int local;
        char *action;
        char *value;
        char *value2;
        int congenital;
        char *loc;
} method_cfg;

module AP_MODULE_DECLARE_DATA method_module;

static method_cfg *our_dconfig(const request_rec *r)
{
        return (method_cfg *) ap_get_module_config(r->per_dir_config, &method_module);
}

static const char *cmd_method(cmd_parms *cmd, void *mconfig, const char *args)
{
        method_cfg *cfg = (method_cfg *) mconfig;

        const char *action;
        const char *value;
        const char *value2;

        action = ap_getword_conf(cmd->pool, &args);
        value = *args ? ap_getword_conf(cmd->pool, &args) : NULL;
        value2 =*args ? ap_getword_conf(cmd->pool, &args) : NULL;

        cfg->local = 1;
        cfg->action = action;
        cfg->value = value;
        cfg->value2 = value2;

        return NULL;
}

static void *method_create_dir_config(apr_pool_t *p, char *dirspec)
{
        method_cfg *cfg;
        char *dname = dirspec;

        cfg = (method_cfg *) apr_pcalloc(p, sizeof(method_cfg));

        cfg->local = 0;
        cfg->congenital = 0;
        cfg->cmode = CONFIG_MODE_DIRECTORY;

        dname = (dname != NULL) ? dname : "";
        cfg->loc = apr_pstrcat(p, "DIR(", dname, ")", NULL);

        return (void *) cfg;
}

static void *method_merge_dir_config(apr_pool_t *p, void *parent_conf,
                                     void *newloc_conf)
{
        method_cfg *merged_config = (method_cfg *) apr_pcalloc(p, sizeof(method_cfg));
        method_cfg *pconf = (method_cfg *) parent_conf;
        method_cfg *nconf = (method_cfg *) newloc_conf;

        merged_config->local = nconf->local;
        merged_config->loc = apr_pstrdup(p, nconf->loc);

        merged_config->congenital = (pconf->congenital | pconf->local);

        merged_config->cmode =
                (pconf->cmode == nconf->cmode) ? pconf->cmode : CONFIG_MODE_COMBO;

        return (void *) merged_config;
}

static void *method_create_server_config(apr_pool_t *p, server_rec *s)
{
        method_cfg *cfg;
        char *sname = s->server_hostname;

        cfg = (method_cfg *) apr_pcalloc(p, sizeof(method_cfg));
        cfg->local = 0;
        cfg->congenital = 0;
        cfg->cmode = CONFIG_MODE_SERVER;

        sname = (sname != NULL) ? sname : "";
        cfg->loc = apr_pstrcat(p, "SVR(", sname, ")", NULL);

        return (void *) cfg;
}


static void *method_merge_server_config(apr_pool_t *p, void *server1_conf,
                                        void *server2_conf)
{
        method_cfg *merged_config = (method_cfg *) apr_pcalloc(p, sizeof(method_cfg));
        method_cfg *s1conf = (method_cfg *) server1_conf;
        method_cfg *s2conf = (method_cfg *) server2_conf;

        merged_config->cmode =
                (s1conf->cmode == s2conf->cmode) ? s1conf->cmode : CONFIG_MODE_COMBO;
        merged_config->local = s2conf->local;
        merged_config->congenital = (s1conf->congenital | s1conf->local);
        merged_config->loc = apr_pstrdup(p, s2conf->loc);

        return (void *) merged_config;
}

static int method_fixer_upper(request_rec *r)
{
        method_cfg *cfg;

        cfg = our_dconfig(r);

        if (!strcasecmp(cfg->action, "replace")) {
                if((cfg->value != NULL) && (cfg->value2 != NULL)) {
                        if(!strcasecmp(r->method,cfg->value)) {
                                r->method = cfg->value2;
                        }
                }
        }

        return OK;
}

static void method_register_hooks(apr_pool_t *p)
{
        ap_hook_fixups(method_fixer_upper, NULL, NULL, APR_HOOK_FIRST);
}

static const command_rec method_cmds[] =
{
        AP_INIT_RAW_ARGS(
                "Method",
                cmd_method,
                NULL,
                OR_OPTIONS,
                "Method - Sets the request method the the supplied value."
                ),
        {NULL}
};

module AP_MODULE_DECLARE_DATA method_module =
{
        STANDARD20_MODULE_STUFF,
        method_create_dir_config, /* per-directory config creator */
        method_merge_dir_config, /* dir config merger */
        method_create_server_config, /* server config creator */
        method_merge_server_config, /* server config merger */
        method_cmds,             /* command table */
        method_register_hooks,   /* set up other request processing hooks */
};
