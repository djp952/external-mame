/***************************************************************************

	Atari Return of the Jedi hardware

	driver by Dan Boris

	Games supported:
		* Return of the Jedi

	Known bugs:
		* none at this time

****************************************************************************

	Memory map

****************************************************************************

	========================================================================
	CPU #1
	========================================================================
	0000-07FF   R/W   xxxxxxxx    Z-page Working RAM
	0800-08FF   R/W   xxxxxxxx    NVRAM
	0C00        R     xxxx-xxx    Switch inputs #1
	            R     x-------       (right coin)
	            R     -x------       (left coin)
	            R     --x-----       (aux coin)
	            R     ---x----       (self test)
	            R     -----x--       (left thumb switch)
	            R     ------x-       (fire switches)
	            R     -------x       (right thumb switch)
	0C01        R     xxx--x--    Communications
	            R     x-------       (VBLANK)
                R     -x------       (sound CPU communications latch full flag)
                R     --x-----       (sound CPU acknowledge latch flag)
                R     -----x--       (slam switch)
    1400        R     xxxxxxxx    Sound acknowledge latch
    1800        R     xxxxxxxx    Read A/D conversion
    1C00          W   --------    Enable NVRAM
    1C01          W   --------    Disable NVRAM
    1C80          W   --------    Start A/D conversion (horizontal)
    1C82          W   --------    Start A/D conversion (vertical)
    1D00          W   --------    NVRAM store
    1D80          W   --------    Watchdog clear
    1E00          W   --------    Interrupt acknowledge
    1E80          W   x-------    Left coin counter
    1E81          W   x-------    Right coin counter
    1E82          W   x-------    LED 1 (not used)
    1E83          W   x-------    LED 2 (not used)
    1E84          W   x-------    Alphanumerics bank select
    1E86          W   x-------    Sound CPU reset
    1E87          W   x-------    Video off
    1F00          W   xxxxxxxx    Sound communications latch
    1F80          W   -----xxx    Program ROM bank select
    2000-23FF   R/W   xxxxxxxx    Scrolling playfield (low 8 bits)
    2400-27FF   R/W   ----xxxx    Scrolling playfield (upper 4 bits)
    2800-2BFF   R/W   xxxxxxxx    Color RAM low
                R/W   -----xxx       (blue)
                R/W   --xxx---       (green)
                R/W   xx------       (red LSBs)
    2C00-2FFF   R/W   ----xxxx    Color RAM high
                R/W   -------x       (red MSB)
                R/W   ----xxx-       (intensity)
    3000-37BF   R/W   xxxxxxxx    Alphanumerics RAM
    37C0-37EF   R/W   xxxxxxxx    Motion object picture
    3800-382F   R/W   -xxxxxxx    Motion object flags
                R/W   -x---xx-       (picture bank)
                R/W   --x-----       (vertical flip)
                R/W   ---x----       (horizontal flip)
                R/W   ----x---       (32 pixels tall)
                R/W   -------x       (X position MSB)
    3840-386F   R/W   xxxxxxxx       (Y position)
    38C0-38EF   R/W   xxxxxxxx       (X position LSBs)
    3C00-3C01     W   xxxxxxxx    Scrolling playfield vertical position
    3D00-3D01     W   xxxxxxxx    Scrolling playfield horizontal position
    3E00-3FFF     W   xxxxxxxx    PIXI graphics expander RAM
    4000-7FFF   R     xxxxxxxx    Banked program ROM
    8000-FFFF   R     xxxxxxxx    Fixed program ROM
	========================================================================
	Interrupts:
		NMI not connected
		IRQ generated by 32V
	========================================================================


	========================================================================
	CPU #2
	========================================================================
	0000-07FF   R/W   xxxxxxxx    Z-page working RAM
	0800-083F   R/W   xxxxxxxx    Custom I/O
	1000          W   --------    Interrupt acknowledge
	1100          W   xxxxxxxx    Speech data
	1200          W   --------    Speech write strobe on
	1300          W   --------    Speech write strobe off
	1400          W   xxxxxxxx    Main CPU acknowledge latch
	1500          W   -------x    Speech chip reset
	1800        R     xxxxxxxx    Main CPU communication latch
	1C00        R     x-------    Speech chip ready
	1C01        R     xx------    Communications
	            R     x-------       (sound CPU communication latch full flag)
	            R     -x------       (sound CPU acknowledge latch full flag)
	8000-FFFF   R     xxxxxxxx    Program ROM
	========================================================================
	Interrupts:
		NMI not connected
		IRQ generated by 32V
	========================================================================

***************************************************************************/

#include "driver.h"
#include "cpu/m6502/m6502.h"
#include "vidhrdw/generic.h"


