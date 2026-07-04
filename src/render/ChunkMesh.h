#pragma once

#include <glad/gl.h>
#include "../Types.h"

#include <vector>

// ── ChunkMesh ─────────────────────────────────────────────────────────────────
// Owns a VAO + VBO pair for one chunk.
// MeshBuilder produces the vertex list; ChunkMesh uploads it to the GPU.
class ChunkMesh {
public:
    ChunkMesh() {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

        // position  (location = 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, pos)));
        glEnableVertexAttribArray(0);

        // normal    (location = 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, normal)));
        glEnableVertexAttribArray(1);

        // uv        (location = 2)
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, uv)));
        glEnableVertexAttribArray(2);

        // color     (location = 3)
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, color)));
        glEnableVertexAttribArray(3);

        glBindVertexArray(0);
    }

    ~ChunkMesh() {
        if (m_vao) glDeleteVertexArrays(1, &m_vao);
        if (m_vbo) glDeleteBuffers(1, &m_vbo);
    }

    // Non-copyable, movable
    ChunkMesh(const ChunkMesh&)            = delete;
    ChunkMesh& operator=(const ChunkMesh&) = delete;

    ChunkMesh(ChunkMesh&& o) noexcept
        : m_vao(o.m_vao), m_vbo(o.m_vbo), m_vertexCount(o.m_vertexCount) {
        o.m_vao = o.m_vbo = 0;
        o.m_vertexCount   = 0;
    }

    // Upload new geometry — call this whenever the chunk is dirty
    void upload(const std::vector<Vertex>& vertices) {
        m_vertexCount = static_cast<GLsizei>(vertices.size());

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     m_vertexCount * sizeof(Vertex),
                     vertices.data(),
                     GL_DYNAMIC_DRAW);
    }

    // Issue the draw call — Renderer calls this after binding the shader
    void draw() const {
        if (m_vertexCount == 0) return;
        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
    }

    bool isEmpty()     const { return m_vertexCount == 0; }
    int  vertexCount() const { return m_vertexCount; }

private:
    GLuint  m_vao         = 0;
    GLuint  m_vbo         = 0;
    GLsizei m_vertexCount = 0;
};
