#include "Math/Frustum.h"

namespace math {

bool Frustum::check_shpere(const Aabb &aabb)
{
	glm::vec4 center(aabb.get_center(), 1.0f);
	float radius = aabb.get_radius();

	for (const auto& plane : planes)
		if (dot(plane, center) < -radius)
			return false;

	return true;
}
void Frustum::build(const glm::mat4 &inv_view_proj_mat)
{
	inv_view_proj_matrix_ = inv_view_proj_mat;
	static const glm::vec4 tln(-1.0f, -1.0f, 0.0f, 1.0f);
	static const glm::vec4 tlf(-1.0f, -1.0f, 1.0f, 1.0f);
	static const glm::vec4 bln(-1.0f, +1.0f, 0.0f, 1.0f);
	static const glm::vec4 blf(-1.0f, +1.0f, 1.0f, 1.0f);
	static const glm::vec4 trn(+1.0f, -1.0f, 0.0f, 1.0f);
	static const glm::vec4 trf(+1.0f, -1.0f, 1.0f, 1.0f);
	static const glm::vec4 brn(+1.0f, +1.0f, 0.0f, 1.0f);
	static const glm::vec4 brf(+1.0f, +1.0f, 1.0f, 1.0f);
	static const glm::vec4 c(0.0f, 0.0f, 0.5f, 1.0f);

	const auto project = [](const glm::vec4& v) {
		return v.xyz() / glm::vec3(v.w);
	};

	glm::vec3 TLN = project(inv_view_proj_mat * tln);
	glm::vec3 BLN = project(inv_view_proj_mat * bln);
	glm::vec3 BLF = project(inv_view_proj_mat * blf);
	glm::vec3 TRN = project(inv_view_proj_mat * trn);
	glm::vec3 TRF = project(inv_view_proj_mat * trf);
	glm::vec3 BRN = project(inv_view_proj_mat * brn);
	glm::vec3 BRF = project(inv_view_proj_mat * brf);
	glm::vec4 center = inv_view_proj_mat * c;

	glm::vec3 l = normalize(cross(BLF - BLN, TLN - BLN));
	glm::vec3 r = normalize(cross(TRF - TRN, BRN - TRN));
	glm::vec3 n = normalize(cross(BLN - BRN, TRN - BRN));
	glm::vec3 f = normalize(cross(TRF - BRF, BLF - BRF));
	glm::vec3 t = normalize(cross(TLN - TRN, TRF - TRN));
	glm::vec3 b = normalize(cross(BRF - BRN, BLN - BRN));

	planes[LEFT] = glm::vec4(l, -dot(l, BLN));
	planes[RIGHT] = glm::vec4(r, -dot(r, TRN));
	planes[NEAR] = glm::vec4(n, -dot(n, BRN));
	planes[FAR] = glm::vec4(f, -dot(f, BRF));
	planes[TOP] = glm::vec4(t, -dot(t, TRN));
	planes[BOTTOM] = glm::vec4(b, -dot(b, BRN));

	// Winding order checks.
	for (auto& p : planes)
		if (glm::dot(center, p) < 0.0f)
			p = -p;

}

}