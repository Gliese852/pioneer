#include "graphics/VertexArray.h"
#include "graphics/VertexBuffer.h"
#include "graphics/Material.h"

#include <memory>

inline Graphics::VertexArray my_debug_lines(Graphics::ATTRIB_POSITION | Graphics::ATTRIB_DIFFUSE, 256);
inline std::unique_ptr<Graphics::Material> my_debug_material;
inline std::unique_ptr<Graphics::MeshObject> my_debug_mesh;
inline matrix4x4d my_debug_base = matrix4x4d::Identity();

template <typename Vec, typename... Types>
void my_debug_lines_add(const Vec &vec, const Types &...args)
{
	my_debug_lines.Add(vector3f(my_debug_base.InvTransform(vec)), args...);
}

// using camera's parent frame coords
//
// example usage
//
//	my_debug_lines.Clear();
//	my_debug_base = matrix4x4d(m_target->GetOrient(), m_target->GetPosition());
//	my_debug_lines_add(ship->GetPosition(), Color::RED);
//	my_debug_lines_add(nextPosition, Color::RED);
