#ifndef GD_VECTOR2D_H
#define GD_VECTOR2D_H

#include "gd.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Experimental API: source and ABI compatibility are not yet guaranteed. */
#define GD_VECTOR2D_EXPERIMENTAL 1
#define GD_VECTOR2D_VERSION 1

/**
 * Group: Vector 2D
 *
 * typedef: gdContext
 *
 * Opaque drawing context for the 2D API. A context owns the current path and a
 * stack of graphics state, and draws into an ARGB surface backed by a truecolor
 * <gdImage>.
 *
 * See also:
 *  - <gdContextCreateForImage>
 *  - <gdContextDestroy>
 */
typedef struct gdContextStruct gdContext;
typedef gdContext *gdContextPtr;

/**
 * typedef: gdPath
 *
 * Opaque vector path made of contours, lines, curves, arcs, and rectangles.
 *
 * See also:
 *  - <gdPathCreate>
 *  - <gdContextAppendPath>
 */
typedef struct gdPathStruct gdPath;
typedef gdPath *gdPathPtr;

/**
 * typedef: gdPaint
 *
 * Opaque paint source. A paint may be a solid color, image pattern, or gradient.
 *
 * See also:
 *  - <gdContextSetSource>
 *  - <gdPaintCreateFromPattern>
 *  - <gdPaintCreateLinear>
 */
typedef struct gdPaintStruct gdPaint;
typedef gdPaint *gdPaintPtr;

/**
 * typedef: gdPathPattern
 *
 * Opaque image pattern used as a paint source.
 *
 * See also:
 *  - <gdPathPatternCreateForImage>
 *  - <gdPathPatternSetFilter>
 */
typedef struct gdPathPatternStruct gdPathPattern;
typedef gdPathPattern *gdPathPatternPtr;

/**
 * typedef: gdFontFace
 *
 * Opaque font face used by text APIs.
 *
 * See also:
 *  - <gdFontFaceCreateFromFile>
 *  - <gdContextSetFontFace>
 */
typedef struct gdFontFaceStruct gdFontFace;
typedef gdFontFace *gdFontFacePtr;

/**
 * struct: gdPathMatrix
 *
 * Affine transformation matrix used by paths, contexts, paints, and patterns.
 *
 * Points are mapped as:
 * (x * m00 + y * m01 + m02, x * m10 + y * m11 + m12).
 *
 * See also:
 *  - <gdPathMatrixInit>
 *  - <gdContextTransform>
 */
typedef struct gdPathMatrixStruct {
    double m00, m10, m01, m11, m02, m12;
} gdPathMatrix;
typedef gdPathMatrix *gdPathMatrixPtr;

/**
 * struct: gdRectF
 *
 * Floating-point rectangle.
 *
 * Members:
 *   x - Left coordinate.
 *   y - Top coordinate.
 *   w - Width.
 *   h - Height.
 */
typedef struct gdRectFStruct {
    double x, y, w, h;
} gdRectF;
typedef gdRectF *gdRectFPtr;

/**
 * Constants: gdExtendMode
 *
 * Describes how paints are sampled outside their natural bounds.
 *
 *  GD_EXTEND_NONE    - Outside samples are transparent.
 *  GD_EXTEND_REPEAT  - Repeat the source.
 *  GD_EXTEND_REFLECT - Repeat the source with alternating mirrored copies.
 *  GD_EXTEND_PAD     - Clamp to the nearest edge sample.
 *
 * See also:
 *  - <gdPaintSetExtend>
 *  - <gdPathPatternSetExtend>
 */
typedef enum { GD_EXTEND_NONE, GD_EXTEND_REPEAT, GD_EXTEND_REFLECT, GD_EXTEND_PAD } gdExtendMode;

/**
 * Constants: gdLineCap
 *
 * Stroke endpoint style.
 *
 *  gdLineCapButt   - End the stroke at the endpoint.
 *  gdLineCapRound  - Add a round cap.
 *  gdLineCapSquare - Add a square cap.
 *
 * See also:
 *  - <gdContextSetLineCap>
 */
typedef enum { gdLineCapButt, gdLineCapRound, gdLineCapSquare } gdLineCap;

/**
 * Constants: gdLineJoin
 *
 * Stroke corner style.
 *
 *  gdLineJoinMiter - Join segments with a miter when possible.
 *  gdLineJoinRound - Join segments with a round corner.
 *  gdLineJoinBevel - Join segments with a bevel.
 *
 * See also:
 *  - <gdContextSetLineJoin>
 */
typedef enum { gdLineJoinMiter, gdLineJoinRound, gdLineJoinBevel } gdLineJoin;

/**
 * Constants: gdFillRule
 *
 * Rule used to decide which parts of a path are inside the fill.
 *
 *  gdFillRuleNonZero - Non-zero winding rule.
 *  gdFillRuleEvenOdd - Even-odd rule.
 *
 * See also:
 *  - <gdContextSetFillRule>
 */
typedef enum { gdFillRuleNonZero, gdFillRuleEvenOdd } gdFillRule;

/**
 * Group: Vector 2D
 *
 * Constants: gdPatternFilter
 *
 * Selects the image sampling quality used when an image pattern is transformed
 * while filling or painting through the 2D API.
 *
 *  GD_PATTERN_FILTER_FAST - Fastest sampling. Uses nearest-neighbour sampling.
 *  GD_PATTERN_FILTER_GOOD - Balanced quality. Uses bilinear sampling and
 *                           mipmapped sampling for minification.
 *  GD_PATTERN_FILTER_BEST - Highest quality. Uses Mitchell bicubic sampling
 *                           where appropriate and trilinear mipmapped sampling
 *                           for minification.
 *
 * The constants describe the requested quality level, not a fixed low-level
 * kernel. The exact sampler used for GOOD or BEST may change between releases
 * to improve output while keeping the public API stable.
 *
 * See also:
 *  - <gdContextSetPatternFilter>
 *  - <gdPathPatternSetFilter>
 */
typedef enum {
    GD_PATTERN_FILTER_FAST,
    GD_PATTERN_FILTER_GOOD,
    GD_PATTERN_FILTER_BEST
} gdPatternFilter;

