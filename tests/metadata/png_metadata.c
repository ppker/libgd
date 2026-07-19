#include <string.h>

#include "gd.h"
#include "gdtest.h"

static void assert_profile_equals(gdImageMetadata *metadata, const char *key,
								  const unsigned char *expected,
								  size_t expected_size) {
	const unsigned char *actual;
	size_t actual_size;

	actual = gdImageMetadataGetProfile(metadata, key, &actual_size);
	gdTestAssert(actual != NULL);
	gdTestAssert(actual_size == expected_size);
	if (actual != NULL && actual_size == expected_size) {
		gdTestAssert(memcmp(actual, expected, expected_size) == 0);
	}
}

int main(void) {
	static const unsigned char exif[] = {'E', 'x', 'i', 'f', 0, 0, 'M', 'M', 0, 42, 0, 0, 0, 8};
	static const unsigned char icc[] = {'g', 'd', '-', 'i', 'c',
										'c', 0,	  0,   'x'};
	static const unsigned char xmp[] =
		"XML:com.adobe.xmp\0\0\0\0\0<x:xmpmeta>gd</x:xmpmeta>";
	static const unsigned char text[] = "Comment\0gd text";
	gdImagePtr im;
	gdImagePtr decoded;
	gdImageMetadata *metadata;
	gdImageMetadata *decoded_metadata;
	gdPngInfo info;
	gdIOCtx *out;
	void *png;
	int size = 0;
	int color;
	gdPngWriteOptions options;

	metadata = gdImageMetadataCreate();
	decoded_metadata = gdImageMetadataCreate();
	gdTestAssert(metadata != NULL);
	gdTestAssert(decoded_metadata != NULL);

	gdTestAssert(gdImageMetadataSetProfile(metadata, "exif", exif,
										   sizeof(exif)) == GD_META_OK);
	gdTestAssert(gdImageMetadataSetProfile(metadata, "icc", icc, sizeof(icc)) ==
				 GD_META_OK);
	gdTestAssert(gdImageMetadataSetProfile(metadata, "xmp", xmp, sizeof(xmp)) ==
				 GD_META_OK);
	gdTestAssert(gdImageMetadataSetProfile(metadata, "png:text:Comment", text,
										   sizeof(text)) == GD_META_OK);

	im = gdImageCreateTrueColor(8, 8);
	gdTestAssert(im != NULL);
	color = gdTrueColorAlpha(10, 20, 30, 0);
	gdImageFilledRectangle(im, 0, 0, 7, 7, color);
	gdImageSaveAlpha(im, 1);

	out = gdNewDynamicCtx(2048, NULL);
	gdTestAssert(out != NULL);
	gdPngWriteOptionsInit(&options);
	options.metadata = metadata;
	gdTestAssert(gdImagePngCtxWithOptions(im, out, &options) == 0);
	png = gdDPExtractData(out, &size);
	out->gd_free(out);
	gdTestAssert(png != NULL);
	gdTestAssert(size > 0);

	gdPngInfoInit(&info);
	info.metadata = decoded_metadata;
	gdTestAssert(gdPngGetInfoPtr(size, png, &info) == 0);
	decoded = gdImageCreateFromPngPtr(size, png);
	gdTestAssert(decoded != NULL);

	assert_profile_equals(decoded_metadata, "exif", exif, sizeof(exif));
	gdTestAssert(gdImageMetadataGetProfile(decoded_metadata, "icc", NULL) == NULL);
	assert_profile_equals(decoded_metadata, "xmp", xmp, sizeof(xmp));
	assert_profile_equals(decoded_metadata, "png:text:Comment", text,
						  sizeof(text));

	gdFree(png);
	gdImageDestroy(decoded);
	gdImageDestroy(im);
	gdImageMetadataFree(decoded_metadata);
	gdImageMetadataFree(metadata);

	return gdNumFailures();
}
