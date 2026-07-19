#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <libexif/exif-content.h>
#include <libexif/exif-data.h>
#include <libexif/exif-entry.h>
#include <libexif/exif-tag.h>

#include "gd.h"

static unsigned char *read_file(const char *path, size_t *size)
{
	FILE *file;
	long length;
	unsigned char *data;

	file = fopen(path, "rb");
	if (file == NULL || fseek(file, 0, SEEK_END) != 0) {
		if (file != NULL) {
			fclose(file);
		}
		return NULL;
	}
	length = ftell(file);
	if (length < 0 || fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		return NULL;
	}
	data = (unsigned char *)malloc((size_t)length);
	if (data == NULL || fread(data, 1, (size_t)length, file) != (size_t)length) {
		free(data);
		fclose(file);
		return NULL;
	}
	fclose(file);
	*size = (size_t)length;
	return data;
}

static int write_file(const char *path, const void *data, size_t size)
{
	FILE *file = fopen(path, "wb");
	int ok = file != NULL;

	if (ok) {
		ok = fwrite(data, 1, size, file) == size;
		if (fclose(file) != 0) {
			ok = 0;
		}
	}
	return ok;
}

static void print_entry(ExifData *data, ExifIfd ifd, ExifTag tag)
{
	ExifEntry *entry = exif_content_get_entry(data->ifd[ifd], tag);
	char value[256];

	if (entry == NULL) {
		printf("%s: <not present>\n", exif_tag_get_name(tag));
		return;
	}
	printf("%s: %s\n", exif_tag_get_name(tag),
	       exif_entry_get_value(entry, value, sizeof(value)));
}

static int set_artist(ExifData *data, const char *artist)
{
	ExifEntry *entry = exif_content_get_entry(data->ifd[EXIF_IFD_0], EXIF_TAG_ARTIST);
	unsigned int size = (unsigned int)strlen(artist) + 1;

	/* The sample image has room for this short value.  Keeping the edit small
	 * makes the example focus on metadata handling rather than allocation. */
	if (entry == NULL || entry->format != EXIF_FORMAT_ASCII || entry->size < size) {
		return 0;
	}
	memset(entry->data, 0, entry->size);
	memcpy(entry->data, artist, size - 1);
	entry->components = size;
	entry->size = size;
	return 1;
}

static int write_jpeg(const char *path, gdImagePtr image, gdImageMetadata *metadata)
{
	void *data;
	int size;
	int ok;
	gdJpegWriteOptions options;

	gdJpegWriteOptionsInit(&options);
	options.quality = 90;
	options.metadata = metadata;
	data = gdImageJpegPtrWithOptions(image, &size, &options);
	if (data == NULL) {
		return 0;
	}
	ok = write_file(path, data, (size_t)size);
	gdFree(data);
	return ok;
}

#ifdef HAVE_PNG
static int write_png(const char *path, gdImagePtr image, gdImageMetadata *metadata)
{
	void *data;
	int size;
	int ok;
	gdPngWriteOptions options;

	gdPngWriteOptionsInit(&options);
	options.metadata = metadata;
	data = gdImagePngPtrWithOptions(image, &size, &options);
	if (data == NULL) {
		return 0;
	}
	ok = write_file(path, data, (size_t)size);
	gdFree(data);
	return ok;
}
#endif

int main(int argc, char **argv)
{
	unsigned char *input_data = NULL;
	size_t input_size;
	gdImagePtr image = NULL;
	gdImageMetadata *metadata = NULL;
	const unsigned char *profile;
	size_t profile_size;
	ExifData *exif = NULL;
	unsigned char *libexif_data = NULL;
	size_t libexif_size;
	unsigned char *serialized = NULL;
	unsigned int serialized_size = 0;
	int ok = 0;

	if (argc != 3 && argc != 4) {
		fprintf(stderr, "usage: %s input.jpg output.jpg [output.png]\n", argv[0]);
		return EXIT_FAILURE;
	}

	input_data = read_file(argv[1], &input_size);
	if (input_data == NULL || input_size > (size_t)INT_MAX) {
		fprintf(stderr, "could not read input JPEG: %s\n", argv[1]);
		goto cleanup;
	}
	metadata = gdImageMetadataCreate();
	if (metadata == NULL) {
		fprintf(stderr, "could not create metadata container\n");
		goto cleanup;
	}
	if (gdJpegGetMetadataPtr((int)input_size, input_data, metadata) != 0) {
		fprintf(stderr, "could not read input JPEG metadata: %s\n", argv[1]);
		goto cleanup;
	}
	image = gdImageCreateFromJpegPtr((int)input_size, input_data);
	if (image == NULL) {
		fprintf(stderr, "could not decode input JPEG: %s\n", argv[1]);
		goto cleanup;
	}
	profile = gdImageMetadataGetProfile(metadata, "exif", &profile_size);
	if (profile == NULL || profile_size > (size_t)UINT_MAX) {
		fprintf(stderr, "input JPEG has no usable EXIF profile\n");
		goto cleanup;
	}

	if (profile_size > (size_t)UINT_MAX - 6) {
		fprintf(stderr, "input JPEG EXIF profile is too large\n");
		goto cleanup;
	}
	libexif_size = profile_size + 6;
	libexif_data = (unsigned char *)malloc(libexif_size);
	if (libexif_data == NULL) {
		fprintf(stderr, "could not allocate EXIF input buffer\n");
		goto cleanup;
	}
	memcpy(libexif_data, "Exif\0\0", 6);
	memcpy(libexif_data + 6, profile, profile_size);
	exif = exif_data_new_from_data(libexif_data, (unsigned int)libexif_size);
	if (exif == NULL) {
		fprintf(stderr, "libexif could not parse the EXIF profile\n");
		goto cleanup;
	}
	print_entry(exif, EXIF_IFD_0, EXIF_TAG_ARTIST);
	if (!set_artist(exif, "libgd")) {
		fprintf(stderr, "could not update Exif.Image.Artist\n");
		goto cleanup;
	}
	exif_data_save_data(exif, &serialized, &serialized_size);
	if (serialized == NULL || gdImageMetadataSetProfile(metadata, "exif", serialized,
	                                                    serialized_size) != GD_META_OK) {
		fprintf(stderr, "could not serialize or store updated EXIF\n");
		goto cleanup;
	}
	if (!write_jpeg(argv[2], image, metadata)) {
		fprintf(stderr, "could not write output JPEG: %s\n", argv[2]);
		goto cleanup;
	}

	if (argc == 4) {
#ifdef HAVE_PNG
		if (!write_png(argv[3], image, metadata)) {
			fprintf(stderr, "could not write output PNG: %s\n", argv[3]);
			goto cleanup;
		}
#else
		fprintf(stderr, "this build does not have PNG support\n");
		goto cleanup;
#endif
	}
	ok = 1;

cleanup:
	free(serialized);
	free(libexif_data);
	if (exif != NULL) {
		exif_data_unref(exif);
	}
	if (image != NULL) {
		gdImageDestroy(image);
	}
	if (metadata != NULL) {
		gdImageMetadataFree(metadata);
	}
	free(input_data);
	return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