/**
 * Constants: gdCompositeOperator
 *
 * Porter-Duff and blend operators used when painting source pixels over the
 * destination.
 *
 *  GD_OP_CLEAR          - Clear the destination.
 *  GD_OP_SOURCE         - Replace with the source.
 *  GD_OP_OVER           - Draw source over destination.
 *  GD_OP_IN             - Keep source only where destination exists.
 *  GD_OP_OUT            - Keep source only where destination is transparent.
 *  GD_OP_ATOP           - Source atop destination.
 *  GD_OP_DEST           - Keep destination unchanged.
 *  GD_OP_DEST_OVER      - Draw destination over source.
 *  GD_OP_DEST_IN        - Keep destination only where source exists.
 *  GD_OP_DEST_OUT       - Keep destination only where source is transparent.
 *  GD_OP_DEST_ATOP      - Destination atop source.
 *  GD_OP_XOR            - Source and destination outside their overlap.
 *  GD_OP_ADD            - Add source and destination.
 *  GD_OP_SATURATE       - Saturating source-over operation.
 *  GD_OP_MULTIPLY       - Multiply blend mode.
 *  GD_OP_SCREEN         - Screen blend mode.
 *  GD_OP_OVERLAY        - Overlay blend mode.
 *  GD_OP_DARKEN         - Darken blend mode.
 *  GD_OP_LIGHTEN        - Lighten blend mode.
 *  GD_OP_COLOR_DODGE    - Color dodge blend mode.
 *  GD_OP_COLOR_BURN     - Color burn blend mode.
 *  GD_OP_HARD_LIGHT     - Hard light blend mode.
 *  GD_OP_SOFT_LIGHT     - Soft light blend mode.
 *  GD_OP_DIFFERENCE     - Difference blend mode.
 *  GD_OP_EXCLUSION      - Exclusion blend mode.
 *  GD_OP_HSL_HUE        - HSL hue blend mode.
 *  GD_OP_HSL_SATURATION - HSL saturation blend mode.
 *  GD_OP_HSL_COLOR      - HSL color blend mode.
 *  GD_OP_HSL_LUMINOSITY - HSL luminosity blend mode.
 *
 * See also:
 *  - <gdContextSetOperator>
 */
typedef enum {
    GD_OP_CLEAR,
    GD_OP_SOURCE,
    GD_OP_OVER,
    GD_OP_IN,
    GD_OP_OUT,
    GD_OP_ATOP,
    GD_OP_DEST,
    GD_OP_DEST_OVER,
    GD_OP_DEST_IN,
    GD_OP_DEST_OUT,
    GD_OP_DEST_ATOP,
    GD_OP_XOR,
    GD_OP_ADD,
    GD_OP_SATURATE,
    GD_OP_MULTIPLY,
    GD_OP_SCREEN,
    GD_OP_OVERLAY,
    GD_OP_DARKEN,
    GD_OP_LIGHTEN,
    GD_OP_COLOR_DODGE,
    GD_OP_COLOR_BURN,
    GD_OP_HARD_LIGHT,
    GD_OP_SOFT_LIGHT,
    GD_OP_DIFFERENCE,
    GD_OP_EXCLUSION,
    GD_OP_HSL_HUE,
    GD_OP_HSL_SATURATION,
    GD_OP_HSL_COLOR,
    GD_OP_HSL_LUMINOSITY,
    GD_OP_COUNT
} gdCompositeOperator;
typedef gdCompositeOperator gdImageOp;
#define gdImageOpsSrc GD_OP_SOURCE
#define gdImageOpsSrcOver GD_OP_OVER
#define gdImageOpsDstIn GD_OP_DEST_IN
#define gdImageOpsDstOut GD_OP_DEST_OUT

/**
 * Constants: gdTextStatus
 *
 * Status values returned by 2D text functions.
 *
 *  GD_TEXT_OK                 - The operation succeeded.
 *  GD_TEXT_E_INVALID_ARGUMENT - An argument was invalid.
 *  GD_TEXT_E_UNAVAILABLE      - Required text support is unavailable.
 *  GD_TEXT_E_FONT             - A font provider error occurred.
 *  GD_TEXT_E_LAYOUT           - Text layout or shaping failed.
 *  GD_TEXT_E_MEMORY           - Memory allocation failed.
 *
 * See also:
 *  - <gdTextError>
 */
typedef enum {
    GD_TEXT_OK = 0,
    GD_TEXT_E_INVALID_ARGUMENT,
    GD_TEXT_E_UNAVAILABLE,
    GD_TEXT_E_FONT,
    GD_TEXT_E_LAYOUT,
    GD_TEXT_E_MEMORY
} gdTextStatus;

#define GD_TEXT_ERROR_MESSAGE_SIZE 128

/**
 * struct: gdTextError
 *
 * Optional detailed error information for 2D text APIs.
 *
 * Members:
 *   code          - A <gdTextStatus> value.
 *   provider_code - Provider-specific error code, such as a FreeType error.
 *   message       - Human-readable error message.
 */
typedef struct {
    gdTextStatus code;
    int provider_code;
    char message[GD_TEXT_ERROR_MESSAGE_SIZE];
} gdTextError;

/**
 * struct: gdTextExtents
 *
 * Text metrics returned by <gdContextTextExtents>.
 *
 * Members:
 *   x_bearing  - Horizontal bearing of the text bounds.
 *   y_bearing  - Vertical bearing of the text bounds.
 *   width      - Bounds width.
 *   height     - Bounds height.
 *   x_advance  - Horizontal advance.
 *   y_advance  - Vertical advance.
 */
typedef struct {
    double x_bearing, y_bearing;
    double width, height;
    double x_advance, y_advance;
} gdTextExtents;

/**
 * Constants: gdTextShaping
 *
 * Text shaping mode.
 *
 *  GD_TEXT_SHAPING_NONE - Use basic glyph loading without complex shaping.
 *  GD_TEXT_SHAPING_RAQM - Use libraqm shaping when available.
 *
 * See also:
 *  - <gdTextOptions>
 */
typedef enum {
    GD_TEXT_SHAPING_NONE = 0,
    GD_TEXT_SHAPING_RAQM = 1
} gdTextShaping;

/**
 * struct: gdTextOptions
 *
 * Options for text path, drawing, and measurement functions.
 *
 * Initialize this structure with <gdTextOptionsInit> before use.
 *
 * Members:
 *   shaping         - Text shaping mode.
 *   line_spacing    - Line spacing multiplier.
 *   reserved_flags  - Reserved for future use. Initialize to zero.
 *   reserved_double - Reserved for future use. Initialize to zero.
 */
typedef struct {
    gdTextShaping shaping;
    double line_spacing;
    unsigned int reserved_flags;
    double reserved_double;
} gdTextOptions;

/**
 * Function: gdTextOptionsInit
 *
 * Initialize text options to defaults.
 *
 * Defaults are basic shaping, line spacing of 1.0, and zeroed reserved fields.
 *
 * Parameters:
 *   options - The options structure to initialize.
 */
BGD_DECLARE(void) gdTextOptionsInit(gdTextOptions *options);

/**
 * Function: gdContextCreateForImage
 *
 * Create a 2D drawing context for a truecolor image.
 *
 * The context keeps an internal premultiplied ARGB surface. Existing image
 * pixels are loaded into the context when it is created. Use
 * <gdContextFlushImage> or <gdContextDestroy> to write drawing results back to
 * the image.
 *
 * Parameters:
 *   image - A truecolor image.
 *
 * Returns:
 *   A new drawing context, or NULL if image is NULL, not truecolor, or
 *   allocation fails.
 *
 * See also:
 *  - <gdContextFlushImage>
 *  - <gdContextDestroy>
 *  - <gdContextDestroyNoFlush>
 */
BGD_DECLARE(gdContextPtr) gdContextCreateForImage(gdImagePtr image);

/**
 * Function: gdContextFlushImage
 *
 * Copy the context's drawing surface back into its image.
 *
 * Parameters:
 *   context - The drawing context.
 *
 * See also:
 *  - <gdContextCreateForImage>
 *  - <gdContextReloadImage>
 */
