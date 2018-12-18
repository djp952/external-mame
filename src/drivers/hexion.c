/***************************************************************************

Hexion (GX122) (c) 1992 Konami

driver by Nicola Salmoria

Notes:
- There are probably palette PROMs missing. Palette data doesn't seem to be
  written anywhere in RAM.
- The board has a 052591, which is used for protection in Thunder Cross and
  S.P.Y. In this game, however, the only thing it seems to do is clear the
  screen.
  This is the program for the 052591:
00: 5f 80 01 e0 08
01: df 80 00 e0 0c
02: df 90 02 e0 0c
03: df a0 03 e0 0c
04: df b0 0f e0 0c
05: df c0 ff bf 0c
06: 5c 02 00 33 0c
07: 5f 80 04 80 0c
08: 5c 0e 00 2b 0c
09: df 70 00 cb 08
0a: 5f 80 00 80 0c
0b: 5c 04 00 2b 0c
0c: df 60 00 cb 08
0d: 5c 0c 1f e9 0c
0e: 4c 0c 2d e9 08
0f: 5f 80 03 80 0c
10: 5c 04 00 2b 0c
11: 5f 00 00 cb 00
12: 5f 80 02 a0 0c
13: df d0 00 c0 04
14: 01 3a 00 f3 0a
15: 5c 08 00 b3 0c
16: 5c 0e 00 13 0c
17: 5f 80 00 a0 0c
18: 5c 00 00 13 0c
19: 5c 08 00 b3 0c
1a: 5c 00 00 13 0c
1b: 84 5a 00 b3 0c
1c: 48 0a 5b d1 0c
1d: 5f 80 00 e0 08
1e: 5f 00 1e fd 0c
1f: 5f 80 01 a0 0c
20: df 20 00 cb 08
21: 5c 08 00 b3 0c
22: 5f 80 03 00 0c
23: 5c 08 00 b3 0c
24: 5f 80 00 80 0c
25: 5c 00 00 33 0c
26: 5c 08 00 93 0c
27: 9f 91 ff cf 0e
28: 5c 84 00 20 0c
29: 84 00 00 b3 0c
2a: 49 10 69 d1 0c
2b: 5f 80 00 e0 08
2c: 5f 00 2c fd 0c
2d: 5f 80 01 a0 0c
2e: df 20 00 cb 08
2f: 5c 08 00 b3 0c
30: 5f 80 03 00 0c
31: 5c 00 00 b3 0c
32: 5f 80 01 00 0c
33: 5c 08 00 b3 0c
34: 5f 80 00 80 0c
35: 5c 00 00 33 0c
36: 5c 08 00 93 0c
37: 9f 91 ff cf 0e
38: 5c 84 00 20 0c
39: 84 00 00 b3 0c
3a: 49 10 79 d1 0c
3b: 5f 80 00 e0 08
3c: 5f 00 3c fd 0c
3d: ff ff ff ff ff
3e: ff ff ff ff ff
3f: ff ff ff ff ff

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


VIDEO_START( hexion );
VIDEO_UPDATE( hexion );

WRITE_HANDLER( hexion_bankswitch_w );
READ_HANDLER( hexion_bankedram_r );
WRITE_HANDLER( hexion_bankedram_w );
WRITE_HANDLER( hexion_bankctrl_w );
WRITE_HANDLER( hexion_gfxrom_select_w );



static WRITE_HANDLER( coincntr_w )
{
//logerror("%04x: coincntr_w %02x\n",activecpu_get_pc(),data);

	/* bits 0/1 = coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);

	/* bit 5 = flip screen */
	flip_screen_set(data & 0x20);

	/* other bit unknown */
