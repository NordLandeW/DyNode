
#include <glm/glm.hpp>

void vertex_tri_write(char*& vertBuf, const glm::vec2& p0, const glm::vec2& p1,
                      const glm::vec2& p2, const glm::vec2& uv0,
                      const glm::vec2& uv1, const glm::vec2& uv2,
                      const glm::i8vec4& color);

void vertex_quad_write(char*& vertBuf, const glm::vec2& p0, const glm::vec2& p1,
                       const glm::vec2& p2, const glm::vec2& p3,
                       const glm::vec2& uv0, const glm::vec2& uv1,
                       const glm::vec2& uv2, const glm::vec2& uv3,
                       const glm::i8vec4& color);
