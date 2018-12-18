/*** DRIVER INFO & NOTES ******************************************************
 Speed Spin (c)1994 TCH
  driver by David Haywood & Farfetch'd

Notes:

Banking is a bit hackish at the moment.
Colours
Tile Attributes (needs flipping on the championship table)
Unknown Port Writes:
cpu #0 (PC=00000D88): unmapped port byte write to 00000001 = 02
cpu #0 (PC=00006974): unmapped port byte write to 00000010 = 10
cpu #0 (PC=00006902): unmapped port byte write to 00000010 = 20
etc.
Writes to ROM regions
cpu #0 (PC=00001119): byte write to ROM 0000C8B9 = 01
cpu #0 (PC=00001119): byte write to ROM 0000C899 = 01
etc.


******************************************************************************/

#include "driver.h"

/*** README INFO **************************************************************

ROMSET: speedspn

Speed Spin
1994, TCH

PCB No: PR02/2
CPU   : Z80 (6Mhz)
SOUND : Z80 (6Mhz), OKI M6295
RAM   : 62256 (x1), 6264 (x1), 6116 (x6)
XTAL  : 12.000MHz (near Z80's), 10.000MHz (near PLCC84)
DIP   : 8 position (x2)
OTHER : TPC1020AFN-084C (PLCC84, Gfx controller)

ROMs          Type    Used            C'sum
-------------------------------------------
TCH-SS1.u78   27C040  Main Program    C246h
TCH-SS2.u96   27C512  Sound Program   5D04h
TCH-SS3.u95   27C040  Oki Samples     7340h
TCH-SS4.u70   27C010  \               ECD8h
TCH-SS5.u69     "     |               9E7Bh
TCH-SS6.u60     "     |               E844h
TCH-SS7.u59     "     |  Gfx          3DDah
TCH-SS8.u39     "     |               A9B5h
TCH-SS9.u34     "     /               AB2Bh

Developers:
More info Reqd? Email me....
theguru@emuunlim.com

******************************************************************************/

/* in vidhrdw */
WRITE_HANDLER( speedspn_vidram_w );
READ_HANDLER( speedspn_vidram_r );
VIDEO_START(speedspn);
VIDEO_UPDATE(speedspn);
WRITE_HANDLER(speedspn_banked_vidram_change);
WRITE_HANDLER(speedspn_global_display_w);

static READ_HANDLER(speedspn_irq_ack_r)
{
	// I think this simply acknowledges the IRQ #0, it's read within the handler and the
	//  value is discarded
	return 0;
}

static WRITE_HANDLER(speedspn_banked_rom_change)
{
	/* this is currently a hack I guess (or is it some odd protection?) :-/
	*/

	unsigned char *rom = memory_region(REGION_CPU1);
	int addr;

	addr = -1;

	if (data == 0x00) addr = 0x28000; // mmk
	if (data == 0x01) addr = 0x14000;
	if (data == 0x02) addr = 0x1c000;
	if (data == 0x03) addr = 0x54000;
	if (data == 0x04) addr = 0x48000;
	if (data == 0x05) addr = 0x3c000;
	if (data == 0x08) addr = 0x50000;

	if (addr == -1)
	{
		usrintf_showmessage ("Unmapped Bank Write %02x", data);
		addr = 0;
	}

	cpu_setbank(1,&rom[addr + 0x8000]);
}

/*** SOUND RELATED ***********************************************************/

static WRITE_HANDLER(speedspn_sound_w)
{
	soundlatch_w(1,data);
	cpu_set_irq_line(1,0,HOLD_LINE);
}

static struct OKIM6295interface okim6295_interface =
{
	1,				/* 1 chip */
	{ 8500 },		/* frequency (Hz) */
	{ REGION_SOUND1 },	/* memory region */
	{ 47 }
};

/*** MEMORY MAPS *************************************************************/

/* main cpu */

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8800, 0x8fff, MRA_RAM },
	{ 0x9000, 0x9fff, speedspn_vidram_r }, /* banked? */
	{ 0xa000, 0xa7ff, MRA_RAM },
	{ 0xa800, 0xafff, MRA_RAM },
	{ 0xb000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xffff, MRA_BANK1 }, /* banked rom? */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, paletteram_xxxxBBBBGGGGRRRR_w, &paletteram },	/* RAM COLOUR */
	{ 0x8800, 0x8fff, MWA_RAM },
	{ 0x9000, 0x9fff, speedspn_vidram_w },	/* RAM FIX / RAM OBJECTS (selected by bit 0 of port 07?)*/
	{ 0xa000, 0xa7ff, MWA_RAM },
	{ 0xa800, 0xafff, MWA_RAM },
	{ 0xb000, 0xbfff, MWA_RAM },	/* RAM PROGRAM */
	{ 0xc000, 0xffff, MWA_ROM },	/* banked rom? */
MEMORY_END

static PORT_WRITE_START( writeport )
	{ 0x07, 0x07, speedspn_global_display_w },
	{ 0x12, 0x12, speedspn_banked_rom_change },
	{ 0x13, 0x13, speedspn_sound_w },
	{ 0x17, 0x17, speedspn_banked_vidram_change },
PORT_END

