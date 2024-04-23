//
// Created by xiongqimin on 2023/12/14.
//
#define GMTC_CLASS "XmppClient"

#include "xmppclient.h"

#include <thread>

#include "logger.h"
#include "xmppclientmgr.h"

using namespace std;

namespace xmppclient {
XmppClient::XmppClient(const std::string &id, const std::string &room_id, const std::string &room_secret)
	:
        m_client_id(id),
	  m_xmppclient(std::make_shared<xmppclient::XmppConnection>(XmppClientMgr::getXmppSrvHost(),
																XmppClientMgr::getXmppSrvPort(), room_id,
                                                                   room_secret)) {
    m_xmppclient->setOnSdpExchange([this](auto &&PH1) { return onSdpExchange(std::forward<decltype(PH1)>(PH1)); });
    m_xmppclient->setOnSourceChange(
            [this](auto &&PH1, auto &&PH2, auto &&PH3) {
                onSourceChange(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
                               std::forward<decltype(PH3)>(PH3));
            });
}

XmppClient::~XmppClient()
{
    if (m_xmppclient) {
        m_xmppclient->disconnect();
    }
}

void XmppClient::onSourceChange(SourceEvent evt, AVType t, const std::vector<uint32_t> &ssrc_vec)
{
    for (auto ssrc: ssrc_vec) {
//        m_client->addSsrc(t, ssrc);
		GMTC_DEBUG("---------------------------------------------------- %d", ssrc);
    }
}

void XmppClient::setAnswer(const std::string& answer) { m_answer = answer; }

std::string XmppClient::getAnswer() const { return m_answer; }

std::string XmppClient::getOffer() const { return m_offer; }

std::string XmppClient::onSdpExchange(const string &offer)
{ 
    m_offer = offer;
	while (m_answer.empty()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return m_answer;
}



bool XmppClient::doconnect() {
    return m_xmppclient->connect();
}

bool XmppClient::disconnect() { return m_xmppclient->disconnect(); }
//void xmppClient::addTrack(const Track &track) {
//    if (m_client) {
//        m_client->addTrack(track);
//    }
//}

const string &XmppClient::id() const {
    return m_client_id;
}


} // xmppclient