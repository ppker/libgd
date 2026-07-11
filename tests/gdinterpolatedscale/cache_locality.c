#include "gd.h"
#include "gdtest.h"

static void assert_gray_between(int pixel, int min, int max, const char *label)
{
	int red = gdTrueColorGetRed(pixel);
	int green = gdTrueColorGetGreen(pixel);
	int blue = gdTrueColorGetBlue(pixel);
	int alpha = gdTrueColorGetAlpha(pixel);

	gdTestAssertMsg(red >= min && red <= max,
					"%s: expected red between %d and %d, got %d in %x", label,
					min, max, red, pixel);
	gdTestAssertMsg(green >= min && green <= max,
					"%s: expected green between %d and %d, got %d in %x",
					label, min, max, green, pixel);
	gdTestAssertMsg(blue >= min && blue <= max,
					"%s: expected blue between %d and %d, got %d in %x", label,
					min, max, blue, pixel);
	gdTestAssertMsg(alpha == gdAlphaOpaque,
					"%s: expected opaque alpha, got %d in %x", label, alpha,
					pixel);
}

static void test_vertical_only_truecolor(void)
{
	gdImagePtr src, dst;

	src = gdImageCreateTrueColor(1, 2);
	gdTestAssert(src != NULL);
	if (src == NULL) {
		return;
	}

	src->tpixels[0][0] = gdTrueColorAlpha(0, 0, 0, gdAlphaOpaque);
	src->tpixels[1][0] = gdTrueColorAlpha(255, 255, 255, gdAlphaOpaque);
	gdImageSetInterpolationMethod(src, GD_BOX);

	dst = gdImageScale(src, 1, 1);
	gdTestAssert(dst != NULL);
	if (dst != NULL) {
		assert_gray_between(gdImageGetTrueColorPixel(dst, 0, 0), 186, 190,
							"vertical-only truecolor");
		gdImageDestroy(dst);
	}
	gdImageDestroy(src);
}

static void test_vertical_only_palette(void)
{
	gdImagePtr src, dst;
	int black, white;

	src = gdImageCreate(1, 2);
	gdTestAssert(src != NULL);
	if (src == NULL) {
		return;
	}

	black = gdImageColorAllocateAlpha(src, 0, 0, 0, gdAlphaOpaque);
	white = gdImageColorAllocateAlpha(src, 255, 255, 255, gdAlphaOpaque);
	gdImageSetPixel(src, 0, 0, black);
	gdImageSetPixel(src, 0, 1, white);
	gdImageSetInterpolationMethod(src, GD_BOX);

	dst = gdImageScale(src, 1, 1);
	gdTestAssert(dst != NULL);
	if (dst != NULL) {
		assert_gray_between(gdImageGetTrueColorPixel(dst, 0, 0), 186, 190,
							"vertical-only palette");
		gdImageDestroy(dst);
	}
	gdImageDestroy(src);
}

static void test_vertical_only_alpha(void)
{
	gdImagePtr src, dst;

	src = gdImageCreateTrueColor(1, 2);
	gdTestAssert(src != NULL);
	if (src == NULL) {
		return;
	}

	src->tpixels[0][0] = gdTrueColorAlpha(255, 0, 0, gdAlphaOpaque);
	src->tpixels[1][0] = gdTrueColorAlpha(0, 0, 0, gdAlphaTransparent);
	gdImageSetInterpolationMethod(src, GD_BOX);

	dst = gdImageScale(src, 1, 1);
	gdTestAssert(dst != NULL);
	if (dst != NULL) {
		const int pixel = gdImageGetTrueColorPixel(dst, 0, 0);

		gdTestAssertMsg(gdTrueColorGetRed(pixel) > 240,
						"vertical alpha fade should remain red, got %x",
						pixel);
		gdTestAssertMsg(gdTrueColorGetGreen(pixel) < 10,
						"vertical alpha fade green should stay near zero, got %x",
						pixel);
		gdTestAssertMsg(gdTrueColorGetBlue(pixel) < 10,
						"vertical alpha fade blue should stay near zero, got %x",
						pixel);
		gdTestAssertMsg(gdTrueColorGetAlpha(pixel) >= 60 &&
							gdTrueColorGetAlpha(pixel) <= 67,
						"vertical alpha fade should be about half transparent, got %d in %x",
						gdTrueColorGetAlpha(pixel), pixel);
		gdImageDestroy(dst);
	}
	gdImageDestroy(src);
}

static void test_two_axis_checkerboard(void)
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
		assert_gray_between(gdImageGetTrueColorPixel(dst, 0, 0), 186, 190,
							"two-axis checkerboard");
		gdImageDestroy(dst);
	}
	gdImageDestroy(src);
}

static void test_vertical_gradient_remains_ordered(void)
{
	gdImagePtr src, dst;
	int y;

	src = gdImageCreateTrueColor(3, 8);
	gdTestAssert(src != NULL);
	if (src == NULL) {
		return;
	}

	for (y = 0; y < gdImageSY(src); y++) {
		int x;
		const int v = y * 255 / (gdImageSY(src) - 1);

		for (x = 0; x < gdImageSX(src); x++) {
			src->tpixels[y][x] = gdTrueColorAlpha(v, v, v, gdAlphaOpaque);
		}
	}
	gdImageSetInterpolationMethod(src, GD_LANCZOS3);

	dst = gdImageScale(src, 3, 3);
	gdTestAssert(dst != NULL);
	if (dst != NULL) {
		int previous = -1;

		for (y = 0; y < gdImageSY(dst); y++) {
			const int left = gdImageGetTrueColorPixel(dst, 0, y);
			const int middle = gdImageGetTrueColorPixel(dst, 1, y);
			const int right = gdImageGetTrueColorPixel(dst, 2, y);
			const int gray = gdTrueColorGetRed(middle);

			gdTestAssertMsg(left == middle && middle == right,
							"vertical gradient row %d should remain column-uniform", y);
			gdTestAssertMsg(gray > previous,
							"vertical gradient should increase by row, got %d after %d",
							gray, previous);
			gdTestAssertMsg(gdTrueColorGetAlpha(middle) == gdAlphaOpaque,
							"vertical gradient alpha should stay opaque");
			previous = gray;
		}
		gdImageDestroy(dst);
	}
	gdImageDestroy(src);
}

int main(void)
{
	test_vertical_only_truecolor();
	test_vertical_only_palette();
	test_vertical_only_alpha();
	test_two_axis_checkerboard();
	test_vertical_gradient_remains_ordered();

	return gdNumFailures();
}
