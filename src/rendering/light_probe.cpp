#include "pch.h"
#include "light_probe.h"

#include "dx/dx_pipeline.h"
#include "dx/dx_command_list.h"
#include "dx/dx_context.h"
#include "geometry/mesh_builder.h"
#include "render_utils.h"
#include "core/imgui.h"

#include "light_probe_rs.hlsli"



static dx_pipeline visualizeGridPipeline;
static dx_mesh sphereMesh;
static submesh_info sphereSubmesh;

static light_probe_tracer tracer;

void initializeLightProbePipelines()
{
	if (!dxContext.featureSupport.raytracing())
	{
		return;
	}

	auto desc = CREATE_GRAPHICS_PIPELINE
		.inputLayout(inputLayout_position)
		.renderTargets(ldrFormat, depthStencilFormat);

	visualizeGridPipeline = createReloadablePipeline(desc, { "light_probe_grid_visualization_vs", "light_probe_grid_visualization_ps" });


	mesh_builder builder(mesh_creation_flags_with_positions);
	builder.pushSphere({});
	sphereSubmesh = builder.endSubmesh();
	sphereMesh = builder.createDXMesh();

	tracer.initialize();
}








void light_probe_grid::initialize(vec3 minCorner, vec3 dimensions, float cellSize)
{
	if (!dxContext.featureSupport.raytracing())
	{
		return;
	}

	uint32 gridSizeX = (uint32)(ceilf(dimensions.x / cellSize));
	uint32 gridSizeY = (uint32)(ceilf(dimensions.y / cellSize));
	uint32 gridSizeZ = (uint32)(ceilf(dimensions.z / cellSize));

	vec3 oldDimensions = dimensions;

	dimensions = vec3((float)gridSizeX, (float)gridSizeY, (float)gridSizeZ) * cellSize;
	minCorner -= (dimensions - oldDimensions) * 0.5f;

	this->minCorner = minCorner;
	this->cellSize = cellSize;

	this->numNodesX = gridSizeX + 1;
	this->numNodesY = gridSizeY + 1;
	this->numNodesZ = gridSizeZ + 1;
	this->totalNumNodes = numNodesX * numNodesY * numNodesZ;


	uint32 irradianceYSliceWidth = numNodesX * LIGHT_PROBE_TOTAL_RESOLUTION;
	uint32 irradianceTexWidth = numNodesY * irradianceYSliceWidth;
	uint32 irradianceTexHeight = numNodesZ * LIGHT_PROBE_TOTAL_RESOLUTION;

	uint32 depthYSliceWidth = numNodesX * LIGHT_PROBE_TOTAL_DEPTH_RESOLUTION;
	uint32 depthTexWidth = numNodesY * depthYSliceWidth;
	uint32 depthTexHeight = numNodesZ * LIGHT_PROBE_TOTAL_DEPTH_RESOLUTION;

#if 1
	struct color_rgba
	{
		uint8 r, g, b, a;
	};

	color_rgba* testBuffer = new color_rgba[irradianceTexWidth * irradianceTexHeight];


	color_rgba testProbe[LIGHT_PROBE_TOTAL_RESOLUTION][LIGHT_PROBE_TOTAL_RESOLUTION] = {};

	for (uint32 ly = 1; ly < LIGHT_PROBE_TOTAL_RESOLUTION - 1; ++ly)
	{
		for (uint32 lx = 1; lx < LIGHT_PROBE_TOTAL_RESOLUTION - 1; ++lx)
		{
			// Subtract the border.
			float x = (float)(lx - 1);
			float y = (float)(ly - 1);

			vec2 uv = (vec2(x, y) + 0.5f) / LIGHT_PROBE_RESOLUTION;
			vec2 oct = uv * 2.f - 1.f;

			vec3 dir = decodeOctahedral(oct);

			vec3 c = lerp(vec3(0.f, 0.f, 1.f), vec3(1.f, 0.f, 0.f), dir.x * 0.5f + 0.5f);
			//vec3 c = vec3(dir.x, 0.f, 0.f);
			testProbe[ly][lx] = { (uint8)(clamp01(c.x) * 255.f), (uint8)(clamp01(c.y) * 255.f), (uint8)(clamp01(c.z) * 255.f), 255 };
			int a = 0;
		}
	}

	// Copy border.
	testProbe[0][0] = testProbe[6][6];
	testProbe[0][7] = testProbe[6][1];
	testProbe[7][0] = testProbe[1][6];
	testProbe[7][7] = testProbe[1][1];

	testProbe[0][1] = testProbe[1][6];
	testProbe[0][2] = testProbe[1][5];
	testProbe[0][3] = testProbe[1][4];
	testProbe[0][4] = testProbe[1][3];
	testProbe[0][5] = testProbe[1][2];
	testProbe[0][6] = testProbe[1][1];

	testProbe[7][1] = testProbe[6][6];
	testProbe[7][2] = testProbe[6][5];
	testProbe[7][3] = testProbe[6][4];
	testProbe[7][4] = testProbe[6][3];
	testProbe[7][5] = testProbe[6][2];
	testProbe[7][6] = testProbe[6][1];
	
	testProbe[1][0] = testProbe[6][1];
	testProbe[2][0] = testProbe[5][1];
	testProbe[3][0] = testProbe[4][1];
	testProbe[4][0] = testProbe[3][1];
	testProbe[5][0] = testProbe[2][1];
	testProbe[6][0] = testProbe[1][1];
	
	testProbe[1][7] = testProbe[6][6];
	testProbe[2][7] = testProbe[5][6];
	testProbe[3][7] = testProbe[4][6];
	testProbe[4][7] = testProbe[3][6];
	testProbe[5][7] = testProbe[2][6];
	testProbe[6][7] = testProbe[1][6];


	for (uint32 z = 0; z < numNodesZ; ++z)
	{
		for (uint32 y = 0; y < numNodesY; ++y)
		{
			for (uint32 x = 0; x < numNodesX; ++x)
			{
				uint32 texStartX = irradianceYSliceWidth * y + x * LIGHT_PROBE_TOTAL_RESOLUTION;
				uint32 texStartY = z * LIGHT_PROBE_TOTAL_RESOLUTION;

				for (uint32 ly = 0; ly < LIGHT_PROBE_TOTAL_RESOLUTION; ++ly)
				{
					for (uint32 lx = 0; lx < LIGHT_PROBE_TOTAL_RESOLUTION; ++lx)
					{
						uint32 gx = texStartX + lx;
						uint32 gy = texStartY + ly;

						uint32 c = irradianceTexWidth * gy + gx;

						testBuffer[c] = testProbe[ly][lx];
					}
				}
			}
		}
	}

	this->irradiance = createTexture(testBuffer, irradianceTexWidth, irradianceTexHeight, DXGI_FORMAT_R8G8B8A8_UNORM, false, false, true);

	delete[] testBuffer;
#else
	this->irradiance = createTexture(0, irradianceTexWidth, irradianceTexHeight, DXGI_FORMAT_R11G11B10_FLOAT, false, false, true);
#endif

	this->depth = createTexture(0, depthTexWidth, depthTexHeight, DXGI_FORMAT_R16G16_FLOAT, false, false, true);
}


