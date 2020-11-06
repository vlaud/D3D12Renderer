#pragma once

#include "dx_render_primitives.h"
#include "math.h"

enum aspect_ratio_mode
{
	aspect_ratio_free,
	aspect_ratio_fix_16_9,
	aspect_ratio_fix_16_10,

	aspect_ratio_mode_count,
};

static const char* aspectRatioNames[] =
{
	"Free",
	"16:9",
	"16:10",
};


struct pbr_environment
{
	dx_texture sky;
	dx_texture prefiltered;
	dx_texture irradiance;

	dx_descriptor_handle skyHandle;
	dx_descriptor_handle prefilteredHandle;
	dx_descriptor_handle irradianceHandle;
};


struct dx_renderer
{
	static void initialize(uint32 windowWidth, uint32 windowHeight);

	static void beginFrame(uint32 windowWidth, uint32 windowHeight);
	static void recalculateViewport(bool resizeTextures);
	static void dummyRender(float dt);


	static dx_cbv_srv_uav_descriptor_heap globalDescriptorHeap;

	static dx_render_target hdrRenderTarget;
	static dx_texture hdrColorTexture;
	static dx_descriptor_handle hdrColorTextureSRV;
	static dx_texture depthBuffer;

	static uint32 renderWidth;
	static uint32 renderHeight;
	static uint32 windowWidth;
	static uint32 windowHeight;

	static dx_render_target windowRenderTarget;
	static dx_descriptor_handle frameResultSRV;
	static dx_texture frameResult;

	static D3D12_VIEWPORT windowViewport;

	static aspect_ratio_mode aspectRatioMode;
};