if ((data & 0xdc) != 0x10) usrintf_showmessage("coincntr %02x",data);
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x9fff, MRA_BANK1 },
	{ 0xa000, 0xbfff, MRA_RAM },
	{ 0xc000, 0xdffe, hexion_bankedram_r },
	{ 0xf400, 0xf400, input_port_0_r },
	{ 0xf401, 0xf401, input_port_1_r },
	{ 0xf402, 0xf402, input_port_3_r },
	{ 0xf403, 0xf403, input_port_4_r },
	{ 0xf440, 0xf440, input_port_2_r },
	{ 0xf441, 0xf441, input_port_5_r },
	{ 0xf540, 0xf540, watchdog_reset_r },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xa000, 0xbfff, MWA_RAM },
	{ 0xc000, 0xdffe, hexion_bankedram_w },
	{ 0xdfff, 0xdfff, hexion_bankctrl_w },
	{ 0xe800, 0xe87f, K051649_waveform_w },
	{ 0xe880, 0xe889, K051649_frequency_w },
	{ 0xe88a, 0xe88e, K051649_volume_w },
	{ 0xe88f, 0xe88f, K051649_keyonoff_w },
	{ 0xf000, 0xf00f, MWA_NOP },	/* 053252? f00e = IRQ ack, f00f = NMI ack */
	{ 0xf200, 0xf200, OKIM6295_data_0_w },
	{ 0xf480, 0xf480, hexion_bankswitch_w },
	{ 0xf4c0, 0xf4c0, coincntr_w },
	{ 0xf500, 0xf500, hexion_gfxrom_select_w },
MEMORY_END



INPUT_PORTS_START( hexion )
	PORT_START
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) )
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x70, 0x70, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x70, "Easiest" )
	PORT_DIPSETTING(    0x60, "Very Easy" )
	PORT_DIPSETTING(    0x50, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x30, "Medium Hard" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x10, "Very Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* 052591? game waits for it to be 0 */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2, 3 },
	{ RGN_FRAC(1,2)+0*4, RGN_FRAC(1,2)+1*4, 0*4, 1*4, RGN_FRAC(1,2)+2*4, RGN_FRAC(1,2)+3*4, 2*4, 3*4 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 16 },
	{ -1 } /* end of array */
};



static struct OKIM6295interface okim6295_interface =
{
	1,                  /* 1 chip */
	{ 8000 },           /* 8000Hz frequency */
	{ REGION_SOUND1 },	/* memory region */
	{ 100 }
};

static struct k051649_interface k051649_interface =
{
	24000000/16,	/* Clock */
	100,			/* Volume */
};



static INTERRUPT_GEN( hexion_interrupt )
{
	/* NMI handles start and coin inputs, origin unknown */
	if (cpu_getiloops())
		cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
	else
		cpu_set_irq_line(0, 0, HOLD_LINE);
}

static MACHINE_DRIVER_START( hexion )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,24000000/4)	/* Z80B 6 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(hexion_interrupt,3)	/* both IRQ and NMI are used */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)

	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_VIDEO_START(hexion)
	MDRV_VIDEO_UPDATE(hexion)

	/* sound hardware */
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
	MDRV_SOUND_ADD(K051649, k051649_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( hexion )
	ROM_REGION( 0x34800, REGION_CPU1, 0 )	/* ROMs + space for additional RAM */
	ROM_LOAD( "122jab01.bin", 0x00000, 0x20000, 0xeabc6dd1 )
	ROM_RELOAD(               0x10000, 0x20000 )	/* banked at 8000-9fff */

	ROM_REGION( 0x80000, REGION_GFX1, 0 )	/* addressable by the main CPU */
	ROM_LOAD( "122a07.bin",   0x00000, 0x40000, 0x22ae55e3 )
	ROM_LOAD( "122a06.bin",   0x40000, 0x40000, 0x438f4388 )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )	/* OKIM6295 samples */
	ROM_LOAD( "122a05.bin",   0x0000, 0x40000, 0xbcc831bf )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "122a04.10b",   0x0000, 0x0100, 0x506eb8c6 )
	ROM_LOAD( "122a03.11b",   0x0100, 0x0100, 0x590c4f64 )
	ROM_LOAD( "122a02.13b",   0x0200, 0x0100, 0x5734305c )
ROM_END


GAME( 1992, hexion, 0, hexion, hexion, 0, ROT0, "Konami", "Hexion (Japan)" )
