#pragma once

#include "../Types.h"
#include "../world/Chunk.h"
#include "../world/BlockRegistry.h"

#include <glm/glm.hpp>
#include <array>
#include <vector>

// ── MeshBuilder ───────────────────────────────────────────────────────────────
// Converts a Chunk's voxel data into a flat vertex list using greedy meshing.
//
// Algorithm (per face direction, per slice):
//   1. Build a 2D visibility mask: each cell = block type if face is exposed, Air if not.
//   2. Scan the mask: find the widest run of the same type (width w).
//   3. Extend downward as long as all w cells match (height h).
//   4. Emit one quad covering w×h blocks. Clear those mask cells.
//   5. Repeat until the mask is empty.
//
// Result: coplanar, same-type faces are merged into a single large quad,
//         dramatically reducing vertex count for flat or homogeneous surfaces.
class MeshBuilder {
public:
    // neighbors[6] — adjacent chunks in face order: +Y,-Y,+X,-X,+Z,-Z
    // Pass nullptr for any neighbor that isn't loaded (treated as Air).
    static std::vector<Vertex> build(const Chunk&   chunk,
                                     ChunkPos       pos,
                                     const Chunk*   neighbors[6] = nullptr)
    {
        std::vector<Vertex> verts;
        verts.reserve(512);

        const float ox = static_cast<float>(pos.x * CHUNK_W);
        const float oz = static_cast<float>(pos.z * CHUNK_D);

        buildYFaces(verts, chunk, neighbors, ox, oz);
        buildXFaces(verts, chunk, neighbors, ox, oz);
        buildZFaces(verts, chunk, neighbors, ox, oz);

        return verts;
    }

private:
    // ── Mask types ────────────────────────────────────────────────────────────
    // Top/Bottom:  W=CHUNK_W (x), H=CHUNK_D (z)
    // East/West:   W=CHUNK_D (z), H=CHUNK_H (y)
    // South/North: W=CHUNK_W (x), H=CHUNK_H (y)

    template<int W, int H>
    using Mask = std::array<BlockType, W * H>;

    template<int W, int H>
    static BlockType& at(Mask<W,H>& m, int u, int v) { return m[u + v * W]; }

    template<int W, int H>
    static BlockType  at(const Mask<W,H>& m, int u, int v) { return m[u + v * W]; }

    // ── Face visibility ───────────────────────────────────────────────────────
    // A face from block `a` into block `b` is visible when:
    //   a is solid AND b is either air or transparent.
    static bool visible(BlockType a, BlockType b) {
        if (a == BlockType::Air) return false;
        const Voxel& va = BlockRegistry::get(a);
        if (!va.isSolid()) return false;
        const Voxel& vb = BlockRegistry::get(b);
        return !vb.isSolid() || vb.isTransparent();
    }

    // ── Greedy merge ──────────────────────────────────────────────────────────
    // Calls emit(u0, v0, w, h, type) for each merged rectangle, then clears it.
    template<int W, int H, typename Fn>
    static void greedyMerge(Mask<W,H>& mask, Fn&& emit) {
        for (int v = 0; v < H; ++v) {
            int u = 0;
            while (u < W) {
                const BlockType type = at<W,H>(mask, u, v);
                if (type == BlockType::Air) { ++u; continue; }

                // Width: longest same-type run in u
                int w = 1;
                while (u + w < W && at<W,H>(mask, u+w, v) == type) ++w;

                // Height: extend in v while the full width matches
                int h = 1;
                while (v + h < H) {
                    bool ok = true;
                    for (int k = 0; k < w; ++k)
                        if (at<W,H>(mask, u+k, v+h) != type) { ok = false; break; }
                    if (!ok) break;
                    ++h;
                }

                emit(u, v, w, h, type);

                // Clear merged cells
                for (int dv = 0; dv < h; ++dv)
                for (int du = 0; du < w; ++du)
                    at<W,H>(mask, u+du, v+dv) = BlockType::Air;

                u += w;
            }
        }
    }

