//
// Created by xiongqimin on 2023/10/23.
//

#include "sdputil.h"

#include <iostream>
#include <sstream>

#include "jinglecontent.h"
#include "jingledescription.h"
#include "jinglegroup.h"
#include "jingletransport.h"
#include "sdptransform.hpp"

using namespace std;

namespace gloox
{

namespace Jingle
{

std::time_t getTimeStamp()
{
	std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp =
		std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());	//��ȡ��ǰʱ���
	std::time_t timestamp = tp.time_since_epoch().count();	//�������1970-1-1,00:00��ʱ�䳤��
	return timestamp;
}

vector<string> split(const string &s, const char *delim)
{
	vector<string> ret;
	size_t last = 0;
	auto index = s.find(delim, last);
	while (index != string::npos) {
		if (index - last > 0) {
			ret.push_back(s.substr(last, index - last));
		}
		last = index + strlen(delim);
		index = s.find(delim, last);
	}
	if (!s.size() || s.size() - last > 0) {
		ret.push_back(s.substr(last));
	}
	return ret;
}

Type to_type(const std::string &type)
{
	if (type == "prflx")
		return PeerReflexive;
	else if (type == "relay")
		return Relayed;
	else if (type == "srflx")
		return ServerReflexive;
	else
		return Host;
}

std::string candidateFromJingle(Candidate &can);
// Candidate toCandidate(const std::string &sdp);

std::string removeQuotes(const std::string &str)
{
	std::string result = str;
	result.erase(std::remove_if(result.begin(), result.end(), [](const char &c) { return c == '"'; }), result.end());
	return result;
}

PluginList to_jingle(const std::string &sdp)
{
	PluginList plugins;
	auto session = sdptransform::parse(sdp);
	if (session.size() <= 0 || session.find("media") == session.end()) {
		return plugins;
	}
	if (session.contains("groups")) {
		auto &group = session.at("groups")[0];
		auto semantics = group.at("type");
		auto mids = group.at("mids");
		std::vector<std::string> names;
		for (auto mid : split(mids, " ")) {
			names.emplace_back(mid);
		}
		Group *group_value = new Group(names, semantics);
		plugins.emplace_back(group_value);
	}

	Fingerprint fp;
	string iceUfrag;
	string icePwd;
	if (session.contains("iceUfrag")) {
		iceUfrag = session.at("iceUfrag");
	}
	if (session.contains("icePwd")) {
		icePwd = session.at("icePwd");
	}
	if (session.contains("fingerprint")) {
		auto fingerprint = session.at("fingerprint");
		fp.value = fingerprint.at("hash");
		fp.hash = fingerprint.at("type");
		fp.setup = "active";
		session.at("setup");
	}
	auto media_itr = session.find("media");
	for (auto m : *media_itr) {
		auto name = m.at("type");
		if (name != "audio" && name != "video") {
			continue;
		}
		//        cout << m.at("ssrcs") << endl;
		string ssrc = "";
		if (m.contains("ssrcs")) {
			ssrc = to_string(m.at("ssrcs")[0].at("id"));
		}

		//        cout << name << endl;
		//        cout << m.at("mid") << endl;

		auto media_type = getMediaType(name);
		auto rtcp_mux = m.contains("rtcpMux");
		auto rtp_pt_itr = m.find("rtp");

		auto has_fmtp = m.contains("fmtp");
		auto has_rtcp_fb = m.contains("rtcpFb");

		PayloadTypeList pt_list;
		for (auto rtp : *rtp_pt_itr) {
			PayloadType pt;
			pt.id = to_string(rtp.at("payload"));
			pt.name = rtp.at("codec");
			pt.clockrate = to_string(rtp.at("rate"));
			if (rtp.contains("encoding")) {
				pt.channels = rtp.at("encoding");
			}

			if (has_rtcp_fb) {
				auto rtcp_fb_itr = m.find("rtcpFb");
				for (auto fb : *rtcp_fb_itr) {
					if (fb.at("payload") == to_string(rtp.at("payload"))) {
						RtcpFb rfb;
						rfb.type = fb.at("type");
						if (fb.contains("subtype")) {
							rfb.subtype = fb.at("subtype");
						}
						pt.rtcp_fbs.emplace_back(move(rfb));
					}
				}
			}

			if (has_fmtp) {
				auto fmtp_itr = m.find("fmtp");
				for (auto fmtp : *fmtp_itr) {
					if (fmtp.at("payload") == rtp.at("payload")) {
						auto cfg_vec = split(fmtp.at("config"), ";");
						for (auto cfg : cfg_vec) {
							auto c = split(cfg, "=");
							if (c.size() == 2) {
								Parameter p;
								p.first = c[0];
								p.second = c[1];
								pt.params.emplace_back(move(p));
							}
						}
					}
				}
			}
			pt_list.emplace_back(move(pt));
		}
		RtpHdrexts hdrexts;
		if (m.contains("ext")) {
			auto ext_itr = m.find("ext");
			for (auto ext : *ext_itr) {
				RtpHdrext ex;
				ex.uri = ext.at("uri");
				ex.id = to_string(ext.at("value"));
				hdrexts.emplace_back(move(ex));
			}
		}
		SourceList src_list;
		if (m.contains("ssrcs")) {
			auto ssrc_itr = m.find("ssrcs");
			for (auto ssrc : *ssrc_itr) {
				if (ssrc.contains("attribute") && "msid" == ssrc.at("attribute")) {
					Source src;
					src.ssrc = to_string(ssrc.at("id"));
					Parameter p;
					p.first = "msid";
					p.second = ssrc.at("value");
					if (p.second.find("- ") != std::string::npos) {
						p.second = p.second.substr(2);
					}
					src.params.emplace_back(move(p));

					src_list.emplace_back(std::move(src));
				}
			}
		}
		SsrcGroup sg;
		if (m.contains("ssrcGroups")) {
			auto ssrc_group = m.at("ssrcGroups")[0];
			sg.semantics = ssrc_group.at("semantics");
			std::string ssrcs = ssrc_group.at("ssrcs");
			for (auto ssrc : split(ssrcs, " ")) {
				sg.ssrcs.emplace_back(ssrc);
			}
		}
		Description *desc = new Description(media_type, ssrc, rtcp_mux, src_list, pt_list, hdrexts, sg);

		auto candidate_itr = m.find("candidates");
		if (m.contains("iceUfrag")) {
			iceUfrag = m.at("iceUfrag");
		}
		if (m.contains("icePwd")) {
			icePwd = m.at("icePwd");
		}
		if (m.contains("fingerprint")) {
			auto fingerprint = m.at("fingerprint");
			fp.value = fingerprint.at("hash");
			fp.hash = fingerprint.at("type");
			fp.setup = "active";
			// m.at("setup");
		}
		CandidateList candidates;
		if (candidate_itr != m.end()) {
			for (auto cand : *candidate_itr) {
				Candidate candidate;
				candidate.component = to_string(cand.at("component"));
				if (candidate.component == "2") {
					continue;
				}
				candidate.foundation = removeQuotes(to_string(cand.at("foundation")));
				candidate.ip = removeQuotes(to_string(cand.at("ip")));
				candidate.port = cand.at("port");
				candidate.priority = cand.at("priority");
				candidate.protocol = removeQuotes(to_string(cand.at("transport")));
				candidate.type = to_type(to_string(cand.at("type")));
				if (cand.contains("generation")) {
					candidate.generation = to_string(cand.at("generation"));
				}
				if (cand.contains("network-id")) {
					candidate.network = to_string(cand.at("network-id"));
				}
				candidates.emplace_back(std::move(candidate));
			}
		}

		TransportExt *trans = new TransportExt(icePwd, iceUfrag, true, fp, candidates);
		PluginList p;
		p.emplace_back(desc);
		p.emplace_back(trans);
		Content *content = new Content(name, p, gloox::Jingle::Content::CInitiator, gloox::Jingle::Content::SResponder);
		plugins.emplace_back(content);
	}
	return plugins;
}

std::string to_sdp_str(const Session::Jingle *jingle)
{
	auto session_id = getTimeStamp();
	std::stringstream ss;
	ss << "v=0\r\n";
	ss << "o=- " << session_id << " 2 IN IP4 220.231.206.186\r\n";
	ss << "s=-\r\n";
	ss << "t=0 0\r\n";

	auto plugins = jingle->plugins();
	if (plugins.empty()) {
		return ss.str();
	}
	// a=group:BUNDLE 0 1 2\r\n
	for (auto plugin : plugins) {
		const Group *group;
		if (plugin && plugin->pluginType() == PluginGroup && (group = static_cast<const Group *>(plugin)) &&
			!group->getNames().empty()) {
			ss << "a=group:" << group->getSemantics() << " ";
			for (auto name : group->getNames()) {
				ss << getMediaType(name) << " ";
			}
			ss << "\r\n";
		}
	}
	ss << "a=msid-semantic:WMS *\r\n";
	ss << "a=ice-lite\r\n";

	for (auto plugin : plugins) {
		const Content *content;
		if (plugin && plugin->pluginType() == PluginContent && (content = static_cast<const Content *>(plugin))) {
			/**
			 * ��ʱֻ���� Audio �� Video Content
			 */
			MediaType media_type = getMediaType(content->name());
			if (media_type != Audio && media_type != Video) {
				continue;
			}
			auto transport = content->findPlugin<TransportExt>(PluginTransport);
			auto desc = content->findPlugin<Description>(PluginDescription);
			if (!transport || !desc) {
				continue;
			}
			// m=audio 9 UDP/TLS/RTP/SAVPF 111 103 104 126 8 0\r\n
            auto media_port = "9";
			auto proto = "UDP/TLS/RTP/SAVPF";
			ss << "m=" << content->name() << " " << media_port << " " << proto << " ";
			auto j = desc->pts().size();
			decltype(j) i = 0;
			for (auto pt : desc->pts()) {
				// ������ PCMU ����
				if (media_type == Audio && pt.id != "0") {
					continue;
				}
				ss << pt.id;
				if (++i <= j - 1) {
					ss << " ";
				}
			}
			ss << "\r\n";

			// sdp += 'c=IN IP4 0.0.0.0\r\n';
            ss << "c=IN IP4 0.0.0.0\r\n";

			// a=ice-ufrag:c9p7c1hdd9uqag\r\n
			ss << "a=ice-ufrag:" << transport->ufrag() << "\r\n";

			// a=ice-pwd:o9ivbqv82j18kgm2sl3obotlo\r\n
			ss << "a=ice-pwd:" << transport->pwd() << "\r\n";

			// a=fingerprint:sha-256
			// 33:5B:D0:EA:9B:F6:92:08:8C:12:BE:D1:83:C2:A4:09:75:46:C1:E3:40:16:1F:BF:D1:98:D7:56:F1:93:8B:2F\r\n
			ss << "a=fingerprint:" << transport->fingerprint().hash << " " << transport->fingerprint().value << "\r\n";
			if (!transport->fingerprint().setup.empty()) {
                ss << "a=setup:" << transport->fingerprint().setup << "\r\n";
			}

			// a=candidate:1 1 udp 2130706431 172.17.0.1 25000 typ host generation 0\r\n
			for (auto can : transport->candidates()) {
				if (can.foundation == "1") {
					continue;
				}
				ss << candidateFromJingle(can);
			}

			// a=mid:0\r\n
			ss << "a=mid:" << getMediaType(content->name()) << "\r\n";
			ss << "a=ice-lite\r\n";
			// a=sendrecv\r\n
			switch (content->senders()) {
				case Content::SInitiator:
					ss << "a=sendonly\r\n";
					break;
				case Content::SResponder:
					ss << "a=recvonly\r\n";
					break;
				case Content::SNone:
					ss << "a=inactive\r\n";
					break;
				case Content::SBoth:
					ss << "a=sendrecv\r\n";
					break;
			}
			// a=rtcp-mux\r\n
			if (desc->rtcp_mux()) {
				//ss << "a=rtcp-mux\r\n";
			}

			for (auto pt : desc->pts()) {
				if (media_type == Audio && pt.id != "0") {
					continue;
				}

				// a=rtpmap:107 H264/90000
				ss << "a=rtpmap:" << pt.id << " " << pt.name << "/" << pt.clockrate;
				if (!pt.channels.empty()) {
					ss << "/" << pt.channels;
				}
				ss << "\r\n";

				// a=fmtp:107 x-google-start-bitrate=800;
				// profile-level-id=42e01f;level-asymmetry-allowed=1;packetization-mode=1;;
				if (!pt.params.empty()) {
					ss << "a=fmtp:" << pt.id << " ";
					for (auto p : pt.params) {
						if (p.first.empty()) {
							ss << p.second;
						} else {
							ss << p.first << "=" << p.second;
						}
						ss << "; ";
					}
					ss << "\r\n";
				}

				// a=rtcp-fb:107 ccm fir\r\n
				if (!pt.rtcp_fbs.empty()) {
					for (auto fb : pt.rtcp_fbs) {
						ss << "a=rtcp-fb:" << pt.id << " " << fb.type;
						if (!fb.subtype.empty()) {
							ss << " " << fb.subtype;
						}
						ss << "\r\n";
					}
				}
			}

			// xep-0294
			// a=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level
			if (!desc->rtp_hdrexts().empty()) {
				for (auto ext : desc->rtp_hdrexts()) {
					ss << "a=extmap:" << ext.id << " " << ext.uri << "\r\n";
				}
			}

			// XEP-0339 handle source attributes
			if (!desc->sources().empty()) {
				for (auto src : desc->sources()) {
					if (!src.params.empty()) {
						for (auto p : src.params) {
							ss << "a=ssrc:" << src.ssrc << " " << p.first;

							if (!p.second.empty()) {
								ss << ":" << p.second << "\r\n";
							}
						}
					}
				}
			}
		}
	}

	return ss.str();
}

std::string candidateFromJingle(Candidate &can)
{
	std::stringstream ss;
	ss << "a=candidate:";

	ss << can.foundation << " ";
	ss << can.component << " ";
	ss << can.protocol << " ";
	ss << can.priority << " ";
	ss << can.ip << " ";
	ss << can.port << " ";
	ss << "typ " << to_string(can.type) << " ";
	switch (can.type) {
		case Type::PeerReflexive:
		case Type::Relayed:
		case Type::ServerReflexive: {
			ss << "raddr"
			   << " " << can.rel_addr << " rport " << can.rel_port << " ";
		}
	}
	ss << "generation " << (can.generation.empty() ? "0" : can.generation);
	ss << "\r\n";
	return ss.str();
}

// Candidate toCandidate(const std::string &sdp) {
//
// }

}  // namespace Jingle
}  // namespace gloox