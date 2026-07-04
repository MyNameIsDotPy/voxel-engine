#pragma once

#include "../Types.h"
#include "../world/Chunk.h"
#include "../world/BlockRegistry.h"

#include <glm/glm.hpp>
#include <array>
#include <vector>

// ── MeshBuilder ───────────────────────────────────────────────────────────────
// Converts a Chunk's voxel data into two greedy-meshed vertex lists:
//   opaque      — fully solid blocks, rendered first
//   transparent — water / liquids, rendered in a separate blended pass
class MeshBuilder {
public:
    struct MeshData {
        std::vector<Vertex> opaque;
        std::vector<Vertex> transparent;
    };

    // neighbors[6] — adjacent chunks: +Y,-Y,+X,-X,+Z,-Z (nullptr = treat as Air)
    static MeshData build(const Chunk&  chunk,
                          ChunkPos      pos,
                          const Chunk*  neighbors[6] = nullptr)
    {
        MeshData data;
        data.opaque.reserve(512);
        data.transparent.reserve(128);

        const float ox = static_cast<float>(pos.x * CHUNK_W);
        const float oz = static_cast<float>(pos.z * CHUNK_D);

        buildYFaces(data, chunk, neighbors, ox, oz);
        buildXFaces(data, chunk, neighbors, ox, oz);
        buildZFaces(data, chunk, neighbors, ox, oz);

        return data;
    }

private:
    // ── Mask types ────────────────────────────────────────────────────────────
    template<int W, int H>
    using Mask = std::array<BlockType, W * H>;

    template<int W, int H>
    static BlockType& at(Mask<W,H>& m, int u, int v) { return m[u + v * W]; }

    template<int W, int H>
    static BlockType  at(const Mask<W,H>& m, int u, int v) { return m[u + v * W]; }

    // ── Face visibility ───────────────────────────────────────────────────────
    // Opaque face: solid block `a` toward `b` → visible if b is air or transparent
    static bool visibleOpaque(BlockType a, BlockType b) {
        if (a == BlockType::Air) return false;
        const Voxel& va = BlockRegistry::get(a);
        if (!va.isSolid() || va.alpha() < 1.0f) return false;
        const Voxel& vb = BlockRegistry::get(b);
        return !vb.isSolid() || vb.isTransparent();
    }

