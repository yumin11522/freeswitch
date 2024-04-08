//
// Created by xiongqimin on 2023/10/16.
//

#ifndef TEST_PROJECT_JINGLEDESCRIPTION_H
#define TEST_PROJECT_JINGLEDESCRIPTION_H

#include "jingleplugin.h"

namespace gloox {
namespace Jingle {

static const char *mediaTypeValues[] = {
        "audio",
        "video",
        "data"
};

enum MediaType {
    Audio = 0,
    Video,
    Data
};

inline MediaType getMediaType(const std::string &name) {
    return static_cast<MediaType>( util::lookup(name, mediaTypeValues));
}

inline std::string to_string(MediaType type) {
    return util::lookup(type, mediaTypeValues);
}

struct RtcpFb {
    std::string subtype;
    std::string type;
};

struct RtpHdrext {
    std::string uri;
    std::string id;
};

using Parameter = std::pair<std::string, std::string>;

using PtParams = std::list<Parameter>;
using RtcpFbs = std::list<RtcpFb>;
using RtpHdrexts = std::list<RtpHdrext>;

struct PayloadType {
    std::string id;
    std::string name;
    std::string clockrate;
    std::string channels;

    PtParams params;

    RtcpFbs rtcp_fbs;
};

struct Source {
    std::string ssrc;
    std::string name;
    std::string ssrc_info_owner;
    PtParams params;
};
struct SsrcGroup {
    std::string semantics;
    std::list<std::string> ssrcs;
};

using PayloadTypeList = std::list<PayloadType>;
using SourceList = std::list<Source>;

class Description : public Plugin {
public:

    Description(MediaType type, const std::string &ssrc, bool rtcp_mux, const SourceList &sources, const PayloadTypeList &pts,
                const RtpHdrexts &hdrs, const SsrcGroup &sg);

    /**
     * Constructs a new instance from the given tag.
     * @param tag The Tag to parse.
     */
    explicit Description(const Tag *tag = 0);

    MediaType type() const;

    const std::string &ssrc() const;

    bool rtcp_mux() const;

    const SourceList &sources() const;

    const PayloadTypeList &pts() const;

    const RtpHdrexts &rtp_hdrexts() const;

    const SsrcGroup &ssrc_group() const;

    /**
     * Virtual destructor.
     */
    virtual ~Description() {}

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
        return new Description(*this);
    }

private:
    MediaType m_type;
    std::string m_ssrc;
    bool m_rtcp_mux;
    SourceList m_sources;
    PayloadTypeList m_pts;
    RtpHdrexts m_rtp_hdrexts;
    SsrcGroup m_source_group;
};

} // Jingle

} // gloox

#endif //TEST_PROJECT_JINGLEDESCRIPTION_H
