#include "gd.h"
#include "gdtest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include "readdir.h"
#else
#include <dirent.h>
#endif

#include <sys/stat.h>

typedef enum {
	PNG_EXPECT_DECODE,
	PNG_EXPECT_REJECT
} png_expectation;

static int has_png_suffix(const char *name)
{
	size_t len = strlen(name);
	return len > 4 && strcmp(name + len - 4, ".png") == 0;
}

static int is_directory(const char *path)
{
	struct stat st;
	return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static gdImagePtr read_png(const char *path)
{
	FILE *fp;
	gdImagePtr im;

	fp = fopen(path, "rb");
	if (fp == NULL) {
		return NULL;
	}
	im = gdImageCreateFromPng(fp);
	fclose(fp);
	return im;
}

static void assert_valid_file(const char *path)
{
	gdImagePtr im = read_png(path);

	gdTestAssertMsg(im != NULL, "valid PNG failed to decode: %s\n", path);
	if (im == NULL) {
		return;
	}

	gdTestAssertMsg(gdImageSX(im) > 0 && gdImageSY(im) > 0,
	                "decoded PNG has invalid dimensions: %s\n", path);
	gdImageDestroy(im);
}

static void assert_invalid_file(const char *path)
{
	gdImagePtr im = read_png(path);

	gdTestAssertMsg(im == NULL, "invalid PNG decoded successfully: %s\n", path);
	if (im != NULL) {
		gdImageDestroy(im);
	}
}

static int scan_directory(const char *dir, png_expectation expectation)
{
	DIR *handle;
	struct dirent *entry;
	int files = 0;

	handle = opendir(dir);
	if (handle == NULL) {
		gdTestErrorMsg("cannot open PNG conformance directory: %s\n", dir);
		return 0;
	}

	while ((entry = readdir(handle)) != NULL) {
		char path[4096];

		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}
		if (snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name) >= (int)sizeof(path)) {
			gdTestErrorMsg("PNG corpus path is too long: %s/%s\n", dir, entry->d_name);
			continue;
		}
		if (is_directory(path)) {
			files += scan_directory(path, expectation);
			continue;
		}
		if (!has_png_suffix(entry->d_name)) {
			continue;
		}
		files++;
		if (expectation == PNG_EXPECT_DECODE) {
			assert_valid_file(path);
		} else {
			assert_invalid_file(path);
		}
	}

	closedir(handle);
	return files;
}

int main(void)
{
	char *valid = gdTestFilePathX("png", "png-conformance", "valid", NULL);
	char *invalid = gdTestFilePathX("png", "png-conformance", "invalid", NULL);
	int valid_files;
	int invalid_files;

	gdSetErrorMethod(gdSilence);
	valid_files = scan_directory(valid, PNG_EXPECT_DECODE);
	invalid_files = scan_directory(invalid, PNG_EXPECT_REJECT);
	gdClearErrorMethod();

	gdTestAssertMsg(valid_files > 0,
	                "PNG conformance valid corpus has no .png files in %s "
	                "(valid=%d invalid=%d)\n",
	                valid, valid_files, invalid_files);
	gdTestAssertMsg(invalid_files > 0,
	                "PNG conformance invalid corpus has no .png files in %s "
	                "(valid=%d invalid=%d)\n",
	                invalid, valid_files, invalid_files);

	free(valid);
	free(invalid);
	return gdNumFailures();
}
