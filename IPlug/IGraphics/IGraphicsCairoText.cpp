#include "IGraphicsCairoText.h"


IGraphicsCairoText::IGraphicsCairoText()
{
	ext_height = new cairo_text_extents_t;
	text_extents = new cairo_text_extents_t;
	font_extents = new cairo_font_extents_t;
}

IGraphicsCairoText::~IGraphicsCairoText()
{
	delete ext_height;
	delete text_extents;
	delete font_extents;

	// If local font was initialized destroy font on exit
	if (local_font_initialized)
	{
		FT_Done_Face(local_ft_face);
		FT_Done_FreeType(local_ft_library);
	}
}

void IGraphicsCairoText::icairo_create_font_from_path(const char * path)
{
	// If local font was initialized destroy old font
	if (local_font_initialized)
	{
		FT_Done_Face(local_ft_face);
		FT_Done_FreeType(local_ft_library);
	}

	FT_Init_FreeType(&local_ft_library);
	FT_New_Face(local_ft_library, path, 0, &local_ft_face);

	local_font_initialized = true;
}

void IGraphicsCairoText::icairo_create_font_from_memory(int name, int type, void* histance_bundle_id)
{
	// If local font was initialized destroy old font
	if (local_font_initialized)
	{
		FT_Done_Face(local_ft_face);
		FT_Done_FreeType(local_ft_library);
	}

#ifdef _WIN32
	hinstance_handle = *(HINSTANCE*)histance_bundle_id;
	HRSRC rc = ::FindResource(hinstance_handle, MAKEINTRESOURCE(name), MAKEINTRESOURCE(type));
	HGLOBAL rcData = ::LoadResource(hinstance_handle, rc);
	int size = ::SizeofResource(hinstance_handle, rc);
	const FT_Byte* data = static_cast<const FT_Byte*>(::LockResource(rcData));

	FT_Init_FreeType(&local_ft_library);
	FT_New_Memory_Face(local_ft_library, data, size, 0, &local_ft_face);

#elif defined(__APPLE__)
	bundleID = *histance_bundle_id; // TODO
	CFStringRef CFBundleID = __CFStringMakeConstantString(bundleID);
	CFBundleRef requestedBundle = CFBundleGetBundleWithIdentifier(CFBundleID);
	CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(requestedBundle);
	char path[PATH_MAX];
	CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX);

	CFRelease(resourcesURL);

	chdir(path);

	std::string font_path = path;
	std::string font_name = relative_path;

	int lastindex = font_name.find_last_of("/");

	font_path.append(font_name.substr(lastindex, font_name.size() - lastindex));

	FT_Init_FreeType(&local_ft_library);
	FT_New_Face(local_ft_library, font_path.c_str(), 0, &local_ft_face);

#endif
	local_font_initialized = true;
}

void IGraphicsCairoText::icairo_initialize_font_face(cairo_t * cr)
{
	if (local_font_initialized)
	{
		current_font_face = cairo_ft_font_face_create_for_ft_face(local_ft_face, 0);
		cairo_set_font_face(cr, current_font_face);
	}
	else if (global_font_initialized)
	{
		current_font_face = cairo_ft_font_face_create_for_ft_face(*global_ft_face, 0);
		cairo_set_font_face(cr, current_font_face);
	}
	else
	{
		return;
	}
}

void IGraphicsCairoText::icairo_destroy_font_face()
{
	cairo_font_face_destroy(current_font_face);
}

void IGraphicsCairoText::icairo_set_text(cairo_t * cr, const char * text)
{
	draw_text = text;
}

void IGraphicsCairoText::icairo_set_text_position(cairo_t * cr, IRECT rect, icairo_text_w_aligement w_aligement, icairo_text_h_aligement h_aligement)
{
	width_aligement = w_aligement;
	height_aligement = h_aligement;

	text_rect = rect;

	icairo_calculate_extents(cr);

	double x, y;

	switch (width_aligement)
	{
	case W_ALIGN_LEFT:
		x = text_rect.L;
		break;

	case W_ALIGN_RIGHT:
		x = text_rect.R - text_extents->width - text_extents->x_bearing;
		break;

	case W_ALIGN_CENTER:
		x = text_rect.L + ((text_rect.W() - text_extents->width - text_extents->x_bearing) / 2.0);
		break;

	default:
		break;
	}

	switch (height_aligement)
	{
	case H_ALIGN_TOP:
		y = text_rect.T + font_extents->ascent;
		break;

	case H_ALIGN_BOTTOM:
		y = text_rect.B - font_extents->descent;
		break;

	case H_ALIGN_CENTER:
		y = text_rect.B - ((text_rect.H() - font_extents->height) / 2.0) - font_extents->descent;
		break;

	default:
		break;
	}

	cairo_move_to(cr, x, y);
}

void IGraphicsCairoText::icairo_calculate_extents(cairo_t * cr)
{
	cairo_font_extents(cr, font_extents);
	cairo_text_extents(cr, draw_text, text_extents);
}

cairo_font_extents_t * IGraphicsCairoText::icairo_get_font_extents(cairo_t * cr)
{
	return font_extents;
}

cairo_text_extents_t * IGraphicsCairoText::icairo_get_text_extents(cairo_t * cr)
{
	return text_extents;
}

void IGraphicsCairoText::icairo_show_text(cairo_t * cr, const char * text, double size, IColor color, IRECT rect, icairo_text_w_aligement w_aligement, icairo_text_h_aligement h_aligement)
{
	cairo_set_source_rgba(cr, color.R / 255.0, color.G / 255.0, color.B / 255.0, color.A / 255.0);

	icairo_initialize_font_face(cr);
	cairo_set_font_size(cr, size);
	icairo_set_text(cr, text);
	icairo_set_text_position(cr, rect, w_aligement, h_aligement);

	//// Adding subpixel rendering improves rendering speed by 10% on my system
	//cairo_font_options_t *options;
	//cairo_get_font_options(cr, options);

	//cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_SUBPIXEL);
	//cairo_set_font_options(cr, options);
	//// -----------------------------------------------------------------------

	cairo_show_text(cr, text);

	icairo_destroy_font_face();
}

void IGraphicsCairoText::icairo_show_text(cairo_t * cr)
{
	cairo_show_text(cr, draw_text);
}