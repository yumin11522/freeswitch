//
// Created by xiongqimin on 2023/12/15.
//
#define GMTC_CLASS "XmppClientMgr"

#include "xmppclientmgr.h"

#include <map>
#include <mutex>

#include "logger.h"

const char kServerName[] = "XmppClient(build in " __DATE__ " " __TIME__ ")";

namespace xmppclient
{

static std::string g_xmppSrvHost;
static uint16_t g_xmppSrvPort;

class XmppClientMgrImpl
{
   public:
	std::mutex m_client_mutex;
	std::map<std::string, XmppClient::Ptr> m_client_map;
};

std::string version() { return kServerName; }

XmppClientMgr &XmppClientMgr::Instance()
{
	static XmppClientMgr instance;
	return instance;
}

XmppClientMgr::XmppClientMgr() : _impl(std::make_unique<XmppClientMgrImpl>()) {}

const std::string &XmppClientMgr::getXmppSrvHost() { return g_xmppSrvHost; }

void XmppClientMgr::setXmppSrvHost(const std::string &xmppSrvHost) { g_xmppSrvHost = xmppSrvHost; }

uint16_t XmppClientMgr::getXmppSrvPort() { return g_xmppSrvPort; }

void XmppClientMgr::setXmppSrvPort(uint16_t xmppSrvPort) { g_xmppSrvPort = xmppSrvPort; }

XmppClient::Ptr XmppClientMgr::addClient(const std::string &room_id, const std::string &room_secret)
{
	std::lock_guard<std::mutex> lock(_impl->m_client_mutex);
	auto id = randomId();
	auto c = std::make_shared<XmppClient>(id, room_id, room_secret);
	_impl->m_client_map[id] = c;
	return c;
}

bool XmppClientMgr::removeClient(const std::string &id)
{
	std::lock_guard<std::mutex> lock(_impl->m_client_mutex);
	return _impl->m_client_map.erase(id) > 0;
}

XmppClient::Ptr XmppClientMgr::getClient(const std::string &id)
{
	std::lock_guard<std::mutex> lock(_impl->m_client_mutex);
	return _impl->m_client_map[id];
}

void XmppClientMgr::clearClients()
{
	std::lock_guard<std::mutex> lock(_impl->m_client_mutex);
	_impl->m_client_map.clear();
}

XmppClientMgr::~XmppClientMgr() { XmppClientMgr::clearClients(); }
}  // namespace XmppClient