    // ── Quad emission ─────────────────────────────────────────────────────────
    // Vertices must be in CCW order when viewed from the outside.
    // Triangles: 0-1-2, 0-2-3
    static void emitQuad(std::vector<Vertex>& out,
                          glm::vec3 v0, glm::vec3 v1,
                          glm::vec3 v2, glm::vec3 v3,
                          glm::vec3 normal, float uw, float uh,
                          glm::vec3 color)
    {
        const glm::vec2 uv0(0,  0 );
        const glm::vec2 uv1(0,  uh);
        const glm::vec2 uv2(uw, uh);
        const glm::vec2 uv3(uw, 0 );

        out.push_back({ v0, normal, uv0, color });
        out.push_back({ v1, normal, uv1, color });
        out.push_back({ v2, normal, uv2, color });
        out.push_back({ v0, normal, uv0, color });
        out.push_back({ v2, normal, uv2, color });
        out.push_back({ v3, normal, uv3, color });
    }

    // ── +Y / -Y  (mask: u=x, v=z) ────────────────────────────────────────────
    static void buildYFaces(std::vector<Vertex>& verts, const Chunk& chunk,
                             const Chunk* nb[6], float ox, float oz)
    {
        Mask<CHUNK_W, CHUNK_D> maskTop, maskBot;

        for (int y = 0; y < CHUNK_H; ++y) {
            const float fy = static_cast<float>(y);

            for (int x = 0; x < CHUNK_W; ++x)
            for (int z = 0; z < CHUNK_D; ++z) {
                const BlockType cur = chunk.get(x, y, z);

                // +Y neighbor
                BlockType abv = (y+1 < CHUNK_H) ? chunk.get(x,y+1,z) : BlockType::Air;
                // -Y neighbor
                BlockType blw = (y-1 >= 0)       ? chunk.get(x,y-1,z) : BlockType::Air;

                at<CHUNK_W,CHUNK_D>(maskTop, x, z) = visible(cur, abv) ? cur : BlockType::Air;
                at<CHUNK_W,CHUNK_D>(maskBot, x, z) = visible(cur, blw) ? cur : BlockType::Air;
            }

            // +Y  — CCW from above: v0(x,z) → v1(x,z+h) → v2(x+w,z+h) → v3(x+w,z)
            greedyMerge<CHUNK_W,CHUNK_D>(maskTop, [&](int u0,int v0,int w,int h,BlockType bt){
                const glm::vec3 col = BlockRegistry::get(bt).color();
                const float fy1 = fy + 1.0f;
                emitQuad(verts,
                    {ox+u0,   fy1, oz+v0  },
                    {ox+u0,   fy1, oz+v0+h},
                    {ox+u0+w, fy1, oz+v0+h},
                    {ox+u0+w, fy1, oz+v0  },
                    {0,1,0}, (float)w, (float)h, col);
            });

            // -Y  — CCW from below: v0(x,z) → v1(x+w,z) → v2(x+w,z+h) → v3(x,z+h)
            greedyMerge<CHUNK_W,CHUNK_D>(maskBot, [&](int u0,int v0,int w,int h,BlockType bt){
                const glm::vec3 col = BlockRegistry::get(bt).color();
                emitQuad(verts,
                    {ox+u0,   fy, oz+v0  },
                    {ox+u0+w, fy, oz+v0  },
                    {ox+u0+w, fy, oz+v0+h},
                    {ox+u0,   fy, oz+v0+h},
                    {0,-1,0}, (float)w, (float)h, col);
            });
        }
    }

