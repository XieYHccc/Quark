# Quark Agent Entry

Before planning or editing in this repository, read:
- `.agents/README.md`
- `.agents/skills/README.md`

Then, as needed:
- relevant task files under `.agents/tasks`, if they exist
- relevant files under `.agents/knowledge`

## Project Overview

Quark is a personal game engine for learning and personal use. The goal
is to understand game engine architecture and to implement frontier
techniques by building them from scratch.

**Tech Stack**: C++20, CMake, Vulkan

## Architecture Overview

Engine code lives in `Quark/` (namespace `quark`). Layered bottom-up:

- `Core/` — base types, logging, assert, math, job system, application/window.
- `Platform/` — OS detection and platform glue.
- `Events/` — event manager and event types.
- `RHI/` — graphic API abstraction. Backend in `RHI/Vulkan/`, selected by the
  `USE_VULKAN_DRIVER` macro (`Core/Base.h`).
- `Render/` — renderer built on top of `RHI`.
- `Ecs/` + `Scene/` — entity-component model, scene graph, and serialization.
- `Asset/` + `Project/` — asset management and project load/save.
- `Animation/`, `Physics/`, `UI/` — feature subsystems.

Consumers of the engine:

- `Editor/` — main editor application (`QuarkEditor`, VS startup project).
- `Tests/` — standalone test executables (e.g. `JobSystem_Test`).
- `Examples/`, `SandboxProject/` — sample clients.
- `ThirdParty/` — git submodules (Vulkan SDK required, plus `glslangValidator`
  for compiling shaders in `BuiltInResources/`).

Include conventions:

- `Quark/qkpch.h` is the precompiled header (common STL + `Core/Base|Logger|Assert`).
- `Quark/Quark.h` is the client-facing umbrella header; never include it inside
  the engine codebase.


## Rules
- Preserve unrelated user changes in the worktree. 
- Prefer dry-run proposals before project content writes.
- Promote repeated stable facts to `ai/knowledge/`.
- Promote repeated workflows to skills.

## Verification
Use the smallest command set that proves the change. Common checks include:

- Configure (pick preset matching host/generator):
  - `cmake --preset <name>`
- Build:
  - `cmake --build --preset <name>` (build preset name matches the configure
    preset, except `windows-vs` builds use `windows-vs-debug`/`windows-vs-release`)
- Build single target (faster feedback): `cmake --build build/<name> --target quark`
- Run a test executable after build, e.g. `./build/macos-ninja-debug/Tests/JobSystem_Test`

Available configure presets: `macos-make-debug`, `macos-make-release`,
`macos-ninja-debug`, `macos-ninja-release`, `windows-vs`,
`windows-ninja-debug`, `windows-ninja-release`.
