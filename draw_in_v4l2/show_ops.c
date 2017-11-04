
#include "common.h"
#include "font.h"

#if 0
static int palette [] =
{
	0x000000, 0xffe080, 0xffffff, 0xe0c0a0, 0x304050, 0x80b8c0
};
#define NR_COLORS (sizeof (palette) / sizeof (palette [0]))
#endif
#define NR_BUTTONS 3
static struct ts_button buttons[NR_BUTTONS];

void button_init(void)
{
	memset (&buttons, 0, sizeof (buttons));
	buttons [0].w = buttons [1].w = buttons [2].w = xres / 4;
	buttons [0].h = buttons [1].h = buttons [2].h = 20;
	buttons [0].x = 0;
	buttons [1].x = (3 * xres) / 8;
	buttons [2].x = (3 * xres) / 4;
	buttons [0].y = buttons [1].y = buttons [2].y = 10;
	buttons [0].text = "Drag";
	buttons [1].text = "Draw";
	buttons [2].text = "Quit";
}

/*draw the rect*/
void fillrect (int x1, int y1, int x2, int y2)
{

	int tmp;
	/* Clipping and sanity checking */
	
	if (x1 > x2) { tmp = x1; x1 = x2; x2 = tmp; }
	if (y1 > y2) { tmp = y1; y1 = y2; y2 = tmp; }
	
	if (x1 < 0)
		x1 = 0;
	if ((uint32_t)x1 >= xres)
		x1 = xres - 1;
	
	if (x2 < 0)
		x2 = 0;
	if ((uint32_t)x2 >= xres)
		x2 = xres - 1;
	
	if (y1 < 0)
		y1 = 0;
	if ((uint32_t)y1 >= yres)
		y1 = yres - 1;
	
	if (y2 < 0)
		y2 = 0;
	if ((uint32_t)y2 >= yres)
		y2 = yres - 1;
	
	if ((x1 > x2) || (y1 > y2))
		return;
	
	unsigned char *addr_tmp;
	RgbQuad rgb;
    rgb.Blue = 128;
    rgb.Green = 138;
    rgb.Red = 135;
    rgb.Reserved = 255;
	for (; y1 <= y2; y1++) {
		addr_tmp = line_addr[y1] + x1 * bytes_per_pixel;
		for (tmp = x1; tmp <= x2; tmp++) {
			*addr_tmp = rgb.Blue;
            addr_tmp++;
            *addr_tmp = rgb.Green;
            addr_tmp++;
            *addr_tmp = rgb.Red;
            addr_tmp++;
            *addr_tmp = rgb.Reserved;
            addr_tmp++;
		}
	}
}


/*flush the screen*/
void refresh_screen ()
{
	int i;

	fillrect (0, 0, xres - 1, yres - 1);
	put_string_center (xres/2, yres/4,   "ADAYO Linux platform test tool");
	put_string_center (xres/2, yres/4+20,"This is the tool for touch screen test");

	for (i = 0; i < NR_BUTTONS; i++)
		button_draw (&buttons [i]);
}

void put_string_center(int x, int y, char *s)
{
	size_t sl = strlen (s);
        put_string (x - (sl / 2) * font_vga_8x8.width,
                    y - font_vga_8x8.height / 2, s);
}

void put_string(int x, int y, char *s)
{
	int i;
	for (i = 0; *s; i++, x += font_vga_8x8.width, s++)
		put_char (x, y, *s);
}

void put_char(int x, int y, int c)
{
	int i,j,bits;

	for (i = 0; i < font_vga_8x8.height; i++) {
		bits = font_vga_8x8.data [font_vga_8x8.height * c + i];
		for (j = 0; j < font_vga_8x8.width; j++, bits <<= 1)
			if (bits & 0x80)
				pixel (x + j, y + i);
	}
}

void pixel (int x, int y)
{

	unsigned char *addr_tmp;
	if ((x < 0) || ((uint32_t)x >= xres) ||
	    (y < 0) || ((uint32_t)y >= yres))
		return;
	
	RgbQuad rgb;
    rgb.Blue = 0;
    rgb.Green = 0;
    rgb.Red = 255;
    rgb.Reserved = 255;

	addr_tmp = line_addr[y] + x * bytes_per_pixel;
	*addr_tmp = rgb.Blue;
    addr_tmp++;
    *addr_tmp = rgb.Green;
    addr_tmp++;
    *addr_tmp = rgb.Red;
    addr_tmp++;
    *addr_tmp = rgb.Reserved;
    addr_tmp++;
	
}

void line (int x1, int y1, int x2, int y2)
{
	int tmp;
	int dx = x2 - x1;
	int dy = y2 - y1;

	if (abs (dx) < abs (dy)) {
		if (y1 > y2) {
			tmp = x1; x1 = x2; x2 = tmp;
			tmp = y1; y1 = y2; y2 = tmp;
			dx = -dx; dy = -dy;
		}
		x1 <<= 16;
		/* dy is apriori >0 */
		dx = (dx << 16) / dy;
		while (y1 <= y2) {
			pixel_line(x1 >> 16, y1);
			x1 += dx;
			y1++;
		}
	} else {
		if (x1 > x2) {
			tmp = x1; x1 = x2; x2 = tmp;
			tmp = y1; y1 = y2; y2 = tmp;
			dx = -dx; dy = -dy;
		}
		y1 <<= 16;
		dy = dx ? (dy << 16) / dx : 0;
		while (x1 <= x2) {
			pixel_line(x1, y1 >> 16);
			y1 += dy;
			x1++;
		}
	}
}