    // Transparent face: liquid/transparent block `a` toward `b`
    // Only emit if neighbor is air (no liquid-to-liquid faces)
    static bool visibleTransparent(BlockType a, BlockType b) {
        if (a == BlockType::Air) return false;
        const Voxel& va = BlockRegistry::get(a);
        if (va.alpha() >= 1.0f) return false; // only transparent blocks here
        return b == BlockType::Air;
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
    static void emitQuad(std::vector<Vertex>& out,
                          glm::vec3 v0, glm::vec3 v1,
                          glm::vec3 v2, glm::vec3 v3,
                          glm::vec3 normal, float uw, float uh,
                          glm::vec4 color)
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

    static glm::vec4 blockColor(BlockType bt) {
        const Voxel& v = BlockRegistry::get(bt);
        return glm::vec4(v.color(), v.alpha());
    }

    // ── +Y / -Y  (mask: u=x, v=z) ────────────────────────────────────────────
    static void buildYFaces(MeshData& data, const Chunk& chunk,
                             const Chunk* nb[6], float ox, float oz)
    {
        Mask<CHUNK_W, CHUNK_D> maskOpaqueTop, maskOpaqueBot;
        Mask<CHUNK_W, CHUNK_D> maskTransTop,  maskTransBot;

        for (int y = 0; y < CHUNK_H; ++y) {
            const float fy = static_cast<float>(y);

            for (int x = 0; x < CHUNK_W; ++x)
            for (int z = 0; z < CHUNK_D; ++z) {
                const BlockType cur = chunk.get(x, y, z);

                // +Y neighbor
                BlockType abv = (y+1 < CHUNK_H) ? chunk.get(x,y+1,z) : BlockType::Air;
                // -Y neighbor
                BlockType blw = (y-1 >= 0) ? chunk.get(x,y-1,z) : BlockType::Air;

                at<CHUNK_W,CHUNK_D>(maskOpaqueTop, x, z) = visibleOpaque(cur, abv)      ? cur : BlockType::Air;
                at<CHUNK_W,CHUNK_D>(maskOpaqueBot, x, z) = visibleOpaque(cur, blw)      ? cur : BlockType::Air;
                at<CHUNK_W,CHUNK_D>(maskTransTop,  x, z) = visibleTransparent(cur, abv) ? cur : BlockType::Air;
                at<CHUNK_W,CHUNK_D>(maskTransBot,  x, z) = visibleTransparent(cur, blw) ? cur : BlockType::Air;
            }

            auto emitTop = [&](std::vector<Vertex>& buf, Mask<CHUNK_W,CHUNK_D>& mask) {
                const float fy1 = fy + 1.0f;
                greedyMerge<CHUNK_W,CHUNK_D>(mask, [&](int u0,int v0,int w,int h,BlockType bt){
                    emitQuad(buf,
                        {ox+u0,   fy1, oz+v0  }, {ox+u0,   fy1, oz+v0+h},
                        {ox+u0+w, fy1, oz+v0+h}, {ox+u0+w, fy1, oz+v0  },
                        {0,1,0}, (float)w, (float)h, blockColor(bt));
                });
            };
            auto emitBot = [&](std::vector<Vertex>& buf, Mask<CHUNK_W,CHUNK_D>& mask) {
                greedyMerge<CHUNK_W,CHUNK_D>(mask, [&](int u0,int v0,int w,int h,BlockType bt){
                    emitQuad(buf,
                        {ox+u0,   fy, oz+v0  }, {ox+u0+w, fy, oz+v0  },
                        {ox+u0+w, fy, oz+v0+h}, {ox+u0,   fy, oz+v0+h},
                        {0,-1,0}, (float)w, (float)h, blockColor(bt));
                });
            };

            emitTop(data.opaque,      maskOpaqueTop);
            emitBot(data.opaque,      maskOpaqueBot);
            emitTop(data.transparent, maskTransTop);
            emitBot(data.transparent, maskTransBot);
        }
    }

    // ── +X / -X  (mask: u=z, v=y) ────────────────────────────────────────────
    static void buildXFaces(MeshData& data, const Chunk& chunk,
                             const Chunk* nb[6], float ox, float oz)
    {
        Mask<CHUNK_D, CHUNK_H> maskOE, maskOW, maskTE, maskTW;

        for (int x = 0; x < CHUNK_W; ++x) {
            const float fx = ox + static_cast<float>(x);

            for (int z = 0; z < CHUNK_D; ++z)
            for (int y = 0; y < CHUNK_H; ++y) {
                const BlockType cur = chunk.get(x, y, z);

                BlockType east = BlockType::Air;
                if (x+1 < CHUNK_W)    east = chunk.get(x+1, y, z);
                else if (nb && nb[2]) east = nb[2]->get(0, y, z);

                BlockType west = BlockType::Air;
                if (x-1 >= 0)         west = chunk.get(x-1, y, z);
                else if (nb && nb[3]) west = nb[3]->get(CHUNK_W-1, y, z);

                at<CHUNK_D,CHUNK_H>(maskOE, z, y) = visibleOpaque(cur, east)      ? cur : BlockType::Air;
                at<CHUNK_D,CHUNK_H>(maskOW, z, y) = visibleOpaque(cur, west)      ? cur : BlockType::Air;
                at<CHUNK_D,CHUNK_H>(maskTE, z, y) = visibleTransparent(cur, east) ? cur : BlockType::Air;
                at<CHUNK_D,CHUNK_H>(maskTW, z, y) = visibleTransparent(cur, west) ? cur : BlockType::Air;
            }

            auto emitE = [&](std::vector<Vertex>& buf, Mask<CHUNK_D,CHUNK_H>& mask) {
                greedyMerge<CHUNK_D,CHUNK_H>(mask, [&](int u0,int v0,int w,int h,BlockType bt){
                    emitQuad(buf,
                        {fx+1,(float)v0,  oz+u0+w}, {fx+1,(float)v0,  oz+u0  },
                        {fx+1,(float)v0+h,oz+u0  }, {fx+1,(float)v0+h,oz+u0+w},
                        {1,0,0}, (float)w, (float)h, blockColor(bt));
                });
            };
            auto emitW = [&](std::vector<Vertex>& buf, Mask<CHUNK_D,CHUNK_H>& mask) {
                greedyMerge<CHUNK_D,CHUNK_H>(mask, [&](int u0,int v0,int w,int h,BlockType bt){
                    emitQuad(buf,
                        {fx,  (float)v0,  oz+u0  }, {fx,  (float)v0,  oz+u0+w},
                        {fx,  (float)v0+h,oz+u0+w}, {fx,  (float)v0+h,oz+u0  },
                        {-1,0,0}, (float)w, (float)h, blockColor(bt));
                });
            };

            emitE(data.opaque, maskOE); emitW(data.opaque, maskOW);
            emitE(data.transparent, maskTE); emitW(data.transparent, maskTW);
        }
    }

    // ── +Z / -Z  (mask: u=x, v=y) ────────────────────────────────────────────
    static void buildZFaces(MeshData& data, const Chunk& chunk,
                             const Chunk* nb[6], float ox, float oz)
    {
        Mask<CHUNK_W, CHUNK_H> maskOS, maskON, maskTS, maskTN;

        for (int z = 0; z < CHUNK_D; ++z) {
            const float fz = oz + static_cast<float>(z);

            for (int x = 0; x < CHUNK_W; ++x)
            for (int y = 0; y < CHUNK_H; ++y) {
                const BlockType cur = chunk.get(x, y, z);

                BlockType south = BlockType::Air;
                if (z+1 < CHUNK_D)    south = chunk.get(x, y, z+1);
                else if (nb && nb[4]) south = nb[4]->get(x, y, 0);

                BlockType north = BlockType::Air;
                if (z-1 >= 0)         north = chunk.get(x, y, z-1);
                else if (nb && nb[5]) north = nb[5]->get(x, y, CHUNK_D-1);

                at<CHUNK_W,CHUNK_H>(maskOS, x, y) = visibleOpaque(cur, south)      ? cur : BlockType::Air;
                at<CHUNK_W,CHUNK_H>(maskON, x, y) = visibleOpaque(cur, north)      ? cur : BlockType::Air;
                at<CHUNK_W,CHUNK_H>(maskTS, x, y) = visibleTransparent(cur, south) ? cur : BlockType::Air;
                at<CHUNK_W,CHUNK_H>(maskTN, x, y) = visibleTransparent(cur, north) ? cur : BlockType::Air;
            }

            auto emitS = [&](std::vector<Vertex>& buf, Mask<CHUNK_W,CHUNK_H>& mask) {
                greedyMerge<CHUNK_W,CHUNK_H>(mask, [&](int u0,int v0,int w,int h,BlockType bt){
                    emitQuad(buf,
                        {ox+u0,  (float)v0,  fz+1}, {ox+u0+w,(float)v0,  fz+1},
                        {ox+u0+w,(float)v0+h,fz+1}, {ox+u0,  (float)v0+h,fz+1},
                        {0,0,1}, (float)w, (float)h, blockColor(bt));
                });
            };
            auto emitN = [&](std::vector<Vertex>& buf, Mask<CHUNK_W,CHUNK_H>& mask) {
                greedyMerge<CHUNK_W,CHUNK_H>(mask, [&](int u0,int v0,int w,int h,BlockType bt){
                    emitQuad(buf,
                        {ox+u0+w,(float)v0,  fz}, {ox+u0,  (float)v0,  fz},
                        {ox+u0,  (float)v0+h,fz}, {ox+u0+w,(float)v0+h,fz},
                        {0,0,-1}, (float)w, (float)h, blockColor(bt));
                });
            };

            emitS(data.opaque, maskOS); emitN(data.opaque, maskON);
            emitS(data.transparent, maskTS); emitN(data.transparent, maskTN);
        }
    }
};
