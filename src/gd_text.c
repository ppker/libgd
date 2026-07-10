#include <math.h>
#include <stdint.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gd_errors.h"
#include "gd_path.h"
#include "gd_vector2d_private.h"
#include "gdhelpers.h"

#ifdef HAVE_LIBFREETYPE
#ifdef HAVE_FT2BUILD_H
#include <ft2build.h>
#include FT_BBOX_H
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#else
#include <freetype/freetype.h>
#include <freetype/ftbbox.h>
#include <freetype/ftoutln.h>
#endif
#ifdef HAVE_LIBRAQM
#include <raqm.h>
#endif

#define FT_POS_TO_DOUBLE(v) ((double)(v) / 64.0)

struct gdFontFaceStruct {
    int ref;
    FT_Library library;
    FT_Face face;
};

typedef struct {
    unsigned int index;
    FT_Pos x_advance;
    FT_Pos y_advance;
    FT_Pos x_offset;
    FT_Pos y_offset;
    uint32_t cluster;
} gdTextGlyphInfo;

typedef struct {
    gdPathPtr path;
    double origin_x;
    double origin_y;
    FT_Pos pen_x;
    FT_Pos pen_y;
    FT_Pos x_offset;
    FT_Pos y_offset;
    int started;
} gdTextOutlineBuilder;

static gdTextStatus gdTextSetError(gdTextError *err, gdTextStatus code, int provider_code,
                                   const char *message)
{
    if (err) {
        err->code = code;
        err->provider_code = provider_code;
        err->message[0] = '\0';
        if (message) {
            strncpy(err->message, message, sizeof(err->message) - 1);
            err->message[sizeof(err->message) - 1] = '\0';
        }
    }
    return code;
}

static gdTextStatus gdTextClearError(gdTextError *err)
{
    return gdTextSetError(err, GD_TEXT_OK, 0, NULL);
}

BGD_DECLARE(void) gdTextOptionsInit(gdTextOptions *options)
{
    if (!options)
        return;
    options->shaping = GD_TEXT_SHAPING_NONE;
    options->line_spacing = 1.0;
    options->reserved_flags = 0;
    options->reserved_double = 0.0;
}

static void gdTextMapPoint(gdTextOutlineBuilder *builder, const FT_Vector *src, double *x,
                           double *y)
{
    *x = builder->origin_x + FT_POS_TO_DOUBLE(builder->pen_x + builder->x_offset + src->x);
    *y = builder->origin_y - FT_POS_TO_DOUBLE(builder->pen_y - builder->y_offset + src->y);
}

static int gdTextMoveTo(const FT_Vector *to, void *user)
{
    gdTextOutlineBuilder *builder = (gdTextOutlineBuilder *)user;
    double x, y;

    if (builder->started)
        gdPathClose(builder->path);
    gdTextMapPoint(builder, to, &x, &y);
    gdPathMoveTo(builder->path, x, y);
    builder->started = 1;
    return 0;
}

static int gdTextLineTo(const FT_Vector *to, void *user)
{
    gdTextOutlineBuilder *builder = (gdTextOutlineBuilder *)user;
    double x, y;

    gdTextMapPoint(builder, to, &x, &y);
    gdPathLineTo(builder->path, x, y);
    return 0;
}

static int gdTextConicTo(const FT_Vector *control, const FT_Vector *to, void *user)
{
    gdTextOutlineBuilder *builder = (gdTextOutlineBuilder *)user;
    double x1, y1, x2, y2;

    gdTextMapPoint(builder, control, &x1, &y1);
    gdTextMapPoint(builder, to, &x2, &y2);
    gdPathQuadTo(builder->path, x1, y1, x2, y2);
    return 0;
}

static int gdTextCubicTo(const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to,
                         void *user)
{
    gdTextOutlineBuilder *builder = (gdTextOutlineBuilder *)user;
    double x1, y1, x2, y2, x3, y3;

    gdTextMapPoint(builder, control1, &x1, &y1);
    gdTextMapPoint(builder, control2, &x2, &y2);
    gdTextMapPoint(builder, to, &x3, &y3);
    gdPathCurveTo(builder->path, x1, y1, x2, y2, x3, y3);
    return 0;
}

