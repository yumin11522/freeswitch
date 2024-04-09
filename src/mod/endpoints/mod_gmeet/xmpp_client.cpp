//
// Created by xiongqimin on 2023/12/19.
//
#define GMTC_CLASS ""

#include <algorithm>
#include <mutex>
#include <cstddef>
#include <vector>

#include "xmpp/xmppclientmgr.h"
#include "xmpp_client.h"

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

xmpp_client_t *join_room(char *room_id, char *room_secret)
{
	xmppclient::XmppClientMgr::setXmppSrvHost("10.8.106.128");
	xmppclient::XmppClientMgr::setXmppSrvPort(6222);
	//	xmppclient::Logger::SetLogLevel(xmppclient::Logger::LogLevel::LOG_DEBUG);
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

int exchange_sdp(xmpp_client_t *client, const char *sdp, char *answer, int size)
{
	auto c = (xmppclient::XmppClient *)client;
	if (!c) {
		return false;
	}
	return copyAndReturn(c->onSdpExchange(sdp), answer, size);
}

int leave_room(xmpp_client_t *client)
{
	auto c = (xmppclient::XmppClient *)client;
	if (!c) {
		return GMT_ERR_INVALID;
	}
	return xmppclient::XmppClientMgr::Instance().removeClient(c->id());
}