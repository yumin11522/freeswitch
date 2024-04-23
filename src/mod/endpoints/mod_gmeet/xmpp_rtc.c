#include <switch.h>
#include <switch_http.h>
#include <switch_stun.h>

#include "mod_xmpp.h"
#include "xmpp_client.h"

#define VERTO_CHAT_PROTO "xmpp"

#define set_string(x, y) strncpy(x, y, sizeof(x) - 1)

const char *modname = "mod_xmpp";

typedef enum { PTYPE_CLIENT = (1 << 0), PTYPE_CLIENT_SSL = (1 << 1) } jsock_type_t;

typedef enum {
	JPFLAG_INIT = (1 << 0),
	JPFLAG_AUTHED = (1 << 1),
	JPFLAG_CHECK_ATTACH = (1 << 2),
	JPFLAG_EVENTS = (1 << 3),
	JPFLAG_AUTH_EVENTS = (1 << 4),
	JPFLAG_ALL_EVENTS_AUTHED = (1 << 5)
} jpflag_t;

struct xmpp_profile_s;

struct jsock_s {
	switch_memory_pool_t *pool;
	switch_thread_t *thread;
	char *name;
	jsock_type_t ptype;

	uint8_t drop;
	uint8_t nodelete;

	jpflag_t flags;

	char uuid_str[SWITCH_UUID_FORMATTED_LENGTH + 1];
	char ctrl_uuid[SWITCH_UUID_FORMATTED_LENGTH + 1];

	char *id;
	char *domain;
	char *uid;
	char *dialplan;
	char *context;

	struct xmpp_profile_s *profile;
	switch_thread_rwlock_t *rwlock;

	switch_mutex_t *write_mutex;
	switch_event_t *params;
	switch_event_t *vars;
	switch_event_t *user_vars;

	struct jsock_s *next;
};

typedef struct jsock_s jsock_t;

#define MAX_BIND 25
#define MAX_RTPIP 25

typedef struct ips {
	char local_ip[256];
	uint16_t local_port;
	int secure;
} ips_t;

typedef enum { TFLAG_SENT_MEDIA = (1 << 0), TFLAG_ATTACH_REQ = (1 << 1), TFLAG_TRACKED = (1 << 2) } tflag_t;

typedef struct xmpp_pvt_s {
	switch_memory_pool_t *pool;
	char *destination_number;
	char *ctrl_uuid;
	char *call_id;
	char *r_sdp;
	tflag_t flags;
	switch_core_session_t *session;
	switch_channel_t *channel;
	switch_media_handle_t *smh;
	switch_core_media_params_t *mparams;
	switch_call_cause_t remote_hangup_cause;
	time_t detach_time;
	struct xmpp_pvt_s *next;
	switch_byte_t text_read_frame_data[SWITCH_RTP_MAX_BUF_LEN];
	switch_frame_t text_read_frame;

	switch_thread_cond_t *text_cond;
	switch_mutex_t *text_cond_mutex;

	jsock_t *jsock;	 // the old jsock object
	int got_bye;

	char *url;

	xmpp_client_t *client;
} xmpp_pvt_t;

struct xmpp_profile_s {
	char *name;
	switch_mutex_t *mutex;
	switch_memory_pool_t *pool;
	switch_thread_rwlock_t *rwlock;

	struct ips ip[MAX_BIND];
	int i;

	char cert[512];
	char key[512];
	char chain[512];

	jsock_t *jsock_head;
	int jsock_count;
	int running;

	int debug;

	int in_thread;
	int blind_reg;

	char *userauth;
	char *root_passwd;

	char *context;
	char *dialplan;

	char *mcast_ip;
	switch_port_t mcast_port;

	char *extrtpip;

	char *rtpip[MAX_RTPIP];
	int rtpip_index;
	int rtpip_cur;

	char *rtpip6[MAX_RTPIP];
	int rtpip_index6;
	int rtpip_cur6;

	char *cand_acl[SWITCH_MAX_CAND_ACL];
	uint32_t cand_acl_count;

	char *conn_acl[SWITCH_MAX_CAND_ACL];
	uint32_t conn_acl_count;

	char *inbound_codec_string;
	char *outbound_codec_string;

	char *timer_name;
	char *local_network;

	char *register_domain;

	int enable_text;
	char *media_timeout;

	char *xmpp_ctrl_subject;
	struct xmpp_profile_s *next;
};

typedef struct xmpp_profile_s xmpp_profile_t;

struct rtc_globals_s {
	switch_mutex_t *mutex;
	switch_memory_pool_t *pool;

	int profile_count;
	xmpp_profile_t *profile_head;
	int sig;
	int running;

	switch_hash_t *event_channel_hash;
	switch_thread_rwlock_t *event_channel_rwlock;

	int debug;
	int ready;
	int profile_threads;
	int enable_presence;

	xmpp_pvt_t *tech_head;
	switch_thread_rwlock_t *tech_rwlock;

	switch_thread_cond_t *detach_cond;
	switch_mutex_t *detach_mutex;
	switch_mutex_t *detach2_mutex;

	uint32_t detached;
	uint32_t detach_timeout;

	switch_event_channel_id_t event_channel_id;
};

#define ENDPOINT_NAME "xmpp"

static struct rtc_globals_s xmpp_globals;

static xmpp_profile_t *find_profile(const char *name);

typedef struct jsock_sub_node_s {
	jsock_t *jsock;
	uint32_t serno;
	struct jsock_sub_node_head_s *head;
	struct jsock_sub_node_s *next;
} jsock_sub_node_t;

typedef struct jsock_sub_node_head_s {
	jsock_sub_node_t *node;
	jsock_sub_node_t *tail;
	char *event_channel;
} jsock_sub_node_head_t;

static uint32_t jsock_unsub_head(jsock_t *jsock, jsock_sub_node_head_t *head)
{
	uint32_t x = 0;

	jsock_sub_node_t *thisnp = NULL, *np, *last = NULL;

	np = head->tail = head->node;

	while (np) {
		thisnp = np;
		np = np->next;

		if (!jsock || thisnp->jsock == jsock) {
			x++;

			if (last) {
				last->next = np;
			} else {
				head->node = np;
			}

			if (thisnp->jsock->profile->debug || xmpp_globals.debug) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ALERT, "UNSUBBING %s [%s]\n", thisnp->jsock->name,
								  thisnp->head->event_channel);
			}

			thisnp->jsock = NULL;
			free(thisnp);
		} else {
			last = thisnp;
			head->tail = last;
		}
	}

	return x;
}

static void detach_calls(jsock_t *jsock);

static void unsub_all_jsock(void)
{
	switch_hash_index_t *hi;
	void *val;
	jsock_sub_node_head_t *head;

	switch_thread_rwlock_wrlock(xmpp_globals.event_channel_rwlock);
top:
	head = NULL;

	for (hi = switch_core_hash_first(xmpp_globals.event_channel_hash); hi;) {
		switch_core_hash_this(hi, NULL, NULL, &val);
		head = (jsock_sub_node_head_t *)val;
		jsock_unsub_head(NULL, head);
		switch_core_hash_delete(xmpp_globals.event_channel_hash, head->event_channel);
		free(head->event_channel);
		free(head);
		switch_safe_free(hi);
		goto top;
	}

	switch_thread_rwlock_unlock(xmpp_globals.event_channel_rwlock);
}

static uint32_t ID = 1;

static uint32_t next_id(void)
{
	uint32_t id;

	switch_mutex_lock(xmpp_globals.mutex);
	id = ID++;
	switch_mutex_unlock(xmpp_globals.mutex);

	return id;
}

static cJSON *jrpc_new(uint32_t id)
{
	cJSON *obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, "jsonrpc", cJSON_CreateString("2.0"));

	if (id) {
		cJSON_AddItemToObject(obj, "id", cJSON_CreateNumber(id));
	}

	return obj;
}

static cJSON *jrpc_new_req(const char *method, const char *call_id, cJSON **paramsP)
{
	cJSON *msg, *params = NULL;
	uint32_t id = next_id();

	msg = jrpc_new(id);

	if (paramsP && *paramsP) {
		params = *paramsP;
	}

	if (!params) {
		params = cJSON_CreateObject();
	}

	cJSON_AddItemToObject(msg, "method", cJSON_CreateString(method));
	cJSON_AddItemToObject(msg, "params", params);

	if (call_id) {
		cJSON_AddItemToObject(params, "callID", cJSON_CreateString(call_id));
	}

	if (paramsP) {
		*paramsP = params;
	}

	return msg;
}

jsock_t *jsock_new(switch_memory_pool_t *pool, const char *profile_name)
{
	jsock_t *jsock = malloc(sizeof(jsock_t));
	switch_assert(jsock);
	memset(jsock, 0, sizeof(jsock_t));

	jsock->profile = find_profile(profile_name);
	if (!jsock->profile) {
		free(jsock);
		return NULL;
	}

	jsock->pool = pool;
	switch_event_create(&jsock->params, SWITCH_EVENT_CHANNEL_DATA);
	switch_event_create(&jsock->vars, SWITCH_EVENT_CHANNEL_DATA);
	switch_event_create(&jsock->user_vars, SWITCH_EVENT_CHANNEL_DATA);
	switch_mutex_init(&jsock->write_mutex, SWITCH_MUTEX_NESTED, jsock->pool);
	switch_thread_rwlock_create(&jsock->rwlock, jsock->pool);

	return jsock;
}

