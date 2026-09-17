# moth_noised

A node editor for authoring noise graphs, forked from the editor that ships with
[FastNoise2](https://github.com/Auburn/FastNoise2).

Upstream's editor is very close to what I want. What it does not have is named
project files — it keeps a single implicit workspace in an ImGui `.ini` — and its
only portable form of a graph is a base64 blob. This fork adds saved projects and
writes graphs as JSON, so a noise graph is a file you can name, diff, and review
like any other asset.

## Relationship to moth_toolkit

The editor is a separate application, not a toolkit one. It owns its own window,
its own UI stack and its own main loop, and it shares exactly one thing with the
engine: the file format.

That format lives in `moth::noise`, a module of
[moth_toolkit](https://github.com/instinkt900/moth_toolkit), and it is this
editor's only Moth dependency. Nothing here reaches for the rest of the toolkit,
and a game reading these files does not need the editor.

## Project files

A project is saved as JSON. The canvas is a forest rather than a single graph —
upstream treats every node with nothing feeding off it as a root — so a project
holds an array of graphs and the index of the one that is the output:

```json
{
  "format": "moth.noise.project",
  "version": 1,
  "output": 0,
  "trees": [
    {
      "tree": { "format": "moth.noise.tree", "version": 1, "root": 0, "nodes": [ ... ] },
      "layout": [ { "x": 0.0, "y": 0.0 } ]
    }
  ],
  "preview": { "seed": 1337, "scale": 2.5, "type": 0 }
}
```

What sits under `"tree"` is an ordinary `moth.noise.tree` document. Node
positions live beside it in `"layout"`, parallel to the graph's node order, so
an engine reads the graph knowing nothing about this editor and canvas
coordinates never reach engine-side parsing.

A bare `moth.noise.tree` document opens too, as a one-tree project — a graph
exported for an engine can be reopened here without a conversion step.

Selecting a node part way down a graph previews that node, but saves the whole
graph it belongs to: which node is being previewed is transient, the graph is
the asset.

## Building

Dependencies come from two places, deliberately.

**Conan** supplies the data half — `moth::noise` and FastNoise2 beneath it.
Taking FastNoise2 through Conan rather than building a second copy here means the
editor and the engine link the same library revision, so a graph that round-trips
in the editor round-trips in the game.

**CPM** supplies the UI half — Magnum, ImGui, imnodes, GLFW, freetype. That is
what upstream does, and those pins are load-bearing: imnodes is a fork carried by
commit, and Magnum and Corrade have to agree with each other.

The two halves do not collide because `moth::noise` is built against `moth_core`
with `enable_platform=False`, which drops core's GLFW backend and leaves the
types. Magnum's GLFW is then the only one in the link. The recipe sets this
itself; you do not have to remember it.

`moth_noise` is published to an Artifactory remote rather than Conan Center.
Register the remote once before installing (it is publicly readable, so no login is
required):

```sh
conan remote add moth https://artifactory.matthewcotton.net/artifactory/api/conan/conan-local
```

C++20 is required: the editor code from upstream uses C++20, so the recipe checks for
it. Conan's detected profiles default to lower (`gnu17` on Linux, 14 with MSVC), so
pass the standard on the command line as below, or set it in your Conan profile.

On Linux, the UI half is built from source and needs the X11, Wayland and OpenGL
development headers:

```sh
sudo apt install libgl-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev \
    libxi-dev libwayland-dev libxkbcommon-dev wayland-protocols
```

```sh
# Linux
conan install . --build=missing -s build_type=Release -s compiler.cppstd=gnu20
cmake --preset conan-release
cmake --build --preset conan-release

# Windows
conan install . --build=missing -s build_type=Release -s compiler.cppstd=20
cmake --preset conan-default
cmake --build --preset conan-release
```

The first configure clones the UI stack, which takes a while. Subsequent ones
reuse it.

## Layout

| Path        | What it is                                                     |
|-------------|----------------------------------------------------------------|
| `src/`      | The editor. Forked from upstream `tools/NodeEditor`.            |
| `ipc/`      | Shared-memory channel a running game uses to read the selected graph. Forked from upstream `tools/NodeEditorIpc`. |
| `cmake/`    | CPM.                                                            |

## Fork provenance

Both vendored trees were imported verbatim from Auburn/FastNoise2 at **v1.1.1**
(`903c1f2d2f9d53ddce94cd223f32727d9ab3aeaa`) — the same revision the
`fastnoise2/1.1.1` Conan package builds from — each in its own commit with
nothing edited. Every later commit is therefore a legible diff against upstream,
which is what makes pulling in a future upstream release by hand tractable.

Upstream is MIT licensed; see `LICENSE.FastNoise2`.
