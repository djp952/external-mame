/*
 * vidhrdw/mystwarr.c - Konami "Pre-GX" video hardware (here there be dragons)
 *
 */

#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "machine/konamigx.h"

//static int scrolld[2][4][2] = {
// 	{{ 42-64, 16 }, {42-64, 16}, {42-64-2, 16}, {42-64-4, 16}},
// 	{{ 53-64, 16 }, {53-64, 16}, {53-64-2, 16}, {53-64-4, 16}}
//};

static int scrolld[2][4][2] = {
 	{{ 0, 0 }, {0, 0}, {0, 0}, {0, 0}},
 	{{ 0, 0 }, {0, 0}, {0, 0}, {0, 0}}
};

static int layer_colorbase[6];
static int oinprion, cbparam=0;
static int sprite_colorbase, sprite_subcolorbase, psac_colorbase, last_psac_colorbase, gametype;
static int roz_enable, roz_rombank;
static struct tilemap *ult_936_tilemap;


// Mystic Warriors requires tile based blending.
static void mystwarr_tile_callback(int layer, int *code, int *color)
{
	if (layer==1) {if ((*code&0xff00)+(*color)==0x4101) cbparam++; else cbparam--;} //* water hack (TEMPORARY)

	*color = layer_colorbase[layer] | (*color>>1 & 0x1f);
}

// for games with 5bpp tile data
static void game5bpp_tile_callback(int layer, int *code, int *color)
{
	*color = layer_colorbase[layer] | (*color>>1 & 0x1f);
}

// for games with 4bpp tile data
static void game4bpp_tile_callback(int layer, int *code, int *color)
{
	*color = layer_colorbase[layer] | (*color>>2 & 0x0f);
}

static void mystwarr_sprite_callback(int *code, int *color, int *priority)
{
	int c = *color;

	*color = sprite_colorbase | (c & 0x001f);
	*priority = c & 0x00f0;
}

static void metamrph_sprite_callback(int *code, int *color, int *priority)
{
	int c = *color;

	// Bit8 & 9 are effect attributes. It is not known whether the effects are handled by external logic.
	if ((c & 0x300) != 0x300)
		*color = sprite_colorbase | (c & 0x001f);
	else
		*color = sprite_subcolorbase;

	*priority  = (c & 0x00f0) >> 2;
}

static void gaiapols_sprite_callback(int *code, int *color, int *priority)
{
	int c = *color;

	*color = sprite_colorbase | (c>>4 & 0x20) | (c & 0x001f);
	*priority = c & 0x00e0;
}

static void martchmp_sprite_callback(int *code, int *color, int *priority)
{
	int c = *color;

	// Bit8 & 9 are effect attributes. It is not known whether the effects are handled by external logic.
	if ((c & 0x3ff) != 0x11f)
		*color = sprite_colorbase | (c & 0x001f);
	else
		*color = -1;

	if (oinprion & 0xf0)
		*priority = cbparam;  // use PCU2 internal priority
	else
		*priority = c & 0xf0; // use color implied priority
}



static void get_gai_936_tile_info(int tile_index)
{
	int tileno, colour;
	unsigned char *ROM = memory_region(REGION_GFX4);
	unsigned char *dat1 = ROM, *dat2 = ROM + 0x20000, *dat3 = ROM + 0x60000;

	tileno = dat3[tile_index] | ((dat2[tile_index]&0x3f)<<8);

	if (tile_index & 1)
		colour = (dat1[tile_index>>1]&0xf);
	else
		colour = ((dat1[tile_index>>1]>>4)&0xf);

	if (dat2[tile_index] & 0x80) colour |= 0x10;

	colour |= psac_colorbase << 4;

	SET_TILE_INFO(0, tileno, colour, 0)
}

VIDEO_START(gaiapols)
{
	K055555_vh_start();
	K054338_vh_start();

	gametype = 0;

	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 0, scrolld, game4bpp_tile_callback))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, 1, -61, -22, gaiapols_sprite_callback)) // stage2 brick walls
	{
		return 1;
	}

	K056832_set_LayerOffset(0, -1, -1);
	K056832_set_LayerOffset(1,  2,  0);
	K056832_set_LayerOffset(2,  4,  0);
	K056832_set_LayerOffset(3,  5,  0);

	K053936_wraparound_enable(0, 1);
	K053936GP_set_offset(0, -10,  0); // floor tiles in demo loop2 (Elaine vs. boss)

	ult_936_tilemap = tilemap_create(get_gai_936_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 512, 512);
	tilemap_set_transparent_pen(ult_936_tilemap, 0);

	if (konamigx_mixer_init(0)) return 1;

	return 0;
}

