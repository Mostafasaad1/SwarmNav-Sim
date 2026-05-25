// Copyright 2026 SwarmNav-Sim Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// bt_plugin_registration.cpp
// Single unified BT_REGISTER_NODES entry point for the swarm_bt_nodes library.
//
// Each BT node class is forward-declared and registered here so the linker
// only sees ONE BT_RegisterNodesFromPlugin symbol in the shared library.

#define BT_PLUGIN_EXPORT
#include <behaviortree_cpp/bt_factory.h>

extern void RegisterMapCoverageCheck(BT::BehaviorTreeFactory& factory);
extern void RegisterFrontierDetectorBT(BT::BehaviorTreeFactory& factory);
extern void RegisterRunAuctionBT(BT::BehaviorTreeFactory& factory);

BT_REGISTER_NODES(factory)
{
  RegisterMapCoverageCheck(factory);
  RegisterFrontierDetectorBT(factory);
  RegisterRunAuctionBT(factory);
}
