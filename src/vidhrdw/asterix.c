#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"


static int sprite_colorbase;
static int layer_colorbase[4], layerpri[3];
static data16_t spritebank;
static int tilebanks[4];
static int spritebanks[4];

void reset_spritebank(void)
{
	K053244_bankselect(spritebank & 7);
	spritebanks[0] = (spritebank << 12) & 0x7000;
	spritebanks[1] = (spritebank <<  9) & 0x7000;
	spritebanks[2] = (spritebank <<  6) & 0x7000;
	spritebanks[3] = (spritebank <<  3) & 0x7000;
}

WRITE16_HANDLER( asterix_spritebank_w )
{
	COMBINE_DATA(&spritebank);
	reset_spritebank();
}

static void asterix_sprite_callback(int *code, int *color, int *priority_mask)
{
	int pri = (*color & 0x00e0) >> 2;
	if (pri <= layerpri[2])								*priority_mask = 0;
	else if (pri > layerpri[2] && pri <= layerpri[1])	*priority_mask = 0xf0;
	else if (pri > layerpri[1] && pri <= layerpri[0])	*priority_mask = 0xf0|0xcc;
	else 												*priority_mask = 0xf0|0xcc|0xaa;
	*color = sprite_colorbase | (*color & 0x001f);
	*code = (*code & 0xfff) | spritebanks[(*code >> 12) & 3];
}


static void asterix_tile_callback(int layer, int *code, int *color)
{
	tile_info.flags = *code & 0x1000 ? TILE_FLIPX : 0;
	*color = (layer_colorbase[layer] + ((*code & 0xe000) >> 13)) & 0x7f;
	*code = (*code & 0x03ff) | tilebanks[(*code >> 10) & 3];
}

static int scrolld[2][4][2] = {
 	{{ 23-112, 0 }, { 23-112, 0}, { 23-112, 0}, { 23-112, 0}},
 	{{-73-112, 0 }, {-73-112, 0}, {-73-112, 0}, {-73-112, 0}}
};

int asterix_vh_start(void)
{
	K054157_vh_start(REGION_GFX1, 0, scrolld, NORMAL_PLANE_ORDER, asterix_tile_callback);
	if (K053245_vh_start(REGION_GFX2,NORMAL_PLANE_ORDER, asterix_sprite_callback))
	{
		K054157_vh_stop();
		return 1;
	}
	return 0;
}

void asterix_vh_stop(void)
{
	K054157_vh_stop();
	K053245_vh_stop();
}

/* useful function to sort the three tile layers by priority order */
static void sortlayers(int *layer,int *pri)
{
#define SWAP(a,b) \
	if (pri[a] < pri[b]) \
	{ \
		int t; \
		t = pri[a]; pri[a] = pri[b]; pri[b] = t; \
		t = layer[a]; layer[a] = layer[b]; layer[b] = t; \
	}

	SWAP(0,1)
	SWAP(0,2)
	SWAP(1,2)
}

void asterix_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int layer[3];
	int new_base;

	tilebanks[0] = (K054157_get_lookup(0) << 10);
	tilebanks[1] = (K054157_get_lookup(1) << 10);
	tilebanks[2] = (K054157_get_lookup(2) << 10);
	tilebanks[3] = (K054157_get_lookup(3) << 10);

	new_base = K053251_get_palette_index(K053251_CI0);
	if(layer_colorbase[0] != new_base) {
		layer_colorbase[0] = new_base;
		K054157_mark_plane_dirty(0);
	}

	new_base = K053251_get_palette_index(K053251_CI2);
	if(layer_colorbase[1] != new_base) {
		layer_colorbase[1] = new_base;
		K054157_mark_plane_dirty(1);
	}

	new_base = K053251_get_palette_index(K053251_CI3);
	if(layer_colorbase[2] != new_base) {
		layer_colorbase[2] = new_base;
		K054157_mark_plane_dirty(2);
	}

	new_base = K053251_get_palette_index(K053251_CI4);
	if(layer_colorbase[3] != new_base) {
		layer_colorbase[3] = new_base;
		K054157_mark_plane_dirty(3);
	}

	sprite_colorbase = K053251_get_palette_index(K053251_CI1);

	K054157_tilemap_update();

	palette_init_used_colors();
	K053245_mark_sprites_colors();

	palette_recalc();

	layer[0] = 0;
	layerpri[0] = K053251_get_priority(K053251_CI0);
	layer[1] = 1;
	layerpri[1] = K053251_get_priority(K053251_CI2);
	layer[2] = 3;
	layerpri[2] = K053251_get_priority(K053251_CI4);

	sortlayers(layer, layerpri);

	fillbitmap(priority_bitmap, 0, NULL);
	fillbitmap(bitmap, Machine->pens[0], &Machine->visible_area);

	K054157_tilemap_draw(bitmap, layer[0], 0, 1);
	K054157_tilemap_draw(bitmap, layer[1], 0, 2);
	K054157_tilemap_draw(bitmap, layer[2], 0, 4);

	K053245_sprites_draw(bitmap);

	K054157_tilemap_draw(bitmap, 2, 0, 0);
}
