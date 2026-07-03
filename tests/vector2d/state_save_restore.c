#include "gd_vector2d_private.h"
#include "gd_array.h"
#include "gd_path.h"
#include "gdtest.h"

#include <math.h>
#include <stdarg.h>

static int error_count;

static void count_error(int priority, const char *format, va_list args)
{
    (void)priority;
    (void)format;
    (void)args;
    error_count++;
}

static gdContextPtr new_context(gdImagePtr *image)
{
    *image = gdImageCreateTrueColor(32, 32);
    gdTestAssert(*image != NULL);
    return gdContextCreateForImage(*image);
}

static void add_clip_rect(gdContextPtr context, double x, double y,
                          double width, double height)
{
    gdContextRectangle(context, x, y, width, height);
    gdTestAssert(gdContextClip(context) == 1);
}

static void test_state_round_trip(void)
{
    const double dash[] = {3.0, 2.0};
    gdImagePtr image;
    gdContextPtr context = new_context(&image);
    gdStatePtr base = context->state;
    gdPaintPtr base_source;
    gdPathPtr path = context->path;

    gdContextMoveTo(context, 1, 2);
    gdContextLineTo(context, 3, 4);
    gdContextSetSourceRgba(context, 0.1, 0.2, 0.3, 0.4);
    gdContextSetOperator(context, GD_OP_MULTIPLY);
    gdContextSetOpacity(context, 0.75);
    gdContextSetLineWidth(context, 7.0);
    gdContextSetLineCap(context, gdLineCapRound);
    gdContextSetLineJoin(context, gdLineJoinBevel);
    gdContextSetDash(context, 1.5, dash, 2);
    gdContextSetFillRule(context, gdFillRuleEvenOdd);
    gdContextTranslate(context, 5, 6);
    add_clip_rect(context, 0, 0, 24, 24);
    base_source = base->source;
    gdContextMoveTo(context, 1, 2);
    gdContextLineTo(context, 3, 4);

    gdTestAssert(gdContextSave(context) == 1);
    gdTestAssert(context->state != base);
    gdTestAssert(context->state->clippath == base->clippath);
    gdTestAssert(base->clippath->ref == 2);
    gdTestAssert(context->state->stroke.dash != base->stroke.dash);

    gdContextSetSourceRgb(context, 1, 0, 0);
    gdContextSetOperator(context, GD_OP_SOURCE);
    gdContextSetOpacity(context, 0.25);
    gdContextSetLineWidth(context, 2.0);
    gdContextSetLineCap(context, gdLineCapSquare);
    gdContextSetLineJoin(context, gdLineJoinRound);
    gdContextSetDash(context, 0, NULL, 0);
    gdContextSetFillRule(context, gdFillRuleNonZero);
    gdContextScale(context, 2, 3);
    add_clip_rect(context, 4, 4, 8, 8);
    gdTestAssert(context->state->clippath != base->clippath);
    gdTestAssert(base->clippath->ref == 1);
    gdContextMoveTo(context, 7, 8);
    gdContextLineTo(context, 9, 10);

    gdTestAssert(gdContextSave(context) == 1);
    gdContextSetOpacity(context, 0.1);
    gdTestAssert(gdContextRestore(context) == 1);
    gdTestAssert(fabs(context->state->opacity - 0.25) < 1e-12);
    gdTestAssert(gdContextRestore(context) == 1);
    gdTestAssert(context->state == base);
    gdTestAssert(context->state->source == base_source);
    gdTestAssert(context->path == path);
    gdTestAssert(gdArrayNumElements(&path->elements) == 2);
    gdTestAssert(context->state->op == GD_OP_MULTIPLY);
    gdTestAssert(fabs(context->state->opacity - 0.75) < 1e-12);
    gdTestAssert(fabs(context->state->stroke.width - 7.0) < 1e-12);
    gdTestAssert(context->state->stroke.cap == gdLineCapRound);
    gdTestAssert(context->state->stroke.join == gdLineJoinBevel);
    gdTestAssert(context->state->stroke.dash->size == 2);
    gdTestAssert(context->state->winding == gdFillRuleEvenOdd);
    gdTestAssert(fabs(context->state->matrix.m02 - 5.0) < 1e-12);
    gdTestAssert(fabs(context->state->matrix.m12 - 6.0) < 1e-12);

    gdContextDestroy(context);
    gdImageDestroy(image);
}

static void test_path_and_stack_lifetime(void)
{
    gdImagePtr image;
    gdContextPtr context = new_context(&image);

    gdContextRectangle(context, 1, 1, 10, 10);
    unsigned int path_size = gdArrayNumElements(&context->path->elements);
    gdTestAssert(gdContextSave(context) == 1);
    gdTestAssert(gdContextRestore(context) == 1);
    gdTestAssert(gdArrayNumElements(&context->path->elements) == path_size);
    gdTestAssert(gdContextClipPreserve(context) == 1);
    gdTestAssert(gdArrayNumElements(&context->path->elements) == path_size);
    gdTestAssert(gdContextClip(context) == 1);
    gdTestAssert(gdArrayNumElements(&context->path->elements) == 0);

    gdTestAssert(gdContextSave(context) == 1);
    gdTestAssert(gdContextSave(context) == 1);
    gdContextDestroy(context); /* Must release the complete unbalanced stack. */
    gdImageDestroy(image);
}

static void test_invalid_calls(void)
{
    gdImagePtr image;
    gdContextPtr context = new_context(&image);
    gdStatePtr state = context->state;

    error_count = 0;
    gdSetErrorMethod(count_error);
    gdTestAssert(gdContextSave(NULL) == 0);
    gdTestAssert(gdContextRestore(NULL) == 0);
    gdTestAssert(gdContextClip(NULL) == 0);
    gdTestAssert(gdContextClipPreserve(NULL) == 0);
    gdTestAssert(gdContextRestore(context) == 0);
    gdTestAssert(context->state == state);
    gdTestAssert(error_count == 5);
    gdClearErrorMethod();

    gdContextDestroy(context);
    gdImageDestroy(image);
}

int main(void)
{
    gdSetErrorMethod(count_error);
    test_state_round_trip();
    test_path_and_stack_lifetime();
    test_invalid_calls();
    gdClearErrorMethod();
    return gdNumFailures();
}
