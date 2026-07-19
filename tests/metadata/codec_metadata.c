#include "gd.h"
#include "gdtest.h"

#include <string.h>

static const unsigned char raw_exif[] = {'I', 'I', 42, 0, 8, 0, 0, 0};
static const unsigned char prefixed_exif[] = {
    'E', 'x', 'i', 'f', 0, 0, 'I', 'I', 42, 0, 8, 0, 0, 0
};
static const unsigned char invalid_exif[] = {'I', 'I', 43, 0, 8, 0, 0, 0};

static void assert_exif(gdImageMetadata *metadata)
{
    const unsigned char *data;
    size_t size;

    data = gdImageMetadataGetProfile(metadata, "exif", &size);
    gdTestAssertMsg(data != NULL, "decoded EXIF profile is missing\n");
    if (data == NULL) {
        return;
    }
    if (size >= 6 && memcmp(data, "Exif\0\0", 6) == 0) {
        data += 6;
        size -= 6;
    }
    gdTestAssertMsg(size == sizeof(raw_exif) && memcmp(data, raw_exif, sizeof(raw_exif)) == 0,
                    "decoded EXIF TIFF payload changed\n");
}

static gdImagePtr make_image(void)
{
    gdImagePtr image = gdImageCreateTrueColor(4, 4);
    if (image != NULL) {
        gdImageFilledRectangle(image, 0, 0, 3, 3, gdTrueColor(40, 80, 120));
    }
    return image;
}

static gdImageMetadata *make_metadata(const unsigned char *data, size_t size)
{
    gdImageMetadata *metadata = gdImageMetadataCreate();
    if (metadata == NULL || gdImageMetadataSetProfile(metadata, "exif", data, size) != GD_META_OK) {
        gdImageMetadataFree(metadata);
        return NULL;
    }
    return metadata;
}

#ifdef GD_TEST_HAVE_JPEG
static void test_jpeg(const unsigned char *data, size_t data_size)
{
    gdImagePtr image = make_image();
    gdImageMetadata *metadata = make_metadata(data, data_size);
    gdImageMetadata *decoded_metadata = gdImageMetadataCreate();
    gdJpegWriteOptions options;
    void *encoded;
    int encoded_size = 0;

    gdJpegWriteOptionsInit(&options);
    options.metadata = metadata;
    encoded = gdImageJpegPtrWithOptions(image, &encoded_size, &options);
    gdTestAssertMsg(encoded != NULL && encoded_size > 0, "JPEG metadata write failed\n");
    if (encoded != NULL && decoded_metadata != NULL) {
        gdTestAssertMsg(gdJpegGetMetadataPtr(encoded_size, encoded, decoded_metadata) == 0,
                        "JPEG metadata read failed\n");
        assert_exif(decoded_metadata);
    }
    gdFree(encoded);
    gdImageMetadataFree(decoded_metadata);
    gdImageMetadataFree(metadata);
    gdImageDestroy(image);
}

static void test_jpeg_invalid(void)
{
    gdImagePtr image = make_image();
    gdImageMetadata *metadata = make_metadata(invalid_exif, sizeof(invalid_exif));
    gdJpegWriteOptions options;
    int size = 0;
    void *encoded;

    gdJpegWriteOptionsInit(&options);
    options.metadata = metadata;
    encoded = gdImageJpegPtrWithOptions(image, &size, &options);
    gdTestAssertMsg(encoded == NULL && size == 0, "invalid JPEG EXIF was accepted\n");
    gdFree(encoded);
    gdImageMetadataFree(metadata);
    gdImageDestroy(image);
}
#endif

#ifdef GD_TEST_HAVE_PNG
static void test_png(const unsigned char *data, size_t data_size)
{
    gdImagePtr image = make_image();
    gdImageMetadata *metadata = make_metadata(data, data_size);
    gdImageMetadata *decoded_metadata = gdImageMetadataCreate();
    gdPngWriteOptions options;
    gdPngInfo info;
    void *encoded;
    int encoded_size = 0;

    gdPngWriteOptionsInit(&options);
    options.metadata = metadata;
    encoded = gdImagePngPtrWithOptions(image, &encoded_size, &options);
    gdTestAssertMsg(encoded != NULL && encoded_size > 0, "PNG metadata write failed\n");
    if (encoded != NULL && decoded_metadata != NULL) {
        gdPngInfoInit(&info);
        info.metadata = decoded_metadata;
        gdTestAssertMsg(gdPngGetInfoPtr(encoded_size, encoded, &info) == 0,
                        "PNG metadata read failed\n");
        assert_exif(decoded_metadata);
    }
    gdFree(encoded);
    gdImageMetadataFree(decoded_metadata);
    gdImageMetadataFree(metadata);
    gdImageDestroy(image);
}