static void get_ult_936_tile_info(int tile_index)
{
	int tileno, colour;
	unsigned char *ROM = memory_region(REGION_GFX4);
	unsigned char *dat1 = ROM, *dat2 = ROM + 0x40000;

	tileno = dat2[tile_index] | ((dat1[tile_index]&0x1f)<<8);

	colour = psac_colorbase;

	SET_TILE_INFO(0, tileno, colour, (dat1[tile_index]&0x40) ? TILE_FLIPX : 0)
}

VIDEO_START(dadandrn)
{
	K055555_vh_start();
	K054338_vh_start();

	gametype = 1;

	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 0, scrolld, game5bpp_tile_callback))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, 0, -42, -22, gaiapols_sprite_callback))
	{
		return 1;
	}

	K056832_set_LayerOffset(0, -2+4, 0);
	K056832_set_LayerOffset(1,  0+4, 0);
	K056832_set_LayerOffset(2,  2+4, 0);
	K056832_set_LayerOffset(3,  3+4, 0);

	K053936_wraparound_enable(0, 1);
	K053936GP_set_offset(0, -8, 0); // Brainy's laser

	ult_936_tilemap = tilemap_create(get_ult_936_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16, 16, 512, 512);
	tilemap_set_transparent_pen(ult_936_tilemap, 0);

	konamigx_mixer_primode(1);

	if (konamigx_mixer_init(0)) return 1;

	return 0;
}

VIDEO_START(mystwarr)
{
	K055555_vh_start();
	K054338_vh_start();

	gametype = 0;

	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 0, scrolld, mystwarr_tile_callback))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, 0, -48, -24, mystwarr_sprite_callback))
	{
		return 1;
	}

	K056832_set_LayerOffset(0, -2-3, 0);
	K056832_set_LayerOffset(1,  0-3, 0);
	K056832_set_LayerOffset(2,  2-3, 0);
	K056832_set_LayerOffset(3,  3-3, 0);

	if (konamigx_mixer_init(0)) return 1;

	return 0;
}

VIDEO_START(metamrph)
{
	int rgn_250 = REGION_GFX3;

	gametype = 0;

	K055555_vh_start();
	K054338_vh_start();

	K053250_vh_start(1, &rgn_250);

	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 0, scrolld, game4bpp_tile_callback))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, 1, -51, -22, metamrph_sprite_callback))
	{
		return 1;
	}

	// other reference, floor at first boss
	K056832_set_LayerOffset(0, -2+4, 0); // text
	K056832_set_LayerOffset(1,  0+4, 0); // attract sea
	K056832_set_LayerOffset(2,  2+4, 0); // attract red monster in background of sea
	K056832_set_LayerOffset(3,  3+4, 0); // attract sky background to sea

	konamigx_mixer_primode(2);

	if (konamigx_mixer_init(0)) return 1;

	return 0;
}

VIDEO_START(viostorm)
{
	gametype = 0;

	K055555_vh_start();
	K054338_vh_start();

	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 0, scrolld, game4bpp_tile_callback))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, 1, -62, -23, metamrph_sprite_callback))
	{
		return 1;
	}

	K056832_set_LayerOffset(0, -2+1, 0);
	K056832_set_LayerOffset(1,  0+1, 0);
	K056832_set_LayerOffset(2,  2+1, 0);
	K056832_set_LayerOffset(3,  3+1, 0);

	if (konamigx_mixer_init(0)) return 1;

	return 0;
}

