#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gd_avif_metadata.h"
#include "gd_intern.h"
#include "gdhelpers.h"

#include <limits.h>
#include <string.h>

#ifdef HAVE_LIBAVIF

static int gd_avif_set_profile(gdImageMetadata *metadata, const char *key,
                               const avifRWData *profile)
{
    const unsigned char *data = profile->data;
    size_t size = profile->size;

    if (metadata == NULL) {
        return GD_META_OK;
    }
    if (profile->size == 0) {
        return GD_META_OK;
    }
    if (strcmp(key, "exif") == 0) {
        if (gdMetadataGetExifTiff(data, size, &data, &size) != GD_META_OK) {
            return GD_META_ERR_PARSE;
        }
    }
    return gdImageMetadataSetProfile(metadata, key, data, size);
}

BGD_DECLARE(void) gdAvifInfoInit(gdAvifInfo *info)
{
    if (info == NULL) {
        return;
    }
    memset(info, 0, sizeof(*info));
}

BGD_DECLARE(int) gdAvifGetInfoPtr(int size, const void *data, gdAvifInfo *info)
{
    avifDecoder *decoder = NULL;
    avifResult result;
    int status;
    gdImageMetadata *metadata;

    if (size < 0 || data == NULL || info == NULL) {
        return GD_META_ERR_INVALID;
    }

    metadata = info->metadata;
    memset(info, 0, sizeof(*info));
    info->metadata = metadata;

    decoder = avifDecoderCreate();
    if (decoder == NULL) {
        return GD_META_ERR_NOMEM;
    }

#if AVIF_VERSION >= 90100
    decoder->strictFlags &= ~AVIF_STRICT_PIXI_REQUIRED;
#endif

    result = avifDecoderSetIOMemory(decoder, data, (size_t) size);
    if (result != AVIF_RESULT_OK) {
        avifDecoderDestroy(decoder);
        return GD_META_ERR_PARSE;
    }

    result = avifDecoderParse(decoder);
    if (result != AVIF_RESULT_OK || decoder->image == NULL) {
        avifDecoderDestroy(decoder);
        return GD_META_ERR_PARSE;
    }

    if (decoder->image->width > INT_MAX || decoder->image->height > INT_MAX) {
        avifDecoderDestroy(decoder);
        return GD_META_ERR_LIMIT;
    }

    if (decoder->image->depth > INT_MAX || decoder->imageCount > INT_MAX) {
        avifDecoderDestroy(decoder);
        return GD_META_ERR_LIMIT;
    }

    info->width = (int) decoder->image->width;
    info->height = (int) decoder->image->height;
    info->is_progressive = decoder->progressiveState != AVIF_PROGRESSIVE_STATE_UNAVAILABLE;
    info->is_animation = decoder->imageCount > 1;
    info->frame_count = decoder->imageCount;
    info->duration = decoder->duration;
    info->has_alpha = decoder->alphaPresent == AVIF_TRUE;
    info->bit_depth = (int) decoder->image->depth;
    switch (decoder->image->yuvFormat) {
        case AVIF_PIXEL_FORMAT_YUV444:
            info->yuv_format = GD_AVIF_PIXEL_FORMAT_YUV444;
            break;
        case AVIF_PIXEL_FORMAT_YUV422:
            info->yuv_format = GD_AVIF_PIXEL_FORMAT_YUV422;
            break;
        case AVIF_PIXEL_FORMAT_YUV420:
            info->yuv_format = GD_AVIF_PIXEL_FORMAT_YUV420;
            break;
        case AVIF_PIXEL_FORMAT_YUV400:
            info->yuv_format = GD_AVIF_PIXEL_FORMAT_YUV400;
            break;
        default:
            info->yuv_format = GD_AVIF_PIXEL_FORMAT_NONE;
            break;
    }
    status = gd_avif_set_profile(info->metadata, "exif", &decoder->image->exif);
    if (status == GD_META_OK) {
        status = gd_avif_set_profile(info->metadata, "xmp", &decoder->image->xmp);
    }

    avifDecoderDestroy(decoder);
    return status;
}

