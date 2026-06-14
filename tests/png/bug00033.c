#include "gd.h"
#include "gdtest.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
	gdImagePtr im;
	FILE *fp;

	gdSetErrorMethod(gdSilence);

	fp = gdTestFileOpen("png/bug00033.png");
	im = gdImageCreateFromPng(fp);
	fclose(fp);

	if (im) {
		gdImageDestroy(im);
		return 1;
	} else {
		return 0;
	}
}