VIDEO_START(martchmp)
{
	gametype = 0;

	K055555_vh_start();
	K054338_vh_start();

	if (K056832_vh_start(REGION_GFX1, K056832_BPP_5, 0, scrolld, game5bpp_tile_callback))
	{
		return 1;
	}

	if (K055673_vh_start(REGION_GFX2, 0, -58, -23, martchmp_sprite_callback))
	{
		return 1;
	}

	K056832_set_LayerOffset(0, -2-4, 0);
	K056832_set_LayerOffset(1,  0-4, 0);
	K056832_set_LayerOffset(2,  2-4, 0);
	K056832_set_LayerOffset(3,  3-4, 0);

	if (konamigx_mixer_init(0)) return 1;

	return 0;
}



VIDEO_UPDATE(mystwarr)
{
	int i, old, blendmode=0;

	//* water hack (TEMPORARY)
	if (cbparam<0) cbparam=0; else if (cbparam>=32) blendmode=(1<<16|GXMIX_BLEND_FORCE)<<2;

	for (i = 0; i < 4; i++)
	{
		old = layer_colorbase[i];
		layer_colorbase[i] = K055555_get_palette_index(i)<<4;
		if( old != layer_colorbase[i] ) K056832_mark_plane_dirty(i);
	}

	sprite_colorbase = K055555_get_palette_index(4)<<5;
	sprite_subcolorbase = K055555_get_palette_index(5)<<5;

	konamigx_mixer(bitmap, cliprect, 0, 0, 0, 0, blendmode);
}

VIDEO_UPDATE(metamrph)
{
	int i, old;

	for (i = 0; i < 4; i++)
	{
		old = layer_colorbase[i];
		layer_colorbase[i] = K055555_get_palette_index(i)<<4;
		if (old != layer_colorbase[i]) K056832_mark_plane_dirty(i);
	}

	sprite_colorbase = K055555_get_palette_index(4)<<4;
	sprite_subcolorbase = K055555_get_palette_index(5)<<4;

	//K053250_draw(bitmap, cliprect, 0, 0, 0);

	konamigx_mixer(bitmap, cliprect, 0, 0, 0, 0, 0);
}

VIDEO_UPDATE(martchmp)
{
	int i, old, blendmode;

	for (i = 0; i < 4; i++)
	{
		old = layer_colorbase[i];
		layer_colorbase[i] = K055555_get_palette_index(i)<<4;
		if (old != layer_colorbase[i]) K056832_mark_plane_dirty(i);
	}

	sprite_colorbase = K055555_get_palette_index(4)<<5;
	sprite_subcolorbase = K055555_get_palette_index(5)<<5;

	cbparam = K055555_read_register(K55_PRIINP_8);
	oinprion = K055555_read_register(K55_OINPRI_ON);

	// Not quite right. Martial Champion uses undocumented PCU2 registers.
	blendmode = (oinprion==0xef && K055555_read_register(0x5a)) ? ((3<<16|GXMIX_BLEND_FORCE)<<2) : 0;

	konamigx_mixer(bitmap, cliprect, 0, 0, 0, 0, blendmode);
}



WRITE16_HANDLER(ddd_053936_enable_w)
{
	if (ACCESSING_MSB)
	{
		roz_enable = data & 0x0100;
		roz_rombank = (data & 0xc000)>>14;
	}
}

WRITE16_HANDLER(ddd_053936_clip_w)
{
	static data16_t clip;
	int old, clip_x, clip_y, size_x, size_y;
	int minx, maxx, miny, maxy;

	if (offset == 1)
	{
 		if (ACCESSING_MSB) K053936GP_clip_enable(0, data & 0x0100);
	}
	else
	{
		old = clip;
		COMBINE_DATA(&clip);
		if (clip != old)
		{
			clip_x = (clip & 0x003f) >> 0;
			clip_y = (clip & 0x0fc0) >> 6;
			size_x = (clip & 0x3000) >> 12;
			size_y = (clip & 0xc000) >> 14;

			switch (size_x)
			{
				case 0x3: size_x = 1; break;
				case 0x2: size_x = 2; break;
				default:  size_x = 4; break;
			}

			switch (size_y)
			{
				case 0x3: size_y = 1; break;
				case 0x2: size_y = 2; break;
				default:  size_y = 4; break;
			}

			minx = clip_x << 7;
			maxx = ((clip_x + size_x) << 7) - 1;
			miny = clip_y << 7;
			maxy = ((clip_y + size_y) << 7) - 1;

			K053936GP_set_cliprect(0, minx, maxx, miny, maxy);
		}
	}
}