static int gdTextDecodeUtf8(const char *utf8, uint32_t **out_text, size_t *out_len,
                            gdTextError *err)
{
    const unsigned char *p = (const unsigned char *)utf8;
    size_t max_len;
    size_t len = 0;
    uint32_t *text;

    if (!utf8 || !out_text || !out_len) {
        gdTextSetError(err, GD_TEXT_E_INVALID_ARGUMENT, 0, "Invalid text arguments");
        return 0;
    }
    max_len = strlen(utf8);
    text = gdMalloc(sizeof(uint32_t) * (max_len ? max_len : 1));
    if (!text) {
        gdTextSetError(err, GD_TEXT_E_MEMORY, 0, "Out of memory");
        return 0;
    }

    while (*p) {
        uint32_t ch;
        unsigned int need;
        unsigned int i;
        unsigned char c = *p++;

        if (c < 0x80) {
            text[len++] = c;
            continue;
        } else if (c >= 0xC2 && c <= 0xDF) {
            ch = c & 0x1F;
            need = 1;
        } else if (c >= 0xE0 && c <= 0xEF) {
            ch = c & 0x0F;
            need = 2;
        } else if (c >= 0xF0 && c <= 0xF4) {
            ch = c & 0x07;
            need = 3;
        } else {
            gdFree(text);
            gdTextSetError(err, GD_TEXT_E_INVALID_ARGUMENT, 0, "Invalid UTF-8 text");
            return 0;
        }

        for (i = 0; i < need; i++) {
            unsigned char trail = *p++;
            if ((trail & 0xC0) != 0x80) {
                gdFree(text);
                gdTextSetError(err, GD_TEXT_E_INVALID_ARGUMENT, 0, "Invalid UTF-8 text");
                return 0;
            }
            ch = (ch << 6) | (uint32_t)(trail & 0x3F);
        }

        if ((need == 2 && ch < 0x800) || (need == 3 && ch < 0x10000) ||
            (ch >= 0xD800 && ch <= 0xDFFF) || ch > 0x10FFFF) {
            gdFree(text);
            gdTextSetError(err, GD_TEXT_E_INVALID_ARGUMENT, 0, "Invalid UTF-8 text");
            return 0;
        }
        text[len++] = ch;
    }

    *out_text = text;
    *out_len = len;
    return 1;
}

static int gdTextValidateOptions(const gdTextOptions *options, gdTextShaping *shaping,
                                 double *line_spacing, gdTextError *err)
{
    gdTextOptions defaults;

    if (!options) {
        gdTextOptionsInit(&defaults);
        options = &defaults;
    }
    *shaping = options->shaping;
    *line_spacing = isfinite(options->line_spacing) && options->line_spacing > 0.0
                        ? options->line_spacing
                        : 1.0;
    if (*shaping != GD_TEXT_SHAPING_NONE && *shaping != GD_TEXT_SHAPING_RAQM) {
        gdTextSetError(err, GD_TEXT_E_INVALID_ARGUMENT, 0, "Invalid text shaping option");
        return 0;
    }
    if (*shaping == GD_TEXT_SHAPING_RAQM) {
#ifndef HAVE_LIBRAQM
        gdTextSetError(err, GD_TEXT_E_UNAVAILABLE, 0, "RAQM text shaping is not available");
        return 0;
#endif
    }
    return 1;
}

static int gdTextLayoutSimple(FT_Face face, const uint32_t *text, size_t len,
                              gdTextGlyphInfo **out_info, size_t *out_count, gdTextError *err)
{
    size_t i;
    FT_UInt previous = 0;
    gdTextGlyphInfo *info;

    info = gdMalloc(sizeof(gdTextGlyphInfo) * (len ? len : 1));
    if (!info) {
        gdTextSetError(err, GD_TEXT_E_MEMORY, 0, "Out of memory");
        return 0;
    }

    for (i = 0; i < len; i++) {
        FT_UInt glyph_index = FT_Get_Char_Index(face, text[i]);
        FT_Vector delta;
        FT_Error ft_err;

        delta.x = delta.y = 0;
        if (!FT_IS_FIXED_WIDTH(face) && FT_HAS_KERNING(face) && previous && glyph_index)
            FT_Get_Kerning(face, previous, glyph_index, ft_kerning_default, &delta);

        ft_err = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
        if (ft_err) {
            gdFree(info);
            gdTextSetError(err, GD_TEXT_E_LAYOUT, ft_err, "Could not load glyph for layout");
            return 0;
        }
        if (delta.x != 0 && i > 0)
            info[i - 1].x_advance += delta.x;
        info[i].index = glyph_index;
        info[i].x_advance = face->glyph->advance.x;
        info[i].y_advance = face->glyph->advance.y;
        info[i].x_offset = 0;
        info[i].y_offset = 0;
        info[i].cluster = (uint32_t)i;
        previous = (text[i] == '\r' || text[i] == '\n') ? 0 : glyph_index;
    }

    *out_info = info;
    *out_count = len;
    return 1;
}

