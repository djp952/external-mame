/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


void rastan_vh_stop (void);

size_t rastan_videoram_size;
data16_t *rastan_videoram1;
data16_t *rastan_videoram3;
data16_t *rastan_spriteram;
data16_t *rastan_scrollx;
data16_t *rastan_scrolly;

static unsigned char *rastan_dirty1;
static unsigned char *rastan_dirty3;

static unsigned char spritepalettebank;
static int flipscreen;

static struct osd_bitmap *tmpbitmap1;
static struct osd_bitmap *tmpbitmap3;



int rastan_vh_start (void)
{
	/* Allocate a video RAM */
	rastan_dirty1 = malloc ( rastan_videoram_size/2);
	if (!rastan_dirty1)
	{
		rastan_vh_stop();
		return 1;
	}
	memset(rastan_dirty1,1,rastan_videoram_size / 2);

	rastan_dirty3 = malloc ( rastan_videoram_size/2);
	if (!rastan_dirty3)
	{
		rastan_vh_stop();
		return 1;
	}
	memset(rastan_dirty3,1,rastan_videoram_size / 2);

	/* Allocate temporary bitmaps */
 	if ((tmpbitmap1 = bitmap_alloc(512,512)) == 0)
	{
		rastan_vh_stop ();
		return 1;
	}
	if ((tmpbitmap3 = bitmap_alloc(512,512)) == 0)
	{
		rastan_vh_stop ();
		return 1;
	}

	flipscreen = 0; /*maybe not needed*/

	return 0;
}



void rastan_vh_stop (void)
{
	/* Free temporary bitmaps */
	if (tmpbitmap3)
		bitmap_free (tmpbitmap3);
	tmpbitmap3 = 0;
	if (tmpbitmap1)
		bitmap_free (tmpbitmap1);
	tmpbitmap1 = 0;

	/* Free video RAM */
	if (rastan_dirty1)
	        free (rastan_dirty1);
	if (rastan_dirty3)
	        free (rastan_dirty3);
	rastan_dirty1 = rastan_dirty3 = 0;
}



WRITE16_HANDLER( rastan_videoram1_w )
{
	int oldword = rastan_videoram1[offset];
	COMBINE_DATA(&rastan_videoram1[offset]);

	if (oldword != rastan_videoram1[offset])
	{
		rastan_dirty1[offset / 2] = 1;
	}
}
READ16_HANDLER( rastan_videoram1_r )
{
   return rastan_videoram1[offset];
}

WRITE16_HANDLER( rastan_videoram3_w )
{
	int oldword = rastan_videoram3[offset];
	COMBINE_DATA(&rastan_videoram3[offset]);

	if (oldword != rastan_videoram3[offset])
	{
		rastan_dirty3[offset / 2] = 1;
	}
}
READ16_HANDLER( rastan_videoram3_r )
{
   return rastan_videoram3[offset];
}



WRITE16_HANDLER( rastan_videocontrol_w )
{
	if (offset == 0)
	{
		/* bits 0 and 1 are coin lockout */
		coin_lockout_w(1,~data & 0x01);
		coin_lockout_w(0,~data & 0x02);

		/* bits 2 and 3 are the coin counters */
		coin_counter_w(1,data & 0x04);
		coin_counter_w(0,data & 0x08);

		/* bits 5-7 look like the sprite palette bank */
		spritepalettebank = (data & 0xe0) >> 5;

		/* other bits unknown */
	}
}

