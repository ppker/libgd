#include "gd_vector2d.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

static int draw_sine_text(gdContextPtr ctx, const char *text, double left, double baseline,
                          double amplitude, double cycles, gdPaintPtr fill, gdTextError *err)
{
    gdTextExtents total;
    double pen = 0.0;
    size_t i, len = strlen(text);

    if (gdContextTextExtents(ctx, text, NULL, &total, err) != GD_TEXT_OK)
        return 0;
    if (total.x_advance <= 0.0)
        return 1;

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
            return 0;

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
            if (gdContextTextPath(ctx, ch, -ext.x_advance * 0.5, 0.0, NULL, err) != GD_TEXT_OK) {
                gdContextRestore(ctx);
                return 0;
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
    return 1;
}

int main(int argc, char **argv)
{
    const int width = 1200;
    const int height = 320;
    const char *text = "Text follows a curve with gd 2D paths";
    gdImagePtr im;
    gdImagePtr tile;
    gdContextPtr ctx;
    gdFontFacePtr face;
    gdPathPatternPtr pattern;
    gdPaintPtr paint;
    gdTextError err = {0};
    gdTextExtents extents;
    FILE *out;
    double left;

    if (argc != 3) {
        fprintf(stderr, "usage: %s font.ttf output.png\n", argv[0]);
        return 1;
    }

    im = gdImageCreateTrueColor(width, height);
    tile = create_pattern_tile();
    if (!im || !tile)
        return 1;

    gdImageAlphaBlending(im, 0);
    gdImageSaveAlpha(im, 1);
    gdImageFilledRectangle(im, 0, 0, width - 1, height - 1, gdTrueColorAlpha(0, 0, 0, 127));
    gdImageAlphaBlending(im, 1);

    ctx = gdContextCreateForImage(im);
    face = gdFontFaceCreateFromFile(argv[1], 0, &err);
    if (!ctx || !face) {
        fprintf(stderr, "%s\n", err.message[0] ? err.message : "failed to initialize text example");
        return 1;
    }

    gdContextSetFontFace(ctx, face);
    gdContextSetFontSize(ctx, 54);
    if (gdContextTextExtents(ctx, text, NULL, &extents, &err) != GD_TEXT_OK) {
        fprintf(stderr, "%s\n", err.message[0] ? err.message : "failed to measure text");
        return 1;
    }
    left = (width - extents.x_advance) * 0.5;

    pattern = gdPathPatternCreateForImage(tile);
    gdPathPatternSetExtend(pattern, GD_EXTEND_REPEAT);
    paint = gdPaintCreateFromPattern(pattern);
    if (!pattern || !paint)
        return 1;

    if (!draw_sine_text(ctx, text, left, 178, 52, 1.35, paint, &err)) {
        fprintf(stderr, "%s\n", err.message[0] ? err.message : "failed to render text");
        return 1;
    }

    gdContextFlushImage(ctx);

    out = fopen(argv[2], "wb");
    if (!out) {
        fprintf(stderr, "could not open output file\n");
        return 1;
    }
    gdImagePng(im, out);
    fclose(out);

    gdPaintDestroy(paint);
    gdPathPatternDestroy(pattern);
    gdFontFaceDestroy(face);
    gdContextDestroy(ctx);
    gdImageDestroy(tile);
    gdImageDestroy(im);
    return 0;
}
