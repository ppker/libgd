# Metadata handling in libgd {#metadata}

This page explains how to attach, read, and move metadata with libgd. The same
`gdImageMetadata` object can be passed through several codecs. Applications do
not need to know how a codec stores EXIF, XMP, or other profiles internally.

The complete working example is
[`examples/metadata_libexif.c`](../examples/metadata_libexif.c). It reads EXIF
from a JPEG, edits it with libexif, and writes the same metadata object to both
JPEG and PNG.

## The basic model

Metadata is an optional collection of named binary profiles:

| Profile name | Typical contents |
| --- | --- |
| `"exif"` | EXIF/TIFF bytes |
| `"xmp"` | XMP XML packet bytes |
| `"iptc"` | IPTC/IIM bytes |
| `"icc"` | Not part of the public metadata path |

The profile name is a GD-level key. The profile data is opaque to the metadata
container; the selected codec interprets supported keys when reading or
writing.

Create the object once, add profiles to it, and release it when it is no longer
needed:

```c
gdImageMetadata *metadata;
static const unsigned char xmp[] = "<x:xmpmeta>...</x:xmpmeta>";

metadata = gdImageMetadataCreate();
if (metadata == NULL) {
    /* handle allocation failure */
}

if (gdImageMetadataSetProfile(metadata, "xmp", xmp, sizeof(xmp) - 1) != GD_META_OK) {
    gdImageMetadataFree(metadata);
    /* handle an invalid profile, size limit, or allocation failure */
}

/* use metadata with one or more codec writers */

gdImageMetadataFree(metadata);
```

`gdImageMetadataSetProfile()` copies the supplied bytes. The caller may reuse
or release its input buffer after the call. Replacing an existing profile keeps
one profile under that name; use `gdImageMetadataRemoveProfile()` to remove it
explicitly.

The default safety limits are 64 MiB per profile and 256 MiB total metadata.
Applications processing larger trusted profiles can change them with
`gdImageMetadataSetLimits()`.

## Writing metadata

Always initialize the codec write-options structure before setting fields. The
metadata pointer is borrowed for the duration of the write, so the metadata
object must remain alive until the writer returns.

For example, writing JPEG and PNG from the same object:

```c
gdJpegWriteOptions jpeg_options;
gdPngWriteOptions png_options;
int jpeg_size = 0;
int png_size = 0;
void *jpeg_data;
void *png_data;

gdJpegWriteOptionsInit(&jpeg_options);
jpeg_options.quality = 90;
jpeg_options.metadata = metadata;
jpeg_data = gdImageJpegPtrWithOptions(image, &jpeg_size, &jpeg_options);

gdPngWriteOptionsInit(&png_options);
png_options.metadata = metadata;
png_data = gdImagePngPtrWithOptions(image, &png_size, &png_options);

/* pointer-writer results are released with gdFree() */
gdFree(jpeg_data);
gdFree(png_data);
```

The same pattern applies to WebP, AVIF, HEIF, and JPEG XL write options. A
profile unsupported by a codec is ignored according to that codec's metadata
contract; it does not need to be removed from the shared object first.

## EXIF is transparent across codecs

EXIF consists of a TIFF payload. For convenience and compatibility, GD writers
accept either:

1. raw TIFF bytes beginning with `II` or `MM`, or
2. the same TIFF bytes preceded by `Exif\0\0`.

Applications should pass the metadata object directly between codecs. They
should not add JPEG APP1 headers, PNG chunk headers, WebP chunk headers, or
four-byte ISO-BMFF/JPEG XL offsets themselves.

GD performs the required conditioning internally:

| Codec | Physical representation produced by GD |
| --- | --- |
| JPEG | APP1 EXIF with exactly one `Exif\0\0` prefix |
| PNG | Official `eXIf` chunk containing raw TIFF |
| WebP | `EXIF` RIFF chunk containing raw TIFF |
| AVIF | libavif receives raw TIFF and owns the container offset |
| HEIF | libheif receives raw TIFF and owns the container offset |
| JPEG XL | GD adds the required four-byte TIFF offset for the low-level box API |

