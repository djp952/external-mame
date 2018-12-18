/***************************************************************************

	Legion video hardware (copied from D-Con)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

data16_t *legionna_back_data,*legionna_fore_data,*legionna_mid_data,*legionna_scrollram16,*legionna_textram;

static struct tilemap *background_layer,*foreground_layer,*midground_layer,*text_layer;
//static int legionna_enable;

/******************************************************************************/

WRITE16_HANDLER( legionna_control_w )
{
#if 0
	if (ACCESSING_LSB)
	{
		legionna_enable=data;
		if ((legionna_enable&4)==4)
			tilemap_set_enable(foreground_layer,0);
		else
			tilemap_set_enable(foreground_layer,1);

		if ((legionna_enable&2)==2)
			tilemap_set_enable(midground_layer,0);
		else
			tilemap_set_enable(midground_layer,1);

		if ((legionna_enable&1)==1)
			tilemap_set_enable(background_layer,0);
		else
			tilemap_set_enable(background_layer,1);
	}
#endif
}

WRITE16_HANDLER( legionna_background_w )
{
	int oldword = legionna_back_data[offset];
	COMBINE_DATA(&legionna_back_data[offset]);
	if (oldword != legionna_back_data[offset])
		tilemap_mark_tile_dirty(background_layer,offset);
}

WRITE16_HANDLER( legionna_midground_w )
{
	int oldword = legionna_mid_data[offset];
	COMBINE_DATA(&legionna_mid_data[offset]);
	if (oldword != legionna_mid_data[offset])
		tilemap_mark_tile_dirty(midground_layer,offset);
}

WRITE16_HANDLER( legionna_foreground_w )
{
	int oldword = legionna_fore_data[offset];
	COMBINE_DATA(&legionna_fore_data[offset]);
	if (oldword != legionna_fore_data[offset])
		tilemap_mark_tile_dirty(foreground_layer,offset);
}

WRITE16_HANDLER( legionna_text_w )
{
	int oldword = legionna_textram[offset];
	COMBINE_DATA(&legionna_textram[offset]);
	if (oldword != legionna_textram[offset])
		tilemap_mark_tile_dirty(text_layer,offset);
}

static void get_back_tile_info(int tile_index)
{
	int tile=legionna_back_data[tile_index];
	int color=(tile>>12)&0xf;

	tile &= 0xfff;

	SET_TILE_INFO(1,tile,color,0)
}

static void get_mid_tile_info(int tile_index)
{
	int tile=legionna_mid_data[tile_index];
	int color=(tile>>12)&0xf;

	tile &= 0xfff;
	tile |= 0x1000;

	SET_TILE_INFO(3,tile,color,0)
}

static void get_fore_tile_info(int tile_index)	/* this is giving bad tiles... */
{
	int tile=legionna_fore_data[tile_index];
	int color=(tile>>12)&0xf;

	// tile numbers / gfx set wrong, see screen after coin insertion
	tile &= 0xfff;
	tile |= 0x1000;

	SET_TILE_INFO(3,tile,color,0)	// gfx 1 ???
}

static void get_text_tile_info(int tile_index)
{
	int tile = legionna_textram[tile_index];
	int color=(tile>>12)&0xf;

	tile&=0x7ff;

	SET_TILE_INFO(0,tile,color,0)
}

