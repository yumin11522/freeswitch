//
// Created by xiongqimin on 2023/12/14.
//

#ifndef XMPP_CLIENT_XMPPCLIENT_H
#define XMPP_CLIENT_XMPPCLIENT_H

#include <string>
#include "xmppconnection.h"

namespace xmppclient {

class XmppClient : public std::enable_shared_from_this<XmppClient>
{
   public:
    using Ptr = std::shared_ptr<XmppClient>;

    XmppClient(const std::string &id, const std::string &room_id, const std::string &room_secret);

    XmppClient(const XmppClient &client) = delete;

    virtual ~XmppClient();

    const std::string &id() const;

    virtual bool doconnect();

    virtual bool disconnect();

    void setAnswer(const std::string &answer);

    std::string getAnswer() const;

    std::string getOffer() const;

    std::string onSdpExchange(const std::string &offer);

    void onSourceChange(SourceEvent evt, AVType t, const std::vector<uint32_t> &ssrc_vec);

private:
    std::string m_client_id;
    std::shared_ptr<xmppclient::XmppConnection> m_xmppclient;
	std::string m_offer;
	std::string m_answer;
};

} // xmppclient

#endif //xmpp_CLIENT_xmppCLIENT_H
