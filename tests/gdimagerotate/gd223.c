#include "gd.h"
#include "gdtest.h"

#define GD223_SIZE 64
#define GD223_BG gdTrueColor(255, 0, 0)
#define GD223_BLACK gdTrueColor(0, 0, 0)
#define GD223_WHITE gdTrueColor(255, 255, 255)

static gdImagePtr create_checker(int block_size)
{
	gdImagePtr im;
	int x, y;

	im = gdImageCreateTrueColor(GD223_SIZE, GD223_SIZE);
	gdTestAssert(im != NULL);
	if (im == NULL) {
		return NULL;
	}

	for (y = 0; y < GD223_SIZE; y++) {
		for (x = 0; x < GD223_SIZE; x++) {
			const int white = block_size == 1 ?
				((x % 2) == 0 && (y % 2) == 0) :
				(((x / block_size) + (y / block_size)) % 2) == 0;

			im->tpixels[y][x] = white ? GD223_WHITE : GD223_BLACK;
		}
	}

	return im;
}

static void assert_gd223_dimensions(gdImagePtr im, const char *label)
{
	gdTestAssertMsg(gdImageSX(im) == 92,
					"%s: expected width 92, got %d", label, gdImageSX(im));
	gdTestAssertMsg(gdImageSY(im) == 91,
					"%s: expected height 91, got %d", label, gdImageSY(im));
}

static void assert_no_transparent_pixels(gdImagePtr im, const char *label)
{
	int x, y;
	int transparent = 0;
	int max_alpha = 0;

	for (y = 0; y < gdImageSY(im); y++) {
		for (x = 0; x < gdImageSX(im); x++) {
			const int alpha = gdTrueColorGetAlpha(gdImageGetTrueColorPixel(im, x, y));

			if (alpha > max_alpha) {
				max_alpha = alpha;
			}
			if (alpha == gdAlphaTransparent) {
				transparent++;
			}
		}
	}

	gdTestAssertMsg(transparent == 0,
					"%s: expected no fully transparent pixels, got %d", label,
					transparent);
	gdTestAssertMsg(max_alpha <= 16,
					"%s: expected only opaque/near-opaque pixels, max alpha was %d",
					label, max_alpha);
}

static void assert_red_corners(gdImagePtr im, const char *label)
{
	const int w = gdImageSX(im);
	const int h = gdImageSY(im);

	gdTestAssertMsg((gdImageGetTrueColorPixel(im, 0, 0) & 0xffffff) == GD223_BG,
					"%s: top-left corner is not red", label);
	gdTestAssertMsg((gdImageGetTrueColorPixel(im, w - 1, 0) & 0xffffff) == GD223_BG,
					"%s: top-right corner is not red", label);
	gdTestAssertMsg((gdImageGetTrueColorPixel(im, 0, h - 1) & 0xffffff) == GD223_BG,
					"%s: bottom-left corner is not red", label);
	gdTestAssertMsg((gdImageGetTrueColorPixel(im, w - 1, h - 1) & 0xffffff) == GD223_BG,
					"%s: bottom-right corner is not red", label);
}

static void test_nearest_one_pixel_checker(void)
{
	gdImagePtr src, dst;
	int x, y;
	int red = 0;
	int black = 0;
	int white = 0;
	int unexpected = 0;

	src = create_checker(1);
	if (src == NULL) {
		return;
	}

	gdImageSetInterpolationMethod(src, GD_NEAREST_NEIGHBOUR);
	dst = gdImageRotateInterpolated(src, 45.0f, GD223_BG);
	gdTestAssert(dst != NULL);
	if (dst != NULL) {
		assert_gd223_dimensions(dst, "nearest gd223");
		assert_no_transparent_pixels(dst, "nearest gd223");
		assert_red_corners(dst, "nearest gd223");

		for (y = 0; y < gdImageSY(dst); y++) {
			for (x = 0; x < gdImageSX(dst); x++) {
				const int pixel = gdImageGetTrueColorPixel(dst, x, y);
				const int rgb = pixel & 0xffffff;

				if (gdTrueColorGetAlpha(pixel) != gdAlphaOpaque) {
					unexpected++;
				} else if (rgb == GD223_BG) {
					red++;
				} else if (rgb == GD223_BLACK) {
					black++;
				} else if (rgb == GD223_WHITE) {
					white++;
				} else {
					unexpected++;
				}
			}
		}

		gdTestAssertMsg(unexpected == 0,
						"nearest gd223 produced %d blended or non-opaque pixels",
						unexpected);
		gdTestAssertMsg(red > 0 && black > 0 && white > 0,
						"nearest gd223 should contain red, black, and white pixels");
		gdImageDestroy(dst);
	}

	gdImageDestroy(src);
}

