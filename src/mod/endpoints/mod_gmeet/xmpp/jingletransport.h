//
// Created by xiongqimin on 2023/10/17.
//

#ifndef TEST_PROJECT_JINGLETRANSPORT_H
#define TEST_PROJECT_JINGLETRANSPORT_H

#include "jingleplugin.h"

namespace gloox {

namespace Jingle {
/**
     * Describes the candidate type.
     */
enum Type {
    Host,                     /**< A host candidate. */
    PeerReflexive,            /**< A peer reflexive candidate. */
    Relayed,                  /**< A relayed candidate. */
    ServerReflexive           /**< A server reflexive candidate. */
};

std::string to_string(Type type);

/**
 * Describes a single transport candidate.
 */
struct Candidate {
    std::string component;    /**< A Component ID as defined in ICE-CORE. */
    std::string foundation;   /**< A Foundation as defined in ICE-CORE.*/
    std::string generation;   /**< An index, starting at 0, that enables the parties to keep track of
                                         updates to the candidate throughout the life of the session. */
    std::string id;           /**< A unique identifier for the candidate. */
    std::string ip;           /**< The IP address for the candidate transport mechanism. */
    std::string network;      /**< An index, starting at 0, referencing which network this candidate is on for a given peer. */
    int port;                 /**< The port at the candidate IP address. */
    int priority;             /**< A Priority as defined in ICE-CORE. */
    std::string protocol;     /**< The protocol to be used. Should be @b udp. */
	std::string rel_addr{""}; /**< A related address as defined in ICE-CORE. */
	int rel_port{-1};			  /**< A related port as defined in ICE-CORE. */
    Type type;                /**< A Candidate Type as defined in ICE-CORE. */
};

struct Fingerprint {
    std::string hash;
    bool required;
    std::string setup;

    std::string value;
};

/** A list of transport candidates. */
typedef std::list<Candidate> CandidateList;

class TransportExt : public Plugin {

public:

    /**
     * Constructs a new instance.
     * @param pwd The @c pwd value.
     * @param ufrag The @c ufrag value.
     * @param candidates A list of connection candidates.
     */
    TransportExt(const std::string &pwd, const std::string &ufrag, bool rtcp_mux,
                 Fingerprint &fingerprint, CandidateList &candidates);

    /**
     * Constructs a new instance from the given tag.
     * @param tag The Tag to parse.
     */
    TransportExt(const Tag *tag = 0);

    /**
     * Virtual destructor.
     */
    virtual ~TransportExt() {}

    /**
     * Returns the @c pwd value.
     * @return The @c pwd value.
     */
    const std::string &pwd() const { return m_pwd; }

    /**
     * Returns the @c ufrag value.
     * @return The @c ufrag value.
     */
    const std::string &ufrag() const { return m_ufrag; }

    const bool rtcp_mux() const { return m_rtcp_mux; }

    const std::string &websocket_url() const {
        return m_websocket_url;
    }

    const Fingerprint &fingerprint() const {
        return m_fingerprint;
    }

    /**
     * Returns the list of connection candidates.
     * @return The list of connection candidates.
     */
    const CandidateList &candidates() const { return m_candidates; }

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
        return new TransportExt(*this);
    }

private:
    std::string m_pwd;
    std::string m_ufrag;
    bool m_rtcp_mux;
    CandidateList m_candidates;
    std::string m_websocket_url;
    Fingerprint m_fingerprint;
};
}

} // gloox

#endif //TEST_PROJECT_JINGLETRANSPORT_H
