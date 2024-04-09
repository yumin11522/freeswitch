//
// Created by xiongqimin on 2023/12/13.
//

#define GMTC_CLASS "XmppConnection"

#include <chrono>
#include <random>
#include "xmppconnection.h"

#include "client.h"
#include "connectionlistener.h"
#include "jinglecontent.h"
#include "jinglesessionhandler.h"
#include "jinglesessionmanager.h"
#include "loghandler.h"
#include "mucroom.h"
#include "mucroomhandler.h"
#include "message.h"
#include "presencehandler.h"

#include "jinglefiletransfer.h"
#include "jingletransport.h"
#include "jingledescription.h"
#include "jinglegroup.h"
#include "jinglebridgesession.h"
#include "sdputil.h"
#include "jinglepresence.h"
#include "logger.h"

using namespace gloox;
using namespace gloox::Jingle;
using namespace std;

namespace xmppclient {

#define GET_ENUM_NAME(value) case value: return (#value)

std::string getJingleActionName(Action action) {
    switch (action) {
        GET_ENUM_NAME(ContentAccept);
        GET_ENUM_NAME(ContentAdd);
        GET_ENUM_NAME(ContentModify);
        GET_ENUM_NAME(ContentReject);
        GET_ENUM_NAME(ContentRemove);
        GET_ENUM_NAME(DescriptionInfo);
        GET_ENUM_NAME(SecurityInfo);
        GET_ENUM_NAME(SessionAccept);
        GET_ENUM_NAME(SessionInfo);
        GET_ENUM_NAME(SessionInitiate);
        GET_ENUM_NAME(SessionTerminate);
        GET_ENUM_NAME(TransportAccept);
        GET_ENUM_NAME(TransportInfo);
        GET_ENUM_NAME(TransportReject);
        GET_ENUM_NAME(TransportReplace);
        GET_ENUM_NAME(InvalidAction);
        default:
            return "";
    }
}

// Helper function to generate a random ID
std::string randomId(size_t length) {
    using std::chrono::high_resolution_clock;
    static thread_local std::mt19937 rng(
            static_cast<unsigned int>(high_resolution_clock::now().time_since_epoch().count()));
    static const std::string characters(
            "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    std::string id(length, '0');
    std::uniform_int_distribution<int> uniform(0, int(characters.size() - 1));
    std::generate(id.begin(), id.end(), [&]() { return characters.at(uniform(rng)); });
    return id;
}

class XmppConnectionImpl
        : public ConnectionListener,
          LogHandler,
          public MUCRoomHandler,
          public Jingle::SessionHandler,
          PresenceHandler {
public:
    XmppConnectionImpl(const std::string &id, const std::string &ip, uint16_t port, const std::string &room_id,
                       const std::string &room_secret, const std::string &conference_domain,
                       const std::string &join_domain) : _ip(ip), _port(port), _room_secret(room_secret),
                                                         self_jid(id + "@" + join_domain),
                                                         room_jid(room_id + "@" + conference_domain) {};

    ~XmppConnectionImpl() = default;

    bool connect() {
        client = std::make_unique<Client>(self_jid, "", _port);
        client->registerConnectionListener(this);
        client->registerPresenceHandler(this);
        client->setPresence(Presence::Available, -1);
        client->disco()->setVersion("gloox muc_example", GLOOX_VERSION, "Linux");
        client->disco()->setIdentity("client", "bot");
        client->setCompression(false);
        StringList ca;
        ca.push_back("/path/to/cacert.crt");
        client->setCACerts(ca);
        client->setTls(gloox::TLSDisabled);
        client->setServer(_ip);

        client->logInstance().registerLogHandler(LogLevelDebug, LogAreaAll, this);
        room_jid.setResource("SIP_" + randomId(10));
//        nick.setResource("251e703c");
        session_mgr = make_unique<Jingle::SessionManager>(client.get(), this);
        session_mgr->registerPlugin(new Jingle::Description());
        session_mgr->registerPlugin(new Jingle::Content());
        session_mgr->registerPlugin(new Jingle::FileTransfer());
        session_mgr->registerPlugin(new Jingle::TransportExt());
        session_mgr->registerPlugin(new Jingle::Group());
        session_mgr->registerPlugin(new Jingle::BridgeSession());

        room = make_unique<MUCRoom>(client.get(), room_jid, this, nullptr);
//        room->setPassword(_room_secret);

        if (client->connect(false)) {
            ConnectionError ce = ConnNoError;
            while (ce == ConnNoError) {
                ce = client->recv();
            }
            GMTC_ERROR("Connection error: %d ", ce);
        }
        return true;
    }

    bool disconnect() {
        room->leave();
        client->disconnect();
        return true;
    }

    void onConnect() override {
        GMTC_DEBUG("Connected!!!");
        room->join();
        room->getRoomInfo();
        room->getRoomItems();
    }

    void onDisconnect(ConnectionError e) override {
        GMTC_ERROR("Disconnect. reason: %d", client->authError());
    }

    void handleMUCParticipantPresence(MUCRoom *room, const MUCRoomParticipant participant,
                                      const Presence &presence) override {
        if (presence.presence() == Presence::Available) {
            GMTC_DEBUG("%s is in the room, too", participant.nick->resource().c_str());
        } else if (presence.presence() == Presence::Unavailable) {
            GMTC_DEBUG("%s left the room", participant.nick->resource().c_str());
        } else {
            GMTC_DEBUG("Presence is %d of %s", presence.presence(), participant.nick->resource().c_str());
        }
    }

    void handleMUCMessage(MUCRoom *room, const Message &msg, bool priv) override {
        GMTC_DEBUG("%s said: '%s' (history: %s, private: %s)", msg.from().resource().c_str(), msg.body().c_str(),
                   msg.when() ? "yes" : "no", priv ? "yes" : "no");
    }

    bool handleMUCRoomCreation(MUCRoom *room) override {
        GMTC_DEBUG("Room %s didn't exist, beeing created.", room->name().c_str());
        return true;
    }

    void handleMUCSubject(MUCRoom *room, const std::string &nick, const std::string &subject) override {
        if (nick.empty()) {
            GMTC_DEBUG("Subject: %s", subject.c_str());
        } else {
            GMTC_DEBUG("%s has set the subject to: %s", nick.c_str(), subject.c_str());
        }
    }

    void handleMUCInviteDecline(MUCRoom *room, const JID &invitee, const std::string &reason) override {
        GMTC_WARN("Invitee %s declined invitation. reason given: %s", invitee.full().c_str(), reason.c_str());
    }

    void handleMUCError(MUCRoom *room, StanzaError error) override {
        GMTC_ERROR("Muc got an error:  %d", error);
    }

    void handleMUCInfo(MUCRoom *room, int features, const std::string &name, const DataForm *infoForm) override {

    }

    void handleMUCItems(MUCRoom *room, const Disco::ItemList &items) override {
        Disco::ItemList::const_iterator it = items.begin();
        for (; it != items.end(); ++it) {
        }
    }

    bool onTLSConnect(const CertInfo &info) override {
//        helper::printfCert(info);
        return true;
    }

    void handleSessionActionError(Jingle::Action action, Jingle::Session *session, const Error *error) override {
        GMTC_ERROR("Handle session action error %s", getJingleActionName(action).c_str());
    }

    void handleIncomingSession(Jingle::Session *session) override {

    }

    void handleSessionAction(Jingle::Action action, Jingle::Session *session,
                             const Jingle::Session::Jingle *jingle) override {
        if (action == SessionInitiate) {
            const std::string &offer = to_sdp_str(jingle);
            if (_on_sdp_change) {
                auto answer = _on_sdp_change(offer);
                auto jingle_answer = to_jingle(answer);
                GMTC_DEBUG("Offer: %s, answer: %s", offer.c_str(), answer.c_str());
                session->sessionAccept(jingle_answer);
                Presence p(Presence::PresenceType::Available, room_jid);
                p.addExtension(new AudioVideoMutePresence(gloox::Jingle::Video, false));
                p.addExtension(new AudioVideoMutePresence(gloox::Jingle::Audio, false));
                client->send(p);
            }
        } else if (action == SourceAdd) {
            for (auto p: jingle->plugins()) {
                Content *content = (Content *) p;
                if (!content || content->plugins().empty()) {
                    continue;
                }
                if (_on_source_change) {
                    vector<uint32_t > ssrcs;
                    AVType mt = content->name() == "audio" ? AVType::Audio : AVType::Video;
                    for (auto c: content->plugins()) {
                        Description *desc = (Description *) c;
                        if (desc && !desc->sources().empty()) {
                            for (auto s: desc->sources()) {
                                ssrcs.emplace_back((uint32_t)stoul(s.ssrc));
                            }
                        }
                    }
                    _on_source_change(SourceEvent::Add, mt, ssrcs);
                }
            }

        } else {
        }
    }

    void handleLog(gloox::LogLevel level, LogArea area, const std::string &message) override {
        if (area == gloox::LogAreaXmlIncoming) {
            GMTC_DEBUG("Receive jingle message: %s", message.c_str());
        } else if (area == gloox::LogAreaXmlOutgoing) {
            GMTC_DEBUG("Send jingle message: %s", message.c_str());
        } else {
            switch (level) {
                case LogLevelDebug:
                    GMTC_DEBUG("Area: %d %s ", area, message.c_str());
                    break;
                case LogLevelWarning:
                    GMTC_WARN("Area: %d %s ", area, message.c_str());
                    break;
                case LogLevelError:
                    GMTC_ERROR("Area: %d %s ", area, message.c_str());
                    break;
            }
        }
    }

    void handlePresence(const Presence &presence) override {
        GMTC_DEBUG("%s", presence.tag()->xml().c_str());
    }

    void setOnSdpExchange(XmppConnection::OnSdpExchange cb) {
        _on_sdp_change = cb;
    }

    void setOnSourceChange(XmppConnection::OnSourceChange cb) {
        _on_source_change = cb;
    }

private:
    std::string _ip;
    uint16_t _port;
    JID self_jid;
    JID room_jid;
	std::string _room_secret;
	std::unique_ptr <Client> client;
	std::unique_ptr <MUCRoom> room;
	std::unique_ptr <Jingle::SessionManager> session_mgr;
    XmppConnection::OnSdpExchange _on_sdp_change;
    XmppConnection::OnSourceChange _on_source_change;
};

XmppConnection::XmppConnection(const std::string &ip, uint16_t port, const std::string &room_id,
                               const std::string &room_secret) :
        XmppConnection(randomId(10), ip, port, room_id, room_secret,
                       GMEET_DEFAULT_CONFERENCE_DOMAIN, GMEET_DEFAULT_JOIN_DOMAIN) {
}

XmppConnection::XmppConnection(const std::string &id, const std::string &ip, uint16_t port,
                               const std::string &room_id, const std::string &room_secret,
                               const std::string &conference_domain, const std::string &join_domain) :
        _impl(std::make_unique<XmppConnectionImpl>(id, ip, port, room_id, room_secret, conference_domain, join_domain)),
        _id(id) {
}

XmppConnection::~XmppConnection() {
    XmppConnection::disconnect();
}

bool XmppConnection::connect() {
    return _impl->connect();
}


const string &XmppConnection::getId() const {
    return _id;
}

void XmppConnection::setOnSdpExchange(XmppConnection::OnSdpExchange cb) {
    _impl->setOnSdpExchange(cb);
}

void XmppConnection::setOnSourceChange(XmppConnection::OnSourceChange cb) {
    _impl->setOnSourceChange(cb);
}

bool XmppConnection::disconnect() {
    return _impl->disconnect();
}


} // gmeet