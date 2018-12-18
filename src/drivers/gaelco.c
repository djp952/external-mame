/***************************************************************************

Driver by Manuel Abadia <manu@teleline.es>

Gaelco Game list:
=================

1987:	Master Boy
1991:	Big Karnak, Master Boy 2
1992:	Splash!**, Thunder Hoop, Squash
1993:	World Rally*, Glass**
1994:	Strike Back, Target Hits*, Thunder Hoop 2
1995:	Alligator Hunt, Mechanical Toy***, World Rally 2*, Salter, Touch & Go
1996:	Maniac Square****, Snow Board**, Speed Up (3D)
1997:	Surf Planet (3D)
1998:	Radikal Bikers (3D), Bang
1999:	Rolling Extreme (3D)
2000:	Football Power* (3D)

(*)		Created by Zigurat Software
(**)	Created by OMK Software
(***)	Created by Zeus Software
(****)	Created by Mandragora Soft

All games newer than Splash are heavily protected:
	World Rally, Squash, Thunder Hoop (and probably others) have encrypted Video RAM.
	World Rally (and probably others) has a protected MCU.


============================================================================
							SPLASH!
============================================================================

Splash! memory map:
-------------------
0x000000-0x03ffff	ROM (m68000 code + graphics)
0x100000-0x3fffff	ROM (graphics)
0x800000-0x83ffff	Screen 2	(pixel layer				(512x256))
0x840000-0x840001	Dipsw #1
0x840002-0x840003	Dipsw #2
0x840004-0x840005	Input #1
0x840006-0x840007	Input #2
0x84000e-0x84000f	Sound command
0x880000-0x880fff	Screen 0	(8x8 tiles		64x32		(512x256))
0x881000-0x8817ff	Screen 1	(16x16 tiles	32x32		(512x512))
0x881800-0x881801	Screen 0 scroll registers
0x881802-0x881803	Screen 1 scroll registers
0x881804-0x881fff	Work RAM (1/2)
0x8c0000-0x8c0fff	Palette (xRRRRxGGGGxBBBBx)
0x900000-0x900fff	Sprite RAM
0xffc000-0xffffff	Work RAM (2/2)

Interrupts:
	Level 6 INT generated by VBLANK

Unmapped addresses in the driver:

0x84000a-0x84000b\
0x84001a-0x84001b |	These registers seems to be used by the time
0x84002a-0x84002b |	measurement system for coin detector
0x84003a-0x84003b/

0x84007a-0x84007b	Set to 0 when clearing the pixel layer.
					After that is set to 0xffff.


In the Z80, what does $e000 do?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "cpu/m68000/m68000.h"

extern unsigned char *splash_vregs;
extern unsigned char *splash_videoram;
extern unsigned char *splash_spriteram;
extern unsigned char *splash_pixelram;

/* from vidhrdw/gaelco.c */
READ_HANDLER( splash_vram_r );
READ_HANDLER( splash_pixelram_r );
WRITE_HANDLER( splash_vram_w );
WRITE_HANDLER( splash_pixelram_w );
int splash_vh_start( void );
void splash_vh_screenrefresh( struct osd_bitmap *bitmap,int full_refresh );
WRITE_HANDLER( paletteram_xRRRRxGGGGxBBBBx_word_w );


static WRITE_HANDLER( splash_sh_irqtrigger_w )
{
	if ((data & 0x00ff0000) == 0)
	{
		soundlatch_w(0,data & 0xff);
		cpu_cause_interrupt(1,Z80_IRQ_INT);
	}
}


static struct MemoryReadAddress splash_readmem[] =
{
	{ 0x000000, 0x3fffff, MRA_ROM },			/* ROM */
	{ 0x800000, 0x83ffff, splash_pixelram_r },	/* Pixel Layer */
	{ 0x840000, 0x840001, input_port_0_r },		/* DIPSW #1 */
	{ 0x840002, 0x840003, input_port_1_r },		/* DIPSW #2 */
	{ 0x840004, 0x840005, input_port_2_r },		/* INPUT #1 */
	{ 0x840006, 0x840007, input_port_3_r },		/* INPUT #2 */
	{ 0x880000, 0x8817ff, splash_vram_r },		/* Video RAM */
	{ 0x881800, 0x881803, MRA_BANK1 },			/* Scroll registers */
	{ 0x881804, 0x881fff, MRA_BANK2 },			/* Work RAM */
	{ 0x8c0000, 0x8c0fff, paletteram_word_r },	/* Palette */
	{ 0x900000, 0x900fff, MRA_BANK3 },			/* Sprite RAM */
	{ 0xffc000, 0xffffff, MRA_BANK4 },			/* Work RAM */
	{ -1 }
};