#ifdef HAVE_LIBRAQM
static int gdTextLayoutRaqm(FT_Face face, const uint32_t *text, size_t len,
                            gdTextGlyphInfo **out_info, size_t *out_count, gdTextError *err)
{
    size_t i;
    size_t count = 0;
    raqm_t *rq;
    raqm_glyph_t *glyphs;
    gdTextGlyphInfo *info;

    rq = raqm_create();
    if (!rq || !raqm_set_text(rq, text, len) || !raqm_set_freetype_face(rq, face) ||
        !raqm_set_par_direction(rq, RAQM_DIRECTION_DEFAULT) || !raqm_layout(rq)) {
        raqm_destroy(rq);
        gdTextSetError(err, GD_TEXT_E_LAYOUT, 0, "Could not shape text with RAQM");
        return 0;
    }
    glyphs = raqm_get_glyphs(rq, &count);
    if (!glyphs) {
        raqm_destroy(rq);
        gdTextSetError(err, GD_TEXT_E_LAYOUT, 0, "Could not read RAQM glyphs");
        return 0;
    }
    info = gdMalloc(sizeof(gdTextGlyphInfo) * (count ? count : 1));
    if (!info) {
        raqm_destroy(rq);
        gdTextSetError(err, GD_TEXT_E_MEMORY, 0, "Out of memory");
        return 0;
    }
    for (i = 0; i < count; i++) {
        info[i].index = glyphs[i].index;
        info[i].x_advance = glyphs[i].x_advance;
        info[i].y_advance = glyphs[i].y_advance;
        info[i].x_offset = glyphs[i].x_offset;
        info[i].y_offset = glyphs[i].y_offset;
        info[i].cluster = glyphs[i].cluster;
    }
    raqm_destroy(rq);
    *out_info = info;
    *out_count = count;
    return 1;
}
#endif

static int gdTextLayout(FT_Face face, const uint32_t *text, size_t len, gdTextShaping shaping,
                        gdTextGlyphInfo **out_info, size_t *out_count, gdTextError *err)
{
    if (shaping == GD_TEXT_SHAPING_RAQM) {
#ifdef HAVE_LIBRAQM
        return gdTextLayoutRaqm(face, text, len, out_info, out_count, err);
#else
        gdTextSetError(err, GD_TEXT_E_UNAVAILABLE, 0, "RAQM text shaping is not available");
        return 0;
#endif
    }
    return gdTextLayoutSimple(face, text, len, out_info, out_count, err);
}

static void gdTextUpdateBounds(int *has_bounds, double *min_x, double *min_y, double *max_x,
                               double *max_y, double x0, double y0, double x1, double y1)
{
    if (!*has_bounds) {
        *min_x = x0;
        *max_x = x1;
        *min_y = y0;
        *max_y = y1;
        *has_bounds = 1;
        return;
    }
    if (x0 < *min_x)
        *min_x = x0;
    if (x1 > *max_x)
        *max_x = x1;
    if (y0 < *min_y)
        *min_y = y0;
    if (y1 > *max_y)
        *max_y = y1;
}

