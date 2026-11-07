#pragma once

#include <cstddef>
#include <glm/glm.hpp>

// Position, UV, color, then mapping (width/repeat-end ownership, draw type).
inline constexpr size_t VERTEX_BYTES = 28;
static_assert(sizeof(glm::vec2) == 8 && sizeof(glm::i8vec4) == 4);

void vertex_tri_write(char*& vertBuf, const glm::vec2& p0, const glm::vec2& p1,
                      const glm::vec2& p2, const glm::vec2& uv0,
                      const glm::vec2& uv1, const glm::vec2& uv2,
                      const glm::i8vec4& color, const glm::vec2& mapping = {});

void vertex_quad_write(char*& vertBuf, const glm::vec2& p0, const glm::vec2& p1,
                       const glm::vec2& p2, const glm::vec2& p3,
                       const glm::vec2& uv0, const glm::vec2& uv1,
                       const glm::vec2& uv2, const glm::vec2& uv3,
                       const glm::i8vec4& color, const glm::vec2& mapping = {});
