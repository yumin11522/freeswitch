//
// Created by xiongqimin on 2023/12/19.
//
#define GMTC_CLASS ""

#include "xmpp_client.h"

#include <switch.h>

#include <algorithm>
#include <cstddef>
#include <mutex>
#include <thread>
#include <vector>

#include "xmpp/logger.h"
#include "xmpp/xmppclientmgr.h"

int copyAndReturn(std::string s, char *buffer, int size)
{
	if (!buffer) return int(s.size() + 1);

	if (size < int(s.size() + 1)) return GMT_ERR_TOO_SMALL;

	std::copy(s.begin(), s.end(), buffer);
	buffer[s.size()] = '\0';
	return int(s.size() + 1);
}

int copyAndReturn(std::vector<std::byte> b, char *buffer, int size)
{
	if (!buffer) return int(b.size());

	if (size < int(b.size())) return GMT_ERR_TOO_SMALL;

	auto data = reinterpret_cast<const char *>(b.data());
	std::copy(data, data + b.size(), buffer);
	return int(b.size());
}

template <typename T> int copyAndReturn(std::vector<T> b, T *buffer, int size)
{
	if (!buffer) return int(b.size());

	if (size < int(b.size())) return GMT_ERR_TOO_SMALL;
	std::copy(b.begin(), b.end(), buffer);
	return int(b.size());
}

void printVersion() {}

class SwitchLogHandler : public xmppclient::Log::LogHandlerInterface
{
	void OnLog(xmppclient::Log::LLevel level, char *payload, size_t len)
	{
		switch (level) {
			case xmppclient::Log::LLevel::LOG_ERROR:
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "%s \n", payload);
				break;
			case xmppclient::Log::LLevel::LOG_WARN:
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "%s \n", payload);
				break;
			case xmppclient::Log::LLevel::LOG_DEBUG:
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s \n", payload);
				break;
			case xmppclient::Log::LLevel::LOG_TRACE:
				switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "%s \n", payload);
				break;
		}
	}
};

void xmpp_init() {
	xmppclient::XmppClientMgr::setXmppSrvHost("10.8.106.128");
	xmppclient::XmppClientMgr::setXmppSrvPort(6222);
	xmppclient::Log::SetLogLevel(xmppclient::Log::LLevel::LOG_DEBUG);
	xmppclient::Log::SetHandler(new SwitchLogHandler);
}

xmpp_client_t *join_room(char *room_id, char *room_secret)
{
	return (xmpp_client_t *)xmppclient::XmppClientMgr::Instance().addClient(room_id, room_secret).get();
}

int doconnect(xmpp_client_t *client)
{
	auto c = (xmppclient::XmppClient *)client;
	if (!c) {
		return GMT_ERR_INVALID;
	}
	return c->doconnect();
}

int get_xmpp_offer(xmpp_client_t *client, char *answer, int size)
{
	auto c = (xmppclient::XmppClient *)client;
	if (!c) {
		return GMT_ERR_INVALID;
	}
	while (c->getOffer().empty()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	return copyAndReturn(c->getOffer(), answer, size);
}

int set_switch_answer(xmpp_client_t* client, const char* answer) {
	auto c = (xmppclient::XmppClient *)client;
	if (!c) {
		return GMT_ERR_INVALID;
	}
	c->setAnswer(answer);
	return 0;
};

int leave_room(xmpp_client_t *client)
{
	auto c = (xmppclient::XmppClient *)client;
	if (!c) {
		return GMT_ERR_INVALID;
	}
	c->disconnect();
	return xmppclient::XmppClientMgr::Instance().removeClient(c->id());
}