static void test_png_invalid(void)
{
    gdImagePtr image = make_image();
    gdImageMetadata *metadata = make_metadata(invalid_exif, sizeof(invalid_exif));
    gdPngWriteOptions options;
    int size = 0;
    void *encoded;

    gdPngWriteOptionsInit(&options);
    options.metadata = metadata;
    encoded = gdImagePngPtrWithOptions(image, &size, &options);
    gdTestAssertMsg(encoded == NULL && size == 0, "invalid PNG EXIF was accepted\n");
    gdFree(encoded);
    gdImageMetadataFree(metadata);
    gdImageDestroy(image);
}
#endif

#ifdef GD_TEST_HAVE_WEBP
static void test_webp(const unsigned char *data, size_t data_size)
{
    gdImagePtr image = make_image();
    gdImageMetadata *metadata = make_metadata(data, data_size);
    gdImageMetadata *decoded_metadata = gdImageMetadataCreate();
    gdWebpWriteOptions options;
    gdWebpReadPtr reader;
    void *encoded;
    int encoded_size = 0;

    gdWebpWriteOptionsInit(&options);
    options.metadata = metadata;
    encoded = gdImageWebpPtrWithOptions(image, &encoded_size, &options);
    gdTestAssertMsg(encoded != NULL && encoded_size > 0, "WebP metadata write failed\n");
    reader = encoded != NULL ? gdWebpReadOpenPtr(encoded_size, encoded, NULL) : NULL;
    if (reader != NULL && decoded_metadata != NULL) {
        gdTestAssertMsg(gdWebpReadGetMetadata(reader, decoded_metadata) == GD_META_OK,
                        "WebP metadata read failed\n");
        assert_exif(decoded_metadata);
    }
    gdWebpReadClose(reader);
    gdFree(encoded);
    gdImageMetadataFree(decoded_metadata);
    gdImageMetadataFree(metadata);
    gdImageDestroy(image);
}
#endif

#ifdef GD_TEST_HAVE_AVIF
static void test_avif(const unsigned char *data, size_t data_size)
{
    gdImagePtr image = make_image();
    gdImageMetadata *metadata = make_metadata(data, data_size);
    gdImageMetadata *decoded_metadata = gdImageMetadataCreate();
    gdAvifWriteOptions options;
    gdAvifInfo info;
    void *encoded;
    int encoded_size = 0;

    gdAvifWriteOptionsInit(&options);
    options.metadata = metadata;
    encoded = gdImageAvifPtrWithOptions(image, &encoded_size, &options);
    gdTestAssertMsg(encoded != NULL && encoded_size > 0, "AVIF metadata write failed\n");
    if (encoded != NULL && decoded_metadata != NULL) {
        gdAvifInfoInit(&info);
        info.metadata = decoded_metadata;
        gdTestAssertMsg(gdAvifGetInfoPtr(encoded_size, encoded, &info) == GD_META_OK,
                        "AVIF metadata read failed\n");
        assert_exif(decoded_metadata);
    }
    gdFree(encoded);
    gdImageMetadataFree(decoded_metadata);
    gdImageMetadataFree(metadata);
    gdImageDestroy(image);
}
#endif

#ifdef GD_TEST_HAVE_HEIF
static void test_heif(const unsigned char *data, size_t data_size)
{
    gdImagePtr image = make_image();
    gdImageMetadata *metadata = make_metadata(data, data_size);
    gdImageMetadata *decoded_metadata = gdImageMetadataCreate();
    gdHeifWriteOptions options;
    gdHeifInfo info;
    void *encoded;
    int encoded_size = 0;

    gdHeifWriteOptionsInit(&options);
    options.metadata = metadata;
    encoded = gdImageHeifPtrWithOptions(image, &encoded_size, &options);
    gdTestAssertMsg(encoded != NULL && encoded_size > 0, "HEIF metadata write failed\n");
    if (encoded != NULL && decoded_metadata != NULL) {
        gdHeifInfoInit(&info);
        info.metadata = decoded_metadata;
        gdTestAssertMsg(gdHeifGetInfoPtr(encoded_size, encoded, &info) == GD_META_OK,
                        "HEIF metadata read failed\n");
        assert_exif(decoded_metadata);
    }
    gdFree(encoded);
    gdImageMetadataFree(decoded_metadata);
    gdImageMetadataFree(metadata);
    gdImageDestroy(image);
}
#endif

#ifdef GD_TEST_HAVE_JXL
static void test_jxl(const unsigned char *data, size_t data_size)
{
    gdImagePtr image = make_image();
    gdImageMetadata *metadata = make_metadata(data, data_size);
    gdImageMetadata *decoded_metadata = gdImageMetadataCreate();
    gdJxlWriteOptions options;
    gdJxlReadPtr reader;
    void *encoded;
    int encoded_size = 0;

    gdJxlWriteOptionsInit(&options);
    options.metadata = metadata;
    encoded = gdImageJxlPtrWithOptions(image, &encoded_size, &options);
    gdTestAssertMsg(encoded != NULL && encoded_size > 0, "JPEG XL metadata write failed\n");
    reader = encoded != NULL ? gdJxlReadOpenPtr(encoded_size, encoded, NULL) : NULL;
    if (reader != NULL && decoded_metadata != NULL) {
        gdTestAssertMsg(gdJxlReadGetMetadata(reader, decoded_metadata) == GD_META_OK,
                        "JPEG XL metadata read failed\n");
        assert_exif(decoded_metadata);
    }
    gdJxlReadClose(reader);
    gdFree(encoded);
    gdImageMetadataFree(decoded_metadata);
    gdImageMetadataFree(metadata);
    gdImageDestroy(image);
}
#endif