static switch_status_t xmpp_publish_event(xmpp_pvt_t *tech_pvt, cJSON **obj)
{
	*obj = NULL;
	return SWITCH_STATUS_SUCCESS;
}

static void set_call_params(cJSON *params, xmpp_pvt_t *tech_pvt)
{
	const char *caller_id_name = NULL;
	const char *caller_id_number = NULL;
	const char *callee_id_name = NULL;
	const char *callee_id_number = NULL;
	const char *prefix = "xmpp_h_";
	switch_event_header_t *var = NULL;

	caller_id_name = switch_channel_get_variable(tech_pvt->channel, "caller_id_name");
	caller_id_number = switch_channel_get_variable(tech_pvt->channel, "caller_id_number");
	callee_id_name = switch_channel_get_variable(tech_pvt->channel, "callee_id_name");
	callee_id_number = switch_channel_get_variable(tech_pvt->channel, "callee_id_number");

	if (caller_id_name) cJSON_AddItemToObject(params, "caller_id_name", cJSON_CreateString(caller_id_name));
	if (caller_id_number) cJSON_AddItemToObject(params, "caller_id_number", cJSON_CreateString(caller_id_number));

	if (callee_id_name) cJSON_AddItemToObject(params, "callee_id_name", cJSON_CreateString(callee_id_name));
	if (callee_id_number) cJSON_AddItemToObject(params, "callee_id_number", cJSON_CreateString(callee_id_number));

	cJSON_AddItemToObject(
		params, "display_direction",
		cJSON_CreateString(switch_channel_direction(tech_pvt->channel) == SWITCH_CALL_DIRECTION_OUTBOUND ? "outbound"
																										 : "inbound"));

	for (var = switch_channel_variable_first(tech_pvt->channel); var; var = var->next) {
		const char *name = (char *)var->name;
		char *value = (char *)var->value;
		if (!strncasecmp(name, prefix, strlen(prefix))) {
			cJSON_AddItemToObject(params, name, cJSON_CreateString(value));
		}
	}
	switch_channel_variable_last(tech_pvt->channel);
}

static int attach_wake(void)
{
	switch_status_t status;
	int tries = 0;

top:

	status = switch_mutex_trylock(xmpp_globals.detach_mutex);

	if (status == SWITCH_STATUS_SUCCESS) {
		switch_thread_cond_signal(xmpp_globals.detach_cond);
		switch_mutex_unlock(xmpp_globals.detach_mutex);
		return 1;
	} else {
		if (switch_mutex_trylock(xmpp_globals.detach2_mutex) == SWITCH_STATUS_SUCCESS) {
			switch_mutex_unlock(xmpp_globals.detach2_mutex);
		} else {
			if (++tries < 10) {
				switch_cond_next();
				goto top;
			}
		}
	}

	return 0;
}

static void tech_reattach(xmpp_pvt_t *tech_pvt, jsock_t *jsock)
{
	cJSON *params = NULL;
	cJSON *msg = NULL;

	tech_pvt->detach_time = 0;
	xmpp_globals.detached--;
	attach_wake();
	switch_set_flag(tech_pvt, TFLAG_ATTACH_REQ);
	msg = jrpc_new_req("xmpp.attach", tech_pvt->call_id, &params);

	switch_channel_set_flag(tech_pvt->channel, CF_REINVITE);
	switch_channel_set_flag(tech_pvt->channel, CF_RECOVERING);
	switch_core_media_gen_local_sdp(tech_pvt->session, SDP_TYPE_REQUEST, NULL, 0, NULL, 0);
	switch_channel_clear_flag(tech_pvt->channel, CF_REINVITE);
	switch_channel_clear_flag(tech_pvt->channel, CF_RECOVERING);
	switch_core_session_request_video_refresh(tech_pvt->session);

	cJSON_AddItemToObject(params, "sdp", cJSON_CreateString(tech_pvt->mparams->local_sdp_str));
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(tech_pvt->session), SWITCH_LOG_DEBUG, "Local attach SDP %s:\n%s\n",
					  switch_channel_get_name(tech_pvt->channel), tech_pvt->mparams->local_sdp_str);
	set_call_params(params, tech_pvt);
	if (xmpp_publish_event(tech_pvt, &msg) != SWITCH_STATUS_SUCCESS) {
		switch_channel_hangup(tech_pvt->channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
	}
}