/* constants */
#define MAIN_CPU_OSC		10000000
#define SOUND_CPU_OSC		12096000


/* local variables */
static UINT8 control_num;
static UINT8 sound_latch;
static UINT8 sound_ack_latch;
static UINT8 sound_comm_stat;
static UINT8 speech_write_buffer;
static UINT8 speech_strobe_state;
static UINT8 *nvram;
static size_t nvram_size;
static UINT8 nvram_enabled;


/* video driver data & functions */
extern UINT8 *jedi_PIXIRAM;
extern UINT8 *jedi_backgroundram;
extern size_t jedi_backgroundram_size;

int  jedi_vh_start(void);
void jedi_vh_stop(void);
void jedi_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
WRITE_HANDLER( jedi_alpha_banksel_w );
WRITE_HANDLER( jedi_paletteram_w );
WRITE_HANDLER( jedi_backgroundram_w );
WRITE_HANDLER( jedi_vscroll_w );
WRITE_HANDLER( jedi_hscroll_w );
WRITE_HANDLER( jedi_video_off_w );
WRITE_HANDLER( jedi_PIXIRAM_w );



/*************************************
 *
 *	Interrupt handling
 *
 *************************************/

static void generate_interrupt(int scanline)
{
	/* IRQ is set by /32V */
	cpu_set_irq_line(0, M6502_INT_IRQ, (scanline & 32) ? CLEAR_LINE : ASSERT_LINE);
	cpu_set_irq_line(1, M6502_INT_IRQ, (scanline & 32) ? CLEAR_LINE : ASSERT_LINE);

	/* set up for the next */
	scanline += 32;
	if (scanline > 256)
		scanline = 32;
	timer_set(cpu_getscanlinetime(scanline), scanline, generate_interrupt);
}


static WRITE_HANDLER( main_irq_ack_w )
{
	cpu_set_irq_line(0, M6502_INT_IRQ, CLEAR_LINE);
}


static WRITE_HANDLER( sound_irq_ack_w )
{
	cpu_set_irq_line(1, M6502_INT_IRQ, CLEAR_LINE);
}


static void init_machine(void)
{
	/* init globals */
	control_num = 0;
	sound_latch = 0;
	sound_ack_latch = 0;
	sound_comm_stat = 0;
	speech_write_buffer = 0;
	speech_strobe_state = 0;
	nvram_enabled = 0;

	/* set a timer to run the interrupts */
	timer_set(cpu_getscanlinetime(32), 32, generate_interrupt);
}



/*************************************
 *
 *	Main program ROM banking
 *
 *************************************/

static WRITE_HANDLER( rom_banksel_w )
{
	UINT8 *RAM = memory_region(REGION_CPU1);

    if (data & 0x01) cpu_setbank(1, &RAM[0x10000]);
    if (data & 0x02) cpu_setbank(1, &RAM[0x14000]);
    if (data & 0x04) cpu_setbank(1, &RAM[0x18000]);
}



/*************************************
 *
 *	Main CPU -> Sound CPU communications
 *
 *************************************/

static WRITE_HANDLER( sound_reset_w )
{
	cpu_set_reset_line(1, (data & 1) ? CLEAR_LINE : ASSERT_LINE);
}


static void delayed_sound_latch_w(int data)
{
    sound_latch = data;
    sound_comm_stat |= 0x80;
}


static WRITE_HANDLER( sound_latch_w )
{
	timer_set(TIME_NOW, data, delayed_sound_latch_w);
}


static READ_HANDLER( sound_latch_r )
{
    sound_comm_stat &= ~0x80;
    return sound_latch;
}



/*************************************
 *
 *	Sound CPU -> Main CPU communications
 *
 *************************************/

static READ_HANDLER( sound_ack_latch_r )
{
    sound_comm_stat &= ~0x40;
    return sound_ack_latch;
}


static WRITE_HANDLER( sound_ack_latch_w )
{
    sound_ack_latch = data;
    sound_comm_stat |= 0x40;
}



/*************************************
 *
 *	I/O ports
 *
 *************************************/

static READ_HANDLER( a2d_data_r )
{
	switch (control_num)
	{
		case 0:		return readinputport(2);
		case 2:		return readinputport(3);
		default:	return 0;
	}
    return 0;
}


static READ_HANDLER( special_port1_r )
{
	return readinputport(1) ^ ((sound_comm_stat >> 1) & 0x60);
}


static WRITE_HANDLER( a2d_select_w )
{
    control_num = offset;
}


static READ_HANDLER( soundstat_r )
{
    return sound_comm_stat;
}


static WRITE_HANDLER( jedi_coin_counter_w )
{
	coin_counter_w(offset, data >> 7);
}



