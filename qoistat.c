#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

struct __attribute__((packed)) qoi_header {
	char magic[4];
	uint32_t width;
	uint32_t height;
	uint8_t channels;
	uint8_t colorspace;
};

struct qoi_tags_stats {
	uint32_t total;
	uint32_t rgb;
	uint32_t rgba;
	uint32_t index;
	uint32_t diff;
	uint32_t luma;
	uint32_t run;
	uint32_t run_pxls;
};

#define QOI_MAGIC "qoif"

#define ISOP_RGB(t) ((t) == 0xFE)
#define ISOP_RGBA(t) ((t) == 0xFF)
#define ISOP_INDEX(t) (((t) & 0xC0) == 0x00)
#define ISOP_DIFF(t) (((t) & 0xC0) == 0x40)
#define ISOP_LUMA(t) (((t) & 0xC0) == 0x80)
#define ISOP_RUN(t) (((t) & 0xC0) == 0xC0)

static inline uint32_t qoi_read32(void* p) {
	uint32_t n;
	uint8_t* c = p;
	n = *c++;
	n = (n << 8) | *c++;
	n = (n << 8) | *c++;
	n = (n << 8) | *c++;
	return n;
}

static __attribute__((noreturn)) void fatal(const char* fmt, ...) {
	va_list ap;
	char msg[256];
	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	fprintf(stderr, "%s", msg);
	exit(EXIT_FAILURE);
}

static struct qoi_header* read_qoi(const char* path) {
	FILE* f;
	size_t sz, rsz;
	struct qoi_header* data;

	f = fopen(path, "r");
	if (f == NULL)
		fatal("ERROR: could not open file '%s': %s\n", path, strerror(errno));

	fseek(f, 0, SEEK_END);
	sz = ftell(f);
	fseek(f, 0, SEEK_SET);

	data = malloc(sz);
	assert(data != NULL && "Out of memory!");
	if ((rsz = fread(data, 1, sz, f)) != sz)
		fatal(
			"ERROR: could not read file '%s' (read %llu/%llu bytes): %s\n",
			path, rsz, sz, strerror(errno));

	fclose(f);

	if (strncmp(data->magic, QOI_MAGIC, sizeof(data->magic)) != 0)
		fatal("ERROR: '%s' is not a valid QOI file");

	// make width and height lsb
	uint32_t width = qoi_read32(&data->width);
	uint32_t height = qoi_read32(&data->height);
	data->width = width;
	data->height = height;
	return data;
}

static void stat_qoi(struct qoi_header* hdr, struct qoi_tags_stats* ts) {
	size_t pxlc;
	uint8_t* pxlp;

	pxlc = hdr->width * hdr->height;
	pxlp = (uint8_t*)(hdr + 1);

	while (pxlc > 0) {
		if (ISOP_RGB(*pxlp)) {
			++ts->rgb;
			pxlp += 3;
		} else if (ISOP_RGBA(*pxlp)) {
			++ts->rgba;
			pxlp += 4;
		} else if (ISOP_INDEX(*pxlp)) {
			++ts->index;
		} else if (ISOP_DIFF(*pxlp)) {
			++ts->diff;
		} else if (ISOP_LUMA(*pxlp)) {
			++ts->luma;
			++pxlp;
		} else if (ISOP_RUN(*pxlp)) {
			++ts->run;
			ts->run_pxls += (*pxlp & 0x3F) + 1;
			pxlc -= (*pxlp & 0x3F);
		}

		++ts->total;
		++pxlp;
		--pxlc;
	}
}

static void print_stats(
  	const char* file,
  	struct qoi_header* hdr,
  	struct qoi_tags_stats* ts) {
	if (file != NULL && hdr != NULL) {
		printf("Statistics for QOI file '%s'\n", file);
		printf("  size:       %ux%u\n", hdr->width, hdr->height);
		printf("  channels:   RGB%c\n", hdr->channels == 3 ? '\0' : 'A');
		printf("  colorspace: %s\n", hdr->colorspace == 0 ? "sRGB with linear alpha" : "all channels linear");
	} else {
		printf("Statistics for QOI batch\n");
	}
	printf("  total tags: %u\t(100.00%%)\n", ts->total);
	printf("  rgb tags:   %u\t(%0.2f%%)\n", ts->rgb, (float) ts->rgb / ts->total * 100.0f);
	printf("  rgba tags:  %u\t(%0.2f%%)\n", ts->rgba, (float) ts->rgba / ts->total * 100.0f);
	printf("  index tags: %u\t(%0.2f%%)\n", ts->index, (float) ts->index / ts->total * 100.0f);
	printf("  diff tags:  %u\t(%0.2f%%)\n", ts->diff, (float) ts->diff / ts->total * 100.0f);
	printf("  luma tags:  %u\t(%0.2f%%)\n", ts->luma, (float) ts->luma / ts->total * 100.0f);
	printf("  run tags:   %u\t(%0.2f%%)\n", ts->run, (float) ts->run / ts->total * 100.0f);
	printf("  avg. run:   %u pixels\n\n", ts->run_pxls / ts->run);
}

static const char* get_program_name(const char* str) {
	size_t index = strlen(str)-1;
	while (index > 0) {
		if (str[index] == '/' || str[index] == '\\') {
			return &str[index+1];
		}
		--index;
	}
	return str;
}

int main(int argc, char* argv[]) {
	struct qoi_header* qoi;
	struct qoi_tags_stats ts;
	int i, batch_mode;

	if (argc < 2)
		fatal("USAGE: %s [FILE]...", argv[0]);

	batch_mode = strcmp(get_program_name(argv[0]), "qoistatbatch") == 0;
	memset(&ts, 0, sizeof(struct qoi_tags_stats));

	for (i = 1; i < argc; i++) {
		qoi = read_qoi(argv[i]);
		stat_qoi(qoi, &ts);

		if (!batch_mode)
			print_stats(argv[i], qoi, &ts);

		free(qoi);
	}

	if (batch_mode)
		print_stats(NULL, NULL, &ts);

	return EXIT_SUCCESS;
}
