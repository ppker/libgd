#include "gd.h"
#include "gdtest.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
	gdImagePtr im;
	FILE *fp;

	fp = gdTestFileOpen("png/emptyfile");
	im = gdImageCreateFromPng(fp);
	fclose(fp);

	if (!im) {
		return 0;
	} else {
		return 1;
	}
}
