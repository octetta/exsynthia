#ifndef _PLOT_H_
#define _PLOT_H_
int plot_rows(void);
int plot_cols(void);
void plot(void);

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

void ansi(char *s);
void set(int16_t x, int16_t y, int16_t c);
void setrgb(int16_t x, int16_t y, uint8_t r, uint8_t g, uint8_t b);
double map(double x, double in_min, double in_max, double out_min, double out_max);
void cls(void);
#endif