static struct MemoryWriteAddress splash_writemem[] =
{
	{ 0x000000, 0x3fffff, MWA_ROM },										/* ROM */
	{ 0x800000, 0x83ffff, splash_pixelram_w, &splash_pixelram },			/* Pixel Layer */
	{ 0x84000e, 0x84000f, splash_sh_irqtrigger_w },							/* Sound command */
	{ 0x880000, 0x8817ff, splash_vram_w, &splash_videoram },				/* Video RAM */
	{ 0x881800, 0x881803, MWA_BANK1, &splash_vregs },						/* Scroll registers */
	{ 0x881804, 0x881fff, MWA_BANK2 },										/* Work RAM */
	{ 0x8c0000, 0x8c0fff, paletteram_xRRRRxGGGGxBBBBx_word_w, &paletteram },/* Palette */
	{ 0x900000, 0x900fff, MWA_BANK3, &splash_spriteram },					/* Sprite RAM */
	{ 0xffc000, 0xffffff, MWA_BANK4 },										/* Work RAM */
	{ -1 }
};


static struct MemoryReadAddress splash_readmem_sound[] =
{
	{ 0x0000, 0xd7ff, MRA_ROM },					/* ROM */
	{ 0xe800, 0xe800, soundlatch_r },				/* Sound latch */
	{ 0xf000, 0xf000, YM3812_status_port_0_r },		/* YM3812 */
	{ 0xf800, 0xffff, MRA_RAM },					/* RAM */
	{ -1 }
};

static int adpcm_data;

static WRITE_HANDLER( splash_adpcm_data_w ){
	adpcm_data = data;
}

static void splash_msm5205_int(int data)
{
	MSM5205_data_w(0,adpcm_data >> 4);
	adpcm_data = (adpcm_data << 4) & 0xf0;
}


static struct MemoryWriteAddress splash_writemem_sound[] =
{
	{ 0x0000, 0xd7ff, MWA_ROM },					/* ROM */
	{ 0xd800, 0xd800, splash_adpcm_data_w },		/* ADPCM data for the MSM5205 chip */
//	{ 0xe000, 0xe000, MWA_NOP },					/* ??? */
	{ 0xf000, 0xf000, YM3812_control_port_0_w },	/* YM3812 */
	{ 0xf001, 0xf001, YM3812_write_port_0_w },		/* YM3812 */
	{ 0xf800, 0xffff, MWA_RAM },					/* RAM */
	{ -1 }
};


INPUT_PORTS_START( splash )

PORT_START	/* DSW #1 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "1C/1C or Free Play (if Coin B too)" )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x00, "1C/1C or Free Play (if Coin A too)" )

PORT_START	/* DSW #2 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, "Easy" )
	PORT_DIPSETTING(    0x03, "Normal" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	/* 	according to the manual, Lives = 0x00 is NOT used */
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Girls" )
	PORT_DIPSETTING(    0x00, "Light" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPNAME( 0x40, 0x40, "Paint Mode" )
	PORT_DIPSETTING(    0x00, "Paint again" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

PORT_START	/* 1P INPUTS & COINSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

PORT_START	/* 2P INPUTS & STARTSW */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )
INPUT_PORTS_END


#define TILELAYOUT8(NUM) static struct GfxLayout tilelayout8_##NUM =	\
{																		\
	8,8,									/* 8x8 tiles */				\
	NUM/8,									/* number of tiles */		\
	4,										/* bitplanes */				\
	{ 3*NUM*8, 1*NUM*8, 2*NUM*8, 0*NUM*8 }, /* plane offsets */			\
	{ 0,1,2,3,4,5,6,7 },												\
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },								\
	8*8																	\
}

