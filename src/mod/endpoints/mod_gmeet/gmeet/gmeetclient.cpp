//
// Created by xiongqimin on 2023/12/14.
//

#include "gmeetclient.h"

#include "gmeetclientmgr.h"

using namespace std;

namespace gmeetclient {
GMeetClient::GMeetClient(const std::string &id, const std::string &room_id, const std::string &room_secret) :
        m_client_id(id),
//        m_client(make_unique<gmeetclient::RtcClient>()),
        m_xmppclient(std::make_shared<gmeetclient::XmppConnection>(GMeetClientMgr::getXmppSrvHost(),
                                                                   GMeetClientMgr::getXmppSrvPort(), room_id,
                                                                   room_secret)) {
    m_xmppclient->setOnSdpExchange([this](auto &&PH1) { return onSdpExchange(std::forward<decltype(PH1)>(PH1)); });
    m_xmppclient->setOnSourceChange(
            [this](auto &&PH1, auto &&PH2, auto &&PH3) {
                onSourceChange(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
                               std::forward<decltype(PH3)>(PH3));
            });
}

GMeetClient::~GMeetClient() {
    if (m_xmppclient) {
        m_xmppclient->disconnect();
    }
}

bool GMeetClient::sendRtp(AVType type, const char *data, size_t size) {
//    m_client->sendRtp(type, data, size);
    return true;
}

void GMeetClient::onSourceChange(SourceEvent evt, AVType t, const std::vector<uint32_t> &ssrc_vec) {
    for (auto ssrc: ssrc_vec) {
//        m_client->addSsrc(t, ssrc);
    }
}

std::string GMeetClient::onSdpExchange(const string &offer) {
//    m_client->setOfferSdp(offer);
//    m_client->start();
//    return m_client->answer();
	return "";
}

bool GMeetClient::doconnect() {
    return m_xmppclient->connect();
}

//void GMeetClient::addTrack(const Track &track) {
//    if (m_client) {
//        m_client->addTrack(track);
//    }
//}

const string &GMeetClient::id() const {
    return m_client_id;
}


} // gmeetclient