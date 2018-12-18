/***************************************************************************

  Capcom System 2
  ===============


    Driver by Paul Leaman (paul@vortexcomputing.demon.co.uk)

    Thanks to Raz, Crashtest and the CPS2 decryption team without whom
    none of this would have been possible.

***************************************************************************/

#include "driver.h"
#include "machine/eeprom.h"
#include "cpu/m68000/m68000.h"

#include "cps1.h"       /* External CPS1 definitions */

/*
Export this function so that the vidhrdw routine can drive the
Q-Sound hardware
*/
WRITE16_HANDLER( cps2_qsound_sharedram_w )
{
    qsound_sharedram1_w(offset/2, data, 0xff00);
}

/* Maximum size of Q Sound Z80 region */
#define QSOUND_SIZE 0x50000

/* Maximum 680000 code size */
#undef  CODE_SIZE
#define CODE_SIZE   0x0800000

#define XOR_BASE    (CODE_SIZE/2)

extern void cps_setversion(int v);
extern int cps_getversion(void);
extern data16_t *cps2_objram;
extern size_t cps2_objram_size;
extern data16_t *cps2_output;
extern size_t cps2_output_size;
extern int cps2_vh_start(void);


int cps2_interrupt(void)
{
	/* 2 is vblank, 4 is some sort of scanline interrupt, 6 is both at the same time. */

	if (cpu_getiloops() == 0)
		return 2;
	else
	{
		if (cps1_output[0x52/2])	/* scanline counter? */
			cps1_output[0x52/2]--;
		return 4;
	}
}



static struct EEPROM_interface cps2_eeprom_interface =
{
	6,		/* address bits */
	16,		/* data bits */
	"0110",	/*  read command */
	"0101",	/* write command */
	"0111"	/* erase command */
};

static void cps2_nvram_handler(void *file,int read_or_write)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
        EEPROM_init(&cps2_eeprom_interface);

		if (file)
			EEPROM_load(file);
	}
}

READ16_HANDLER( cps2_eeprom_port_r )
{
    return (input_port_2_word_r(offset) & 0xfffe) | EEPROM_read_bit();
}

WRITE16_HANDLER( cps2_eeprom_port_w )
{
    if (ACCESSING_MSB)
    {
		/* bit 0 - Unused */
		/* bit 1 - Unused */
        /* bit 2 - Unused */
        /* bit 3 - Unused */
        /* bit 4 - Eeprom data  */
        /* bit 5 - Eeprom clock */
        /* bit 6 - */
        /* bit 7 - */

        /* EEPROM */
        EEPROM_write_bit(data & 0x1000);
        EEPROM_set_clock_line((data & 0x2000) ? ASSERT_LINE : CLEAR_LINE);
        EEPROM_set_cs_line((data & 0x4000) ? CLEAR_LINE : ASSERT_LINE);

	}
    else
    {
        /* bit 0 - coin counter */
		/* bit 1 - Unused */
        /* bit 2 - Unused */
        /* bit 3 - On all the time */
        /* bit 4 - lock 1  */
        /* bit 5 - */
        /* bit 6 - */
        /* bit 7 - */

        coin_counter_w(0, data & 0x0001);
        coin_lockout_w(0,~data & 0x0010);
        coin_lockout_w(1,~data & 0x0020);
        coin_lockout_w(2,~data & 0x0040);
        coin_lockout_w(3,~data & 0x0080);
        /*
        set_led_status(0,data & 0x01);
        set_led_status(1,data & 0x10);
        set_led_status(2,data & 0x20);
        */
    }
}

READ16_HANDLER( cps2_qsound_volume_r )
{
    return 0xe021;
}

#ifdef A68K0

data8_t CPS2_Read8(offs_t address)
{
    return m68k_read_pcrelative_8(address);
}

data16_t CPS2_Read16(offs_t address)
{
    return m68k_read_pcrelative_16(address);
}

data32_t CPS2_Read32(offs_t address)
{
    return m68k_read_pcrelative_32(address);
}

void init_cps2_memory(void)
{
    // Patch Memory array with Decryption Calls

    m68000_memory_interface_set(8,(void *)CPS2_Read8);
    m68000_memory_interface_set(9,(void *)CPS2_Read16);
    m68000_memory_interface_set(10,(void *)CPS2_Read32);
    m68000_memory_interface_set(11,(void *)CPS2_Read16);
    m68000_memory_interface_set(12,(void *)CPS2_Read32);
}

#endif

void init_cps2(void)
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int i;
	const int decode=XOR_BASE;

#if 0
	FILE *fp;
	fp = fopen ("ROM.DMP", "w+b");
	if (fp)
	{
		for (i=0; i<decode; i+=2)
		{
			int value=READ_WORD(&RAM[i]);
			fputc(value>>8,   fp);
			fputc(value&0xff, fp);
		}
		fclose(fp);
	}
#endif

	if (RAM[decode])
	{
		/* user region exists, real CPS2 */

		for (i=0; i<decode; i++)
			RAM[decode+i] = RAM[i] ^ RAM[decode+i];

		memory_set_opcode_base(0, RAM+decode);
		memory_set_encrypted_opcode_range(0, 0, XOR_BASE);
	}
	else
	{
		/* no user region (XOR tables) use tile viewer */
		logerror("No user region defined, using tile viewer");
		cps_setversion(99);
	}

#if 0
	fp=fopen ("ROMD.DMP", "w+b");
	if (fp)
	{
		for (i=0; i<decode; i+=2)
		{
			int value=READ_WORD(&RAM[decode+i]);
			fputc(value>>8,   fp);
			fputc(value&0xff, fp);
		}

		fclose(fp);
	}
#endif

    if (cps_getversion() == 99)
    {
        /*
        Poke in a dummy program to stop the 68K core from crashing due
        to silly addresses.
        */
        WRITE_WORD(&RAM[0x0000], 0x00ff);
        WRITE_WORD(&RAM[0x0002], 0x8000);  /* Dummy stack pointer */
        WRITE_WORD(&RAM[0x0004], 0x0000);
        WRITE_WORD(&RAM[0x0006], 0x00c2);  /* Dummy start vector */

        for (i=0x0008; i<0x00c0; i+=4)
        {
            WRITE_WORD(&RAM[i+0], 0x0000);
            WRITE_WORD(&RAM[i+2], 0x00c0);
        }

        WRITE_WORD(&RAM[0x00c0], 0x4e73);   /* RETE */
        WRITE_WORD(&RAM[0x00c2], 0x6000);
        WRITE_WORD(&RAM[0x00c4], 0x00c2);   /* BRA 00c2 */

        for (i=0; i<decode; i++)
        {
            RAM[decode+i]=RAM[i];
        }

    }
}



static READ16_HANDLER( kludge_r )
{
	return 0xffff;
}


static MEMORY_READ16_START( cps2_readmem )
	{ 0x000000, 0x3fffff, MRA16_ROM },             /* 68000 ROM */
	{ 0x400000, 0x40000b, MRA16_RAM },             /* CPS2 object output */
	{ 0x618000, 0x619fff, qsound_sharedram1_r },   /* Q RAM */
	{ 0x660000, 0x663fff, MRA16_RAM },             /* Unknown */
	{ 0x664000, 0x664001, MRA16_RAM },             /* Unknown - accessed at routine 0xcf4a in SFZJ*/
	{ 0x708000, 0x70ffff, MRA16_RAM },             /* Object RAM */

	{ 0x800100, 0x8001ff, cps1_output_r },         /* Output ports mirror (sfa) */
	{ 0x804000, 0x804001, input_port_0_word_r },   /* IN0 */
	{ 0x804010, 0x804011, input_port_1_word_r },   /* IN1 */
	{ 0x804020, 0x804021, cps2_eeprom_port_r  },   /* IN2 + EEPROM */
	{ 0x804030, 0x804031, cps2_qsound_volume_r },  /* Master volume */
	{ 0x8040b0, 0x8040b3, kludge_r },  				/* unknown (xmcotaj hangs if this is 0) */
	{ 0x804100, 0x8041ff, cps1_output_r },         /* CPS1 Output ports */
	{ 0x900000, 0x92ffff, MRA16_RAM },             /* Video RAM */
	{ 0xff0000, 0xffffff, MRA16_RAM },             /* RAM */
MEMORY_END

static MEMORY_WRITE16_START( cps2_writemem )
	{ 0x000000, 0x3fffff, MWA16_ROM },             /* ROM */
	{ 0x400000, 0x40000b, MWA16_RAM, &cps2_output, &cps2_output_size },             /* CPS2 output */
	{ 0x618000, 0x619fff, qsound_sharedram1_w },   /* Q RAM */
	{ 0x660000, 0x663fff, MWA16_RAM },             /* Unknown */
	{ 0x664000, 0x664001, MWA16_RAM },             /* Unknown */
	{ 0x708000, 0x70ffff, MWA16_RAM, &cps2_objram, &cps2_objram_size },           /* Object RAM */
	{ 0x800100, 0x8001ff, cps1_output_w },         /* Output ports mirror (sfa) */
	{ 0x804040, 0x804041, cps2_eeprom_port_w },    /* EEPROM */
	{ 0x8040a0, 0x8040a1, MWA16_NOP },             /* Unknown (reset once on startup) */
	{ 0x8040e0, 0x8040e1, MWA16_NOP },       /* toggles bit 1 */
	{ 0x804100, 0x8041ff, cps1_output_w, &cps1_output, &cps1_output_size },  /* Output ports */

	{ 0x900000, 0x92ffff, MWA16_RAM, &cps1_gfxram, &cps1_gfxram_size },
	{ 0xff0000, 0xffffff, MWA16_RAM },             /* RAM */
MEMORY_END



INPUT_PORTS_START( ssf2 )
    PORT_START      /* IN0 (0x00) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )


    PORT_START      /* IN1 (0x10) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN2 (0x20) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SPECIAL )   /* EEPROM bit */
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
    PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( ddtod )
    PORT_START      /* IN0 (0x00) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )

    PORT_START      /* IN1 (0x10) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER3 )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER4 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER4 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER4 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER4 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER4 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER4 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER4 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER4 )

    PORT_START      /* IN2 (0x20) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SPECIAL )   /* EEPROM bit */
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
    PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START3 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START4 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN3 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_COIN4 )
INPUT_PORTS_END

INPUT_PORTS_START( avsp )
    PORT_START      /* IN0 (0x00) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN1 (0x10) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER3 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER3 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER3 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER3 )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN2 (0x20) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SPECIAL )   /* EEPROM bit */
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
    PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_START3 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_COIN3 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END

INPUT_PORTS_START( cps2 )
    PORT_START      /* IN0 (0x00) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )


    PORT_START      /* IN1 (0x10) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER1 )
    PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER1 )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER1 )
    PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
    PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON5 | IPF_PLAYER2 )
    PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNKNOWN )

    PORT_START      /* IN2 (0x20) */
    PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_SPECIAL )   /* EEPROM bit */
	PORT_BITX(0x0002, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
    PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
    PORT_BIT( 0x00f8, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
    PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
    PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_COIN1 )
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN2 )
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON6 | IPF_PLAYER2 )
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static const struct MachineDriver machine_driver_cps2 =
{
	/* basic machine hardware */
	{
		{
            CPU_M68000,
			16000000,
            cps2_readmem,cps2_writemem,0,0,
			cps2_interrupt, 1	// 262  /* ??? interrupts per frame */
		},
		{
            CPU_Z80,
            8000000,  /* 6 MHz ??? TODO: find real FRQ */
			qsound_readmem,qsound_writemem,0,0,
			0,0,
			interrupt,250	/* ?? */
		}
	},
    60, DEFAULT_60HZ_VBLANK_DURATION,
	1,
#ifdef A68K0
    init_cps2_memory,
#else
    0,
#endif

	/* video hardware */
	0x30*8+32*2, 0x1c*8+32*3, { 32, 32+0x30*8-1, 32+16, 32+16+0x1c*8-1 },

	cps1_gfxdecodeinfo,
    4096,
    4096,
	0,

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
    cps1_eof_callback,
    cps2_vh_start,
    cps1_vh_stop,
    cps1_vh_screenrefresh,

	/* sound hardware */
	SOUND_SUPPORTS_STEREO,0,0,0,
	{
		{
			SOUND_QSOUND,
			&qsound_interface
		}
    },
    cps2_nvram_handler
};



ROM_START( 19xx )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "19xu.03", 0x000000, 0x80000, 0x05955268 )
	ROM_LOAD16_WORD_SWAP( "19xu.04", 0x080000, 0x80000, 0x3111ab7f )
	ROM_LOAD16_WORD_SWAP( "19xu.05", 0x100000, 0x80000, 0x38df4a63 )
	ROM_LOAD16_WORD_SWAP( "19xu.06", 0x180000, 0x80000, 0x5c7e60d3 )
	ROM_LOAD16_WORD_SWAP( "19x.07",  0x200000, 0x80000, 0x61c0296c )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "19x.13",   0x0600000, 0x080000, 0x427aeb18, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.15",   0x0600002, 0x080000, 0x63bdbf54, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.17",   0x0600004, 0x080000, 0x2dfe18b5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.19",   0x0600006, 0x080000, 0xcbef9579, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.14",   0x0800000, 0x200000, 0xe916967c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.16",   0x0800002, 0x200000, 0x6e75f3db, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.18",   0x0800004, 0x200000, 0x2213e798, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.20",   0x0800006, 0x200000, 0xab9d5b96, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "19x.01",   0x00000, 0x08000, 0xef55195e )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "19x.11",   0x000000, 0x200000, 0xd38beef3 )
	ROM_LOAD16_WORD_SWAP( "19x.12",   0x200000, 0x200000, 0xd47c96e2 )
ROM_END

ROM_START( 19xxj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "19xj.03", 0x000000, 0x80000, 0x26a381ed )
	ROM_LOAD16_WORD_SWAP( "19xj.04", 0x080000, 0x80000, 0x30100cca )
	ROM_LOAD16_WORD_SWAP( "19xj.05", 0x100000, 0x80000, 0xde67e938 )
	ROM_LOAD16_WORD_SWAP( "19xj.06", 0x180000, 0x80000, 0x39f9a409 )
	ROM_LOAD16_WORD_SWAP( "19x.07",  0x200000, 0x80000, 0x61c0296c )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "19x.13",   0x0600000, 0x080000, 0x427aeb18, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.15",   0x0600002, 0x080000, 0x63bdbf54, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.17",   0x0600004, 0x080000, 0x2dfe18b5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.19",   0x0600006, 0x080000, 0xcbef9579, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.14",   0x0800000, 0x200000, 0xe916967c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.16",   0x0800002, 0x200000, 0x6e75f3db, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.18",   0x0800004, 0x200000, 0x2213e798, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.20",   0x0800006, 0x200000, 0xab9d5b96, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "19x.01",   0x00000, 0x08000, 0xef55195e )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "19x.11",   0x000000, 0x200000, 0xd38beef3 )
	ROM_LOAD16_WORD_SWAP( "19x.12",   0x200000, 0x200000, 0xd47c96e2 )
ROM_END

ROM_START( 19xxh )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "19xh.03a", 0x000000, 0x80000, 0x357be2ac )
	ROM_LOAD16_WORD_SWAP( "19xh.04a", 0x080000, 0x80000, 0xbb13ea3b )
	ROM_LOAD16_WORD_SWAP( "19xh.05a", 0x100000, 0x80000, 0xcbd76601 )
	ROM_LOAD16_WORD_SWAP( "19xh.06a", 0x180000, 0x80000, 0xb362de8b )
	ROM_LOAD16_WORD_SWAP( "19x.07",   0x200000, 0x80000, 0x61c0296c )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "19x.13",   0x0600000, 0x080000, 0x427aeb18, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.15",   0x0600002, 0x080000, 0x63bdbf54, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.17",   0x0600004, 0x080000, 0x2dfe18b5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.19",   0x0600006, 0x080000, 0xcbef9579, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.14",   0x0800000, 0x200000, 0xe916967c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.16",   0x0800002, 0x200000, 0x6e75f3db, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.18",   0x0800004, 0x200000, 0x2213e798, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "19x.20",   0x0800006, 0x200000, 0xab9d5b96, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "19xh.01",  0x00000, 0x08000, 0xd686b1bb )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "19x.11",   0x000000, 0x200000, 0xd38beef3 )
	ROM_LOAD16_WORD_SWAP( "19x.12",   0x200000, 0x200000, 0xd47c96e2 )
ROM_END

ROM_START( armwar )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pwgu.03b", 0x000000, 0x80000, 0x8b95497a )
	ROM_LOAD16_WORD_SWAP( "pwgu.04b", 0x080000, 0x80000, 0x29eb5661 )
	ROM_LOAD16_WORD_SWAP( "pwgu.05b", 0x100000, 0x80000, 0xa54e9e44 )
	ROM_LOAD16_WORD_SWAP( "pwg.06",   0x180000, 0x80000, 0x87a60ce8 )
	ROM_LOAD16_WORD_SWAP( "pwg.07",   0x200000, 0x80000, 0xf7b148df )
	ROM_LOAD16_WORD_SWAP( "pwg.08",   0x280000, 0x80000, 0xcc62823e )
	ROM_LOAD16_WORD_SWAP( "pwg.09a",  0x300000, 0x80000, 0x4c26baee )
	ROM_LOAD16_WORD_SWAP( "pwg.10",   0x380000, 0x80000, 0x07c4fb28 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "pwg.13",   0x0000000, 0x400000, 0xae8fe08e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.15",   0x0000002, 0x400000, 0xdb560f58, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.17",   0x0000004, 0x400000, 0xbc475b94, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.19",   0x0000006, 0x400000, 0x07439ff7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.14",   0x1000000, 0x100000, 0xc3f9ba63, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.16",   0x1000002, 0x100000, 0x815b0e7b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.18",   0x1000004, 0x100000, 0x0109c71b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.20",   0x1000006, 0x100000, 0xeb75ffbe, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pwg.01",   0x00000, 0x08000, 0x18a5c0e4 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pwg.02",   0x28000, 0x20000, 0xc9dfffa6 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pwg.11",   0x000000, 0x200000, 0xa78f7433 )
	ROM_LOAD16_WORD_SWAP( "pwg.12",   0x200000, 0x200000, 0x77438ed0 )
