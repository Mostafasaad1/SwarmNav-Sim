<!--
  SYNC IMPACT REPORT
  Version change: 0.0.0 (empty template) → 1.0.0
  Modified sections:
    - Core Principles: 5 principles filled from template placeholders
    - Development Environment & Tooling: filled from template placeholder
    - Quality Gates & Workflow: filled from template placeholder
    - Governance: filled from template placeholder
  Added sections: none (all existed as templates)
  Removed sections: none
  Templates requiring updates:
    - .specify/templates/spec-template.md ✅ reviewed — no constitution-specific references
    - .specify/templates/plan-template.md ✅ reviewed — "Constitution Check" section already generic
    - .specify/templates/tasks-template.md ✅ reviewed — test-first language aligns with Principle III
    - .specify/templates/commands/ — directory does not exist; no review needed
    - AGENTS.md ✅ reviewed — no principle references
    - README.md ✅ reviewed — no principle references
  Deferred TODOs: none
-->

# SwarmNav-Sim Constitution

## Core Principles

### I. ROS 2 & Simulation-First

All components MUST integrate with the ROS 2 Jazzy ecosystem. Nodes
communicate via ROS 2 topics, services, and actions. Custom message types
MUST be defined in dedicated interface packages.

- Nodes MUST use ROS 2 typed interfaces (topics/services/actions)
- Custom messages MUST live in a dedicated package (e.g., `swarm_nav_msgs`)
- Simulation environments MUST be reproducible via launch files
- Gazebo Fortress or NVIDIA Isaac Sim are the supported simulators
- BehaviorTree.CPP v4 MUST be used for coordination logic
- Nav2 costmap plugins MUST be used for navigation layers

### II. Spec-Driven Development

Every feature starts with a specification in `.specify/`. Requirements are
documented as user stories with acceptance criteria and independent test
descriptions.

- Specifications MUST be written before implementation begins
- Each spec MUST contain user stories, functional requirements, and success
  criteria
- Implementation plans MUST include a research phase, design phase, and task
  breakdown
- Plans MUST pass the Constitution Check gate before proceeding

### III. Test-First (NON-NEGOTIABLE)

TDD is mandatory: tests MUST be written and reviewed before implementation
begins. The Red-Green-Refactor cycle MUST be strictly followed.

- Tests MUST be written and MUST fail before any implementation code is written
- All unit tests MUST pass via `colcon test` before merge
- Integration tests are REQUIRED for cross-package contracts
- Coverage goals: all new packages MUST have unit tests; critical paths need
  integration tests
- Linting (flake8, pep257, uncrustify) MUST pass

### IV. Modular Architecture

Each robotic capability (SLAM, navigation, coordination, evaluation) lives in
its own ROS 2 package with clearly defined interfaces.

- Packages MUST be independently buildable via `colcon`
- Inter-package communication MUST use ROS 2 messages/services only
- No circular dependencies between packages
- Each package MUST have a single well-defined responsibility
- Shared message types belong in the `swarm_nav_msgs` interface package

### V. Performance & Observability

The system MUST meet defined performance targets. All nodes MUST produce
structured logs. System metrics MUST be collected during evaluation.

- Coverage MUST reach ≥95% within ≤10 minutes
- SLAM ATE MUST be <0.3m RMSE
- Zero collisions MUST be maintained
- Structured logging required for all ROS 2 nodes
- Benchmark suite with CPU/Memory metrics MUST be run before release
- Parameter sensitivity and Bayesian optimization results MUST be documented

## Development Environment & Tooling

The project targets the following environment:

- **OS**: Ubuntu 22.04 LTS
- **ROS 2**: Jazzy Jalisco (desktop install)
- **Build System**: colcon
- **Languages**: Python 3.10+ (nodes), C++17 (performance-critical paths)
- **Simulation**: Gazebo Fortress or NVIDIA Isaac Sim 5.0
- **Behavior Trees**: BehaviorTree.CPP v4
- **Navigation**: Nav2 costmap plugins

All developers MUST use the same environment to ensure reproducibility.
Deviations MUST be documented in the feature specification.

## Quality Gates & Workflow

Every contribution MUST pass the following gates:

1. **Lint Gate**: flake8, pep257, uncrustify — zero warnings
2. **Test Gate**: All unit tests pass via `colcon test`
3. **Review Gate**: Code review required for all PRs
4. **Benchmark Gate**: Benchmark suite runs without regression before release
5. **Quickstart Gate**: Quickstart guide MUST be validated for new features

PRs that introduce complexity without justification (per YAGNI) MUST be
rejected. Simpler solutions MUST be preferred unless demonstrably inadequate.

## Governance

- This constitution supersedes all other development practices and guidelines
- Amendments require: documented proposal → team review → migration plan →
  approval
- Amendment proposals MUST be filed in `.specify/` alongside affected features
- All PRs and reviews MUST verify compliance with constitution principles
- Complexity MUST be justified: every violation of "start simple" requires an
  explicit rationale
- Versioning follows Semantic Versioning for governance changes:
  - MAJOR: backward-incompatible principle removals or redefinitions
  - MINOR: new principles or materially expanded guidance
  - PATCH: clarifications, wording fixes, non-semantic refinements
- Use `AGENTS.md` for runtime development guidance and tool-specific notes
- Compliance is reviewed as part of the standard PR review process

**Version**: 1.0.0 | **Ratified**: 2026-05-09 | **Last Amended**: 2026-05-22