BGD_DECLARE(void) gdContextFlushImage(gdContextPtr context);

/**
 * Function: gdContextReloadImage
 *
 * Reload the context drawing surface from its backing image.
 *
 * This discards unflushed drawing changes in the context.
 *
 * Parameters:
 *   context - The drawing context.
 *
 * Returns:
 *   Non-zero on success, zero on failure.
 *
 * See also:
 *  - <gdContextFlushImage>
 */
BGD_DECLARE(int) gdContextReloadImage(gdContextPtr context);

/**
 * Function: gdContextGetImage
 *
 * Get the image associated with a context.
 *
 * Parameters:
 *   context - The drawing context.
 *
 * Returns:
 *   The backing image, or NULL if context is NULL or has no backing image.
 */
BGD_DECLARE(gdImagePtr) gdContextGetImage(gdContextPtr context);

/**
 * Function: gdContextDestroy
 *
 * Destroy a context and flush drawing changes to the backing image.
 *
 * Passing NULL has no effect.
 *
 * Parameters:
 *   context - The context to destroy.
 *
 * See also:
 *  - <gdContextDestroyNoFlush>
 */
BGD_DECLARE(void) gdContextDestroy(gdContextPtr context);

/**
 * Function: gdContextDestroyNoFlush
 *
 * Destroy a context without flushing drawing changes to the backing image.
 *
 * Passing NULL has no effect.
 *
 * Parameters:
 *   context - The context to destroy.
 *
 * See also:
 *  - <gdContextDestroy>
 */
BGD_DECLARE(void) gdContextDestroyNoFlush(gdContextPtr context);

/**
 * Function: gdContextSave
 *
 * Save the current graphics state.
 *
 * The saved state includes source, operator, opacity, transform, clip, stroke
 * settings, fill rule, font settings, and pattern filter.
 *
 * Parameters:
 *   context - The drawing context.
 *
 * Returns:
 *   Non-zero on success, zero on failure.
 *
 * See also:
 *  - <gdContextRestore>
 */
BGD_DECLARE(int) gdContextSave(gdContextPtr context);

/**
 * Function: gdContextRestore
 *
 * Restore the most recently saved graphics state.
 *
 * Parameters:
 *   context - The drawing context.
 *
 * Returns:
 *   Non-zero on success, zero if there is no saved state or context is NULL.
 *
 * See also:
 *  - <gdContextSave>
 */
BGD_DECLARE(int) gdContextRestore(gdContextPtr context);

/**
 * Function: gdContextClip
 *
 * Intersect the current clip with the current path and clear the path.
 *
 * Returns:
 *   Non-zero on success, zero on allocation failure.
 *
 * See also:
 *  - <gdContextClipPreserve>
 */
BGD_DECLARE(int) gdContextClip(gdContextPtr context);

/**
 * Function: gdContextClipPreserve
 *
 * Intersect the current clip with the current path without clearing the path.
 *
 * Returns:
 *   Non-zero on success, zero on allocation failure.
 *
 * See also:
 *  - <gdContextClip>
 */
BGD_DECLARE(int) gdContextClipPreserve(gdContextPtr context);

/**
 * Function: gdContextNewPath
 *
 * Clear the current path.
 */
BGD_DECLARE(void) gdContextNewPath(gdContextPtr context);

/**
 * Function: gdContextAppendPath
 *
 * Append a path to the context's current path.
 *
 * Parameters:
 *   context - The drawing context.
 *   path    - The path to append.
 */
BGD_DECLARE(void) gdContextAppendPath(gdContextPtr context, gdPathPtr path);

/**
 * Function: gdContextSetSourceRgba
 *
 * Set the current source paint to a solid RGBA color.
 *
 * Color components use the range 0.0 to 1.0.
 *
 * Parameters:
 *   context - The drawing context.
 *   r       - Red component.
 *   g       - Green component.
 *   b       - Blue component.
 *   a       - Alpha component.
 */
BGD_DECLARE(void)
gdContextSetSourceRgba(gdContextPtr context, double r, double g, double b, double a);

/**
 * Function: gdContextSetSourceRgb
 *
 * Set the current source paint to an opaque RGB color.
 *
 * Color components use the range 0.0 to 1.0.
 *
 * Parameters:
 *   context - The drawing context.
 *   r       - Red component.
 *   g       - Green component.
 *   b       - Blue component.
 */
BGD_DECLARE(void) gdContextSetSourceRgb(gdContextPtr context, double r, double g, double b);

/**
 * Function: gdContextSetSourceImage
 *
 * Set the current source paint to an image pattern.
 *
 * The image is snapshotted when this function is called, so later changes to
 * the source image do not affect the pattern. The source image is positioned by
 * translating the pattern by (x, y). The pattern inherits the context's current
 * pattern filter; use <gdContextSetPatternFilter> before this call to choose
 * FAST, GOOD, or BEST sampling for transformed image fills.
 *
 * Parameters:
 *   context - The drawing context.
 *   image   - The image to use as the source pattern.
 *   x       - The pattern x offset in user space.
 *   y       - The pattern y offset in user space.
 *
 * See also:
 *   - <gdContextSetPatternFilter>
 *   - <gdPathPatternCreateForImage>
 *   - <gdPathPatternSetFilter>
 */
BGD_DECLARE(void)
gdContextSetSourceImage(gdContextPtr context, gdImagePtr image, double x, double y);

/**
 * Function: gdContextSetSource
 *
 * Set the current source paint.
 *
 * The context keeps its own reference to the paint. The caller may destroy its
 * reference after this call.
 *
 * Parameters:
 *   context - The drawing context.
 *   source  - The paint to use for subsequent drawing.
 */
BGD_DECLARE(void) gdContextSetSource(gdContextPtr context, gdPaintPtr source);

/**
 * Function: gdContextSetOperator
 *
 * Set the compositing operator used for subsequent drawing.
 *
 * Parameters:
 *   context - The drawing context.
 *   op      - The compositing operator.
 *
 * See also:
 *   - <gdCompositeOperator>
 */
BGD_DECLARE(void) gdContextSetOperator(gdContextPtr context, gdCompositeOperator op);

/**
 * Function: gdContextSetOpacity
 *
 * Set a global opacity multiplier for subsequent drawing.
 *
 * Parameters:
 *   context - The drawing context.
 *   opacity - Opacity value. Values are clamped to the range 0.0 to 1.0.
 */
BGD_DECLARE(void) gdContextSetOpacity(gdContextPtr context, double opacity);

/**
 * Function: gdContextSetPatternFilter
 *
 * Set the default image-pattern filter for patterns created by context source
 * helpers such as <gdContextSetSourceImage>.
 *
 * This value is part of the graphics state and is saved and restored by
 * <gdContextSave> and <gdContextRestore>. It does not modify patterns that have
 * already been created; use <gdPathPatternSetFilter> for explicit pattern
 * objects.
 *
 * Parameters:
 *   context - The drawing context.
 *   filter  - The requested pattern sampling quality.
 *
 * See also:
 *   - <gdPatternFilter>
 *   - <gdContextGetPatternFilter>
 *   - <gdPathPatternSetFilter>
 */