void pixel_line (int x, int y)
{

    unsigned char *addr_tmp;
    if ((x < 0) || ((uint32_t)x >= xres) ||
        (y < 0) || ((uint32_t)y >= yres))
        return;

    RgbQuad rgb;
    rgb.Blue = 0;
    rgb.Green = 255;
    rgb.Red = 255;
    rgb.Reserved = 255;

    addr_tmp = line_addr[y] + x * bytes_per_pixel;
    *addr_tmp = rgb.Blue;
    addr_tmp++;
    *addr_tmp = rgb.Green;
    addr_tmp++;
    *addr_tmp = rgb.Red;
    addr_tmp++;
    *addr_tmp = rgb.Reserved;
    addr_tmp++;

}

void rect (int x1, int y1, int x2, int y2)
{
	line (x1, y1, x2, y1);
	line (x2, y1+1, x2, y2-1);
	line (x2, y2, x1, y2);
	line (x1, y2-1, x1, y1+1);
}

void button_draw (struct ts_button *button)
{
	//int s = (button->flags & BUTTON_ACTIVE) ? 3 : 0;

	rect(button->x, button->y, button->x + button->w,
	     button->y + button->h);
	fillrect(button->x + 1, button->y + 1,
		 button->x + button->w - 2,
		 button->y + button->h - 2);
	put_string_center(button->x + button->w / 2,
			  button->y + button->h / 2,
			  button->text);
}

int button_handle (struct ts_button *button, int x, int y, unsigned int p)
{
	int inside = (x >= button->x) && (y >= button->y) &&
		     (x < button->x + button->w) &&
		     (y < button->y + button->h);

	if (p > 0) {
		if (inside) {
			if (!(button->flags & BUTTON_ACTIVE)) {
				button->flags |= BUTTON_ACTIVE;
			}
		} else if (button->flags & BUTTON_ACTIVE) {
			button->flags &= ~BUTTON_ACTIVE;
		}
	} else if (button->flags & BUTTON_ACTIVE) {
		button->flags &= ~BUTTON_ACTIVE;
		return 1;
	}

	return 0;
}

void put_cross(int x, int y)
{
	line (x - 10, y, x - 2, y);
	line (x + 2, y, x + 10, y);
	line (x, y - 10, x, y - 2);
	line (x, y + 2, x, y + 10);

#if 1
	line_green (x - 6, y - 9, x - 9, y - 9);
	line_green (x - 9, y - 8, x - 9, y - 6);
	line_green (x - 9, y + 6, x - 9, y + 9);
	line_green (x - 8, y + 9, x - 6, y + 9);
	line_green (x + 6, y + 9, x + 9, y + 9);
	line_green (x + 9, y + 8, x + 9, y + 6);
	line_green (x + 9, y - 6, x + 9, y - 9);
	line_green (x + 8, y - 9, x + 6, y - 9);
#else
	line (x - 7, y - 7, x - 4, y - 4, colidx + 1);
	line (x - 7, y + 7, x - 4, y + 4, colidx + 1);
	line (x + 4, y - 4, x + 7, y - 7, colidx + 1);
	line (x + 4, y + 4, x + 7, y + 7, colidx + 1);
#endif
}

void line_green(int x1, int y1, int x2, int y2)
{
    int tmp;
    int dx = x2 - x1;
    int dy = y2 - y1;

    if (abs (dx) < abs (dy)) {
        if (y1 > y2) {
            tmp = x1; x1 = x2; x2 = tmp;
            tmp = y1; y1 = y2; y2 = tmp;
            dx = -dx; dy = -dy;
        }
        x1 <<= 16;
        /* dy is apriori >0 */
        dx = (dx << 16) / dy;
        while (y1 <= y2) {
            pixel_green(x1 >> 16, y1);
            x1 += dx;
            y1++;
        }
    } else {
        if (x1 > x2) {
            tmp = x1; x1 = x2; x2 = tmp;
            tmp = y1; y1 = y2; y2 = tmp;
            dx = -dx; dy = -dy;
        }
        y1 <<= 16;
        dy = dx ? (dy << 16) / dx : 0;
        while (x1 <= x2) {
            pixel_green(x1, y1 >> 16);
            y1 += dy;
            x1++;
        }
    }
}

void pixel_green (int x, int y)
{

    unsigned char *addr_tmp;
    if ((x < 0) || ((uint32_t)x >= xres) ||
        (y < 0) || ((uint32_t)y >= yres))
        return;

    RgbQuad rgb;
    rgb.Blue = 0;
    rgb.Green = 255;
    rgb.Red = 0;
    rgb.Reserved = 255;

    addr_tmp = line_addr[y] + x * bytes_per_pixel;
    *addr_tmp = rgb.Blue;
    addr_tmp++;
    *addr_tmp = rgb.Green;
    addr_tmp++;
    *addr_tmp = rgb.Red;
    addr_tmp++;
    *addr_tmp = rgb.Reserved;
    addr_tmp++;

}

void handle_input_data(int x_abs,int y_abs,int p_prs,int *quit_pressed,unsigned int *mode)
{
	int i;
	 for (i = 0; i < NR_BUTTONS; i++)
            if (button_handle(&buttons[i], x_abs, y_abs, p_prs))
                switch (i) {
                case 0:
                    *mode = 0;
                    refresh_screen ();
                    break;
                case 1:
                    *mode = 1;
                    refresh_screen ();
                    break;
                case 2:
                    *quit_pressed = 1;
                }
}

