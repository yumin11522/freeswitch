//
// Created by xiongqimin on 2023/12/11.
//

#include "jinglepresence.h"

namespace gloox {
namespace Jingle {
Tag *AudioVideoMutePresence::tag() const {
    auto tag_name = _type == Audio ? "audiomuted": "videomuted";
    auto tag = new Tag(tag_name);
    tag->setCData(_mute ? "true": "false");
    return tag;
}

Stanza *AudioVideoMutePresence::embeddedStanza() const {
    return StanzaExtension::embeddedStanza();
}

Tag *AudioVideoMutePresence::embeddedTag() const {
    return StanzaExtension::embeddedTag();
}

const std::string &AudioVideoMutePresence::filterString() const {
    return "";
}

StanzaExtension *AudioVideoMutePresence::newInstance(const Tag *tag) const {
    return nullptr;
}

StanzaExtension *AudioVideoMutePresence::clone() const {
    return nullptr;
}
} // Jingle
} // gloox