static void detach_calls(jsock_t *jsock)
{
	xmpp_pvt_t *tech_pvt;
	int wake = 0;

	switch_thread_rwlock_rdlock(xmpp_globals.tech_rwlock);
	for (tech_pvt = xmpp_globals.tech_head; tech_pvt; tech_pvt = tech_pvt->next) {
		if (!strcmp(tech_pvt->jsock->uuid_str, jsock->uuid_str)) {
			if (!switch_channel_up_nosig(tech_pvt->channel)) {
				continue;
			}

			if (!switch_channel_test_flag(tech_pvt->channel, CF_ANSWERED)) {
				switch_channel_hangup(tech_pvt->channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
				continue;
			}

			if (switch_channel_test_flag(tech_pvt->channel, CF_NO_RECOVER)) {
				switch_channel_hangup(tech_pvt->channel, SWITCH_CAUSE_NORMAL_CLEARING);
				continue;
			}

			switch_core_session_stop_media(tech_pvt->session);
			tech_pvt->detach_time = switch_epoch_time_now(NULL);
			xmpp_globals.detached++;
			wake = 1;
		}
	}
	switch_thread_rwlock_unlock(xmpp_globals.tech_rwlock);

	if (wake) attach_wake();
}

static void track_pvt(xmpp_pvt_t *tech_pvt)
{
	switch_thread_rwlock_wrlock(xmpp_globals.tech_rwlock);
	tech_pvt->next = xmpp_globals.tech_head;
	xmpp_globals.tech_head = tech_pvt;
	switch_set_flag(tech_pvt, TFLAG_TRACKED);
	switch_thread_rwlock_unlock(xmpp_globals.tech_rwlock);
}

static void untrack_pvt(xmpp_pvt_t *tech_pvt)
{
	xmpp_pvt_t *p, *last = NULL;
	int wake = 0;

	switch_thread_rwlock_wrlock(xmpp_globals.tech_rwlock);

	if (tech_pvt->detach_time) {
		xmpp_globals.detached--;
		tech_pvt->detach_time = 0;
		wake = 1;
	}

	if (switch_test_flag(tech_pvt, TFLAG_TRACKED)) {
		switch_clear_flag(tech_pvt, TFLAG_TRACKED);
		for (p = xmpp_globals.tech_head; p; p = p->next) {
			if (p == tech_pvt) {
				if (last) {
					last->next = p->next;
				} else {
					xmpp_globals.tech_head = p->next;
				}
				break;
			}

			last = p;
		}
	}

	switch_thread_rwlock_unlock(xmpp_globals.tech_rwlock);

	if (wake) attach_wake();
}

switch_endpoint_interface_t *xmpp_endpoint_interface = NULL;

static switch_status_t xmpp_on_destroy(switch_core_session_t *session)
{
	xmpp_pvt_t *tech_pvt = switch_core_session_get_private_class(session, SWITCH_PVT_SECONDARY);

	switch_safe_free(tech_pvt->jsock);

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t xmpp_on_hangup(switch_core_session_t *session)
{
	xmpp_pvt_t *tech_pvt = switch_core_session_get_private_class(session, SWITCH_PVT_SECONDARY);

	untrack_pvt(tech_pvt);
	leave_room(tech_pvt->client);

	// get the jsock and send hangup notice
	if (!tech_pvt->remote_hangup_cause) {
		cJSON *params = NULL;
		switch_call_cause_t cause = switch_channel_get_cause(tech_pvt->channel);
		switch_channel_set_variable(tech_pvt->channel, "xmpp_hangup_disposition", "send_bye");
		switch_channel_set_variable(tech_pvt->channel, "sip_hangup_disposition", "send_bye");

		cJSON_AddItemToObject(params, "causeCode", cJSON_CreateNumber(cause));
		cJSON_AddItemToObject(params, "cause", cJSON_CreateString(switch_channel_cause2str(cause)));
		tech_pvt->got_bye++;
	}

	return SWITCH_STATUS_SUCCESS;
}
switch_status_t xmpp_tech_media(xmpp_pvt_t *tech_pvt, const char *r_sdp, switch_sdp_type_t sdp_type);

static switch_status_t xmpp_set_media_options(xmpp_pvt_t *tech_pvt, xmpp_profile_t *profile);

static switch_status_t xmpp_connect(switch_core_session_t *session, const char *method)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	xmpp_pvt_t *tech_pvt = switch_core_session_get_private_class(session, SWITCH_PVT_SECONDARY);
	jsock_t *jsock = tech_pvt->jsock;

	cJSON *params = NULL;
	cJSON *msg = NULL;
	const char *var = NULL;
	switch_caller_profile_t *caller_profile = switch_channel_get_caller_profile(tech_pvt->channel);
	switch_event_header_t *hp;

	DUMP_EVENT(jsock->params);

	switch_channel_set_variable(tech_pvt->channel, "presence_id", jsock->uid);
	switch_channel_set_variable(tech_pvt->channel, "chat_proto", VERTO_CHAT_PROTO);

	for (hp = jsock->user_vars->headers; hp; hp = hp->next) {
		switch_channel_set_variable(tech_pvt->channel, hp->name, hp->value);
	}

	if ((var = switch_event_get_header(jsock->params, "caller-id-name"))) {
		caller_profile->callee_id_name = switch_core_strdup(caller_profile->pool, var);
	}

	if ((var = switch_event_get_header(jsock->params, "caller-id-number"))) {
		caller_profile->callee_id_number = switch_core_strdup(caller_profile->pool, var);
	}

	if (switch_channel_test_flag(tech_pvt->channel, CF_PROXY_MODE)) {
		switch_core_media_absorb_sdp(session);
	} else {
		switch_channel_set_variable(tech_pvt->channel, "media_webrtc", "true");
		switch_core_session_set_ice(tech_pvt->session);

		if (xmpp_set_media_options(tech_pvt, jsock->profile) != SWITCH_STATUS_SUCCESS) {
			status = SWITCH_STATUS_FALSE;
			switch_thread_rwlock_unlock(jsock->rwlock);
			return status;
		}

		if (!switch_channel_test_flag(tech_pvt->channel, CF_RECOVERING)) {
			switch_channel_set_variable(tech_pvt->channel, "codec_string", NULL);
			switch_core_media_prepare_codecs(tech_pvt->session, SWITCH_TRUE);

			if ((status = switch_core_media_choose_ports(tech_pvt->session, SWITCH_TRUE, SWITCH_TRUE)) !=
				SWITCH_STATUS_SUCCESS) {
				switch_thread_rwlock_unlock(jsock->rwlock);
				return status;
			}
		}

		switch_core_media_gen_local_sdp(session, SDP_TYPE_REQUEST, NULL, 0, NULL, 0);
	}

	msg = jrpc_new_req(method, tech_pvt->call_id, &params);

	if (tech_pvt->mparams->local_sdp_str) {
		char *new_sdp = tech_pvt->mparams->local_sdp_str;

		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "Local %s SDP %s:\n%s\n", method,
						  switch_channel_get_name(tech_pvt->channel), tech_pvt->mparams->local_sdp_str);

		if (!zstr(tech_pvt->url)) {	 // publish whip/whap
			switch_curl_slist_t *headers = NULL;
			int curl_connect_timeout = 3;
			int curl_timeout = 10;
			http_data_t *response = NULL;

			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO, "url=%s\n", tech_pvt->url);

			// sdp add BUNDLE
			if (!strstr(tech_pvt->mparams->local_sdp_str, "a=group:BUNDLE")) {
				char *p;
				char *sdp = switch_core_session_strdup(session, tech_pvt->mparams->local_sdp_str);

				if ((p = strstr(sdp, "m=audio "))) {
					*p++ = 0;
					new_sdp = switch_core_sprintf(switch_core_session_get_pool(session),
												  "%s"
												  "a=group:BUNDLE 0 1\r\nm%s",
												  sdp, p);
				}
				if ((p = strstr(new_sdp, "m=video "))) {
					*p++ = 0;
					new_sdp = switch_core_sprintf(switch_core_session_get_pool(session),
												  "%s"
												  "a=mid:0\r\nm%s",
												  new_sdp, p);
				}

				new_sdp = switch_core_sprintf(switch_core_session_get_pool(session),
											  "%s"
											  "a=mid:1\r\n",
											  new_sdp);

				switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "new:\n%s\n", new_sdp);
			}

			// HTTP 请求 Whip or  Whep
			//headers = switch_curl_slist_append(headers, "Content-Type: application/sdp");
			set_switch_answer(tech_pvt->client, new_sdp);
			doconnect(tech_pvt->client);
			char offer[1024 * 5];
			memset(offer,0, 1024 * 5);
			int offer_size = get_xmpp_offer(tech_pvt->client, offer, 1024 * 5);
			if (offer_size > 0) {
				switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "res=%s\n", (char *)offer);
				status = handle_xmpp_answer(session, (const char *)offer);
			}

			/*response = switch_http_request(SWITCH_HTTP_CM_POST, tech_pvt->url, new_sdp, strlen(new_sdp), headers,
										   mod_xmpp_globals.pool, curl_connect_timeout, curl_timeout);

			if (response) {
				switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "code=%ld\n", response->code);
				if (response->body_buffer) {
					const void *data = NULL;
					switch_buffer_peek_zerocopy(response->body_buffer, &data);
					switch_assert(data);
					switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "res=%s\n", (char *)data);
					status = handle_xmpp_answer(session, (const char *)data);
					switch_buffer_destroy(&response->body_buffer);
				}
				if (response->headers) switch_curl_slist_free_all(response->headers);
			} else {
				switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_WARNING, "NO Response\n");
				status = SWITCH_STATUS_FALSE;
			}*/
		} else {
			cJSON_AddItemToObject(params, "sdp", cJSON_CreateString(tech_pvt->mparams->local_sdp_str));
			set_call_params(params, tech_pvt);
			cJSON_AddStringToObject(params, "destination_number", tech_pvt->destination_number);  // parsed destination
			add_current_node(params);
			char subject[1024];
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "publish subject:%s\n", subject);
			cJSON_Delete(msg);
		}
	} else {
		status = SWITCH_STATUS_FALSE;
	}

	return status;
}

switch_status_t xmpp_tech_media(xmpp_pvt_t *tech_pvt, const char *r_sdp, switch_sdp_type_t sdp_type)
{
	uint8_t p = 0;

	switch_assert(tech_pvt != NULL);
	switch_assert(r_sdp != NULL);

	if (zstr(r_sdp)) {
		return SWITCH_STATUS_FALSE;
	}

	if (switch_core_media_negotiate_sdp(tech_pvt->session, r_sdp, &p, sdp_type)) {
		if (switch_core_media_choose_ports(tech_pvt->session, SWITCH_TRUE, SWITCH_FALSE) != SWITCH_STATUS_SUCCESS) {
			return SWITCH_STATUS_FALSE;
		}

		if (switch_core_media_activate_rtp(tech_pvt->session) != SWITCH_STATUS_SUCCESS) {
			return SWITCH_STATUS_FALSE;
		}

		return SWITCH_STATUS_SUCCESS;
	}

	return SWITCH_STATUS_FALSE;
}

