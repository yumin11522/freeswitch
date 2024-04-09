//
// Created by xiongqimin on 2023/12/11.
//

#ifndef GMEET_CLIENT_JINGLEPRESENCE_H
#define GMEET_CLIENT_JINGLEPRESENCE_H


#include "stanza.h"
#include "jingledescription.h"

namespace gloox {
namespace Jingle {

class AudioVideoMutePresence : public StanzaExtension {
public:
    AudioVideoMutePresence(MediaType type, bool mute): _type(type), _mute(mute), StanzaExtension(mute) {}

    ~AudioVideoMutePresence() override {
    }

    virtual Tag *tag() const override;

    Stanza *embeddedStanza() const override;

    Tag *embeddedTag() const override;

    const std::string &filterString() const override;

    StanzaExtension *newInstance(const Tag *tag) const override;

    StanzaExtension *clone() const override;

private:
    MediaType _type;
    bool _mute;

};

} // Jingle
} // gloox
#endif //GMEET_CLIENT_JINGLEPRESENCE_H
