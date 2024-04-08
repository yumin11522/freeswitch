//
// Created by xiongqimin on 2023/12/15.
//

#ifndef GMEET_CLIENT_GMEETCLIENTMGR_H
#define GMEET_CLIENT_GMEETCLIENTMGR_H

#include "gmeetclient.h"

namespace gmeetclient {

class GMeetClientMgrImpl;

std::string version();

std::string randomId(size_t length = 10);

class GMeetClientMgr {
public:
    ~GMeetClientMgr();
    static GMeetClientMgr &Instance();

    static const std::string &getXmppSrvHost();

    static void setXmppSrvHost(const std::string &xmppSrvHost);

    static uint16_t getXmppSrvPort();

    static void setXmppSrvPort(uint16_t xmppSrvPort);

    GMeetClient::Ptr addClient(const std::string &room_id, const std::string &room_secret);

    bool removeClient(const std::string &id);

    GMeetClient::Ptr getClient(const std::string &id);

    void clearClients();
private:

    GMeetClientMgr();

private:
	std::unique_ptr<GMeetClientMgrImpl> _impl;
};

} // gmeetclient

#endif //GMEET_CLIENT_GMEETCLIENTMGR_H