static switch_status_t xmpp_on_init(switch_core_session_t *session)
{
	switch_status_t status = SWITCH_STATUS_SUCCESS;
	xmpp_pvt_t *tech_pvt = switch_core_session_get_private_class(session, SWITCH_PVT_SECONDARY);

	if (switch_channel_test_flag(tech_pvt->channel, CF_RECOVERING_BRIDGE) ||
		switch_channel_test_flag(tech_pvt->channel, CF_RECOVERING)) {
		int tries = 120;

		switch_core_session_clear_crypto(session);

		while (--tries > 0) {
			status = xmpp_connect(session, "xmpp.attach");

			if (status == SWITCH_STATUS_SUCCESS) {
				switch_set_flag(tech_pvt, TFLAG_ATTACH_REQ);
				break;
			} else if (status == SWITCH_STATUS_BREAK) {
				switch_yield(1000000);
				continue;
			} else {
				tries = 0;
				break;
			}
		}

		if (!tries) {
			switch_channel_hangup(tech_pvt->channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
			status = SWITCH_STATUS_FALSE;
		}

		switch_channel_set_flag(tech_pvt->channel, CF_VIDEO_BREAK);
		switch_core_session_kill_channel(tech_pvt->session, SWITCH_SIG_BREAK);

		tries = 500;
		while (--tries > 0 && switch_test_flag(tech_pvt, TFLAG_ATTACH_REQ)) {
			switch_yield(10000);
		}

		switch_core_session_request_video_refresh(session);
		switch_channel_set_flag(tech_pvt->channel, CF_VIDEO_BREAK);
		switch_core_session_kill_channel(tech_pvt->session, SWITCH_SIG_BREAK);

		goto end;
	}

	if (switch_channel_direction(tech_pvt->channel) == SWITCH_CALL_DIRECTION_OUTBOUND) {
		tech_pvt->client = join_room("12345678", "");
		if ((status = xmpp_connect(tech_pvt->session, "xmpp.invite")) != SWITCH_STATUS_SUCCESS) {
			switch_channel_hangup(tech_pvt->channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
		} else {
			switch_channel_mark_ring_ready(tech_pvt->channel);
		}
	}

end:

	if (status == SWITCH_STATUS_SUCCESS) {
		track_pvt(tech_pvt);
	}

	return status;
}

static switch_state_handler_table_t switch_state_handler = {
	/*.on_init */ xmpp_on_init,
	/*.on_routing */ NULL,
	/*.on_execute */ NULL,
	/*.on_hangup */ xmpp_on_hangup,
	/*.on_exchange_media */ NULL,
	/*.on_soft_execute */ NULL,
	/*.on_consume_media */ NULL,
	/*.on_hibernate */ NULL,
	/*.on_reset */ NULL,
	/*.on_park */ NULL,
	/*.on_reporting */ NULL,
	/*.on_destroy */ xmpp_on_destroy,
	SSH_FLAG_STICKY};

static switch_status_t xmpp_set_media_options(xmpp_pvt_t *tech_pvt, xmpp_profile_t *profile)
{
	uint32_t i;

	switch_mutex_lock(profile->mutex);
	if (!zstr(profile->rtpip[profile->rtpip_cur])) {
		tech_pvt->mparams->rtpip4 = switch_core_session_strdup(tech_pvt->session, profile->rtpip[profile->rtpip_cur++]);
		tech_pvt->mparams->rtpip = tech_pvt->mparams->rtpip4;
		if (profile->rtpip_cur == profile->rtpip_index) {
			profile->rtpip_cur = 0;
		}
	}

	if (!zstr(profile->rtpip6[profile->rtpip_cur6])) {
		tech_pvt->mparams->rtpip6 = switch_core_session_strdup(tech_pvt->session, profile->rtpip6[profile->rtpip_cur6++]);

		if (zstr(tech_pvt->mparams->rtpip)) {
			tech_pvt->mparams->rtpip = tech_pvt->mparams->rtpip6;
		}

		if (profile->rtpip_cur6 == profile->rtpip_index6) {
			profile->rtpip_cur6 = 0;
		}
	}
	switch_mutex_unlock(profile->mutex);

	if (zstr(tech_pvt->mparams->rtpip)) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(tech_pvt->session), SWITCH_LOG_ERROR,
						  "%s has no media ip, check your configuration\n", switch_channel_get_name(tech_pvt->channel));
		return SWITCH_STATUS_FALSE;
	}

	tech_pvt->mparams->extrtpip = tech_pvt->mparams->extsipip = profile->extrtpip;

	switch_channel_set_flag(tech_pvt->channel, CF_TRACKABLE);
	switch_channel_set_variable(tech_pvt->channel, "secondary_recovery_module", modname);

	switch_core_media_check_dtmf_type(tech_pvt->session);

	switch_channel_set_cap(tech_pvt->channel, CC_BYPASS_MEDIA);
	switch_channel_set_cap(tech_pvt->channel, CC_JITTERBUFFER);
	switch_channel_set_cap(tech_pvt->channel, CC_FS_RTP);

	tech_pvt->mparams->inbound_codec_string = switch_core_session_strdup(tech_pvt->session, profile->inbound_codec_string);
	tech_pvt->mparams->outbound_codec_string = switch_core_session_strdup(tech_pvt->session, profile->outbound_codec_string);

	tech_pvt->mparams->jb_msec = "1p:50p";
	switch_media_handle_set_media_flag(tech_pvt->smh, SCMF_SUPPRESS_CNG);

	tech_pvt->mparams->timer_name = profile->timer_name;
	tech_pvt->mparams->local_network = switch_core_session_strdup(tech_pvt->session, profile->local_network);

	for (i = 0; i < profile->cand_acl_count; i++) {
		switch_core_media_add_ice_acl(tech_pvt->session, SWITCH_MEDIA_TYPE_AUDIO, profile->cand_acl[i]);
		switch_core_media_add_ice_acl(tech_pvt->session, SWITCH_MEDIA_TYPE_VIDEO, profile->cand_acl[i]);
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t xmpp_media(switch_core_session_t *session)
{
	xmpp_pvt_t *tech_pvt = switch_core_session_get_private_class(session, SWITCH_PVT_SECONDARY);
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	switch_core_media_prepare_codecs(tech_pvt->session, SWITCH_TRUE);

	if (tech_pvt->r_sdp) {
		if (xmpp_tech_media(tech_pvt, tech_pvt->r_sdp, SDP_TYPE_REQUEST) != SWITCH_STATUS_SUCCESS) {
			switch_channel_set_variable(tech_pvt->channel, SWITCH_ENDPOINT_DISPOSITION_VARIABLE, "CODEC NEGOTIATION ERROR");
			return SWITCH_STATUS_FALSE;
		}
	}

	if ((status = switch_core_media_choose_ports(tech_pvt->session, SWITCH_TRUE, SWITCH_FALSE)) != SWITCH_STATUS_SUCCESS) {
		switch_channel_hangup(tech_pvt->channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
		return status;
	}

	switch_core_media_gen_local_sdp(session, SDP_TYPE_RESPONSE, NULL, 0, NULL, 0);

	if (switch_core_media_activate_rtp(tech_pvt->session) != SWITCH_STATUS_SUCCESS) {
		switch_channel_hangup(tech_pvt->channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
	}

	if (tech_pvt->mparams->local_sdp_str) {
		switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "Local SDP %s:\n%s\n",
						  switch_channel_get_name(tech_pvt->channel), tech_pvt->mparams->local_sdp_str);
	} else {
		status = SWITCH_STATUS_FALSE;
	}

	return status;
}

static switch_status_t xmpp_send_media_indication(switch_core_session_t *session, const char *method)
{
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "xmpp_send_media_indication %s \n", method);
	switch_status_t status = SWITCH_STATUS_FALSE;
	xmpp_pvt_t *tech_pvt = switch_core_session_get_private_class(session, SWITCH_PVT_SECONDARY);
	const char *proxy_sdp = NULL;

	if (switch_test_flag(tech_pvt, TFLAG_SENT_MEDIA)) {
		status = SWITCH_STATUS_SUCCESS;
	}

	if (switch_channel_test_flag(tech_pvt->channel, CF_PROXY_MODE)) {
		if ((proxy_sdp = switch_channel_get_variable(tech_pvt->channel, SWITCH_B_SDP_VARIABLE))) {
			status = SWITCH_STATUS_SUCCESS;
			switch_core_media_set_local_sdp(session, proxy_sdp, SWITCH_TRUE);
		}
	}

	if (status == SWITCH_STATUS_SUCCESS || (status = xmpp_media(session)) == SWITCH_STATUS_SUCCESS) {
		cJSON *params = NULL;
		cJSON *msg = jrpc_new_req(method, tech_pvt->call_id, &params);
		if (!switch_test_flag(tech_pvt, TFLAG_SENT_MEDIA)) {
			cJSON_AddItemToObject(params, "sdp", cJSON_CreateString(tech_pvt->mparams->local_sdp_str));
		}

		switch_set_flag(tech_pvt, TFLAG_SENT_MEDIA);

		if (xmpp_publish_event(tech_pvt, &msg) != SWITCH_STATUS_SUCCESS) {
			switch_channel_hangup(tech_pvt->channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
		}
	}

	return status;
}

static switch_status_t xmpp_send_media_update(switch_core_session_t *session, const char *method)
{
	switch_status_t status = SWITCH_STATUS_FALSE;
	xmpp_pvt_t *tech_pvt = switch_core_session_get_private_class(session, SWITCH_PVT_SECONDARY);
	if (switch_true(switch_channel_get_private(tech_pvt->channel, "_xmpp_updateMedia_"))) {
		cJSON *params = NULL;
		cJSON *msg = jrpc_new_req(method, tech_pvt->call_id, &params);
		switch_core_media_gen_local_sdp(session, SDP_TYPE_RESPONSE, NULL, 0, NULL, 0);
		cJSON_AddItemToObject(params, "sdp", cJSON_CreateString(tech_pvt->mparams->local_sdp_str));
		cJSON_AddStringToObject(params, "action", "updateMedia");
		if (tech_pvt->mparams->local_sdp_str) {
			switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "updateMedia: Local SDP %s:\n%s\n",
							  switch_channel_get_name(tech_pvt->channel), tech_pvt->mparams->local_sdp_str);
		}
		if (xmpp_publish_event(tech_pvt, &msg) != SWITCH_STATUS_SUCCESS) {
			switch_channel_hangup(tech_pvt->channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
		}
	}
	switch_channel_set_private(tech_pvt->channel, "_xmpp_updateMedia_", NULL);

	return status;
}

static switch_status_t xmpp_message_hook(switch_core_session_t *session, switch_core_session_message_t *msg)
{
	switch_status_t r = SWITCH_STATUS_SUCCESS;
	xmpp_pvt_t *tech_pvt = switch_core_session_get_private_class(session, SWITCH_PVT_SECONDARY);
	//switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG,
	//				  " ------------------------------- %d ------------\n", msg->message_id);
	switch (msg->message_id) {
		case SWITCH_MESSAGE_INDICATE_DISPLAY: {
			const char *name, *number;
			cJSON *params = NULL;
			jsock_t *jsock = NULL;

			if ((jsock = tech_pvt->jsock)) {
				name = msg->string_array_arg[0];
				number = msg->string_array_arg[1];

				if (name || number) {
					switch_ivr_eavesdrop_update_display(session, name, number);
					switch_channel_set_variable(tech_pvt->channel, "last_sent_display_name", name);
					switch_channel_set_variable(tech_pvt->channel, "last_sent_display_number", number);
					cJSON_AddItemToObject(params, "display_name", cJSON_CreateString(name));
					cJSON_AddItemToObject(params, "display_number", cJSON_CreateString(number));
					set_call_params(params, tech_pvt);
				}

				switch_thread_rwlock_unlock(jsock->rwlock);
			}

		} break;
		case SWITCH_MESSAGE_INDICATE_MEDIA_RENEG: {
			jsock_t *jsock = NULL;

			if (switch_channel_test_flag(tech_pvt->channel, CF_ANSWERED)) {
				// switch_channel_set_flag(tech_pvt->channel, CF_AWAITING_REINVITE);
			}

			if ((jsock = tech_pvt->jsock)) {
				switch_core_session_stop_media(session);
				detach_calls(jsock);
				tech_reattach(tech_pvt, jsock);
				switch_thread_rwlock_unlock(jsock->rwlock);
			}
		} break;
		case SWITCH_MESSAGE_INDICATE_ANSWER:
			r = xmpp_send_media_indication(session, "xmpp.answer");
			break;
		case SWITCH_MESSAGE_INDICATE_BRIDGE:
			r = xmpp_send_media_indication(session, "xmpp.bridge");
			break;
		case SWITCH_MESSAGE_INDICATE_PROGRESS:
			r = xmpp_send_media_indication(session, "xmpp.media");
			break;
		case SWITCH_MESSAGE_INDICATE_RESPOND:
			r = xmpp_send_media_update(session, "xmpp.modify");
			break;
		case SWITCH_MESSAGE_INDICATE_HOLD: {
			if (msg->numeric_arg) {
				switch_core_media_toggle_hold(session, 1);
			} else {
				jsock_t *jsock = NULL;
				switch_channel_t *channel = switch_core_session_get_channel(session);
				if ((jsock = tech_pvt->jsock)) {
					switch_channel_set_flag(channel, CF_LEG_HOLDING);
					if (switch_channel_test_flag(tech_pvt->channel, CF_ANSWERED)) {
					}
					switch_core_media_set_smode(session, SWITCH_MEDIA_TYPE_AUDIO, SWITCH_MEDIA_FLOW_SENDONLY, SDP_TYPE_REQUEST);
					switch_core_session_stop_media(session);
					detach_calls(jsock);
					tech_reattach(tech_pvt, jsock);
					switch_thread_rwlock_unlock(jsock->rwlock);
				}
			}
		} break;

		case SWITCH_MESSAGE_INDICATE_UNHOLD: {
			jsock_t *jsock = NULL;
			switch_channel_t *channel = switch_core_session_get_channel(session);
			if ((jsock = tech_pvt->jsock)) {
				switch_channel_clear_flag(channel, CF_LEG_HOLDING);
				if (switch_channel_test_flag(tech_pvt->channel, CF_ANSWERED)) {
				}
				switch_core_media_set_smode(session, SWITCH_MEDIA_TYPE_AUDIO, SWITCH_MEDIA_FLOW_SENDRECV, SDP_TYPE_REQUEST);
				switch_core_session_stop_media(session);
				detach_calls(jsock);
				tech_reattach(tech_pvt, jsock);
				switch_thread_rwlock_unlock(jsock->rwlock);
			}
		} break;
		default:
			break;
	}

	return r;
}

static void pass_sdp(xmpp_pvt_t *tech_pvt)
{
	switch_core_session_t *other_session = NULL;
	switch_channel_t *other_channel = NULL;

	if (switch_core_session_get_partner(tech_pvt->session, &other_session) == SWITCH_STATUS_SUCCESS) {
		other_channel = switch_core_session_get_channel(other_session);
		switch_channel_pass_sdp(tech_pvt->channel, other_channel, tech_pvt->r_sdp);

		switch_channel_set_flag(other_channel, CF_PROXY_MODE);
		switch_core_session_queue_indication(other_session, SWITCH_MESSAGE_INDICATE_ANSWER);
		switch_core_session_rwunlock(other_session);
	}
}

//// METHODS

#define switch_either(_A, _B) zstr(_A) ? _B : _A
char *xmpp_version() { return ""; }

switch_status_t handle_xmpp_answer(switch_core_session_t *session, const char *sdp)
{
	int err = 0;
	switch_status_t status = SWITCH_STATUS_FALSE;
	const char *callee_id_name = NULL, *callee_id_number = NULL;
	xmpp_pvt_t *tech_pvt = switch_core_session_get_private_class(session, SWITCH_PVT_SECONDARY);
	switch_core_session_t *other_session = NULL;
	jsock_t *jsock = tech_pvt->jsock;

	if (zstr(sdp)) return status;

	if (1) {  // del audio candidate
		int a_cand = 0;
		char *p, *tmp, *audio = NULL, *video = NULL;
		char *new = switch_core_session_strdup(session, sdp);

		audio = strstr(new, "m=audio ");
		video = strstr(new, "m=video ");
		if (audio && video) {
			*audio++ = 0;
			*video++ = 0;

			tmp = audio;
			while ((p = strstr(tmp, "a=candidate"))) {
				*p++ = 0;
				tmp = p;
				a_cand++;
			}

			if (a_cand && (p = strstr(tmp, "\r\n"))) {
				*p = 0;
				p += 2;

				new = switch_core_sprintf(switch_core_session_get_pool(session), "%sm%s%sm%s", new, audio, p, video);

				sdp = new;
			}
		}
	}

	tech_pvt->r_sdp = switch_core_session_strdup(session, sdp);
	switch_channel_set_variable(tech_pvt->channel, SWITCH_R_SDP_VARIABLE, sdp);
	switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_DEBUG, "Remote SDP %s:\n%s\n",
					  switch_channel_get_name(tech_pvt->channel), sdp);
	switch_core_media_set_sdp_codec_string(session, sdp, SDP_TYPE_RESPONSE);

	//switch_ivr_set_user(session, jsock->uid);

	if (switch_core_session_get_partner(tech_pvt->session, &other_session) == SWITCH_STATUS_SUCCESS) {
		switch_channel_t *other_channel = switch_core_session_get_channel(other_session);
		switch_channel_set_variable(other_channel, SWITCH_B_SDP_VARIABLE, sdp);
		switch_core_session_rwunlock(other_session);
	}

	if (switch_channel_test_flag(tech_pvt->channel, CF_PROXY_MODE)) {
		pass_sdp(tech_pvt);
	} else {
		if (xmpp_tech_media(tech_pvt, tech_pvt->r_sdp, SDP_TYPE_RESPONSE) != SWITCH_STATUS_SUCCESS) {
			switch_channel_set_variable(tech_pvt->channel, SWITCH_ENDPOINT_DISPOSITION_VARIABLE, "CODEC NEGOTIATION ERROR");
			err = 1;
		}

		if (!err && switch_core_media_activate_rtp(tech_pvt->session) != SWITCH_STATUS_SUCCESS) {
			switch_channel_hangup(tech_pvt->channel, SWITCH_CAUSE_DESTINATION_OUT_OF_ORDER);
			err = 1;
		}
	}

	if (!err) {
		if (callee_id_name) {
			switch_channel_set_profile_var(tech_pvt->channel, "callee_id_name", callee_id_name);
		}
		if (callee_id_number) {
			switch_channel_set_profile_var(tech_pvt->channel, "callee_id_number", callee_id_number);
		}
		switch_channel_mark_answered(tech_pvt->channel);

		status = SWITCH_STATUS_SUCCESS;
	}

	return status;
}

static void kill_profile(xmpp_profile_t *profile)
{
	profile->running = 0;

	if (switch_thread_rwlock_tryrdlock(profile->rwlock) != SWITCH_STATUS_SUCCESS) {
		return;
	}

	switch_thread_rwlock_unlock(profile->rwlock);
}

static void kill_profiles(void)
{
	xmpp_profile_t *pp;
	int sanity = 50;

	switch_mutex_lock(xmpp_globals.mutex);
	for (pp = xmpp_globals.profile_head; pp; pp = pp->next) {
		kill_profile(pp);
	}
	switch_mutex_unlock(xmpp_globals.mutex);

	while (--sanity > 0 && xmpp_globals.profile_threads > 0) {
		switch_yield(100000);
	}
}

static void do_shutdown(void)
{
	xmpp_globals.running = 0;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Shutting down (SIG %d)\n", xmpp_globals.sig);

	kill_profiles();

	unsub_all_jsock();

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Done\n");
}

static void parse_ip(char *host, switch_size_t host_len, uint16_t *port, char *input)
{
	char *p;

	if ((p = strchr(input, '['))) {
		char *end = switch_find_end_paren(p, '[', ']');
		if (end) {
			p++;
			strncpy(host, p, end - p);
			if (*(end + 1) == ':' && end + 2 < end_of_p(input)) {
				end += 2;
				if (end) {
					*port = (uint16_t)atoi(end);
				}
			}
		} else {
			strncpy(host, "::", host_len);
		}
	} else {
		strncpy(host, input, host_len);
		if ((p = strrchr(host, ':')) != NULL) {
			*p++ = '\0';
			*port = (uint16_t)atoi(p);
		}
	}
}

static xmpp_profile_t *find_profile(const char *name)
{
	xmpp_profile_t *p, *r = NULL;
	switch_mutex_lock(xmpp_globals.mutex);
	for (p = xmpp_globals.profile_head; p; p = p->next) {
		if (!strcmp(name, p->name)) {
			r = p;
			break;
		}
	}

	if (r && (!r->in_thread || !r->running)) {
		r = NULL;
	}

	if (r && switch_thread_rwlock_tryrdlock(r->rwlock) != SWITCH_STATUS_SUCCESS) {
		r = NULL;
	}
	switch_mutex_unlock(xmpp_globals.mutex);

	return r;
}

static switch_bool_t profile_exists(const char *name)
{
	switch_bool_t r = SWITCH_FALSE;
	xmpp_profile_t *p;

	switch_mutex_lock(xmpp_globals.mutex);
	for (p = xmpp_globals.profile_head; p; p = p->next) {
		if (!strcmp(p->name, name)) {
			r = SWITCH_TRUE;
			break;
		}
	}
	switch_mutex_unlock(xmpp_globals.mutex);

	return r;
}

static void del_profile(xmpp_profile_t *profile)
{
	xmpp_profile_t *p, *last = NULL;

	switch_mutex_lock(xmpp_globals.mutex);
	for (p = xmpp_globals.profile_head; p; p = p->next) {
		if (p == profile) {
			if (last) {
				last->next = p->next;
			} else {
				xmpp_globals.profile_head = p->next;
			}
			xmpp_globals.profile_count--;
			break;
		}

		last = p;
	}
	switch_mutex_unlock(xmpp_globals.mutex);
}

static switch_status_t add_profile(xmpp_profile_t *profile)
{
	switch_status_t status = SWITCH_STATUS_FALSE;

	switch_mutex_lock(xmpp_globals.mutex);

	if (!profile_exists(profile->name)) {
		status = SWITCH_STATUS_SUCCESS;
	}

	if (status == SWITCH_STATUS_SUCCESS) {
		profile->next = xmpp_globals.profile_head;
		xmpp_globals.profile_head = profile;
		xmpp_globals.profile_count++;
	}

	switch_mutex_unlock(xmpp_globals.mutex);

	return status;
}

static switch_status_t parse_config(const char *cf)
{
	switch_xml_t cfg, xml, settings, param, xprofile, xprofiles;
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	if (!(xml = switch_xml_open_cfg(cf, &cfg, NULL))) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Open of %s failed\n", cf);
		return SWITCH_STATUS_TERM;
	}

	if ((xprofiles = switch_xml_child(cfg, "profiles"))) {
		for (xprofile = switch_xml_child(xprofiles, "profile"); xprofile; xprofile = xprofile->next) {
			xmpp_profile_t *profile;
			switch_memory_pool_t *pool;
			const char *name = switch_xml_attr(xprofile, "name");

			if (zstr(name)) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Required field name missing\n");
				continue;
			}

			if (profile_exists(name)) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Profile %s already exists\n", name);
				continue;
			}

			switch_core_new_memory_pool(&pool);
			profile = switch_core_alloc(pool, sizeof(*profile));
			profile->pool = pool;
			profile->name = switch_core_strdup(profile->pool, name);
			switch_mutex_init(&profile->mutex, SWITCH_MUTEX_NESTED, profile->pool);
			switch_thread_rwlock_create(&profile->rwlock, profile->pool);
			add_profile(profile);

			profile->local_network = "localnet.auto";

			for (param = switch_xml_child(xprofile, "param"); param; param = param->next) {
				char *var = NULL;
				char *val = NULL;
				int i = 0;

				var = (char *)switch_xml_attr_soft(param, "name");
				val = (char *)switch_xml_attr_soft(param, "value");

				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s = %s\n", var, val);

				if (!strcasecmp(var, "bind-local")) {
					const char *secure = switch_xml_attr_soft(param, "secure");
					if (i < MAX_BIND) {
						parse_ip(profile->ip[profile->i].local_ip, sizeof(profile->ip[profile->i].local_ip),
								 &profile->ip[profile->i].local_port, val);
						if (switch_true(secure)) {
							profile->ip[profile->i].secure = 1;
						}
						profile->i++;
					} else {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Max Bindings Reached!\n");
					}
				} else if (!strcasecmp(var, "enable-text")) {
					profile->enable_text = 1;
				} else if (!strcasecmp(var, "secure-combined")) {
					set_string(profile->cert, val);
					set_string(profile->key, val);
					switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "Secure key and cert specified\n");
				} else if (!strcasecmp(var, "secure-cert")) {
					set_string(profile->cert, val);
				} else if (!strcasecmp(var, "secure-key")) {
					set_string(profile->key, val);
				} else if (!strcasecmp(var, "secure-chain")) {
					set_string(profile->chain, val);
				} else if (!strcasecmp(var, "inbound-codec-string") && !zstr(val)) {
					profile->inbound_codec_string = switch_core_strdup(profile->pool, val);
				} else if (!strcasecmp(var, "outbound-codec-string") && !zstr(val)) {
					profile->outbound_codec_string = switch_core_strdup(profile->pool, val);
				} else if (!strcasecmp(var, "blind-reg") && !zstr(val)) {
					profile->blind_reg = switch_true(val);
				} else if (!strcasecmp(var, "userauth") && !zstr(val)) {
					profile->userauth = switch_core_strdup(profile->pool, val);
				} else if (!strcasecmp(var, "root-password") && !zstr(val)) {
					profile->root_passwd = switch_core_strdup(profile->pool, val);
				} else if (!strcasecmp(var, "context") && !zstr(val)) {
					profile->context = switch_core_strdup(profile->pool, val);
				} else if (!strcasecmp(var, "dialplan") && !zstr(val)) {
					profile->dialplan = switch_core_strdup(profile->pool, val);
				} else if (!strcasecmp(var, "mcast-ip") && val) {
					profile->mcast_ip = switch_core_strdup(profile->pool, val);
				} else if (!strcasecmp(var, "mcast-port") && val) {
					profile->mcast_port = (switch_port_t)atoi(val);
				} else if (!strcasecmp(var, "timer-name") && !zstr(var)) {
					profile->timer_name = switch_core_strdup(profile->pool, val);
				} else if (!strcasecmp(var, "force-register-domain") && !zstr(val)) {
					profile->register_domain = switch_core_strdup(profile->pool, val);
				} else if (!strcasecmp(var, "local-network") && !zstr(val)) {
					profile->local_network = switch_core_strdup(profile->pool, val);
				} else if (!strcasecmp(var, "apply-candidate-acl")) {
					if (profile->cand_acl_count < SWITCH_MAX_CAND_ACL) {
						profile->cand_acl[profile->cand_acl_count++] = switch_core_strdup(profile->pool, val);
					} else {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Max acl records of %d reached\n",
										  SWITCH_MAX_CAND_ACL);
					}
				} else if (!strcasecmp(var, "apply-connection-acl")) {
					if (profile->conn_acl_count < SWITCH_MAX_CAND_ACL) {
						profile->conn_acl[profile->conn_acl_count++] = switch_core_strdup(profile->pool, val);
					} else {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Max acl records of %d reached\n",
										  SWITCH_MAX_CAND_ACL);
					}
				} else if (!strcasecmp(var, "rtp-ip")) {
					if (zstr(val)) {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Invalid RTP IP.\n");
					} else {
						if (strchr(val, ':')) {
							if (profile->rtpip_index6 < MAX_RTPIP - 1) {
								profile->rtpip6[profile->rtpip_index6++] = switch_core_strdup(profile->pool, val);
							} else {
								switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Too many RTP IP.\n");
							}
						} else {
							if (profile->rtpip_index < MAX_RTPIP - 1) {
								profile->rtpip[profile->rtpip_index++] = switch_core_strdup(profile->pool, val);
							} else {
								switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Too many RTP IP.\n");
							}
						}
					}
				} else if (!strcasecmp(var, "ext-rtp-ip")) {
					if (zstr(val)) {
						switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Invalid External RTP IP.\n");
					} else {
						switch_stun_ip_lookup(&profile->extrtpip, val, profile->pool);
					}
				} else if (!strcasecmp(var, "debug")) {
					if (val) {
						profile->debug = atoi(val);
					}
				} else if (!strcasecmp(var, "media-timeout")) {
					if (!zstr(val)) {
						profile->media_timeout = switch_core_strdup(profile->pool, val);
					}
				} else if (!strcasecmp(var, "xmpp-ctrl-subject")) {
					if (!zstr(val)) {
						profile->xmpp_ctrl_subject = switch_core_strdup(profile->pool, val);
					}
				}
			}

			if (zstr(profile->outbound_codec_string)) {
				profile->outbound_codec_string = "opus,vp8";
			}

			if (zstr(profile->inbound_codec_string)) {
				profile->inbound_codec_string = profile->outbound_codec_string;
			}

			if (zstr(profile->timer_name)) {
				profile->timer_name = "soft";
			}

			if (zstr(profile->dialplan)) {
				profile->dialplan = "XML";
			}

			if (zstr(profile->context)) {
				profile->context = "default";
			}

			int i;

			for (i = 0; i < profile->i; i++) {
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "starting profile [%s]\n", profile->name);
			}
		}  // xprofile
	}	   // xprofiles

	if ((settings = switch_xml_child(cfg, "settings"))) {
		for (param = switch_xml_child(settings, "param"); param; param = param->next) {
			char *var = NULL;
			char *val = NULL;

			var = (char *)switch_xml_attr_soft(param, "name");
			val = (char *)switch_xml_attr_soft(param, "value");

			if (!strcasecmp(var, "debug")) {
				if (val) {
					xmpp_globals.debug = atoi(val);
				}
			} else if (!strcasecmp(var, "enable-presence") && val) {
				xmpp_globals.enable_presence = switch_true(val);
			} else if (!strcasecmp(var, "detach-timeout-sec") && val) {
				int tmp = atoi(val);
				if (tmp > 0) {
					xmpp_globals.detach_timeout = tmp;
				}
			}
		}
	}

	switch_xml_free(xml);

	return status;
}