ROM_END

ROM_START( pgear )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pwgj.03", 0x000000, 0x80000, 0xf264e74b )
	ROM_LOAD16_WORD_SWAP( "pwgj.04", 0x080000, 0x80000, 0x23a84983 )
	ROM_LOAD16_WORD_SWAP( "pwgj.05", 0x100000, 0x80000, 0xbef58c62 )
	ROM_LOAD16_WORD_SWAP( "pwg.06",  0x180000, 0x80000, 0x87a60ce8 )
	ROM_LOAD16_WORD_SWAP( "pwg.07",  0x200000, 0x80000, 0xf7b148df )
	ROM_LOAD16_WORD_SWAP( "pwg.08",  0x280000, 0x80000, 0xcc62823e )
	ROM_LOAD16_WORD_SWAP( "pwg.09",  0x300000, 0x80000, 0xddc85ca6 )
	ROM_LOAD16_WORD_SWAP( "pwg.10",  0x380000, 0x80000, 0x07c4fb28 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "pwg.13",   0x0000000, 0x400000, 0xae8fe08e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.15",   0x0000002, 0x400000, 0xdb560f58, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.17",   0x0000004, 0x400000, 0xbc475b94, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.19",   0x0000006, 0x400000, 0x07439ff7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.14",   0x1000000, 0x100000, 0xc3f9ba63, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.16",   0x1000002, 0x100000, 0x815b0e7b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.18",   0x1000004, 0x100000, 0x0109c71b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.20",   0x1000006, 0x100000, 0xeb75ffbe, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pwg.01",   0x00000, 0x08000, 0x18a5c0e4 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pwg.02",   0x28000, 0x20000, 0xc9dfffa6 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pwg.11",   0x000000, 0x200000, 0xa78f7433 )
	ROM_LOAD16_WORD_SWAP( "pwg.12",   0x200000, 0x200000, 0x77438ed0 )
ROM_END

ROM_START( armwara )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pwga.03a", 0x000000, 0x80000, 0x8d474ab1 )
	ROM_LOAD16_WORD_SWAP( "pwga.04a", 0x080000, 0x80000, 0x81b5aec7 )
	ROM_LOAD16_WORD_SWAP( "pwga.05a", 0x100000, 0x80000, 0x2618e819 )
	ROM_LOAD16_WORD_SWAP( "pwg.06",   0x180000, 0x80000, 0x87a60ce8 )
	ROM_LOAD16_WORD_SWAP( "pwg.07",   0x200000, 0x80000, 0xf7b148df )
	ROM_LOAD16_WORD_SWAP( "pwg.08",   0x280000, 0x80000, 0xcc62823e )
	ROM_LOAD16_WORD_SWAP( "pwg.09",   0x300000, 0x80000, 0xddc85ca6 )
	ROM_LOAD16_WORD_SWAP( "pwg.10",   0x380000, 0x80000, 0x07c4fb28 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "pwg.13",   0x0000000, 0x400000, 0xae8fe08e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.15",   0x0000002, 0x400000, 0xdb560f58, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.17",   0x0000004, 0x400000, 0xbc475b94, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.19",   0x0000006, 0x400000, 0x07439ff7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.14",   0x1000000, 0x100000, 0xc3f9ba63, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.16",   0x1000002, 0x100000, 0x815b0e7b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.18",   0x1000004, 0x100000, 0x0109c71b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pwg.20",   0x1000006, 0x100000, 0xeb75ffbe, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pwg.01",   0x00000, 0x08000, 0x18a5c0e4 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pwg.02",   0x28000, 0x20000, 0xc9dfffa6 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pwg.11",   0x000000, 0x200000, 0xa78f7433 )
	ROM_LOAD16_WORD_SWAP( "pwg.12",   0x200000, 0x200000, 0x77438ed0 )
ROM_END

ROM_START( avsp )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "avpu.03d", 0x000000, 0x80000, 0x42757950 )
	ROM_LOAD16_WORD_SWAP( "avpu.04d", 0x080000, 0x80000, 0x5abcdee6 )
	ROM_LOAD16_WORD_SWAP( "avp.05d",  0x100000, 0x80000, 0xfbfb5d7a )
	ROM_LOAD16_WORD_SWAP( "avp.06",   0x180000, 0x80000, 0x190b817f )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "avpux.03d", XOR_BASE+0x000000, 0x80000, 0xd5b01046 )
	ROM_LOAD16_WORD_SWAP( "avpux.04d", XOR_BASE+0x080000, 0x80000, 0x94bd7603 )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "avp.13",   0x0000000, 0x200000, 0x8f8b5ae4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.15",   0x0000002, 0x200000, 0xb00280df, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.17",   0x0000004, 0x200000, 0x94403195, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.19",   0x0000006, 0x200000, 0xe1981245, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.14",   0x0800000, 0x200000, 0xebba093e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.16",   0x0800002, 0x200000, 0xfb228297, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.18",   0x0800004, 0x200000, 0x34fb7232, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.20",   0x0800006, 0x200000, 0xf90baa21, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "avp.01",   0x00000, 0x08000, 0x2d3b4220 )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "avp.11",   0x000000, 0x200000, 0x83499817 )
	ROM_LOAD16_WORD_SWAP( "avp.12",   0x200000, 0x200000, 0xf4110d49 )
ROM_END

ROM_START( avspa )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "avpa.03d", 0x000000, 0x80000, 0x6c1c1858 )
	ROM_LOAD16_WORD_SWAP( "avpa.04d", 0x080000, 0x80000, 0x94f50b0c )
	ROM_LOAD16_WORD_SWAP( "avp.05d",  0x100000, 0x80000, 0xfbfb5d7a )
	ROM_LOAD16_WORD_SWAP( "avp.06",   0x180000, 0x80000, 0x190b817f )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "avpax.03d", XOR_BASE+0x000000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "avpax.04d", XOR_BASE+0x080000, 0x80000, 0x00000000 )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "avp.13",   0x0000000, 0x200000, 0x8f8b5ae4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.15",   0x0000002, 0x200000, 0xb00280df, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.17",   0x0000004, 0x200000, 0x94403195, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.19",   0x0000006, 0x200000, 0xe1981245, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.14",   0x0800000, 0x200000, 0xebba093e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.16",   0x0800002, 0x200000, 0xfb228297, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.18",   0x0800004, 0x200000, 0x34fb7232, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "avp.20",   0x0800006, 0x200000, 0xf90baa21, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "avp.01",   0x00000, 0x08000, 0x2d3b4220 )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "avp.11",   0x000000, 0x200000, 0x83499817 )
	ROM_LOAD16_WORD_SWAP( "avp.12",   0x200000, 0x200000, 0xf4110d49 )
ROM_END

ROM_START( batcirj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "btcj.03", 0x000000, 0x80000, 0x6b7e168d )
	ROM_LOAD16_WORD_SWAP( "btcj.04", 0x080000, 0x80000, 0x46ba3467 )
	ROM_LOAD16_WORD_SWAP( "btcj.05", 0x100000, 0x80000, 0x0e23a859 )
	ROM_LOAD16_WORD_SWAP( "btcj.06", 0x180000, 0x80000, 0xa853b59c )
	ROM_LOAD16_WORD_SWAP( "btc.07",  0x200000, 0x80000, 0x7322d5db )
	ROM_LOAD16_WORD_SWAP( "btc.08",  0x280000, 0x80000, 0x6aac85ab )
	ROM_LOAD16_WORD_SWAP( "btc.09",  0x300000, 0x80000, 0x1203db08 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "btc.13",   0x000000, 0x400000, 0xdc705bad, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "btc.15",   0x000002, 0x400000, 0xe5779a3c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "btc.17",   0x000004, 0x400000, 0xb33f4112, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "btc.19",   0x000006, 0x400000, 0xa6fcdb7e, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "btc.01",   0x00000, 0x08000, 0x1e194310 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "btc.02",   0x28000, 0x20000, 0x01aeb8e6 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "btc.11",   0x000000, 0x200000, 0xc27f2229 )
	ROM_LOAD16_WORD_SWAP( "btc.12",   0x200000, 0x200000, 0x418a2e33 )
ROM_END

ROM_START( batcira )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "btca.03", 0x000000, 0x80000, 0x1ad20d87 )
	ROM_LOAD16_WORD_SWAP( "btca.04", 0x080000, 0x80000, 0x2b3f4dbe )
	ROM_LOAD16_WORD_SWAP( "btca.05", 0x100000, 0x80000, 0x8238a3d9 )
	ROM_LOAD16_WORD_SWAP( "btca.06", 0x180000, 0x80000, 0x446c7c02 )
	ROM_LOAD16_WORD_SWAP( "btc.07",  0x200000, 0x80000, 0x7322d5db )
	ROM_LOAD16_WORD_SWAP( "btc.08",  0x280000, 0x80000, 0x6aac85ab )
	ROM_LOAD16_WORD_SWAP( "btc.09",  0x300000, 0x80000, 0x1203db08 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "btc.13",   0x000000, 0x400000, 0xdc705bad, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "btc.15",   0x000002, 0x400000, 0xe5779a3c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "btc.17",   0x000004, 0x400000, 0xb33f4112, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "btc.19",   0x000006, 0x400000, 0xa6fcdb7e, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "btc.01",   0x00000, 0x08000, 0x1e194310 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "btc.02",   0x28000, 0x20000, 0x01aeb8e6 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "btc.11",   0x000000, 0x200000, 0xc27f2229 )
	ROM_LOAD16_WORD_SWAP( "btc.12",   0x200000, 0x200000, 0x418a2e33 )
ROM_END

ROM_START( cybotsj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "cybj.03", 0x000000, 0x80000, 0x6096eada )
	ROM_LOAD16_WORD_SWAP( "cybj.04", 0x080000, 0x80000, 0x7b0ffaa9 )
	ROM_LOAD16_WORD_SWAP( "cybj.05", 0x100000, 0x80000, 0xec40408e )
	ROM_LOAD16_WORD_SWAP( "cybj.06", 0x180000, 0x80000, 0x1ad0bed2 )
	ROM_LOAD16_WORD_SWAP( "cybj.07", 0x200000, 0x80000, 0x6245a39a )
	ROM_LOAD16_WORD_SWAP( "cybj.08", 0x280000, 0x80000, 0x4b48e223 )
	ROM_LOAD16_WORD_SWAP( "cybj.09", 0x300000, 0x80000, 0xe15238f6 )
	ROM_LOAD16_WORD_SWAP( "cybj.10", 0x380000, 0x80000, 0x75f4003b )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "cyb.13",   0x0000000, 0x400000, 0x49d1de79, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.15",   0x0000002, 0x400000, 0x3852535f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.17",   0x0000004, 0x400000, 0x514a5ae0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.19",   0x0000006, 0x400000, 0x74d6327e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.14",   0x1000000, 0x400000, 0x15c339d0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.16",   0x1000002, 0x400000, 0xB6b56ca4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.18",   0x1000004, 0x400000, 0x06a05c14, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "cyb.20",   0x1000006, 0x400000, 0x3c9d7033, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "cyb.01",   0x00000, 0x08000, 0x9c0fb079 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "cyb.02",   0x28000, 0x20000, 0x51cb0c4e )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "cyb.11",   0x000000, 0x200000, 0x362ccab2 )
	ROM_LOAD16_WORD_SWAP( "cyb.12",   0x200000, 0x200000, 0x7066e9cc )
ROM_END

ROM_START( ddtod )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "dadu.03b", 0x000000, 0x80000, 0xa519905f )
	ROM_LOAD16_WORD_SWAP( "dadu.04b", 0x080000, 0x80000, 0x52562d38 )
	ROM_LOAD16_WORD_SWAP( "dadu.05b", 0x100000, 0x80000, 0xee1cfbfe )
	ROM_LOAD16_WORD_SWAP( "dad.06",   0x180000, 0x80000, 0x13aa3e56 )
	ROM_LOAD16_WORD_SWAP( "dad.07",   0x200000, 0x80000, 0x431cb6dd )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "dadux.03b", XOR_BASE+0x000000, 0x80000, 0xf59ee70c )
	ROM_LOAD16_WORD_SWAP( "dadux.04b", XOR_BASE+0x080000, 0x80000, 0x622628ae )
	ROM_LOAD16_WORD_SWAP( "dadux.05b", XOR_BASE+0x100000, 0x80000, 0x424bd6e3 )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "dad.13",   0x000000, 0x200000, 0xda3cb7d6, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.15",   0x000002, 0x200000, 0x92b63172, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.17",   0x000004, 0x200000, 0xb98757f5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.19",   0x000006, 0x200000, 0x8121ce46, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.14",   0x800000, 0x100000, 0x837e6f3f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.16",   0x800002, 0x100000, 0xf0916bdb, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.18",   0x800004, 0x100000, 0xcef393ef, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.20",   0x800006, 0x100000, 0x8953fe9e, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dad.01",   0x00000, 0x08000, 0x3f5e2424 )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dad.11",   0x000000, 0x200000, 0x0c499b67 )
	ROM_LOAD16_WORD_SWAP( "dad.12",   0x200000, 0x200000, 0x2f0b5a4e )
ROM_END

ROM_START( ddtodj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "dadj.03a", 0x000000, 0x80000, 0x711638dc )
	ROM_LOAD16_WORD_SWAP( "dadj.04a", 0x080000, 0x80000, 0x4869639c )
	ROM_LOAD16_WORD_SWAP( "dadj.05a", 0x100000, 0x80000, 0x484c0efa )
	ROM_LOAD16_WORD_SWAP( "dad.06",   0x180000, 0x80000, 0x13aa3e56 )
	ROM_LOAD16_WORD_SWAP( "dad.07",   0x200000, 0x80000, 0x431cb6dd )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "dadjx.03a", XOR_BASE+0x000000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "dadjx.04a", XOR_BASE+0x080000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "dadjx.05a", XOR_BASE+0x100000, 0x80000, 0x00000000 )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "dad.13",   0x000000, 0x200000, 0xda3cb7d6, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.15",   0x000002, 0x200000, 0x92b63172, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.17",   0x000004, 0x200000, 0xb98757f5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.19",   0x000006, 0x200000, 0x8121ce46, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.14",   0x800000, 0x100000, 0x837e6f3f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.16",   0x800002, 0x100000, 0xf0916bdb, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.18",   0x800004, 0x100000, 0xcef393ef, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.20",   0x800006, 0x100000, 0x8953fe9e, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dad.01",   0x00000, 0x08000, 0x3f5e2424 )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dad.11",   0x000000, 0x200000, 0x0c499b67 )
	ROM_LOAD16_WORD_SWAP( "dad.12",   0x200000, 0x200000, 0x2f0b5a4e )
ROM_END

ROM_START( ddtoda )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "dada.03a", 0x000000, 0x80000, 0xfc6f2dd7 )
	ROM_LOAD16_WORD_SWAP( "dada.04a", 0x080000, 0x80000, 0xd4be4009 )
	ROM_LOAD16_WORD_SWAP( "dada.05a", 0x100000, 0x80000, 0x6712d1cf )
	ROM_LOAD16_WORD_SWAP( "dad.06",   0x180000, 0x80000, 0x13aa3e56 )
	ROM_LOAD16_WORD_SWAP( "dad.07",   0x200000, 0x80000, 0x431cb6dd )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "dadax.03a", XOR_BASE+0x000000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "dadax.04a", XOR_BASE+0x080000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "dadax.05a", XOR_BASE+0x100000, 0x80000, 0x00000000 )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "dad.13",   0x000000, 0x200000, 0xda3cb7d6, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.15",   0x000002, 0x200000, 0x92b63172, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.17",   0x000004, 0x200000, 0xb98757f5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.19",   0x000006, 0x200000, 0x8121ce46, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.14",   0x800000, 0x100000, 0x837e6f3f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.16",   0x800002, 0x100000, 0xf0916bdb, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.18",   0x800004, 0x100000, 0xcef393ef, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.20",   0x800006, 0x100000, 0x8953fe9e, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dad.01",   0x00000, 0x08000, 0x3f5e2424 )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dad.11",   0x000000, 0x200000, 0x0c499b67 )
	ROM_LOAD16_WORD_SWAP( "dad.12",   0x200000, 0x200000, 0x2f0b5a4e )
ROM_END

ROM_START( ddtodr1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
  	ROM_LOAD16_WORD_SWAP( "dadu.03a", 0x000000, 0x80000, 0x4413f177 )
	ROM_LOAD16_WORD_SWAP( "dadu.04a", 0x080000, 0x80000, 0x168de230 )
	ROM_LOAD16_WORD_SWAP( "dadu.05a", 0x100000, 0x80000, 0x03d39e91 )
	ROM_LOAD16_WORD_SWAP( "dad.06",   0x180000, 0x80000, 0x13aa3e56 )
	ROM_LOAD16_WORD_SWAP( "dad.07",   0x200000, 0x80000, 0x431cb6dd )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "dadux.03a", XOR_BASE+0x000000, 0x80000, 0xf9ba14b6 )
	ROM_LOAD16_WORD_SWAP( "dadux.04a", XOR_BASE+0x080000, 0x80000, 0xed85ec29 )
	ROM_LOAD16_WORD_SWAP( "dadux.05a", XOR_BASE+0x100000, 0x80000, 0xdbae3d1b )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "dad.13",   0x000000, 0x200000, 0xda3cb7d6, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.15",   0x000002, 0x200000, 0x92b63172, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.17",   0x000004, 0x200000, 0xb98757f5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.19",   0x000006, 0x200000, 0x8121ce46, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.14",   0x800000, 0x100000, 0x837e6f3f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.16",   0x800002, 0x100000, 0xf0916bdb, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.18",   0x800004, 0x100000, 0xcef393ef, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dad.20",   0x800006, 0x100000, 0x8953fe9e, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dad.01",   0x00000, 0x08000, 0x3f5e2424 )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dad.11",   0x000000, 0x200000, 0x0c499b67 )
	ROM_LOAD16_WORD_SWAP( "dad.12",   0x200000, 0x200000, 0x2f0b5a4e )
