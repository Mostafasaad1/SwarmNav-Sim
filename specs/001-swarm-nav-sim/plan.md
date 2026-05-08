# Implementation Plan: SwarmNav-Sim

**Branch**: `001-swarm-nav-sim` | **Date**: 2026-05-08 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `specs/001-swarm-nav-sim/spec.md`

**Note**: This template is filled in by the `/speckit-plan` command. See `.specify/templates/plan-template.md` for the execution workflow.

## Summary

Build a simulation-only multi-robot system for warehouse exploration. 3-5 robots will collaboratively build a global map using mrg_slam, navigate dynamically without ML using Nav2 TEB and ORCA (RVO2), and allocate exploration tasks via a decentralized Vickrey auction orchestrated by BehaviorTree.CPP.

## Technical Context

**Language/Version**: C++17 / Python 3.10
**Primary Dependencies**: ROS 2 Humble, mrg_slam, Nav2 (nav2_teb_controller), RVO2-ROS2, BehaviorTree.CPP v4
**Storage**: N/A (Simulation-only, map artifacts can be saved to disk)
**Testing**: launch_testing, gtest for C++ nodes, ament_lint
**Target Platform**: Ubuntu 22.04 LTS / NVIDIA Isaac Sim 5.0
**Project Type**: ROS 2 simulation workspace
**Performance Goals**: Real-time factor 1.0, >= 95% map coverage in <= 10 min, SLAM ATE < 0.3m RMSE
**Constraints**: strictly NO machine learning, fully decentralized, 0 collisions
**Scale/Scope**: 3-5 robots, 3 forklifts, 5 human NPCs, 40m x 60m warehouse

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

No constitution violations. (Constitution placeholders initialized, but no specific technical restrictions were violated by this decentralized, ROS 2 standard architectural approach).

## Project Structure

### Documentation (this feature)

```text
specs/001-swarm-nav-sim/
├── plan.md              # This file (/speckit-plan command output)
├── research.md          # Phase 0 output (/speckit-plan command)
├── data-model.md        # Phase 1 output (/speckit-plan command)
├── quickstart.md        # Phase 1 output (/speckit-plan command)
├── contracts/           # Phase 1 output (/speckit-plan command)
└── tasks.md             # Phase 2 output (/speckit-tasks command - NOT created by /speckit-plan)
```

### Source Code (repository root)

```text
swarm_nav_sim/
├── swarm_nav_bringup/          # Launch files, RViz configs, params
├── swarm_nav_slam/             # SLAM configuration and graph merging hooks
├── swarm_nav_navigation/       # Custom TEB layer, ORCA filter node
├── swarm_nav_coordination/     # BT XML, Auction nodes, Frontier detector
├── swarm_nav_msgs/             # Custom messages (Frontier, Obstacle, Auction)
└── swarm_nav_evaluation/       # Python scripts for coverage, collision, ATE metrics
```

**Structure Decision**: The project uses a standard ROS 2 multi-package workspace layout broken down by subsystem (Bringup, SLAM, Navigation, Coordination, Messages, Evaluation) to maintain clean dependency trees and logical separation of concerns.

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| N/A       | N/A        | N/A                                 |
