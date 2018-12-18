/* vidhrdw/namconb1.c */

#include "driver.h"
#include "vidhrdw/generic.h"
#include "namconb1.h"
#include "namcoic.h"
#include "namcos2.h"


/* tilemap_palette_bank is used to cache tilemap color, so that we can
 * mark whole tilemaps dirty only when necessary.
 */
static int tilemap_palette_bank[6];
static UINT8 *mpMaskData;
static struct tilemap *background[6];

/* nth_word32 is a general-purpose utility function, which allows us to
 * read from 32-bit aligned memory as if it were an array of 16 bit words.
 */
INLINE data16_t nth_word32( const data32_t *source, int which )
{
	source += which/2;
	if( which&1 )
	{
		return (*source)&0xffff;
	}
	else
	{
		return (*source)>>16;
	}
}

/* nth_byte32 is a general-purpose utility function, which allows us to
 * read from 32-bit aligned memory as if it were an array of bytes.
 */
INLINE data8_t
nth_byte32( const data32_t *pSource, int which )
{
		data32_t data = pSource[which/4];
		switch( which&3 )
		{
		case 0: return data>>24;
		case 1: return (data>>16)&0xff;
		case 2: return (data>>8)&0xff;
		default: return data&0xff;
		}
} /* nth_byte32 */

INLINE void tilemapNB1_get_info(int tile_index,int tilemap_color,const data32_t *tilemap_videoram)
{
	data16_t tile = nth_word32( tilemap_videoram, tile_index );
	SET_TILE_INFO(
			NAMCONB1_TILEGFX,
			tile,
			tilemap_color,
			0)
	tile_info.mask_data = 8*tile + mpMaskData;
}

static void tilemapNB1_get_info0(int tile_index) { tilemapNB1_get_info(tile_index,tilemap_palette_bank[0],&videoram32[0x0000]); }
static void tilemapNB1_get_info1(int tile_index) { tilemapNB1_get_info(tile_index,tilemap_palette_bank[1],&videoram32[0x0800]); }
static void tilemapNB1_get_info2(int tile_index) { tilemapNB1_get_info(tile_index,tilemap_palette_bank[2],&videoram32[0x1000]); }
static void tilemapNB1_get_info3(int tile_index) { tilemapNB1_get_info(tile_index,tilemap_palette_bank[3],&videoram32[0x1800]); }
static void tilemapNB1_get_info4(int tile_index) { tilemapNB1_get_info(tile_index,tilemap_palette_bank[4],&videoram32[NAMCONB1_FG1BASE/2]); }
static void tilemapNB1_get_info5(int tile_index) { tilemapNB1_get_info(tile_index,tilemap_palette_bank[5],&videoram32[NAMCONB1_FG2BASE/2]); }

WRITE32_HANDLER( namconb1_videoram_w )
{
	int layer;
	data32_t old_data;

	old_data = videoram32[offset];
	COMBINE_DATA( &videoram32[offset] );
	if( videoram32[offset]!=old_data )
	{
		offset*=2; /* convert dword offset to word offset */
		layer = offset/(64*64);
		if( layer<4 )
		{
			offset &= 0xfff;
			tilemap_mark_tile_dirty( background[layer], offset );
			tilemap_mark_tile_dirty( background[layer], offset+1 );
		}
		else
		{
			if( offset >= NAMCONB1_FG1BASE &&
				offset<NAMCONB1_FG1BASE+NAMCONB1_COLS*NAMCONB1_ROWS )
			{
				offset -= NAMCONB1_FG1BASE;
				tilemap_mark_tile_dirty( background[4], offset );
				tilemap_mark_tile_dirty( background[4], offset+1 );
			}
			else if( offset >= NAMCONB1_FG2BASE &&
				offset<NAMCONB1_FG2BASE+NAMCONB1_COLS*NAMCONB1_ROWS )
			{
				offset -= NAMCONB1_FG2BASE;
				tilemap_mark_tile_dirty( background[5], offset );
				tilemap_mark_tile_dirty( background[5], offset+1 );
			}
		}
	}
}

static void namconb1_install_palette( void )
{
	int pen, page, dword_offset, byte_offset;
	data32_t r,g,b;
	data32_t *pSource;

	/* this is unnecessarily expensive.  Better would be to mark palette entries dirty as
	 * they are modified, and only process those that have changed.
	 */
	pen = 0;
	for( page=0; page<4; page++ )
	{
		pSource = &paletteram32[page*0x2000/4];
		for( dword_offset=0; dword_offset<0x800/4; dword_offset++ )
		{
			r = pSource[dword_offset+0x0000/4];
			g = pSource[dword_offset+0x0800/4];
			b = pSource[dword_offset+0x1000/4];

			for( byte_offset=0; byte_offset<4; byte_offset++ )
			{
				palette_set_color( pen++, r>>24, g>>24, b>>24 );
				r<<=8; g<<=8; b<<=8;
			}
		}
	}
}

