#include "config.h"
#include "gd_vector2d.h"
#include "gdtest.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define TEXT_CURVE_WIDTH 1200
#define TEXT_CURVE_HEIGHT 320
#define TEXT_CURVE_THRESHOLD 0.05
#define TEXT_CURVE_MAX_PIXELS_CHANGED 800

static gdImagePtr create_pattern_tile(void)
{
    gdImagePtr tile = gdImageCreateTrueColor(48, 48);
    int blue = gdTrueColor(32, 132, 255);
    int cyan = gdTrueColor(72, 224, 230);
    int gold = gdTrueColor(255, 206, 72);
    int pink = gdTrueColor(255, 106, 166);

    if (!tile)
        return NULL;

    gdImageFilledRectangle(tile, 0, 0, 47, 47, blue);
    gdImageFilledRectangle(tile, 0, 0, 47, 15, pink);
    gdImageFilledRectangle(tile, 0, 32, 47, 47, cyan);
    gdImageFilledRectangle(tile, 0, 0, 11, 47, gold);
    gdImageFilledRectangle(tile, 28, 0, 35, 47, gdTrueColor(255, 245, 185));
    return tile;
}

static const char *text_error_message(const gdTextError *err, const char *fallback)
{
    return err->message[0] ? err->message : fallback;
}

static int draw_sine_text(gdContextPtr ctx, const char *text, double left, double baseline,
                          double amplitude, double cycles, gdPaintPtr fill, gdTextError *err)
{
    gdTextExtents total;
    double pen = 0.0;
    size_t i, len = strlen(text);

    if (gdContextTextExtents(ctx, text, NULL, &total, err) != GD_TEXT_OK)
        return GD_FALSE;
    if (total.x_advance <= 0.0)
        return GD_TRUE;

    for (i = 0; i < len; i++) {
        char ch[2] = {text[i], 0};
        gdTextExtents ext;
        double center;
        double phase;
        double x;
        double y;
        double slope;
        double angle;

        if (gdContextTextExtents(ctx, ch, NULL, &ext, err) != GD_TEXT_OK)
            return GD_FALSE;

        center = pen + ext.x_advance * 0.5;
        phase = (center / total.x_advance) * (2.0 * M_PI * cycles);
        x = left + center;
        y = baseline + amplitude * sin(phase);
        slope = amplitude * (2.0 * M_PI * cycles / total.x_advance) * cos(phase);
        angle = atan(slope);

        if (ch[0] != ' ') {
            gdContextSave(ctx);
            gdContextTranslate(ctx, x, y);
            gdContextRotate(ctx, angle);
            if (gdContextTextPath(ctx, ch, -ext.x_advance * 0.5, 0.0, NULL, err) !=
                GD_TEXT_OK) {
                gdContextRestore(ctx);
                return GD_FALSE;
            }
            gdContextSetSourceRgba(ctx, 0.03, 0.05, 0.09, 0.98);
            gdContextSetLineJoin(ctx, gdLineJoinRound);
            gdContextSetLineWidth(ctx, 7.0);
            gdContextStrokePreserve(ctx);
            gdContextSetSource(ctx, fill);
            gdContextFillPreserve(ctx);
            gdContextSetSourceRgba(ctx, 0.03, 0.05, 0.09, 0.98);
            gdContextSetLineJoin(ctx, gdLineJoinRound);
            gdContextSetLineWidth(ctx, 2.2);
            gdContextStroke(ctx);
            gdContextRestore(ctx);
        }
        pen += ext.x_advance;
    }
    return GD_TRUE;
}

static int write_png(const char *path, gdImagePtr im)
{
    FILE *fp = fopen(path, "wb");

    if (!fp)
        return GD_FALSE;
    gdImagePng(im, fp);
    fclose(fp);
    return GD_TRUE;
}

