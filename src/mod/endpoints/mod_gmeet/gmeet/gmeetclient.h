//
// Created by xiongqimin on 2023/12/14.
//

#ifndef GMEET_CLIENT_GMEETCLIENT_H
#define GMEET_CLIENT_GMEETCLIENT_H

#include <string>
#include "xmppconnection.h"

namespace gmeetclient {

class GMeetClient : public std::enable_shared_from_this<GMeetClient> {
public:
    using Ptr = std::shared_ptr<GMeetClient>;

    GMeetClient(const std::string &id, const std::string &room_id, const std::string &room_secret);

    GMeetClient(const GMeetClient &client) = delete;

    virtual ~GMeetClient();

    const std::string &id() const;

//    void addTrack(const gmeetclient::Track &track);

    virtual bool doconnect();

    virtual bool sendRtp(AVType type, const char *data, size_t size);

    std::string onSdpExchange(const std::string &offer);
    void onSourceChange(SourceEvent evt, AVType t, const std::vector<uint32_t> &ssrc_vec);

private:
    std::string m_client_id;
    std::shared_ptr<gmeetclient::XmppConnection> m_xmppclient;
//    std::unique_ptr<gmeetclient::RtcClient> m_client;
};

} // gmeetclient

#endif //GMEET_CLIENT_GMEETCLIENT_H