static void handle_mcu( void )
{
	static data16_t credits;
	static int old_coin_state;
	int new_coin_state = readinputport(3)&0x3; /* coin1,2 */

	/* MCU simulation.  It manages coinage, input ports, and presumably
	 * communication with the sound CPU.
	 */
	namconb1_workram32[0x6000/4] = ((0x80|readinputport(0))<<16)|(readinputport(1)<<8);
	namconb1_workram32[0x6004/4] = readinputport(2)<<24;

	if( new_coin_state && !old_coin_state )
	{
		credits++;
	}
	old_coin_state = new_coin_state;

	namconb1_workram32[0x601e/4] &= 0xffff0000;
	namconb1_workram32[0x601e/4] |= credits;
} /* handle_mcu */

static void
video_update_common( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int bROZ )
{
	const int xadjust[4] = { 0,2,3,4 };
	int i,pri;

	handle_mcu();
	namconb1_install_palette();
	fillbitmap(priority_bitmap,0,NULL); /* not actually used (yet) */

	/* I have no idea what the background color should be, but I doubt it ever pokes through. */
	fillbitmap( bitmap, 0, 0 );

	for( i=0; i<6; i++ )
	{
		int tilemap_color = nth_word32( &namconb1_scrollram32[0x30/4], i )&7;
		if( tilemap_palette_bank[i]!= tilemap_color )
		{
			tilemap_palette_bank[i] = tilemap_color;
			tilemap_mark_all_tiles_dirty( background[i] );
		}
		if( i<4 )
		{
			tilemap_set_scrollx( background[i],0,namconb1_scrollram32[i*2]+48-xadjust[i] );
			tilemap_set_scrolly( background[i],0,namconb1_scrollram32[i*2+1]+24 );
		}
	}

	for( pri=0; pri<8; pri++ )
	{
		if( bROZ )
		{
			namco_roz_draw( bitmap,cliprect,pri );
		}

		for( i=0; i<6; i++ )
		{
			if( nth_word32( &namconb1_scrollram32[0x20/4],i ) == pri )
			{
				tilemap_draw( bitmap,cliprect,background[i],0,0/*1<<pri*/ );
			}
		}
		namco_obj_draw( bitmap, pri );
	}
} /* video_update_common */

/************************************************************************************************/

VIDEO_UPDATE( namconb1 )
{
	int beamx,beamy;

	video_update_common( bitmap, cliprect, 0 );

	if( namconb1_type == key_gunbulet )
	{
		beamx = ((readinputport(4)) * 320)/256;
		beamy = readinputport(5);
		draw_crosshair( bitmap, beamx, beamy, cliprect );

		beamx = ((readinputport(6)) * 320)/256;
		beamy = readinputport(7);
		draw_crosshair( bitmap, beamx, beamy, cliprect );
	}
}

static int NB1objcode2tile( int code )
{
	int bank;
	bank = nth_word32( namconb1_spritebank32, code>>11 );
	return (code&0x7ff) + bank*0x800;
}

VIDEO_START( namconb1 )
{
	int i;
	static void (*get_info[6])(int tile_index) =
	{
		tilemapNB1_get_info0, tilemapNB1_get_info1, tilemapNB1_get_info2,
		tilemapNB1_get_info3, tilemapNB1_get_info4, tilemapNB1_get_info5
	};

	namcos2_gametype = NAMCONB1;
	namco_obj_init(NAMCONB1_SPRITEGFX,0x0,NB1objcode2tile);
	mpMaskData = (UINT8 *)memory_region( NAMCONB1_TILEMASKREGION );
	for( i=0; i<6; i++ )
	{
		if( i<4 )
		{
			background[i] = tilemap_create(
				get_info[i],
				tilemap_scan_rows,
				TILEMAP_BITMASK,8,8,64,64 );
		}
		else
		{
			background[i] = tilemap_create(
				get_info[i],
				tilemap_scan_rows,
				TILEMAP_BITMASK,8,8,NAMCONB1_COLS,NAMCONB1_ROWS );
		}

		if( background[i]==NULL ) return 1; /* error */

		tilemap_palette_bank[i] = -1;
	}

	/* Rotate the mask ROM if needed */
	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		/* borrowed from Namco SystemII */
		int loopX,loopY,tilenum;
		unsigned char tilecache[8],*tiledata;

		for(tilenum=0;tilenum<0x10000;tilenum++)
		{
			tiledata=memory_region(REGION_GFX3)+(tilenum*0x08);
			/* Cache tile data */
			for(loopY=0;loopY<8;loopY++) tilecache[loopY]=tiledata[loopY];
			/* Wipe source data */
			for(loopY=0;loopY<8;loopY++) tiledata[loopY]=0;
			/* Swap X/Y data */
			for(loopY=0;loopY<8;loopY++)
			{
				for(loopX=0;loopX<8;loopX++)
				{
					tiledata[loopX]|=(tilecache[loopY]&(0x01<<loopX))?(1<<loopY):0x00;
				}
			}
		}

		/* preprocess bitmask */
		for(tilenum=0;tilenum<0x10000;tilenum++){
			tiledata=memory_region(REGION_GFX3)+(tilenum*0x08);
			/* Cache tile data */
			for(loopY=0;loopY<8;loopY++) tilecache[loopY]=tiledata[loopY];
			/* Flip in Y - write back in reverse */
			for(loopY=0;loopY<8;loopY++) tiledata[loopY]=tilecache[7-loopY];
		}

		for(tilenum=0;tilenum<0x10000;tilenum++){
			tiledata=memory_region(REGION_GFX3)+(tilenum*0x08);
			/* Cache tile data */
			for(loopY=0;loopY<8;loopY++) tilecache[loopY]=tiledata[loopY];
			/* Wipe source data */
			for(loopY=0;loopY<8;loopY++) tiledata[loopY]=0;
			/* Flip in X - do bit reversal */
			for(loopY=0;loopY<8;loopY++)
			{
				for(loopX=0;loopX<8;loopX++)
				{
					tiledata[loopY]|=(tilecache[loopY]&(1<<loopX))?(0x80>>loopX):0x00;
				}
			}
		}
	}

	return 0; /* no error */
}