int legionna_vh_start(void)
{
	background_layer = tilemap_create(get_back_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	foreground_layer = tilemap_create(get_fore_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	midground_layer =  tilemap_create(get_mid_tile_info, tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	text_layer =       tilemap_create(get_text_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,  8,8,64,32);

	if (!background_layer || !foreground_layer || !midground_layer || !text_layer)
		return 1;

	legionna_scrollram16 = malloc(0x60);

	if (!legionna_scrollram16)	return 1;

	tilemap_set_transparent_pen(background_layer,15);
	tilemap_set_transparent_pen(midground_layer,15);
	tilemap_set_transparent_pen(foreground_layer,15);
	tilemap_set_transparent_pen(text_layer,15);

	return 0;
}

void legionna_vh_stop(void)
{
	free (legionna_scrollram16);
	legionna_scrollram16 = 0;
}


/*************************************************************************

	Legionnaire Spriteram (similar to Dcon)
	---------------------

	It has "big sprites" created by setting width or height >0. Tile
	numbers are read consecutively.

      +0   x....... ........  Sprite enable
	+0   .x...... ........  Flip x
	+0   ..x..... ........  Flip y ???
	+0   ...xxx.. ........  Width: do this many tiles horizontally
	+0   ......xx x.......  Height: do this many tiles vertically
	+0   ........ .?......  unused ?
 	+0   ........ ..xxxxxx  Color bank

	+1   .x...... ........  Priority? (1=high?)
	+1   ..xxxxxx xxxxxxxx  Tile number
	+2   xxxxxxxx xxxxxxxx  X coordinate (signed)
	+3   xxxxxxxx xxxxxxxx  Y coordinate (signed)

*************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap,int pri)
{
	int offs,fx,fy,x,y,color,sprite;
	int dx,dy,ax,ay;

	for (offs = 0x400-4;offs >= 0;offs -= 4)
	{
		UINT16 data = spriteram16[offs];
		if (!(data &0x8000)) continue;

		sprite = spriteram16[offs+1];
		if ((sprite>>14)!=pri) continue;

		sprite &= 0x3fff;

		y = spriteram16[offs+3];
		x = spriteram16[offs+2];

		if (x &0x8000)	x = -(0x200-(x &0x1ff));
		else	x &= 0x1ff;
		if (y &0x8000)	y = -(0x200-(y &0x1ff));
		else	y &= 0x1ff;

		color = (data &0x3f) + 0x40;
		fx =  (data &0x4000) >> 14;
		fy =  (data &0x2000) >> 13;	/* ??? */
		dy = ((data &0x0380) >> 7)  + 1;
		dx = ((data &0x1c00) >> 10) + 1;

		if (!fx)
		{
			for (ax=0; ax<dx; ax++)
				for (ay=0; ay<dy; ay++)
				{
					drawgfx(bitmap,Machine->gfx[4],
					sprite++,
					color,fx,fy,x+ax*16,y+ay*16,
					&Machine->visible_area,TRANSPARENCY_PEN,15);
				}
		}
		else
		{
			for (ax=0; ax<dx; ax++)
				for (ay=0; ay<dy; ay++)
				{
					drawgfx(bitmap,Machine->gfx[4],
					sprite++,
					color,fx,fy,x+(dx-ax-1)*16,y+ay*16,
					&Machine->visible_area,TRANSPARENCY_PEN,15);
				}
		}
	}
}


void legionna_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
#ifdef MAME_DEBUG
	static int dislayer[5];	/* Layer toggles to help get the layers correct */
	char buf[80];
#endif

#ifdef MAME_DEBUG
	if (keyboard_pressed_memory (KEYCODE_Z))
	{
		dislayer[0] ^= 1;
		sprintf(buf,"bg0: %01x",dislayer[0]);
		usrintf_showmessage(buf);
	}

	if (keyboard_pressed_memory (KEYCODE_X))
	{
		dislayer[1] ^= 1;
		sprintf(buf,"bg1: %01x",dislayer[1]);
		usrintf_showmessage(buf);
	}

	if (keyboard_pressed_memory (KEYCODE_C))
	{
		dislayer[2] ^= 1;
			sprintf(buf,"bg2: %01x",dislayer[2]);
		usrintf_showmessage(buf);
	}

	if (keyboard_pressed_memory (KEYCODE_V))
	{
		dislayer[3] ^= 1;
		sprintf(buf,"sprites: %01x",dislayer[3]);
		usrintf_showmessage(buf);
	}

	if (keyboard_pressed_memory (KEYCODE_B))
	{
		dislayer[4] ^= 1;
		sprintf(buf,"text: %01x",dislayer[4]);
		usrintf_showmessage(buf);
	}
#endif

	/* Setup the tilemaps */
	tilemap_set_scrollx( background_layer, 0, legionna_scrollram16[0] );
	tilemap_set_scrolly( background_layer, 0, legionna_scrollram16[1] );
	tilemap_set_scrollx( midground_layer,  0, legionna_scrollram16[2] );
	tilemap_set_scrolly( midground_layer,  0, legionna_scrollram16[3] );
	tilemap_set_scrollx( foreground_layer, 0, legionna_scrollram16[4] );
	tilemap_set_scrolly( foreground_layer, 0, legionna_scrollram16[5] );

//	if ((legionna_enable&1)!=1)

	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);	/* wrong color? */

#ifdef MAME_DEBUG
	if (dislayer[1]==0)
#endif
	tilemap_draw(bitmap,midground_layer,TILEMAP_IGNORE_TRANSPARENCY,0);

#ifdef MAME_DEBUG
	if (dislayer[0]==0)
#endif
	tilemap_draw(bitmap,background_layer,0,0);

#ifdef MAME_DEBUG
	if (dislayer[2]==0)
#endif
	tilemap_draw(bitmap,foreground_layer,0,0);
	draw_sprites(bitmap,2);
	draw_sprites(bitmap,1);
	draw_sprites(bitmap,0);
	draw_sprites(bitmap,3);

#ifdef MAME_DEBUG
	if (dislayer[4]==0)
#endif
	tilemap_draw(bitmap,text_layer,0,0);
}