/*************************************
 *
 *	Speech access
 *
 *************************************/

static WRITE_HANDLER( speech_data_w )
{
	speech_write_buffer = data;
}


static WRITE_HANDLER( speech_strobe_w )
{
	int state = (~offset >> 8) & 1;

	if ((state ^ speech_strobe_state) && state)
		tms5220_data_w(0, speech_write_buffer);
	speech_strobe_state = state;
}


static READ_HANDLER( speech_ready_r )
{
    return (!tms5220_ready_r()) << 7;
}



/*************************************
 *
 *	NVRAM
 *
 *************************************/

static WRITE_HANDLER( nvram_data_w )
{
	if (nvram_enabled)
		nvram[offset] = data;
}


static WRITE_HANDLER( nvram_enable_w )
{
	nvram_enabled = ~offset & 1;
}


static void nvram_handler(void *file, int read_or_write)
{
	if (read_or_write)
		osd_fwrite(file, nvram, nvram_size);
	else if (file)
		osd_fread(file, nvram, nvram_size);
	else
		memset(nvram, 0, nvram_size);
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x0800, 0x08ff, MRA_RAM },
	{ 0x0c00, 0x0c00, input_port_0_r },
	{ 0x0c01, 0x0c01, special_port1_r },
	{ 0x1400, 0x1400, sound_ack_latch_r },
	{ 0x1800, 0x1800, a2d_data_r },
	{ 0x2000, 0x27ff, MRA_RAM },
	{ 0x2800, 0x2fff, MRA_RAM },
	{ 0x3000, 0x37bf, MRA_RAM },
	{ 0x37c0, 0x3bff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_BANK1 },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x0800, 0x08ff, nvram_data_w, &nvram, &nvram_size },
	{ 0x1c00, 0x1c01, nvram_enable_w },
	{ 0x1c80, 0x1c82, a2d_select_w },
	{ 0x1d00, 0x1d00, MWA_NOP },	/* NVRAM store */
	{ 0x1d80, 0x1d80, watchdog_reset_w },
	{ 0x1e00, 0x1e00, main_irq_ack_w },
	{ 0x1e80, 0x1e81, jedi_coin_counter_w },
	{ 0x1e82, 0x1e83, MWA_NOP },	/* LED control; not used */
	{ 0x1e84, 0x1e84, jedi_alpha_banksel_w },
	{ 0x1e86, 0x1e86, sound_reset_w },
	{ 0x1e87, 0x1e87, jedi_video_off_w },
	{ 0x1f00, 0x1f00, sound_latch_w },
	{ 0x1f80, 0x1f80, rom_banksel_w },
	{ 0x2000, 0x27ff, jedi_backgroundram_w, &jedi_backgroundram, &jedi_backgroundram_size },
	{ 0x2800, 0x2fff, jedi_paletteram_w, &paletteram },
	{ 0x3000, 0x37bf, videoram_w, &videoram, &videoram_size },
	{ 0x37c0, 0x3bff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x3c00, 0x3c01, jedi_vscroll_w },
	{ 0x3d00, 0x3d01, jedi_hscroll_w },
	{ 0x3e00, 0x3fff, jedi_PIXIRAM_w, &jedi_PIXIRAM },
	{ 0x4000, 0xffff, MWA_ROM },
MEMORY_END



/*************************************
 *
 *	Sound CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( readmem2 )
	{ 0x0000, 0x07ff, MRA_RAM },
	{ 0x0800, 0x080f, pokey1_r },
	{ 0x0810, 0x081f, pokey2_r },
	{ 0x0820, 0x082f, pokey3_r },
	{ 0x0830, 0x083f, pokey4_r },
	{ 0x1800, 0x1800, sound_latch_r },
	{ 0x1c00, 0x1c00, speech_ready_r },
	{ 0x1c01, 0x1c01, soundstat_r },
	{ 0x8000, 0xffff, MRA_ROM },
MEMORY_END


static MEMORY_WRITE_START( writemem2 )
	{ 0x0000, 0x07ff, MWA_RAM },
	{ 0x0800, 0x080f, pokey1_w },
	{ 0x0810, 0x081f, pokey2_w },
	{ 0x0820, 0x082f, pokey3_w },
	{ 0x0830, 0x083f, pokey4_w },
	{ 0x1000, 0x1000, sound_irq_ack_w },
	{ 0x1100, 0x11ff, speech_data_w },
	{ 0x1200, 0x13ff, speech_strobe_w },
	{ 0x1400, 0x1400, sound_ack_latch_w },
	{ 0x8000, 0xffff, MWA_ROM },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( jedi )
	PORT_START	/* 0C00 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_COIN1 )

	PORT_START	/* 0C01 */
	PORT_BIT( 0x03, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x18, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x60, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* sound comm */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START	/* analog Y */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_Y, 100, 10, 0, 255 )

	PORT_START	/* analog X */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X, 100, 10, 0, 255 )