ROM_END

ROM_START( ddsom )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "dd2u.03d", 0x000000, 0x80000, 0x0f700d84 )
	ROM_LOAD16_WORD_SWAP( "dd2u.04d", 0x080000, 0x80000, 0xb99eb254 )
	ROM_LOAD16_WORD_SWAP( "dd2u.05d", 0x100000, 0x80000, 0xb23061f3 )
	ROM_LOAD16_WORD_SWAP( "dd2u.06d", 0x180000, 0x80000, 0x8bf1d8ce )
	ROM_LOAD16_WORD_SWAP( "dd2.07",   0x200000, 0x80000, 0x909a0b8b )
	ROM_LOAD16_WORD_SWAP( "dd2.08",   0x280000, 0x80000, 0xe53c4d01 )
	ROM_LOAD16_WORD_SWAP( "dd2.09",   0x300000, 0x80000, 0x5f86279f )
	ROM_LOAD16_WORD_SWAP( "dd2.10",   0x380000, 0x80000, 0xad954c26 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x1800000, REGION_GFX1, 0 )
	ROMX_LOAD( "dd2.13",   0x0000000, 0x400000, 0xa46b4e6e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.15",   0x0000002, 0x400000, 0xd5fc50fc, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.17",   0x0000004, 0x400000, 0x837c0867, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.19",   0x0000006, 0x400000, 0xbb0ec21c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.14",   0x1000000, 0x200000, 0x6d824ce2, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.16",   0x1000002, 0x200000, 0x79682ae5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.18",   0x1000004, 0x200000, 0xacddd149, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.20",   0x1000006, 0x200000, 0x117fb0c0, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dd2.01",   0x00000, 0x08000, 0x99d657e5 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "dd2.02",   0x28000, 0x20000, 0x117a3824 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dd2.11",   0x000000, 0x200000, 0x98d0c325 )
	ROM_LOAD16_WORD_SWAP( "dd2.12",   0x200000, 0x200000, 0x5ea2e7fa )
ROM_END

ROM_START( ddsomj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "dd2j.03b", 0x000000, 0x80000, 0x965d74e5 )
	ROM_LOAD16_WORD_SWAP( "dd2j.04b", 0x080000, 0x80000, 0x958eb8f3 )
	ROM_LOAD16_WORD_SWAP( "dd2j.05b", 0x100000, 0x80000, 0xd38571ca )
	ROM_LOAD16_WORD_SWAP( "dd2j.06b", 0x180000, 0x80000, 0x6d5a3bbb )
	ROM_LOAD16_WORD_SWAP( "dd2.07",   0x200000, 0x80000, 0x909a0b8b )
	ROM_LOAD16_WORD_SWAP( "dd2.08",   0x280000, 0x80000, 0xe53c4d01 )
	ROM_LOAD16_WORD_SWAP( "dd2.09",   0x300000, 0x80000, 0x5f86279f )
	ROM_LOAD16_WORD_SWAP( "dd2.10",   0x380000, 0x80000, 0xad954c26 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x1800000, REGION_GFX1, 0 )
	ROMX_LOAD( "dd2.13",   0x0000000, 0x400000, 0xa46b4e6e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.15",   0x0000002, 0x400000, 0xd5fc50fc, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.17",   0x0000004, 0x400000, 0x837c0867, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.19",   0x0000006, 0x400000, 0xbb0ec21c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.14",   0x1000000, 0x200000, 0x6d824ce2, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.16",   0x1000002, 0x200000, 0x79682ae5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.18",   0x1000004, 0x200000, 0xacddd149, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "dd2.20",   0x1000006, 0x200000, 0x117fb0c0, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "dd2.01",   0x00000, 0x08000, 0x99d657e5 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "dd2.02",   0x28000, 0x20000, 0x117a3824 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "dd2.11",   0x000000, 0x200000, 0x98d0c325 )
	ROM_LOAD16_WORD_SWAP( "dd2.12",   0x200000, 0x200000, 0x5ea2e7fa )
ROM_END

ROM_START( dstlk )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vamu.03b", 0x000000, 0x80000, 0x68a6343f )
	ROM_LOAD16_WORD_SWAP( "vamu.04b", 0x080000, 0x80000, 0x58161453 )
	ROM_LOAD16_WORD_SWAP( "vamu.05b", 0x100000, 0x80000, 0xdfc038b8 )
	ROM_LOAD16_WORD_SWAP( "vamu.06b", 0x180000, 0x80000, 0xc3842c89 )
	ROM_LOAD16_WORD_SWAP( "vamu.07b", 0x200000, 0x80000, 0x25b60b6e )
	ROM_LOAD16_WORD_SWAP( "vamu.08b", 0x280000, 0x80000, 0x2113c596 )
	ROM_LOAD16_WORD_SWAP( "vamu.09b", 0x300000, 0x80000, 0x2d1e9ae5 )
	ROM_LOAD16_WORD_SWAP( "vamu.10b", 0x380000, 0x80000, 0x81145622 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "vamux.03b", XOR_BASE+0x000000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "vamux.04b", XOR_BASE+0x080000, 0x80000, 0x00000000 )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "vam.13",   0x0000000, 0x400000, 0xc51baf99, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.15",   0x0000002, 0x400000, 0x3ce83c77, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.17",   0x0000004, 0x400000, 0x4f2408e0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.19",   0x0000006, 0x400000, 0x9ff60250, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.14",   0x1000000, 0x100000, 0xbd87243c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.16",   0x1000002, 0x100000, 0xafec855f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.18",   0x1000004, 0x100000, 0x3a033625, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.20",   0x1000006, 0x100000, 0x2bff6a89, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vam.01",   0x00000, 0x08000, 0x64b685d5 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vam.02",   0x28000, 0x20000, 0xcf7c97c7 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vam.11",   0x000000, 0x200000, 0x4a39deb2 )
	ROM_LOAD16_WORD_SWAP( "vam.12",   0x200000, 0x200000, 0x1a3e5c03 )
ROM_END

ROM_START( vampj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vamj.03a", 0x000000, 0x80000, 0xf55d3722 )
	ROM_LOAD16_WORD_SWAP( "vamj.04b", 0x080000, 0x80000, 0x4d9c43c4 )
	ROM_LOAD16_WORD_SWAP( "vamj.05a", 0x100000, 0x80000, 0x6c497e92 )
	ROM_LOAD16_WORD_SWAP( "vamj.06a", 0x180000, 0x80000, 0xf1bbecb6 )
	ROM_LOAD16_WORD_SWAP( "vamj.07a", 0x200000, 0x80000, 0x1067ad84 )
	ROM_LOAD16_WORD_SWAP( "vamj.08a", 0x280000, 0x80000, 0x4b89f41f )
	ROM_LOAD16_WORD_SWAP( "vamj.09a", 0x300000, 0x80000, 0xfc0a4aac )
	ROM_LOAD16_WORD_SWAP( "vamj.10a", 0x380000, 0x80000, 0x9270c26b )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "vamjx.03a", XOR_BASE+0x000000, 0x80000, 0x2549f7bc )
	ROM_LOAD16_WORD_SWAP( "vamjx.04b", XOR_BASE+0x080000, 0x80000, 0xbb5a30a5 )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "vam.13",   0x0000000, 0x400000, 0xc51baf99, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.15",   0x0000002, 0x400000, 0x3ce83c77, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.17",   0x0000004, 0x400000, 0x4f2408e0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.19",   0x0000006, 0x400000, 0x9ff60250, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.14",   0x1000000, 0x100000, 0xbd87243c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.16",   0x1000002, 0x100000, 0xafec855f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.18",   0x1000004, 0x100000, 0x3a033625, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.20",   0x1000006, 0x100000, 0x2bff6a89, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vam.01",   0x00000, 0x08000, 0x64b685d5 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vam.02",   0x28000, 0x20000, 0xcf7c97c7 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vam.11",   0x000000, 0x200000, 0x4a39deb2 )
	ROM_LOAD16_WORD_SWAP( "vam.12",   0x200000, 0x200000, 0x1a3e5c03 )
ROM_END

ROM_START( vampja )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vamj.03a", 0x000000, 0x80000, 0xf55d3722 )
	ROM_LOAD16_WORD_SWAP( "vamj.04a", 0x080000, 0x80000, 0xfdcbdae3 )
	ROM_LOAD16_WORD_SWAP( "vamj.05a", 0x100000, 0x80000, 0x6c497e92 )
	ROM_LOAD16_WORD_SWAP( "vamj.06a", 0x180000, 0x80000, 0xf1bbecb6 )
	ROM_LOAD16_WORD_SWAP( "vamj.07a", 0x200000, 0x80000, 0x1067ad84 )
	ROM_LOAD16_WORD_SWAP( "vamj.08a", 0x280000, 0x80000, 0x4b89f41f )
	ROM_LOAD16_WORD_SWAP( "vamj.09a", 0x300000, 0x80000, 0xfc0a4aac )
	ROM_LOAD16_WORD_SWAP( "vamj.10a", 0x380000, 0x80000, 0x9270c26b )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "vamjx.03a", XOR_BASE+0x000000, 0x80000, 0x2549f7bc )
	ROM_LOAD16_WORD_SWAP( "vamjx.04a", XOR_BASE+0x080000, 0x80000, 0xfe64a5cf )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "vam.13",   0x0000000, 0x400000, 0xc51baf99, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.15",   0x0000002, 0x400000, 0x3ce83c77, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.17",   0x0000004, 0x400000, 0x4f2408e0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.19",   0x0000006, 0x400000, 0x9ff60250, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.14",   0x1000000, 0x100000, 0xbd87243c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.16",   0x1000002, 0x100000, 0xafec855f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.18",   0x1000004, 0x100000, 0x3a033625, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.20",   0x1000006, 0x100000, 0x2bff6a89, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vam.01",   0x00000, 0x08000, 0x64b685d5 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vam.02",   0x28000, 0x20000, 0xcf7c97c7 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vam.11",   0x000000, 0x200000, 0x4a39deb2 )
	ROM_LOAD16_WORD_SWAP( "vam.12",   0x200000, 0x200000, 0x1a3e5c03 )
ROM_END

ROM_START( vampa )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vama.03a", 0x000000, 0x80000, 0x294e0bec )
	ROM_LOAD16_WORD_SWAP( "vama.04a", 0x080000, 0x80000, 0xbc18e128 )
	ROM_LOAD16_WORD_SWAP( "vama.05a", 0x100000, 0x80000, 0xe709fa59 )
	ROM_LOAD16_WORD_SWAP( "vama.06a", 0x180000, 0x80000, 0x55e4d387 )
	ROM_LOAD16_WORD_SWAP( "vama.07a", 0x200000, 0x80000, 0x24e8f981 )
	ROM_LOAD16_WORD_SWAP( "vama.08a", 0x280000, 0x80000, 0x743f3a8e )
	ROM_LOAD16_WORD_SWAP( "vama.09a", 0x300000, 0x80000, 0x67fa5573 )
	ROM_LOAD16_WORD_SWAP( "vama.10a", 0x380000, 0x80000, 0x5e03d747 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "vamax.03a", XOR_BASE+0x000000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "vamax.04a", XOR_BASE+0x080000, 0x80000, 0x00000000 )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "vam.13",   0x0000000, 0x400000, 0xc51baf99, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.15",   0x0000002, 0x400000, 0x3ce83c77, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.17",   0x0000004, 0x400000, 0x4f2408e0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.19",   0x0000006, 0x400000, 0x9ff60250, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.14",   0x1000000, 0x100000, 0xbd87243c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.16",   0x1000002, 0x100000, 0xafec855f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.18",   0x1000004, 0x100000, 0x3a033625, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vam.20",   0x1000006, 0x100000, 0x2bff6a89, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vam.01",   0x00000, 0x08000, 0x64b685d5 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vam.02",   0x28000, 0x20000, 0xcf7c97c7 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vam.11",   0x000000, 0x200000, 0x4a39deb2 )
	ROM_LOAD16_WORD_SWAP( "vam.12",   0x200000, 0x200000, 0x1a3e5c03 )
ROM_END

ROM_START( ecofe )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "uece.03", 0x000000, 0x80000, 0xec2c1137 )
	ROM_LOAD16_WORD_SWAP( "uece.04", 0x080000, 0x80000, 0xb35f99db )
	ROM_LOAD16_WORD_SWAP( "uece.05", 0x100000, 0x80000, 0xd9d42d31 )
	ROM_LOAD16_WORD_SWAP( "uece.06", 0x180000, 0x80000, 0x9d9771cf )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "uec.13",   0x000000, 0x200000, 0xdcaf1436, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.15",   0x000002, 0x200000, 0x2807df41, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.17",   0x000004, 0x200000, 0x8a708d02, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.19",   0x000006, 0x200000, 0xde7be0ef, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.14",   0x800000, 0x100000, 0x1a003558, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.16",   0x800002, 0x100000, 0x4ff8a6f9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.18",   0x800004, 0x100000, 0xb167ae12, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.20",   0x800006, 0x100000, 0x1064bdc2, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "uec.01",   0x00000, 0x08000, 0xc235bd15 )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "uec.11",   0x000000, 0x200000, 0x81b25d39 )
	ROM_LOAD16_WORD_SWAP( "uec.12",   0x200000, 0x200000, 0x27729e52 )
ROM_END

ROM_START( uecology )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "uecj.03", 0x000000, 0x80000, 0x94c40a4c )
	ROM_LOAD16_WORD_SWAP( "uecj.04", 0x080000, 0x80000, 0x8d6e3a09 )
	ROM_LOAD16_WORD_SWAP( "uecj.05", 0x100000, 0x80000, 0x8604ecd7 )
	ROM_LOAD16_WORD_SWAP( "uecj.06", 0x180000, 0x80000, 0xb7e1d31f )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "uec.13",   0x000000, 0x200000, 0xdcaf1436, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.15",   0x000002, 0x200000, 0x2807df41, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.17",   0x000004, 0x200000, 0x8a708d02, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.19",   0x000006, 0x200000, 0xde7be0ef, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.14",   0x800000, 0x100000, 0x1a003558, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.16",   0x800002, 0x100000, 0x4ff8a6f9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.18",   0x800004, 0x100000, 0xb167ae12, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "uec.20",   0x800006, 0x100000, 0x1064bdc2, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "uec.01",   0x00000, 0x08000, 0xc235bd15 )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "uec.11",   0x000000, 0x200000, 0x81b25d39 )
	ROM_LOAD16_WORD_SWAP( "uec.12",   0x200000, 0x200000, 0x27729e52 )
ROM_END

ROM_START( msh )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mshu.03", 0x000000, 0x80000, 0xd2805bdd )
	ROM_LOAD16_WORD_SWAP( "mshu.04", 0x080000, 0x80000, 0x743f96ff )
	ROM_LOAD16_WORD_SWAP( "msh.05",  0x100000, 0x80000, 0x6a091b9e )
	ROM_LOAD16_WORD_SWAP( "msh.06",  0x180000, 0x80000, 0x803e3fa4 )
	ROM_LOAD16_WORD_SWAP( "msh.07",  0x200000, 0x80000, 0xc45f8e27 )
	ROM_LOAD16_WORD_SWAP( "msh.08",  0x280000, 0x80000, 0x9ca6f12c )
	ROM_LOAD16_WORD_SWAP( "msh.09",  0x300000, 0x80000, 0x82ec27af )
	ROM_LOAD16_WORD_SWAP( "msh.10",  0x380000, 0x80000, 0x8d931196 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "mshux.03", XOR_BASE+0x000000, 0x80000, 0x10bfc357 )
	ROM_LOAD16_WORD_SWAP( "mshux.04", XOR_BASE+0x080000, 0x80000, 0x871f0863 )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "msh.13",   0x0000000, 0x400000, 0x09d14566, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.15",   0x0000002, 0x400000, 0xee962057, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.17",   0x0000004, 0x400000, 0x604ece14, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.19",   0x0000006, 0x400000, 0x94a731e8, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.14",   0x1000000, 0x400000, 0x4197973e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.16",   0x1000002, 0x400000, 0x438da4a0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.18",   0x1000004, 0x400000, 0x4db92d94, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.20",   0x1000006, 0x400000, 0xa2b0c6c0, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "msh.01",   0x00000, 0x08000, 0xc976e6f9 )
	ROM_CONTINUE(         0x10000, 0x18000 )
    ROM_LOAD( "msh.02",   0x28000, 0x20000, 0xce67d0d9 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "msh.11",   0x000000, 0x200000, 0x37ac6d30 )
	ROM_LOAD16_WORD_SWAP( "msh.12",   0x200000, 0x200000, 0xde092570 )
ROM_END

