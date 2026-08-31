// SPDX-License-Identifier: Apache-2.0
#include "control/control_server.hpp"
#include "test_support.hpp"

void RunControlServerTests() {
  using vrealone::control::Command;
  using vrealone::control::ParseCommand;
  Check(ParseCommand("recenter\n") == Command::recenter,
        "recenter command must parse");
  Check(ParseCommand("click\r\n") == Command::click,
        "click command must parse");
  Check(ParseCommand("status") == Command::status,
        "status command must parse");
  Check(ParseCommand("unknown") == Command::invalid,
        "unknown control command must be rejected");
}
