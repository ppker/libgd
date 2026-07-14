#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "gd.h"

typedef struct {
	const char *name;
	unsigned int src_w;
	unsigned int src_h;
	unsigned int dst_w;
	unsigned int dst_h;
} BenchCase;

typedef struct {
	const char *name;
	void (*fill)(gdImagePtr);
} BenchPattern;

typedef struct {
	const char *name;
	gdInterpolationMethod method;
} BenchMethod;

static double now_ms(void)
{
	return (double)clock() * 1000.0 / (double)CLOCKS_PER_SEC;
}

static int compare_double(const void *a, const void *b)
{
	const double da = *(const double *)a;
	const double db = *(const double *)b;

	if (da < db) {
		return -1;
	}
	if (da > db) {
		return 1;
	}
	return 0;
}

static int matches_filter(const char *value, const char *filter)
{
	return filter == NULL || strcmp(value, filter) == 0;
}

static void fill_photo_like(gdImagePtr im)
{
	int x, y;

	for (y = 0; y < gdImageSY(im); y++) {
		for (x = 0; x < gdImageSX(im); x++) {
			const int r = (x * 13 + y * 3 + ((x * y) & 63)) & 255;
			const int g = (x * 5 + y * 11 + ((x + y) & 31)) & 255;
			const int b = (x * 7 + y * 17 + ((x ^ y) & 127)) & 255;

			im->tpixels[y][x] = gdTrueColorAlpha(r, g, b, gdAlphaOpaque);
		}
	}
}

static void fill_checker_noise(gdImagePtr im)
{
	int x, y;

	for (y = 0; y < gdImageSY(im); y++) {
		for (x = 0; x < gdImageSX(im); x++) {
			const int checker = (((x >> 3) ^ (y >> 3)) & 1) ? 255 : 0;
			const int noise = (x * 37 + y * 73 + ((x * y) & 255)) & 255;
			const int v = (checker * 3 + noise) / 4;

			im->tpixels[y][x] = gdTrueColorAlpha(v, 255 - v, noise, gdAlphaOpaque);
		}
	}
}

static void fill_alpha_edges(gdImagePtr im)
{
	int x, y;

	for (y = 0; y < gdImageSY(im); y++) {
		for (x = 0; x < gdImageSX(im); x++) {
			const int stripe = ((x / 17) + (y / 13)) & 1;
			const int alpha = stripe ? gdAlphaOpaque : gdAlphaTransparent;
			const int r = stripe ? 255 : 0;
			const int g = (x * 9 + y * 5) & 255;
			const int b = stripe ? 32 : 0;

			im->tpixels[y][x] = gdTrueColorAlpha(r, g, b, alpha);
		}
	}
}

static unsigned long long image_checksum(gdImagePtr im)
{
	unsigned long long hash = 1469598103934665603ULL;
	int x, y;

	for (y = 0; y < gdImageSY(im); y++) {
		for (x = 0; x < gdImageSX(im); x++) {
			unsigned int pixel = (unsigned int)gdImageGetTrueColorPixel(im, x, y);
			int shift;

			for (shift = 0; shift < 32; shift += 8) {
				hash ^= (pixel >> shift) & 0xffU;
				hash *= 1099511628211ULL;
			}
		}
	}

	return hash;
}

static int run_case(const char *profile, const BenchCase *bench_case, const BenchPattern *pattern,
					const BenchMethod *method, unsigned int iterations)
{
	gdImagePtr src;
	gdImagePtr dst;
	double *samples;
	double min_ms, median_ms, max_ms;
	unsigned long long checksum = 0;
	unsigned int i;

	src = gdImageCreateTrueColor(bench_case->src_w, bench_case->src_h);
	if (src == NULL) {
		fprintf(stderr, "failed to create source image for %s\n", bench_case->name);
		return 0;
	}
	pattern->fill(src);
	gdImageSetInterpolationMethod(src, method->method);

	dst = gdImageScale(src, bench_case->dst_w, bench_case->dst_h);
	if (dst == NULL) {
		fprintf(stderr, "warm-up scale failed for %s/%s/%s\n", bench_case->name,
				pattern->name, method->name);
		gdImageDestroy(src);
		return 0;
	}
	checksum = image_checksum(dst);
	gdImageDestroy(dst);

	samples = (double *)calloc(iterations, sizeof(double));
	if (samples == NULL) {
		gdImageDestroy(src);
		return 0;
	}

	for (i = 0; i < iterations; i++) {
		const double start = now_ms();

		dst = gdImageScale(src, bench_case->dst_w, bench_case->dst_h);
		if (dst == NULL) {
			fprintf(stderr, "scale failed for %s/%s/%s\n", bench_case->name,
					pattern->name, method->name);
			free(samples);
			gdImageDestroy(src);
			return 0;
		}
		samples[i] = now_ms() - start;
		checksum = image_checksum(dst);
		gdImageDestroy(dst);
	}

	qsort(samples, iterations, sizeof(double), compare_double);
	min_ms = samples[0];
	median_ms = samples[iterations / 2];
	max_ms = samples[iterations - 1];

	printf("%s,%s,%s,%s,%u,%u,%u,%u,%u,%.3f,%.3f,%.3f,%llu\n", profile,
		   bench_case->name, pattern->name, method->name, bench_case->src_w,
		   bench_case->src_h, bench_case->dst_w, bench_case->dst_h, iterations,
		   min_ms, median_ms, max_ms, checksum);

	free(samples);
	gdImageDestroy(src);
	return 1;
}

