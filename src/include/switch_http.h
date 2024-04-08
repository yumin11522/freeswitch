#ifndef FREESWITCH_SWITCH_HTTP_H
#define FREESWITCH_SWITCH_HTTP_H

#include <switch.h>
#include <switch_curl.h>

SWITCH_BEGIN_EXTERN_C

#define SWITCH_HTTP_CM_GET 0
#define SWITCH_HTTP_CM_POST 1

typedef struct http_data_s {
	switch_buffer_t *body_buffer;
	switch_memory_pool_t *pool;
	char *content_type;
	int err;
	long code;
	switch_size_t body_size;
	switch_curl_slist_t *headers;
	switch_CURLcode perform_code;
} http_data_t;

SWITCH_DECLARE(http_data_t *) switch_http_post(const char *url, const char *data, switch_memory_pool_t *pool);
SWITCH_DECLARE(http_data_t *) switch_http_get(const char *url, switch_memory_pool_t *pool);
SWITCH_DECLARE(http_data_t *)
switch_http_request(int method, const char *url, const void *data, size_t datalen, switch_curl_slist_t *headers,
					switch_memory_pool_t *pool, int curl_connect_timeout, int curl_timeout);

SWITCH_END_EXTERN_C
#endif
/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
