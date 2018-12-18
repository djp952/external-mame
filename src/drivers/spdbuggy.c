/***************************************************************************

							Speed Buggy / Buggy Boy

							  (C) 1986 Data East
							  Developed by Tatsumi

				    driver by Luca Elia (eliavit@unina.it)

CPU   : AMD 8086 x 2	Z80
SOUND : 8253?	2 x 2149?
OSC.  : ?

Hardware-wise, it uses some funky stuff too. The sound system is pretty
standard... Z80, 8253 and two YM2149's, but I don't see anything I'm
familiar with CPU-wise. It's got two 40 pin NEC 70116 chips and a 40 pin
Hitachi 46505.

Test
1000-1003
write 1004-1005
6 words before 300
3 words before 00a
read word 3f00 & 300e
clear 1800-1eff

chcksum 0-3fff even:	5ac4 -> [670]
chcksum 0-3fff odd :	2c77 -> [672]

added together (=873b), subtracted [f840] (=87d9)

1000
1400	error text written ($34 words)

***************************************************************************/
#include "driver.h"

unsigned char *devram, *sharedram;

/* Variables that vidhrdw has access to */
int flipscreen;

/* Variables defined in vidhrdw */
unsigned char *spdbuggy_bgram, *spdbuggy_fgram;
unsigned char *spdbuggy_ram, *spdbuggy_ram2;

/* Functions defined in vidhrdw */
WRITE_HANDLER( spdbuggy_bgram_w );
WRITE_HANDLER( spdbuggy_fgram_w );

WRITE_HANDLER( spdbuggy_scrollregs_w );

int  spdbuggy_vh_start(void);
void spdbuggy_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);



/***************************************************************************


								Main CPU


***************************************************************************/

static READ_HANDLER ( sharedram_r )	{ return sharedram[offset]; }
static WRITE_HANDLER( sharedram_w )	{ sharedram[offset] = data; }

/*
	a9e.b	accel (00-70?)
	aa0.b	brake
	52a.b	steering

	aa6.b
		0	coin 1	(1276.b)
		1	coin 2	(1277.b)
		2	coin 3	(1278.b)

		6	start?
		7	?

	aa8.b
		0	shift
		7	service / coin3


[Main]
	8000-8fff	text ram (words)
	a400		shared with sub 1800

*/
READ_HANDLER( spdbuggy_ram_r )
{
	switch (offset)
	{
		case 0x0aa6:
		case 0x0aa7:
		case 0x0aa8:
		case 0x0aa9:	return 0xff;

		default:		return spdbuggy_ram[offset];
	}
}