ROM_START( mshj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mshj.03f", 0x000000, 0x80000, 0xff172fd2 )
	ROM_LOAD16_WORD_SWAP( "mshj.04f", 0x080000, 0x80000, 0xebbb205a )
	ROM_LOAD16_WORD_SWAP( "msh.05",   0x100000, 0x80000, 0x6a091b9e )
	ROM_LOAD16_WORD_SWAP( "msh.06",   0x180000, 0x80000, 0x803e3fa4 )
	ROM_LOAD16_WORD_SWAP( "msh.07",   0x200000, 0x80000, 0xc45f8e27 )
	ROM_LOAD16_WORD_SWAP( "msh.08",   0x280000, 0x80000, 0x9ca6f12c )
	ROM_LOAD16_WORD_SWAP( "msh.09",   0x300000, 0x80000, 0x82ec27af )
	ROM_LOAD16_WORD_SWAP( "msh.10",   0x380000, 0x80000, 0x8d931196 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "mshjx.03f", XOR_BASE+0x000000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "mshjx.04f", XOR_BASE+0x080000, 0x80000, 0x00000000 )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "msh.13",   0x0000000, 0x400000, 0x09d14566, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.15",   0x0000002, 0x400000, 0xee962057, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.17",   0x0000004, 0x400000, 0x604ece14, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.19",   0x0000006, 0x400000, 0x94a731e8, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.14",   0x1000000, 0x400000, 0x4197973e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.16",   0x1000002, 0x400000, 0x438da4a0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.18",   0x1000004, 0x400000, 0x4db92d94, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.20",   0x1000006, 0x400000, 0xa2b0c6c0, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "msh.01",   0x00000, 0x08000, 0xc976e6f9 )
	ROM_CONTINUE(         0x10000, 0x18000 )
    ROM_LOAD( "msh.02",   0x28000, 0x20000, 0xce67d0d9 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "msh.11",   0x000000, 0x200000, 0x37ac6d30 )
	ROM_LOAD16_WORD_SWAP( "msh.12",   0x200000, 0x200000, 0xde092570 )
ROM_END

ROM_START( mshh )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mshh.03c", 0x000000, 0x80000, 0x8d84b0fa )
	ROM_LOAD16_WORD_SWAP( "mshh.04c", 0x080000, 0x80000, 0xd638f601 )
	ROM_LOAD16_WORD_SWAP( "mshh.05a", 0x100000, 0x80000, 0xf37539e6 )
	ROM_LOAD16_WORD_SWAP( "msh.06",   0x180000, 0x80000, 0x803e3fa4 )
	ROM_LOAD16_WORD_SWAP( "msh.07",   0x200000, 0x80000, 0xc45f8e27 )
	ROM_LOAD16_WORD_SWAP( "msh.08",   0x280000, 0x80000, 0x9ca6f12c )
	ROM_LOAD16_WORD_SWAP( "msh.09",   0x300000, 0x80000, 0x82ec27af )
	ROM_LOAD16_WORD_SWAP( "msh.10",   0x380000, 0x80000, 0x8d931196 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "mshhx.03", XOR_BASE+0x000000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "mshhx.04", XOR_BASE+0x080000, 0x80000, 0x00000000 )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "msh.13",   0x0000000, 0x400000, 0x09d14566, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.15",   0x0000002, 0x400000, 0xee962057, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.17",   0x0000004, 0x400000, 0x604ece14, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.19",   0x0000006, 0x400000, 0x94a731e8, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.14",   0x1000000, 0x400000, 0x4197973e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.16",   0x1000002, 0x400000, 0x438da4a0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.18",   0x1000004, 0x400000, 0x4db92d94, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "msh.20",   0x1000006, 0x400000, 0xa2b0c6c0, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "msh.01",   0x00000, 0x08000, 0xc976e6f9 )
	ROM_CONTINUE(         0x10000, 0x18000 )
    ROM_LOAD( "msh.02",   0x28000, 0x20000, 0xce67d0d9 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "msh.11",   0x000000, 0x200000, 0x37ac6d30 )
	ROM_LOAD16_WORD_SWAP( "msh.12",   0x200000, 0x200000, 0xde092570 )
ROM_END

ROM_START( mshvsf )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvsu.03d", 0x000000, 0x80000, 0xae60a66a )
	ROM_LOAD16_WORD_SWAP( "mvsu.04d", 0x080000, 0x80000, 0x91f67d8a )
	ROM_LOAD16_WORD_SWAP( "mvsu.05a", 0x100000, 0x80000, 0x1a5de0cb )
	ROM_LOAD16_WORD_SWAP( "mvs.06a",  0x180000, 0x80000, 0x959f3030 )
	ROM_LOAD16_WORD_SWAP( "mvs.07b",  0x200000, 0x80000, 0x7f915bdb )
	ROM_LOAD16_WORD_SWAP( "mvs.08a",  0x280000, 0x80000, 0xc2813884 )
	ROM_LOAD16_WORD_SWAP( "mvs.09b",  0x300000, 0x80000, 0x3ba08818 )
	ROM_LOAD16_WORD_SWAP( "mvs.10b",  0x380000, 0x80000, 0xcf0dba98 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvs.13",   0x0000000, 0x400000, 0x29b05fd9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.15",   0x0000002, 0x400000, 0xfaddccf1, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.17",   0x0000004, 0x400000, 0x97aaf4c7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.19",   0x0000006, 0x400000, 0xcb70e915, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.14",   0x1000000, 0x400000, 0xb3b1972d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.16",   0x1000002, 0x400000, 0x08aadb5d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.18",   0x1000004, 0x400000, 0xc1228b35, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.20",   0x1000006, 0x400000, 0x366cc6c2, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvs.01",   0x00000, 0x08000, 0x68252324 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvs.02",   0x28000, 0x20000, 0xb34e773d )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvs.11",   0x000000, 0x400000, 0x86219770 )
	ROM_LOAD16_WORD_SWAP( "mvs.12",   0x400000, 0x400000, 0xf2fd7f68 )
ROM_END

ROM_START( mshvsfj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvsj.03i", 0x000000, 0x80000, 0xd8cbb691 )
	ROM_LOAD16_WORD_SWAP( "mvsj.04i", 0x080000, 0x80000, 0x32741ace )
	ROM_LOAD16_WORD_SWAP( "mvsj.05h", 0x100000, 0x80000, 0x77870dc3 )
	ROM_LOAD16_WORD_SWAP( "mvs.06a",  0x180000, 0x80000, 0x959f3030 )
	ROM_LOAD16_WORD_SWAP( "mvs.07b",  0x200000, 0x80000, 0x7f915bdb )
	ROM_LOAD16_WORD_SWAP( "mvs.08a",  0x280000, 0x80000, 0xc2813884 )
	ROM_LOAD16_WORD_SWAP( "mvs.09b",  0x300000, 0x80000, 0x3ba08818 )
	ROM_LOAD16_WORD_SWAP( "mvs.10b",  0x380000, 0x80000, 0xcf0dba98 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvs.13",   0x0000000, 0x400000, 0x29b05fd9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.15",   0x0000002, 0x400000, 0xfaddccf1, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.17",   0x0000004, 0x400000, 0x97aaf4c7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.19",   0x0000006, 0x400000, 0xcb70e915, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.14",   0x1000000, 0x400000, 0xb3b1972d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.16",   0x1000002, 0x400000, 0x08aadb5d, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.18",   0x1000004, 0x400000, 0xc1228b35, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvs.20",   0x1000006, 0x400000, 0x366cc6c2, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvs.01",   0x00000, 0x08000, 0x68252324 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvs.02",   0x28000, 0x20000, 0xb34e773d )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvs.11",   0x000000, 0x400000, 0x86219770 )
	ROM_LOAD16_WORD_SWAP( "mvs.12",   0x400000, 0x400000, 0xf2fd7f68 )
ROM_END

ROM_START( mvsc )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvcu.03d", 0x000000, 0x80000, 0xc6007557 )
	ROM_LOAD16_WORD_SWAP( "mvcu.04d", 0x080000, 0x80000, 0x724b2b20 )
	ROM_LOAD16_WORD_SWAP( "mvc.05a",  0x100000, 0x80000, 0x2d8c8e86 )
	ROM_LOAD16_WORD_SWAP( "mvc.06",   0x180000, 0x80000, 0x8528e1f5 )
	ROM_LOAD16_WORD_SWAP( "mvc.07",   0x200000, 0x80000, 0xc3baa32b )
	ROM_LOAD16_WORD_SWAP( "mvc.08",   0x280000, 0x80000, 0xbc002fcd )
	ROM_LOAD16_WORD_SWAP( "mvc.09",   0x300000, 0x80000, 0xc67b26df )
	ROM_LOAD16_WORD_SWAP( "mvc.10",   0x380000, 0x80000, 0x0fdd1e26 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvc.13",   0x0000000, 0x400000, 0xfa5f74bc, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.15",   0x0000002, 0x400000, 0x71938a8f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.17",   0x0000004, 0x400000, 0x92741d07, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.19",   0x0000006, 0x400000, 0xbcb72fc6, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.14",   0x1000000, 0x400000, 0x7f1df4e4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.16",   0x1000002, 0x400000, 0x90bd3203, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.18",   0x1000004, 0x400000, 0x67aaf727, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.20",   0x1000006, 0x400000, 0x8b0bade8, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvc.01",   0x00000, 0x08000, 0x41629e95 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvc.02",   0x28000, 0x20000, 0x963abf6b )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvc.11",   0x000000, 0x400000, 0x850fe663 )
	ROM_LOAD16_WORD_SWAP( "mvc.12",   0x400000, 0x400000, 0x7ccb1896 )
ROM_END

ROM_START( mvscj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvcj.03a", 0x000000, 0x80000, 0x3df18879 )
	ROM_LOAD16_WORD_SWAP( "mvcj.04a", 0x080000, 0x80000, 0x07d212e8 )
	ROM_LOAD16_WORD_SWAP( "mvc.05a",  0x100000, 0x80000, 0x2d8c8e86 )
	ROM_LOAD16_WORD_SWAP( "mvc.06",   0x180000, 0x80000, 0x8528e1f5 )
	ROM_LOAD16_WORD_SWAP( "mvc.07",   0x200000, 0x80000, 0xc3baa32b )
	ROM_LOAD16_WORD_SWAP( "mvc.08",   0x280000, 0x80000, 0xbc002fcd )
	ROM_LOAD16_WORD_SWAP( "mvc.09",   0x300000, 0x80000, 0xc67b26df )
	ROM_LOAD16_WORD_SWAP( "mvc.10",   0x380000, 0x80000, 0x0fdd1e26 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvc.13",   0x0000000, 0x400000, 0xfa5f74bc, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.15",   0x0000002, 0x400000, 0x71938a8f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.17",   0x0000004, 0x400000, 0x92741d07, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.19",   0x0000006, 0x400000, 0xbcb72fc6, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.14",   0x1000000, 0x400000, 0x7f1df4e4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.16",   0x1000002, 0x400000, 0x90bd3203, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.18",   0x1000004, 0x400000, 0x67aaf727, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.20",   0x1000006, 0x400000, 0x8b0bade8, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvc.01",   0x00000, 0x08000, 0x41629e95 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvc.02",   0x28000, 0x20000, 0x963abf6b )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvc.11",   0x000000, 0x400000, 0x850fe663 )
	ROM_LOAD16_WORD_SWAP( "mvc.12",   0x400000, 0x400000, 0x7ccb1896 )
ROM_END

ROM_START( mvsch )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "mvch.03",  0x000000, 0x80000, 0x6a0ec9f7 )
	ROM_LOAD16_WORD_SWAP( "mvch.04",  0x080000, 0x80000, 0x00f03fa4 )
	ROM_LOAD16_WORD_SWAP( "mvc.05a",  0x100000, 0x80000, 0x2d8c8e86 )
	ROM_LOAD16_WORD_SWAP( "mvc.06",   0x180000, 0x80000, 0x8528e1f5 )
	ROM_LOAD16_WORD_SWAP( "mvc.07",   0x200000, 0x80000, 0xc3baa32b )
	ROM_LOAD16_WORD_SWAP( "mvc.08",   0x280000, 0x80000, 0xbc002fcd )
	ROM_LOAD16_WORD_SWAP( "mvc.09",   0x300000, 0x80000, 0xc67b26df )
	ROM_LOAD16_WORD_SWAP( "mvc.10",   0x380000, 0x80000, 0x0fdd1e26 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "mvc.13",   0x0000000, 0x400000, 0xfa5f74bc, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.15",   0x0000002, 0x400000, 0x71938a8f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.17",   0x0000004, 0x400000, 0x92741d07, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.19",   0x0000006, 0x400000, 0xbcb72fc6, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.14",   0x1000000, 0x400000, 0x7f1df4e4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.16",   0x1000002, 0x400000, 0x90bd3203, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.18",   0x1000004, 0x400000, 0x67aaf727, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "mvc.20",   0x1000006, 0x400000, 0x8b0bade8, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "mvc.01",   0x00000, 0x08000, 0x41629e95 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "mvc.02",   0x28000, 0x20000, 0x963abf6b )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "mvc.11",   0x000000, 0x400000, 0x850fe663 )
	ROM_LOAD16_WORD_SWAP( "mvc.12",   0x400000, 0x400000, 0x7ccb1896 )
ROM_END

ROM_START( nwarr )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vphu.03f", 0x000000, 0x80000, 0x85d6a359 )
	ROM_LOAD16_WORD_SWAP( "vphu.04c", 0x080000, 0x80000, 0xcb7fce77 )
	ROM_LOAD16_WORD_SWAP( "vphu.05e", 0x100000, 0x80000, 0xe08f2bba )
	ROM_LOAD16_WORD_SWAP( "vphu.06c", 0x180000, 0x80000, 0x08c04cdb )
	ROM_LOAD16_WORD_SWAP( "vphu.07b", 0x200000, 0x80000, 0xb5a5ab19 )
	ROM_LOAD16_WORD_SWAP( "vphu.08b", 0x280000, 0x80000, 0x51bb20fb )
	ROM_LOAD16_WORD_SWAP( "vphu.09b", 0x300000, 0x80000, 0x41a64205 )
	ROM_LOAD16_WORD_SWAP( "vphu.10b", 0x380000, 0x80000, 0x2b1d43ae )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vph.13",   0x0000000, 0x400000, 0xc51baf99, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.15",   0x0000002, 0x400000, 0x3ce83c77, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.17",   0x0000004, 0x400000, 0x4f2408e0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.19",   0x0000006, 0x400000, 0x9ff60250, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.14",   0x1000000, 0x400000, 0x7a0e1add, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.16",   0x1000002, 0x400000, 0x2f41ca75, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.18",   0x1000004, 0x400000, 0x64498eed, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.20",   0x1000006, 0x400000, 0x17f2433f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vph.01",   0x00000, 0x08000, 0x5045dcac )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vph.02",   0x28000, 0x20000, 0x86b60e59 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vph.11",   0x000000, 0x200000, 0xe1837d33 )
	ROM_LOAD16_WORD_SWAP( "vph.12",   0x200000, 0x200000, 0xfbd3cd90 )
ROM_END

ROM_START( vhuntj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vphj.03b", 0x000000, 0x80000, 0x679c3fa9 )
	ROM_LOAD16_WORD_SWAP( "vphj.04a", 0x080000, 0x80000, 0xeb6e71e4 )
	ROM_LOAD16_WORD_SWAP( "vphj.05a", 0x100000, 0x80000, 0xeaf634ea )
	ROM_LOAD16_WORD_SWAP( "vphj.06a", 0x180000, 0x80000, 0xb70cc6be )
	ROM_LOAD16_WORD_SWAP( "vphj.07a", 0x200000, 0x80000, 0x46ab907d )
	ROM_LOAD16_WORD_SWAP( "vphj.08a", 0x280000, 0x80000, 0x1c00355e )
	ROM_LOAD16_WORD_SWAP( "vphj.09a", 0x300000, 0x80000, 0x026e6f82 )
	ROM_LOAD16_WORD_SWAP( "vphj.10a", 0x380000, 0x80000, 0xaadfb3ea )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vph.13",   0x0000000, 0x400000, 0xc51baf99, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.15",   0x0000002, 0x400000, 0x3ce83c77, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.17",   0x0000004, 0x400000, 0x4f2408e0, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.19",   0x0000006, 0x400000, 0x9ff60250, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.14",   0x1000000, 0x400000, 0x7a0e1add, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.16",   0x1000002, 0x400000, 0x2f41ca75, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.18",   0x1000004, 0x400000, 0x64498eed, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vph.20",   0x1000006, 0x400000, 0x17f2433f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vph.01",   0x00000, 0x08000, 0x5045dcac )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vph.02",   0x28000, 0x20000, 0x86b60e59 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vph.11",   0x000000, 0x200000, 0xe1837d33 )
	ROM_LOAD16_WORD_SWAP( "vph.12",   0x200000, 0x200000, 0xfbd3cd90 )
ROM_END

ROM_START( rckman2j )
	ROM_REGION(CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "rm2j.03",  0x000000, 0x80000, 0xdbaa1437 )
	ROM_LOAD16_WORD_SWAP( "rm2j.04",  0x080000, 0x80000, 0xcf5ba612 )
	ROM_LOAD16_WORD_SWAP( "rm2j.05",  0x100000, 0x80000, 0x02ee9efc )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x0800000, REGION_GFX1, 0 )
	ROMX_LOAD( "rm2.14",   0x000000, 0x200000, 0x9b1f00b4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rm2.16",   0x000002, 0x200000, 0xc2bb0c24, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rm2.18",   0x000004, 0x200000, 0x12257251, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "rm2.20",   0x000006, 0x200000, 0xf9b6e786, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION(QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "rm2.01",   0x00000, 0x08000, 0xd18e7859 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "rm2.02",   0x28000, 0x20000, 0xc463ece0 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "rm2.11",   0x000000, 0x200000, 0x2106174d )
	ROM_LOAD16_WORD_SWAP( "rm2.12",   0x200000, 0x200000, 0x546c1636 )
ROM_END

ROM_START( qndream )
	ROM_REGION(CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "tqzj.03",  0x000000, 0x80000, 0x7acf3e30 )
	ROM_LOAD16_WORD_SWAP( "tqzj.04",  0x080000, 0x80000, 0xf1044a87 )
	ROM_LOAD16_WORD_SWAP( "tqzj.05",  0x100000, 0x80000, 0x4105ba0e )
	ROM_LOAD16_WORD_SWAP( "tqzj.06",  0x100000, 0x80000, 0xc371e8a5 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x0800000, REGION_GFX1, 0 )
	ROMX_LOAD( "tqz.14",   0x000000, 0x200000, 0x98af88a2, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tqz.16",   0x000002, 0x200000, 0xdf82d491, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tqz.18",   0x000004, 0x200000, 0x42f132ff, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "tqz.20",   0x000006, 0x200000, 0xb2e128a3, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION(QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "tqz.01",   0x00000, 0x08000, 0xe9ce9d0a )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "tqz.11",   0x000000, 0x200000, 0x78e7884f )
	ROM_LOAD16_WORD_SWAP( "tqz.12",   0x200000, 0x200000, 0x2e049b13 )
ROM_END

ROM_START( sfa )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfzu.03a",  0x000000, 0x80000, 0x49fc7db9 )
	ROM_LOAD16_WORD_SWAP( "sfz.04a",   0x080000, 0x80000, 0x5f99e9a5 )
	ROM_LOAD16_WORD_SWAP( "sfz.05a",   0x100000, 0x80000, 0x0810544d )
	ROM_LOAD16_WORD_SWAP( "sfz.06",    0x180000, 0x80000, 0x806e8f38 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "sfzux.03a", XOR_BASE+0x000000, 0x80000, 0x1a3160ed )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "sfz.14",   0x800000, 0x200000, 0x90fefdb3, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.16",   0x800002, 0x200000, 0x5354c948, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.18",   0x800004, 0x200000, 0x41a1e790, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.20",   0x800006, 0x200000, 0xa549df98, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz.01",   0x00000, 0x08000, 0xffffec7d )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfz.02",   0x28000, 0x20000, 0x45f46a08 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfz.11",   0x000000, 0x200000, 0xc4b093cd )
	ROM_LOAD16_WORD_SWAP( "sfz.12",   0x200000, 0x200000, 0x8bdbc4b4 )
