#include "gd_vector2d.h"
#include "gdtest.h"

#include <math.h>

#define EPSILON 1e-10
#define PI 3.14159265358979323846

static void assert_close(double actual, double expected)
{
	gdTestAssert(fabs(actual - expected) < EPSILON);
}

static void assert_matrix(const gdPathMatrix *matrix,
		double m00, double m10, double m01, double m11,
		double m02, double m12)
{
	assert_close(matrix->m00, m00);
	assert_close(matrix->m10, m10);
	assert_close(matrix->m01, m01);
	assert_close(matrix->m11, m11);
	assert_close(matrix->m02, m02);
	assert_close(matrix->m12, m12);
}

static void test_initializers(void)
{
	gdPathMatrix matrix;
	double x, y;

	gdPathMatrixInit(&matrix, 1, 2, 3, 4, 5, 6);
	assert_matrix(&matrix, 1, 2, 3, 4, 5, 6);
	gdPathMatrixInitIdentity(&matrix);
	assert_matrix(&matrix, 1, 0, 0, 1, 0, 0);
	gdPathMatrixInitTranslate(&matrix, 2, 3);
	assert_matrix(&matrix, 1, 0, 0, 1, 2, 3);
	gdPathMatrixInitScale(&matrix, 2, 3);
	assert_matrix(&matrix, 2, 0, 0, 3, 0, 0);
	gdPathMatrixInitShear(&matrix, atan(2.0), atan(3.0));
	assert_matrix(&matrix, 1, 3, 2, 1, 0, 0);
	gdPathMatrixInitRotate(&matrix, PI / 2.0);
	gdPathMatrixMap(&matrix, 2, 0, &x, &y);
	assert_close(x, 0);
	assert_close(y, 2);
	gdPathMatrixInitRotateTranslate(&matrix, PI / 2.0, 2, 3);
	gdPathMatrixMap(&matrix, 2, 3, &x, &y);
	assert_close(x, 2);
	assert_close(y, 3);
}

static void test_composition_and_aliasing(void)
{
	gdPathMatrix a, b, result, expected;
	double x, y;

	gdPathMatrixInitTranslate(&a, 2, 3);
	gdPathMatrixInitScale(&b, 2, 3);
	gdPathMatrixMultiply(&result, &a, &b);
	gdPathMatrixMap(&result, 1, 1, &x, &y);
	assert_close(x, 6);
	assert_close(y, 12);

	expected = result;
	result = a;
	gdPathMatrixMultiply(&result, &result, &b);
	assert_matrix(&result, expected.m00, expected.m10, expected.m01,
		expected.m11, expected.m02, expected.m12);
	result = b;
	gdPathMatrixMultiply(&result, &a, &result);
	assert_matrix(&result, expected.m00, expected.m10, expected.m01,
		expected.m11, expected.m02, expected.m12);

	gdPathMatrixInitIdentity(&result);
	gdPathMatrixInitTranslate(&a, 2, 3);
	gdPathMatrixMultiply(&expected, &a, &result);
	gdPathMatrixTranslate(&result, 2, 3);
	assert_matrix(&result, expected.m00, expected.m10, expected.m01,
		expected.m11, expected.m02, expected.m12);
	gdPathMatrixInitScale(&a, 2, 3);
	gdPathMatrixMultiply(&expected, &a, &result);
	gdPathMatrixScale(&result, 2, 3);
	assert_matrix(&result, expected.m00, expected.m10, expected.m01,
		expected.m11, expected.m02, expected.m12);
	gdPathMatrixInitShear(&a, atan(0.5), 0);
	gdPathMatrixMultiply(&expected, &a, &result);
	gdPathMatrixShear(&result, atan(0.5), 0);
	assert_matrix(&result, expected.m00, expected.m10, expected.m01,
		expected.m11, expected.m02, expected.m12);
	gdPathMatrixInitRotate(&a, PI / 2.0);
	gdPathMatrixMultiply(&expected, &a, &result);
	gdPathMatrixRotate(&result, PI / 2.0);
	assert_matrix(&result, expected.m00, expected.m10, expected.m01,
		expected.m11, expected.m02, expected.m12);
	gdPathMatrixInitRotateTranslate(&a, -PI / 2.0, 4, 5);
	gdPathMatrixMultiply(&expected, &a, &result);
	gdPathMatrixRotateTranslate(&result, -PI / 2.0, 4, 5);
	assert_matrix(&result, expected.m00, expected.m10, expected.m01,
		expected.m11, expected.m02, expected.m12);
}

static void test_invert_and_mapping(void)
{
	gdPathMatrix matrix, inverse, singular, unchanged;
	gdPointF point = {1, 2};
	gdRectF rect = {1, 2, 3, 4};
	double x, y;

	gdPathMatrixInit(&matrix, 2, 0, 0, 3, 4, 5);
	inverse = matrix;
	gdTestAssert(gdPathMatrixInvert(&inverse));
	gdPathMatrixMap(&matrix, point.x, point.y, &x, &y);
	gdPathMatrixMap(&inverse, x, y, &x, &y);
	assert_close(x, point.x);
	assert_close(y, point.y);

	gdPathMatrixInitScale(&singular, 0, 2);
	unchanged = singular;
	gdTestAssert(!gdPathMatrixInvert(&singular));
	assert_matrix(&singular, unchanged.m00, unchanged.m10, unchanged.m01,
		unchanged.m11, unchanged.m02, unchanged.m12);

	gdPathMatrixInitTranslate(&matrix, 5, 7);
	gdPathMatrixMapPoint(&matrix, &point, &point);
	assert_close(point.x, 6);
	assert_close(point.y, 9);

	gdPathMatrixInitRotate(&matrix, PI / 2.0);
	gdPathMatrixMapRect(&matrix, &rect, &rect);
	assert_close(rect.x, -6);
	assert_close(rect.y, 1);
	assert_close(rect.w, 4);
	assert_close(rect.h, 3);
}

int main(void)
{
	test_initializers();
	test_composition_and_aliasing();
	test_invert_and_mapping();
	return gdNumFailures();
}
