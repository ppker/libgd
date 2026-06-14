#include "gd.h"
#include "gdtest.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif

#define QUALITY_COUNT 2

#ifndef _WIN32
static int has_single_quote(const char *s) {
	const char *p;

	if (!s) {
		return 0;
	}

	for (p = s; *p != '\0'; p++) {
		if (*p == '\'') {
			return 1;
		}
	}

	return 0;
}
#endif

#ifdef _WIN32
static int has_double_quote(const char *s) {
	const char *p;

	if (!s) {
		return 0;
	}

	for (p = s; *p != '\0'; p++) {
		if (*p == '"') {
			return 1;
		}
	}

	return 0;
}
#endif

static int cli_status_succeeded(int status, const char *cmd) {
#ifdef _WIN32
	if (status != 0) {
		gdTestErrorMsg("CLI invocation failed: exit=%d, command=%s\n", status,
					   cmd);
		return 0;
	}
#else
	if (WIFEXITED(status)) {
		int exit_code = WEXITSTATUS(status);
		if (exit_code != 0) {
			gdTestErrorMsg("CLI invocation failed: exit=%d, command=%s\n",
						   exit_code, cmd);
			return 0;
		}
	} else {
		gdTestErrorMsg("CLI invocation did not exit normally: command=%s\n",
					   cmd);
		return 0;
	}
#endif

	return 1;
}

static int run_cli_decode(const char *input_jpg, const char *metadata_path,
						  const char *raw_path) {
	char cmd[4096];
	int status;
	int rc;

	if (!input_jpg || !metadata_path || !raw_path) {
		gdTestErrorMsg("CLI invocation has NULL path argument\n");
		return 0;
	}

#ifdef _WIN32
	if (has_double_quote(input_jpg) || has_double_quote(metadata_path) ||
		has_double_quote(raw_path)) {
		gdTestErrorMsg(
			"CLI invocation paths must not include double quote characters\n");
		return 0;
	}

	rc = snprintf(cmd, sizeof(cmd),
				  "ultrahdr_app -m 1 -j \"%s\" -f \"%s\" -z \"%s\" >nul 2>&1",
				  input_jpg, metadata_path, raw_path);
#else
	if (has_single_quote(input_jpg) || has_single_quote(metadata_path) ||
		has_single_quote(raw_path)) {
		gdTestErrorMsg(
			"CLI invocation paths must not include single quote characters\n");
		return 0;
	}

	rc = snprintf(cmd, sizeof(cmd),
				  "ultrahdr_app -m 1 -j '%s' -f '%s' -z '%s' >/dev/null 2>&1",
				  input_jpg, metadata_path, raw_path);
#endif
	if (rc < 0 || (size_t)rc >= sizeof(cmd)) {
		gdTestErrorMsg("CLI command buffer overflow while preparing command\n");
		return 0;
	}

	status = system(cmd);
	if (status == -1) {
		gdTestErrorMsg("CLI invocation failed to spawn process: errno=%d\n",
					   errno);
		return 0;
	}

	return cli_status_succeeded(status, cmd);
}

static unsigned char *read_binary_file(const char *path, int *size) {
	FILE *fp;
	long len;
	unsigned char *data;

	if (!path || !size) {
		return NULL;
	}

	fp = fopen(path, "rb");
	if (!fp) {
		return NULL;
	}

	if (fseek(fp, 0, SEEK_END) != 0) {
		fclose(fp);
		return NULL;
	}

	len = ftell(fp);
	if (len <= 0 || fseek(fp, 0, SEEK_SET) != 0) {
		fclose(fp);
		return NULL;
	}

	data = (unsigned char *)malloc((size_t)len);
	if (!data) {
		fclose(fp);
		return NULL;
	}

	if (fread(data, 1, (size_t)len, fp) != (size_t)len) {
		free(data);
		fclose(fp);
		return NULL;
	}

	fclose(fp);
	*size = (int)len;
	return data;
}

static int compare_files_exact(const char *lhs_path, const char *rhs_path,
							   const char *label) {
	unsigned char *lhs = NULL;
	unsigned char *rhs = NULL;
	int lhs_size = 0;
	int rhs_size = 0;
	int i;

	lhs = read_binary_file(lhs_path, &lhs_size);
	rhs = read_binary_file(rhs_path, &rhs_size);
	if (!lhs || !rhs) {
		gdTestErrorMsg("failed to read %s files for exact comparison\n", label);
		free(lhs);
		free(rhs);
		return 0;
	}

	if (lhs_size != rhs_size) {
		gdTestErrorMsg("%s size mismatch: %d vs %d\n", label, lhs_size,
					   rhs_size);
		free(lhs);
		free(rhs);
		return 0;
	}

	for (i = 0; i < lhs_size; i++) {
		if (lhs[i] != rhs[i]) {
			gdTestErrorMsg("%s byte mismatch at index %d: %u vs %u\n", label, i,
						   (unsigned)lhs[i], (unsigned)rhs[i]);
			free(lhs);
			free(rhs);
			return 0;
		}
	}

	free(lhs);
	free(rhs);
	return 1;
}

