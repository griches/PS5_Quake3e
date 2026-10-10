/*
===========================================================================
Quake3e for the PlayStation 5 - the display: RADV linked into the title, a VK_KHR_display surface.
Replaces code/sdl/sdl_glimp.c and sdl_gamma.c.

Copyright (C) 1999-2005 Id Software, Inc. (the interfaces this implements)

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
===========================================================================
*/
#include "client/client.h"

#include "platform.h"

void IN_Init( void );
void IN_Shutdown( void );

// RADV's entry point: the console has no Vulkan loader
VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr( VkInstance instance, const char *name );

static glconfig_t *ps5_config;
static cvar_t *r_ps5Height;
static cvar_t *r_ps5Refresh;


void GLimp_InitGamma( glconfig_t *config )
{
	// no hardware gamma ramp; the renderer corrects in its own pass (r_fbo)
	config->deviceSupportsGamma = qfalse;
}


void GLimp_SetGamma( unsigned char red[256], unsigned char green[256], unsigned char blue[256] )
{
}


void VKimp_Init( glconfig_t *config )
{
	int height;

	Com_DPrintf( "VKimp_Init()\n" );

	r_ps5Height = Cvar_Get( "r_ps5Height", "2160", CVAR_ARCHIVE );
	Cvar_SetDescription( r_ps5Height, "PS5: the display mode's height when the graphics menu has not chosen one: 2160, 1440 or 1080. The console scales it to the screen." );
	r_ps5Refresh = Cvar_Get( "r_ps5Refresh", "0", CVAR_ARCHIVE | CVAR_LATCH );
	Cvar_SetDescription( r_ps5Refresh, "PS5: the highest refresh rate to take, in Hz; 0 takes the fastest the display offers." );

	ps5_config = config;

	// The modes the graphics menu lists (the title's ui.qvm reads this, as ioquake3's does)
	Cvar_Get( "r_availableModes", "3840x2160 2560x1440 1920x1080", CVAR_ROM );

	// The menu's choice arrives as r_mode -1 with r_customwidth and r_customheight;
	// anything else (r_mode -2, the desktop's size, is the default) takes r_ps5Height.
	// VK_CreateSurface settles the size from the display mode it takes.
	height = r_ps5Height->integer;
	if ( r_mode->integer == -1 )
		height = Cvar_VariableIntegerValue( "r_customheight" );
	config->vidHeight = height == 1080 || height == 1440 ? height : 2160;
	config->vidWidth = config->vidHeight * 16 / 9;
	config->windowAspect = 16.0f / 9.0f;
	config->displayFrequency = 60;
	config->isFullscreen = qtrue;
	config->stereoEnabled = qfalse;
	config->colorBits = 32;
	config->depthBits = 24;
	config->stencilBits = 8;

	// These values force the UI to disable driver selection
	config->driverType = GLDRV_ICD;
	config->hardwareType = GLHW_GENERIC;

	IN_Init();
	HandleEvents();
	Key_ClearStates();
}


void *VK_GetInstanceProcAddr( VkInstance instance, const char *name )
{
	return (void *)vk_icdGetInstanceProcAddr( instance, name );
}


/*
===============
VK_CreateSurface

The console has one display. Its modes come largest first, each size's fastest
refresh first; a smaller size is scaled to the screen by the console.
===============
*/
qboolean VK_CreateSurface( VkInstance instance, VkSurfaceKHR *surface )
{
#define ICD( name ) PFN_##name q##name = (PFN_##name)vk_icdGetInstanceProcAddr( instance, #name ); \
	if ( !q##name ) { Com_Printf( "VK_CreateSurface: the driver has no " #name "\n" ); return qfalse; }
	ICD( vkEnumeratePhysicalDevices )
	ICD( vkGetPhysicalDeviceDisplayPropertiesKHR )
	ICD( vkGetDisplayModePropertiesKHR )
	ICD( vkCreateDisplayPlaneSurfaceKHR )
#undef ICD
	VkPhysicalDevice device;
	VkDisplayPropertiesKHR display;
	VkDisplayModePropertiesKHR modes[32];
	VkDisplaySurfaceCreateInfoKHR desc;
	uint32_t count, i, pick;
	VkResult res;

	count = 1;
	res = qvkEnumeratePhysicalDevices( instance, &count, &device );
	if ( ( res != VK_SUCCESS && res != VK_INCOMPLETE ) || count == 0 ) {
		Com_Printf( "VK_CreateSurface: no physical device (%d)\n", (int)res );
		return qfalse;
	}
	count = 1;
	res = qvkGetPhysicalDeviceDisplayPropertiesKHR( device, &count, &display );
	if ( ( res != VK_SUCCESS && res != VK_INCOMPLETE ) || count == 0 ) {
		Com_Printf( "VK_CreateSurface: no display (%d)\n", (int)res );
		return qfalse;
	}
	count = ARRAY_LEN( modes );
	res = qvkGetDisplayModePropertiesKHR( device, display.display, &count, modes );
	if ( ( res != VK_SUCCESS && res != VK_INCOMPLETE ) || count == 0 ) {
		Com_Printf( "VK_CreateSurface: no display mode (%d)\n", (int)res );
		return qfalse;
	}

	pick = count;
	for ( i = 0; i < count; i++ ) {
		const VkDisplayModeParametersKHR *p = &modes[i].parameters;
		Com_Printf( "display mode %u: %ux%u at %.2f Hz\n", i, p->visibleRegion.width, p->visibleRegion.height, p->refreshRate / 1000.0 );
		if ( pick != count )
			continue;
		if ( (int)p->visibleRegion.height != ps5_config->vidHeight )
			continue;
		if ( r_ps5Refresh->integer > 0 && p->refreshRate > (uint32_t)( r_ps5Refresh->integer * 1000 + 500 ) )
			continue;
		pick = i;
	}
	if ( pick == count )
		pick = 0;

	memset( &desc, 0, sizeof( desc ) );
	desc.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
	desc.displayMode = modes[pick].displayMode;
	desc.planeIndex = 0;
	desc.planeStackIndex = 0;
	desc.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	desc.globalAlpha = 1.0f;
	desc.alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
	desc.imageExtent = modes[pick].parameters.visibleRegion;
	res = qvkCreateDisplayPlaneSurfaceKHR( instance, &desc, NULL, surface );
	if ( res != VK_SUCCESS ) {
		Com_Printf( "VK_CreateSurface: vkCreateDisplayPlaneSurfaceKHR gave %d\n", (int)res );
		return qfalse;
	}

	ps5_config->vidWidth = (int)desc.imageExtent.width;
	ps5_config->vidHeight = (int)desc.imageExtent.height;
	ps5_config->windowAspect = (float)ps5_config->vidWidth / (float)ps5_config->vidHeight;
	ps5_config->displayFrequency = (int)( ( modes[pick].parameters.refreshRate + 500 ) / 1000 );
	Com_Printf( "display: mode %u, %dx%d at %d Hz\n", pick, ps5_config->vidWidth, ps5_config->vidHeight, ps5_config->displayFrequency );
	return qtrue;
}


void VKimp_Shutdown( qboolean unloadDLL )
{
	IN_Shutdown();
}