struct visualize_grid_material
{
	float cellSize;
	uint32 countX;
	uint32 countY;
	uint32 total;

	ref<dx_texture> irradianceTexture;
};

struct octahedral_light_probe_grid_pipeline
{
	using material_t = visualize_grid_material;

	PIPELINE_SETUP_DECL;
	PIPELINE_RENDER_DECL;
};


PIPELINE_SETUP_IMPL(octahedral_light_probe_grid_pipeline)
{
	cl->setPipelineState(*visualizeGridPipeline.pipeline);
	cl->setGraphicsRootSignature(*visualizeGridPipeline.rootSignature);
	cl->setPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

PIPELINE_RENDER_IMPL(octahedral_light_probe_grid_pipeline)
{
	vec2 uvScale = (float)LIGHT_PROBE_TOTAL_RESOLUTION / vec2((float)rc.material.irradianceTexture->width, (float)rc.material.irradianceTexture->height);
	cl->setGraphics32BitConstants(LIGHT_PROBE_GRID_VISUALIZATION_RS_CB, light_probe_visualization_cb{ viewProj * rc.transform, uvScale, rc.material.cellSize, rc.material.countX, rc.material.countY });
	cl->setDescriptorHeapSRV(LIGHT_PROBE_GRID_VISUALIZATION_RS_IRRADIANCE, 0, rc.material.irradianceTexture);

	cl->setVertexBuffer(0, rc.vertexBuffer.positions);
	cl->setVertexBuffer(1, rc.vertexBuffer.others);
	cl->setIndexBuffer(rc.indexBuffer);
	cl->drawIndexed(rc.submesh.numIndices, rc.material.total, rc.submesh.firstIndex, rc.submesh.baseVertex, 0);
}


void light_probe_grid::visualize(ldr_render_pass* ldrRenderPass)
{
	if (!dxContext.featureSupport.raytracing())
	{
		return;
	}

	mat4 transform = createTranslationMatrix(minCorner);
	visualize_grid_material material = { cellSize, numNodesX, numNodesY, totalNumNodes, irradiance };

	ldrRenderPass->renderObject<octahedral_light_probe_grid_pipeline>(transform, sphereMesh.vertexBuffer, sphereMesh.indexBuffer, sphereSubmesh, material);

	ImGui::Begin("Light probe");
	ImGui::Image(irradiance, irradiance->width * 15, irradiance->height * 15);
	ImGui::End();
}





void light_probe_tracer::initialize()
{
	const wchar* shaderPath = L"shaders/light_probe/light_probe_trace_rts.hlsl";


	const uint32 numInputResources = sizeof(input_resources) / sizeof(dx_cpu_descriptor_handle);
	const uint32 numOutputResources = sizeof(output_resources) / sizeof(dx_cpu_descriptor_handle);

	CD3DX12_DESCRIPTOR_RANGE resourceRanges[] =
	{
		// Must be input first, then output.
		CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, numInputResources, 0),
		CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, numOutputResources, 0),
	};

	CD3DX12_ROOT_PARAMETER globalRootParameters[] =
	{
		root_descriptor_table(arraysize(resourceRanges), resourceRanges),
		root_constants<light_probe_trace_cb>(0),
	};

	CD3DX12_STATIC_SAMPLER_DESC globalStaticSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	D3D12_ROOT_SIGNATURE_DESC globalDesc =
	{
		arraysize(globalRootParameters), globalRootParameters,
		1, &globalStaticSampler
	};


	// 6 Elements: Vertex buffer, index buffer, albedo texture, normal map, roughness texture, metallic texture.
	CD3DX12_DESCRIPTOR_RANGE hitSRVRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0, 1);
	CD3DX12_ROOT_PARAMETER hitRootParameters[] =
	{
		root_constants<pbr_material_cb>(0, 1),
		root_descriptor_table(1, &hitSRVRange),
	};

	D3D12_ROOT_SIGNATURE_DESC hitDesc =
	{
		arraysize(hitRootParameters), hitRootParameters
	};


	raytracing_mesh_hitgroup radianceHitgroup = { L"radianceClosestHit" };
	raytracing_mesh_hitgroup shadowHitgroup = { };

	pipeline =
		raytracing_pipeline_builder(shaderPath, maxPayloadSize, maxRecursionDepth, true, false)
		.globalRootSignature(globalDesc)
		.raygen(L"rayGen")
		.hitgroup(L"RADIANCE", L"radianceMiss", radianceHitgroup, hitDesc)
		.hitgroup(L"SHADOW", L"shadowMiss", shadowHitgroup)
		.finish();

	pbr_raytracer::initialize();

	allocateDescriptorHeapSpaceForGlobalResources<input_resources, output_resources>(descriptorHeap);
}

void light_probe_tracer::render(dx_command_list* cl, const raytracing_tlas& tlas, const light_probe_grid& grid, const common_material_info& materialInfo)
{
}