    // ── +X / -X  (mask: u=z, v=y) ────────────────────────────────────────────
    static void buildXFaces(std::vector<Vertex>& verts, const Chunk& chunk,
                             const Chunk* nb[6], float ox, float oz)
    {
        Mask<CHUNK_D, CHUNK_H> maskE, maskW;

        for (int x = 0; x < CHUNK_W; ++x) {
            const float fx = ox + static_cast<float>(x);

            for (int z = 0; z < CHUNK_D; ++z)
            for (int y = 0; y < CHUNK_H; ++y) {
                const BlockType cur = chunk.get(x, y, z);

                // +X neighbor
                BlockType east = BlockType::Air;
                if (x+1 < CHUNK_W)         east = chunk.get(x+1, y, z);
                else if (nb && nb[2])       east = nb[2]->get(0, y, z);

                // -X neighbor
                BlockType west = BlockType::Air;
                if (x-1 >= 0)              west = chunk.get(x-1, y, z);
                else if (nb && nb[3])      west = nb[3]->get(CHUNK_W-1, y, z);

                at<CHUNK_D,CHUNK_H>(maskE, z, y) = visible(cur, east) ? cur : BlockType::Air;
                at<CHUNK_D,CHUNK_H>(maskW, z, y) = visible(cur, west) ? cur : BlockType::Air;
            }

            // +X  — CCW from +X: v0(z+w,y) → v1(z,y) → v2(z,y+h) → v3(z+w,y+h)
            greedyMerge<CHUNK_D,CHUNK_H>(maskE, [&](int u0,int v0,int w,int h,BlockType bt){
                const glm::vec3 col = BlockRegistry::get(bt).color();
                emitQuad(verts,
                    {fx+1, (float)v0,   oz+u0+w},
                    {fx+1, (float)v0,   oz+u0  },
                    {fx+1, (float)v0+h, oz+u0  },
                    {fx+1, (float)v0+h, oz+u0+w},
                    {1,0,0}, (float)w, (float)h, col);
            });

            // -X  — CCW from -X: v0(z,y) → v1(z+w,y) → v2(z+w,y+h) → v3(z,y+h)
            greedyMerge<CHUNK_D,CHUNK_H>(maskW, [&](int u0,int v0,int w,int h,BlockType bt){
                const glm::vec3 col = BlockRegistry::get(bt).color();
                emitQuad(verts,
                    {fx,   (float)v0,   oz+u0  },
                    {fx,   (float)v0,   oz+u0+w},
                    {fx,   (float)v0+h, oz+u0+w},
                    {fx,   (float)v0+h, oz+u0  },
                    {-1,0,0}, (float)w, (float)h, col);
            });
        }
    }

    // ── +Z / -Z  (mask: u=x, v=y) ────────────────────────────────────────────
    static void buildZFaces(std::vector<Vertex>& verts, const Chunk& chunk,
                             const Chunk* nb[6], float ox, float oz)
    {
        Mask<CHUNK_W, CHUNK_H> maskS, maskN;

        for (int z = 0; z < CHUNK_D; ++z) {
            const float fz = oz + static_cast<float>(z);

            for (int x = 0; x < CHUNK_W; ++x)
            for (int y = 0; y < CHUNK_H; ++y) {
                const BlockType cur = chunk.get(x, y, z);

                // +Z neighbor
                BlockType south = BlockType::Air;
                if (z+1 < CHUNK_D)         south = chunk.get(x, y, z+1);
                else if (nb && nb[4])       south = nb[4]->get(x, y, 0);

                // -Z neighbor
                BlockType north = BlockType::Air;
                if (z-1 >= 0)              north = chunk.get(x, y, z-1);
                else if (nb && nb[5])      north = nb[5]->get(x, y, CHUNK_D-1);

                at<CHUNK_W,CHUNK_H>(maskS, x, y) = visible(cur, south) ? cur : BlockType::Air;
                at<CHUNK_W,CHUNK_H>(maskN, x, y) = visible(cur, north) ? cur : BlockType::Air;
            }

            // +Z  — CCW from +Z: v0(x,y) → v1(x+w,y) → v2(x+w,y+h) → v3(x,y+h)
            greedyMerge<CHUNK_W,CHUNK_H>(maskS, [&](int u0,int v0,int w,int h,BlockType bt){
                const glm::vec3 col = BlockRegistry::get(bt).color();
                emitQuad(verts,
                    {ox+u0,   (float)v0,   fz+1},
                    {ox+u0+w, (float)v0,   fz+1},
                    {ox+u0+w, (float)v0+h, fz+1},
                    {ox+u0,   (float)v0+h, fz+1},
                    {0,0,1}, (float)w, (float)h, col);
            });

            // -Z  — CCW from -Z: v0(x+w,y) → v1(x,y) → v2(x,y+h) → v3(x+w,y+h)
            greedyMerge<CHUNK_W,CHUNK_H>(maskN, [&](int u0,int v0,int w,int h,BlockType bt){
                const glm::vec3 col = BlockRegistry::get(bt).color();
                emitQuad(verts,
                    {ox+u0+w, (float)v0,   fz},
                    {ox+u0,   (float)v0,   fz},
                    {ox+u0,   (float)v0+h, fz},
                    {ox+u0+w, (float)v0+h, fz},
                    {0,0,-1}, (float)w, (float)h, col);
            });
        }
    }
};