// f002 read : watchdog reset
static MEMORY_READ_START ( spdbuggy_readmem )
	{ 0x00000, 0x023ff, MRA_RAM },

	{ 0x08000, 0x08fff, MRA_RAM },
	{ 0x0a000, 0x0afff, MRA_RAM },	// shared?
	{ 0x18000, 0x18fff, MRA_RAM },

	{ 0x10000, 0x17fff, MRA_ROM },
	{ 0x20000, 0x2ffff, MRA_ROM },
	{ 0xf0000, 0xfffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START ( spdbuggy_writemem )
	{ 0x00000, 0x023ff, MWA_RAM },

	{ 0x08000, 0x08fff, spdbuggy_fgram_w, &spdbuggy_fgram	},	// fg
	{ 0x18000, 0x18fff, spdbuggy_bgram_w, &spdbuggy_bgram	},	// bg
	{ 0x0a000, 0x0afff, MWA_RAM },	// shared?

	{ 0x10000, 0x17fff, MWA_ROM },
	{ 0x20000, 0x2ffff, MWA_ROM },
	{ 0xf0000, 0xfffff, MWA_ROM },
MEMORY_END






/***************************************************************************


									Sub CPU


***************************************************************************/


READ_HANDLER( spdbuggy_ram2_r )
{
	switch (offset)
	{
		case 0x3e00:
		case 0x3e01:
		{
			unsigned char *fn_rom = memory_region( REGION_USER1 );
			int bank = spdbuggy_ram2[0x3680] & 7;
			int fn_offs = (spdbuggy_ram2[0x3601]*256 + spdbuggy_ram2[0x3600])&0x7ff;

			return fn_rom[bank * 0x800 + fn_offs + 0x4000 * (offset&1)];
		}
		default:	return spdbuggy_ram2[offset];
	}
}


static MEMORY_READ_START ( spdbuggy_readmem2 )
	{ 0x0000, 0x7fff, spdbuggy_ram2_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START ( spdbuggy_writemem2 )
	{ 0x0000, 0x7fff, MWA_RAM, &spdbuggy_ram2 },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END






/***************************************************************************


								Input Ports


***************************************************************************/

INPUT_PORTS_START( spdbuggy )

	PORT_START	// IN0 - Player 1
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN1 - Player 2
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN2 - Service
	PORT_BIT(  0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT(  0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	// IN3 - $aa6 - DSW 1
	PORT_DIPNAME( 0x01, 0x01, "Unknown 1-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 1-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 1-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 1-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 1-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Unknown 1-5" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Unknown 1-6" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Unknown 1-7" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	// IN4 - $aa8 - DSW 2
	PORT_DIPNAME( 0x01, 0x01, "Unknown 2-0" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Unknown 2-1" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "Unknown 2-2" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Unknown 2-3" )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Unknown 2-4" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x60, "Copyright" )
	PORT_DIPSETTING(    0x60, "Tatsumi" )
	PORT_DIPSETTING(    0x40, "Taito" )
	PORT_DIPSETTING(    0x20, "Data East" )	// BUGGY BOY
	PORT_DIPSETTING(    0x00, "Tatsumi" )
	PORT_DIPNAME( 0x80, 0x80, "Coin Slots" )
	PORT_DIPSETTING(    0x80, "2" )
	PORT_DIPSETTING(    0x00, "3" )

INPUT_PORTS_END



/***************************************************************************


								Gfx Layouts


***************************************************************************/

#define LAYOUT_PLANAR_8x8x2(_name_,_romsize_) \
static struct GfxLayout _name_ =\
{\
	8,8,\
	(_romsize_)*8/(8*8*2),\
	2,\
	{ 0, ((_romsize_) / 2) * 8},\
	{0,1,2,3,4,5,6,7},\
	{0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8},\
	8*8\
};

#if 0
#define LAYOUT_CHUNKY_16x16x8(_name_,_romsize_) \
static struct GfxLayout _name_ =\
{\
	32,16,\
	(_romsize_)*8/(32*16*4),\
	4,\
	{0, 1, 2, 3},\
	{0*4,1*4,2*4,3*4,4*4,5*4,6*4,7*4, \
	 8*4,9*4,10*4,11*4,12*4,13*4,14*4,15*4, \
	 16*4,17*4,18*4,19*4,20*4,21*4,22*4,23*4, \
	 24*4,25*4,26*4,27*4,28*4,29*4,30*4,31*4}, \
	{0*128,1*128,2*128,3*128,4*128,5*128,6*128,7*128,\
	 8*128,9*128,10*128,11*128,12*128,13*128,14*128,15*128}, \
	32*16*4\
};
#else
#define LAYOUT_CHUNKY_16x16x8(_name_,_romsize_) \
static struct GfxLayout _name_ =\
{\
	64,64,\
	(_romsize_)*8/(64*16*4),\
	4,\
	{0, 1, ((_romsize_)/2)*8,((_romsize_)/2)*8+1 }, \
	{0*2,1*2,2*2,3*2,4*2,5*2,6*2,7*2, \
	 8*2,9*2,10*2,11*2,12*2,13*2,14*2,15*2, \
	 16*2,17*2,18*2,19*2,20*2,21*2,22*2,23*2, \
	 24*2,25*2,26*2,27*2,28*2,29*2,30*2,31*2, \
\
	 32*2,33*2,34*2,35*2,36*2,37*2,38*2,39*2, \
	 40*2,41*2,42*2,43*2,44*2,45*2,46*2,47*2, \
	 48*2,49*2,50*2,51*2,52*2,53*2,54*2,55*2, \
	 56*2,57*2,58*2,59*2,60*2,61*2,62*2,63*2}, \
\
	{0*128,0*128,0*128,0*128,1*128,1*128,1*128,1*128,\
	 2*128,2*128,2*128,2*128,3*128,3*128,3*128,3*128, \
	 4*128,4*128,4*128,4*128,5*128,5*128,5*128,5*128, \
	 6*128,6*128,6*128,6*128,7*128,7*128,7*128,7*128, \
\
	 8*128,8*128,8*128,8*128,9*128,9*128,9*128,9*128,\
	 10*128,10*128,10*128,10*128,11*128,11*128,11*128,11*128, \
	 12*128,12*128,12*128,12*128,13*128,13*128,13*128,13*128, \
	 14*128,14*128,14*128,14*128,15*128,15*128,15*128,15*128}, \
	64*16*2\
};

#endif

#if 0

static struct GfxLayout _name_ =\
{\
	16,16,\
	(_romsize_)*8/(16*16*8),\
	8,\
	{0, 1, 2, 3, 4, 5, 6, 7},\
	{0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8, \
	 8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8}, \
	{0*128,1*128,2*128,3*128,4*128,5*128,6*128,7*128,\
	 8*128,9*128,10*128,11*128,12*128,13*128,14*128,15*128}, \
	16*16*8\
};

#endif

LAYOUT_PLANAR_8x8x2( tilelayout, 0x30000 )
LAYOUT_PLANAR_8x8x2( charlayout, 0x08000 )

LAYOUT_CHUNKY_16x16x8( spritelayout,			 0x10000 )


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x000000, &tilelayout,   256*0, 16 }, // [0] bg
	{ REGION_GFX1, 0x030000, &charlayout,   256*1, 16 }, // [1] txt
	{ REGION_GFX1, 0x038000, &spritelayout, 256*2, 16 }, // [2] sprites
	{ -1 }
};


static struct MachineDriver machine_driver_spdbuggy =
{
	{
		{
			CPU_V20,
			4000000,	/* ?? */
			spdbuggy_readmem, spdbuggy_writemem, 0,0,
ignore_interrupt, 1,
		},
		{
			CPU_V20,
			4000000,	/* ?? */
			spdbuggy_readmem2, spdbuggy_writemem2, 0,0,
ignore_interrupt, 1
		}
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	256, 256, { 0, 256-1, 0, 256-1 },
	gfxdecodeinfo,
	256 * 3, 256 * 3,
	0,
	VIDEO_TYPE_RASTER,
	0,
	spdbuggy_vh_start,
	0,
	spdbuggy_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{ 0 },
	}
};



/***************************************************************************

  Game driver(s)

CPU AMD 8086 X2
SOUND ? MISSING SOUND ROM

Luca:
ic25.2                                          FIRST AND SECOND HALF IDENTICAL
ic26.3                                          FIRST AND SECOND HALF IDENTICAL
ic32.11                                                    1xxxxxxxxxxxx = 0xFF
ic95.4                                          BAD ADDRESS LINES (mask=000010)
ic162-30.bin                                    FIXED BITS (0000xxxx)
ic190-31.bin                                    FIXED BITS (0000xxxx)
ic189-15.bin                                    FIXED BITS (0000xxxx)
ic31-23.bin                                     FIXED BITS (0000xxxx)
ic39-24.bin                                     FIXED BITS (0000xxxx)
ic40-25.bin                                     FIXED BITS (0000xxxx)
ic41-26.bin                                     FIXED BITS (0000xxxx)
ic42-27.bin                                     FIXED BITS (00001xxx)
ic19-22.bin                                     FIXED BITS (0000xxxx)
ic162-30.bin            ic190-31.bin            IDENTICAL

(4 bits)
IC189-15 BIN           256
IC31-23  BIN           256
IC39-24  BIN           256
IC40-25  BIN           256
IC41-26  BIN           256
IC42-27  BIN           256
IC19-22  BIN           256
IC162-30 BIN         2.048\	Identical
IC190-31 BIN         2.048/

IC32     11          8.192

IC95     4          16.384	bad fn_data odd?
IC138    5          16.384	fn_data even

IC46     12         16.384	planar 8x8	(charset)
IC47     13         16.384	planar 8x8

IC139    14         32.768	chunky 16x16x8?
IC140    15         32.768	""
IC141    17         16.384	""

IC142    16         32.768	planar 8x8
IC145    20         32.768	planar 8x8

IC43     18         32.768	planar 8x8
IC146    21         32.768	planar 8x8

IC144    19         32.768	planar 8x8
IC147    22         32.768	planar 8x8

IC26     3          16.384	sub even
IC25     2          16.384	sub odd

IC213    8          32.768	main even
IC174    6          32.768	main odd
IC214    9          32.768	main even
IC175    7          32.768	main odd

IC225    10         16.384	main even???


***************************************************************************/

ROM_START( spdbuggy )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )		/* Main CPU Code */
	ROM_LOAD16_BYTE( "ic174.6",  0xf0001, 0x08000, 0x5e352d8d )
	ROM_LOAD16_BYTE( "ic213.8",  0xf0000, 0x08000, 0xabcc8ad2 )

	ROM_LOAD16_BYTE( "ic175.7",  0x20001, 0x08000, 0x40ce3930 )
	ROM_LOAD16_BYTE( "ic214.9",  0x20000, 0x08000, 0x92797c25 )

	ROM_LOAD16_BYTE( "ic225.10", 0x10000, 0x04000, 0x771af4e1 )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )		/* Sub CPU Code */
	ROM_LOAD16_BYTE( "ic25.2", 0x08001, 0x04000, 0x197beee2 )
	ROM_LOAD16_BYTE( "ic26.3", 0x08000, 0x04000, 0xe91d4eef )

	ROM_REGION( 0x010000, REGION_CPU3, 0 )		/* Sound CPU Code */

	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )	/* */
	ROM_LOAD( "ic142.16", 0x00000, 0x08000, 0x015db5d8 )	// 8x8 plane 1
	ROM_LOAD( "ic43.18",  0x08000, 0x08000, 0x876a5666 )
	ROM_LOAD( "ic144.19", 0x10000, 0x08000, 0x838e0697 )
	ROM_LOAD( "ic145.20", 0x18000, 0x08000, 0x11d8e2a8 )	// 8x8 plane 2
	ROM_LOAD( "ic146.21", 0x20000, 0x08000, 0x8b47d227 )
	ROM_LOAD( "ic147.22", 0x28000, 0x08000, 0x14033710 )

	/* halves swapped */
	ROM_LOAD( "ic46.12",  0x32000, 0x02000, 0x8ea8fec4 )	// 8x8 plane 1 (charset)
	ROM_CONTINUE(         0x30000, 0x02000             )
	ROM_LOAD( "ic47.13",  0x36000, 0x02000, 0x459c2b03 )	// 8x8 plane 2
	ROM_CONTINUE(         0x34000, 0x02000             )

	// 16x16x8? chunky
	ROM_LOAD( "ic140.15", 0x38000, 0x08000, 0x82cabdd4 )	//
	ROM_LOAD( "ic139.14", 0x40000, 0x08000, 0x1903a9ad )	// every bit
	ROM_LOAD( "ic141.17", 0x48000, 0x04000, 0x67786327 )

	ROM_REGION( 0x8000, REGION_USER1, 0 )	/* ? zoom table or something ? */
	ROM_LOAD( "ic138.5", 0x00000, 0x04000, 0x7d84135b )	// fn_data even
	ROM_LOAD( "ic95.4",  0x04000, 0x04000, 0x493ea590 )	// odd

