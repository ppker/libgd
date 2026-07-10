#include "config.h"
#include "gd_vector2d.h"
#include "gdtest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int count_changed_pixels(gdImagePtr im, int bg)
{
    int changed = 0;
    int x, y;

    for (y = 0; y < gdImageSY(im); y++) {
        for (x = 0; x < gdImageSX(im); x++) {
            if (gdImageGetTrueColorPixel(im, x, y) != bg)
                changed++;
        }
    }
    return changed;
}

static unsigned char *read_file(const char *path, size_t *size)
{
    FILE *fp;
    unsigned char *data;
    long len;

    fp = fopen(path, "rb");
    if (!fp)
        return NULL;
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }
    len = ftell(fp);
    if (len <= 0) {
        fclose(fp);
        return NULL;
    }
    rewind(fp);
    data = malloc((size_t)len);
    if (!data) {
        fclose(fp);
        return NULL;
    }
    if (fread(data, 1, (size_t)len, fp) != (size_t)len) {
        free(data);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    *size = (size_t)len;
    return data;
}

static const char *text_error_message(const gdTextError *err, const char *fallback)
{
    return err->message[0] ? err->message : fallback;
}

static gdFontFacePtr load_test_face(void)
{
    char *font = gdTestFilePath("freetype/DejaVuSans.ttf");
    gdTextError err = {0};
    gdFontFacePtr face = gdFontFaceCreateFromFile(font, 0, &err);

    gdTestAssertMsg(face != NULL, "%s\n", text_error_message(&err, "font load failed"));
    free(font);
    return face;
}

static void test_options_init_defaults(void)
{
    gdTextOptions options;

    memset(&options, 0x7f, sizeof(options));
    gdTextOptionsInit(&options);
    gdTestAssert(options.shaping == GD_TEXT_SHAPING_NONE);
    gdTestAssert(options.line_spacing == 1.0);
    gdTestAssert(options.reserved_flags == 0);
    gdTestAssert(options.reserved_double == 0.0);
}

static void test_show_text_renders_pixels(void)
{
    int bg = gdTrueColor(255, 255, 255);
    gdImagePtr im = gdImageCreateTrueColor(180, 80);
    gdContextPtr ctx;
    gdFontFacePtr face = load_test_face();
    gdTextError err = {0};

    gdImageFilledRectangle(im, 0, 0, 179, 79, bg);
    ctx = gdContextCreateForImage(im);
    gdTestAssert(ctx != NULL);
    gdContextSetFontFace(ctx, face);
    gdFontFaceDestroy(face);
    gdContextSetFontSize(ctx, 36);
    gdContextSetSourceRgb(ctx, 0.0, 0.0, 0.0);
    gdTestAssertMsg(gdContextShowText(ctx, "Hello", 10, 50, NULL, &err) == GD_TEXT_OK,
                    "%s\n", text_error_message(&err, "show text failed"));
    gdContextFlushImage(ctx);
    gdTestAssert(count_changed_pixels(im, bg) > 100);
    gdContextDestroy(ctx);
    gdImageDestroy(im);
}

static void test_text_path_can_be_filled_and_stroked(void)
{
    int bg = gdTrueColor(255, 255, 255);
    gdImagePtr im = gdImageCreateTrueColor(180, 80);
    gdContextPtr ctx;
    gdFontFacePtr face = load_test_face();
    gdTextError err = {0};
    int filled;

    gdImageFilledRectangle(im, 0, 0, 179, 79, bg);
    ctx = gdContextCreateForImage(im);
    gdContextSetFontFace(ctx, face);
    gdFontFaceDestroy(face);
    gdContextSetFontSize(ctx, 38);
    gdTestAssertMsg(gdContextTextPath(ctx, "Path", 8, 54, NULL, &err) == GD_TEXT_OK,
                    "%s\n", text_error_message(&err, "text path failed"));
    gdContextSetSourceRgb(ctx, 0.0, 0.2, 0.8);
    gdContextFillPreserve(ctx);
    gdContextSetSourceRgb(ctx, 0.0, 0.0, 0.0);
    gdContextSetLineWidth(ctx, 1.0);
    gdContextStroke(ctx);
    gdContextFlushImage(ctx);
    filled = count_changed_pixels(im, bg);
    gdTestAssert(filled > 100);
    gdContextDestroy(ctx);
    gdImageDestroy(im);
}

static void test_text_extents_do_not_mutate_path(void)
{
    int bg = gdTrueColor(255, 255, 255);
    gdImagePtr im = gdImageCreateTrueColor(160, 70);
    gdContextPtr ctx;
    gdFontFacePtr face = load_test_face();
    gdTextExtents extents;
    gdTextError err = {0};

    gdImageFilledRectangle(im, 0, 0, 159, 69, bg);
    ctx = gdContextCreateForImage(im);
    gdContextSetFontFace(ctx, face);
    gdFontFaceDestroy(face);
    gdContextSetFontSize(ctx, 24);
    gdTestAssertMsg(gdContextTextExtents(ctx, "Metrics", NULL, &extents, &err) == GD_TEXT_OK,
                    "%s\n", text_error_message(&err, "text extents failed"));
    gdTestAssert(extents.width > 0.0);
    gdTestAssert(extents.height > 0.0);
    gdTestAssert(extents.x_advance > extents.width * 0.5);
    gdContextSetSourceRgb(ctx, 1.0, 0.0, 0.0);
    gdContextFill(ctx);
    gdContextFlushImage(ctx);
    gdTestAssert(count_changed_pixels(im, bg) == 0);
    gdContextDestroy(ctx);
    gdImageDestroy(im);
}

