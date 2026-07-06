/* Regression test for invalid TGA color-map entry depth handling. */

#include <stdio.h>

#include "gd.h"
#include "gdtest.h"

int main()
{
	gdImagePtr im;
	FILE *fp;

	gdSetErrorMethod(gdSilence);
	fp = gdTestFileOpen("tga/colormap_bits_stack_oob.tga");
	if (!gdTestAssert(fp != NULL)) {
		return gdNumFailures();
	}

	im = gdImageCreateFromTga(fp);
	gdTestAssert(im == NULL);
	if (im != NULL) {
		gdImageDestroy(im);
	}

	fclose(fp);
	return gdNumFailures();
}
