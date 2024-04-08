//
// Created by xiongqimin on 2023/10/23.
//

#ifndef TEST_PROJECT_JINGLEGROUP_H
#define TEST_PROJECT_JINGLEGROUP_H

#include <vector>
#include "jingleplugin.h"

namespace gloox {

namespace Jingle {

class Group: public Plugin {
public:
    Group(const std::vector<std::string> &names, const std::string &semantics);


    /**
     * Constructs a new instance from the given tag.
     * @param tag The Tag to parse.
     */
    explicit Group(const Tag *tag = 0);

    const std::string &getSemantics() const;

    const std::vector<std::string> &getNames() const;

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
        return new Group(*this);
    }

private:
    std::string m_semantics;
    std::vector<std::string> m_names;
};

} // Jingle

} // gloox

#endif //TEST_PROJECT_JINGLEGROUP_H
