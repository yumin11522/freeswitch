//
// Created by xiongqimin on 2023/10/23.
//

#ifndef GMEET_CLIENT_JINGLEBRIDGESESSION_H
#define GMEET_CLIENT_JINGLEBRIDGESESSION_H

#include <string>
#include "jingleplugin.h"

namespace gloox {

namespace Jingle {
class BridgeSession : public Plugin {
public:
    BridgeSession(const std::string &id, const std::string &region);

    BridgeSession(const Tag *tag = nullptr);

    // reimplemented from Plugin
    virtual const StringList features() const;

    // reimplemented from Plugin
    virtual const std::string &filterString() const;

    // reimplemented from Plugin
    virtual Tag *tag() const;

    // reimplemented from Plugin
    virtual Plugin *newInstance(const Tag *tag) const;

    // reimplemented from Plugin
    virtual Plugin *clone() const {
        return new BridgeSession(*this);
    }

private:
    std::string m_id;
    std::string m_region;
};

}
} // gloox

#endif //GMEET_CLIENT_JINGLEBRIDGESESSION_H
