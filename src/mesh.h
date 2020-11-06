#pragma once

#include "colliders.h"
#include "dx_render_primitives.h"

struct aiScene;


struct single_mesh
{
	submesh_info submesh;
	aabb_collider boundingBox;
	std::string name;
};

struct lod_mesh
{
	uint32 firstMesh;
	uint32 numMeshes;
};

struct composite_mesh
{
	std::vector<single_mesh> singleMeshes;
	std::vector<lod_mesh> lods;
	std::vector<float> lodDistances;
	dx_mesh mesh;
};


const aiScene* loadAssimpSceneFile(const char* filepath);
void freeAssimpScene(const aiScene* scene);
composite_mesh createCompositeMeshFromScene(const aiScene* scene);