static int gdTextProcess(gdContextPtr context, const char *utf8, double x, double y,
                         const gdTextOptions *options, gdPathPtr append_path,
                         gdTextExtents *extents, gdTextError *err)
{
    gdStatePtr state;
    FT_Face face;
    uint32_t *text = NULL;
    size_t text_len = 0;
    gdTextGlyphInfo *info = NULL;
    size_t count = 0;
    size_t i;
    FT_Pos pen_x = 0;
    FT_Pos pen_y = 0;
    gdTextShaping shaping;
    double line_spacing;
    int has_bounds = 0;
    double min_x = 0.0, min_y = 0.0, max_x = 0.0, max_y = 0.0;

    if (!context || !utf8) {
        gdTextSetError(err, GD_TEXT_E_INVALID_ARGUMENT, 0, "Invalid text arguments");
        return 0;
    }
    state = context->state;
    if (!state || !state->font_face || state->font_size <= 0.0 || !isfinite(state->font_size)) {
        gdTextSetError(err, GD_TEXT_E_INVALID_ARGUMENT, 0, "No valid font is set on context");
        return 0;
    }
    if (!gdTextValidateOptions(options, &shaping, &line_spacing, err))
        return 0;
    if (!gdTextDecodeUtf8(utf8, &text, &text_len, err))
        return 0;
    if (text_len == 0) {
        if (extents)
            memset(extents, 0, sizeof(*extents));
        gdFree(text);
        gdTextClearError(err);
        return 1;
    }

    face = state->font_face->face;
    if (FT_Select_Charmap(face, FT_ENCODING_UNICODE)) {
        gdFree(text);
        gdTextSetError(err, GD_TEXT_E_FONT, 0, "Font does not have a Unicode charmap");
        return 0;
    }
    if (FT_Set_Char_Size(face, 0, (FT_F26Dot6)(state->font_size * 64.0), 72, 72)) {
        gdFree(text);
        gdTextSetError(err, GD_TEXT_E_FONT, 0, "Could not set font size");
        return 0;
    }
    if (!gdTextLayout(face, text, text_len, shaping, &info, &count, err)) {
        gdFree(text);
        return 0;
    }

    for (i = 0; i < count; i++) {
        FT_Error ft_err;
        FT_GlyphSlot glyph;
        uint32_t ch = info[i].cluster < text_len ? text[info[i].cluster] : 0;

        if (ch == '\r') {
            pen_x = 0;
            continue;
        }
        if (ch == '\n') {
            pen_x = 0;
            pen_y -= (FT_Pos)(state->font_size * line_spacing * 64.0);
            continue;
        }

        ft_err = FT_Load_Glyph(face, info[i].index, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING);
        if (ft_err) {
            gdFree(info);
            gdFree(text);
            gdTextSetError(err, GD_TEXT_E_LAYOUT, ft_err, "Could not load glyph outline");
            return 0;
        }

        glyph = face->glyph;
        if (glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
            FT_BBox cbox;
            double gx0, gy0, gx1, gy1;

            FT_Outline_Get_CBox(&glyph->outline, &cbox);
            gx0 = x + FT_POS_TO_DOUBLE(pen_x + info[i].x_offset + cbox.xMin);
            gx1 = x + FT_POS_TO_DOUBLE(pen_x + info[i].x_offset + cbox.xMax);
            gy0 = y - FT_POS_TO_DOUBLE(pen_y - info[i].y_offset + cbox.yMax);
            gy1 = y - FT_POS_TO_DOUBLE(pen_y - info[i].y_offset + cbox.yMin);
            gdTextUpdateBounds(&has_bounds, &min_x, &min_y, &max_x, &max_y, gx0, gy0, gx1, gy1);

            if (append_path) {
                gdTextOutlineBuilder builder;
                FT_Outline_Funcs funcs;

                builder.path = append_path;
                builder.origin_x = x;
                builder.origin_y = y;
                builder.pen_x = pen_x;
                builder.pen_y = pen_y;
                builder.x_offset = info[i].x_offset;
                builder.y_offset = info[i].y_offset;
                builder.started = 0;
                funcs.move_to = gdTextMoveTo;
                funcs.line_to = gdTextLineTo;
                funcs.conic_to = gdTextConicTo;
                funcs.cubic_to = gdTextCubicTo;
                funcs.shift = 0;
                funcs.delta = 0;
                ft_err = FT_Outline_Decompose(&glyph->outline, &funcs, &builder);
                if (ft_err) {
                    gdFree(info);
                    gdFree(text);
                    gdTextSetError(err, GD_TEXT_E_LAYOUT, ft_err, "Could not convert glyph outline");
                    return 0;
                }
                if (builder.started)
                    gdPathClose(append_path);
            }
        }
        pen_x += info[i].x_advance;
        pen_y += info[i].y_advance;
    }

    if (extents) {
        extents->x_bearing = has_bounds ? min_x - x : 0.0;
        extents->y_bearing = has_bounds ? min_y - y : 0.0;
        extents->width = has_bounds ? max_x - min_x : 0.0;
        extents->height = has_bounds ? max_y - min_y : 0.0;
        extents->x_advance = FT_POS_TO_DOUBLE(pen_x);
        extents->y_advance = -FT_POS_TO_DOUBLE(pen_y);
    }

    gdFree(info);
    gdFree(text);
    gdTextClearError(err);
    return 1;
}

