//
// Created by xiongqimin on 2023/10/17.
//

#include "jingletransport.h"

#include "tag.h"

namespace gloox {

namespace Jingle {
static const char *typeValues[] = {
        "host",
        "prflx",
        "relay",
        "srflx"
};

std::string to_string(Type type) {
    return util::lookup(type, typeValues);
}

TransportExt::TransportExt(const std::string &pwd, const std::string &ufrag, bool rtcp_mux,
                           Fingerprint &fingerprint, CandidateList &candidates)
        : Plugin(PluginTransport), m_pwd(pwd), m_ufrag(ufrag), m_rtcp_mux(rtcp_mux),
          m_fingerprint(fingerprint), m_candidates(candidates) {
}

TransportExt::TransportExt(const Tag *tag)
        : Plugin(PluginTransport) {
    if (!tag || tag->name() != "transport" || tag->xmlns() != XMLNS_JINGLE_ICE_UDP)
        return;

    m_pwd = tag->findAttribute("pwd");
    m_ufrag = tag->findAttribute("ufrag");
    const TagList candidates = tag->findChildren("candidate");
    TagList::const_iterator it = candidates.begin();
    for (; it != candidates.end(); ++it) {
        Candidate c;
        c.component = (*it)->findAttribute("component");
        c.foundation = (*it)->findAttribute("foundation");
        c.generation = (*it)->findAttribute("generation");
        c.id = (*it)->findAttribute("id");
        c.ip = (*it)->findAttribute("ip");
        c.network = (*it)->findAttribute("network");
        c.port = atoi((*it)->findAttribute("port").c_str());
        c.priority = atoi((*it)->findAttribute("priority").c_str());
        c.protocol = (*it)->findAttribute("protocol");
        c.rel_addr = (*it)->findAttribute("rel-addr");
        c.rel_port = atoi((*it)->findAttribute("rel-port").c_str());
        c.type = static_cast<Type>( util::lookup((*it)->findAttribute("type"), typeValues));
        m_candidates.push_back(c);
    }

    // <rtcp-mux/>
    m_rtcp_mux = tag->findChild("rtcp-mux") != nullptr;
    // <web-socket xmlns='http://jitsi.org/protocol/colibri' url='wss://10.8.106.125:1443/colibri-ws/default-id/809d83687ccaea3b/SIP_e717jap0?pwd=5onks2nahj3l45lig6l09qcgb5'/>
    auto web_socket = tag->findChild("web-socket");
    if (web_socket) {
        m_websocket_url = web_socket->findAttribute("url");
    }

    /**
     * <fingerprint hash='sha-256' required='false' xmlns='urn:xmpp:jingle:apps:dtls:0' setup='actpass'>
          65:9C:06:60:16:56:B8:F1:98:54:19:5E:F6:79:BF:93:35:02:0F:30:34:7B:7D:08:82:B5:12:95:A3:84:1C:1C
        </fingerprint>
     */
    auto fingerprint_ele = tag->findChild("fingerprint");
    if (fingerprint_ele) {
        m_fingerprint.hash = fingerprint_ele->findAttribute("hash");
        m_fingerprint.required = ("true" == fingerprint_ele->findAttribute("required"));
        m_fingerprint.setup = fingerprint_ele->findAttribute("setup");
        m_fingerprint.value = fingerprint_ele->cdata();
    }
}

const StringList TransportExt::features() const {
    StringList sl;
    sl.push_back(XMLNS_JINGLE_ICE_UDP);
    sl.push_back("urn:xmpp:jingle:apps:rtp:audio");
    sl.push_back("urn:xmpp:jingle:apps:rtp:video");
    sl.push_back("urn:xmpp:jingle:apps:dtls:0");
    return sl;
}

const std::string &TransportExt::filterString() const {
    static const std::string filter = "content/transport[@xmlns='" + XMLNS_JINGLE_ICE_UDP + "']";
    return filter;
}

Plugin *TransportExt::newInstance(const Tag *tag) const {
    return new TransportExt(tag);
}

Tag *TransportExt::tag() const {
    Tag *t = new Tag("transport");
    t->setXmlns(XMLNS_JINGLE_ICE_UDP);
    t->addAttribute("pwd", m_pwd);
    t->addAttribute("ufrag", m_ufrag);

    Tag *wt = new Tag(t, "web-socket");
    wt->setXmlns("http://jitsi.org/protocol/colibri");
    wt->addAttribute("url", m_websocket_url);

    if (m_rtcp_mux) {
        new Tag(t, "rtcp-mux");
    }

    Tag *fp = new Tag(t, "fingerprint");
    fp->setXmlns("urn:xmpp:jingle:apps:dtls:0");
    fp->addAttribute("hash", m_fingerprint.hash);
    fp->addAttribute("required", m_fingerprint.required ? "true" : "false");
    fp->addAttribute("setup", m_fingerprint.setup);
    fp->setCData(m_fingerprint.value);

    CandidateList::const_iterator it = m_candidates.begin();
    for (; it != m_candidates.end(); ++it) {
        // a=candidate <foundation> <component-id> <transport> <priority> <connection-address> typ <candidate-types> <rel-addr> <rel-port>
        Tag *c = new Tag(t, "candidate");
        c->addAttribute("component", (*it).component);
        c->addAttribute("foundation", (*it).foundation);
		/*if ((*it).generation != "0") {
            c->addAttribute("generation", (*it).generation);
		
        }*/
        c->addAttribute("id", (*it).id);
        c->addAttribute("ip", (*it).ip);
        c->addAttribute("network", (*it).network);
        c->addAttribute("port", (*it).port);
        c->addAttribute("priority", (*it).priority);
        c->addAttribute("protocol", (*it).protocol);
		if (!(*it).rel_addr.empty()) {
			c->addAttribute("rel-addr", (*it).rel_addr);
		}
		if ((*it).rel_port > 0) {
			c->addAttribute("rel-port", (*it).rel_port);
		}
        c->addAttribute("type", util::lookup((*it).type, typeValues));
    }

    return t;
}
} // Jingle

} // gloox