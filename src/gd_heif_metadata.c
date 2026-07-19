#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gd_heif_metadata.h"
#include "gd_intern.h"
#include "gdhelpers.h"

#include <limits.h>
#include <string.h>

#ifdef HAVE_LIBHEIF

#define GD_HEIF_METADATA_ALLOC_STEP (4 * 1024)

static int gd_heif_set_profile(gdImageMetadata *metadata, const char *key,
                                const void *data, size_t size)
{
    if (gdImageMetadataGetProfile(metadata, key, NULL) != NULL) {
        return GD_META_OK;
    }
    return gdImageMetadataSetProfile(metadata, key, (const unsigned char *) data, size);
}

static int gd_heif_collect_metadata(const struct heif_image_handle *handle,
                                     gdImageMetadata *metadata)
{
    int count, i;
    heif_item_id *ids = NULL;

    if (handle == NULL) {
        return GD_META_ERR_INVALID;
    }

    if (metadata == NULL) {
        return GD_META_OK;
    }

    count = heif_image_handle_get_number_of_metadata_blocks(handle, NULL);
    if (count < 0) {
        return GD_META_ERR_PARSE;
    }
    if (count > 0) {
        ids = (heif_item_id *) gdMalloc((size_t) count * sizeof(*ids));
        if (ids == NULL) {
            return GD_META_ERR_NOMEM;
        }
        count = heif_image_handle_get_list_of_metadata_block_IDs(handle, NULL, ids, count);
    }

    for (i = 0; i < count; i++) {
        const char *type = heif_image_handle_get_metadata_type(handle, ids[i]);
        const char *content_type = heif_image_handle_get_metadata_content_type(handle, ids[i]);
        const char *key = NULL;
        size_t size;
        unsigned char *data;
        const unsigned char *profile_data;
        struct heif_error error;

        if (type != NULL && strcmp(type, "Exif") == 0) {
            key = "exif";
        } else if (type != NULL && strcmp(type, "iptc") == 0) {
            key = "iptc";
        } else if (type != NULL && strcmp(type, "mime") == 0 && content_type != NULL &&
                   strcmp(content_type, "application/rdf+xml") == 0) {
            key = "xmp";
        }
        if (key == NULL || gdImageMetadataGetProfile(metadata, key, NULL) != NULL) {
            continue;
        }

        size = heif_image_handle_get_metadata_size(handle, ids[i]);
        data = (unsigned char *) gdMalloc(size ? size : 1);
        if (data == NULL) {
            gdFree(ids);
            return GD_META_ERR_NOMEM;
        }
        error = heif_image_handle_get_metadata(handle, ids[i], data);
        if (error.code != heif_error_Ok) {
            gdFree(data);
            gdFree(ids);
            return GD_META_ERR_PARSE;
        }
        profile_data = data;
        if (key != NULL && strcmp(key, "exif") == 0) {
            size_t offset;
            const unsigned char *tiff_data;
            if (size < 4) {
                gdFree(data);
                gdFree(ids);
                return GD_META_ERR_PARSE;
            }
            offset = ((size_t)data[0] << 24) | ((size_t)data[1] << 16) |
                ((size_t)data[2] << 8) | (size_t)data[3];
            if (offset > size - 4) {
                gdFree(data);
                gdFree(ids);
                return GD_META_ERR_PARSE;
            }
            profile_data = data + 4 + offset;
            size -= 4 + offset;
            if (gdMetadataGetExifTiff(profile_data, size, &tiff_data, &size) != GD_META_OK) {
                gdFree(data);
                gdFree(ids);
                return GD_META_ERR_PARSE;
            }
            profile_data = tiff_data;
        }
        if (gd_heif_set_profile(metadata, key, profile_data, size) != GD_META_OK) {
            gdFree(data);
            gdFree(ids);
            return GD_META_ERR_LIMIT;
        }
        gdFree(data);
    }
    gdFree(ids);

    return GD_META_OK;
}

static int gd_heif_is_animation(const void *data, int size)
{
    static const char *const sequence_brands[] = {
        "hevc", "hevx", "hevm", "hevs", "avis", "msf1", "vvis", "evbs", "evms", "jpgs", "j2is"
    };
    size_t i;

    for (i = 0; i < sizeof(sequence_brands) / sizeof(sequence_brands[0]); i++) {
        if (heif_has_compatible_brand((const uint8_t *) data, size, sequence_brands[i])) {
            return 1;
        }
    }
    return 0;
}

BGD_DECLARE(void) gdHeifInfoInit(gdHeifInfo *info)
{
    if (info == NULL) {
        return;
    }
    memset(info, 0, sizeof(*info));
}

