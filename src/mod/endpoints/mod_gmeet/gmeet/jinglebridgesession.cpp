//
// Created by xiongqimin on 2023/10/23.
//

#include "jinglebridgesession.h"

#include "tag.h"

GLOOX_API const std::string XMLNS_PROTOCOL_FOCUS = "http://jitsi.org/protocol/focus";

namespace gloox {
Jingle::BridgeSession::BridgeSession(const std::string &id, const std::string &region)
        : Plugin(PluginBridgeSession), m_id(id), m_region(region) {}

Jingle::BridgeSession::BridgeSession(const Tag *tag) : Plugin(PluginBridgeSession) {
    if (!tag || tag->name() != "bridge-session" || tag->xmlns() != XMLNS_PROTOCOL_FOCUS)
        return;
    m_region = tag->findAttribute("region");
    m_id = tag->findAttribute("id");
}

const StringList Jingle::BridgeSession::features() const {
    return Plugin::features();
}

const std::string &Jingle::BridgeSession::filterString() const {
    static const std::string filter = "jingle/bridge-session[@xmlns='" + XMLNS_PROTOCOL_FOCUS + "']";
    return filter;
}

Tag *Jingle::BridgeSession::tag() const {
    Tag *t = new Tag("bridge-session");
    t->setXmlns(XMLNS_PROTOCOL_FOCUS);
    t->addAttribute("region", m_region);
    t->addAttribute("id", m_id);
    return t;
}

Jingle::Plugin *Jingle::BridgeSession::newInstance(const Tag *tag) const {
    return new Jingle::BridgeSession(tag);
}
} // gloox