#define TILELAYOUT16(NUM) static struct GfxLayout tilelayout16_##NUM =				\
{																					\
	16,16,									/* 16x16 tiles */						\
	NUM/32,									/* number of tiles */					\
	4,										/* bitplanes */							\
	{ 3*NUM*8, 1*NUM*8, 2*NUM*8, 0*NUM*8 }, /* plane offsets */						\
	{ 0,1,2,3,4,5,6,7, 16*8+0,16*8+1,16*8+2,16*8+3,16*8+4,16*8+5,16*8+6,16*8+7 },	\
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8, 8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8 },		\
	32*8																			\
}


TILELAYOUT8(0x020000);
TILELAYOUT16(0x020000);


#define GFXDECODEINFO(NUM) static struct GfxDecodeInfo gfxdecodeinfo_##NUM[] =		\
{																					\
	{ REGION_GFX1, 0x000000, &tilelayout8_##NUM,0, 128 },							\
	{ REGION_GFX1, 0x000000, &tilelayout16_##NUM,0, 128 },							\
	{ -1 }																			\
}

GFXDECODEINFO(0x020000);



static struct YM3812interface ym3812_interface =
{
	1,						/* 1 chip */
	3000000,				/* 3 MHz? */
	{ 40 },					/* volume */
	{ 0 }					/* IRQ handler */
};

static struct MSM5205interface msm5205_interface =
{
	1,						/* 1 chip */
	384000,					/* 384KHz */
	{ splash_msm5205_int },	/* IRQ handler */
	{ MSM5205_S48_4B },		/* 8KHz */
	{ 80 }					/* volume */
};


static struct MachineDriver machine_driver_splash =
{
	{
		{
			CPU_M68000,
			24000000/2,			/* 12 MHz */
			splash_readmem,splash_writemem,0,0,
			m68_level6_irq,1
		},
		{
			CPU_Z80,
			30000000/8,			/* 3.75 MHz? */
			splash_readmem_sound, splash_writemem_sound,0,0,
			nmi_interrupt,64	/* needed for the msm5205 to play the samples */
		}
	},
	60,DEFAULT_REAL_60HZ_VBLANK_DURATION,
	1,
	0,

	/* video hardware */
	64*8, 64*8, { 2*8, 49*8-1, 2*8, 32*8-1 },
	gfxdecodeinfo_0x020000,
	2048, 2048,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	splash_vh_start,
	0,
	splash_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_YM3812,
			&ym3812_interface
		},
		{
			SOUND_MSM5205,
			&msm5205_interface
	    }
	}
};


ROM_START( splash )
	ROM_REGION( 0x400000, REGION_CPU1 )	/* 68000 code + gfx */
	ROM_LOAD_EVEN(	"4g",	0x000000, 0x020000, 0xb38fda40 )
	ROM_LOAD_ODD(	"4i",	0x000000, 0x020000, 0x02359c47 )
	ROM_LOAD_EVEN(	"5g",	0x100000, 0x080000, 0xa4e8ed18 )
	ROM_LOAD_ODD(	"5i",	0x100000, 0x080000, 0x73e1154d )
	ROM_LOAD_EVEN(	"6g",	0x200000, 0x080000, 0xffd56771 )
	ROM_LOAD_ODD(	"6i",	0x200000, 0x080000, 0x16e9170c )
	ROM_LOAD_EVEN(	"8g",	0x300000, 0x080000, 0xdc3a3172 )
	ROM_LOAD_ODD(	"8i",	0x300000, 0x080000, 0x2e23e6c3 )

	ROM_REGION( 0x010000, REGION_CPU2 )	/* Z80 code + sound data */
	ROM_LOAD( "5c",	0x00000, 0x10000, 0x0ed7ebc9 )

	ROM_REGION( 0x080000, REGION_GFX1 | REGIONFLAG_DISPOSE )
	ROM_LOAD( "13i",	0x000000, 0x020000, 0xfebb9893 )
	ROM_LOAD( "15i",	0x020000, 0x020000, 0x2a8cb830 )
	ROM_LOAD( "16i",	0x040000, 0x020000, 0x21aeff2c )
	ROM_LOAD( "18i",	0x060000, 0x020000, 0x028a4a68 )
ROM_END


GAME( 1992, splash, 0, splash, splash, 0, ROT0_16BIT, "Gaelco", "Splash!" )
