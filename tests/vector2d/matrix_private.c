#include "gd_vector2d_private.h"
#include "gd_path_matrix.h"
#include "gdtest.h"

#include <math.h>

#define EPSILON 1e-10
#define PI 3.14159265358979323846

static void assert_major_axis(gdPathMatrixPtr matrix, double radius, double expected)
{
	double actual = _gd_matrix_transformed_circle_major_axis(matrix, radius);
	gdTestAssert(fabs(actual - expected) < EPSILON);
}

int main(void)
{
	gdPathMatrix matrix;

	gdPathMatrixInitIdentity(&matrix);
	assert_major_axis(&matrix, 3, 3);
	gdPathMatrixInitRotate(&matrix, PI / 3.0);
	assert_major_axis(&matrix, 3, 3);
	gdPathMatrixInitScale(&matrix, 2, 0.5);
	assert_major_axis(&matrix, 3, 6);
	gdPathMatrixInitScale(&matrix, -1, 1);
	assert_major_axis(&matrix, 3, 3);
	gdPathMatrixInitShear(&matrix, PI / 4.0, 0);
	assert_major_axis(&matrix, 2, 1 + sqrt(5.0));
	gdPathMatrixInit(&matrix, 2, 0, 0, 0.5, 100, -200);
	assert_major_axis(&matrix, 3, 6);

	return gdNumFailures();
}
