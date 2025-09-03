#include "vertex.h"

#include <glm/glm.hpp>

#include "bitio.h"

void vertex_tri_write(char*& vertBuf, const glm::vec2& p0, const glm::vec2& p1,
                      const glm::vec2& p2, const glm::vec2& uv0,
                      const glm::vec2& uv1, const glm::vec2& uv2,
                      const glm::i8vec4& color) {
    bitwrite(vertBuf, p0);
    bitwrite(vertBuf, uv0);
    bitwrite(vertBuf, color);
    bitwrite(vertBuf, p1);
    bitwrite(vertBuf, uv1);
    bitwrite(vertBuf, color);
    bitwrite(vertBuf, p2);
    bitwrite(vertBuf, uv2);
    bitwrite(vertBuf, color);
}

void vertex_quad_write(char*& vertBuf, const glm::vec2& p0, const glm::vec2& p1,
                       const glm::vec2& p2, const glm::vec2& p3,
                       const glm::vec2& uv0, const glm::vec2& uv1,
                       const glm::vec2& uv2, const glm::vec2& uv3,
                       const glm::i8vec4& color) {
    vertex_tri_write(vertBuf, p0, p1, p2, uv0, uv1, uv2, color);
    vertex_tri_write(vertBuf, p1, p2, p3, uv1, uv2, uv3, color);
}