BGD_DECLARE(int) gdHeifGetInfoPtr(int size, const void *data, gdHeifInfo *info)
{
    struct heif_context *context;
    struct heif_image_handle *handle = NULL;
    struct heif_error error;
    int status;
    gdImageMetadata *metadata;

    if (size < 0 || data == NULL || info == NULL) {
        return GD_META_ERR_INVALID;
    }
    metadata = info->metadata;
    memset(info, 0, sizeof(*info));
    info->metadata = metadata;
    context = heif_context_alloc();
    if (context == NULL) {
        return GD_META_ERR_NOMEM;
    }
    error = heif_context_read_from_memory_without_copy(context, data, (size_t) size, NULL);
    if (error.code != heif_error_Ok) {
        heif_context_free(context);
        return GD_META_ERR_PARSE;
    }
    error = heif_context_get_primary_image_handle(context, &handle);
    if (error.code != heif_error_Ok || handle == NULL) {
        heif_context_free(context);
        return GD_META_ERR_PARSE;
    }
    info->width = heif_image_handle_get_width(handle);
    info->height = heif_image_handle_get_height(handle);
    info->top_level_image_count = heif_context_get_number_of_top_level_images(context);
    info->has_alpha = heif_image_handle_has_alpha_channel(handle) != 0;
    info->bit_depth = heif_image_handle_get_luma_bits_per_pixel(handle);
    info->is_animation = gd_heif_is_animation(data, size);
    status = gd_heif_collect_metadata(handle, info->metadata);
    heif_image_handle_release(handle);
    heif_context_free(context);
    return status;
}

static int gd_heif_read_ctx_to_memory(gdIOCtxPtr in, void **data, int *size)
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

        if (used > INT_MAX - GD_HEIF_METADATA_ALLOC_STEP) {
            gdFree(buffer);
            return GD_META_ERR_LIMIT;
        }
        if (capacity - used < GD_HEIF_METADATA_ALLOC_STEP) {
            capacity += GD_HEIF_METADATA_ALLOC_STEP;
            next = (unsigned char *) gdRealloc(buffer, capacity);
            if (next == NULL) {
                gdFree(buffer);
                return GD_META_ERR_NOMEM;
            }
            buffer = next;
        }
        count = gdGetBuf(buffer + used, GD_HEIF_METADATA_ALLOC_STEP, in);
        if (count < 0) {
            gdFree(buffer);
            return GD_META_ERR_PARSE;
        }
        used += (size_t) count;
        if (count != GD_HEIF_METADATA_ALLOC_STEP) {
            break;
        }
    }
    *data = buffer;
    *size = (int) used;
    return GD_META_OK;
}

BGD_DECLARE(int) gdHeifGetInfoCtx(gdIOCtxPtr in, gdHeifInfo *info)
{
    void *data;
    int size;
    int status = gd_heif_read_ctx_to_memory(in, &data, &size);

    if (status != GD_META_OK) {
        return status;
    }
    status = gdHeifGetInfoPtr(size, data, info);
    gdFree(data);
    return status;
}

BGD_DECLARE(int) gdHeifGetInfo(FILE *inFile, gdHeifInfo *info)
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
    status = gdHeifGetInfoCtx(in, info);
    in->gd_free(in);
    return status;
}

static int gd_heif_apply_profile(const gdImageMetadata *metadata, const char *key,
                                 const unsigned char **data, size_t *size)
{
    *data = gdImageMetadataGetProfile(metadata, key, size);
    if (*data == NULL) {
        return GD_META_OK;
    }
    return *size > INT_MAX ? GD_META_ERR_LIMIT : GD_META_OK;
}

int gdHeifApplyMetadata(struct heif_context *context, struct heif_image *image,
                        struct heif_image_handle *handle, const gdImageMetadata *metadata)
{
    const unsigned char *data;
    size_t size;
    struct heif_error error;

    (void) image;

    if (metadata == NULL) {
        return GD_META_OK;
    }
    if (handle == NULL || context == NULL) {
        return GD_META_ERR_INVALID;
    }

    if (gd_heif_apply_profile(metadata, "exif", &data, &size) != GD_META_OK) {
        return GD_META_ERR_LIMIT;
    }
    if (data != NULL) {
        if (gdMetadataGetExifTiff(data, size, &data, &size) != GD_META_OK) {
            return GD_META_ERR_PARSE;
        }
        error = heif_context_add_exif_metadata(context, handle, data, (int) size);
        if (error.code != heif_error_Ok) {
            return GD_META_ERR_INVALID;
        }
    }
    if (gd_heif_apply_profile(metadata, "xmp", &data, &size) != GD_META_OK) {
        return GD_META_ERR_LIMIT;
    }
    if (data != NULL) {
        error = heif_context_add_XMP_metadata(context, handle, data, (int) size);
        if (error.code != heif_error_Ok) {
            return GD_META_ERR_INVALID;
        }
    }
    if (gd_heif_apply_profile(metadata, "iptc", &data, &size) != GD_META_OK) {
        return GD_META_ERR_LIMIT;
    }
    if (data != NULL) {
        error = heif_context_add_generic_metadata(context, handle, data, (int) size,
                                                   "iptc", NULL);
        if (error.code != heif_error_Ok) {
            return GD_META_ERR_INVALID;
        }
    }
    return GD_META_OK;
}

#else

BGD_DECLARE(void) gdHeifInfoInit(gdHeifInfo *info)
{
    if (info != NULL) {
        memset(info, 0, sizeof(*info));
    }
}

BGD_DECLARE(int) gdHeifGetInfoPtr(int size, const void *data, gdHeifInfo *info)
{
    (void) size;
    (void) data;
    (void) info;
    return GD_META_ERR_UNSUPPORTED;
}

BGD_DECLARE(int) gdHeifGetInfoCtx(gdIOCtxPtr in, gdHeifInfo *info)
{
    (void) in;
    (void) info;
    return GD_META_ERR_UNSUPPORTED;
}

BGD_DECLARE(int) gdHeifGetInfo(FILE *inFile, gdHeifInfo *info)
{
    (void) inFile;
    (void) info;
    return GD_META_ERR_UNSUPPORTED;
}

#endif
