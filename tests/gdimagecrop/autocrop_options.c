#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gd.h"
#include "gdtest.h"

static int pixel(gdImagePtr im, int x, int y)
{
	return gdImageGetTrueColorPixel(im, x, y);
}

static void assert_dimensions(gdImagePtr im, int width, int height)
{
	gdTestAssert(im != NULL);
	gdTestAssertMsg(gdImageSX(im) == width && gdImageSY(im) == height,
					"expected %dx%d, got %dx%d\n", width, height,
					im ? gdImageSX(im) : -1, im ? gdImageSY(im) : -1);
}

static void test_default_transparent_mode(void)
{
	gdImagePtr im, cropped;
	int transparent, red;

	im = gdImageCreateTrueColor(16, 14);
	gdTestAssert(im != NULL);

	transparent = gdTrueColor(10, 20, 30);
	red = gdTrueColor(200, 40, 60);
	gdImageColorTransparent(im, transparent);
	gdImageFilledRectangle(im, 0, 0, 15, 13, transparent);
	gdImageFilledRectangle(im, 4, 3, 10, 8, red);

	cropped = gdImageAutoCropWithOptions(im, NULL);
	assert_dimensions(cropped, 7, 6);
	gdTestAssert(pixel(cropped, 0, 0) == red);
	gdTestAssert(pixel(cropped, 6, 5) == red);

	gdImageDestroy(cropped);
	gdImageDestroy(im);
}

static void test_sides_mode(void)
{
	gdImagePtr im, cropped;
	gdAutoCropOptions options = {GD_CROP_SIDES, 0.5f, -1};
	int bg, blue;

	im = gdImageCreateTrueColor(18, 12);
	gdTestAssert(im != NULL);

	bg = gdTrueColor(30, 40, 50);
	blue = gdTrueColor(10, 80, 210);
	gdImageFilledRectangle(im, 0, 0, 17, 11, bg);
	gdImageFilledRectangle(im, 5, 2, 13, 8, blue);

	cropped = gdImageAutoCropWithOptions(im, &options);
	assert_dimensions(cropped, 9, 7);
	gdTestAssert(pixel(cropped, 0, 0) == blue);
	gdTestAssert(pixel(cropped, 8, 6) == blue);

	gdImageDestroy(cropped);
	gdImageDestroy(im);
}

static void test_threshold_mode(void)
{
	gdImagePtr im, cropped;
	int background, near_background, green;
	gdAutoCropOptions options;

	im = gdImageCreateTrueColor(20, 16);
	gdTestAssert(im != NULL);

	background = gdTrueColor(250, 250, 250);
	near_background = gdTrueColor(248, 249, 250);
	green = gdTrueColor(40, 180, 90);
	gdImageFilledRectangle(im, 0, 0, 19, 15, background);
	gdImageFilledRectangle(im, 1, 1, 18, 14, near_background);
	gdImageFilledRectangle(im, 6, 5, 12, 10, green);

	options.mode = GD_CROP_THRESHOLD;
	options.threshold = 0.5f;
	options.color = background;
	cropped = gdImageAutoCropWithOptions(im, &options);
	assert_dimensions(cropped, 7, 6);
	gdTestAssert(pixel(cropped, 0, 0) == green);
	gdTestAssert(pixel(cropped, 6, 5) == green);

	gdImageDestroy(cropped);
	gdImageDestroy(im);
}

static void test_alpha_preservation_and_source_unchanged(void)
{
	gdImagePtr im, cropped;
	gdAutoCropOptions options = {GD_CROP_SIDES, 0.5f, -1};
	int transparent, semi_red, source_pixel;

	im = gdImageCreateTrueColor(12, 10);
	gdTestAssert(im != NULL);
	gdImageAlphaBlending(im, gdEffectReplace);
	gdImageSaveAlpha(im, 1);

	transparent = gdTrueColorAlpha(0, 0, 0, gdAlphaTransparent);
	semi_red = gdTrueColorAlpha(220, 20, 40, 63);
	gdImageFilledRectangle(im, 0, 0, 11, 9, transparent);
	gdImageFilledRectangle(im, 3, 2, 8, 6, semi_red);
	source_pixel = pixel(im, 3, 2);

	cropped = gdImageAutoCropWithOptions(im, &options);
	assert_dimensions(cropped, 6, 5);
	gdTestAssert(pixel(cropped, 0, 0) == semi_red);
	gdTestAssert(pixel(im, 3, 2) == source_pixel);
	gdTestAssert(gdImageSX(im) == 12);
	gdTestAssert(gdImageSY(im) == 10);

	gdImageDestroy(cropped);
	gdImageDestroy(im);
}

static void test_failures(void)
{
	gdImagePtr im, palette, cropped;
	gdAutoCropOptions threshold_missing = {GD_CROP_THRESHOLD, 0.5f, -1};
	gdAutoCropOptions threshold_bad_palette = {GD_CROP_THRESHOLD, 0.5f, 100};
	gdAutoCropOptions black = {GD_CROP_BLACK, 0.5f, -1};
	int bg;

	im = gdImageCreateTrueColor(4, 4);
	gdTestAssert(im != NULL);
	gdImageFilledRectangle(im, 0, 0, 3, 3, gdTrueColor(255, 255, 255));
	cropped = gdImageAutoCropWithOptions(im, &threshold_missing);
	gdTestAssert(cropped == NULL);

	palette = gdImageCreate(4, 4);
	gdTestAssert(palette != NULL);
	bg = gdImageColorAllocate(palette, 255, 255, 255);
	gdImageFilledRectangle(palette, 0, 0, 3, 3, bg);
	cropped = gdImageAutoCropWithOptions(palette, &threshold_bad_palette);
	gdTestAssert(cropped == NULL);

	gdImageFilledRectangle(im, 0, 0, 3, 3, gdTrueColor(0, 0, 0));
	cropped = gdImageAutoCropWithOptions(im, &black);
	gdTestAssert(cropped == NULL);

	gdImageDestroy(palette);
	gdImageDestroy(im);
}

int main(void)
{
	test_default_transparent_mode();
	test_sides_mode();
	test_threshold_mode();
	test_alpha_preservation_and_source_unchanged();
	test_failures();

	return gdNumFailures();
}