/****************************************************************************************************/

INLINE void
tilemapNB2_get_info(int tile_index,int which,const data32_t *tilemap_videoram)
{
	data16_t tile = nth_word32( tilemap_videoram, tile_index );
	int mangle;

	/* the pixmap index is mangled, the transparency bitmask index is not */
	mangle = tile&~(0x140);
	if( tile&0x100 ) mangle |= 0x040;
	if( tile&0x040 ) mangle |= 0x100;

	SET_TILE_INFO( NAMCONB1_TILEGFX,mangle,tilemap_palette_bank[which],0)
	tile_info.mask_data = 8*tile + mpMaskData;
} /* tilemapNB2_get_info */

static void tilemapNB2_get_info0(int tile_index) { tilemapNB2_get_info(tile_index,0,&videoram32[0x0000/4]); }
static void tilemapNB2_get_info1(int tile_index) { tilemapNB2_get_info(tile_index,1,&videoram32[0x2000/4]); }
static void tilemapNB2_get_info2(int tile_index) { tilemapNB2_get_info(tile_index,2,&videoram32[0x4000/4]); }
static void tilemapNB2_get_info3(int tile_index) { tilemapNB2_get_info(tile_index,3,&videoram32[0x6000/4]); }
static void tilemapNB2_get_info4(int tile_index) { tilemapNB2_get_info(tile_index,4,&videoram32[NAMCONB1_FG1BASE/2]); }
static void tilemapNB2_get_info5(int tile_index) { tilemapNB2_get_info(tile_index,5,&videoram32[NAMCONB1_FG2BASE/2]); }

VIDEO_UPDATE( namconb2 )
{
	video_update_common( bitmap, cliprect, 1 );
} /* namconb2_vh_screenrefresh */

static int NB2objcode2tile( int code )
{
	int bank;
	bank = nth_byte32( namconb1_spritebank32, (code>>11)&0xf );
	code &= 0x7ff;
	if( bank&0x01 ) code += 0x01*0x800;
	if( bank&0x02 ) code += 0x04*0x800;
	if( bank&0x04 ) code += 0x02*0x800;
	if( bank&0x08 ) code += 0x08*0x800;
	if( bank&0x10 ) code += 0x10*0x800;
	if( bank&0x40 ) code += 0x20*0x800;
	return code;
}

VIDEO_START( namconb2 )
{
	int i;
	static void (*get_info[6])(int tile_index) =
		{ tilemapNB2_get_info0, tilemapNB2_get_info1, tilemapNB2_get_info2,
		  tilemapNB2_get_info3, tilemapNB2_get_info4, tilemapNB2_get_info5 };

	namcos2_gametype = NAMCONB2;
	if( namco_roz_init(NAMCONB1_ROTGFX,NAMCONB1_ROTMASKREGION)!=0 ) return 1;

	namco_obj_init(NAMCONB1_SPRITEGFX,0x0,NB2objcode2tile);
	mpMaskData = (UINT8 *)memory_region( NAMCONB1_TILEMASKREGION );
	for( i=0; i<6; i++ )
	{
		if( i<4 )
		{
			background[i] = tilemap_create(
				get_info[i],
				tilemap_scan_rows,
				TILEMAP_BITMASK,8,8,64,64 );
		}
		else
		{
			background[i] = tilemap_create(
				get_info[i],
				tilemap_scan_rows,
				TILEMAP_BITMASK,8,8,NAMCONB1_COLS,NAMCONB1_ROWS );
		}

		if( background[i]==NULL ) return 1; /* error */
	}

	return 0;
} /* namconb2_vh_start */
