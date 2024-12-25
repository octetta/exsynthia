#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define ROWS (48)
#define COLS (128)
#define _ROWS (ROWS/4)
#define _ZERO (_ROWS/2)
#define _COLS (COLS/2)

int plot_rows(void) { return ROWS; }
int plot_cols(void) { return COLS; }

static uint32_t  canvas[(ROWS>>2)*(COLS>>1)];
static uint32_t  colors[(ROWS>>2)*(COLS>>1)];

static char *reset = "\033[39;49m";
static char *clear = "\033[2J";
static char *home = "\033[H";
static char *italic_on = "\033[3m";
static char *italic_off = "\033[23m";
static char rotochar[32] = ": ";
static char rotoicon[32] = "[R]";

void cls(void) {
    memset(canvas, 0, sizeof(canvas));
    memset(colors, 0, sizeof(colors));
}

static uint16_t pixel_map[4][2] = {
    {0x01, 0x08},
    {0x02, 0x10},
    {0x04, 0x20},
    {0x40, 0x80},
};

double map(
    double x,
    double in_min,
    double in_max,
    double out_min,
    double out_max) {
    double n = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    return n;
}

int getoffset(int16_t x, int16_t y) {
    return (y * _COLS + x);
}

void setrgb(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b) {
    int offset;
    if (x < 0) return;
    if (y < 0) return;
    if (x > COLS-1) return;
    if (y > ROWS-1) return;
    uint16_t p = pixel_map[y % 4][x % 2];
    x /= 2;
    y /= 4;
    offset = getoffset(x, y);
    if (offset < sizeof(canvas)) {
        canvas[offset] |= p;
        uint32_t rgb = colors[offset];
        uint8_t *ptr = (uint8_t *)&rgb;
        // r = [0], g = [1], b = [2]
        int16_t pr = ptr[0];
        int16_t pg = ptr[1];
        int16_t pb = ptr[2];
        //
        int16_t nr = (r + pr) / 2;
        int16_t ng = (g + pg) / 2;
        int16_t nb = (b + pb) / 2;
        if (nr < 0) nr = 0;
        if (ng < 0) ng = 0;
        if (nb < 0) nb = 0;
        if (nr > 255) nr = 255;
        if (ng > 255) ng = 255;
        if (nb > 255) nb = 255;
        ptr[0] = nr;
        ptr[1] = ng;
        ptr[2] = nb;
        colors[offset] = rgb;
    }
}

void set(int16_t x, int16_t y, int16_t c) {
    int offset;
    if (x < 0) return;
    if (y < 0) return;
    if (x > COLS-1) return;
    if (y > ROWS-1) return;
    uint16_t p = pixel_map[y % 4][x % 2];
    x /= 2;
    y /= 4;
    offset = getoffset(x, y);
    if (offset < sizeof(canvas)) {
        canvas[offset] |= p;
        if (colors[offset] == 0) {
            colors[offset] = c;
        } else {
            if (c < 0) {
                colors[offset] = -c;
            }
        }
    }
}

#define UNICODE_BOX (0x2500)

#define UNICODE_BRAILLE (0x2800)
static char *fg_color[] = {
    "\033[30m", // 0 black
    "\033[31m", // 1 red
    "\033[32m", // 2 green
    "\033[33m", // 3 yellow
    "\033[34m", // 4 blue
    "\033[35m", // 5 purple
    "\033[37m", // 6 cyan
    "\033[37m", // 7 white
    "\033[38;2;85;85;85m", // 8 dkgray
    "\033[38;2;0;128;0m", // 8 dkgray
};

#define BLACK  0
#define RED    1
#define GREEN  2
#define YELLOW 3
#define BLUE   4
#define PURPLE 5
#define CYAN   6
#define WHITE  7
#define COLOR0 8
#define COLOR1 9
#define COLOR2 10
#define COLOR3 11
#define COLOR4 12
#define COLOR5 13
#define COLOR6 14
#define COLOR7 15

#define DKGRAY 8

#define RED_RGB    248, 0,   0
#define GREEN_RGB  0,   220, 1
#define YELLOW_RGB 240, 240, 0
#define WHITE_RGB  240, 240, 240
#define BLUE_RGB   0,   0,   240

void ansi(char *s) {
    write(1, s, strlen(s));
}

static int use_lut = 0;

int utf8_encode(char *out, uint32_t utf);

void plot(void) {
    uint32_t x, y;
    char raw[32];
    for (y = 0; y < _ROWS; y++) {
        ansi(reset);
        for (x = 0; x < _COLS; x++) {
            uint32_t offset = y * _COLS + x;
            uint32_t b = canvas[offset];
            if (b) {
                uint32_t c = colors[offset];
                if (c >= 0) {
                    if (use_lut) {
                        char *s = fg_color[c];
                        ansi(s);
                    } else { // using rgb system
                        char buf[32];
                        uint8_t *ptr = (uint8_t *)&c;
                        uint8_t r = ptr[0];
                        uint8_t g = ptr[1];
                        uint8_t b = ptr[2];
                        sprintf(buf, "\033[38;2;%d;%d;%dm", r, g, b);
                        ansi(buf);
                    }
                }
                b += UNICODE_BRAILLE;
                utf8_encode(raw, b);
                ansi(raw);
            } else {
                ansi(" ");
            }
        }
        ansi(reset);
        ansi("\n");
    }
}

/**
 * Encode a code point using UTF-8
 * 
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @license MIT
 * 
 * @param out - output buffer (min 5 characters), will be 0-terminated
 * @param utf - code point 0-0x10FFFF
 * @return number of bytes on success, 0 on failure (also produces U+FFFD, which uses 3 bytes)
 */
int utf8_encode(char *out, uint32_t utf) {
    if (utf <= 0x7F) {
        // Plain ASCII
        out[0] = (char) utf;
        out[1] = 0;
        return 1;
    } else if (utf <= 0x07FF) {
        // 2-byte unicode
        out[0] = (char) (((utf >> 6) & 0x1F) | 0xC0);
        out[1] = (char) (((utf >> 0) & 0x3F) | 0x80);
        out[2] = 0;
        return 2;
    } else if (utf <= 0xFFFF) {
        // 3-byte unicode
        out[0] = (char) (((utf >> 12) & 0x0F) | 0xE0);
        out[1] = (char) (((utf >>  6) & 0x3F) | 0x80);
        out[2] = (char) (((utf >>  0) & 0x3F) | 0x80);
        out[3] = 0;
        return 3;
    } else if (utf <= 0x10FFFF) {
        // 4-byte unicode
        out[0] = (char) (((utf >> 18) & 0x07) | 0xF0);
        out[1] = (char) (((utf >> 12) & 0x3F) | 0x80);
        out[2] = (char) (((utf >>  6) & 0x3F) | 0x80);
        out[3] = (char) (((utf >>  0) & 0x3F) | 0x80);
        out[4] = 0;
        return 4;
    } else { 
        // error - use replacement character
        out[0] = (char) 0xEF;  
        out[1] = (char) 0xBF;
        out[2] = (char) 0xBD;
        out[3] = 0;
        return 0;
    }
}