BGD_DECLARE(gdFontFacePtr)
gdFontFaceCreateFromFile(const char *path, int face_index, gdTextError *err)
{
    gdFontFacePtr face;
    FT_Error ft_err;

    if (!path || face_index < 0) {
        gdTextSetError(err, GD_TEXT_E_INVALID_ARGUMENT, 0, "Invalid font file arguments");
        return NULL;
    }
    face = gdMalloc(sizeof(gdFontFace));
    if (!face) {
        gdTextSetError(err, GD_TEXT_E_MEMORY, 0, "Out of memory");
        return NULL;
    }
    memset(face, 0, sizeof(*face));
    face->ref = 1;
    ft_err = FT_Init_FreeType(&face->library);
    if (ft_err) {
        gdFree(face);
        gdTextSetError(err, GD_TEXT_E_FONT, ft_err, "Could not initialize FreeType");
        return NULL;
    }
    ft_err = FT_New_Face(face->library, path, face_index, &face->face);
    if (ft_err) {
        FT_Done_FreeType(face->library);
        gdFree(face);
        gdTextSetError(err, GD_TEXT_E_FONT, ft_err, "Could not load font file");
        return NULL;
    }
    gdTextClearError(err);
    return face;
}

BGD_DECLARE(gdFontFacePtr)
gdFontFaceCreateFromData(const unsigned char *data, size_t size, int face_index, gdTextError *err)
{
    gdFontFacePtr face;
    FT_Error ft_err;

    if (!data || size == 0 || face_index < 0) {
        gdTextSetError(err, GD_TEXT_E_INVALID_ARGUMENT, 0, "Invalid font data arguments");
        return NULL;
    }
    face = gdMalloc(sizeof(gdFontFace));
    if (!face) {
        gdTextSetError(err, GD_TEXT_E_MEMORY, 0, "Out of memory");
        return NULL;
    }
    memset(face, 0, sizeof(*face));
    face->ref = 1;
    ft_err = FT_Init_FreeType(&face->library);
    if (ft_err) {
        gdFree(face);
        gdTextSetError(err, GD_TEXT_E_FONT, ft_err, "Could not initialize FreeType");
        return NULL;
    }
    ft_err = FT_New_Memory_Face(face->library, data, (FT_Long)size, face_index, &face->face);
    if (ft_err) {
        FT_Done_FreeType(face->library);
        gdFree(face);
        gdTextSetError(err, GD_TEXT_E_FONT, ft_err, "Could not load font data");
        return NULL;
    }
    gdTextClearError(err);
    return face;
}

BGD_DECLARE(gdFontFacePtr) gdFontFaceAddRef(gdFontFacePtr face)
{
    if (!face)
        return NULL;
    face->ref++;
    return face;
}

BGD_DECLARE(void) gdFontFaceDestroy(gdFontFacePtr face)
{
    if (!face)
        return;
    face->ref--;
    if (face->ref == 0) {
        if (face->face)
            FT_Done_Face(face->face);
        if (face->library)
            FT_Done_FreeType(face->library);
        gdFree(face);
    }
}

BGD_DECLARE(void) gdContextSetFontFace(gdContextPtr context, gdFontFacePtr face)
{
    if (!context || !context->state)
        return;
    face = gdFontFaceAddRef(face);
    gdFontFaceDestroy(context->state->font_face);
    context->state->font_face = face;
}

BGD_DECLARE(void) gdContextSetFontSize(gdContextPtr context, double size)
{
    if (!context || !context->state || !isfinite(size) || size <= 0.0)
        return;
    context->state->font_size = size;
}