static int init(void)
{
	parse_config("xmpp-rtc.conf");
	xmpp_globals.running = 1;

	return 0;
}

SWITCH_STANDARD_API(xmpp_function)
{
	char *argv[1024] = {0};
	int argc = 0;
	char *mycmd = NULL;
	switch_status_t status = SWITCH_STATUS_SUCCESS;

	static const char usage_string[] = "USAGE:\n"
									   "--------------------------------------------------------------------------------\n"
									   "xmpp [debug <on [1-10]|off>]\n"
									   "xmpp help\n"
									   "--------------------------------------------------------------------------------\n";

	if (zstr(cmd)) {
		stream->write_function(stream, "%s", usage_string);
		goto done;
	}

	if (!(mycmd = strdup(cmd))) {
		status = SWITCH_STATUS_MEMERR;
		goto done;
	}

	if (!(argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])))) || !argv[0]) {
		stream->write_function(stream, "%s", usage_string);
		goto done;
	}

	if (!strcasecmp(argv[0], "help")) {
		stream->write_function(stream, "%s", usage_string);
		goto done;
	} else if ((!strcasecmp(argv[0], "debug")) && (!argv[1])) {
		stream->write_function(stream, "+OK debug %s\n", xmpp_globals.debug ? "on" : "off");
		goto done;
	} else if ((!strcasecmp(argv[0], "debug")) && (!strcasecmp(argv[1], "off"))) {
		xmpp_globals.debug = 0;
		stream->write_function(stream, "+OK debug off\n");
		goto done;
	} else if ((!strcasecmp(argv[0], "debug")) && (!strcasecmp(argv[1], "on"))) {
		int level = 10;
		if (argc > 2) {
			level = atoi(argv[2]);

			if (level < 0) level = 0;
			if (level > 10) level = 10;
		}

		xmpp_globals.debug = level;

		if (level == 0) {
			stream->write_function(stream, "+OK debug off\n");
		} else {
			stream->write_function(stream, "+OK debug on %d\n", level);
		}
		goto done;
	}