BGD_DECLARE(void) gdContextSetPatternFilter(gdContextPtr context, gdPatternFilter filter);

/**
 * Function: gdContextGetPatternFilter
 *
 * Get the current default image-pattern filter from a context.
 *
 * Parameters:
 *   context - The drawing context.
 *
 * Returns:
 *   The current pattern filter. If context is NULL,
 *   <GD_PATTERN_FILTER_GOOD> is returned.
 *
 * See also:
 *   - <gdContextSetPatternFilter>
 */
BGD_DECLARE(gdPatternFilter) gdContextGetPatternFilter(gdContextPtr context);

/**
 * Function: gdContextSetFontFace
 *
 * Set the current font face for text operations.
 *
 * The context keeps its own reference to the font face. Passing NULL clears the
 * current font face.
 *
 * Parameters:
 *   context - The drawing context.
 *   face    - The font face to use, or NULL.
 *
 * See also:
 *   - <gdFontFaceCreateFromFile>
 *   - <gdContextShowText>
 */
BGD_DECLARE(void) gdContextSetFontFace(gdContextPtr context, gdFontFacePtr face);

/**
 * Function: gdContextSetFontSize
 *
 * Set the current font size for text operations.
 *
 * Parameters:
 *   context - The drawing context.
 *   size    - Font size in user-space units. Non-positive or non-finite values
 *             are ignored.
 */
BGD_DECLARE(void) gdContextSetFontSize(gdContextPtr context, double size);

/**
 * Function: gdContextTextPath
 *
 * Convert UTF-8 text to path contours and append them to the current path.
 *
 * Text is positioned with its origin at (x, y). The current font face and font
 * size are taken from the context.
 *
 * Parameters:
 *   context - The drawing context.
 *   utf8    - UTF-8 text.
 *   x       - Text origin x coordinate.
 *   y       - Text origin y coordinate.
 *   options - Optional text options, or NULL for defaults.
 *   err     - Optional error output.
 *
 * Returns:
 *   <GD_TEXT_OK> on success, or another <gdTextStatus> value on failure.
 *
 * See also:
 *   - <gdContextShowText>
 *   - <gdContextTextExtents>
 */
BGD_DECLARE(gdTextStatus)
gdContextTextPath(gdContextPtr context, const char *utf8, double x, double y,
                  const gdTextOptions *options, gdTextError *err);

/**
 * Function: gdContextShowText
 *
 * Draw UTF-8 text using the current source paint.
 *
 * This appends the text path, fills it, and clears the current path as part of
 * <gdContextFill>.
 *
 * Parameters:
 *   context - The drawing context.
 *   utf8    - UTF-8 text.
 *   x       - Text origin x coordinate.
 *   y       - Text origin y coordinate.
 *   options - Optional text options, or NULL for defaults.
 *   err     - Optional error output.
 *
 * Returns:
 *   <GD_TEXT_OK> on success, or another <gdTextStatus> value on failure.
 */
BGD_DECLARE(gdTextStatus)
gdContextShowText(gdContextPtr context, const char *utf8, double x, double y,
                  const gdTextOptions *options, gdTextError *err);

/**
 * Function: gdContextTextExtents
 *
 * Measure UTF-8 text using the current font face and font size.
 *
 * Parameters:
 *   context - The drawing context.
 *   utf8    - UTF-8 text.
 *   options - Optional text options, or NULL for defaults.
 *   extents - Where to store text metrics.
 *   err     - Optional error output.
 *
 * Returns:
 *   <GD_TEXT_OK> on success, or another <gdTextStatus> value on failure.
 */
BGD_DECLARE(gdTextStatus)
gdContextTextExtents(gdContextPtr context, const char *utf8, const gdTextOptions *options,
                     gdTextExtents *extents, gdTextError *err);

/**
 * Function: gdContextMoveTo
 *
 * Move the current point in the context path.
 */
BGD_DECLARE(void) gdContextMoveTo(gdContextPtr context, double x, double y);

/**
 * Function: gdContextRelMoveTo
 *
 * Move the current point by an offset in the context path.
 */
BGD_DECLARE(void) gdContextRelMoveTo(gdContextPtr context, double dx, double dy);

/**
 * Function: gdContextLineTo
 *
 * Add a line to the context path.
 */
BGD_DECLARE(void) gdContextLineTo(gdContextPtr context, double x, double y);

/**
 * Function: gdContextRelLineTo
 *
 * Add a relative line to the context path.
 */
BGD_DECLARE(void) gdContextRelLineTo(gdContextPtr context, double dx, double dy);

/**
 * Function: gdContextCurveTo
 *
 * Add a cubic Bezier curve to the context path.
 */
BGD_DECLARE(void)
gdContextCurveTo(gdContextPtr context, double x1, double y1, double x2, double y2, double x3,
                 double y3);

/**
 * Function: gdContextRelCurveTo
 *
 * Add a relative cubic Bezier curve to the context path.
 */
BGD_DECLARE(void)
gdContextRelCurveTo(gdContextPtr context, double dx1, double dy1, double dx2, double dy2,
                    double dx3, double dy3);

/**
 * Function: gdContextQuadTo
 *
 * Add a quadratic Bezier curve to the context path.
 */
BGD_DECLARE(void) gdContextQuadTo(gdContextPtr context, double x1, double y1, double x2, double y2);

/**
 * Function: gdContextRelQuadTo
 *
 * Add a relative quadratic Bezier curve to the context path.
 */
BGD_DECLARE(void)
gdContextRelQuadTo(gdContextPtr context, double dx1, double dy1, double dx2, double dy2);

/**
 * Function: gdContextArc
 *
 * Add a positive-direction circular arc to the context path.
 */
BGD_DECLARE(void)
gdContextArc(gdContextPtr context, double cx, double cy, double radius, double a0, double a1);

/**
 * Function: gdContextNegativeArc
 *
 * Add a negative-direction circular arc to the context path.
 */
BGD_DECLARE(void)
gdContextNegativeArc(gdContextPtr context, double cx, double cy, double radius, double a0,
                     double a1);

/**
 * Function: gdContextRectangle
 *
 * Add a rectangle to the context path.
 */
BGD_DECLARE(void)
gdContextRectangle(gdContextPtr context, double x, double y, double width, double height);

/**
 * Function: gdContextClosePath
 *
 * Close the current contour in the context path.
 */
BGD_DECLARE(void) gdContextClosePath(gdContextPtr context);

/**
 * Function: gdContextScale
 *
 * Apply scaling to the current transformation matrix.
 */
BGD_DECLARE(void) gdContextScale(gdContextPtr context, double x, double y);

/**
 * Function: gdContextTranslate
 *
 * Apply translation to the current transformation matrix.
 */
BGD_DECLARE(void) gdContextTranslate(gdContextPtr context, double x, double y);

