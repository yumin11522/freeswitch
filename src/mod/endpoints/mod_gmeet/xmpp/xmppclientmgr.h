//
// Created by xiongqimin on 2023/12/15.
//

#ifndef GMEET_CLIENT_XmppClientMGR_H
#define GMEET_CLIENT_XmppClientMGR_H

#include "xmppclient.h"

namespace xmppclient {

class XmppClientMgrImpl;

std::string version();

std::string randomId(size_t length = 10);

class XmppClientMgr {
public:
    ~XmppClientMgr();
    static XmppClientMgr &Instance();

    static const std::string &getXmppSrvHost();

    static void setXmppSrvHost(const std::string &xmppSrvHost);

    static uint16_t getXmppSrvPort();

    static void setXmppSrvPort(uint16_t xmppSrvPort);

    XmppClient::Ptr addClient(const std::string &room_id, const std::string &room_secret);

    bool removeClient(const std::string &id);

    XmppClient::Ptr getClient(const std::string &id);

    void clearClients();
private:

    XmppClientMgr();

private:
	std::unique_ptr<XmppClientMgrImpl> _impl;
};

} // XmppClient

#endif //GMEET_CLIENT_XmppClientMGR_H
