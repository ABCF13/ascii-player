/*
 * ascii_conv.c - raw grayscale frames -> fixed-width ASCII clip file.
 *
 * Build:  gcc -O2 -o ascii_conv ascii_conv.c -lm
 * Use (file based, works everywhere incl. Windows):
 *   ffmpeg -y -i source/video.mp4 -vf "fps=12,scale=154:43" -f rawvideo -pix_fmt gray temp.raw
 *   ascii_conv temp.raw 001 154 43 clips/clip_001.txt [bg_cutoff=24] [edge_thresh=90]
 * (convert.ps1 runs all of this for you.)
 * Pass "-" instead of temp.raw to read from stdin (Linux/macOS pipes).
 *
 * File layout:
 *   bytes 0..63 : header, space padded, ends in '\n'
 *                 "ASCV1 AAA W H FPS FRAMES"   e.g. "ASCV1 001 100 28 12 002160"
 *   then FRAMES blocks of exactly n = W*H chars (no newlines).
 *   Frame i starts at byte 64 + i*n; its time is i/12 s -> MM:SS:FF is derived.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define HEADER_LEN 64
#define FPS 12
static const char RAMP[] = " .`',:;~=+*ox%O#&@";   /* dark -> bright, 19 levels (no | - / \ so outlines stay distinct) */

static void write_header(FILE *f, int id, int w, int h, long frames) {
    char hdr[HEADER_LEN];
    char tmp[HEADER_LEN];
    snprintf(tmp, sizeof tmp, "ASCV1 %03d %d %d %d %06ld", id, w, h, FPS, frames);
    memset(hdr, ' ', HEADER_LEN);
    memcpy(hdr, tmp, strlen(tmp));
    hdr[HEADER_LEN - 1] = '\n';
    fseek(f, 0, SEEK_SET);
    fwrite(hdr, 1, HEADER_LEN, f);
}

static inline int px(const unsigned char *s, int w, int h, int x, int y) {
    if (x < 0) x = 0;
    if (x >= w) x = w - 1;
    if (y < 0) y = 0;
    if (y >= h) y = h - 1;
    return s[y * w + x];
}

/* Pick '|', '-', '/' or '\' perpendicular to the gradient. */
static char edge_char(int gx, int gy) {
    double ax = fabs((double)gx), ay = fabs((double)gy);
    double ang = atan2(ay, ax) * 180.0 / M_PI;      /* 0 = horizontal gradient */
    if (ang < 22.5) return '|';
    if (ang > 67.5) return '-';
    return ((gx > 0) == (gy > 0)) ? '/' : '\\';
}

int main(int argc, char **argv) {
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY);   /* stop Windows mangling raw bytes */
#endif
    if (argc < 6) {
        fprintf(stderr, "usage: %s in.raw|- ID W H out.txt [bg_cutoff] [edge_thresh]\n", argv[0]);
        return 1;
    }
    const char *inpath = argv[1];
    int id = atoi(argv[2]), w = atoi(argv[3]), h = atoi(argv[4]);
    const char *outpath = argv[5];
    int cutoff = argc > 6 ? atoi(argv[6]) : 24;   /* below this = background = ' ' */
    int ethr   = argc > 7 ? atoi(argv[7]) : 90;   /* Sobel magnitude for outlines  */
    if (id < 0 || id > 999 || w <= 0 || h <= 0) { fprintf(stderr, "bad args\n"); return 1; }

    FILE *in = strcmp(inpath, "-") == 0 ? stdin : fopen(inpath, "rb");
    if (!in) { perror(inpath); return 1; }

    size_t n = (size_t)w * h;
    unsigned char *src = malloc(n);
    char *out = malloc(n);
    FILE *f = fopen(outpath, "wb");
    if (!src || !out || !f) { perror("setup"); return 1; }

    write_header(f, id, w, h, 0);                 /* placeholder */
    fseek(f, HEADER_LEN, SEEK_SET);

    long frames = 0;
    while (fread(src, 1, n, in) == n) {
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int p = src[y * w + x];
                char c = ' ';
                if (p >= cutoff) {
                    int a = px(src,w,h,x-1,y-1), b = px(src,w,h,x,y-1), cc = px(src,w,h,x+1,y-1);
                    int d = px(src,w,h,x-1,y),                          e  = px(src,w,h,x+1,y);
                    int g = px(src,w,h,x-1,y+1), hh = px(src,w,h,x,y+1), i = px(src,w,h,x+1,y+1);
                    int gx = -a + cc - 2*d + 2*e - g + i;
                    int gy = -a - 2*b - cc + g + 2*hh + i;
                    double mag = sqrt((double)gx*gx + (double)gy*gy);
                    if (mag > ethr) c = edge_char(gx, gy);
                    else            c = RAMP[p * (int)(sizeof RAMP - 2) / 255];
                    if (c == ' ') c = '.';        /* keep dim-but-present pixels visible */
                }
                out[y * w + x] = c;
            }
        }
        fwrite(out, 1, n, f);
        frames++;
    }

    write_header(f, id, w, h, frames);
    fclose(f);
    fprintf(stderr, "clip %03d: %ld frames (%.1fs), %dx%d, %zu bytes\n",
            id, frames, frames / (double)FPS, w, h, HEADER_LEN + frames * n);
    free(src); free(out);
    return 0;
}
