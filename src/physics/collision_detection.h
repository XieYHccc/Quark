#pragma once

#include "./mesh_collider.h"
#include "./box_collider.h"

bool check_collision(std::shared_ptr<PlaneCollider> plane, std::shared_ptr<MeshCollider> mesh);