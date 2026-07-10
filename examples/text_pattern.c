#include "gd_vector2d.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    gdImagePtr im;
    gdImagePtr tile;
    gdContextPtr ctx;
    gdFontFacePtr face;
    gdPathPatternPtr pattern;
    gdPaintPtr paint;
    gdTextError err = {0};
    FILE *out;
    int white, blue, green;

    if (argc != 3) {
        fprintf(stderr, "usage: %s font.ttf output.png\n", argv[0]);
        return 1;
    }

    im = gdImageCreateTrueColor(420, 140);
    tile = gdImageCreateTrueColor(24, 24);
    if (!im || !tile)
        return 1;

    white = gdTrueColor(255, 255, 255);
    blue = gdTrueColor(30, 80, 190);
    green = gdTrueColor(70, 170, 120);
    gdImageFilledRectangle(im, 0, 0, 419, 139, white);
    gdImageFilledRectangle(tile, 0, 0, 23, 23, blue);
    gdImageFilledRectangle(tile, 0, 0, 11, 11, green);
    gdImageFilledRectangle(tile, 12, 12, 23, 23, green);

    ctx = gdContextCreateForImage(im);
    face = gdFontFaceCreateFromFile(argv[1], 0, &err);
    if (!ctx || !face) {
        fprintf(stderr, "%s\n", err.message[0] ? err.message : "failed to initialize text example");
        return 1;
    }

    pattern = gdPathPatternCreateForImage(tile);
    gdPathPatternSetExtend(pattern, GD_EXTEND_REPEAT);
    paint = gdPaintCreateFromPattern(pattern);
    gdContextSetSource(ctx, paint);
    gdContextSetFontFace(ctx, face);
    gdContextSetFontSize(ctx, 72);

    if (gdContextTextPath(ctx, "libgd", 24, 96, NULL, &err) != GD_TEXT_OK) {
        fprintf(stderr, "%s\n", err.message[0] ? err.message : "failed to create text path");
        return 1;
    }
    gdContextFillPreserve(ctx);
    gdContextSetSourceRgb(ctx, 0.05, 0.05, 0.05);
    gdContextSetLineWidth(ctx, 2.0);
    gdContextStroke(ctx);
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