Before writing, GD validates the TIFF byte order and magic number. Invalid or
truncated EXIF data is rejected rather than written in a form that could lose
metadata or produce a corrupt file.

### Reading EXIF

Metadata is collected into an object supplied by the caller. For codecs with
an info structure, attach the object before probing the encoded data:

```c
gdPngInfo info;
gdImageMetadata *metadata = gdImageMetadataCreate();

gdPngInfoInit(&info);
info.metadata = metadata;
if (gdPngGetInfoPtr(size, data, &info) != GD_META_OK) {
    gdImageMetadataFree(metadata);
    /* handle an invalid PNG */
}

/* metadata now contains profiles found in the PNG */
gdImageMetadataFree(metadata);
```

JPEG uses `gdJpegGetMetadataPtr()`. WebP uses `gdWebpReadGetMetadata()`, while
AVIF and HEIF return metadata through `gdAvifInfo` and `gdHeifInfo`. JPEG XL
uses `gdJxlReadGetMetadata()`.

The established reader representation is preserved where necessary for
compatibility. In particular, PNG EXIF metadata may include the `Exif\0\0`
identifier when returned through `gdImageMetadata`; writers accept that form
and normalize it automatically.

## Legacy PNG EXIF

Older tools commonly store EXIF in a compressed `zTXt` chunk named
`Raw profile type exif`. GD continues to read this legacy form. When the
metadata is written again, GD decodes it and emits the standardized `eXIf`
chunk instead. New applications should use the normal `"exif"` profile and do
not need to construct PNG chunks or legacy text profiles.

## XMP, IPTC, and ICC

XMP is supplied as raw XML packet data under the `"xmp"` key. GD maps it to the
native metadata container of each supported codec.

IPTC is supplied as raw IPTC/IIM bytes under `"iptc"` where the codec supports
it. Codecs without a native IPTC mapping silently ignore that profile; GD does
not invent an XMP conversion and does not alter other profiles.

ICC is deliberately not part of the public `gdImageMetadata` path yet. All
codecs discard ICC profiles when reading and ignore an `"icc"` entry when
writing. GD does not apply ICC profiles while decoding, transform pixels based
on them, or claim that embedding one makes the pixel data color-managed.

UHDR is a separate boundary: libuhdr owns its ICC data and GD preserves it
internally while handling UHDR. It must not be routed through
`gdImageMetadata`. ICC support in the common metadata API can be added later,
but only together with explicit color-profile semantics, tests, and
documentation.

## TIFF metadata

TIFF uses a separate metadata mechanism. Its `tiff:tag:*` profiles use GD's
`GDTF` encoding for supported TIFF tags and are not EXIF profiles. TIFF metadata
is therefore tested and handled independently from the EXIF prefix and
container-offset rules described above.

TIFF writing is multi-page capable:

```c
gdTiffWriteOptions options;
gdTiffWritePtr writer;

gdTiffWriteOptionsInit(&options);
options.metadata = metadata;
writer = gdTiffWriteOpenPtr(&options);
if (writer != NULL) {
    if (!gdTiffWriteAddImage(writer, image)) {
        gdTiffWriteClose(writer);
        /* handle the write error */
    }
    /* use gdTiffWritePtrFinish(writer, &size) for a memory result */
}
```

## UHDR and gain-map metadata

UHDR is intentionally different. Gain maps, gain-map metadata, and related
Ultra HDR information belong to libuhdr. They are not represented by GD's
ordinary `gdImageMetadata` object and must not be copied through GD metadata
profiles. Use the UHDR APIs for UHDR input and output so gain maps and their
metadata remain untouched.

## Validating results

For important workflows, validate the output with a format-aware tool such as
Exiv2 and test both directions of a conversion. A useful test pattern is:

1. Read metadata into one `gdImageMetadata` object.
2. Write the image to each required destination codec using that same object.
3. Read metadata back from each destination.
4. Compare the profile bytes and verify codec-specific framing.

The repository's `tests/metadata/codec_metadata.c` demonstrates this matrix for
all enabled codecs, including raw and prefixed EXIF input, malformed EXIF
rejection, and TIFF profile round trips.