static void test_save_restore_retains_font_face(void)
{
    int bg = gdTrueColor(255, 255, 255);
    gdImagePtr im = gdImageCreateTrueColor(160, 80);
    gdContextPtr ctx;
    gdFontFacePtr face = load_test_face();
    gdTextError err = {0};

    gdImageFilledRectangle(im, 0, 0, 159, 79, bg);
    ctx = gdContextCreateForImage(im);
    gdContextSetFontFace(ctx, face);
    gdFontFaceDestroy(face);
    gdContextSetFontSize(ctx, 26);
    gdTestAssert(gdContextSave(ctx));
    gdContextSetFontSize(ctx, 10);
    gdTestAssert(gdContextRestore(ctx));
    gdContextSetSourceRgb(ctx, 0.0, 0.0, 0.0);
    gdTestAssertMsg(gdContextShowText(ctx, "Saved", 8, 50, NULL, &err) == GD_TEXT_OK,
                    "%s\n", text_error_message(&err, "save/restore font failed"));
    gdContextFlushImage(ctx);
    gdTestAssert(count_changed_pixels(im, bg) > 50);
    gdContextDestroy(ctx);
    gdImageDestroy(im);
}

static void test_font_from_data_and_errors(void)
{
    char *font = gdTestFilePath("freetype/DejaVuSans.ttf");
    size_t size = 0;
    unsigned char *data = read_file(font, &size);
    gdTextError err = {0};
    gdFontFacePtr face;

    gdTestAssert(data != NULL);
    face = gdFontFaceCreateFromData(data, size, 0, &err);
    gdTestAssertMsg(face != NULL, "%s\n", text_error_message(&err, "font data load failed"));
    gdFontFaceDestroy(face);
    face = gdFontFaceCreateFromFile(font, 99999, &err);
    gdTestAssert(face == NULL);
    gdTestAssert(err.code == GD_TEXT_E_FONT);
    free(data);
    free(font);
}

static void test_invalid_utf8_and_empty_string(void)
{
    gdImagePtr im = gdImageCreateTrueColor(80, 50);
    gdContextPtr ctx = gdContextCreateForImage(im);
    gdFontFacePtr face = load_test_face();
    gdTextExtents extents;
    gdTextError err = {0};

    gdContextSetFontFace(ctx, face);
    gdFontFaceDestroy(face);
    gdContextSetFontSize(ctx, 20);
    gdTestAssert(gdContextTextExtents(ctx, "\xC0\xAF", NULL, &extents, &err) == GD_TEXT_E_INVALID_ARGUMENT);
    gdTestAssert(err.code == GD_TEXT_E_INVALID_ARGUMENT);
    gdTestAssert(err.message[0] != '\0');
    memset(&extents, 1, sizeof(extents));
    gdTestAssert(gdContextTextExtents(ctx, "", NULL, &extents, &err) == GD_TEXT_OK);
    gdTestAssert(err.code == GD_TEXT_OK);
    gdTestAssert(err.message[0] == '\0');
    gdTestAssert(extents.width == 0.0 && extents.height == 0.0 && extents.x_advance == 0.0);
    gdContextDestroy(ctx);
    gdImageDestroy(im);
}

static void test_raqm_is_explicit(void)
{
    gdImagePtr im = gdImageCreateTrueColor(100, 50);
    gdContextPtr ctx = gdContextCreateForImage(im);
    gdFontFacePtr face = load_test_face();
    gdTextOptions options;
    gdTextExtents extents;
    gdTextError err = {0};

    gdContextSetFontFace(ctx, face);
    gdFontFaceDestroy(face);
    gdContextSetFontSize(ctx, 20);
    gdTextOptionsInit(&options);
    options.shaping = GD_TEXT_SHAPING_RAQM;
#ifdef HAVE_LIBRAQM
    gdTestAssertMsg(gdContextTextExtents(ctx, "abc", &options, &extents, &err) == GD_TEXT_OK,
                    "%s\n", text_error_message(&err, "RAQM layout failed"));
#else
    gdTestAssert(gdContextTextExtents(ctx, "abc", &options, &extents, &err) == GD_TEXT_E_UNAVAILABLE);
    gdTestAssert(err.code == GD_TEXT_E_UNAVAILABLE);
#endif
    gdContextDestroy(ctx);
    gdImageDestroy(im);
}

int main(void)
{
    test_options_init_defaults();
    test_show_text_renders_pixels();
    test_text_path_can_be_filled_and_stroked();
    test_text_extents_do_not_mutate_path();
    test_save_restore_retains_font_face();
    test_font_from_data_and_errors();
    test_invalid_utf8_and_empty_string();
    test_raqm_is_explicit();
    return gdNumFailures();
}
