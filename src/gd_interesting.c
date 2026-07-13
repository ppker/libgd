/**
 * File: Interesting region detection
 *
 * Deterministic content-aware crop-region selection for smart cover scaling.
 */

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "gd.h"
#include "gdhelpers.h"

#define GD_INTERESTING_DOWNSAMPLE_MAX 256
#define GD_INTERESTING_ATTENTION_SIZE 32
#define GD_INTERESTING_EPSILON 0.000001

typedef struct {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char luminance;
} gdInterestingPixel;

static int gdInterestingClampInt(int value, int min, int max)
{
	if (value < min) {
		return min;
	}
	if (value > max) {
		return max;
	}
	return value;
}

static int gdInterestingAbsInt(int value)
{
	return value < 0 ? -value : value;
}

static int gdInterestingMinInt(int a, int b)
{
	return a < b ? a : b;
}

static int gdInterestingMaxInt(int a, int b)
{
	return a > b ? a : b;
}

static unsigned char gdInterestingLuminanceFromRgb(int red, int green, int blue)
{
	return (unsigned char)((299 * red + 587 * green + 114 * blue + 500) / 1000);
}

static gdInterestingPixel *gdInterestingBuildPixelMap(gdImagePtr im, int width, int height)
{
	gdInterestingPixel *map;
	const int src_width = gdImageSX(im);
	const int src_height = gdImageSY(im);

	map = (gdInterestingPixel *)gdMalloc((size_t)width * (size_t)height * sizeof(gdInterestingPixel));
	if (map == NULL) {
		return NULL;
	}

	for (int y = 0; y < height; y++) {
		const int src_y = gdInterestingClampInt((int)((int64_t)y * src_height / height), 0, src_height - 1);
		for (int x = 0; x < width; x++) {
			const int src_x = gdInterestingClampInt((int)((int64_t)x * src_width / width), 0, src_width - 1);
			const int pixel = gdImageGetPixel(im, src_x, src_y);
			gdInterestingPixel *entry = &map[y * width + x];

			entry->red = (unsigned char)gdImageRed(im, pixel);
			entry->green = (unsigned char)gdImageGreen(im, pixel);
			entry->blue = (unsigned char)gdImageBlue(im, pixel);
			entry->luminance = gdInterestingLuminanceFromRgb(entry->red, entry->green, entry->blue);
		}
	}

	return map;
}

static int gdInterestingDownsampleSize(int src_width, int src_height, int *width, int *height)
{
	if (src_width <= 0 || src_height <= 0) {
		return 0;
	}

	if (src_width >= src_height) {
		*width = src_width > GD_INTERESTING_DOWNSAMPLE_MAX ? GD_INTERESTING_DOWNSAMPLE_MAX : src_width;
		*height = (int)floor((double)src_height * (double)*width / (double)src_width + 0.5);
		if (*height < 1) {
			*height = 1;
		}
	} else {
		*height = src_height > GD_INTERESTING_DOWNSAMPLE_MAX ? GD_INTERESTING_DOWNSAMPLE_MAX : src_height;
		*width = (int)floor((double)src_width * (double)*height / (double)src_height + 0.5);
		if (*width < 1) {
			*width = 1;
		}
	}

	return 1;
}

static void gdInterestingTargetCropSize(
	int src_width,
	int src_height,
	unsigned int target_width,
	unsigned int target_height,
	int *crop_width,
	int *crop_height)
{
	const double src_aspect = (double)src_width / (double)src_height;
	const double target_aspect = (double)target_width / (double)target_height;

	if (src_aspect > target_aspect) {
		*crop_height = src_height;
		*crop_width = (int)floor((double)src_height * target_aspect + 0.5);
		*crop_width = gdInterestingClampInt(*crop_width, 1, src_width);
	} else {
		*crop_width = src_width;
		*crop_height = (int)floor((double)src_width / target_aspect + 0.5);
		*crop_height = gdInterestingClampInt(*crop_height, 1, src_height);
	}
}

static int gdInterestingIsSkin(const gdInterestingPixel *pixel)
{
	const int red = pixel->red;
	const int green = pixel->green;
	const int blue = pixel->blue;
	const int max = red > green ? (red > blue ? red : blue) : (green > blue ? green : blue);
	const int min = red < green ? (red < blue ? red : blue) : (green < blue ? green : blue);

	return red > 95 && green > 40 && blue > 20
		&& (max - min) > 15
		&& gdInterestingAbsInt(red - green) > 15
		&& red > green
		&& red > blue;
}

