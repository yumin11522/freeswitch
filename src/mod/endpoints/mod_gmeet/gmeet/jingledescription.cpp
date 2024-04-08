//
// Created by xiongqimin on 2023/10/16.
//

#include "jingledescription.h"

#include "tag.h"

GLOOX_API const std::string XMLNS_JINGLE_DESCRIPTION = "urn:xmpp:jingle:apps:rtp:1";

namespace gloox {
namespace Jingle {

Description::Description(MediaType type, const std::string &ssrc, bool rtcp_mux, const SourceList &sources,
                         const PayloadTypeList &pts, const RtpHdrexts &hdrs, const SsrcGroup &sg) :
        Plugin(PluginDescription), m_type(type), m_ssrc(ssrc), m_rtcp_mux(rtcp_mux), m_sources(sources), m_pts(pts),
        m_rtp_hdrexts(hdrs), m_source_group(sg){}

Description::Description(const Tag *tag) : Plugin(PluginDescription) {
    if (!tag || tag->name() != "description" || tag->xmlns() != XMLNS_JINGLE_DESCRIPTION)
        return;
    m_type = getMediaType(tag->findAttribute("media"));
    m_ssrc = tag->findAttribute("ssrc");
    TagList pt_list = tag->findChildren("payload-type");
    TagList::const_iterator it = pt_list.begin();
    for (; it != pt_list.end(); ++it) {
        PayloadType c;
        c.id = (*it)->findAttribute("id");
        c.name = (*it)->findAttribute("name");
        c.clockrate = (*it)->findAttribute("clockrate");
        c.channels = (*it)->findAttribute("channels");

        TagList params_list = (*it)->findChildren("parameter");
        TagList::const_iterator param_it = params_list.begin();
        for (; param_it != params_list.end(); ++param_it) {
            Parameter p;
            p.first = (*param_it)->findAttribute("name");
            p.second = (*param_it)->findAttribute("value");
            c.params.emplace_back(std::move(p));
        }
        TagList rtcp_fb_list = (*it)->findChildren("rtcp-fb");
        TagList::const_iterator rtcp_fb_it = rtcp_fb_list.begin();
        for (; rtcp_fb_it != rtcp_fb_list.end(); ++rtcp_fb_it) {
            RtcpFb rtcp;
            rtcp.type = (*rtcp_fb_it)->findAttribute("type");
            rtcp.subtype = (*rtcp_fb_it)->findAttribute("subtype");
            c.rtcp_fbs.emplace_back(std::move(rtcp));
        }
        m_pts.emplace_back(std::move(c));
    }
    m_rtcp_mux = tag->hasChild("rtcp-mux");

    TagList src_list = tag->findChildren("source");
    TagList::const_iterator src_it = src_list.begin();
    for (; src_it != src_list.end(); ++src_it) {
        Source src;
        src.ssrc = (*src_it)->findAttribute("ssrc");
        src.name = (*src_it)->findAttribute("name");
        TagList source_list = (*src_it)->findChildren("parameter");
        TagList::const_iterator sit = source_list.begin();
        for (; sit != source_list.end(); ++sit) {
            Parameter p;
            p.first = (*sit)->findAttribute("name");
            p.second = (*sit)->findAttribute("value");
            src.params.emplace_back(std::move(p));
        }
        m_sources.emplace_back(std::move(src));
    }

    TagList rtp_hdrext_list = tag->findChildren("rtp-hdrext");
    TagList::const_iterator hdrext_it = rtp_hdrext_list.begin();
    for (; hdrext_it != rtp_hdrext_list.end(); ++hdrext_it) {
        RtpHdrext c;
        c.uri = (*hdrext_it)->findAttribute("uri");
        c.id = (*hdrext_it)->findAttribute("id");
        m_rtp_hdrexts.emplace_back(std::move(c));
    }

    auto sg = tag->findChild("ssrc-group");
    if (sg) {
        m_source_group.semantics = sg->findAttribute("semantics");
        TagList sg_ssrc_list = sg->findChildren("source");
        TagList::const_iterator sg_ssrc_it = sg_ssrc_list.begin();
        for (; sg_ssrc_it != sg_ssrc_list.end(); ++sg_ssrc_it) {
            m_source_group.ssrcs.emplace_back((*sg_ssrc_it)->findAttribute("ssrc"));
        }
    }
}

const StringList Description::features() const {
    StringList sl;
    sl.push_back(XMLNS_JINGLE_DESCRIPTION);
    return sl;
}

const std::string &Description::filterString() const {
    static const std::string filter = "content/description[@xmlns='" + XMLNS_JINGLE_DESCRIPTION + "']";
    return filter;
}


Tag *Description::tag() const {
    Tag *t = new Tag("description");
    t->setXmlns(XMLNS_JINGLE_DESCRIPTION);
    t->addAttribute("media", m_type == Audio ? "audio" : (m_type == Video ? "video" : "data"));
    t->addAttribute("ssrc", m_ssrc);
    if (!m_pts.empty()) {
        for (auto pt: m_pts) {
            auto pte = new Tag(t, "payload-type");
            pte->addAttribute("id", pt.id);
            pte->addAttribute("name", pt.name);
            pte->addAttribute("clockrate", pt.clockrate);
            if (!pt.channels.empty()) {
                pte->addAttribute("channels", pt.channels);
            }
            if (!pt.rtcp_fbs.empty()) {
                for (auto fb: pt.rtcp_fbs) {
                    auto ptep = new Tag(pte, "rtcp-fb");
                    ptep->setXmlns("urn:xmpp:jingle:apps:rtp:rtcp-fb:0");
                    ptep->addAttribute("type", fb.type);
                    if (!fb.subtype.empty()) {
                        ptep->addAttribute("subtype", fb.subtype);
                    }
                }
            }
            if (!pt.params.empty()) {
                for (auto param: pt.params) {
                    auto ptep = new Tag(pte, "parameter");
                    ptep->addAttribute("name", param.first);
                    ptep->addAttribute("value", param.second);
                }
            }
        }
    }
    if (m_rtcp_mux) {
        new Tag(t, "rtcp-mux");
    }

    for (auto s: m_sources) {
        auto source = new Tag(t, "source");
        source->setXmlns("urn:xmpp:jingle:apps:rtp:ssma:0");
        source->addAttribute("ssrc", s.ssrc);
        source->addAttribute("name", s.name);
        if (!s.params.empty()) {
            for (auto param: s.params) {
                auto p = new Tag(source, "parameter");
                p->setXmlns("urn:xmpp:jingle:apps:rtp:1");
                p->addAttribute("name", param.first);
                p->addAttribute("value", param.second);
            }
        }
    }

    if (!m_rtp_hdrexts.empty()) {
        for (auto hdr: m_rtp_hdrexts) {
            auto hdr_ext = new Tag(t, "rtp-hdrext");
            hdr_ext->setXmlns("urn:xmpp:jingle:apps:rtp:rtp-hdrext:0");
            hdr_ext->addAttribute("uri", hdr.uri);
            hdr_ext->addAttribute("id", hdr.id);
        }
    }

    if (!m_source_group.semantics.empty()) {
        auto ssrc_grp = new Tag(t, "ssrc-group");
        ssrc_grp->setXmlns("urn:xmpp:jingle:apps:rtp:ssma:0");
        ssrc_grp->addAttribute("semantics", m_source_group.semantics);
        for (auto ssrc: m_source_group.ssrcs) {
            auto s = new Tag(ssrc_grp, "source");
            s->addAttribute("ssrc", ssrc);
        }
    }

    return t;
}

Plugin *Description::newInstance(const Tag *tag) const {
    return new Description(tag);
}

MediaType Description::type() const {
    return m_type;
}

const std::string &Description::ssrc() const {
    return m_ssrc;
}

bool Description::rtcp_mux() const {
    return m_rtcp_mux;
}

const SourceList &Description::sources() const {
    return m_sources;
}

const PayloadTypeList &Description::pts() const {
    return m_pts;
}

const RtpHdrexts &Description::rtp_hdrexts() const {
    return m_rtp_hdrexts;
}


const SsrcGroup &Description::ssrc_group() const {
    return m_source_group;
}

} // Jingle
} // gloox