done:
	switch_safe_free(mycmd);
	return status;
}

static void runtime(xmpp_profile_t *profile)
{
	while (profile->running) {
		switch_yield(3 * 1000 * 1000);
	}
}

static void *SWITCH_THREAD_FUNC profile_thread(switch_thread_t *thread, void *obj)
{
	xmpp_profile_t *profile = (xmpp_profile_t *)obj;
	int sanity = 50;

	switch_mutex_lock(xmpp_globals.mutex);
	xmpp_globals.profile_threads++;
	switch_mutex_unlock(xmpp_globals.mutex);

	profile->in_thread = 1;
	profile->running = 1;

	runtime(profile);
	profile->running = 0;

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "profile %s shutdown, Waiting for %d threads\n", profile->name,
					  profile->jsock_count);

	while (--sanity > 0 && profile->jsock_count > 0) {
		switch_yield(100000);
	}

	del_profile(profile);

	switch_thread_rwlock_wrlock(profile->rwlock);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s Thread ending\n", profile->name);
	switch_thread_rwlock_unlock(profile->rwlock);
	profile->in_thread = 0;

	switch_mutex_lock(xmpp_globals.mutex);
	xmpp_globals.profile_threads--;
	switch_mutex_unlock(xmpp_globals.mutex);

	return NULL;
}