/**
 * Function: gdContextRotate
 *
 * Apply rotation to the current transformation matrix.
 *
 * Parameters:
 *   context - The drawing context.
 *   radians - Rotation angle in radians.
 */
BGD_DECLARE(void) gdContextRotate(gdContextPtr context, double radians);

/**
 * Function: gdContextTransform
 *
 * Apply an affine transformation to the current transformation matrix.
 *
 * Parameters:
 *   context - The drawing context.
 *   matrix  - The transform to apply.
 */
BGD_DECLARE(void) gdContextTransform(gdContextPtr context, const gdPathMatrixPtr matrix);

/**
 * Function: gdContextSetLineWidth
 *
 * Set the stroke line width.
 */
BGD_DECLARE(void) gdContextSetLineWidth(gdContextPtr context, double width);

/**
 * Function: gdContextSetLineCap
 *
 * Set the stroke line cap style.
 */
BGD_DECLARE(void) gdContextSetLineCap(gdContextPtr context, gdLineCap cap);

/**
 * Function: gdContextSetLineJoin
 *
 * Set the stroke line join style.
 */
BGD_DECLARE(void) gdContextSetLineJoin(gdContextPtr context, gdLineJoin join);

/**
 * Function: gdContextSetDash
 *
 * Set the stroke dash pattern.
 *
 * Parameters:
 *   context - The drawing context.
 *   offset  - Dash phase offset.
 *   data    - Dash segment lengths.
 *   size    - Number of dash segment lengths.
 */
BGD_DECLARE(void)
gdContextSetDash(gdContextPtr context, double offset, const double *data, int size);

/**
 * Function: gdContextSetFillRule
 *
 * Set the fill rule used by subsequent fill and clip operations.
 */
BGD_DECLARE(void) gdContextSetFillRule(gdContextPtr context, gdFillRule rule);

/**
 * Function: gdContextStroke
 *
 * Stroke the current path and then clear it.
 *
 * See also:
 *   - <gdContextStrokePreserve>
 */
BGD_DECLARE(void) gdContextStroke(gdContextPtr context);

/**
 * Function: gdContextStrokePreserve
 *
 * Stroke the current path without clearing it.
 */
BGD_DECLARE(void) gdContextStrokePreserve(gdContextPtr context);

/**
 * Function: gdContextFill
 *
 * Fill the current path and then clear it.
 *
 * See also:
 *   - <gdContextFillPreserve>
 */
BGD_DECLARE(void) gdContextFill(gdContextPtr context);

/**
 * Function: gdContextFillPreserve
 *
 * Fill the current path without clearing it.
 */
BGD_DECLARE(void) gdContextFillPreserve(gdContextPtr context);

/**
 * Function: gdContextPaint
 *
 * Paint the current source over the entire current clip.
 */
BGD_DECLARE(void) gdContextPaint(gdContextPtr context);

/**
 * Function: gdFontFaceCreateFromFile
 *
 * Load a font face from a file.
 *
 * Parameters:
 *   path       - Font file path.
 *   face_index - Face index within the font file.
 *   err        - Optional error output.
 *
 * Returns:
 *   A new font face, or NULL on failure.
 */
BGD_DECLARE(gdFontFacePtr)
gdFontFaceCreateFromFile(const char *path, int face_index, gdTextError *err);

/**
 * Function: gdFontFaceCreateFromData
 *
 * Load a font face from memory.
 *
 * The caller must keep the data buffer alive for the lifetime of the font face.
 *
 * Parameters:
 *   data       - Font data.
 *   size       - Font data size in bytes.
 *   face_index - Face index within the font data.
 *   err        - Optional error output.
 *
 * Returns:
 *   A new font face, or NULL on failure.
 */
BGD_DECLARE(gdFontFacePtr)
gdFontFaceCreateFromData(const unsigned char *data, size_t size, int face_index, gdTextError *err);

/**
 * Function: gdFontFaceAddRef
 *
 * Add a reference to a font face.
 *
 * Returns:
 *   The same font face, or NULL if face is NULL.
 */
BGD_DECLARE(gdFontFacePtr) gdFontFaceAddRef(gdFontFacePtr face);

/**
 * Function: gdFontFaceDestroy
 *
 * Release a font face reference. Passing NULL has no effect.
 */
BGD_DECLARE(void) gdFontFaceDestroy(gdFontFacePtr face);

/**
 * Function: gdPaintCreateFromPattern
 *
 * Create a paint object from an image pattern.
 *
 * The paint holds a reference to the pattern. The caller may destroy its own
 * pattern reference after creating the paint.
 *
 * Parameters:
 *   pattern - The pattern to use as a paint source.
 *
 * Returns:
 *   A new paint object, or NULL on allocation failure.
 *
 * See also:
 *   - <gdPathPatternCreateForImage>
 *   - <gdPathPatternSetFilter>
 *   - <gdContextSetSource>
 */
BGD_DECLARE(gdPaintPtr) gdPaintCreateFromPattern(gdPathPatternPtr pattern);

/**
 * Function: gdPaintCreateLinear
 *
 * Create a linear gradient paint.
 *
 * The gradient parameter runs from (x0, y0) to (x1, y1). Add color stops with
 * <gdPaintAddColorStopRgb> or <gdPaintAddColorStopRgba>.
 *
 * Parameters:
 *   x0 - Start point x coordinate.
 *   y0 - Start point y coordinate.
 *   x1 - End point x coordinate.
 *   y1 - End point y coordinate.
 *
 * Returns:
 *   A new paint object, or NULL on invalid input or allocation failure.
 */
BGD_DECLARE(gdPaintPtr) gdPaintCreateLinear(double x0, double y0, double x1, double y1);

/**
 * Function: gdPaintCreateRadial
 *
 * Create a radial gradient paint between two circles.
 *
 * Add color stops with <gdPaintAddColorStopRgb> or
 * <gdPaintAddColorStopRgba>.
 *
 * Parameters:
 *   x0 - First circle center x coordinate.
 *   y0 - First circle center y coordinate.
 *   r0 - First circle radius. Must be non-negative.
 *   x1 - Second circle center x coordinate.
 *   y1 - Second circle center y coordinate.
 *   r1 - Second circle radius. Must be non-negative.
 *
 * Returns:
 *   A new paint object, or NULL on invalid input or allocation failure.
 */
BGD_DECLARE(gdPaintPtr)
gdPaintCreateRadial(double x0, double y0, double r0, double x1, double y1, double r1);

/**
 * Function: gdPaintAddColorStopRgb
 *
 * Add an opaque color stop to a gradient paint.
 *
 * Parameters:
 *   paint  - A linear or radial gradient paint.
 *   offset - Stop offset in the range 0.0 to 1.0.
 *   r      - Red component in the range 0.0 to 1.0.
 *   g      - Green component in the range 0.0 to 1.0.
 *   b      - Blue component in the range 0.0 to 1.0.
 *
 * Returns:
 *   Non-zero on success, zero on invalid input or allocation failure.
 *
 * See also:
 *   - <gdPaintAddColorStopRgba>
 */
BGD_DECLARE(int)
gdPaintAddColorStopRgb(gdPaintPtr paint, double offset, double r, double g, double b);