static PORT_READ_START( readport )
	{ 0x10, 0x10, input_port_0_r }, // inputs
	{ 0x11, 0x11, input_port_1_r }, // inputs
	{ 0x12, 0x12, input_port_2_r }, // inputs
	{ 0x13, 0x13, input_port_3_r },
	{ 0x14, 0x14, input_port_4_r }, // inputs
	{ 0x16, 0x16, speedspn_irq_ack_r }, // @@@ could be watchdog, value is discarded
PORT_END

/* sound cpu */

static MEMORY_READ_START( readmem2 )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9800, 0x9800, OKIM6295_status_0_r },
	{ 0xa000, 0xa000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( writemem2 )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x9000, MWA_NOP }, // ??
	{ 0x9800, 0x9800, OKIM6295_data_0_w },
MEMORY_END

/*** INPUT PORT **************************************************************/

INPUT_PORTS_START( speedspn )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	PORT_START /* Player 1 Inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )

	PORT_START /* Player 2 Inputs */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )

	PORT_START /* Dips */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x00, "5 Coins/2 Credits" )
	PORT_DIPSETTING(    0x0a, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x05, "3 Coins/5 Credits" )
	PORT_DIPSETTING(    0x09, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x00, "5 Coins/2 Credits" )
	PORT_DIPSETTING(    0xa0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_5C ) )
	PORT_DIPSETTING(    0x50, "3 Coins/5 Credits" )
	PORT_DIPSETTING(    0x90, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )

	PORT_START /* Dips */
	PORT_DIPNAME( 0x01, 0x01, "World Cup" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Backhand" )
	PORT_DIPSETTING(    0x02, "Automatic" )
	PORT_DIPSETTING(    0x00, "Manual" )
	PORT_DIPNAME( 0x0c, 0x0c, "Points to Win" )
	PORT_DIPSETTING(    0x0c, "11 Points and 1 Advantage" )
	PORT_DIPSETTING(    0x08, "11 Points and 2 Advantage" )
	PORT_DIPSETTING(    0x04, "21 Points and 1 Advantage" )
	PORT_DIPSETTING(    0x00, "21 Points and 2 Advantage" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )
INPUT_PORTS_END

/*** GFX DECODE **************************************************************/

static struct GfxLayout speedspn_charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(2,4), RGN_FRAC(3,4), RGN_FRAC(0,4), RGN_FRAC(1,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	8*8
};

static struct GfxLayout speedspn_spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ 0, 4, RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4 },
	{ 16*16+11, 16*16+10, 16*16+9, 16*16+8, 16*16+3, 16*16+2, 16*16+1, 16*16+0,
	        11,       10,       9,       8,       3,       2,       1,       0  },
	{ 8*16+7*16, 8*16+6*16, 8*16+5*16, 8*16+4*16, 8*16+3*16, 8*16+2*16, 8*16+1*16, 8*16+0*16,
	       7*16,      6*16,      5*16,      4*16,      3*16,      2*16,      1*16,      0*16  },
	32*16
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &speedspn_charlayout,   0x000, 0x40 },
	{ REGION_GFX2, 0, &speedspn_spritelayout, 0x000, 0x40 },
	{ -1 } /* end of array */
};

/*** MACHINE DRIVER **********************************************************/

static MACHINE_DRIVER_START( speedspn )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,6000000)		 /* 6 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport, writeport)
	MDRV_CPU_VBLANK_INT(irq0_line_pulse,1)

	MDRV_CPU_ADD(Z80,6000000)		 /* 6 MHz */
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(readmem2,writemem2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(8*8, 56*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x400)

	MDRV_VIDEO_START(speedspn)
	MDRV_VIDEO_UPDATE(speedspn)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

/*** ROM LOADING *************************************************************/

ROM_START( speedspn )
	ROM_REGION( 0x088000, REGION_CPU1, 0 )	/* CPU1 code */
	/* most of this is probably actually banked */
	ROM_LOAD( "tch-ss1.u78", 0x00000, 0x008000, 0x41b6b45b ) /* fixed code */
	ROM_CONTINUE(            0x10000, 0x078000 ) /* banked data */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* CPU2 code */
	ROM_LOAD( "tch-ss2.u96", 0x00000, 0x10000, 0x4611fd0c ) // FIRST AND SECOND HALF IDENTICAL

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )	/* Samples */
	ROM_LOAD( "tch-ss3.u95", 0x00000, 0x080000, 0x1c9deb5e )

	ROM_REGION( 0x80000, REGION_GFX1, ROMREGION_DISPOSE )	/* GFX */
	ROM_LOAD( "tch-ss4.u70", 0x00000, 0x020000, 0x41517859 )
	ROM_LOAD( "tch-ss5.u69", 0x20000, 0x020000, 0x832b2f34 )
	ROM_LOAD( "tch-ss6.u60", 0x40000, 0x020000, 0xf1fd7289 )
	ROM_LOAD( "tch-ss7.u59", 0x60000, 0x020000, 0xc4958543 )

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )	/* GFX */
	ROM_LOAD( "tch-ss8.u39", 0x00000, 0x020000, 0x2f27b16d )
	ROM_LOAD( "tch-ss9.u34", 0x20000, 0x020000, 0xc372f8ec )
ROM_END

/*** GAME DRIVERS ************************************************************/

GAMEX( 1994, speedspn, 0, speedspn, speedspn, 0, ROT180, "TCH", "Speed Spin", GAME_WRONG_COLORS | GAME_IMPERFECT_GRAPHICS )
