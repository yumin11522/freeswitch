#include <switch.h>
#include <switch_curl.h>

#ifndef __MOD_SRS_H__
#define __MOD_SRS_H__

#define MAX_BIND_EVENT 20

char *xmpp_version();

switch_status_t xmpp_load SWITCH_MODULE_LOAD_ARGS;
switch_status_t xmpp_shutdown SWITCH_MODULE_SHUTDOWN_ARGS;

typedef struct {
	char *chan_var_name;
	char *default_value;
	switch_bool_t quote;
	char *fallback_var_name;
} xmpp_param_t;

typedef struct xmpp_bind_event_s {
	char *event_name;
	char *sub_name;
} xmpp_bind_event_t;


typedef struct mod_xmpp_globals_s {
	xmpp_bind_event_t bind_events[MAX_BIND_EVENT];
	switch_memory_pool_t *pool;
	int debug;
	char *uuid;
	char *node_ip;
	char *node_name;
	int dial_queue;
	switch_mutex_t *sps_mutex;
	int sps_total;
	int sps;
	int running;
	switch_mutex_t *cache_mutex;

} mod_xmpp_globals_t;

extern mod_xmpp_globals_t mod_xmpp_globals;

#define add_current_node(json)                                              \
	do {                                                                    \
		cJSON_AddStringToObject(json, "node_uuid", mod_xmpp_globals.node_name); \
	} while (0)


/**
 * 处理 XMPP 响应的 SDP 内容
 * @param session 
 * @param sdp
 * @return
 */
switch_status_t handle_xmpp_answer(switch_core_session_t *session, const char *sdp);

#endif

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:nil
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet expandtab:
 */
