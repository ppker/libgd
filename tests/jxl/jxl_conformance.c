#include "gd.h"
#include "gdtest.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include "readdir.h"
#else
#include <dirent.h>
#endif

typedef enum { JXL_EXPECT_VALID, JXL_EXPECT_INVALID } jxl_expectation;

typedef struct {
	const char *name;
	int expected_count;
	jxl_expectation expectation;
} corpus_category;

static int has_jxl_suffix(const char *name) {
	size_t len = strlen(name);
	return len > 4 && strcmp(name + len - 4, ".jxl") == 0;
}

static unsigned char *read_file(FILE *fp, int *size) {
	long len;
	unsigned char *data;

	*size = 0;
	if (fseek(fp, 0, SEEK_END) != 0) {
		return NULL;
	}
	len = ftell(fp);
	if (len < 0 || len > INT_MAX || fseek(fp, 0, SEEK_SET) != 0) {
		return NULL;
	}
	data = (unsigned char *)malloc(len > 0 ? (size_t)len : 1);
	if (data == NULL) {
		return NULL;
	}
	if (len > 0 && fread(data, 1, (size_t)len, fp) != (size_t)len) {
		free(data);
		return NULL;
	}
	*size = (int)len;
	return data;
}

static void assert_valid_file(const char *directory, const char *filename) {
	unsigned char *data;
	gdJxlAnimReaderPtr reader;
	gdImagePtr image;
	int delay;
	int frames = 0;
	int size;
	FILE *fp;

	fp = gdTestFileOpenX("jxl", "conformance", directory, filename, NULL);
	gdTestAssertMsg(fp != NULL, "cannot open JXL corpus file: %s\n", filename);
	if (fp == NULL) {
		return;
	}
	data = read_file(fp, &size);
	fclose(fp);
	gdTestAssertMsg(data != NULL, "cannot read JXL corpus file: %s\n",
					filename);
	if (data == NULL) {
		return;
	}

	image = gdImageCreateFromJxlPtr(size, data);
	gdTestAssertMsg(image != NULL, "valid JXL failed to decode: %s\n", filename);
	if (image != NULL) {
		gdTestAssertMsg(gdImageSX(image) > 0 && gdImageSY(image) > 0,
						"decoded JXL has invalid dimensions: %s\n", filename);
		gdImageDestroy(image);
	}

	reader = gdImageJxlAnimReaderCreatePtr(size, data);
	gdTestAssertMsg(reader != NULL,
					"animation reader rejected valid JXL: %s\n", filename);
	if (reader != NULL) {
		while ((image = gdJxlReadNextImage(reader, &delay)) != NULL) {
			gdTestAssertMsg(gdImageSX(image) > 0 && gdImageSY(image) > 0,
							"JXL frame has invalid dimensions: %s\n", filename);
			gdTestAssertMsg(delay >= 0,
							"JXL frame has negative duration: %s\n", filename);
			frames++;
			gdImageDestroy(image);
		}
		gdTestAssertMsg(frames > 0, "valid JXL yielded no frames: %s\n",
						filename);
		gdImageJxlAnimReaderDestroy(reader);
	}

	free(data);
}

static void assert_invalid_file(const char *directory, const char *filename) {
	unsigned char *data;
	gdJxlAnimReaderPtr reader;
	gdImagePtr image;
	int delay = -1;
	int size;
	FILE *fp;

	fp = gdTestFileOpenX("jxl", "conformance", directory, filename, NULL);
	gdTestAssertMsg(fp != NULL, "cannot open invalid JXL file: %s\n", filename);
	if (fp == NULL) {
		return;
	}
	data = read_file(fp, &size);
	fclose(fp);
	gdTestAssertMsg(data != NULL, "cannot read invalid JXL file: %s\n",
					filename);
	if (data == NULL) {
		return;
	}

	image = gdImageCreateFromJxlPtr(size, data);
	gdTestAssertMsg(image == NULL, "invalid JXL decoded successfully: %s\n",
					filename);
	if (image != NULL) {
		gdImageDestroy(image);
	}

	reader = gdImageJxlAnimReaderCreatePtr(size, data);
	if (reader != NULL) {
		image = gdJxlReadNextImage(reader, &delay);
		gdTestAssertMsg(image == NULL,
						"invalid JXL animation yielded a frame: %s\n", filename);
		if (image != NULL) {
			gdImageDestroy(image);
		}
		gdImageJxlAnimReaderDestroy(reader);
	}

	free(data);
}

static int scan_category(const corpus_category *category) {
	char *directory;
	DIR *handle;
	struct dirent *entry;
	int files = 0;

	directory = gdTestFilePathX("jxl", "conformance", category->name, NULL);
	handle = opendir(directory);
	if (handle == NULL) {
		gdTestErrorMsg("cannot open JXL conformance directory: %s\n",
					   directory);
		free(directory);
		return 0;
	}

	while ((entry = readdir(handle)) != NULL) {
		if (!has_jxl_suffix(entry->d_name)) {
			continue;
		}
		files++;
		if (category->expectation == JXL_EXPECT_VALID) {
			assert_valid_file(category->name, entry->d_name);
		} else {
			assert_invalid_file(category->name, entry->d_name);
		}
	}
	closedir(handle);

	gdTestAssertMsg(files == category->expected_count,
					"JXL category %s contains %d files, expected %d\n",
					category->name, files, category->expected_count);
	free(directory);
	return files;
}

int main(void) {
	static const corpus_category categories[] = {
		{"conformance", 39, JXL_EXPECT_VALID},
		{"features", 128, JXL_EXPECT_VALID},
		{"photographic", 4, JXL_EXPECT_VALID},
		{"edge-cases", 13, JXL_EXPECT_VALID},
		{"invalid", 4, JXL_EXPECT_INVALID},
	};
	int total_valid = 0;
	int total_invalid = 0;
	size_t i;

	gdSetErrorMethod(gdSilence);
	for (i = 0; i < sizeof(categories) / sizeof(categories[0]); i++) {
		int count = scan_category(&categories[i]);
		if (categories[i].expectation == JXL_EXPECT_VALID) {
			total_valid += count;
		} else {
			total_invalid += count;
		}
	}
	gdClearErrorMethod();

	gdTestAssertMsg(total_valid == 184,
					"JXL valid corpus contains %d files, expected 184\n",
					total_valid);
	gdTestAssertMsg(total_invalid == 4,
					"JXL invalid corpus contains %d files, expected 4\n",
					total_invalid);
	return gdNumFailures();
}