WRITE16_HANDLER( rastan_flipscreen_w )
{
	if (offset == 0)
	{
		/* flipscreen when bit 0 set */
		if (flipscreen != (data & 1) )
		{
			flipscreen = data & 1;
			memset(rastan_dirty1,1,rastan_videoram_size / 2);
			memset(rastan_dirty3,1,rastan_videoram_size / 2);
		}
	}
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void rastan_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int scrollx,scrolly;


palette_init_used_colors();

{
	int color,code,i;
	int colmask[128];
	int pal_base;


	pal_base = 0;

	for (color = 0;color < 128;color++) colmask[color] = 0;

	for (offs = rastan_videoram_size/2 - 2;offs >= 0;offs -= 2)
	{
		code  = rastan_videoram1[offs + 1] & 0x3fff;
		color = rastan_videoram1[offs] & 0x7f;

		colmask[color] |= Machine->gfx[0]->pen_usage[code];
	}

	for (offs = rastan_videoram_size/2 - 2;offs >= 0;offs -= 2)
	{
		code  = rastan_videoram3[offs + 1] & 0x3fff;
		color = rastan_videoram3[offs] & 0x7f;

		colmask[color] |= Machine->gfx[0]->pen_usage[code];
	}

	for (offs = 0x800/2-4; offs >= 0; offs -= 4)
	{
		code = rastan_spriteram[offs+2] & 0x0fff;

		if (code)
		{
			int data1;

			data1 = rastan_spriteram[offs];

			color = (data1 & 0x0f) + 0x10 * spritepalettebank;
			colmask[color] |= Machine->gfx[1]->pen_usage[code];
		}
	}

	for (color = 0;color < 128;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}


	if (palette_recalc())
	{
		memset(rastan_dirty1,1,rastan_videoram_size / 2);
		memset(rastan_dirty3,1,rastan_videoram_size / 2);
	}
}


	for (offs = rastan_videoram_size/2 - 2;offs >= 0;offs -= 2)
	{
		if (rastan_dirty1[offs/2])
		{
			int sx,sy;
			int data1,data2;
			int flipx,flipy;


			rastan_dirty1[offs/2] = 0;

			data1 = rastan_videoram1[offs];
			data2 = rastan_videoram1[offs + 1];

			sx = (offs/2) % 64;
			sy = (offs/2) / 64;

			flipx = data1 & 0x4000;
			flipy = data1 & 0x8000;

			if (flipscreen)
			{
				flipx = !flipx;
				flipy = !flipy;
				sx = 63 - sx;
				sy = 63 - sy;
			}

			drawgfx(tmpbitmap1, Machine->gfx[0],
					data2 & 0x3fff,
					data1 & 0x7f,
					flipx, flipy,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	for (offs = rastan_videoram_size/2 - 2;offs >= 0;offs -= 2)
	{
		if (rastan_dirty3[offs/2])
		{
			int sx,sy;
			int data1,data2;
			int flipx,flipy;


			rastan_dirty3[offs/2] = 0;

			data1 = rastan_videoram3[offs];
			data2 = rastan_videoram3[offs + 1];

			sx = (offs/2) % 64;
			sy = (offs/2) / 64;

			flipx = data1 & 0x4000;
			flipy = data1 & 0x8000;

			if (flipscreen)
			{
				flipx = !flipx;
				flipy = !flipy;
				sx = 63 - sx;
				sy = 63 - sy;
			}

			drawgfx(tmpbitmap3, Machine->gfx[0],
					data2 & 0x3fff,
					data1 & 0x7f,
					flipx, flipy,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	scrollx = rastan_scrollx[0] - 16;
	scrolly = rastan_scrolly[0];
	if (flipscreen)
	{
		scrollx = 320-scrollx;
		scrolly = 240-scrolly+16;
	}
	copyscrollbitmap(bitmap,tmpbitmap1,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);

	scrollx = rastan_scrollx[1] - 16;
	scrolly = rastan_scrolly[1];
	if (flipscreen)
	{
		scrollx = 320-scrollx;
		scrolly = 240-scrolly+16;
	}
	copyscrollbitmap(bitmap,tmpbitmap3,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);


	/* Draw the sprites. 256 sprites in total */
	for (offs = 0x800/2-4; offs >= 0; offs -= 4)
	{
		int num = rastan_spriteram[offs+2];

		if (num)
		{
			int sx,sy,col,data1;
			int flipx,flipy;

			sx = rastan_spriteram[offs+3] & 0x1ff;
			if (sx > 400) sx = sx - 512;
 			sy = rastan_spriteram[offs+1] & 0x1ff;
			if (sy > 400) sy = sy - 512;

			data1 = rastan_spriteram[offs];

			col = (data1 & 0x0f) + 0x10 * spritepalettebank;

			flipx = data1 & 0x4000;
			flipy = data1 & 0x8000;

			if (flipscreen)
			{
				flipx = !flipx;
				flipy = !flipy;
				sx = 320 - sx - 16;
				sy = 240 - sy;
			}

			drawgfx(bitmap,Machine->gfx[1],
					num,
					col,
					flipx, flipy,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}

void rainbow_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int scrollx,scrolly;


palette_init_used_colors();

/* TODO: we are using the same table for background and foreground tiles, but this */
/* causes the sky to be black instead of blue. */
{
	int color,code,i;
	int colmask[128];
	int pal_base;


	pal_base = 0;

	for (color = 0;color < 128;color++)
	{
		colmask[color] = 0;
    }

	for (offs = rastan_videoram_size/2 - 2;offs >= 0;offs -= 2)
	{
		code  = rastan_videoram1[offs + 1] & 0x3FFF;
		color = rastan_videoram1[offs] & 0x7f;

		colmask[color] |= Machine->gfx[0]->pen_usage[code];
	}

	for (offs = rastan_videoram_size/2 - 2;offs >= 0;offs -= 2)
	{
		code  = rastan_videoram3[offs + 1] & 0x3fff;
		color = rastan_videoram3[offs] & 0x7f;

		colmask[color] |= Machine->gfx[0]->pen_usage[code];
	}

	for (offs = 0x800/2-4; offs >= 0; offs -= 4)
	{
		code = rastan_spriteram[offs+2];

		if (code)
		{
			int data1;

			data1 = rastan_spriteram[offs];

			color = (data1 + 0x10) & 0x7f;

            if(code < 4096)
				colmask[color] |= Machine->gfx[1]->pen_usage[code];
            else
				colmask[color] |= Machine->gfx[2]->pen_usage[code-4096];
		}
	}

	for (color = 0;color < 128;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_USED;

		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

    /* Make one transparent colour */

    palette_used_colors[pal_base] = PALETTE_COLOR_TRANSPARENT;

	if (palette_recalc())
	{
		memset(rastan_dirty1,1,rastan_videoram_size / 2);
		memset(rastan_dirty3,1,rastan_videoram_size / 2);
	}
}


	for (offs = rastan_videoram_size/2 - 2;offs >= 0;offs -= 2)
	{
		if (rastan_dirty1[offs/2])
		{
			int sx,sy;
			int data1,data2;
			int flipx,flipy;


			rastan_dirty1[offs/2] = 0;

			data1 = rastan_videoram1[offs];
			data2 = rastan_videoram1[offs + 1];

			sx = (offs/2) % 64;
			sy = (offs/2) / 64;

			flipx = data1 & 0x4000;
			flipy = data1 & 0x8000;

			if (flipscreen)
			{
				flipx = !flipx;
				flipy = !flipy;
				sx = 63 - sx;
				sy = 63 - sy;
			}

			drawgfx(tmpbitmap1, Machine->gfx[0],
					data2 & 0x3fff,
					data1 & 0x7f,
					flipx, flipy,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	for (offs = rastan_videoram_size/2 - 2;offs >= 0;offs -= 2)
	{
		if (rastan_dirty3[offs/2])
		{
			int sx,sy;
			int data1,data2;
			int flipx,flipy;


			rastan_dirty3[offs/2] = 0;

			data1 = rastan_videoram3[offs];
			data2 = rastan_videoram3[offs + 1];

			sx = (offs/2) % 64;
			sy = (offs/2) / 64;

			flipx = data1 & 0x4000;
			flipy = data1 & 0x8000;

			if (flipscreen)
			{
				flipx = !flipx;
				flipy = !flipy;
				sx = 63 - sx;
				sy = 63 - sy;
			}
            /* Colour as Transparent */

			drawgfx(tmpbitmap3, Machine->gfx[0],
					0,
					0,
					flipx, flipy,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);

            /* Draw over with correct Transparency */

			drawgfx(tmpbitmap3, Machine->gfx[0],
					data2 & 0x3fff,
					data1 & 0x7f,
					flipx, flipy,
					8*sx,8*sy,
					0,TRANSPARENCY_PEN,0);
		}
	}

	scrollx = rastan_scrollx[0] - 16;
	scrolly = rastan_scrolly[0];
	if (flipscreen)
	{
		scrollx = 320-scrollx;
		scrolly = 240-scrolly+16;
	}
	copyscrollbitmap(bitmap,tmpbitmap1,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);

	/* Draw the sprites. 256 sprites in total */
	for (offs = 0x800/2-4; offs >= 0; offs -= 4)
	{
		int num = rastan_spriteram[offs+2];

		if (num)
		{
			int sx,sy,col,data1;
			int flipx,flipy;


			sx = rastan_spriteram[offs+3] & 0x1ff;
			if (sx > 400) sx = sx - 512;
			sy = rastan_spriteram[offs+1] & 0x1ff;
			if (sy > 400) sy = sy - 512;

			data1 = rastan_spriteram[offs];

			col = (data1 + 0x10) & 0x7f;

			flipx = data1 & 0x4000;
			flipy = data1 & 0x8000;

			if (flipscreen)
			{
				flipx = !flipx;
				flipy = !flipy;
				sx = 320 - sx - 16;
				sy = 240 - sy;
			}


            if(num < 4096)
			    drawgfx(bitmap,Machine->gfx[1],
					    num,
					    col,
					    flipx, flipy,
					    sx,sy,
					    &Machine->visible_area,TRANSPARENCY_PEN,0);
            else
			    drawgfx(bitmap,Machine->gfx[2],
					    num-4096,
					    col,
					    flipx, flipy,
					    sx,sy,
					    &Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}

	scrollx = rastan_scrollx[1] - 16;
	scrolly = rastan_scrolly[1];
	if (flipscreen)
	{
		scrollx = 320-scrollx;
		scrolly = 240-scrolly+16;
	}
	copyscrollbitmap(bitmap,tmpbitmap3,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);


}


/* Jumping uses different sprite controller   */
/* than rainbow island. - values are remapped */
/* at address 0x2EA in the code. Apart from   */
/* physical layout, the main change is that   */
/* the Y settings are active low              */

void jumping_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int scrollx,scrolly;

    palette_init_used_colors();

    /* TODO: we are using the same table for background and foreground tiles, but this */
    /* causes the sky to be black instead of blue. */
    {
	    int color,code,i;
	    int colmask[128];
    	int pal_base;

	    pal_base = 0;

	    for (color = 0;color < 128;color++) colmask[color] = 0;

	    for (offs = rastan_videoram_size/2 - 2;offs >= 0;offs -= 2)
	    {
		    code  = rastan_videoram1[offs + 1] & 0x3FFF;
		    color = rastan_videoram1[offs] & 0x7f;

		    colmask[color] |= Machine->gfx[0]->pen_usage[code];
	    }

	    for (offs = 0x800/2-4; offs >= 0; offs -= 4)
	    {
		    code = rastan_spriteram[offs];

		    if (code < Machine->gfx[1]->total_elements)
		    {
			    int data1;

			    data1 = rastan_spriteram[offs+4];

			    color = (data1 + 0x10) & 0x7f;
			    colmask[color] |= Machine->gfx[1]->pen_usage[code];
		    }
	    }

	    for (offs = rastan_videoram_size/2 - 2;offs >= 0;offs -= 2)
	    {
		    code  = rastan_videoram3[offs + 1] & 0x3FFF;
		    color = rastan_videoram3[offs] & 0x7f;

		    colmask[color] |= Machine->gfx[0]->pen_usage[code];
	    }


	    for (color = 0;color < 128;color++)
	    {
		    if (colmask[color] & (1 << 15))
			    palette_used_colors[pal_base + 16 * color + 15] = PALETTE_COLOR_USED;

		    for (i = 0;i < 15;i++)
		    {
			    if (colmask[color] & (1 << i))
				    palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		    }
	    }

        /* Make one transparent colour */

        palette_used_colors[pal_base + 15] = PALETTE_COLOR_TRANSPARENT;

	    if (palette_recalc())
	    {
		    memset(rastan_dirty1,1,rastan_videoram_size / 4);
		    memset(rastan_dirty3,1,rastan_videoram_size / 4);
	    }

    }

	for (offs = rastan_videoram_size/2 - 2;offs >= 0;offs -= 2)
	{
		if (rastan_dirty1[offs/2])
		{
			int sx,sy;
			int data1,data2;

			rastan_dirty1[offs/2] = 0;

			data1 = rastan_videoram1[offs];
			data2 = rastan_videoram1[offs + 1];

			sx = (offs/2) % 64;
			sy = (offs/2) / 64;

			drawgfx(tmpbitmap1, Machine->gfx[0],
					data2,
					data1 & 0x7f,
					data1 & 0x4000, data1 & 0x8000,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	for (offs = rastan_videoram_size/2 - 2;offs >= 0;offs -= 2)
	{
		if (rastan_dirty3[offs/2])
		{
			int sx,sy;
			int data1,data2;


			rastan_dirty3[offs/2] = 0;

			data1 = rastan_videoram3[offs];
			data2 = rastan_videoram3[offs + 1];

			sx = (offs/2) % 64;
			sy = (offs/2) / 64;

            /* Colour as Transparent */

			drawgfx(tmpbitmap3, Machine->gfx[0],
					0,
					0,
					0, 0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);

            /* Draw over with correct Transparency */

			drawgfx(tmpbitmap3, Machine->gfx[0],
					data2,
					data1 & 0x7f,
					data1 & 0x4000, data1 & 0x8000,
					8*sx,8*sy,
					0,TRANSPARENCY_PEN,15);
		}
	}

	scrollx = rastan_scrollx[0] - 16;
	scrolly = -rastan_scrolly[0];
	copyscrollbitmap(bitmap,tmpbitmap1,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);

	/* Draw the sprites. 128 sprites in total */

	for (offs = 0x07F0/2; offs >= 0; offs -= 8)
	{
		int num = rastan_spriteram[offs];

		if (num)
		{
			int  sx,col,data1;
            int sy;

			sy = ((rastan_spriteram[offs+1] - 0xFFF1) ^ 0xFFFF) & 0x1FF;
  			if (sy > 400) sy = sy - 512;
			sx = (rastan_spriteram[offs+2] - 0x38) & 0x1ff;
			if (sx > 400) sx = sx - 512;

			data1 = rastan_spriteram[offs+3];
			col   = (rastan_spriteram[offs+4] + 0x10) & 0x7F;

			drawgfx(bitmap,Machine->gfx[1],
					num,
					col,
					data1 & 0x40, data1 & 0x80,
					sx,sy+1,
					&Machine->visible_area,TRANSPARENCY_PEN,15);
		}
	}

	scrollx = - 16;
	scrolly = 0;
  	copyscrollbitmap(bitmap,tmpbitmap3,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
}
