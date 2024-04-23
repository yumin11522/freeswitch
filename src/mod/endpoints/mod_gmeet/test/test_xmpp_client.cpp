/*
 *  Copyright (c) 2004-2023 by Jakob Schr?ter <js@camaya.net>
 *  This file is part of the gloox library. http://camaya.net/gloox
 *
 *  This software is distributed under a license. The full license
 *  agreement can be found in the file LICENSE in this distribution.
 *  This software may not be copied, modified, sold or distributed
 *  other than expressed in the named license agreement.
 *
 *  This software is distributed without any warranty.
 */
#define GMTC_CLASS "test_xmpp_client"

#include "xmpp/logger.h"
#include "xmpp/xmppclientmgr.h"

using namespace std;

#include <stdio.h>

int main(int argc /*argc*/, char **argv /*argv*/)
{
	xmppclient::Log::SetDefaultHandler();
	xmppclient::Log::SetLogLevel(xmppclient::Log::LLevel::LOG_DEBUG);
	xmppclient::XmppClientMgr::setXmppSrvHost("10.8.106.128");
	xmppclient::XmppClientMgr::setXmppSrvPort(6222);

	auto client = xmppclient::XmppClientMgr::Instance().addClient("88888888", "");
	client->setAnswer(R"(v=0
o=FreeSWITCH 1713131336 1713131338 IN IP4 10.8.6.227
s=FreeSWITCH
c=IN IP4 10.8.6.227
t=0 0
a=msid-semantic: WMS LQOzT8y5U9NIWdefzhiSerYrFgRLZ3tn
m=audio 22374 UDP/TLS/RTP/SAVPF 0 126
a=rtpmap:0 PCMU/8000
a=rtpmap:126 telephone-event/8000
a=silenceSupp:off - - - -
a=ptime:20
a=sendrecv
a=fingerprint:sha-256 8E:D5:25:B3:1E:0F:59:4D:3D:C3:78:31:F1:8A:AB:BE:F6:BB:83:76:D9:D5:23:13:BD:E2:DF:34:32:D5:2C:F3
a=setup:active
a=rtcp-mux
a=rtcp:22374 IN IP4 10.8.6.227
a=ice-ufrag:WdoGjDnnt746eT1W
a=ice-pwd:G0GriCAwTxJ9gb5tshrIrrKO
a=candidate:0247068888 1 udp 2130706431 10.8.6.227 22374 typ host generation 0
a=candidate:0247068888 2 udp 2130706430 10.8.6.227 22374 typ host generation 0
a=end-of-candidates
a=ssrc:3122444246 cname:GAco5mJTrSjnpQ5Y
a=ssrc:3122444246 msid:LQOzT8y5U9NIWdefzhiSerYrFgRLZ3tn a0
a=ssrc:3122444246 mslabel:LQOzT8y5U9NIWdefzhiSerYrFgRLZ3tn
a=ssrc:3122444246 label:LQOzT8y5U9NIWdefzhiSerYrFgRLZ3tna0
m=video 22376 UDP/TLS/RTP/SAVPF 107
b=AS:2048
a=rtpmap:107 H264/90000
a=fmtp:107 x-google-start-bitrate=8000; profile-level-id=42e01f;level-asymmetry-allowed=1;packetization-mode=1;;
a=sendrecv
a=fingerprint:sha-256 8E:D5:25:B3:1E:0F:59:4D:3D:C3:78:31:F1:8A:AB:BE:F6:BB:83:76:D9:D5:23:13:BD:E2:DF:34:32:D5:2C:F3
a=setup:active
a=rtcp-mux
a=rtcp:22376 IN IP4 10.8.6.227
a=rtcp-fb:107 ccm fir
a=rtcp-fb:107 nack
a=rtcp-fb:107 nack pli
a=ssrc:2265889111 cname:GAco5mJTrSjnpQ5Y
a=ssrc:2265889111 msid:LQOzT8y5U9NIWdefzhiSerYrFgRLZ3tn v0
a=ssrc:2265889111 mslabel:LQOzT8y5U9NIWdefzhiSerYrFgRLZ3tn
a=ssrc:2265889111 label:LQOzT8y5U9NIWdefzhiSerYrFgRLZ3tnv0
a=ice-ufrag:WdoGjDnnt746eT1W
a=ice-pwd:G0GriCAwTxJ9gb5tshrIrrKO
a=candidate:5359429619 1 udp 2130706431 10.8.6.227 22376 typ host generation 0
a=candidate:5359429619 2 udp 2130706430 10.8.6.227 22376 typ host generation 0
a=end-of-candidates
)");
	client->doconnect();

	GMTC_DEBUG("------------------------------");

	std::getchar();

	client->disconnect();
	xmppclient::XmppClientMgr::Instance().removeClient(client->id());
	return 0;
}