#ifdef GD_TEST_HAVE_TIFF
static void test_tiff_metadata(void)
{
    static const unsigned char profile[] = {
        'G', 'D', 'T', 'F', 1, 1, 3, 0, 4, 0, 0, 0, 8, 0, 0, 0,
        1, 0, 2, 0, 3, 0, 4, 0
    };
    gdImagePtr image = make_image();
    gdImageMetadata *metadata = gdImageMetadataCreate();
    gdImageMetadata *decoded_metadata = gdImageMetadataCreate();
    gdTiffWriteOptions options;
    gdTiffWritePtr writer;
    gdTiffReadPtr reader;
    const unsigned char *decoded;
    size_t decoded_size;
    void *encoded;
    int encoded_size = 0;

    gdTestAssertMsg(metadata != NULL && decoded_metadata != NULL, "TIFF metadata allocation failed\n");
    if (metadata == NULL || decoded_metadata == NULL) {
        gdImageMetadataFree(metadata);
        gdImageMetadataFree(decoded_metadata);
        gdImageDestroy(image);
        return;
    }
    gdTestAssertMsg(gdImageMetadataSetProfile(metadata, "tiff:tag:34735", profile,
                                              sizeof(profile)) == GD_META_OK,
                    "TIFF metadata profile setup failed\n");
    gdTiffWriteOptionsInit(&options);
    options.metadata = metadata;
    writer = gdTiffWriteOpenPtr(&options);
    gdTestAssertMsg(writer != NULL, "TIFF metadata writer open failed\n");
    if (writer != NULL) {
        gdTestAssertMsg(gdTiffWriteAddImage(writer, image) == 1, "TIFF metadata image write failed\n");
    }
    encoded = writer != NULL ? gdTiffWritePtrFinish(writer, &encoded_size) : NULL;
    gdTestAssertMsg(encoded != NULL && encoded_size > 0, "TIFF metadata write failed\n");
    reader = encoded != NULL ? gdTiffReadOpenPtr(encoded_size, encoded, NULL) : NULL;
    gdTestAssertMsg(reader != NULL, "TIFF metadata reader open failed\n");
    if (reader != NULL) {
        gdTestAssertMsg(gdTiffReadGetMetadata(reader, decoded_metadata) == 1,
                        "TIFF metadata read failed\n");
        decoded = gdImageMetadataGetProfile(decoded_metadata, "tiff:tag:34735", &decoded_size);
        gdTestAssertMsg(decoded != NULL && decoded_size == sizeof(profile) &&
                        memcmp(decoded, profile, sizeof(profile)) == 0,
                        "TIFF metadata profile changed\n");
    }
    gdTiffReadClose(reader);
    gdFree(encoded);
    gdImageMetadataFree(decoded_metadata);
    gdImageMetadataFree(metadata);
    gdImageDestroy(image);
}
#endif

int main(void)
{
#ifdef GD_TEST_HAVE_JPEG
    test_jpeg(raw_exif, sizeof(raw_exif));
    test_jpeg(prefixed_exif, sizeof(prefixed_exif));
    test_jpeg_invalid();
#endif
#ifdef GD_TEST_HAVE_PNG
    test_png(raw_exif, sizeof(raw_exif));
    test_png(prefixed_exif, sizeof(prefixed_exif));
    test_png_invalid();
#endif
#ifdef GD_TEST_HAVE_WEBP
    test_webp(raw_exif, sizeof(raw_exif));
    test_webp(prefixed_exif, sizeof(prefixed_exif));
#endif
#ifdef GD_TEST_HAVE_AVIF
    test_avif(raw_exif, sizeof(raw_exif));
    test_avif(prefixed_exif, sizeof(prefixed_exif));
#endif
#ifdef GD_TEST_HAVE_HEIF
    test_heif(raw_exif, sizeof(raw_exif));
    test_heif(prefixed_exif, sizeof(prefixed_exif));
#endif
#ifdef GD_TEST_HAVE_JXL
    test_jxl(raw_exif, sizeof(raw_exif));
    test_jxl(prefixed_exif, sizeof(prefixed_exif));
#endif
#ifdef GD_TEST_HAVE_TIFF
    test_tiff_metadata();
#endif
    gdTestAssertMsg(memcmp(prefixed_exif,
                           "Exif\0\0II*\0\010\0\0\0", sizeof(prefixed_exif)) == 0,
                    "EXIF input buffer was modified\n");
    return gdNumFailures();
}