ROM_END

ROM_START( sfzj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfzj.03b",  0x000000, 0x80000, 0xf5444120 )
	ROM_LOAD16_WORD_SWAP( "sfz.04b",   0x080000, 0x80000, 0x8b73b0e5 )
	ROM_LOAD16_WORD_SWAP( "sfz.05a",   0x100000, 0x80000, 0x0810544d )
	ROM_LOAD16_WORD_SWAP( "sfz.06",    0x180000, 0x80000, 0x806e8f38 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "sfzjx.03b", XOR_BASE+0x000000, 0x80000, 0xd6b17a9b )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "sfz.14",   0x800000, 0x200000, 0x90fefdb3, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.16",   0x800002, 0x200000, 0x5354c948, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.18",   0x800004, 0x200000, 0x41a1e790, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.20",   0x800006, 0x200000, 0xa549df98, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz.01",   0x00000, 0x08000, 0xffffec7d )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfz.02",   0x28000, 0x20000, 0x45f46a08 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfz.11",   0x000000, 0x200000, 0xc4b093cd )
	ROM_LOAD16_WORD_SWAP( "sfz.12",   0x200000, 0x200000, 0x8bdbc4b4 )
ROM_END

ROM_START( sfzjr1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfzj.03a",  0x000000, 0x80000, 0x844220c2 )
	ROM_LOAD16_WORD_SWAP( "sfz.04a",   0x080000, 0x80000, 0x5f99e9a5 )
	ROM_LOAD16_WORD_SWAP( "sfz.05a",   0x100000, 0x80000, 0x0810544d )
	ROM_LOAD16_WORD_SWAP( "sfz.06",    0x180000, 0x80000, 0x806e8f38 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "sfzjx.03a", XOR_BASE+0x000000, 0x80000, 0xb501f03c )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "sfz.14",   0x800000, 0x200000, 0x90fefdb3, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.16",   0x800002, 0x200000, 0x5354c948, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.18",   0x800004, 0x200000, 0x41a1e790, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.20",   0x800006, 0x200000, 0xa549df98, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz.01",   0x00000, 0x08000, 0xffffec7d )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfz.02",   0x28000, 0x20000, 0x45f46a08 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfz.11",   0x000000, 0x200000, 0xc4b093cd )
	ROM_LOAD16_WORD_SWAP( "sfz.12",   0x200000, 0x200000, 0x8bdbc4b4 )
ROM_END

ROM_START( sfzjr2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfzj.03",   0x000000, 0x80000, 0x3cfce93c )
	ROM_LOAD16_WORD_SWAP( "sfz.04",    0x080000, 0x80000, 0x0c436d30 )
	ROM_LOAD16_WORD_SWAP( "sfz.05",    0x100000, 0x80000, 0x1f363612 )
	ROM_LOAD16_WORD_SWAP( "sfz.06",    0x180000, 0x80000, 0x806e8f38 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "sfzjx.03", XOR_BASE+0x000000, 0x80000, 0x00000000 )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "sfz.14",   0x800000, 0x200000, 0x90fefdb3, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.16",   0x800002, 0x200000, 0x5354c948, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.18",   0x800004, 0x200000, 0x41a1e790, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.20",   0x800006, 0x200000, 0xa549df98, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz.01",   0x00000, 0x08000, 0xffffec7d )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfz.02",   0x28000, 0x20000, 0x45f46a08 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfz.11",   0x000000, 0x200000, 0xc4b093cd )
	ROM_LOAD16_WORD_SWAP( "sfz.12",   0x200000, 0x200000, 0x8bdbc4b4 )
ROM_END

ROM_START( sfaer1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfze.03b",  0x000000, 0x80000, 0xebf2054d )
	ROM_LOAD16_WORD_SWAP( "sfz.04b",   0x080000, 0x80000, 0x8b73b0e5 )
	ROM_LOAD16_WORD_SWAP( "sfz.05a",   0x100000, 0x80000, 0x0810544d )
	ROM_LOAD16_WORD_SWAP( "sfz.06",    0x180000, 0x80000, 0x806e8f38 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "sfzex.03b", XOR_BASE+0x000000, 0x80000, 0x00000000 )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "sfz.14",   0x800000, 0x200000, 0x90fefdb3, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.16",   0x800002, 0x200000, 0x5354c948, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.18",   0x800004, 0x200000, 0x41a1e790, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.20",   0x800006, 0x200000, 0xa549df98, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz.01",   0x00000, 0x08000, 0xffffec7d )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfz.02",   0x28000, 0x20000, 0x45f46a08 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfz.11",   0x000000, 0x200000, 0xc4b093cd )
	ROM_LOAD16_WORD_SWAP( "sfz.12",   0x200000, 0x200000, 0x8bdbc4b4 )
ROM_END

ROM_START( sfzh )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfzh.03c",  0x000000, 0x80000, 0xbce635aa )
	ROM_LOAD16_WORD_SWAP( "sfz.04a",   0x080000, 0x80000, 0x5f99e9a5 )
	ROM_LOAD16_WORD_SWAP( "sfz.05a",   0x100000, 0x80000, 0x0810544d )
	ROM_LOAD16_WORD_SWAP( "sfz.06",    0x180000, 0x80000, 0x806e8f38 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "sfzhx.03c", XOR_BASE+0x000000, 0x80000, 0x00000000 )

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROM_FILL(              0x000000, 0x800000, 0 )
	ROMX_LOAD( "sfz.14",   0x800000, 0x200000, 0x90fefdb3, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.16",   0x800002, 0x200000, 0x5354c948, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.18",   0x800004, 0x200000, 0x41a1e790, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfz.20",   0x800006, 0x200000, 0xa549df98, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfz.01",   0x00000, 0x08000, 0xffffec7d )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfz.02",   0x28000, 0x20000, 0x45f46a08 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfz.11",   0x000000, 0x200000, 0xc4b093cd )
	ROM_LOAD16_WORD_SWAP( "sfz.12",   0x200000, 0x200000, 0x8bdbc4b4 )
ROM_END

ROM_START( sfa2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sz2u.03", 0x000000, 0x80000, 0x84a09006 )
	ROM_LOAD16_WORD_SWAP( "sz2u.04", 0x080000, 0x80000, 0xac46e5ed )
	ROM_LOAD16_WORD_SWAP( "sz2u.05", 0x100000, 0x80000, 0x6c0c79d3 )
	ROM_LOAD16_WORD_SWAP( "sz2u.06", 0x180000, 0x80000, 0xc5c8eb63 )
	ROM_LOAD16_WORD_SWAP( "sz2u.07", 0x200000, 0x80000, 0x5de01cc5 )
	ROM_LOAD16_WORD_SWAP( "sz2u.08", 0x280000, 0x80000, 0xbea11d56 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "sz2ux.03", XOR_BASE+0x000000, 0x80000, 0x6bb6005f )
	ROM_LOAD16_WORD_SWAP( "sz2ux.04", XOR_BASE+0x080000, 0x80000, 0x74308a4b )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	/* sz2.13, 14, 15, 16 might be byteswapped on the real board.
	 * However, sz2a has them in the standard order and the data is identical,
	 * so we are using that version.
	 */
	ROMX_LOAD( "sza.13",   0x0000000, 0x400000, 0x4d1f1f22, ROM_GROUPWORD | ROM_SKIP(6) )	// 0x1858afce
	ROMX_LOAD( "sza.15",   0x0000002, 0x400000, 0x19cea680, ROM_GROUPWORD | ROM_SKIP(6) )	// 0x96fcf35c
	ROMX_LOAD( "sz2.17",   0x0000004, 0x400000, 0xe01b4588, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.19",   0x0000006, 0x400000, 0x0feeda64, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sza.14",   0x1000000, 0x100000, 0x0560c6aa, ROM_GROUPWORD | ROM_SKIP(6) )	// 0x08c6bb9c
	ROMX_LOAD( "sza.16",   0x1000002, 0x100000, 0xae940f87, ROM_GROUPWORD | ROM_SKIP(6) )	// 0xca161c9c
	ROMX_LOAD( "sz2.18",   0x1000004, 0x100000, 0x4bc3c8bc, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.20",   0x1000006, 0x100000, 0x39e674c0, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz2.01",   0x00000, 0x08000, 0x1bc323cf )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz2.02",   0x28000, 0x20000, 0xba6a5013 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz2.11",   0x000000, 0x200000, 0xaa47a601 )
	ROM_LOAD16_WORD_SWAP( "sz2.12",   0x200000, 0x200000, 0x2237bc53 )
ROM_END

ROM_START( sfz2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sz2j.03a", 0x000000, 0x80000, 0x97461e28 )
	ROM_LOAD16_WORD_SWAP( "sz2j.04a", 0x080000, 0x80000, 0xae4851a9 )
	ROM_LOAD16_WORD_SWAP( "sz2j.05a", 0x100000, 0x80000, 0x98e8e992 )
	ROM_LOAD16_WORD_SWAP( "sz2j.06a", 0x180000, 0x80000, 0x5b1d49c0 )
	ROM_LOAD16_WORD_SWAP( "sz2j.07a", 0x200000, 0x80000, 0xd910b2a2 )
	ROM_LOAD16_WORD_SWAP( "sz2j.08a", 0x280000, 0x80000, 0x0fe8585d )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "sz2jx.03", XOR_BASE+0x000000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "sz2jx.04", XOR_BASE+0x080000, 0x80000, 0x00000000 )

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	/* sz2.13, 14, 15, 16 might be byteswapped on the real board.
	 * However, sz2a has them in the standard order and the data is identical,
	 * so we are using that version.
	 */
	ROMX_LOAD( "sza.13",   0x0000000, 0x400000, 0x4d1f1f22, ROM_GROUPWORD | ROM_SKIP(6) )	// 0x1858afce
	ROMX_LOAD( "sza.15",   0x0000002, 0x400000, 0x19cea680, ROM_GROUPWORD | ROM_SKIP(6) )	// 0x96fcf35c
	ROMX_LOAD( "sz2.17",   0x0000004, 0x400000, 0xe01b4588, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.19",   0x0000006, 0x400000, 0x0feeda64, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sza.14",   0x1000000, 0x100000, 0x0560c6aa, ROM_GROUPWORD | ROM_SKIP(6) )	// 0x08c6bb9c
	ROMX_LOAD( "sza.16",   0x1000002, 0x100000, 0xae940f87, ROM_GROUPWORD | ROM_SKIP(6) )	// 0xca161c9c
	ROMX_LOAD( "sz2.18",   0x1000004, 0x100000, 0x4bc3c8bc, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.20",   0x1000006, 0x100000, 0x39e674c0, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz2.01",   0x00000, 0x08000, 0x1bc323cf )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz2.02",   0x28000, 0x20000, 0xba6a5013 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz2.11",   0x000000, 0x200000, 0xaa47a601 )
	ROM_LOAD16_WORD_SWAP( "sz2.12",   0x200000, 0x200000, 0x2237bc53 )
ROM_END

ROM_START( sfz2a )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "szaj.03a", 0x000000, 0x80000, 0xa3802fe3 )
	ROM_LOAD16_WORD_SWAP( "szaj.04a", 0x080000, 0x80000, 0xe7ca87c7 )
	ROM_LOAD16_WORD_SWAP( "szaj.05a", 0x100000, 0x80000, 0xc88ebf88 )
	ROM_LOAD16_WORD_SWAP( "szaj.06a", 0x180000, 0x80000, 0x35ed5b7a )
	ROM_LOAD16_WORD_SWAP( "szaj.07a", 0x200000, 0x80000, 0x975dcb3e )
	ROM_LOAD16_WORD_SWAP( "szaj.08a", 0x280000, 0x80000, 0xdc73f2d7 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
	ROMX_LOAD( "sza.13",   0x0000000, 0x400000, 0x4d1f1f22, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sza.15",   0x0000002, 0x400000, 0x19cea680, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.17",   0x0000004, 0x400000, 0xe01b4588, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.19",   0x0000006, 0x400000, 0x0feeda64, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sza.14",   0x1000000, 0x100000, 0x0560c6aa, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sza.16",   0x1000002, 0x100000, 0xae940f87, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.18",   0x1000004, 0x100000, 0x4bc3c8bc, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz2.20",   0x1000006, 0x100000, 0x39e674c0, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz2.01",   0x00000, 0x08000, 0x1bc323cf )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz2.02",   0x28000, 0x20000, 0xba6a5013 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz2.11",   0x000000, 0x200000, 0xaa47a601 )
	ROM_LOAD16_WORD_SWAP( "sz2.12",   0x200000, 0x200000, 0x2237bc53 )
ROM_END