/**
 * Function: gdPaintAddColorStopRgba
 *
 * Add a color stop with opacity to a gradient paint.
 *
 * Multiple stops may have the same offset. Stops are kept in insertion order
 * for equal offsets.
 *
 * Parameters:
 *   paint  - A linear or radial gradient paint.
 *   offset - Stop offset in the range 0.0 to 1.0.
 *   r      - Red component in the range 0.0 to 1.0.
 *   g      - Green component in the range 0.0 to 1.0.
 *   b      - Blue component in the range 0.0 to 1.0.
 *   a      - Alpha component in the range 0.0 to 1.0.
 *
 * Returns:
 *   Non-zero on success, zero on invalid input or allocation failure.
 */
BGD_DECLARE(int)
gdPaintAddColorStopRgba(gdPaintPtr paint, double offset, double r, double g, double b, double a);

/**
 * Function: gdPaintSetExtend
 *
 * Set the extend mode for a gradient or pattern paint.
 *
 * Parameters:
 *   paint  - The paint to update.
 *   extend - The extend mode.
 *
 * Returns:
 *   Non-zero on success, zero if the paint type or extend mode is invalid.
 *
 * See also:
 *   - <gdExtendMode>
 */
BGD_DECLARE(int) gdPaintSetExtend(gdPaintPtr paint, gdExtendMode extend);

/**
 * Function: gdPaintSetMatrix
 *
 * Set the paint's pattern or gradient transform.
 *
 * The matrix must be finite and invertible.
 *
 * Parameters:
 *   paint  - The paint to update.
 *   matrix - The paint matrix.
 *
 * Returns:
 *   Non-zero on success, zero on invalid input.
 */
BGD_DECLARE(int) gdPaintSetMatrix(gdPaintPtr paint, const gdPathMatrixPtr matrix);

/**
 * Function: gdPaintDestroy
 *
 * Release a paint object. Passing NULL has no effect.
 */
BGD_DECLARE(void) gdPaintDestroy(gdPaintPtr paint);

/**
 * Function: gdPathPatternCreateForImage
 *
 * Create an image pattern from a gd image.
 *
 * The image contents are copied into an internal surface when the pattern is
 * created. The new pattern uses <GD_EXTEND_NONE>, identity matrix, full opacity,
 * and <GD_PATTERN_FILTER_GOOD> by default.
 *
 * Parameters:
 *   image - The image to snapshot into a pattern.
 *
 * Returns:
 *   A new pattern, or NULL on failure.
 *
 * See also:
 *   - <gdPathPatternDestroy>
 *   - <gdPathPatternSetFilter>
 *   - <gdPaintCreateFromPattern>
 */
BGD_DECLARE(gdPathPatternPtr) gdPathPatternCreateForImage(gdImagePtr image);

/**
 * Function: gdPathPatternDestroy
 *
 * Release a pattern object. Passing NULL has no effect.
 */
BGD_DECLARE(void) gdPathPatternDestroy(gdPathPatternPtr pattern);

/**
 * Function: gdPathPatternSetExtend
 *
 * Set how an image pattern is sampled outside its image bounds.
 *
 * Parameters:
 *   pattern - The pattern to update.
 *   extend  - The extend mode.
 *
 * See also:
 *   - <gdExtendMode>
 */
BGD_DECLARE(void) gdPathPatternSetExtend(gdPathPatternPtr pattern, gdExtendMode extend);

/**
 * Function: gdPathPatternSetMatrix
 *
 * Set the pattern-to-user-space transform for an image pattern.
 *
 * The matrix controls how the pattern image is positioned, scaled, rotated, or
 * sheared before it is used as a fill source.
 *
 * Parameters:
 *   pattern - The pattern to update.
 *   matrix  - The pattern matrix.
 *
 * See also:
 *   - <gdPathMatrix>
 *   - <gdContextSetSourceImage>
 */
BGD_DECLARE(void) gdPathPatternSetMatrix(gdPathPatternPtr pattern, gdPathMatrixPtr matrix);

/**
 * Function: gdPathPatternSetOpacity
 *
 * Set the opacity multiplier for an image pattern.
 *
 * Parameters:
 *   pattern - The pattern to update.
 *   opacity - The opacity value. Values are clamped to the range 0.0 to 1.0.
 */
BGD_DECLARE(void) gdPathPatternSetOpacity(gdPathPatternPtr pattern, double opacity);

/**
 * Function: gdPathPatternSetFilter
 *
 * Set the sampling quality for an image pattern.
 *
 * The filter is used when the pattern image is transformed during painting.
 * FAST requests nearest-neighbour sampling, GOOD requests balanced quality, and
 * BEST requests the highest practical software quality. The exact kernel used
 * for GOOD or BEST is intentionally an implementation detail.
 *
 * Parameters:
 *   pattern - The pattern to update.
 *   filter  - The requested pattern sampling quality.
 *
 * See also:
 *   - <gdPatternFilter>
 *   - <gdPathPatternGetFilter>
 *   - <gdContextSetPatternFilter>
 */
BGD_DECLARE(void) gdPathPatternSetFilter(gdPathPatternPtr pattern, gdPatternFilter filter);

/**
 * Function: gdPathPatternGetFilter
 *
 * Get the sampling quality for an image pattern.
 *
 * Parameters:
 *   pattern - The pattern to query.
 *
 * Returns:
 *   The pattern filter. If pattern is NULL, <GD_PATTERN_FILTER_GOOD> is
 *   returned.
 *
 * See also:
 *   - <gdPathPatternSetFilter>
 */
BGD_DECLARE(gdPatternFilter) gdPathPatternGetFilter(gdPathPatternPtr pattern);

/**
 * Function: gdPathMatrixInit
 *
 * Initialize an affine transformation matrix from its six coefficients.
 *
 * A point is mapped to (x * m00 + y * m01 + m02,
 * x * m10 + y * m11 + m12).
 *
 * Parameters:
 *   matrix - The matrix to initialize.
 *   m00    - The horizontal x coefficient.
 *   m10    - The vertical x coefficient.
 *   m01    - The horizontal y coefficient.
 *   m11    - The vertical y coefficient.
 *   m02    - The horizontal translation.
 *   m12    - The vertical translation.
 *
 * See also:
 *   - <gdPathMatrixInitIdentity>
 *   - <gdPathMatrixMap>
 */
BGD_DECLARE(void)
gdPathMatrixInit(gdPathMatrixPtr matrix, double m00, double m10, double m01, double m11, double m02,
                 double m12);
/**
 * Function: gdPathMatrixInitIdentity
 *
 * Initialize a matrix to the identity transformation.
 *
 * Parameters:
 *   matrix - The matrix to initialize.
 */
BGD_DECLARE(void) gdPathMatrixInitIdentity(gdPathMatrixPtr matrix);

/**
 * Function: gdPathMatrixInitTranslate
 *
 * Initialize a translation matrix.
 *
 * Parameters:
 *   matrix - The matrix to initialize.
 *   x      - The horizontal translation.
 *   y      - The vertical translation.
 */