BGD_DECLARE(gdTextStatus)
gdContextTextPath(gdContextPtr context, const char *utf8, double x, double y,
                  const gdTextOptions *options, gdTextError *err)
{
    gdTextError local_err;
    gdTextError *target_err = err ? err : &local_err;
    gdPathPtr path;

    gdTextClearError(target_err);
    if (!context) {
        return gdTextSetError(target_err, GD_TEXT_E_INVALID_ARGUMENT, 0, "Invalid text arguments");
    }
    path = gdPathCreate();
    if (!path) {
        return gdTextSetError(target_err, GD_TEXT_E_MEMORY, 0, "Out of memory");
    }
    if (!gdTextProcess(context, utf8, x, y, options, path, NULL, target_err)) {
        gdTextStatus status = target_err->code;
        gdPathDestroy(path);
        return status;
    }
    gdContextAppendPath(context, path);
    gdPathDestroy(path);
    return gdTextClearError(target_err);
}

BGD_DECLARE(gdTextStatus)
gdContextShowText(gdContextPtr context, const char *utf8, double x, double y,
                  const gdTextOptions *options, gdTextError *err)
{
    gdTextStatus status = gdContextTextPath(context, utf8, x, y, options, err);

    if (status != GD_TEXT_OK)
        return status;
    gdContextFill(context);
    return GD_TEXT_OK;
}

BGD_DECLARE(gdTextStatus)
gdContextTextExtents(gdContextPtr context, const char *utf8, const gdTextOptions *options,
                     gdTextExtents *extents, gdTextError *err)
{
    gdTextError local_err;
    gdTextError *target_err = err ? err : &local_err;

    gdTextClearError(target_err);
    if (!extents) {
        return gdTextSetError(target_err, GD_TEXT_E_INVALID_ARGUMENT, 0,
                              "Text extents output is NULL");
    }
    if (!gdTextProcess(context, utf8, 0.0, 0.0, options, NULL, extents, target_err))
        return target_err->code;
    return gdTextClearError(target_err);
}

#else

BGD_DECLARE(void) gdTextOptionsInit(gdTextOptions *options)
{
    if (!options)
        return;
    options->shaping = GD_TEXT_SHAPING_NONE;
    options->line_spacing = 1.0;
    options->reserved_flags = 0;
    options->reserved_double = 0.0;
}

static gdTextStatus gdTextUnavailable(gdTextError *err)
{
    if (err) {
        err->code = GD_TEXT_E_UNAVAILABLE;
        err->provider_code = 0;
        strncpy(err->message, "libgd was not built with FreeType font support",
                sizeof(err->message) - 1);
        err->message[sizeof(err->message) - 1] = '\0';
    }
    return GD_TEXT_E_UNAVAILABLE;
}

BGD_DECLARE(gdFontFacePtr)
gdFontFaceCreateFromFile(const char *path, int face_index, gdTextError *err)
{
    (void)path;
    (void)face_index;
    gdTextUnavailable(err);
    return NULL;
}

BGD_DECLARE(gdFontFacePtr)
gdFontFaceCreateFromData(const unsigned char *data, size_t size, int face_index, gdTextError *err)
{
    (void)data;
    (void)size;
    (void)face_index;
    gdTextUnavailable(err);
    return NULL;
}

BGD_DECLARE(gdFontFacePtr) gdFontFaceAddRef(gdFontFacePtr face) { return face; }

BGD_DECLARE(void) gdFontFaceDestroy(gdFontFacePtr face) { (void)face; }

BGD_DECLARE(void) gdContextSetFontFace(gdContextPtr context, gdFontFacePtr face)
{
    (void)context;
    (void)face;
}

BGD_DECLARE(void) gdContextSetFontSize(gdContextPtr context, double size)
{
    (void)context;
    (void)size;
}

BGD_DECLARE(gdTextStatus)
gdContextTextPath(gdContextPtr context, const char *utf8, double x, double y,
                  const gdTextOptions *options, gdTextError *err)
{
    (void)context;
    (void)utf8;
    (void)x;
    (void)y;
    (void)options;
    return gdTextUnavailable(err);
}

BGD_DECLARE(gdTextStatus)
gdContextShowText(gdContextPtr context, const char *utf8, double x, double y,
                  const gdTextOptions *options, gdTextError *err)
{
    (void)context;
    (void)utf8;
    (void)x;
    (void)y;
    (void)options;
    return gdTextUnavailable(err);
}

BGD_DECLARE(gdTextStatus)
gdContextTextExtents(gdContextPtr context, const char *utf8, const gdTextOptions *options,
                     gdTextExtents *extents, gdTextError *err)
{
    (void)context;
    (void)utf8;
    (void)options;
    (void)extents;
    return gdTextUnavailable(err);
}

#endif
