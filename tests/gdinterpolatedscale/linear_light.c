#include "gd.h"
#include "gdtest.h"

static void assert_linear_midpoint(int pixel, const char *label)
{
	int red = gdTrueColorGetRed(pixel);
	int green = gdTrueColorGetGreen(pixel);
	int blue = gdTrueColorGetBlue(pixel);
	int alpha = gdTrueColorGetAlpha(pixel);

	gdTestAssertMsg(red >= 186 && red <= 190,
					"%s: expected red linear midpoint near 188, got %d in %x",
					label, red, pixel);
	gdTestAssertMsg(green >= 186 && green <= 190,
					"%s: expected green linear midpoint near 188, got %d in %x",
					label, green, pixel);
	gdTestAssertMsg(blue >= 186 && blue <= 190,
					"%s: expected blue linear midpoint near 188, got %d in %x",
					label, blue, pixel);
	gdTestAssertMsg(alpha == gdAlphaOpaque,
					"%s: expected opaque midpoint, got alpha %d in %x", label,
					alpha, pixel);
}

static void test_one_axis_midpoint(void)
{
	gdImagePtr src, dst;

	src = gdImageCreateTrueColor(2, 1);
	gdTestAssert(src != NULL);
	if (src == NULL) {
		return;
	}

	src->tpixels[0][0] = gdTrueColorAlpha(0, 0, 0, gdAlphaOpaque);
	src->tpixels[0][1] = gdTrueColorAlpha(255, 255, 255, gdAlphaOpaque);
	gdImageSetInterpolationMethod(src, GD_BOX);

	dst = gdImageScale(src, 1, 1);
	gdTestAssert(dst != NULL);
	if (dst != NULL) {
		assert_linear_midpoint(gdImageGetTrueColorPixel(dst, 0, 0),
							   "one-axis box downscale");
		gdImageDestroy(dst);
	}
	gdImageDestroy(src);
}

static void test_two_axis_midpoint(void)
{
	gdImagePtr src, dst;

	src = gdImageCreateTrueColor(2, 2);
	gdTestAssert(src != NULL);
	if (src == NULL) {
		return;
	}

	src->tpixels[0][0] = gdTrueColorAlpha(0, 0, 0, gdAlphaOpaque);
	src->tpixels[0][1] = gdTrueColorAlpha(255, 255, 255, gdAlphaOpaque);
	src->tpixels[1][0] = gdTrueColorAlpha(255, 255, 255, gdAlphaOpaque);
	src->tpixels[1][1] = gdTrueColorAlpha(0, 0, 0, gdAlphaOpaque);
	gdImageSetInterpolationMethod(src, GD_BOX);

	dst = gdImageScale(src, 1, 1);
	gdTestAssert(dst != NULL);
	if (dst != NULL) {
		assert_linear_midpoint(gdImageGetTrueColorPixel(dst, 0, 0),
							   "two-axis box downscale");
		gdImageDestroy(dst);
	}
	gdImageDestroy(src);
}

static void test_alpha_red_fade(void)
{
	gdImagePtr src, dst;
	int pixel;

	src = gdImageCreateTrueColor(2, 1);
	gdTestAssert(src != NULL);
	if (src == NULL) {
		return;
	}

	src->tpixels[0][0] = gdTrueColorAlpha(255, 0, 0, gdAlphaOpaque);
	src->tpixels[0][1] = gdTrueColorAlpha(0, 0, 0, gdAlphaTransparent);
	gdImageSetInterpolationMethod(src, GD_BOX);

	dst = gdImageScale(src, 1, 1);
	gdTestAssert(dst != NULL);
	if (dst != NULL) {
		pixel = gdImageGetTrueColorPixel(dst, 0, 0);
		gdTestAssertMsg(gdTrueColorGetRed(pixel) > 240,
						"alpha fade should remain red, got %x", pixel);
		gdTestAssertMsg(gdTrueColorGetGreen(pixel) < 10,
						"alpha fade green should stay near zero, got %x",
						pixel);
		gdTestAssertMsg(gdTrueColorGetBlue(pixel) < 10,
						"alpha fade blue should stay near zero, got %x",
						pixel);
		gdTestAssertMsg(gdTrueColorGetAlpha(pixel) >= 60 &&
							gdTrueColorGetAlpha(pixel) <= 67,
						"alpha fade should be about half transparent, got %d in %x",
						gdTrueColorGetAlpha(pixel), pixel);
		gdImageDestroy(dst);
	}
	gdImageDestroy(src);
}

static void test_palette_midpoint(void)
{
	gdImagePtr src, dst;
	int black, white;

	src = gdImageCreate(2, 1);
	gdTestAssert(src != NULL);
	if (src == NULL) {
		return;
	}

	black = gdImageColorAllocateAlpha(src, 0, 0, 0, gdAlphaOpaque);
	white = gdImageColorAllocateAlpha(src, 255, 255, 255, gdAlphaOpaque);
	gdImageSetPixel(src, 0, 0, black);
	gdImageSetPixel(src, 1, 0, white);
	gdImageSetInterpolationMethod(src, GD_BOX);

	dst = gdImageScale(src, 1, 1);
	gdTestAssert(dst != NULL);
	if (dst != NULL) {
		assert_linear_midpoint(gdImageGetTrueColorPixel(dst, 0, 0),
							   "palette box downscale");
		gdImageDestroy(dst);
	}
	gdImageDestroy(src);
}

int main(void)
{
	test_one_axis_midpoint();
	test_two_axis_midpoint();
	test_alpha_red_fade();
	test_palette_midpoint();

	return gdNumFailures();
}