ROM_START( sfa3 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sz3u.03", 0x000000, 0x80000, 0xb5984a19 )
	ROM_LOAD16_WORD_SWAP( "sz3u.04", 0x080000, 0x80000, 0x7e8158ba )
	ROM_LOAD16_WORD_SWAP( "sz3u.05", 0x100000, 0x80000, 0x9b21518a )
	ROM_LOAD16_WORD_SWAP( "sz3u.06", 0x180000, 0x80000, 0xe7a6c3a7 )
	ROM_LOAD16_WORD_SWAP( "sz3u.07", 0x200000, 0x80000, 0xec4c0cfd )
	ROM_LOAD16_WORD_SWAP( "sz3u.08", 0x280000, 0x80000, 0x5c7e7240 )
	ROM_LOAD16_WORD_SWAP( "sz3u.09", 0x300000, 0x80000, 0xc5589553 )
	ROM_LOAD16_WORD_SWAP( "sz3.10",  0x380000, 0x80000, 0xa9717252 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "sz3.13",   0x0000000, 0x400000, 0x0f7a60d9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.15",   0x0000002, 0x400000, 0x8e933741, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.17",   0x0000004, 0x400000, 0xd6e98147, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.19",   0x0000006, 0x400000, 0xf31a728a, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.14",   0x1000000, 0x400000, 0x5ff98297, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.16",   0x1000002, 0x400000, 0x52b5bdee, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.18",   0x1000004, 0x400000, 0x40631ed5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.20",   0x1000006, 0x400000, 0x763409b4, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz3.01",   0x00000, 0x08000, 0xde810084 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz3.02",   0x28000, 0x20000, 0x72445dc4 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz3.11",   0x000000, 0x400000, 0x1c89eed1 )
	ROM_LOAD16_WORD_SWAP( "sz3.12",   0x400000, 0x400000, 0xf392b13a )
ROM_END

ROM_START( sfz3 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sz3j.03", 0x000000, 0x80000, 0x6ee0beae )
	ROM_LOAD16_WORD_SWAP( "sz3j.04", 0x080000, 0x80000, 0xa6e2978d )
	ROM_LOAD16_WORD_SWAP( "sz3j.05", 0x100000, 0x80000, 0x05964b7d )
	ROM_LOAD16_WORD_SWAP( "sz3j.06", 0x180000, 0x80000, 0x78ce2179 )
	ROM_LOAD16_WORD_SWAP( "sz3j.07", 0x200000, 0x80000, 0x398bf52f )
	ROM_LOAD16_WORD_SWAP( "sz3j.08", 0x280000, 0x80000, 0x866d0588 )
	ROM_LOAD16_WORD_SWAP( "sz3j.09", 0x300000, 0x80000, 0x2180892c )
	ROM_LOAD16_WORD_SWAP( "sz3.10",  0x380000, 0x80000, 0xa9717252 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "sz3.13",   0x0000000, 0x400000, 0x0f7a60d9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.15",   0x0000002, 0x400000, 0x8e933741, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.17",   0x0000004, 0x400000, 0xd6e98147, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.19",   0x0000006, 0x400000, 0xf31a728a, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.14",   0x1000000, 0x400000, 0x5ff98297, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.16",   0x1000002, 0x400000, 0x52b5bdee, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.18",   0x1000004, 0x400000, 0x40631ed5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sz3.20",   0x1000006, 0x400000, 0x763409b4, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sz3.01",   0x00000, 0x08000, 0xde810084 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sz3.02",   0x28000, 0x20000, 0x72445dc4 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sz3.11",   0x000000, 0x400000, 0x1c89eed1 )
	ROM_LOAD16_WORD_SWAP( "sz3.12",   0x400000, 0x400000, 0xf392b13a )
ROM_END

ROM_START( sgemf )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pcfu.03", 0x000000, 0x80000, 0xac2e8566 )
	ROM_LOAD16_WORD_SWAP( "pcf.04",  0x080000, 0x80000, 0xf4314c96 )
	ROM_LOAD16_WORD_SWAP( "pcf.05",  0x100000, 0x80000, 0x215655f6 )
	ROM_LOAD16_WORD_SWAP( "pcf.06",  0x180000, 0x80000, 0xea6f13ea )
	ROM_LOAD16_WORD_SWAP( "pcf.07",  0x200000, 0x80000, 0x5ac6d5ea )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
    /* One of these planes is corrupt */
	ROMX_LOAD( "pcf.13",   0x0000000, 0x400000, 0x22d72ab9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.15",   0x0000002, 0x400000, 0x16a4813c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.17",   0x0000004, 0x400000, 0x1097e035, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.19",   0x0000006, 0x400000, 0xd362d874, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.14",   0x1000000, 0x100000, 0x0383897c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.16",   0x1000002, 0x100000, 0x76f91084, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.18",   0x1000004, 0x100000, 0x756c3754, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.20",   0x1000006, 0x100000, 0x9ec9277d, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pcf.01",   0x00000, 0x08000, 0xe5af9fcd )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pcf.02",   0x28000, 0x20000, 0x8630e818 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pcf.11",   0x000000, 0x400000, 0xa5dea005 )
	ROM_LOAD16_WORD_SWAP( "pcf.12",   0x400000, 0x400000, 0x4ce235fe )
ROM_END

ROM_START( pfghtj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pcfj.03", 0x000000, 0x80000, 0x681da43e )
	ROM_LOAD16_WORD_SWAP( "pcf.04",  0x080000, 0x80000, 0xf4314c96 )
	ROM_LOAD16_WORD_SWAP( "pcf.05",  0x100000, 0x80000, 0x215655f6 )
	ROM_LOAD16_WORD_SWAP( "pcf.06",  0x180000, 0x80000, 0xea6f13ea )
	ROM_LOAD16_WORD_SWAP( "pcf.07",  0x200000, 0x80000, 0x5ac6d5ea )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
    /* One of these planes is corrupt */
	ROMX_LOAD( "pcf.13",   0x0000000, 0x400000, 0x22d72ab9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.15",   0x0000002, 0x400000, 0x16a4813c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.17",   0x0000004, 0x400000, 0x1097e035, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.19",   0x0000006, 0x400000, 0xd362d874, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.14",   0x1000000, 0x100000, 0x0383897c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.16",   0x1000002, 0x100000, 0x76f91084, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.18",   0x1000004, 0x100000, 0x756c3754, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.20",   0x1000006, 0x100000, 0x9ec9277d, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pcf.01",   0x00000, 0x08000, 0xe5af9fcd )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pcf.02",   0x28000, 0x20000, 0x8630e818 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pcf.11",   0x000000, 0x400000, 0xa5dea005 )
	ROM_LOAD16_WORD_SWAP( "pcf.12",   0x400000, 0x400000, 0x4ce235fe )
ROM_END

ROM_START( sgemfh )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pcfh.03", 0x000000, 0x80000, 0xe9103347 )
	ROM_LOAD16_WORD_SWAP( "pcf.04",  0x080000, 0x80000, 0xf4314c96 )
	ROM_LOAD16_WORD_SWAP( "pcf.05",  0x100000, 0x80000, 0x215655f6 )
	ROM_LOAD16_WORD_SWAP( "pcf.06",  0x180000, 0x80000, 0xea6f13ea )
	ROM_LOAD16_WORD_SWAP( "pcf.07",  0x200000, 0x80000, 0x5ac6d5ea )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x1400000, REGION_GFX1, 0 )
    /* One of these planes is corrupt */
	ROMX_LOAD( "pcf.13",   0x0000000, 0x400000, 0x22d72ab9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.15",   0x0000002, 0x400000, 0x16a4813c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.17",   0x0000004, 0x400000, 0x1097e035, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.19",   0x0000006, 0x400000, 0xd362d874, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.14",   0x1000000, 0x100000, 0x0383897c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.16",   0x1000002, 0x100000, 0x76f91084, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.18",   0x1000004, 0x100000, 0x756c3754, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pcf.20",   0x1000006, 0x100000, 0x9ec9277d, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pcfh.01",  0x00000, 0x08000, 0x254e5f33 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pcfh.02",  0x28000, 0x20000, 0x241f2d1a )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pcf.11",   0x000000, 0x400000, 0xa5dea005 )
	ROM_LOAD16_WORD_SWAP( "pcf.12",   0x400000, 0x400000, 0x4ce235fe )
ROM_END

ROM_START( slam2e )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "smbe.03b", 0x000000, 0x80000, 0xb8016278 )
	ROM_LOAD16_WORD_SWAP( "smbe.04b", 0x080000, 0x80000, 0x18c4c447 )
	ROM_LOAD16_WORD_SWAP( "smbe.05b", 0x100000, 0x80000, 0x18ebda7f )
	ROM_LOAD16_WORD_SWAP( "smbe.06b", 0x180000, 0x80000, 0x89c80007 )
	ROM_LOAD16_WORD_SWAP( "smb.07b",  0x200000, 0x80000, 0xb9a11577 )
	ROM_LOAD16_WORD_SWAP( "smb.08b",  0x280000, 0x80000, 0xf931b76b )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x1200000, REGION_GFX1, 0 )
	ROMX_LOAD( "smb.13",   0x0000000, 0x200000, 0xd9b2d1de, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.15",   0x0000002, 0x200000, 0x9a766d92, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.17",   0x0000004, 0x200000, 0x51800f0f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.19",   0x0000006, 0x200000, 0x35757e96, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.14",   0x0800000, 0x200000, 0xe5bfd0e7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.16",   0x0800002, 0x200000, 0xc56c0866, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.18",   0x0800004, 0x200000, 0x4ded3910, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.20",   0x0800006, 0x200000, 0x26ea1ec5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.21",   0x1000000, 0x080000, 0x0a08c5fc, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.23",   0x1000002, 0x080000, 0x0911b6c4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.25",   0x1000004, 0x080000, 0x82d6c4ec, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.27",   0x1000006, 0x080000, 0x9b48678b, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "smb.01",   0x00000, 0x08000, 0x0abc229a )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "smb.02",   0x28000, 0x20000, 0xd051679a )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "smb.11",   0x000000, 0x200000, 0xC56935f9 )
	ROM_LOAD16_WORD_SWAP( "smb.12",   0x200000, 0x200000, 0x955b0782 )
ROM_END

ROM_START( smbomber )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "smbj.03", 0x000000, 0x80000, 0x52eafb10 )
	ROM_LOAD16_WORD_SWAP( "smbj.04", 0x080000, 0x80000, 0xaa6e8078 )
	ROM_LOAD16_WORD_SWAP( "smbj.05", 0x100000, 0x80000, 0xb69e7d5f )
	ROM_LOAD16_WORD_SWAP( "smbj.06", 0x180000, 0x80000, 0x8d857b56 )
	ROM_LOAD16_WORD_SWAP( "smb.07b", 0x200000, 0x80000, 0xb9a11577 )
	ROM_LOAD16_WORD_SWAP( "smb.08b", 0x280000, 0x80000, 0xf931b76b )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x1200000, REGION_GFX1, 0 )
	ROMX_LOAD( "smb.13",   0x0000000, 0x200000, 0xd9b2d1de, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.15",   0x0000002, 0x200000, 0x9a766d92, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.17",   0x0000004, 0x200000, 0x51800f0f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.19",   0x0000006, 0x200000, 0x35757e96, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.14",   0x0800000, 0x200000, 0xe5bfd0e7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.16",   0x0800002, 0x200000, 0xc56c0866, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.18",   0x0800004, 0x200000, 0x4ded3910, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.20",   0x0800006, 0x200000, 0x26ea1ec5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.21",   0x1000000, 0x080000, 0x0a08c5fc, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.23",   0x1000002, 0x080000, 0x0911b6c4, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.25",   0x1000004, 0x080000, 0x82d6c4ec, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "smb.27",   0x1000006, 0x080000, 0x9b48678b, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "smb.01",   0x00000, 0x08000, 0x0abc229a )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "smb.02",   0x28000, 0x20000, 0xd051679a )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "smb.11",   0x000000, 0x200000, 0xC56935f9 )
	ROM_LOAD16_WORD_SWAP( "smb.12",   0x200000, 0x200000, 0x955b0782 )
ROM_END

ROM_START( spf2t )
	ROM_REGION(CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pzfu.03a",  0x000000, 0x80000, 0x346e62ef )
	ROM_LOAD16_WORD_SWAP( "pzf.04a",   0x080000, 0x80000, 0xb80649e2 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "pzf.14",  0x000000, 0x100000, 0x2d4881cb, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pzf.16",  0x000002, 0x100000, 0x4b0fd1be, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pzf.18",  0x000004, 0x100000, 0xe43aac33, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pzf.20",  0x000006, 0x100000, 0x7f536ff1, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION(QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pzf.01",   0x00000, 0x08000, 0x600fb2a3 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pzf.02",   0x28000, 0x20000, 0x496076e0 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pzf.11",   0x000000, 0x200000, 0x78442743 )
	ROM_LOAD16_WORD_SWAP( "pzf.12",   0x200000, 0x200000, 0x399d2c7b )
ROM_END

ROM_START( spf2xj )
	ROM_REGION(CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "pzfj.03a",  0x000000, 0x80000, 0x2070554a )
	ROM_LOAD16_WORD_SWAP( "pzf.04a",   0x080000, 0x80000, 0xb80649e2 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROMX_LOAD( "pzf.14",  0x000000, 0x100000, 0x2d4881cb, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pzf.16",  0x000002, 0x100000, 0x4b0fd1be, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pzf.18",  0x000004, 0x100000, 0xe43aac33, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "pzf.20",  0x000006, 0x100000, 0x7f536ff1, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION(QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "pzf.01",   0x00000, 0x08000, 0x600fb2a3 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "pzf.02",   0x28000, 0x20000, 0x496076e0 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "pzf.11",   0x000000, 0x200000, 0x78442743 )
	ROM_LOAD16_WORD_SWAP( "pzf.12",   0x200000, 0x200000, 0x399d2c7b )
ROM_END

ROM_START( ssf2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "ssfu.03a", 0x000000, 0x80000, 0x72f29c33 )
	ROM_LOAD16_WORD_SWAP( "ssfu.04a", 0x080000, 0x80000, 0x935cea44 )
	ROM_LOAD16_WORD_SWAP( "ssfu.05",  0x100000, 0x80000, 0xa0acb28a )
	ROM_LOAD16_WORD_SWAP( "ssfu.06",  0x180000, 0x80000, 0x47413dcf )
	ROM_LOAD16_WORD_SWAP( "ssfu.07",  0x200000, 0x80000, 0xe6066077 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "ssfux.03a", XOR_BASE+0x000000, 0x80000, 0xec278279 )
	ROM_LOAD16_WORD_SWAP( "ssfux.04a", XOR_BASE+0x080000, 0x80000, 0x6194d896 )
	ROM_LOAD16_WORD_SWAP( "ssfux.05",  XOR_BASE+0x100000, 0x80000, 0xacddaf22 )
	ROM_LOAD16_WORD_SWAP( "ssfux.06",  XOR_BASE+0x180000, 0x80000, 0x235268c4 )
	ROM_LOAD16_WORD_SWAP( "ssfux.07",  XOR_BASE+0x200000, 0x80000, 0xe46e033c )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, 0xcf94d275, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, 0x5eb703af, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, 0xffa60e0f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, 0x34e825c5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, 0xb7cc32e7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, 0x8376ad18, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, 0xf5b1b336, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, 0x459d5c6b, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ssf.01",   0x00000, 0x08000, 0xeb247e8c )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "ssf.q01",  0x000000, 0x080000, 0xa6f9da5c )
	ROM_LOAD( "ssf.q02",  0x080000, 0x080000, 0x8c66ae26 )
	ROM_LOAD( "ssf.q03",  0x100000, 0x080000, 0x695cc2ca )
	ROM_LOAD( "ssf.q04",  0x180000, 0x080000, 0x9d9ebe32 )
	ROM_LOAD( "ssf.q05",  0x200000, 0x080000, 0x4770e7b7 )
	ROM_LOAD( "ssf.q06",  0x280000, 0x080000, 0x4e79c951 )
	ROM_LOAD( "ssf.q07",  0x300000, 0x080000, 0xcdd14313 )
	ROM_LOAD( "ssf.q08",  0x380000, 0x080000, 0x6f5a088c )
ROM_END

ROM_START( ssf2a )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "ssfa.03a", 0x000000, 0x80000, 0xd2a3c520 )
	ROM_LOAD16_WORD_SWAP( "ssfa.04a", 0x080000, 0x80000, 0x5d873642 )
	ROM_LOAD16_WORD_SWAP( "ssfa.05a", 0x100000, 0x80000, 0xf8fb4de2 )
	ROM_LOAD16_WORD_SWAP( "ssfa.06a", 0x180000, 0x80000, 0xaa8acee7 )
	ROM_LOAD16_WORD_SWAP( "ssfa.07a", 0x200000, 0x80000, 0x36e29217 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "ssfax.03a", XOR_BASE+0x000000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "ssfax.04a", XOR_BASE+0x080000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "ssfax.05a", XOR_BASE+0x100000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "ssfax.06a", XOR_BASE+0x180000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "ssfax.07a", XOR_BASE+0x200000, 0x80000, 0x00000000 )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, 0xcf94d275, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, 0x5eb703af, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, 0xffa60e0f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, 0x34e825c5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, 0xb7cc32e7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, 0x8376ad18, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, 0xf5b1b336, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, 0x459d5c6b, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ssf.01",   0x00000, 0x08000, 0xeb247e8c )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "ssf.q01",  0x000000, 0x080000, 0xa6f9da5c )
	ROM_LOAD( "ssf.q02",  0x080000, 0x080000, 0x8c66ae26 )
	ROM_LOAD( "ssf.q03",  0x100000, 0x080000, 0x695cc2ca )
	ROM_LOAD( "ssf.q04",  0x180000, 0x080000, 0x9d9ebe32 )
	ROM_LOAD( "ssf.q05",  0x200000, 0x080000, 0x4770e7b7 )
	ROM_LOAD( "ssf.q06",  0x280000, 0x080000, 0x4e79c951 )
	ROM_LOAD( "ssf.q07",  0x300000, 0x080000, 0xcdd14313 )
	ROM_LOAD( "ssf.q08",  0x380000, 0x080000, 0x6f5a088c )
ROM_END

ROM_START( ssf2j )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "ssfj.03b", 0x000000, 0x80000, 0x5c2e356d )
	ROM_LOAD16_WORD_SWAP( "ssfj.04a", 0x080000, 0x80000, 0x013bd55c )
	ROM_LOAD16_WORD_SWAP( "ssfj.05",  0x100000, 0x80000, 0x0918d19a )
	ROM_LOAD16_WORD_SWAP( "ssfj.06b", 0x180000, 0x80000, 0x014e0c6d )
	ROM_LOAD16_WORD_SWAP( "ssfj.07",  0x200000, 0x80000, 0xeb6a9b1b )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "ssfjx.03b", XOR_BASE+0x000000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "ssfjx.04a", XOR_BASE+0x080000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "ssfjx.05",  XOR_BASE+0x100000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "ssfjx.06b", XOR_BASE+0x180000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "ssfjx.07",  XOR_BASE+0x200000, 0x80000, 0x00000000 )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, 0xcf94d275, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, 0x5eb703af, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, 0xffa60e0f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, 0x34e825c5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, 0xb7cc32e7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, 0x8376ad18, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, 0xf5b1b336, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, 0x459d5c6b, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ssf.01",   0x00000, 0x08000, 0xeb247e8c )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "ssf.q01",  0x000000, 0x080000, 0xa6f9da5c )
	ROM_LOAD( "ssf.q02",  0x080000, 0x080000, 0x8c66ae26 )
	ROM_LOAD( "ssf.q03",  0x100000, 0x080000, 0x695cc2ca )
	ROM_LOAD( "ssf.q04",  0x180000, 0x080000, 0x9d9ebe32 )
	ROM_LOAD( "ssf.q05",  0x200000, 0x080000, 0x4770e7b7 )
	ROM_LOAD( "ssf.q06",  0x280000, 0x080000, 0x4e79c951 )
	ROM_LOAD( "ssf.q07",  0x300000, 0x080000, 0xcdd14313 )
	ROM_LOAD( "ssf.q08",  0x380000, 0x080000, 0x6f5a088c )
ROM_END

ROM_START( ssf2jr1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "ssfj.03a", 0x000000, 0x80000, 0x0bbf1304 )
	ROM_LOAD16_WORD_SWAP( "ssfj.04a", 0x080000, 0x80000, 0x013bd55c )
	ROM_LOAD16_WORD_SWAP( "ssfj.05",  0x100000, 0x80000, 0x0918d19a )
	ROM_LOAD16_WORD_SWAP( "ssfj.06",  0x180000, 0x80000, 0xd808a6cd )
	ROM_LOAD16_WORD_SWAP( "ssfj.07",  0x200000, 0x80000, 0xeb6a9b1b )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "ssfjx.03a", XOR_BASE+0x000000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "ssfjx.04a", XOR_BASE+0x080000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "ssfjx.05",  XOR_BASE+0x100000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "ssfjx.06",  XOR_BASE+0x180000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "ssfjx.07",  XOR_BASE+0x200000, 0x80000, 0x00000000 )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, 0xcf94d275, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, 0x5eb703af, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, 0xffa60e0f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, 0x34e825c5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, 0xb7cc32e7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, 0x8376ad18, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, 0xf5b1b336, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, 0x459d5c6b, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ssf.01",   0x00000, 0x08000, 0xeb247e8c )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "ssf.q01",  0x000000, 0x080000, 0xa6f9da5c )
	ROM_LOAD( "ssf.q02",  0x080000, 0x080000, 0x8c66ae26 )
	ROM_LOAD( "ssf.q03",  0x100000, 0x080000, 0x695cc2ca )
	ROM_LOAD( "ssf.q04",  0x180000, 0x080000, 0x9d9ebe32 )
	ROM_LOAD( "ssf.q05",  0x200000, 0x080000, 0x4770e7b7 )
	ROM_LOAD( "ssf.q06",  0x280000, 0x080000, 0x4e79c951 )
	ROM_LOAD( "ssf.q07",  0x300000, 0x080000, 0xcdd14313 )
	ROM_LOAD( "ssf.q08",  0x380000, 0x080000, 0x6f5a088c )
