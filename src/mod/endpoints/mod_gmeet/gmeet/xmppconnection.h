//
// Created by xiongqimin on 2023/12/13.
//

#ifndef GMEET_CLIENT_XMPPCONNECTION_H
#define GMEET_CLIENT_XMPPCONNECTION_H

#include <string>
#include <memory>
#include <vector>
#include <functional>

namespace gmeetclient {

constexpr char GMEET_DEFAULT_CONFERENCE_DOMAIN[] = "conference.gmeet.numax.tech";

constexpr char GMEET_DEFAULT_JOIN_DOMAIN[] = "guest.gmeet.numax.tech";

enum AVType {
    Audio, Video
};
enum SourceEvent {
    Add, Remove
};

class XmppConnectionImpl;

class XmppConnection {
public:
    friend XmppConnectionImpl;
    using OnSdpExchange = std::function<std::string (const std::string &sdp)>;
    using OnSourceChange = std::function<void(SourceEvent evt, AVType t, const std::vector<uint32_t> ssrcs)>;

    XmppConnection(const std::string &ip, uint16_t port, const std::string &room_id,
                   const std::string &room_secret);

    XmppConnection(const std::string &id, const std::string &ip, uint16_t port, const std::string &room_id,
                   const std::string &room_secret, const std::string &conference_domain,
                   const std::string &join_domain);

    ~XmppConnection();

    virtual bool connect();

    void setOnSdpExchange(OnSdpExchange cb);
    void setOnSourceChange(OnSourceChange cb);

    virtual bool disconnect();

    const std::string &getId() const;

private:
    std::unique_ptr<XmppConnectionImpl> _impl;

    std::string _id;
};

} // gmeet

#endif //GMEET_CLIENT_XMPPCONNECTION_H
