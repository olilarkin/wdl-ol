#pragma once

/*
Youlean - small library for enabling Cairo Freetype text support in IPlug

Copyright (C) 2018 and later, Youlean

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software
in a product, an acknowledgment in the product documentation would be
appreciated but is not required.
2. This notice may not be removed or altered from any source distribution.

*/

#include "cairo/cairo.h"
#ifdef OS_OSX
#include "cairo/cairo-quartz.h"
#include <ft2build.h>
#else
#include "cairo/cairo-win32.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

#include <cairo-ft.h>
#include "IGraphicsConstants.h"
#include "IGraphicsStructs.h"


typedef enum _icairo_text_w_aligement { W_ALIGN_LEFT, W_ALIGN_RIGHT, W_ALIGN_CENTER } icairo_text_w_aligement;
typedef enum _icairo_text_h_aligement { H_ALIGN_CENTER, H_ALIGN_TOP, H_ALIGN_BOTTOM } icairo_text_h_aligement;

class IGraphicsCairoText
{

public:
	IGraphicsCairoText();
	~IGraphicsCairoText();

	void icairo_create_font_from_path(const char* path);
	void icairo_create_font_from_memory(int name, int type, void* histance_bundle_id);

	void icairo_initialize_font_face(cairo_t *cr);
	void icairo_destroy_font_face();

	void icairo_set_text(cairo_t *cr, const char *text);

	void icairo_set_text_position(cairo_t *cr, IRECT rect, icairo_text_w_aligement w_aligement = W_ALIGN_CENTER, icairo_text_h_aligement h_aligement = H_ALIGN_CENTER);

	void icairo_calculate_extents(cairo_t *cr);

	cairo_font_extents_t* icairo_get_font_extents(cairo_t *cr);
	cairo_text_extents_t* icairo_get_text_extents(cairo_t *cr);

	void icairo_show_text(cairo_t *cr, const char *text, double size, IColor color, IRECT rect,
		icairo_text_w_aligement w_aligement = W_ALIGN_CENTER, icairo_text_h_aligement h_aligement = H_ALIGN_CENTER);

	void icairo_show_text(cairo_t *cr);

private:
	const char *draw_text = "";
	IRECT text_rect;
	icairo_text_w_aligement width_aligement;
	icairo_text_h_aligement height_aligement;

	cairo_font_face_t *current_font_face;
	cairo_text_extents_t *text_extents, *ext_height;
	cairo_font_extents_t *font_extents;
	unsigned char *surface_out_test;

	FT_Library *global_ft_library;
	FT_Face *global_ft_face;

	FT_Library local_ft_library;
	FT_Face local_ft_face;

	bool global_font_initialized;
	bool local_font_initialized = false;

#ifdef _WIN32
	HINSTANCE hinstance_handle;
#elif defined(__APPLE__)
	const char* bundleID;
#endif
};