ROM_END

ROM_START( ssf2jr2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "ssfj.03", 0x000000, 0x80000, 0x7eb0efed )
	ROM_LOAD16_WORD_SWAP( "ssfj.04", 0x080000, 0x80000, 0xd7322164 )
	ROM_LOAD16_WORD_SWAP( "ssfj.05", 0x100000, 0x80000, 0x0918d19a )
	ROM_LOAD16_WORD_SWAP( "ssfj.06", 0x180000, 0x80000, 0xd808a6cd )
	ROM_LOAD16_WORD_SWAP( "ssfj.07", 0x200000, 0x80000, 0xeb6a9b1b )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "ssfjx.03", XOR_BASE+0x000000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "ssfjx.04", XOR_BASE+0x080000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "ssfjx.05", XOR_BASE+0x100000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "ssfjx.06", XOR_BASE+0x180000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "ssfjx.07", XOR_BASE+0x200000, 0x80000, 0x00000000 )

	ROM_REGION( 0xc00000, REGION_GFX1, 0 )
	ROMX_LOAD( "ssf.13",   0x000000, 0x200000, 0xcf94d275, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.15",   0x000002, 0x200000, 0x5eb703af, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.17",   0x000004, 0x200000, 0xffa60e0f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.19",   0x000006, 0x200000, 0x34e825c5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.14",   0x800000, 0x100000, 0xb7cc32e7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.16",   0x800002, 0x100000, 0x8376ad18, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.18",   0x800004, 0x100000, 0xf5b1b336, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "ssf.20",   0x800006, 0x100000, 0x459d5c6b, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "ssf.01",   0x00000, 0x08000, 0xeb247e8c )
	ROM_CONTINUE(         0x10000, 0x18000 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD( "ssf.q01",  0x000000, 0x080000, 0xa6f9da5c )
	ROM_LOAD( "ssf.q02",  0x080000, 0x080000, 0x8c66ae26 )
	ROM_LOAD( "ssf.q03",  0x100000, 0x080000, 0x695cc2ca )
	ROM_LOAD( "ssf.q04",  0x180000, 0x080000, 0x9d9ebe32 )
	ROM_LOAD( "ssf.q05",  0x200000, 0x080000, 0x4770e7b7 )
	ROM_LOAD( "ssf.q06",  0x280000, 0x080000, 0x4e79c951 )
	ROM_LOAD( "ssf.q07",  0x300000, 0x080000, 0xcdd14313 )
	ROM_LOAD( "ssf.q08",  0x380000, 0x080000, 0x6f5a088c )
ROM_END

ROM_START( ssf2t )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfxu.03c", 0x000000, 0x80000, 0x86e4a335 )
	ROM_LOAD16_WORD_SWAP( "sfxu.04a", 0x080000, 0x80000, 0x532b5ffd )
	ROM_LOAD16_WORD_SWAP( "sfxu.05",  0x100000, 0x80000, 0xffa3c6de )
	ROM_LOAD16_WORD_SWAP( "sfxu.06a", 0x180000, 0x80000, 0xe4c04c99 )
	ROM_LOAD16_WORD_SWAP( "sfxu.07",  0x200000, 0x80000, 0xd8199e41 )
	ROM_LOAD16_WORD_SWAP( "sfxu.08",  0x280000, 0x80000, 0xb3c71810 )
	ROM_LOAD16_WORD_SWAP( "sfx.09",   0x300000, 0x80000, 0x642fae3f )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "sfx.13",   0x0000000, 0x200000, 0xcf94d275, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.15",   0x0000002, 0x200000, 0x5eb703af, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.17",   0x0000004, 0x200000, 0xffa60e0f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.19",   0x0000006, 0x200000, 0x34e825c5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.14",   0x0800000, 0x100000, 0xb7cc32e7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.16",   0x0800002, 0x100000, 0x8376ad18, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.18",   0x0800004, 0x100000, 0xf5b1b336, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.20",   0x0800006, 0x100000, 0x459d5c6b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.21",   0x0c00000, 0x100000, 0xe32854af, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.23",   0x0c00002, 0x100000, 0x760f2927, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.25",   0x0c00004, 0x100000, 0x1ee90208, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.27",   0x0c00006, 0x100000, 0xf814400f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfx.01",   0x00000, 0x08000, 0xb47b8835 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfx.02",   0x28000, 0x20000, 0x0022633f )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfx.11",   0x000000, 0x200000, 0x9bdbd476 )
	ROM_LOAD16_WORD_SWAP( "sfx.12",   0x200000, 0x200000, 0xa05e3aab )
ROM_END

ROM_START( ssf2xj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "sfxj.03c", 0x000000, 0x80000, 0xa7417b79 )
	ROM_LOAD16_WORD_SWAP( "sfxj.04a", 0x080000, 0x80000, 0xaf7767b4 )
	ROM_LOAD16_WORD_SWAP( "sfxj.05",  0x100000, 0x80000, 0xf4ff18f5 )
	ROM_LOAD16_WORD_SWAP( "sfxj.06a", 0x180000, 0x80000, 0x260d0370 )
	ROM_LOAD16_WORD_SWAP( "sfxj.07",  0x200000, 0x80000, 0x1324d02a )
	ROM_LOAD16_WORD_SWAP( "sfxj.08",  0x280000, 0x80000, 0x2de76f10 )
	ROM_LOAD16_WORD_SWAP( "sfx.09",   0x300000, 0x80000, 0x642fae3f )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x1000000, REGION_GFX1, 0 )
	ROMX_LOAD( "sfx.13",   0x0000000, 0x200000, 0xcf94d275, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.15",   0x0000002, 0x200000, 0x5eb703af, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.17",   0x0000004, 0x200000, 0xffa60e0f, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.19",   0x0000006, 0x200000, 0x34e825c5, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.14",   0x0800000, 0x100000, 0xb7cc32e7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.16",   0x0800002, 0x100000, 0x8376ad18, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.18",   0x0800004, 0x100000, 0xf5b1b336, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.20",   0x0800006, 0x100000, 0x459d5c6b, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.21",   0x0c00000, 0x100000, 0xe32854af, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.23",   0x0c00002, 0x100000, 0x760f2927, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.25",   0x0c00004, 0x100000, 0x1ee90208, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "sfx.27",   0x0c00006, 0x100000, 0xf814400f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "sfx.01",   0x00000, 0x08000, 0xb47b8835 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "sfx.02",   0x28000, 0x20000, 0x0022633f )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "sfx.11",   0x000000, 0x200000, 0x9bdbd476 )
	ROM_LOAD16_WORD_SWAP( "sfx.12",   0x200000, 0x200000, 0xa05e3aab )
ROM_END

ROM_START( vhunt2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vh2j.03", 0x000000, 0x80000, 0x1a5feb13 )
	ROM_LOAD16_WORD_SWAP( "vh2j.04", 0x080000, 0x80000, 0x434611a5 )
	ROM_LOAD16_WORD_SWAP( "vh2j.05", 0x100000, 0x80000, 0xde34f624 )
	ROM_LOAD16_WORD_SWAP( "vh2j.06", 0x180000, 0x80000, 0x6a3b9897 )
	ROM_LOAD16_WORD_SWAP( "vh2j.07", 0x200000, 0x80000, 0xb021c029 )
	ROM_LOAD16_WORD_SWAP( "vh2j.08", 0x280000, 0x80000, 0xac873dff )
	ROM_LOAD16_WORD_SWAP( "vh2j.09", 0x300000, 0x80000, 0xeaefce9c )
	ROM_LOAD16_WORD_SWAP( "vh2j.10", 0x380000, 0x80000, 0x11730952 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vh2.13",   0x0000000, 0x400000, 0x3b02ddaa, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.15",   0x0000002, 0x400000, 0x4e40de66, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.17",   0x0000004, 0x400000, 0xb31d00c9, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.19",   0x0000006, 0x400000, 0x149be3ab, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.14",   0x1000000, 0x400000, 0xcd09bd63, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.16",   0x1000002, 0x400000, 0xe0182c15, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.18",   0x1000004, 0x400000, 0x778dc4f6, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vh2.20",   0x1000006, 0x400000, 0x605d9d1d, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vh2.01",  0x00000, 0x08000, 0x67b9f779 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vh2.02",  0x28000, 0x20000, 0xaaf15fcb )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vh2.11",  0x000000, 0x400000, 0x38922efd )
	ROM_LOAD16_WORD_SWAP( "vh2.12",  0x400000, 0x400000, 0x6e2430af )
ROM_END

ROM_START( vsav )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vm3u.03d", 0x000000, 0x80000, 0x1f295274 )
	ROM_LOAD16_WORD_SWAP( "vm3u.04d", 0x080000, 0x80000, 0xc46adf81 )
	ROM_LOAD16_WORD_SWAP( "vm3u.05a", 0x100000, 0x80000, 0x4118e00f )
	ROM_LOAD16_WORD_SWAP( "vm3u.06a", 0x180000, 0x80000, 0x2f4fd3a9 )
	ROM_LOAD16_WORD_SWAP( "vm3u.07b", 0x200000, 0x80000, 0xcbda91b8 )
	ROM_LOAD16_WORD_SWAP( "vm3u.08a", 0x280000, 0x80000, 0x6ca47259 )
	ROM_LOAD16_WORD_SWAP( "vm3u.09b", 0x300000, 0x80000, 0xf4a339e3 )
	ROM_LOAD16_WORD_SWAP( "vm3u.10b", 0x380000, 0x80000, 0xfffbb5b8 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "vm3ux.03d", XOR_BASE+0x000000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "vm3ux.04d", XOR_BASE+0x080000, 0x80000, 0x00000000 )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vm3.13",   0x0000000, 0x400000, 0xfd8a11eb, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.15",   0x0000002, 0x400000, 0xdd1e7d4e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.17",   0x0000004, 0x400000, 0x6b89445e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.19",   0x0000006, 0x400000, 0x3830fdc7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.14",   0x1000000, 0x400000, 0xc1a28e6c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.16",   0x1000002, 0x400000, 0x194a7304, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.18",   0x1000004, 0x400000, 0xdf9a9f47, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.20",   0x1000006, 0x400000, 0xc22fc3d9, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vm3.01",   0x00000, 0x08000, 0xf778769b )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vm3.02",   0x28000, 0x20000, 0xcc09faa1 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vm3.11",   0x000000, 0x400000, 0xe80e956e )
	ROM_LOAD16_WORD_SWAP( "vm3.12",   0x400000, 0x400000, 0x9cd71557 )
ROM_END

ROM_START( vsavj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vm3j.03d", 0x000000, 0x80000, 0x2a2e74a4 )
	ROM_LOAD16_WORD_SWAP( "vm3j.04d", 0x080000, 0x80000, 0x1c2427bc )
	ROM_LOAD16_WORD_SWAP( "vm3j.05a", 0x100000, 0x80000, 0x95ce88d5 )
	ROM_LOAD16_WORD_SWAP( "vm3j.06b", 0x180000, 0x80000, 0x2c4297e0 )
	ROM_LOAD16_WORD_SWAP( "vm3j.07b", 0x200000, 0x80000, 0xa38aaae7 )
	ROM_LOAD16_WORD_SWAP( "vm3j.08a", 0x280000, 0x80000, 0x5773e5c9 )
	ROM_LOAD16_WORD_SWAP( "vm3j.09b", 0x300000, 0x80000, 0xd064f8b9 )
	ROM_LOAD16_WORD_SWAP( "vm3j.10b", 0x380000, 0x80000, 0x434518e9 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "vm3jx.03d", XOR_BASE+0x000000, 0x80000, 0xa9ab54df )
	ROM_LOAD16_WORD_SWAP( "vm3jx.04d", XOR_BASE+0x080000, 0x80000, 0x20c4aa2d )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vm3.13",   0x0000000, 0x400000, 0xfd8a11eb, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.15",   0x0000002, 0x400000, 0xdd1e7d4e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.17",   0x0000004, 0x400000, 0x6b89445e, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.19",   0x0000006, 0x400000, 0x3830fdc7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.14",   0x1000000, 0x400000, 0xc1a28e6c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.16",   0x1000002, 0x400000, 0x194a7304, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.18",   0x1000004, 0x400000, 0xdf9a9f47, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vm3.20",   0x1000006, 0x400000, 0xc22fc3d9, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vm3.01",   0x00000, 0x08000, 0xf778769b )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vm3.02",   0x28000, 0x20000, 0xcc09faa1 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vm3.11",   0x000000, 0x400000, 0xe80e956e )
	ROM_LOAD16_WORD_SWAP( "vm3.12",   0x400000, 0x400000, 0x9cd71557 )
ROM_END

ROM_START( vsav2 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "vs2j.03", 0x000000, 0x80000, 0x89fd86b4 )
	ROM_LOAD16_WORD_SWAP( "vs2j.04", 0x080000, 0x80000, 0x107c091b )
	ROM_LOAD16_WORD_SWAP( "vs2j.05", 0x100000, 0x80000, 0x61979638 )
	ROM_LOAD16_WORD_SWAP( "vs2j.06", 0x180000, 0x80000, 0xf37c5bc2 )
	ROM_LOAD16_WORD_SWAP( "vs2j.07", 0x200000, 0x80000, 0x8f885809 )
	ROM_LOAD16_WORD_SWAP( "vs2j.08", 0x280000, 0x80000, 0x2018c120 )
	ROM_LOAD16_WORD_SWAP( "vs2j.09", 0x300000, 0x80000, 0xfac3c217 )
	ROM_LOAD16_WORD_SWAP( "vs2j.10", 0x380000, 0x80000, 0xeb490213 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "vs2.13",   0x0000000, 0x400000, 0x5c852f52, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vs2.15",   0x0000002, 0x400000, 0xa20f58af, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vs2.17",   0x0000004, 0x400000, 0x39db59ad, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vs2.19",   0x0000006, 0x400000, 0x00c763a7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vs2.14",   0x1000000, 0x400000, 0xcd09bd63, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vs2.16",   0x1000002, 0x400000, 0xe0182c15, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vs2.18",   0x1000004, 0x400000, 0x778dc4f6, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "vs2.20",   0x1000006, 0x400000, 0x605d9d1d, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "vs2.01",   0x00000, 0x08000, 0x35190139 )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "vs2.02",   0x28000, 0x20000, 0xc32dba09 )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "vs2.11",   0x000000, 0x400000, 0xd67e47b7 )
	ROM_LOAD16_WORD_SWAP( "vs2.12",   0x400000, 0x400000, 0x6d020a14 )
ROM_END

ROM_START( xmcota )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xmnu.03e", 0x000000, 0x80000, 0x0bafeb0e )
	ROM_LOAD16_WORD_SWAP( "xmnu.04e", 0x080000, 0x80000, 0xc29bdae3 )
	ROM_LOAD16_WORD_SWAP( "xmnu.05a", 0x100000, 0x80000, 0xac0d7759 )
	ROM_LOAD16_WORD_SWAP( "xmnu.06a", 0x180000, 0x80000, 0x6a3f0924 )
	ROM_LOAD16_WORD_SWAP( "xmnu.07a", 0x200000, 0x80000, 0x2c142a44 )
	ROM_LOAD16_WORD_SWAP( "xmnu.08a", 0x280000, 0x80000, 0xf712d44f )
	ROM_LOAD16_WORD_SWAP( "xmnu.09a", 0x300000, 0x80000, 0xc24db29a )
	ROM_LOAD16_WORD_SWAP( "xmnu.10a", 0x380000, 0x80000, 0x53c0eab9 )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "xmnux.03e", XOR_BASE+0x000000, 0x80000, 0x00000000 )
	ROM_LOAD16_WORD_SWAP( "xmnux.04e", XOR_BASE+0x080000, 0x80000, 0x00000000 )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xmn.13",   0x0000000, 0x400000, 0xbf4df073, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.15",   0x0000002, 0x400000, 0x4d7e4cef, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.17",   0x0000004, 0x400000, 0x513eea17, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.19",   0x0000006, 0x400000, 0xd23897fc, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.14",   0x1000000, 0x400000, 0x778237b7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.16",   0x1000002, 0x400000, 0x67b36948, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.18",   0x1000004, 0x400000, 0x015a7c4c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.20",   0x1000006, 0x400000, 0x9dde2758, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xmn.01",   0x00000, 0x08000, 0x40f479ea )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xmn.02",   0x28000, 0x20000, 0x39d9b5ad )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xmn.11",   0x000000, 0x200000, 0xc848a6bc )
	ROM_LOAD16_WORD_SWAP( "xmn.12",   0x200000, 0x200000, 0x729c188f )
ROM_END

