//
// Created by xiongqimin on 2023/12/15.
//
#define GMTC_CLASS "GMeetClientMgr"

#include "gmeetclientmgr.h"

#include <map>
#include <mutex>

#include "logger.h"

const char kServerName[] = "GMeetClient(build in " __DATE__ " " __TIME__ ")";

namespace gmeetclient
{

static std::string g_xmppSrvHost;
static uint16_t g_xmppSrvPort;

class GMeetClientMgrImpl
{
   public:
	std::mutex m_client_mutex;
	std::map<std::string, GMeetClient::Ptr> m_client_map;
};

std::string version() { return kServerName; }

GMeetClientMgr &GMeetClientMgr::Instance()
{
	static GMeetClientMgr instance;
	return instance;
}

GMeetClientMgr::GMeetClientMgr() : _impl(std::make_unique<GMeetClientMgrImpl>()) {}

const std::string &GMeetClientMgr::getXmppSrvHost() { return g_xmppSrvHost; }

void GMeetClientMgr::setXmppSrvHost(const std::string &xmppSrvHost) { g_xmppSrvHost = xmppSrvHost; }

uint16_t GMeetClientMgr::getXmppSrvPort() { return g_xmppSrvPort; }

void GMeetClientMgr::setXmppSrvPort(uint16_t xmppSrvPort) { g_xmppSrvPort = xmppSrvPort; }

GMeetClient::Ptr GMeetClientMgr::addClient(const std::string &room_id, const std::string &room_secret)
{
	std::lock_guard<std::mutex> lock(_impl->m_client_mutex);
	auto id = randomId();
	auto c = std::make_shared<GMeetClient>(id, room_id, room_secret);
	_impl->m_client_map[id] = c;
	return c;
}

bool GMeetClientMgr::removeClient(const std::string &id)
{
	std::lock_guard<std::mutex> lock(_impl->m_client_mutex);
	return _impl->m_client_map.erase(id) > 0;
}

GMeetClient::Ptr GMeetClientMgr::getClient(const std::string &id)
{
	std::lock_guard<std::mutex> lock(_impl->m_client_mutex);
	return _impl->m_client_map[id];
}

void GMeetClientMgr::clearClients()
{
	std::lock_guard<std::mutex> lock(_impl->m_client_mutex);
	_impl->m_client_map.clear();
}

GMeetClientMgr::~GMeetClientMgr() { GMeetClientMgr::clearClients(); }
}  // namespace gmeetclient