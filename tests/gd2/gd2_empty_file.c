#include "gd.h"
#include "gdtest.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
	gdImagePtr im;
	FILE *fp = gdTestFileOpen("gd2/empty.gd2");
	im = gdImageCreateFromGd2(fp);
	fclose(fp);

	if (!im) {
		return 0;
	} else {
		gdImageDestroy(im);
		return 1;
	}
}
