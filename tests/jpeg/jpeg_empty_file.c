#include "gd.h"
#include "gdtest.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
	gdImagePtr im;
	FILE *fp;

	gdSetErrorMethod(gdSilence);

	fp = gdTestFileOpen("jpeg/empty.jpeg");
	im = gdImageCreateFromJpeg(fp);
	fclose(fp);

	if (!im) {
		return 0;
	} else {
		gdImageDestroy(im);
		return 1;
	}
}
