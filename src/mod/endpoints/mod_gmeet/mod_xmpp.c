#include "mod_xmpp.h"

SWITCH_MODULE_LOAD_FUNCTION(mod_xmpp_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_xmpp_shutdown);

SWITCH_MODULE_DEFINITION(mod_xmpp, mod_xmpp_load, mod_xmpp_shutdown, NULL);

mod_xmpp_globals_t mod_xmpp_globals;

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
		stream->write_function(stream, "+OK debug %s\n", mod_xmpp_globals.debug ? "on" : "off");
	} else if ((!strcasecmp(argv[0], "debug")) && (!strcasecmp(argv[1], "off"))) {
		mod_xmpp_globals.debug = 0;
		stream->write_function(stream, "+OK debug off\n");
		goto done;
	} else if ((!strcasecmp(argv[0], "debug")) && (!strcasecmp(argv[1], "on"))) {
		int level = 10;
		if (argc > 2) {
			level = atoi(argv[2]);

			if (level < 0) level = 0;
			if (level > 10) level = 10;
		}

		mod_xmpp_globals.debug = level;

		if (level == 0) {
			stream->write_function(stream, "+OK debug off\n");
		} else {
			stream->write_function(stream, "+OK debug on %d\n", level);
		}
	} else {
		stream->write_function(stream, "Unknown Command [%s]\n", argv[0]);
	}

done:
	switch_safe_free(mycmd);
	return status;
}

SWITCH_MODULE_LOAD_FUNCTION(mod_xmpp_load)
{
	switch_api_interface_t *api_interface = NULL;

	/* connect my internal structure to the blank pointer passed to me */
	*module_interface = switch_loadable_module_create_module_interface(pool, modname);

	SWITCH_ADD_API(api_interface, "xmpp", "xmpp api", xmpp_function, "syntax");
	switch_console_set_complete("add xmpp help");
	switch_console_set_complete("add xmpp debug on");
	switch_console_set_complete("add xmpp debug off");

	memset(&mod_xmpp_globals, 0, sizeof(mod_xmpp_globals));
	mod_xmpp_globals.pool = pool;
	mod_xmpp_globals.uuid = switch_core_strdup(pool, switch_core_get_uuid());
	mod_xmpp_globals.node_ip = switch_core_get_variable_pdup("local_ip_v4", pool);

	switch_mutex_init(&mod_xmpp_globals.sps_mutex, SWITCH_MUTEX_NESTED, pool);
	switch_mutex_init(&mod_xmpp_globals.cache_mutex, SWITCH_MUTEX_NESTED, pool);

	const char *err = NULL;
	if (switch_xml_reload(&err) != SWITCH_STATUS_SUCCESS) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "xml reload error: %s\n", err);
	} else {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "xml reload OK\n");
	}

	xmpp_load(module_interface, pool);

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "xmpp version: %s Started\n", xmpp_version());



	/* indicate that the module should continue to be loaded */
	return SWITCH_STATUS_SUCCESS;
}

/*
  Called when the system shuts down
*/
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_xmpp_shutdown)
{
	xmpp_shutdown();

	return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_RUNTIME_FUNCTION(mod_xmpp_runtime)
{
	if (!mod_xmpp_globals.dial_queue) {
		return SWITCH_STATUS_TERM;
	}

	switch_time_t ts = 0, reference = switch_time_now() + 1000000;

	mod_xmpp_globals.sps_total = 0;
	switch_core_session_ctl(SCSC_SPS, &mod_xmpp_globals.sps_total);
	mod_xmpp_globals.sps = mod_xmpp_globals.sps_total * 0.9;

	while (mod_xmpp_globals.running) {
		ts = switch_time_now();

		if (ts > reference) {
			reference = switch_time_now() + 1000000;
			switch_mutex_lock(mod_xmpp_globals.sps_mutex);
			mod_xmpp_globals.sps = mod_xmpp_globals.sps_total * 0.9;
			switch_mutex_unlock(mod_xmpp_globals.sps_mutex);
		}

		switch_yield(1000);
	}

	return SWITCH_STATUS_TERM;
}

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
