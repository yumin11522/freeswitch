//
// Created by xiongqimin on 2023/10/23.
//

#include "jinglegroup.h"

#include "tag.h"

namespace gloox {

namespace Jingle {

GLOOX_API const std::string XMLNS_JINGLE_APPS_GROUPING = "urn:xmpp:jingle:apps:grouping:0";

Group::Group(const std::vector<std::string> &names, const std::string &semantics) : Plugin(PluginGroup), m_names(names),
                                                                                    m_semantics(semantics) {}

Jingle::Group::Group(const Tag *tag) : Plugin(PluginGroup) {
    if (!tag || tag->name() != "group" || tag->xmlns() != XMLNS_JINGLE_APPS_GROUPING)
        return;
    m_semantics = tag->findAttribute("semantics");
    const TagList contents = tag->findChildren("content");
    TagList::const_iterator it = contents.begin();
    for (; it != contents.end(); ++it) {
        m_names.emplace_back((*it)->findAttribute("name"));
    }
}

const StringList Jingle::Group::features() const {
    return Plugin::features();
}

const std::string &Jingle::Group::filterString() const {
    static const std::string filter = "jingle/group[@xmlns='" + XMLNS_JINGLE_APPS_GROUPING + "']";
    return filter;
}

Tag *Jingle::Group::tag() const {
    Tag *t = new Tag("group");
    t->setXmlns(XMLNS_JINGLE_APPS_GROUPING);
    t->addAttribute("semantics", "BUNDLE");

    if (!m_names.empty()) {
        for (auto content: m_names) {
            auto c = new Tag(t, "content");
            c->addAttribute("name", content);
        }
    }
    return t;
}

Jingle::Plugin *Jingle::Group::newInstance(const Tag *tag) const {
    return new Group(tag);
}

const std::string &Group::getSemantics() const {
    return m_semantics;
}

const std::vector<std::string> &Group::getNames() const {
    return m_names;
}
} // Jingle
} // gloox