int main(int argc, char **argv)
{
	const BenchCase quick_cases[] = {
		{"vertical_moderate", 256, 512, 256, 128},
		{"two_axis_moderate", 512, 512, 128, 128},
		{"horizontal_control", 512, 256, 128, 256},
	};
	const BenchCase full_cases[] = {
		{"vertical_mild", 768, 1152, 768, 768},
		{"vertical_moderate", 768, 2048, 768, 512},
		{"vertical_aggressive", 768, 2560, 768, 256},
		{"two_axis_mild", 1152, 1152, 768, 768},
		{"two_axis_moderate", 2048, 2048, 512, 512},
		{"two_axis_aggressive", 2560, 2560, 256, 256},
		{"horizontal_control", 2048, 768, 512, 768},
	};
	const BenchPattern patterns[] = {
		{"photo_like", fill_photo_like},
		{"checker_noise", fill_checker_noise},
		{"alpha_edges", fill_alpha_edges},
	};
	const BenchMethod methods[] = {
		{"triangle", GD_TRIANGLE},
		{"box", GD_BOX},
		{"mitchell", GD_MITCHELL},
		{"lanczos3", GD_LANCZOS3},
	};
	const BenchCase *cases = quick_cases;
	size_t cases_count = sizeof(quick_cases) / sizeof(quick_cases[0]);
	const char *profile = "quick";
	const char *case_filter = NULL;
	const char *pattern_filter = NULL;
	const char *method_filter = NULL;
	unsigned int iterations = 3;
	size_t case_ndx, pattern_ndx, method_ndx;
	int arg_ndx;

	for (arg_ndx = 1; arg_ndx < argc; arg_ndx++) {
		if (strcmp(argv[arg_ndx], "--full") == 0) {
			cases = full_cases;
			cases_count = sizeof(full_cases) / sizeof(full_cases[0]);
			profile = "full";
		} else if (strcmp(argv[arg_ndx], "--case") == 0 && arg_ndx + 1 < argc) {
			case_filter = argv[++arg_ndx];
		} else if (strcmp(argv[arg_ndx], "--pattern") == 0 && arg_ndx + 1 < argc) {
			pattern_filter = argv[++arg_ndx];
		} else if (strcmp(argv[arg_ndx], "--method") == 0 && arg_ndx + 1 < argc) {
			method_filter = argv[++arg_ndx];
		} else {
			iterations = (unsigned int)strtoul(argv[arg_ndx], NULL, 10);
			if (iterations == 0) {
				iterations = 1;
			}
		}
	}

	printf("profile,case,pattern,method,src_w,src_h,dst_w,dst_h,iterations,min_ms,median_ms,max_ms,checksum\n");

	for (case_ndx = 0; case_ndx < cases_count; case_ndx++) {
		if (!matches_filter(cases[case_ndx].name, case_filter)) {
			continue;
		}
		for (pattern_ndx = 0; pattern_ndx < sizeof(patterns) / sizeof(patterns[0]);
			 pattern_ndx++) {
			if (!matches_filter(patterns[pattern_ndx].name, pattern_filter)) {
				continue;
			}
			for (method_ndx = 0; method_ndx < sizeof(methods) / sizeof(methods[0]);
				 method_ndx++) {
				if (!matches_filter(methods[method_ndx].name, method_filter)) {
					continue;
				}
				if (!run_case(profile, &cases[case_ndx], &patterns[pattern_ndx],
							  &methods[method_ndx], iterations)) {
					return 1;
				}
			}
		}
	}

	return 0;
}