static double gdInterestingRegionEntropy(
	const gdInterestingPixel *map,
	int width,
	int left,
	int top,
	int region_width,
	int region_height)
{
	int histogram[256];
	int count = 0;
	double entropy = 0.0;

	memset(histogram, 0, sizeof(histogram));

	for (int y = top; y < top + region_height; y++) {
		for (int x = left; x < left + region_width; x++) {
			histogram[map[y * width + x].luminance]++;
			count++;
		}
	}

	if (count == 0) {
		return 0.0;
	}

	for (int i = 0; i < 256; i++) {
		if (histogram[i] != 0) {
			const double p = (double)histogram[i] / (double)count;
			entropy -= p * (log(p) / log(2.0));
		}
	}

	return entropy;
}

static void gdInterestingEntropySmartCrop(
	const gdInterestingPixel *map,
	int map_width,
	int map_height,
	int target_width,
	int target_height,
	int *left,
	int *top)
{
	int current_left = 0;
	int current_top = 0;
	int current_width = map_width;
	int current_height = map_height;
	const int max_slice_size = gdInterestingMaxInt(
		(int)ceil((double)(map_width - target_width) / 8.0),
		(int)ceil((double)(map_height - target_height) / 8.0));

	while (current_width > target_width || current_height > target_height) {
		const int slice_width = gdInterestingMinInt(current_width - target_width, max_slice_size);
		const int slice_height = gdInterestingMinInt(current_height - target_height, max_slice_size);

		if (slice_width > 0) {
			const double left_score = gdInterestingRegionEntropy(
				map, map_width, current_left, current_top, slice_width, current_height);
			const double right_score = gdInterestingRegionEntropy(
				map, map_width, current_left + current_width - slice_width, current_top, slice_width, current_height);

			current_width -= slice_width;
			if (left_score < right_score) {
				current_left += slice_width;
			}
		}

		if (slice_height > 0) {
			const double top_score = gdInterestingRegionEntropy(
				map, map_width, current_left, current_top, current_width, slice_height);
			const double bottom_score = gdInterestingRegionEntropy(
				map, map_width, current_left, current_top + current_height - slice_height, current_width, slice_height);

			current_height -= slice_height;
			if (top_score < bottom_score) {
				current_top += slice_height;
			}
		}
	}

	*left = current_left;
	*top = current_top;
}

static int gdInterestingAttentionSmartCrop(
	const gdInterestingPixel *map,
	int map_width,
	int map_height,
	int src_width,
	int src_height,
	int crop_width,
	int crop_height,
	int *left,
	int *top)
{
	double *scores, *blurred;
	double hscale, vscale, sigma;
	int radius, best_x = 0, best_y = 0;
	double best_score = -1.0;

	scores = (double *)gdMalloc((size_t)map_width * (size_t)map_height * sizeof(double));
	blurred = (double *)gdMalloc((size_t)map_width * (size_t)map_height * sizeof(double));
	if (scores == NULL || blurred == NULL) {
		if (scores != NULL) {
			gdFree(scores);
		}
		if (blurred != NULL) {
			gdFree(blurred);
		}
		return 0;
	}

	for (int y = 0; y < map_height; y++) {
		for (int x = 0; x < map_width; x++) {
			const gdInterestingPixel *pixel = &map[y * map_width + x];
			const int center = pixel->luminance;
			const int left_luma = map[y * map_width + gdInterestingClampInt(x - 1, 0, map_width - 1)].luminance;
			const int right_luma = map[y * map_width + gdInterestingClampInt(x + 1, 0, map_width - 1)].luminance;
			const int top_luma = map[gdInterestingClampInt(y - 1, 0, map_height - 1) * map_width + x].luminance;
			const int bottom_luma = map[gdInterestingClampInt(y + 1, 0, map_height - 1) * map_width + x].luminance;
			const int edge = gdInterestingAbsInt(4 * center - left_luma - right_luma - top_luma - bottom_luma);
			const int max = pixel->red > pixel->green ? (pixel->red > pixel->blue ? pixel->red : pixel->blue) : (pixel->green > pixel->blue ? pixel->green : pixel->blue);
			const int min = pixel->red < pixel->green ? (pixel->red < pixel->blue ? pixel->red : pixel->blue) : (pixel->green < pixel->blue ? pixel->green : pixel->blue);
			double score = (double)edge * 5.0;

			if (center > 5) {
				score += (double)(max - min);
				if (gdInterestingIsSkin(pixel)) {
					score += 180.0;
				}
			}

			scores[y * map_width + x] = score;
		}
	}

	hscale = (double)map_width / (double)src_width;
	vscale = (double)map_height / (double)src_height;
	sigma = sqrt(pow((double)crop_width * hscale, 2.0) + pow((double)crop_height * vscale, 2.0));
	sigma = sigma / 10.0;
	if (sigma < 1.0) {
		sigma = 1.0;
	}
	radius = gdInterestingClampInt((int)ceil(sigma * 1.5), 1, gdInterestingMaxInt(map_width, map_height));

	for (int y = 0; y < map_height; y++) {
		for (int x = 0; x < map_width; x++) {
			double total = 0.0;
			double weight_total = 0.0;

			for (int ky = -radius; ky <= radius; ky++) {
				const int yy = gdInterestingClampInt(y + ky, 0, map_height - 1);
				for (int kx = -radius; kx <= radius; kx++) {
					const int xx = gdInterestingClampInt(x + kx, 0, map_width - 1);
					const double distance2 = (double)(kx * kx + ky * ky);
					const double weight = exp(-distance2 / (2.0 * sigma * sigma));

					total += scores[yy * map_width + xx] * weight;
					weight_total += weight;
				}
			}

			blurred[y * map_width + x] = weight_total > 0.0 ? total / weight_total : 0.0;
			if (blurred[y * map_width + x] > best_score) {
				best_score = blurred[y * map_width + x];
				best_x = x;
				best_y = y;
			}
		}
	}

	gdFree(scores);
	gdFree(blurred);

	if (best_score <= GD_INTERESTING_EPSILON) {
		return 0;
	}

	*left = gdInterestingClampInt(
		(int)floor((double)best_x * (double)src_width / (double)map_width) - (crop_width + 1) / 2,
		0,
		src_width - crop_width);
	*top = gdInterestingClampInt(
		(int)floor((double)best_y * (double)src_height / (double)map_height) - (crop_height + 1) / 2,
		0,
		src_height - crop_height);

	return 1;
}

