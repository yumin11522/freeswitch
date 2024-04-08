//
// Created by xiongqimin on 2023/10/23.
//

#ifndef TEST_PROJECT_SDPUTIL_H
#define TEST_PROJECT_SDPUTIL_H

#include <string>
#include "jinglesession.h"

namespace gloox {
namespace Jingle {

PluginList to_jingle(const std::string &sdp);

std::string to_sdp_str(const Session::Jingle *jingle);

} // Jingle
} // gloox

#endif //TEST_PROJECT_SDPUTIL_H