INPUT_PORTS_END



/*************************************
 *
 *	Graphics layouts
 *
 *************************************/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 1 },
	{ 0, 2, 4, 6, 8, 10, 12, 14 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};


static struct GfxLayout pflayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 4, RGN_FRAC(1,2), RGN_FRAC(1,2)+4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	16*8
};


static struct GfxLayout spritelayout =
{
	8,16,
	RGN_FRAC(1,2),
	4,
	{ 0, 4, RGN_FRAC(1,2), RGN_FRAC(1,2)+4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	32*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,	  0, 1 },
	{ REGION_GFX2, 0, &pflayout,	  0, 1 },
	{ REGION_GFX3, 0, &spritelayout,  0, 1 },
	{ -1 }
};



/*************************************
 *
 *	Sound definitions
 *
 *************************************/

static struct POKEYinterface pokey_interface =
{
	4,
	SOUND_CPU_OSC/2/4,	/* 1.5MHz */
	{ 30, 30, MIXER(30,MIXER_PAN_LEFT), MIXER(30,MIXER_PAN_RIGHT) },
	/* The 8 pot handlers */
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	/* The allpot handler */
	{ 0, 0, 0, 0 }
};


static struct TMS5220interface tms5220_interface =
{
	SOUND_CPU_OSC/2/9,
	100,
	0
};



/*************************************
 *
 *	Machine driver
 *
 *************************************/

static const struct MachineDriver machine_driver_jedi =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			MAIN_CPU_OSC/2/2,		/* 2.5MHz */
			readmem,writemem,0,0,
			ignore_interrupt,1
		},
		{
			CPU_M6502,
			SOUND_CPU_OSC/2/4,		/* 1.5MHz */
			readmem2,writemem2,0,0,
			ignore_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,
	4,
	init_machine,

	/* video hardware */
	37*8, 30*8, { 0*8, 37*8-1, 0*8, 30*8-1 },
	gfxdecodeinfo,
	1024+1,0,	/* no colortable, we do the lookups ourselves */
				/* reserve color 1024 for black (disabled display) */
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,
	jedi_vh_start,
	jedi_vh_stop,
	jedi_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_POKEY,
			&pokey_interface
		},
		{
			SOUND_TMS5220,
			&tms5220_interface
		}
	},

	nvram_handler
};



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( jedi )
	ROM_REGION( 0x1C000, REGION_CPU1, 0 )	/* 64k for code + 48k for banked ROMs */
	ROM_LOAD( "14f_221.bin",  0x08000, 0x4000, 0x414d05e3 )
	ROM_LOAD( "13f_222.bin",  0x0c000, 0x4000, 0x7b3f21be )
	ROM_LOAD( "13d_123.bin",  0x10000, 0x4000, 0x877f554a ) /* Page 0 */
	ROM_LOAD( "13b_124.bin",  0x14000, 0x4000, 0xe72d41db ) /* Page 1 */
	ROM_LOAD( "13a_122.bin",  0x18000, 0x4000, 0xcce7ced5 ) /* Page 2 */

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* space for the sound ROMs */
	ROM_LOAD( "01c_133.bin",  0x8000, 0x4000, 0x6c601c69 )
	ROM_LOAD( "01a_134.bin",  0xC000, 0x4000, 0x5e36c564 )

	ROM_REGION( 0x02000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "11t_215.bin",  0x00000, 0x2000, 0x3e49491f ) /* Alphanumeric */

	ROM_REGION( 0x10000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "06r_126.bin",  0x00000, 0x8000, 0x9c55ece8 ) /* Playfield */
	ROM_LOAD( "06n_127.bin",  0x08000, 0x8000, 0x4b09dcc5 )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "01h_130.bin",  0x00000, 0x8000, 0x2646a793 ) /* Sprites */
	ROM_LOAD( "01f_131.bin",  0x08000, 0x8000, 0x60107350 )
	ROM_LOAD( "01m_128.bin",  0x10000, 0x8000, 0x24663184 )
	ROM_LOAD( "01k_129.bin",  0x18000, 0x8000, 0xac86b98c )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )	/* background smoothing */
	ROM_LOAD( "136030.117",   0x0000, 0x0400, 0x9831bd55 )
	ROM_LOAD( "136030.118",   0x0400, 0x0400, 0x261fbfe7 )
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1984, jedi, 0, jedi, jedi, 0, ROT0, "Atari", "Return of the Jedi" )
