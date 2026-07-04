# Task Division

Two-developer split. Dev 1 owns everything that touches the GPU and the window. Dev 2 owns all data-side logic and never calls OpenGL directly.

---

## Developer 1 — Rendering & Engine Core

### Classes

| Class | File | Responsibility |
|-------|------|----------------|
| `Renderer` | `src/render/Renderer.h` | Central render loop, clears buffers, dispatches draw calls |
| `ChunkMesh` | `src/render/ChunkMesh.h` | Holds VAO/VBO for one chunk; uploads vertex data produced by `MeshBuilder` |
| `MeshBuilder` | `src/render/MeshBuilder.h` | Reads `Chunk` voxel data, outputs vertices (naive → greedy meshing) |
| `TextureAtlas` | `src/render/TextureAtlas.h` | Packs block textures into a single atlas, maps `BlockType → UV` |
| `Frustum` | `src/render/Frustum.h` | Extracts planes from the VP matrix; `bool isVisible(AABB)` |
| `Shader` *(extend)* | `src/Shader.h` | Add UBO support for camera matrices |
| `Camera` *(extend)* | `src/Camera.h` | Add first-person mode toggle |
| `DebugOverlay` | `src/render/DebugOverlay.h` | FPS counter, chunk/face count, camera position (Dear ImGui) |

### Behaviors

- **Face culling between voxels** — only emit a quad if the neighboring voxel is air
- **Greedy meshing** — merge coplanar same-type faces to minimize draw calls
- **Frustum culling** — skip `ChunkMesh::draw()` when the chunk AABB is outside the frustum
- **Render distance loop** — iterate visible chunks from `World` and draw their meshes
- **Texture atlas loading** — load PNGs with `stb_image`, build atlas, generate UV map

### External libs (add via FetchContent)

| Lib | Purpose |
|-----|---------|
| [stb_image](https://github.com/nothings/stb) | PNG texture loading |
| [Dear ImGui](https://github.com/ocornut/imgui) | Debug overlay |

---

## Developer 2 — World, Voxels & Gameplay

### Classes

| Class | File | Responsibility |
|-------|------|----------------|
| `BlockType` | `src/world/BlockType.h` | `enum class BlockType : uint8_t { Air, Grass, Dirt, Stone, … }` |
| `BlockRegistry` | `src/world/BlockRegistry.h` | Maps `BlockType → BlockDef` (name, hardness, texture IDs, solid flag) |
| `Chunk` | `src/world/Chunk.h` | 16×256×16 array of `BlockType`; `get/set(x,y,z)`, dirty flag |
| `World` | `src/world/World.h` | `unordered_map<ChunkPos, Chunk>` — chunk lifecycle, load/unload radius |
| `TerrainGenerator` | `src/world/TerrainGenerator.h` | Fills a `Chunk` with procedural terrain via FastNoiseLite |
| `Player` | `src/world/Player.h` | Position, velocity, AABB; `update(dt)` moves and resolves collisions |
| `Physics` | `src/world/Physics.h` | `resolveAABB(Player&, World&)` — sweep-based collision against solid voxels |
| `RayCast` | `src/world/RayCast.h` | DDA voxel traversal; returns hit block + face for place/break |

### Behaviors

- **Chunk loading** — generate chunks within render distance, unload distant ones
- **Terrain generation** — Simplex height map + cave noise + biome-driven block selection
- **Dirty flag propagation** — `World::setBlock()` marks the chunk (and neighbors if on edge) dirty
- **Block interaction** — use `RayCast` result to place or remove blocks

### External libs (add via FetchContent)

| Lib | Purpose |
|-----|---------|
| [FastNoiseLite](https://github.com/Auburn/FastNoiseLite) | Single-header noise library |

---

## Shared Contract

Agree on these interfaces before starting so both developers can work in parallel.

```cpp
// Vertex layout — Dev 2 produces data, Dev 1 reads it
struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

// Chunk coordinate in the world grid
struct ChunkPos { int x, z; };

// Dev 1 polls this to know when to re-mesh
bool Chunk::isDirty() const;
void Chunk::clearDirty();

// Dev 1 reads texture IDs during mesh building
const BlockDef& BlockRegistry::get(BlockType) const;
```

### Data flow

```
World ──(dirty chunks)──► MeshBuilder ──(vertices)──► ChunkMesh ──► Renderer
  ▲                                                                      │
  └──────────────── Player + RayCast (place/break) ────────────────────┘
```
