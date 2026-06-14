#include "gd.h"
#include "gdtest.h"
#include <stdio.h>

int main() {
	gdImagePtr im;
	FILE *fp = gdTestFileOpen("gif/bug000498.gif");
	im = gdImageCreateFromGif(fp);
	fclose(fp);
	gdImageDestroy(im);
	return 0;
}