ROM_START( xmcotaj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xmnj.03b", 0x000000, 0x80000, 0xc8175fb3 )
	ROM_LOAD16_WORD_SWAP( "xmnj.04b", 0x080000, 0x80000, 0x54b3fba3 )
	ROM_LOAD16_WORD_SWAP( "xmnj.05",  0x100000, 0x80000, 0xc3ed62a2 )
	ROM_LOAD16_WORD_SWAP( "xmnj.06",  0x180000, 0x80000, 0xf03c52e1 )
	ROM_LOAD16_WORD_SWAP( "xmnj.07",  0x200000, 0x80000, 0x325626b1 )
	ROM_LOAD16_WORD_SWAP( "xmnj.08",  0x280000, 0x80000, 0x7194ea10 )
	ROM_LOAD16_WORD_SWAP( "xmnj.09",  0x300000, 0x80000, 0xae946df3 )
	ROM_LOAD16_WORD_SWAP( "xmnj.10",  0x380000, 0x80000, 0x32a6be1d )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "xmnjx.03b", XOR_BASE+0x000000, 0x80000, 0x523c9589 )
	ROM_LOAD16_WORD_SWAP( "xmnjx.04b", XOR_BASE+0x080000, 0x80000, 0x673765ba )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xmn.13",   0x0000000, 0x400000, 0xbf4df073, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.15",   0x0000002, 0x400000, 0x4d7e4cef, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.17",   0x0000004, 0x400000, 0x513eea17, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.19",   0x0000006, 0x400000, 0xd23897fc, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.14",   0x1000000, 0x400000, 0x778237b7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.16",   0x1000002, 0x400000, 0x67b36948, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.18",   0x1000004, 0x400000, 0x015a7c4c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.20",   0x1000006, 0x400000, 0x9dde2758, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xmn.01",   0x00000, 0x08000, 0x40f479ea )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xmn.02",   0x28000, 0x20000, 0x39d9b5ad )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xmn.11",   0x000000, 0x200000, 0xc848a6bc )
	ROM_LOAD16_WORD_SWAP( "xmn.12",   0x200000, 0x200000, 0x729c188f )
ROM_END

ROM_START( xmcotaj1 )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xmnj.03a", 0x000000, 0x80000, 0x00761611 )
	ROM_LOAD16_WORD_SWAP( "xmnj.04a", 0x080000, 0x80000, 0x614d3f60 )
	ROM_LOAD16_WORD_SWAP( "xmnj.05",  0x100000, 0x80000, 0xc3ed62a2 )
	ROM_LOAD16_WORD_SWAP( "xmnj.06",  0x180000, 0x80000, 0xf03c52e1 )
	ROM_LOAD16_WORD_SWAP( "xmnj.07",  0x200000, 0x80000, 0x325626b1 )
	ROM_LOAD16_WORD_SWAP( "xmnj.08",  0x280000, 0x80000, 0x7194ea10 )
	ROM_LOAD16_WORD_SWAP( "xmnj.09",  0x300000, 0x80000, 0xae946df3 )
	ROM_LOAD16_WORD_SWAP( "xmnj.10",  0x380000, 0x80000, 0x32a6be1d )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)
	ROM_LOAD16_WORD_SWAP( "xmnjx.03a", XOR_BASE+0x000000, 0x80000, 0x515b9bf9 )
	ROM_LOAD16_WORD_SWAP( "xmnjx.04a", XOR_BASE+0x080000, 0x80000, 0x5419572b )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xmn.13",   0x0000000, 0x400000, 0xbf4df073, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.15",   0x0000002, 0x400000, 0x4d7e4cef, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.17",   0x0000004, 0x400000, 0x513eea17, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.19",   0x0000006, 0x400000, 0xd23897fc, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.14",   0x1000000, 0x400000, 0x778237b7, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.16",   0x1000002, 0x400000, 0x67b36948, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.18",   0x1000004, 0x400000, 0x015a7c4c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xmn.20",   0x1000006, 0x400000, 0x9dde2758, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xmn.01",   0x00000, 0x08000, 0x40f479ea )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xmn.02",   0x28000, 0x20000, 0x39d9b5ad )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xmn.11",   0x000000, 0x200000, 0xc848a6bc )
	ROM_LOAD16_WORD_SWAP( "xmn.12",   0x200000, 0x200000, 0x729c188f )
ROM_END

ROM_START( xmvsf )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xvsu.03i", 0x000000, 0x80000, 0x5481155a )
	ROM_LOAD16_WORD_SWAP( "xvsu.04i", 0x080000, 0x80000, 0x1e236388 )
	ROM_LOAD16_WORD_SWAP( "xvs.05a",  0x100000, 0x80000, 0x7db6025d )
	ROM_LOAD16_WORD_SWAP( "xvs.06a",  0x180000, 0x80000, 0xe8e2c75c )
	ROM_LOAD16_WORD_SWAP( "xvs.07",   0x200000, 0x80000, 0x08f0abed )
	ROM_LOAD16_WORD_SWAP( "xvs.08",   0x280000, 0x80000, 0x81929675 )
	ROM_LOAD16_WORD_SWAP( "xvs.09",   0x300000, 0x80000, 0x9641f36b )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xvs.13",   0x0000000, 0x400000, 0xf6684efd, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.15",   0x0000002, 0x400000, 0x29109221, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.17",   0x0000004, 0x400000, 0x92db3474, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.19",   0x0000006, 0x400000, 0x3733473c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.14",   0x1000000, 0x400000, 0xbcac2e41, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.16",   0x1000002, 0x400000, 0xea04a272, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.18",   0x1000004, 0x400000, 0xb0def86a, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.20",   0x1000006, 0x400000, 0x4b40ff9f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xvs.01",   0x00000, 0x08000, 0x3999e93a )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xvs.02",   0x28000, 0x20000, 0x101bdee9 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xvs.11",   0x000000, 0x200000, 0x9cadcdbc )
	ROM_LOAD16_WORD_SWAP( "xvs.12",   0x200000, 0x200000, 0x7b11e460 )
ROM_END

ROM_START( xmvsfj )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xvsj.03d", 0x000000, 0x80000, 0xbeb81de9 )
	ROM_LOAD16_WORD_SWAP( "xvsj.04d", 0x080000, 0x80000, 0x23d11271 )
	ROM_LOAD16_WORD_SWAP( "xvs.05a",  0x100000, 0x80000, 0x7db6025d )
	ROM_LOAD16_WORD_SWAP( "xvs.06a",  0x180000, 0x80000, 0xe8e2c75c )
	ROM_LOAD16_WORD_SWAP( "xvs.07",   0x200000, 0x80000, 0x08f0abed )
	ROM_LOAD16_WORD_SWAP( "xvs.08",   0x280000, 0x80000, 0x81929675 )
	ROM_LOAD16_WORD_SWAP( "xvs.09",   0x300000, 0x80000, 0x9641f36b )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xvs.13",   0x0000000, 0x400000, 0xf6684efd, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.15",   0x0000002, 0x400000, 0x29109221, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.17",   0x0000004, 0x400000, 0x92db3474, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.19",   0x0000006, 0x400000, 0x3733473c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.14",   0x1000000, 0x400000, 0xbcac2e41, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.16",   0x1000002, 0x400000, 0xea04a272, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.18",   0x1000004, 0x400000, 0xb0def86a, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.20",   0x1000006, 0x400000, 0x4b40ff9f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xvs.01",   0x00000, 0x08000, 0x3999e93a )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xvs.02",   0x28000, 0x20000, 0x101bdee9 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xvs.11",   0x000000, 0x200000, 0x9cadcdbc )
	ROM_LOAD16_WORD_SWAP( "xvs.12",   0x200000, 0x200000, 0x7b11e460 )
ROM_END

ROM_START( xmvsfa )
	ROM_REGION( CODE_SIZE, REGION_CPU1, 0 )      /* 68000 code */
	ROM_LOAD16_WORD_SWAP( "xvsa.03", 0x000000, 0x80000, 0xd0cca7a8 )
	ROM_LOAD16_WORD_SWAP( "xvsa.04", 0x080000, 0x80000, 0x8c8e76fd )
	ROM_LOAD16_WORD_SWAP( "xvs.05a", 0x100000, 0x80000, 0x7db6025d )
	ROM_LOAD16_WORD_SWAP( "xvs.06a", 0x180000, 0x80000, 0xe8e2c75c )
	ROM_LOAD16_WORD_SWAP( "xvs.07",  0x200000, 0x80000, 0x08f0abed )
	ROM_LOAD16_WORD_SWAP( "xvs.08",  0x280000, 0x80000, 0x81929675 )
	ROM_LOAD16_WORD_SWAP( "xvs.09",  0x300000, 0x80000, 0x9641f36b )

	ROM_FILL(XOR_BASE, XOR_BASE, 0)

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROMX_LOAD( "xvs.13",   0x0000000, 0x400000, 0xf6684efd, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.15",   0x0000002, 0x400000, 0x29109221, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.17",   0x0000004, 0x400000, 0x92db3474, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.19",   0x0000006, 0x400000, 0x3733473c, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.14",   0x1000000, 0x400000, 0xbcac2e41, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.16",   0x1000002, 0x400000, 0xea04a272, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.18",   0x1000004, 0x400000, 0xb0def86a, ROM_GROUPWORD | ROM_SKIP(6) )
	ROMX_LOAD( "xvs.20",   0x1000006, 0x400000, 0x4b40ff9f, ROM_GROUPWORD | ROM_SKIP(6) )

	ROM_REGION( QSOUND_SIZE, REGION_CPU2, 0 ) /* 64k for the audio CPU (+banks) */
	ROM_LOAD( "xvs.01",   0x00000, 0x08000, 0x3999e93a )
	ROM_CONTINUE(         0x10000, 0x18000 )
	ROM_LOAD( "xvs.02",   0x28000, 0x20000, 0x101bdee9 )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 ) /* QSound samples */
	ROM_LOAD16_WORD_SWAP( "xvs.11",   0x000000, 0x200000, 0x9cadcdbc )
	ROM_LOAD16_WORD_SWAP( "xvs.12",   0x200000, 0x200000, 0x7b11e460 )
ROM_END


GAME( 1995, 19xx,     0,       cps2, cps2,  cps2, ROT270_16BIT, "Capcom", "19XX: The War Against Destiny (US 951207)" )
GAME( 1995, 19xxj,    19xx,    cps2, cps2,  cps2, ROT270_16BIT, "Capcom", "19XX: The War Against Destiny (Japan 951207)" )
GAME( 1995, 19xxh,    19xx,    cps2, cps2,  cps2, ROT270_16BIT, "Capcom", "19XX: The War Against Destiny (Hispanic)" )
GAME( 1994, armwar,   0,       cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Armoured Warriors (US 941024)" )
GAME( 1994, pgear,    armwar,  cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Powered Gear(Japan 940916)" )
GAME( 1994, armwara,  armwar,  cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Armoured Warriors (Asia 940920)" )
GAME( 1994, avsp,     0,       cps2, avsp,  cps2, ROT0_16BIT,   "Capcom", "Aliens vs. Predator (US 940520)" )
GAME( 1994, avspa,    avsp,    cps2, avsp,  cps2, ROT0_16BIT,   "Capcom", "Aliens vs. Predator (Asia 940520)" )
GAME( 1997, batcirj,  0,       cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Battle Circuit (Japan 970319)" )
GAME( 1997, batcira,  batcirj, cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Battle Circuit (Asia 970319)" )
GAME( 1995, cybotsj,  0,       cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Cyberbots: Full Metal Madness (Japan 950420)" )
GAME( 1993, ddtod,    0,       cps2, ddtod, cps2, ROT0_16BIT,   "Capcom", "Dungeons & Dragons: Tower of Doom (US 940125)" )
GAME( 1993, ddtodr1,  ddtod,   cps2, ddtod, cps2, ROT0_16BIT,   "Capcom", "Dungeons & Dragons: Tower of Doom (US 940113)" )
GAME( 1993, ddtodj,   ddtod,   cps2, ddtod, cps2, ROT0_16BIT,   "Capcom", "Dungeons & Dragons: Tower of Doom (Japan 940113)" )
GAME( 1993, ddtoda,   ddtod,   cps2, ddtod, cps2, ROT0_16BIT,   "Capcom", "Dungeons & Dragons: Tower of Doom (Asia 940113)" )
GAME( 1996, ddsom,    0,       cps2, ddtod, cps2, ROT0_16BIT,   "Capcom", "Dungeons & Dragons 2: Shadow over Mystara (US 960209)" )
GAME( 1996, ddsomj,   ddsom,   cps2, ddtod, cps2, ROT0_16BIT,   "Capcom", "Dungeons & Dragons 2: Shadow over Mystara (Japan 960206)" )
GAME( 1994, dstlk,    0,       cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "DarkStalkers: The Night Warriors (US 940818)" )
GAME( 1994, vampj,    dstlk,   cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Vampire: The Night Warriors (Japan 940705)" )
GAME( 1994, vampja,   dstlk,   cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Vampire: The Night Warriors (Japan 940705 alt)" )
GAME( 1994, vampa,    dstlk,   cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Vampire: The Night Warriors (Asia 940705)" )
GAME( 1993, ecofe,    0,       cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Eco Fighters (Etc 931203)" )
GAME( 1993, uecology, ecofe,   cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Ultimate Ecology (Japan 931203)" )
GAME( 1995, msh,      0,       cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Marvel Super Heroes (US 951024)" )
GAME( 1995, mshj,     msh,     cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Marvel Super Heroes (Japan 951024)" )
GAME( 1995, mshh,     msh,     cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Marvel Super Heroes (Hispanic)" )
GAME( 1997, mshvsf,   0,       cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Marvel Super Heroes Vs. Street Fighter (US 970625)" )
GAME( 1997, mshvsfj,  mshvsf,  cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Marvel Super Heroes Vs. Street Fighter (Japan 970707)" )
GAME( 1998, mvsc,     0,       cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Marvel Super Heroes vs. Capcom: Clash of Super Heroes (US 980123)" )
GAME( 1998, mvscj,    mvsc,    cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Marvel Super Heroes vs. Capcom: Clash of Super Heroes (Japan 980123)" )
GAME( 1998, mvsch,    mvsc,    cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Marvel Super Heroes vs. Capcom: Clash of Super Heroes (Hispanic 980123)" )
GAME( 1995, nwarr,    0,       cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Night Warriors: DarkStalkers Revenge (US 950406)" )
GAME( 1995, vhuntj,   nwarr,   cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Vampire Hunter: DarkStalkers Revenge (Japan 950302)" )
GAME( 1996, rckman2j, 0,       cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Rockman 2: The Power Fighters (Japan 960708)" )
GAME( 1996, qndream,  0,       cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Quiz Nanairo Dreams: Miracle of the Rainbow Colored Town (Japan 960626)" )
GAME( 1995, sfa,      0,       cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Street Fighter Alpha: Warriors' Dreams (US 950627)" )
GAME( 1995, sfaer1,   sfa,     cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Street Fighter Alpha: Warriors' Dreams (Euro 950727)" )
GAME( 1995, sfzj,     sfa,     cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Street Fighter Zero (Japan 950727)" )
GAME( 1995, sfzjr1,   sfa,     cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Street Fighter Zero (Japan 950627)" )
GAME( 1995, sfzjr2,   sfa,     cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Street Fighter Zero (Japan 950605)" )
GAME( 1995, sfzh,     sfa,     cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Street Fighter Zero (Hispanic 950627)" )
GAME( 1996, sfa2,     0,       cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Street Fighter Alpha 2 (US 960306)" )
GAME( 1996, sfz2,     sfa2,    cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Street Fighter Zero 2 (Japan 960227)" )
GAME( 1996, sfz2a,    sfa2,    cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Street Fighter Zero 2 Alpha (Japan 960805)" )
GAME( 1998, sfa3,     0,       cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Street Fighter Alpha 3 (US 980629)" )
GAME( 1998, sfz3,     sfa3,    cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Street Fighter Zero 3 (US 980727)" )
GAME( 1997, sgemf,    0,       cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Super Gem Fighter Mini Mix (US 970904)" )
GAME( 1997, pfghtj,   sgemf,   cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Pocket Fighter (Japan 970904)" )
GAME( 1997, sgemfh,   sgemf,   cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Pocket Fighter (Hispanic 970904)" )
GAME( 1994, slam2e,   0,       cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Saturday Night Slammasters II: Ring of Destruction (Euro 940902)" )
GAME( 1994, smbomber, slam2e,  cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Super Muscle Bomber: The International Blowout (Japan 940808)" )
GAME( 1996, spf2t,    0,       cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Super Puzzle Fighter 2 Turbo (US 960620)" )
GAME( 1996, spf2xj,   spf2t,   cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Super Puzzle Fighter 2 X (Japan 960531)" )
GAME( 1993, ssf2,     0,       cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Super Street Fighter 2: The New Challengers (US 930911)" )
GAME( 1993, ssf2a,    ssf2,    cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Super Street Fighter 2: The New Challengers (Asia 930911)" )
GAME( 1993, ssf2j,    ssf2,    cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Super Street Fighter 2: The New Challengers (Japan 931005)" )
GAME( 1993, ssf2jr1,  ssf2,    cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Super Street Fighter 2: The New Challengers (Japan 930911)" )
GAME( 1993, ssf2jr2,  ssf2,    cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Super Street Fighter 2: The New Challengers (Japan 930910)" )
GAME( 1994, ssf2t,    0,       cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Super Street Fighter 2 Turbo (US 940223)" )
GAME( 1994, ssf2xj,   ssf2t,   cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Super Street Fighter 2 X: Grand Master Challenge (Japan 940223)" )
GAME( 1997, vhunt2,   0,       cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Vampire Hunter 2: Darkstalkers Revenge (Japan 970913)" )
GAME( 1997, vsav,     0,       cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Vampire Savior: The Lord of Vampire (US 970519)" )
GAME( 1997, vsavj,    vsav,    cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "Vampire Savior: The Lord of Vampire (Japan 970519)" )
GAME( 1997, vsav2,    0,       cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "Vampire Savior 2: The Lord of Vampire (Japan 970913)" )
GAME( 1994, xmcota,   0,       cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "X-Men: Children of the Atom (US 950105)" )
GAME( 1994, xmcotaj,  xmcota,  cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "X-Men: Children of the Atom (Japan 941219)" )
GAME( 1994, xmcotaj1, xmcota,  cps2, ssf2,  cps2, ROT0_16BIT,   "Capcom", "X-Men: Children of the Atom (Japan 941217)" )
GAME( 1996, xmvsf,    0,       cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "X-Men Vs. Street Fighter (US 961004)" )
GAME( 1996, xmvsfj,   xmvsf,   cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "X-Men Vs. Street Fighter (Japan 960910)" )
GAME( 1996, xmvsfa,   xmvsf,   cps2, cps2,  cps2, ROT0_16BIT,   "Capcom", "X-Men Vs. Street Fighter (Asia 961023)" )