ROM_END

GAME( 1986, spdbuggy, 0, spdbuggy, spdbuggy, 0, ROT0, "Tatsumi", "Speed Buggy" )






/***************************************************************************


							Speed Buggy / Buggy Boy

							  (C) 1986 Data East
							  Developed by Tatsumi

				    driver by Luca Elia (eliavit@unina.it)

**************************************************************************/
#include "vidhrdw/generic.h"

/* Variables only used here: */
static struct tilemap *bg_tilemap, *fg_tilemap;


/* Variables that driver has acces to: */



/* Variables defined in driver: */


/***************************************************************************
							Common routines
***************************************************************************/

/* Useful defines - for debug */
#define KEY(_k_,_action_) \
	if (keyboard_pressed(KEYCODE_##_k_))	{ while (keyboard_pressed(KEYCODE_##_k_)); _action_ }
#define KEY_SHIFT(_k_,_action_) \
	if ( (keyboard_pressed(KEYCODE_LSHIFT)||keyboard_pressed(KEYCODE_RSHIFT)) && \
	      keyboard_pressed(KEYCODE_##_k_) )	{ while (keyboard_pressed(KEYCODE_##_k_)); _action_ }
#define KEY_FAST(_k_,_action_) \
	if (keyboard_pressed(KEYCODE_##_k_))	{ _action_ }



/***************************************************************************

					  Callbacks for the TileMap code

***************************************************************************/

/*------------------------------------------------------------------------
							[ Background ]
------------------------------------------------------------------------*/

#define BG_NX  (0x40)
#define BG_NY  (0x40)
#define BG_GFX (0)

static void spdbuggy_get_bg_tile_info( int tile_index )
{
	int code 		=	spdbuggy_bgram[tile_index*2] + spdbuggy_bgram[tile_index*2+1]*256;
	SET_TILE_INFO(BG_GFX, code & 0x0fff, code >> 12, 0 );	// $3000 tiles!
}

WRITE_HANDLER( spdbuggy_bgram_w )
{
	if (data != spdbuggy_bgram[offset]);
	{
		spdbuggy_bgram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset / 2);
	}
}



/*------------------------------------------------------------------------
							[ Foreground ]
------------------------------------------------------------------------*/

#define FG_NX  (0x40)
#define FG_NY  (0x40)
#define FG_GFX (1)

static void spdbuggy_get_fg_tile_info( int tile_index )
{
	int code 		=	spdbuggy_fgram[tile_index*2] + spdbuggy_fgram[tile_index*2+1]*256;
	SET_TILE_INFO(FG_GFX, code & 0x07ff, code >> 12, 0 );
}

WRITE_HANDLER( spdbuggy_fgram_w )
{
	if (data != spdbuggy_fgram[offset]);
	{
		spdbuggy_fgram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset / 2);
	}
}





/*------------------------------------------------------------------------
						[ Video Hardware Start ]
------------------------------------------------------------------------*/

int spdbuggy_vh_start(void)
{
	bg_tilemap = tilemap_create(spdbuggy_get_bg_tile_info,
								tilemap_scan_rows,
								TILEMAP_OPAQUE,
								8,8,
								BG_NX,BG_NY );

	fg_tilemap = tilemap_create(spdbuggy_get_fg_tile_info,
								tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								8,8,
								FG_NX,FG_NY );

	if ( bg_tilemap && fg_tilemap )
	{
		tilemap_set_scroll_rows(bg_tilemap,1);
		tilemap_set_scroll_cols(bg_tilemap,1);

		tilemap_set_scroll_rows(fg_tilemap,1);
		tilemap_set_scroll_cols(fg_tilemap,1);

		tilemap_set_transparent_pen(fg_tilemap,0);

		return 0;
	}
	else return 1;
}


/***************************************************************************

							Sprites Drawing

***************************************************************************/

static void draw_sprites(struct mame_bitmap *bitmap)
{
}

/***************************************************************************

							Screen Drawing

***************************************************************************/




void spdbuggy_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh)
{
	int layers_ctrl = 0xFFFF;

#if 0
	unsigned char *RAM = memory_region( REGION_CPU1 );

	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);
	for (i = 0; i < 0x1000; i+=2)
	{
		drawgfx(bitmap,Machine->gfx[1],
				RAM[0x8000+i],
				0,
				0, 0,
				((i/2)%FG_NX)*8,((i/2)/FG_NX)*8,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	return;
#endif

#ifdef MAME_DEBUG
{
	if (keyboard_pressed(KEYCODE_Z))
	{
		int msk = 0;
		if (keyboard_pressed(KEYCODE_Q))	{ msk |= 1;}
		if (keyboard_pressed(KEYCODE_W))	{ msk |= 2;}
		if (keyboard_pressed(KEYCODE_E))	{ msk |= 4;}
		if (keyboard_pressed(KEYCODE_A))	{ msk |= 8;}
		if (keyboard_pressed(KEYCODE_R))	{ msk |= 16;}
		if (msk != 0) layers_ctrl &= msk;
	}
}
#endif


	/* Draw the background */
	if (layers_ctrl & 1)	tilemap_draw(bitmap, bg_tilemap,  0, 0);
	else					fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	/* Draw the sprites */
	if (layers_ctrl & 8)	draw_sprites(bitmap);

	/* Draw the foreground (text) */
	if (layers_ctrl & 4)	tilemap_draw(bitmap, fg_tilemap,  0, 0);
}