// reference: 223e5c in gaiapolis (ROMs 34j and 36m)
READ16_HANDLER(gai_053936_tilerom_0_r)
{
	data8_t *ROM1 = (data8_t *)memory_region(REGION_GFX4);
	data8_t *ROM2 = (data8_t *)memory_region(REGION_GFX4);

	ROM1 += 0x20000;
	ROM2 += 0x20000+0x40000;

	return ((ROM1[offset]<<8) | ROM2[offset]);
}

READ16_HANDLER(ddd_053936_tilerom_0_r)
{
	data8_t *ROM1 = (data8_t *)memory_region(REGION_GFX4);
	data8_t *ROM2 = (data8_t *)memory_region(REGION_GFX4);

	ROM2 += 0x40000;

	return ((ROM1[offset]<<8) | ROM2[offset]);
}

// reference: 223e1a in gaiapolis (ROM 36j)
READ16_HANDLER(ddd_053936_tilerom_1_r)
{
	data8_t *ROM = (data8_t *)memory_region(REGION_GFX4);

	return ROM[offset/2];
}

// reference: 223db0 in gaiapolis (ROMs 32n, 29n, 26n)
READ16_HANDLER(gai_053936_tilerom_2_r)
{
	data8_t *ROM = (data8_t *)memory_region(REGION_GFX3);

	offset += (roz_rombank * 0x100000);

	return ROM[offset/2]<<8;
}

READ16_HANDLER(ddd_053936_tilerom_2_r)
{
	data8_t *ROM = (data8_t *)memory_region(REGION_GFX3);

	offset += (roz_rombank * 0x100000);

	return ROM[offset]<<8;
}

VIDEO_UPDATE(dadandrn) /* and gaiapols */
{
	int i, newbase, dirty, rozbpp, blendmode;

	if (gametype == 0)
	{
		sprite_colorbase = (K055555_get_palette_index(4)<<4)&0x7f;
		sprite_subcolorbase = (K055555_get_palette_index(5)<<4)&0x7f;
		rozbpp = 4;
	}
	else
	{
		sprite_colorbase = (K055555_get_palette_index(4)<<3)&0x7f;
		sprite_subcolorbase = (K055555_get_palette_index(5)<<3)&0x7f;
		rozbpp = 8;
	}

	if (K056832_get_LayerAssociation())
	{
		for (i=0; i<4; i++)
		{
			newbase = K055555_get_palette_index(i)<<4;
			if (layer_colorbase[i] != newbase)
			{
				layer_colorbase[i] = newbase;
				K056832_mark_plane_dirty(i);
			}
		}
	}
	else
	{
		for (dirty=0, i=0; i<4; i++)
		{
			newbase = K055555_get_palette_index(i)<<4;
			if (layer_colorbase[i] != newbase)
			{
				layer_colorbase[i] = newbase;
				dirty = 1;
			}
		}
		if (dirty) K056832_MarkAllTilemapsDirty();
	}

	last_psac_colorbase = psac_colorbase;
	psac_colorbase = K055555_get_palette_index(5);

	if (last_psac_colorbase != psac_colorbase)
	{
		tilemap_mark_all_tiles_dirty(ult_936_tilemap);
	}

	// background detail tuning
	switch (readinputport(6))
	{
		// Low : disable alpha on layer A to D, simulate translucency on sub layers
		case 0 : blendmode = 0x0a55; break;

		// Med : emulate alpha on layer A to D, simulate translucency on sub layers
		case 1 : blendmode = 0x0a00; break;

		// High: emulate alpha blending on all layers
		default: blendmode = 0;
	}

	// character detail tuning
	switch (readinputport(7))
	{
		// Low : disable shadows and turn off depth buffers
		case 0 : blendmode |= GXMIX_NOSHADOW + GXMIX_NOZBUF; break;

		// Med : only disable shadows
		case 1 : blendmode |= GXMIX_NOSHADOW; break;

		// High: enable all shadows and depth buffers
		default: blendmode |= 0;
	}

	konamigx_mixer(bitmap, cliprect, (roz_enable) ? ult_936_tilemap : 0, rozbpp, 0, 0, blendmode);
}