static gdImagePtr render_text_curve(void)
{
    const char *text = "Text follows a curve with gd 2D paths";
    gdImagePtr im = NULL;
    gdImagePtr tile = NULL;
    gdContextPtr ctx = NULL;
    gdFontFacePtr face = NULL;
    gdPathPatternPtr pattern = NULL;
    gdPaintPtr paint = NULL;
    gdTextError err = {0};
    gdTextExtents extents;
    char *font = NULL;
    double left;

    im = gdImageCreateTrueColor(TEXT_CURVE_WIDTH, TEXT_CURVE_HEIGHT);
    tile = create_pattern_tile();
    font = gdTestFilePath("freetype/DejaVuSans.ttf");
    if (!im || !tile || !font)
        goto fail;

    gdImageAlphaBlending(im, 0);
    gdImageSaveAlpha(im, 1);
    gdImageFilledRectangle(im, 0, 0, TEXT_CURVE_WIDTH - 1, TEXT_CURVE_HEIGHT - 1,
                           gdTrueColorAlpha(0, 0, 0, 127));
    gdImageAlphaBlending(im, 1);

    ctx = gdContextCreateForImage(im);
    face = gdFontFaceCreateFromFile(font, 0, &err);
    if (!ctx || !face) {
        gdTestErrorMsg("%s\n", text_error_message(&err, "failed to initialize text curve"));
        goto fail;
    }

    gdContextSetFontFace(ctx, face);
    gdContextSetFontSize(ctx, 54);
    if (gdContextTextExtents(ctx, text, NULL, &extents, &err) != GD_TEXT_OK) {
        gdTestErrorMsg("%s\n", text_error_message(&err, "failed to measure text curve"));
        goto fail;
    }
    left = (TEXT_CURVE_WIDTH - extents.x_advance) * 0.5;

    pattern = gdPathPatternCreateForImage(tile);
    if (pattern)
        gdPathPatternSetExtend(pattern, GD_EXTEND_REPEAT);
    paint = pattern ? gdPaintCreateFromPattern(pattern) : NULL;
    if (!paint)
        goto fail;

    if (!draw_sine_text(ctx, text, left, 178, 52, 1.35, paint, &err)) {
        gdTestErrorMsg("%s\n", text_error_message(&err, "failed to render text curve"));
        goto fail;
    }

    gdContextFlushImage(ctx);

    gdPaintDestroy(paint);
    gdPathPatternDestroy(pattern);
    gdFontFaceDestroy(face);
    gdContextDestroy(ctx);
    gdImageDestroy(tile);
    free(font);
    return im;

fail:
    if (paint)
        gdPaintDestroy(paint);
    if (pattern)
        gdPathPatternDestroy(pattern);
    if (face)
        gdFontFaceDestroy(face);
    if (ctx)
        gdContextDestroy(ctx);
    if (tile)
        gdImageDestroy(tile);
    if (im)
        gdImageDestroy(im);
    if (font)
        free(font);
    return NULL;
}

int main(void)
{
    gdImagePtr actual;
    char *ref_path = NULL;
    int error = 0;

    putenv("FREETYPE_PROPERTIES=truetype:interpreter-version=35");

    actual = render_text_curve();
    if (!gdTestAssert(actual != NULL))
        return gdNumFailures();

    ref_path = gdTestFilePath("vector2d/text_curve_reference.png");
    if (getenv("GD_UPDATE_VECTOR2D_TEXT_CURVE_FIXTURE") != NULL) {
        if (!write_png(ref_path, actual)) {
            gdTestErrorMsg("failed to write vector2d text curve reference image\n");
            error = 1;
        }
        goto done;
    }

    if (!gdAssertImageEqualsToFilePerceptual("vector2d/text_curve_reference.png", actual,
                                             TEXT_CURVE_THRESHOLD,
                                             TEXT_CURVE_MAX_PIXELS_CHANGED)) {
        error = 1;
    }

done:
    if (ref_path)
        free(ref_path);
    gdImageDestroy(actual);
    return error || gdNumFailures();
}
