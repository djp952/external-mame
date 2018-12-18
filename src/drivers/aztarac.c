/***************************************************************************

	Centuri Aztarac hardware

	driver by Mathis Rosenhauer
	Thanks to David Fish for additional hardware information.

	Games supported:
		* Aztarac

	Known bugs:
		* none at this time

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "aztarac.h"



/*************************************
 *
 *	Machine init
 *
 *************************************/

static int aztarac_irq_callback(int irqline)
{
	return 0xc;
}


static MACHINE_INIT( aztarac )
{
	cpu_set_irq_callback(0, aztarac_irq_callback);
}



/*************************************
 *
 *	NVRAM handler
 *
 *************************************/

static READ16_HANDLER( nvram_r )
{
	return ((data16_t *)generic_nvram)[offset] | 0xfff0;
}



/*************************************
 *
 *	Input ports
 *
 *************************************/

static READ16_HANDLER( joystick_r )
{
    return (((input_port_0_r (offset) - 0xf) << 8) |
            ((input_port_1_r (offset) - 0xf) & 0xff));
}



/*************************************
 *
 *	Main CPU memory handlers
 *
 *************************************/

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x00bfff, MRA16_ROM },
	{ 0x022000, 0x022fff, nvram_r },
	{ 0x027000, 0x027001, joystick_r },
	{ 0x027004, 0x027005, input_port_3_word_r },
	{ 0x027008, 0x027009, aztarac_sound_r },
	{ 0x02700c, 0x02700d, input_port_2_word_r },
	{ 0x02700e, 0x02700f, watchdog_reset16_r },
	{ 0xff8000, 0xffafff, MRA16_RAM },
	{ 0xffe000, 0xffffff, MRA16_RAM },
MEMORY_END


static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x00bfff, MWA16_ROM },
	{ 0x022000, 0x0220ff, MWA16_RAM, (data16_t **)&generic_nvram, &generic_nvram_size },
	{ 0x027008, 0x027009, aztarac_sound_w },
	{ 0xff8000, 0xffafff, MWA16_RAM, &aztarac_vectorram },
	{ 0xffb000, 0xffb001, aztarac_ubr_w },
	{ 0xffe000, 0xffffff, MWA16_RAM },
MEMORY_END



/*************************************
 *
 *	Sound CPU memory handlers
 *
 *************************************/

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x8800, 0x8800, aztarac_snd_command_r },
	{ 0x8c00, 0x8c01, AY8910_read_port_0_r },
	{ 0x8c02, 0x8c03, AY8910_read_port_1_r },
	{ 0x8c04, 0x8c05, AY8910_read_port_2_r },
	{ 0x8c06, 0x8c07, AY8910_read_port_3_r },
	{ 0x9000, 0x9000, aztarac_snd_status_r },
MEMORY_END


static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8c00, 0x8c00, AY8910_write_port_0_w },
	{ 0x8c01, 0x8c01, AY8910_control_port_0_w },
	{ 0x8c02, 0x8c02, AY8910_write_port_1_w },
	{ 0x8c03, 0x8c03, AY8910_control_port_1_w },
	{ 0x8c04, 0x8c04, AY8910_write_port_2_w },
	{ 0x8c05, 0x8c05, AY8910_control_port_2_w },
	{ 0x8c06, 0x8c06, AY8910_write_port_3_w },
	{ 0x8c07, 0x8c07, AY8910_control_port_3_w },
	{ 0x9000, 0x9000, aztarac_snd_status_w },
MEMORY_END



/*************************************
 *
 *	Port definitions
 *
 *************************************/

INPUT_PORTS_START( aztarac )
	PORT_START /* IN0 */
	PORT_ANALOG( 0x1f, 0xf, IPT_AD_STICK_X | IPF_CENTER, 100, 1, 0, 0x1e )

	PORT_START /* IN1 */
	PORT_ANALOG( 0x1f, 0xf, IPT_AD_STICK_Y | IPF_CENTER | IPF_REVERSE, 100, 1, 0, 0x1e )

	PORT_START /* IN2 */
	PORT_ANALOGX( 0xff, 0x00, IPT_DIAL | IPF_REVERSE, 25, 10, 0, 0, KEYCODE_Z, KEYCODE_X, 0, 0 )

	PORT_START /* IN3 */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
INPUT_PORTS_END



/*************************************
 *
 *	Sound interfaces
 *
 *************************************/

static struct AY8910interface ay8910_interface =
{
	4,	/* 4 chips */
	2000000,	/* 2 MHz */
	{ 15, 15, 15, 15 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 }
};



/*************************************
 *
 *	Machine drivers
 *
 *************************************/

static MACHINE_DRIVER_START( aztarac )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq4_line_hold,1)

	MDRV_CPU_ADD(Z80, 2000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)
	MDRV_CPU_PERIODIC_INT(aztarac_snd_timed_irq,100)

	MDRV_FRAMES_PER_SECOND(40)
	MDRV_MACHINE_INIT(aztarac)
	MDRV_NVRAM_HANDLER(generic_1fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_VECTOR | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(400, 300)
	MDRV_VISIBLE_AREA(0, 1024-1, 0, 768-1)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(aztarac)
	MDRV_VIDEO_UPDATE(vector)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END



/*************************************
 *
 *	ROM definitions
 *
 *************************************/

ROM_START( aztarac )
	ROM_REGION( 0xc000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "l8_6.bin", 0x000000, 0x001000, 0x25f8da18 )
	ROM_LOAD16_BYTE( "n8_0.bin", 0x000001, 0x001000, 0x04e20626 )
	ROM_LOAD16_BYTE( "l7_7.bin", 0x002000, 0x001000, 0x230e244c )
	ROM_LOAD16_BYTE( "n7_1.bin", 0x002001, 0x001000, 0x37b12697 )
	ROM_LOAD16_BYTE( "l6_8.bin", 0x004000, 0x001000, 0x1293fb9d )
	ROM_LOAD16_BYTE( "n6_2.bin", 0x004001, 0x001000, 0x712c206a )
	ROM_LOAD16_BYTE( "l5_9.bin", 0x006000, 0x001000, 0x743a6501 )
	ROM_LOAD16_BYTE( "n5_3.bin", 0x006001, 0x001000, 0xa65cbf99 )
	ROM_LOAD16_BYTE( "l4_a.bin", 0x008000, 0x001000, 0x9cf1b0a1 )
	ROM_LOAD16_BYTE( "n4_4.bin", 0x008001, 0x001000, 0x5f0080d5 )
	ROM_LOAD16_BYTE( "l3_b.bin", 0x00a000, 0x001000, 0x8cc7f7fa )
	ROM_LOAD16_BYTE( "n3_5.bin", 0x00a001, 0x001000, 0x40452376 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "j4_c.bin", 0x0000, 0x1000, 0xe897dfcd )
	ROM_LOAD( "j3_d.bin", 0x1000, 0x1000, 0x4016de77 )
ROM_END



/*************************************
 *
 *	Game drivers
 *
 *************************************/

GAME( 1983, aztarac, 0, aztarac, aztarac, 0, ROT0, "Centuri", "Aztarac" )