int main(void) {
	char *sample_path = NULL;
	char *cli_src_meta = NULL;
	char *cli_src_raw = NULL;
	char *gd_out_jpg[QUALITY_COUNT] = {NULL, NULL};
	char *cli_gd_meta[QUALITY_COUNT] = {NULL, NULL};
	char *cli_gd_raw[QUALITY_COUNT] = {NULL, NULL};
	const int qualities[QUALITY_COUNT] = {90, 50};
	gdUhdrImagePtr src_im = NULL;
	gdUhdrImagePtr gd_im = NULL;
	gdUhdrError err;
	int i;

	if (!gdTestAssertMsg(
			gdUhdrIsAvailable() == 1,
			"UltraHDR support should be enabled for this test\n")) {
		goto cleanup;
	}

	sample_path = gdTestFilePath("uhdr/uhdr_sample.jpg");
	if (!gdTestAssertMsg(sample_path != NULL,
						 "failed to resolve UltraHDR sample path\n")) {
		goto cleanup;
	}

	cli_src_meta = gdTestTempFile("uhdr_cli_quality_baseline_metadata.cfg");
	cli_src_raw = gdTestTempFile("uhdr_cli_quality_baseline_dump.raw");
	if (!gdTestAssertMsg(cli_src_meta && cli_src_raw,
						 "failed to allocate baseline temp paths\n")) {
		goto cleanup;
	}

	if (!gdTestAssertMsg(run_cli_decode(sample_path, cli_src_meta, cli_src_raw),
						 "CLI decode for baseline image failed\n")) {
		goto cleanup;
	}

	memset(&err, 0, sizeof(err));
	src_im = gdUhdrImageCreateFromFile(sample_path, GD_UHDR_FORMAT_JPEG, &err);
	if (!gdTestAssertMsg(src_im != NULL,
						 "gdUhdrImageCreateFromFile failed: code=%d "
						 "provider=%d message=%s\n",
						 err.code, err.provider_code, err.message)) {
		goto cleanup;
	}

	for (i = 0; i < QUALITY_COUNT; i++) {
		char out_name[64];
		char meta_name[64];
		char raw_name[64];
		int rc;

		snprintf(out_name, sizeof(out_name),
				 "uhdr_gd_quality_%d_noop_output.jpg", qualities[i]);
		snprintf(meta_name, sizeof(meta_name),
				 "uhdr_cli_quality_%d_metadata.cfg", qualities[i]);
		snprintf(raw_name, sizeof(raw_name), "uhdr_cli_quality_%d_dump.raw",
				 qualities[i]);

		gd_out_jpg[i] = gdTestTempFile(out_name);
		cli_gd_meta[i] = gdTestTempFile(meta_name);
		cli_gd_raw[i] = gdTestTempFile(raw_name);
		if (!gdTestAssertMsg(gd_out_jpg[i] && cli_gd_meta[i] && cli_gd_raw[i],
							 "failed to allocate temp paths for quality %d\n",
							 qualities[i])) {
			goto cleanup;
		}

		memset(&err, 0, sizeof(err));
		rc = gdUhdrImageFile(src_im, gd_out_jpg[i], GD_UHDR_FORMAT_JPEG,
							 qualities[i], &err);
		if (!gdTestAssertMsg(rc == GD_UHDR_SUCCESS,
							 "gdUhdrImageFile failed for quality %d: code=%d "
							 "provider=%d message=%s\n",
							 qualities[i], err.code, err.provider_code,
							 err.message)) {
			goto cleanup;
		}

		if (!gdTestAssertMsg(
				run_cli_decode(gd_out_jpg[i], cli_gd_meta[i], cli_gd_raw[i]),
				"CLI decode for GD output image failed at quality %d\n",
				qualities[i])) {
			goto cleanup;
		}

		if (!gdTestAssertMsg(
				compare_files_exact(cli_src_meta, cli_gd_meta[i], "metadata"),
				"metadata exact parity check failed at quality %d\n",
				qualities[i])) {
			goto cleanup;
		}

		if (!gdTestAssertMsg(
				compare_files_exact(cli_src_raw, cli_gd_raw[i], "raw"),
				"raw exact parity check failed at quality %d\n",
				qualities[i])) {
			goto cleanup;
		}

		memset(&err, 0, sizeof(err));
		gd_im =
			gdUhdrImageCreateFromFile(gd_out_jpg[i], GD_UHDR_FORMAT_JPEG, &err);
		if (!gdTestAssertMsg(gd_im != NULL,
							 "failed to reload GD output as UHDR at quality "
							 "%d: code=%d provider=%d message=%s\n",
							 qualities[i], err.code, err.provider_code,
							 err.message)) {
			goto cleanup;
		}

		gdTestAssertMsg(gdUhdrImageWidth(src_im) == gdUhdrImageWidth(gd_im),
						"output width mismatch at quality %d: %d vs %d\n",
						qualities[i], gdUhdrImageWidth(src_im),
						gdUhdrImageWidth(gd_im));
		gdTestAssertMsg(gdUhdrImageHeight(src_im) == gdUhdrImageHeight(gd_im),
						"output height mismatch at quality %d: %d vs %d\n",
						qualities[i], gdUhdrImageHeight(src_im),
						gdUhdrImageHeight(gd_im));
		gdTestAssertMsg(
			gdUhdrImageHasGainMap(src_im) == gdUhdrImageHasGainMap(gd_im),
			"gain map flag mismatch at quality %d: %d vs %d\n", qualities[i],
			gdUhdrImageHasGainMap(src_im), gdUhdrImageHasGainMap(gd_im));

		gdUhdrImageDestroy(gd_im);
		gd_im = NULL;
	}

cleanup:
	if (gd_im) {
		gdUhdrImageDestroy(gd_im);
	}
	if (src_im) {
		gdUhdrImageDestroy(src_im);
	}
	for (i = 0; i < QUALITY_COUNT; i++) {
		gdFree(gd_out_jpg[i]);
		gdFree(cli_gd_meta[i]);
		gdFree(cli_gd_raw[i]);
	}
	gdFree(sample_path);
	gdFree(cli_src_meta);
	gdFree(cli_src_raw);

	return gdNumFailures();
}