static void run_profile_thread(xmpp_profile_t *profile)
{
	switch_thread_data_t *td;

	td = switch_core_alloc(profile->pool, sizeof(*td));

	td->alloc = 0;
	td->func = profile_thread;
	td->obj = profile;
	td->pool = profile->pool;

	switch_thread_pool_launch_thread(&td);
}

static void run_profiles(void)
{
	xmpp_profile_t *p;

	switch_mutex_lock(xmpp_globals.mutex);
	for (p = xmpp_globals.profile_head; p; p = p->next) {
		if (!p->in_thread) {
			run_profile_thread(p);
		}
	}
	switch_mutex_unlock(xmpp_globals.mutex);
}

static switch_call_cause_t xmpp_outgoing_channel(switch_core_session_t *session, switch_event_t *var_event,
												 switch_caller_profile_t *outbound_profile, switch_core_session_t **new_session,
												 switch_memory_pool_t **pool, switch_originate_flag_t flags,
												 switch_call_cause_t *cancel_cause);
switch_io_routines_t xmpp_io_routines = {
	/*.outgoing_channel */ xmpp_outgoing_channel};

static char *xmpp_get_dial_string(const char *uid, switch_stream_handle_t *rstream)
{
	switch_stream_handle_t *use_stream = NULL, stream = {0};
	char *gen_uid = NULL;
	int hits = 0;

	if (!strchr(uid, '@')) {
		gen_uid = switch_mprintf("%s@%s", uid, switch_core_get_domain(SWITCH_FALSE));
		uid = gen_uid;
	}

	if (rstream) {
		use_stream = rstream;
	} else {
		SWITCH_STANDARD_STREAM(stream);
		use_stream = &stream;
	}

	use_stream->write_function(use_stream, "%s/%s", ENDPOINT_NAME, uid);
	hits++;
	switch_safe_free(gen_uid);

	if (!hits) {
		use_stream->write_function(use_stream, "error/user_not_registered");
	}

	if (use_stream->data) {
		char *p = use_stream->data;
		if (end_of(p) == ',') {
			end_of(p) = '\0';
		}
	}

	return use_stream->data;
}

SWITCH_STANDARD_API(xmpp_contact_function)
{
	char *uid = (char *)cmd;

	if (!zstr(uid)) {
		xmpp_get_dial_string(uid, stream);
	}

	return SWITCH_STATUS_SUCCESS;
}