BGD_DECLARE(int)
gdImageInterestingCropRegion(const gdImagePtr src, unsigned int target_width,
                             unsigned int target_height, gdInterestingMethod method,
                             gdRectPtr crop)
{
	gdInterestingPixel *map;
	int src_width, src_height, map_width, map_height;
	int crop_width, crop_height, map_crop_width, map_crop_height;
	int map_left = 0, map_top = 0, crop_x = 0, crop_y = 0;

	if (src == NULL || crop == NULL || target_width == 0 || target_height == 0) {
		return 0;
	}
	if (method != GD_INTERESTING_ENTROPY && method != GD_INTERESTING_ATTENTION) {
		return 0;
	}

	src_width = gdImageSX(src);
	src_height = gdImageSY(src);
	if (method == GD_INTERESTING_ATTENTION) {
		map_width = GD_INTERESTING_ATTENTION_SIZE;
		map_height = GD_INTERESTING_ATTENTION_SIZE;
	} else {
		if (!gdInterestingDownsampleSize(src_width, src_height, &map_width, &map_height)) {
			return 0;
		}
	}

	map = gdInterestingBuildPixelMap(src, map_width, map_height);
	if (map == NULL) {
		return 0;
	}

	gdInterestingTargetCropSize(src_width, src_height, target_width, target_height, &crop_width, &crop_height);
	map_crop_width = gdInterestingClampInt((int)floor((double)crop_width * (double)map_width / (double)src_width + 0.5), 1, map_width);
	map_crop_height = gdInterestingClampInt((int)floor((double)crop_height * (double)map_height / (double)src_height + 0.5), 1, map_height);

	if (method == GD_INTERESTING_ENTROPY) {
		gdInterestingEntropySmartCrop(map, map_width, map_height, map_crop_width, map_crop_height, &map_left, &map_top);
		crop_x = (int)floor((double)map_left * (double)src_width / (double)map_width + 0.5);
		crop_y = (int)floor((double)map_top * (double)src_height / (double)map_height + 0.5);
	} else {
		if (!gdInterestingAttentionSmartCrop(map, map_width, map_height, src_width, src_height, crop_width, crop_height, &crop_x, &crop_y)) {
			gdFree(map);
			return 0;
		}
	}
	gdFree(map);

	crop->x = gdInterestingClampInt(crop_x, 0, src_width - crop_width);
	crop->y = gdInterestingClampInt(crop_y, 0, src_height - crop_height);
	crop->width = crop_width;
	crop->height = crop_height;

	return 1;
}

BGD_DECLARE(int)
gdImageEntropyCropRegion(const gdImagePtr src, unsigned int target_width,
                         unsigned int target_height, gdRectPtr crop)
{
	return gdImageInterestingCropRegion(src, target_width, target_height, GD_INTERESTING_ENTROPY, crop);
}
