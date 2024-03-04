#pragma once

#include "./mesh_collider.h"
#include "./box_collider.h"

bool check_collision(const PlaneCollider& plane, const MeshCollider& mesh);