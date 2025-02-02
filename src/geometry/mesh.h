#pragma once

#include "core/asset.h"
#include "physics/bounding_volumes.h"
#include "dx/dx_buffer.h"
#include "animation/animation.h"
#include "mesh_builder.h"

struct pbr_material;


struct submesh
{
	submesh_info info;
	bounding_box aabb; // In multi's local space.
	trs transform;

	ref<pbr_material> material;
	std::string name;
};

struct multi_mesh
{
	std::vector<submesh> submeshes;
	animation_skeleton skeleton;
	dx_mesh mesh;
	bounding_box aabb;

	asset_handle handle;
	uint32 flags;
};


ref<multi_mesh> loadMeshFromFile(const fs::path& sceneFilename, uint32 flags = mesh_creation_flags_default);
ref<multi_mesh> loadMeshFromHandle(asset_handle handle, uint32 flags = mesh_creation_flags_default);

// Same function but with different default flags (includes skin).
inline ref<multi_mesh> loadAnimatedMeshFromFile(const fs::path& sceneFilename, uint32 flags = mesh_creation_flags_animated)
{
	return loadMeshFromFile(sceneFilename, flags);
}

struct raster_component
{
	ref<multi_mesh> mesh;
};