static switch_call_cause_t xmpp_outgoing_channel(switch_core_session_t *session, switch_event_t *var_event,
												 switch_caller_profile_t *outbound_profile, switch_core_session_t **new_session,
												 switch_memory_pool_t **pool, switch_originate_flag_t flags,
												 switch_call_cause_t *cancel_cause)
{
	switch_call_cause_t cause = SWITCH_CAUSE_CHANNEL_UNACCEPTABLE;
	char *dial_string = NULL;  // auto_answer
	char *dest = NULL;
	char *profile_name = NULL;	// default

	// destination_number auto_answer
	if (!zstr(outbound_profile->destination_number)) {
		dial_string = strdup(outbound_profile->destination_number);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Outbound destination number is: %s\n",
						  outbound_profile->destination_number);
	}

	if (zstr(dial_string)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ALERT, "NO VALID DIAL STRING\n");
		goto end;
	}

	//  profile/user@domain
	if (switch_stristr("/", dial_string)) {
		dest = strchr(dial_string, '/');

		if (dest) {
			*(dest++) = '\0';
		}
		// use dial_string 's profile name
		profile_name = dial_string;
	} else {
		// no profile using default
		dest = dial_string;
		profile_name = "default";
	}

	if (zstr(dest)) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ALERT, "NO VALID DESTINATION\n");
		goto end;
	}

	// check event header
	const char *profile_name_from_header = switch_event_get_header(var_event, "profile_name");
	if (!zstr(profile_name_from_header)) {
		profile_name = (char *)profile_name_from_header;
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "profile_name_from_header: %s\n", profile_name);
	}

	const char *dialed_user = switch_event_get_header(var_event, "dialed_user");
	const char *dialed_domain = switch_event_get_header(var_event, "dialed_domain");
	const char *url = switch_event_get_header(var_event, "url");

	const char *video_use_audio_ice = switch_event_get_header(var_event, "video_use_audio_ice");
	const char *rtp_payload_space = switch_event_get_header(var_event, "rtp_payload_space");
	const char *absolute_codec_string = switch_event_get_header(var_event, "absolute_codec_string");

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
					  "dialed_user: %s\n dialed_domain: %s\n url: %s\n video_use_audio_ice: %s\n rtp_payload_space: %s\n "
					  "absolute_codec_string: %s\n ",
					  dialed_user, dialed_domain, url, video_use_audio_ice, rtp_payload_space, absolute_codec_string);

	if (dialed_user) {
		if (dialed_domain) {
			switch_event_add_header(var_event, SWITCH_STACK_BOTTOM, "xmpp_orig_dest", "%s@%s", dialed_user, dialed_domain);
		} else {
			switch_event_add_header_string(var_event, SWITCH_STACK_BOTTOM, "xmpp_orig_dest", dialed_user);
		}
		if (zstr(switch_event_get_header(var_event, "origination_callee_id_number"))) {
			switch_event_add_header_string(var_event, SWITCH_STACK_BOTTOM, "origination_callee_id_number", dialed_user);
			outbound_profile->callee_id_number =
				switch_sanitize_number(switch_core_strdup(outbound_profile->pool, dialed_user));
		}
	}

	if ((cause = switch_core_session_outgoing_channel(session, var_event, "rtc", outbound_profile, new_session, NULL, SOF_NONE,
													  cancel_cause)) == SWITCH_CAUSE_SUCCESS) {
		switch_channel_t *channel = switch_core_session_get_channel(*new_session);

		switch_caller_profile_t *caller_profile;
		xmpp_pvt_t *tech_pvt = NULL;
		char name[512];

		tech_pvt = switch_core_session_alloc(*new_session, sizeof(*tech_pvt));
		tech_pvt->pool = switch_core_session_get_pool(*new_session);
		tech_pvt->session = *new_session;
		tech_pvt->channel = channel;
		tech_pvt->destination_number = switch_core_session_strdup(*new_session, dest);
		if (!zstr(url)) tech_pvt->url = switch_core_session_strdup(*new_session, url);
		jsock_t *jsock = tech_pvt->jsock = jsock_new(tech_pvt->pool, profile_name);

		if (!tech_pvt->jsock) {
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ALERT, "JSOCK CREATE ERROR!\n");
			goto end;
		}

		// get ctrl_uuid from channel varialbe or profile conf
		char *ctrl_uuid = switch_event_get_header(var_event, "xmpp_ctrl_uuid");
		if (zstr(ctrl_uuid)) ctrl_uuid = tech_pvt->jsock->profile->xmpp_ctrl_subject;
		if (zstr(ctrl_uuid)) ctrl_uuid = "rtc";

		// keep consistency with xnode_invite dont add prefix
		tech_pvt->ctrl_uuid = switch_core_session_strdup(*new_session, ctrl_uuid);

		switch_set_string(jsock->ctrl_uuid, ctrl_uuid);

		switch_core_session_set_private_class(*new_session, tech_pvt, SWITCH_PVT_SECONDARY);

		if (session) {
			switch_channel_t *ochannel = switch_core_session_get_channel(session);

			if (switch_true(switch_channel_get_variable(ochannel, SWITCH_BYPASS_MEDIA_VARIABLE))) {
				switch_channel_set_flag(channel, CF_PROXY_MODE);
				switch_channel_set_flag(ochannel, CF_PROXY_MODE);
				switch_channel_set_cap(channel, CC_BYPASS_MEDIA);
			}
		}

		tech_pvt->call_id = switch_core_session_strdup(*new_session, switch_core_session_get_uuid(*new_session));
		switch_set_string(jsock->uuid_str, tech_pvt->call_id);

		if ((tech_pvt->smh = switch_core_session_get_media_handle(*new_session))) {
			tech_pvt->mparams = switch_core_media_get_mparams(tech_pvt->smh);
		}

		switch_snprintf(name, sizeof(name), "xmpp/%s", tech_pvt->destination_number);
		switch_channel_set_name(channel, name);
		switch_channel_set_variable(channel, "jsock_uuid_str", tech_pvt->jsock->uuid_str);
		switch_channel_set_variable(channel, "event_channel_cookie", tech_pvt->jsock->uuid_str);

		if (tech_pvt->jsock->profile && !zstr(tech_pvt->jsock->profile->media_timeout)) {
			switch_channel_set_variable(channel, "media_timeout", tech_pvt->jsock->profile->media_timeout);
		}

		if ((caller_profile = switch_caller_profile_dup(switch_core_session_get_pool(*new_session), outbound_profile))) {
			switch_channel_set_caller_profile(channel, caller_profile);
		}

		if (!zstr(url)) {
			switch_channel_set_flag(channel, CF_AUDIO_VIDEO_BUNDLE);
			switch_channel_set_flag(channel, CF_AUDIO);
			switch_channel_set_variable(channel, "rtp_jitter_buffer_during_bridge", "true");
		}

		if (zstr(video_use_audio_ice)) {
			switch_channel_set_variable(channel, "video_use_audio_ice", "true");
		}

		if (zstr(rtp_payload_space)) {
			switch_channel_set_variable(channel, "rtp_payload_space", "106");
		}

		if (zstr(absolute_codec_string)) {
			switch_channel_set_variable(channel, "absolute_codec_string", "OPUS,H264");
		}

		switch_channel_add_state_handler(channel, &switch_state_handler);
		switch_core_event_hook_add_receive_message(*new_session, xmpp_message_hook);
		switch_channel_set_state(channel, CS_INIT);
	}

end:

	if (cause != SWITCH_CAUSE_SUCCESS) {
		UNPROTECT_INTERFACE(xmpp_endpoint_interface);
		if (session) {
			switch_core_session_destroy(&session);
		}
	}

	// free this from strdup
	switch_safe_free(dial_string);

	return cause;
}

#define add_it(_name, _ename)                                        \
	if ((tmp = switch_event_get_header(event, _ename))) {            \
		cJSON_AddItemToObject(data, _name, cJSON_CreateString(tmp)); \
	}

switch_status_t xmpp_load SWITCH_MODULE_LOAD_ARGS
{
	switch_api_interface_t *api_interface = NULL;
	switch_chat_interface_t *chat_interface = NULL;
	int r;

	memset(&xmpp_globals, 0, sizeof(xmpp_globals));
	xmpp_globals.pool = pool;
#ifndef WIN32
	xmpp_globals.ready = SIGUSR1;
#endif
	xmpp_globals.enable_presence = SWITCH_TRUE;

	switch_mutex_init(&xmpp_globals.mutex, SWITCH_MUTEX_NESTED, xmpp_globals.pool);
	switch_thread_rwlock_create(&xmpp_globals.event_channel_rwlock, xmpp_globals.pool);
	switch_core_hash_init(&xmpp_globals.event_channel_hash);
	switch_thread_rwlock_create(&xmpp_globals.tech_rwlock, xmpp_globals.pool);
	switch_mutex_init(&xmpp_globals.detach_mutex, SWITCH_MUTEX_NESTED, xmpp_globals.pool);
	switch_mutex_init(&xmpp_globals.detach2_mutex, SWITCH_MUTEX_NESTED, xmpp_globals.pool);
	switch_thread_cond_create(&xmpp_globals.detach_cond, xmpp_globals.pool);
	xmpp_globals.detach_timeout = 120;

	// 读取配置文件
	r = init();

	if (r) return SWITCH_STATUS_TERM;

	// 命令行输入 xmpp 入口
	SWITCH_ADD_API(api_interface, "xmpp", "XMPP API", xmpp_function, "syntax");

	// 命令行输入 xmpp_contact 入口
	SWITCH_ADD_API(api_interface, "xmpp_contact", "Generate a xmpp endpoint dialstring", xmpp_contact_function, "user@domain");

	// 命令行提示
	switch_console_set_complete("add xmpp help");
	switch_console_set_complete("add xmpp debug on");
	switch_console_set_complete("add xmpp debug off");

	// 注册 xmpp 处理 schema
	xmpp_endpoint_interface =
		(switch_endpoint_interface_t *)switch_loadable_module_create_interface(*module_interface, SWITCH_ENDPOINT_INTERFACE);
	xmpp_endpoint_interface->interface_name = ENDPOINT_NAME;
	xmpp_endpoint_interface->io_routines = &xmpp_io_routines;

	run_profiles();

	xmpp_init();

	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

switch_status_t xmpp_shutdown SWITCH_MODULE_SHUTDOWN_ARGS
{
	do_shutdown();
	attach_wake();

	switch_core_hash_destroy(&xmpp_globals.event_channel_hash);

	return SWITCH_STATUS_SUCCESS;
}

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet
 */
