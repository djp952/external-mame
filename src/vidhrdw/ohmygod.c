#include "driver.h"
#include "vidhrdw/generic.h"


data16_t *ohmygod_videoram;

static int spritebank;
static struct tilemap *bg_tilemap;



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_tile_info(int tile_index)
{
	UINT16 code = ohmygod_videoram[2*tile_index+1];
	UINT16 attr = ohmygod_videoram[2*tile_index];
	SET_TILE_INFO(
			0,
			code,
			(attr & 0x0f00) >> 8,
			0)
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int ohmygod_vh_start(void)
{
	bg_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,64);

	if (!bg_tilemap)
		return 1;

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE16_HANDLER( ohmygod_videoram_w )
{
	int oldword = ohmygod_videoram[offset];
	COMBINE_DATA(&ohmygod_videoram[offset]);
	if (oldword != ohmygod_videoram[offset])
		tilemap_mark_tile_dirty(bg_tilemap,offset/2);
}

WRITE16_HANDLER( ohmygod_spritebank_w )
{
	if (ACCESSING_MSB)
		spritebank = data & 0x8000;
}

WRITE16_HANDLER( ohmygod_scrollx_w )
{
	static data16_t scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrollx(bg_tilemap,0,scroll - 0x81ec);
}

WRITE16_HANDLER( ohmygod_scrolly_w )
{
	static data16_t scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrolly(bg_tilemap,0,scroll - 0x81ef);
}


/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;

	for (offs = 0;offs < spriteram_size/4;offs += 4)
	{
		int sx,sy,code,color,flipx;
		data16_t *sr;

		sr = spritebank ? (spriteram16+spriteram_size/4) : spriteram16;

		code = sr[offs+3] & 0x0fff;
		color = sr[offs+2] & 0x000f;
		sx = sr[offs+0] - 29;
		sy = sr[offs+1];
		if (sy >= 32768) sy -= 65536;
		flipx = sr[offs+3] & 0x8000;

		drawgfx(bitmap,Machine->gfx[1],
				code,
				color,
				flipx,0,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

void ohmygod_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_update(ALL_TILEMAPS);

	palette_recalc();

	tilemap_draw(bitmap,bg_tilemap,0,0);
	draw_sprites(bitmap);
}