static int gd_avif_read_ctx_to_memory(gdIOCtxPtr in, void **data, int *size)
{
    unsigned char *buffer = NULL;
    size_t used = 0;
    size_t capacity = 0;
    int count;

    if (in == NULL || data == NULL || size == NULL) {
        return GD_META_ERR_INVALID;
    }
    for (;;) {
        unsigned char *next;

        if (used > INT_MAX - 4096) {
            gdFree(buffer);
            return GD_META_ERR_LIMIT;
        }
        if (capacity - used < 4096) {
            capacity += 4096;
            next = gdRealloc(buffer, capacity);
            if (next == NULL) {
                gdFree(buffer);
                return GD_META_ERR_NOMEM;
            }
            buffer = next;
        }
        count = gdGetBuf(buffer + used, 4096, in);
        if (count < 0) {
            gdFree(buffer);
            return GD_META_ERR_PARSE;
        }
        used += (size_t) count;
        if (count != 4096) {
            break;
        }
    }
    *data = buffer;
    *size = (int) used;
    return GD_META_OK;
}

BGD_DECLARE(int) gdAvifGetInfoCtx(gdIOCtxPtr in, gdAvifInfo *info)
{
    void *data;
    int size;
    int status = gd_avif_read_ctx_to_memory(in, &data, &size);

    if (status != GD_META_OK) {
        return status;
    }
    status = gdAvifGetInfoPtr(size, data, info);
    gdFree(data);
    return status;
}

BGD_DECLARE(int) gdAvifGetInfo(FILE *inFile, gdAvifInfo *info)
{
    gdIOCtx *in;
    int status;

    if (inFile == NULL) {
        return GD_META_ERR_INVALID;
    }
    in = gdNewFileCtx(inFile);
    if (in == NULL) {
        return GD_META_ERR_NOMEM;
    }
    status = gdAvifGetInfoCtx(in, info);
    in->gd_free(in);
    return status;
}

int gdAvifApplyMetadata(avifImage *image, const gdImageMetadata *metadata)
{
    const unsigned char *data;
    size_t size;
    avifResult result;

    if (image == NULL || metadata == NULL) {
        return GD_META_OK;
    }

    data = gdImageMetadataGetProfile(metadata, "exif", &size);
    if (data != NULL) {
        if (gdMetadataGetExifTiff(data, size, &data, &size) != GD_META_OK) {
            return GD_META_ERR_PARSE;
        }
        result = avifImageSetMetadataExif(image, data, size);
        if (result != AVIF_RESULT_OK) {
            return GD_META_ERR_INVALID;
        }
    }

    data = gdImageMetadataGetProfile(metadata, "xmp", &size);
    if (data != NULL) {
        if (size > INT_MAX) {
            return GD_META_ERR_LIMIT;
        }
        result = avifImageSetMetadataXMP(image, data, size);
        if (result != AVIF_RESULT_OK) {
            return GD_META_ERR_INVALID;
        }
    }

    return GD_META_OK;
}

#else

BGD_DECLARE(void) gdAvifInfoInit(gdAvifInfo *info)
{
    if (info != NULL) {
        memset(info, 0, sizeof(*info));
    }
}

BGD_DECLARE(int) gdAvifGetInfoPtr(int size, const void *data, gdAvifInfo *info)
{
    (void) size;
    (void) data;
    (void) info;
    return GD_META_ERR_UNSUPPORTED;
}

BGD_DECLARE(int) gdAvifGetInfoCtx(gdIOCtxPtr in, gdAvifInfo *info)
{
    (void) in;
    (void) info;
    return GD_META_ERR_UNSUPPORTED;
}

BGD_DECLARE(int) gdAvifGetInfo(FILE *inFile, gdAvifInfo *info)
{
    (void) inFile;
    (void) info;
    return GD_META_ERR_UNSUPPORTED;
}

#endif
