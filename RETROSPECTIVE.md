# Retrospective

## AI tools used

- **Claude Code** (Anthropic's agentic CLI), driving **Claude Opus 4.8 /
  Sonnet** — the primary development driver: edited source directly, ran the
  CMake/Clang and Emscripten builds, read compiler/linker output, and iterated.
- **Gemini** — used as a second opinion on specific tasks, notably texture
  mapping, where it did noticeably better than Claude (see below).

## Development workflow

The project was driven through the agent in roughly these phases:

1. **Brief task statement** — describe the goal in a few sentences (a Game of
   Life running on a hex-tiled sphere, native + web).
2. **Library selection** — pick the dependencies with the agent (OpenMesh for
   the mesh, RayLib for rendering, Emscripten for the web build).
3. **Brief architecture sketch** — outline the main entities and how they split
   (geometry / simulation / rendering / UI).
4. **Iterative implementation** — build each piece by issuing commands to the
   agent; nearly all code was written this way, not by hand.
5. **Build & visual verification** — rebuild after each change to confirm it
   compiles, then check the result on screen (I describe what I see, since the
   agent can't observe the rendered output) and feed corrections back.
6. **Refactor & document** — consolidate the grown-organically code into clean
   entities (e.g. `SimControls`) and generate the docs from the actual code.

## What worked well

- **Geometry and math.** Solves geometric problems well and is familiar with
  most of the math and rendering libraries used here (mesh topology, dual-mesh
  construction, camera/projection math, RayLib/OpenMesh APIs).
- **Refactoring.** The initial architecture was weak — no upfront split into
  functional entities — but the agent refactors that into clean structure
  (e.g. extracting `SimControls`) **without breaking functionality**.
- **Environment and tooling.** Very good at IDE configuration, config/build
  scripts, and setting up the toolchain (kits, presets, WASM environment).
- **HTML/JS UI.** Building the web UI in HTML/CSS/JS went an order of magnitude
  better than the in-engine RayLib UI (see below): the AI produced a clean,
  working control panel wired to the WASM engine with far fewer layout, font,
  and styling issues.

## What did not work well

- **Fundamental mesh mistakes.** Made conceptual errors working with the scene
  mesh — e.g. trying to apply textures onto dynamically recomputed vertices. The
  initial rendering code was also non-optimal, recomputing full state every frame
  (both Opus and Sonnet).
- **Texture mapping.** Claude (Opus) struggled with it specifically; **Gemini
  handled it noticeably better.**
- **RayLib UI generation.** Many errors building the immediate-mode UI — layout
  bugs, font sizing/blur, contrast, and similar.
- **Hardcoding.** Frequently reaches for hardcoded values instead of
  parameterizing.

## Surprises and discoveries

- **Build and link diagnosis is fast.** Finds errors when building and linking
  against external libraries surprisingly quickly — this was the smoothest part
  of working with the C++/CMake/Emscripten toolchain.

## Estimated percentage of AI-generated code

**90%+.** Essentially all code writing was done deliberately through commands to
the agent. My contribution was direction, visual QA, and design decisions, plus
supplying the texture atlas.

## Time spent

- The application itself came together fairly quickly — **~4 hours**.
- The costly part was fixing the visual context: without hints, the agent could
  **get stuck in loops** repeatedly "fixing" a visual bug without addressing the
  root cause.

## What I would do differently next time

- **Use several different AIs deliberately.** Right now they have clearly
  different strengths per task, so routing each task to the model that's best at
  it pays off. For example, Gemini handled texture work better here — but in
  other projects the reverse holds: Gemini struggles with the PJSIP library
  while Sonnet does markedly better. No single model wins everywhere.

## Key lessons learned

- Typical tasks get done an order of magnitude faster with AI.
- For complex tasks and production projects, use it **only with careful review** —
  reading every diff and verifying behavior, not trusting output blindly.
- On code review it produces **many false positives** — flagging non-existent
  bugs — so its findings need verification before acting on them.