BGD_DECLARE(void) gdPathMatrixInitTranslate(gdPathMatrixPtr matrix, double x, double y);

/**
 * Function: gdPathMatrixInitScale
 *
 * Initialize a scaling matrix.
 *
 * Parameters:
 *   matrix - The matrix to initialize.
 *   x      - The horizontal scale factor.
 *   y      - The vertical scale factor.
 */
BGD_DECLARE(void) gdPathMatrixInitScale(gdPathMatrixPtr matrix, double x, double y);

/**
 * Function: gdPathMatrixInitShear
 *
 * Initialize a shear matrix. The shear factors are the tangents of the
 * supplied angles.
 *
 * Parameters:
 *   matrix - The matrix to initialize.
 *   x      - The horizontal shear angle, in radians.
 *   y      - The vertical shear angle, in radians.
 */
BGD_DECLARE(void) gdPathMatrixInitShear(gdPathMatrixPtr matrix, double x, double y);

/**
 * Function: gdPathMatrixInitRotate
 *
 * Initialize a rotation matrix about the origin.
 *
 * Parameters:
 *   matrix  - The matrix to initialize.
 *   radians - The rotation angle, in radians.
 */
BGD_DECLARE(void) gdPathMatrixInitRotate(gdPathMatrixPtr matrix, double radians);

/**
 * Function: gdPathMatrixInitRotateTranslate
 *
 * Initialize a rotation matrix about a specified point. The point (x, y)
 * remains unchanged by the resulting transformation.
 *
 * Parameters:
 *   matrix  - The matrix to initialize.
 *   radians - The rotation angle, in radians.
 *   x       - The horizontal coordinate of the rotation center.
 *   y       - The vertical coordinate of the rotation center.
 */
BGD_DECLARE(void)
gdPathMatrixInitRotateTranslate(gdPathMatrixPtr matrix, double radians, double x, double y);

/**
 * Function: gdPathMatrixTranslate
 *
 * Apply a translation before the transformation already in a matrix.
 *
 * Parameters:
 *   matrix - The matrix to modify.
 *   x      - The horizontal translation.
 *   y      - The vertical translation.
 */
BGD_DECLARE(void) gdPathMatrixTranslate(gdPathMatrixPtr matrix, double x, double y);

/**
 * Function: gdPathMatrixScale
 *
 * Apply scaling before the transformation already in a matrix.
 *
 * Parameters:
 *   matrix - The matrix to modify.
 *   x      - The horizontal scale factor.
 *   y      - The vertical scale factor.
 */
BGD_DECLARE(void) gdPathMatrixScale(gdPathMatrixPtr matrix, double x, double y);

/**
 * Function: gdPathMatrixShear
 *
 * Apply a shear before the transformation already in a matrix.
 *
 * Parameters:
 *   matrix - The matrix to modify.
 *   x      - The horizontal shear angle, in radians.
 *   y      - The vertical shear angle, in radians.
 */
BGD_DECLARE(void) gdPathMatrixShear(gdPathMatrixPtr matrix, double x, double y);

/**
 * Function: gdPathMatrixRotate
 *
 * Apply a rotation about the origin before the transformation already in a
 * matrix.
 *
 * Parameters:
 *   matrix  - The matrix to modify.
 *   radians - The rotation angle, in radians.
 */
BGD_DECLARE(void) gdPathMatrixRotate(gdPathMatrixPtr matrix, double radians);

/**
 * Function: gdPathMatrixRotateTranslate
 *
 * Apply a rotation about a specified point before the transformation already
 * in a matrix.
 *
 * Parameters:
 *   matrix  - The matrix to modify.
 *   radians - The rotation angle, in radians.
 *   x       - The horizontal coordinate of the rotation center.
 *   y       - The vertical coordinate of the rotation center.
 */
BGD_DECLARE(void)
gdPathMatrixRotateTranslate(gdPathMatrixPtr matrix, double radians, double x, double y);

/**
 * Function: gdPathMatrixMultiply
 *
 * Compose two affine transformations. The result maps through a first and
 * then through b. matrix may alias a or b.
 *
 * Parameters:
 *   matrix - The destination matrix.
 *   a      - The transformation applied first.
 *   b      - The transformation applied second.
 */
BGD_DECLARE(void)
gdPathMatrixMultiply(gdPathMatrixPtr matrix, const gdPathMatrixPtr a, const gdPathMatrixPtr b);

/**
 * Function: gdPathMatrixInvert
 *
 * Invert an affine transformation in place. A singular matrix is left
 * unchanged.
 *
 * Parameters:
 *   matrix - The matrix to invert.
 *
 * Returns:
 *   Non-zero on success, or zero if the matrix is singular.
 */
BGD_DECLARE(int) gdPathMatrixInvert(gdPathMatrixPtr matrix);

/**
 * Function: gdPathMatrixMap
 *
 * Transform a pair of coordinates.
 *
 * Parameters:
 *   matrix   - The transformation matrix.
 *   x        - The source horizontal coordinate.
 *   y        - The source vertical coordinate.
 *   result_x - Where to store the transformed horizontal coordinate.
 *   result_y - Where to store the transformed vertical coordinate.
 */
BGD_DECLARE(void)
gdPathMatrixMap(const gdPathMatrixPtr matrix, double x, double y, double *result_x,
                double *result_y);

/**
 * Function: gdPathMatrixMapPoint
 *
 * Transform a point. src and dst may point to the same object.
 *
 * Parameters:
 *   matrix - The transformation matrix.
 *   src    - The source point.
 *   dst    - Where to store the transformed point.
 */
BGD_DECLARE(void)
gdPathMatrixMapPoint(const gdPathMatrixPtr matrix, const gdPointFPtr src, gdPointFPtr dst);

/**
 * Function: gdPathMatrixMapRect
 *
 * Transform all four corners of a rectangle and calculate their axis-aligned
 * bounding box. src and dst may point to the same object.
 *
 * Parameters:
 *   matrix - The transformation matrix.
 *   src    - The source rectangle.
 *   dst    - Where to store the transformed bounding rectangle.
 */
BGD_DECLARE(void)
gdPathMatrixMapRect(const gdPathMatrixPtr matrix, const gdRectFPtr src, gdRectFPtr dst);

/**
 * Function: gdPathCreate
 *
 * Create an empty path.
 *
 * Returns:
 *   A new path, or NULL if allocation fails. Destroy it with <gdPathDestroy>.
 */
BGD_DECLARE(gdPathPtr) gdPathCreate(void);

/**
 * Function: gdPathDuplicate
 *
 * Create an independent copy of a path.
 *
 * Parameters:
 *   path - The path to copy. Must not be NULL.
 *
 * Returns:
 *   A new path, or NULL if path is NULL or allocation fails. Destroy the copy
 *   with <gdPathDestroy>.
 */
BGD_DECLARE(gdPathPtr) gdPathDuplicate(const gdPathPtr path);

/**
 * Function: gdPathDestroy
 *
 * Release a path. Passing NULL has no effect.
 *
 * Parameters:
 *   path - The path to release.
 */