static void test_bicubic_one_pixel_checker(void)
{
	gdImagePtr src, dst;
	int x, y;
	int gray = 0;
	int maroon = 0;

	src = create_checker(1);
	if (src == NULL) {
		return;
	}

	gdImageSetInterpolationMethod(src, GD_BICUBIC);
	dst = gdImageRotateInterpolated(src, 45.0f, GD223_BG);
	gdTestAssert(dst != NULL);
	if (dst != NULL) {
		assert_gd223_dimensions(dst, "bicubic gd223");
		assert_no_transparent_pixels(dst, "bicubic gd223");
		assert_red_corners(dst, "bicubic gd223");

		for (y = 0; y < gdImageSY(dst); y++) {
			for (x = 0; x < gdImageSX(dst); x++) {
				const int pixel = gdImageGetTrueColorPixel(dst, x, y);
				const int r = gdTrueColorGetRed(pixel);
				const int g = gdTrueColorGetGreen(pixel);
				const int b = gdTrueColorGetBlue(pixel);

				if (r == g && g == b && r > 0 && r < 255) {
					gray++;
				}
				if (r > g && g == b && r < 255) {
					maroon++;
				}
			}
		}

		gdTestAssertMsg(gray > 1000,
						"bicubic gd223 should show gray moire, got %d gray pixels",
						gray);
		gdTestAssertMsg(maroon > 0,
						"bicubic gd223 should blend red background with black edge");
		gdImageDestroy(dst);
	}

	gdImageDestroy(src);
}

static void test_bicubic_low_frequency_checker(void)
{
	gdImagePtr src, dst;
	int x, y;
	int near_black = 0;
	int near_white = 0;
	int transition = 0;

	src = create_checker(8);
	if (src == NULL) {
		return;
	}

	gdImageSetInterpolationMethod(src, GD_BICUBIC);
	dst = gdImageRotateInterpolated(src, 45.0f, GD223_BG);
	gdTestAssert(dst != NULL);
	if (dst != NULL) {
		assert_gd223_dimensions(dst, "bicubic low-frequency checker");
		assert_no_transparent_pixels(dst, "bicubic low-frequency checker");
		assert_red_corners(dst, "bicubic low-frequency checker");

		for (y = 0; y < gdImageSY(dst); y++) {
			for (x = 0; x < gdImageSX(dst); x++) {
				const int pixel = gdImageGetTrueColorPixel(dst, x, y);
				const int rgb = pixel & 0xffffff;
				const int r = gdTrueColorGetRed(pixel);
				const int g = gdTrueColorGetGreen(pixel);
				const int b = gdTrueColorGetBlue(pixel);

				if (rgb == GD223_BG) {
					continue;
				}
				if (r <= 32 && g <= 32 && b <= 32) {
					near_black++;
				} else if (r >= 223 && g >= 223 && b >= 223) {
					near_white++;
				} else {
					transition++;
				}
			}
		}

		gdTestAssertMsg(near_black > 500,
						"low-frequency checker should preserve black regions, got %d",
						near_black);
		gdTestAssertMsg(near_white > 500,
						"low-frequency checker should preserve white regions, got %d",
						near_white);
		gdTestAssertMsg(transition < near_black + near_white,
						"low-frequency checker is too muddy: transition=%d black=%d white=%d",
						transition, near_black, near_white);
		gdImageDestroy(dst);
	}

	gdImageDestroy(src);
}

int main(void)
{
	test_nearest_one_pixel_checker();
	test_bicubic_one_pixel_checker();
	test_bicubic_low_frequency_checker();

	return gdNumFailures();
}
