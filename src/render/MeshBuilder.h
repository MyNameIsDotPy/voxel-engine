#pragma once

#include "../Types.h"
#include "../world/Chunk.h"
#include "../world/BlockRegistry.h"

#include <glm/glm.hpp>
#include <vector>

// ── MeshBuilder ───────────────────────────────────────────────────────────────
// Converts a Chunk's voxel data into a flat vertex list ready for ChunkMesh.
//
// Algorithm: naive face culling.
//   For every non-air block, check all 6 neighbors.
//   Emit a quad only if the neighbor is air (or transparent).
//
// Greedy meshing (issue #3) will replace the inner loop later without changing
// the public interface.
class MeshBuilder {
public:
    // Build geometry for `chunk` at grid position `pos`.
    // `neighbors[6]` — adjacent chunks in order {+X,-X,+Y,-Y,+Z,-Z}.
    // Pass nullptr for any neighbor that isn't loaded yet (treated as all-air).
    static std::vector<Vertex> build(const Chunk&   chunk,
                                     ChunkPos       pos,
                                     const Chunk*   neighbors[6] = nullptr)
    {
        std::vector<Vertex> verts;
        verts.reserve(4096);

        const float ox = static_cast<float>(pos.x * CHUNK_W);
        const float oz = static_cast<float>(pos.z * CHUNK_D);

        for (int y = 0; y < CHUNK_H; ++y)
        for (int z = 0; z < CHUNK_D; ++z)
        for (int x = 0; x < CHUNK_W; ++x)
        {
            const BlockType bt = chunk.get(x, y, z);
            if (bt == BlockType::Air) continue;

            // ── Check each of the 6 faces ─────────────────────────────────
            for (int f = 0; f < 6; ++f) {
                const Face& face = FACES[f];

                const int nx = x + face.dx;
                const int ny = y + face.dy;
                const int nz = z + face.dz;

                BlockType neighbor = BlockType::Air;

                if (Chunk::inBounds(nx, ny, nz)) {
                    neighbor = chunk.get(nx, ny, nz);
                } else if (neighbors && neighbors[f]) {
                    // Wrap into the adjacent chunk's local coords
                    int lx = (nx + CHUNK_W) % CHUNK_W;
                    int ly = (ny + CHUNK_H) % CHUNK_H;
                    int lz = (nz + CHUNK_D) % CHUNK_D;
                    neighbor = neighbors[f]->get(lx, ly, lz);
                }
                // else: out-of-world edge → treat as air (face is visible)

                if (!BlockRegistry::get(neighbor).solid ||
                     BlockRegistry::get(neighbor).transparent)
                {
                    emitFace(verts, face,
                             ox + x, static_cast<float>(y), oz + z,
                             bt);
                }
            }
        }

        return verts;
    }

private:
    struct Face {
        int       dx, dy, dz;       // neighbor offset
        glm::vec3 normal;
        // 4 corner offsets (CCW when facing outward)
        glm::vec3 v[4];
        // UV coords matching the corner order
        glm::vec2 uv[4];
    };

    // clang-format off
    inline static const Face FACES[6] = {
        // +Y  Top
        { 0, 1, 0, {0,1,0},
          {{0,1,0},{1,1,0},{1,1,1},{0,1,1}},
          {{0,1},{1,1},{1,0},{0,0}} },
        // -Y  Bottom
        { 0,-1, 0, {0,-1,0},
          {{0,0,1},{1,0,1},{1,0,0},{0,0,0}},
          {{0,0},{1,0},{1,1},{0,1}} },
        // +X  East
        { 1, 0, 0, {1,0,0},
          {{1,0,1},{1,0,0},{1,1,0},{1,1,1}},
          {{0,0},{1,0},{1,1},{0,1}} },
        // -X  West
        {-1, 0, 0, {-1,0,0},
          {{0,0,0},{0,0,1},{0,1,1},{0,1,0}},
          {{0,0},{1,0},{1,1},{0,1}} },
        // +Z  South
        { 0, 0, 1, {0,0,1},
          {{0,0,1},{1,0,1},{1,1,1},{0,1,1}},
          {{0,0},{1,0},{1,1},{0,1}} },
        // -Z  North
        { 0, 0,-1, {0,0,-1},
          {{1,0,0},{0,0,0},{0,1,0},{1,1,0}},
          {{0,0},{1,0},{1,1},{0,1}} },
    };
    // clang-format on

    static void emitFace(std::vector<Vertex>& out,
                         const Face& face,
                         float wx, float wy, float wz,
                         BlockType /*bt*/)  // bt reserved for texture UVs later
    {
        const glm::vec3 origin(wx, wy, wz);

        // Two triangles: 0-1-2 and 0-2-3
        const int idx[6] = { 0, 1, 2, 0, 2, 3 };
        for (int i : idx) {
            out.push_back({
                origin + face.v[i],
                face.normal,
                face.uv[i]
            });
        }
    }
};