BGD_DECLARE(void) gdPathDestroy(gdPathPtr path);

/**
 * Function: gdPathAppendPath
 *
 * Append all contours from one path to another.
 *
 * Parameters:
 *   path   - The destination path.
 *   source - The path to append.
 */
BGD_DECLARE(void) gdPathAppendPath(gdPathPtr path, const gdPathPtr source);

/**
 * Function: gdPathTransform
 *
 * Transform every point in a path in place.
 *
 * Parameters:
 *   path   - The path to transform.
 *   matrix - The transformation matrix.
 */
BGD_DECLARE(void) gdPathTransform(gdPathPtr path, const gdPathMatrixPtr matrix);

/**
 * Function: gdPathMoveTo
 *
 * Start a new contour at an absolute position.
 *
 * Parameters:
 *   path - The path to modify.
 *   x    - The horizontal coordinate.
 *   y    - The vertical coordinate.
 */
BGD_DECLARE(void) gdPathMoveTo(gdPathPtr path, double x, double y);

/**
 * Function: gdPathRelMoveTo
 *
 * Start a new contour at an offset from the current point. For an empty path,
 * the offset is relative to (0, 0).
 *
 * Parameters:
 *   path - The path to modify.
 *   dx   - The horizontal offset.
 *   dy   - The vertical offset.
 */
BGD_DECLARE(void) gdPathRelMoveTo(gdPathPtr path, double dx, double dy);

/**
 * Function: gdPathLineTo
 *
 * Add a straight line to an absolute position.
 *
 * Parameters:
 *   path - The path to modify.
 *   x    - The endpoint's horizontal coordinate.
 *   y    - The endpoint's vertical coordinate.
 */
BGD_DECLARE(void) gdPathLineTo(gdPathPtr path, double x, double y);

/**
 * Function: gdPathRelLineTo
 *
 * Add a straight line using an offset from the current point.
 *
 * Parameters:
 *   path - The path to modify.
 *   dx   - The horizontal offset.
 *   dy   - The vertical offset.
 */
BGD_DECLARE(void) gdPathRelLineTo(gdPathPtr path, double dx, double dy);

/**
 * Function: gdPathQuadTo
 *
 * Add a quadratic Bezier curve.
 *
 * Parameters:
 *   path - The path to modify.
 *   x1   - The control point's horizontal coordinate.
 *   y1   - The control point's vertical coordinate.
 *   x2   - The endpoint's horizontal coordinate.
 *   y2   - The endpoint's vertical coordinate.
 */
BGD_DECLARE(void) gdPathQuadTo(gdPathPtr path, double x1, double y1, double x2, double y2);

/**
 * Function: gdPathRelQuadTo
 *
 * Add a quadratic Bezier curve using offsets from the current point.
 *
 * Parameters:
 *   path - The path to modify.
 *   dx1  - The control point's horizontal offset.
 *   dy1  - The control point's vertical offset.
 *   dx2  - The endpoint's horizontal offset.
 *   dy2  - The endpoint's vertical offset.
 */
BGD_DECLARE(void) gdPathRelQuadTo(gdPathPtr path, double dx1, double dy1, double dx2, double dy2);

/**
 * Function: gdPathCurveTo
 *
 * Add a cubic Bezier curve.
 *
 * Parameters:
 *   path - The path to modify.
 *   x1   - The first control point's horizontal coordinate.
 *   y1   - The first control point's vertical coordinate.
 *   x2   - The second control point's horizontal coordinate.
 *   y2   - The second control point's vertical coordinate.
 *   x3   - The endpoint's horizontal coordinate.
 *   y3   - The endpoint's vertical coordinate.
 */
BGD_DECLARE(void)
gdPathCurveTo(gdPathPtr path, double x1, double y1, double x2, double y2, double x3, double y3);

/**
 * Function: gdPathRelCurveTo
 *
 * Add a cubic Bezier curve using offsets from the current point.
 *
 * Parameters:
 *   path - The path to modify.
 *   dx1  - The first control point's horizontal offset.
 *   dy1  - The first control point's vertical offset.
 *   dx2  - The second control point's horizontal offset.
 *   dy2  - The second control point's vertical offset.
 *   dx3  - The endpoint's horizontal offset.
 *   dy3  - The endpoint's vertical offset.
 */
BGD_DECLARE(void)
gdPathRelCurveTo(gdPathPtr path, double dx1, double dy1, double dx2, double dy2, double dx3,
                 double dy3);

/**
 * Function: gdPathArc
 *
 * Add a circular arc in the positive-angle direction. Angles are in radians.
 * A line is added from the current point to the beginning of the arc when
 * necessary.
 *
 * Parameters:
 *   path   - The path to modify.
 *   cx     - The center's horizontal coordinate.
 *   cy     - The center's vertical coordinate.
 *   radius - The arc radius.
 *   angle1 - The starting angle in radians.
 *   angle2 - The ending angle in radians.
 */
BGD_DECLARE(void)
gdPathArc(gdPathPtr path, double cx, double cy, double radius, double angle1, double angle2);

/**
 * Function: gdPathNegativeArc
 *
 * Add a circular arc in the negative-angle direction. Angles are in radians.
 * A line is added from the current point to the beginning of the arc when
 * necessary.
 *
 * Parameters:
 *   path   - The path to modify.
 *   cx     - The center's horizontal coordinate.
 *   cy     - The center's vertical coordinate.
 *   radius - The arc radius.
 *   angle1 - The starting angle in radians.
 *   angle2 - The ending angle in radians.
 */
BGD_DECLARE(void)
gdPathNegativeArc(gdPathPtr path, double cx, double cy, double radius, double angle1,
                  double angle2);

/**
 * Function: gdPathArcTo
 *
 * Connect the current point to (x1, y1) and (x2, y2) with a circular arc
 * tangent to both line segments. Degenerate geometry or a non-positive radius
 * adds a line to (x1, y1).
 *
 * Parameters:
 *   path   - The path to modify.
 *   x1     - The corner's horizontal coordinate.
 *   y1     - The corner's vertical coordinate.
 *   x2     - The second tangent line's horizontal endpoint.
 *   y2     - The second tangent line's vertical endpoint.
 *   radius - The arc radius.
 */
BGD_DECLARE(void)
gdPathArcTo(gdPathPtr path, double x1, double y1, double x2, double y2, double radius);

/**
 * Function: gdPathRectangle
 *
 * Add a closed rectangular contour.
 *
 * Parameters:
 *   path   - The path to modify.
 *   x      - The rectangle's left coordinate.
 *   y      - The rectangle's top coordinate.
 *   width  - The rectangle width.
 *   height - The rectangle height.
 */
BGD_DECLARE(void) gdPathRectangle(gdPathPtr path, double x, double y, double width, double height);

/**
 * Function: gdPathClose
 *
 * Close the current contour with a line to its starting point. An empty path
 * or an already closed contour is unchanged.
 *
 * Parameters:
 *   path - The path to modify.
 */
BGD_DECLARE(void) gdPathClose(gdPathPtr path);

#ifdef __cplusplus
}
#endif
#endif
