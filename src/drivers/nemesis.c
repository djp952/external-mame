/***************************************************************************

	Nemesis (Hacked?)		GX400
	Nemesis (World?)		GX400
	Twin Bee				GX412
	Gradius					GX456
	Galactic Warriors		GX578
	Konami GT				GX561
	RF2						GX561
	Salamander (Version D)	GX587
	Salamander (Version J)	GX587
	Lifeforce (US)			GX587
	Lifeforce (Japan)		GX587
	Black Panther			GX604
	City Bomber (World)		GX787
	City Bomber (Japan)		GX787
	Kitten Kaboodle			GX712
	Nyan Nyan Panic (Japan)	GX712

driver by Bryan McPhail
modified by Eisuke Watanabe
 spthx to Unagi,rassy,hina,nori,Tobikage,Tommy,Crimson,yasuken,cupmen,zoo
TODO:
- 'Your Stage Position' gauge is incorrect (nyanpani / kittenk ).

Notes:
- blkpnthr:
There are sprite priority problems in upper part of the screen ,
they can only be noticed in 2nd and 4th level .
Enemy sprites are behind blue walls 2 level) or metal construction (4 )
but when they get close to top of the screen they go in front of them.
--
To display score, priority of upper part is always lower.
So this is the correct behavior of real hardware, not an emulation bug.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"

static data16_t *ram;
static data16_t *ram2;

extern data16_t *nemesis_videoram1b;
extern data16_t *nemesis_videoram1f;
extern data16_t *nemesis_videoram2b;
extern data16_t *nemesis_videoram2f;
extern data16_t *nemesis_characterram;
extern data16_t *nemesis_xscroll1,*nemesis_xscroll2, *nemesis_yscroll;
extern size_t nemesis_characterram_size;

READ16_HANDLER( nemesis_videoram1b_word_r );
READ16_HANDLER( nemesis_videoram1f_word_r );
WRITE16_HANDLER( nemesis_videoram1b_word_w );
WRITE16_HANDLER( nemesis_videoram1f_word_w );
READ16_HANDLER( nemesis_videoram2b_word_r );
READ16_HANDLER( nemesis_videoram2f_word_r );
WRITE16_HANDLER( nemesis_videoram2b_word_w );
WRITE16_HANDLER( nemesis_videoram2f_word_w );
READ16_HANDLER( nemesis_characterram_word_r );
WRITE16_HANDLER( nemesis_characterram_word_w );
VIDEO_UPDATE( nemesis );
VIDEO_START( nemesis );

VIDEO_UPDATE( twinbee );
VIDEO_UPDATE( salamand );
MACHINE_INIT( nemesis );

WRITE16_HANDLER( salamander_palette_word_w );


WRITE16_HANDLER( gx400_xscroll1_word_w );
WRITE16_HANDLER( gx400_xscroll2_word_w );
WRITE16_HANDLER( gx400_yscroll_word_w );
WRITE16_HANDLER( gx400_yscroll1_word_w );
WRITE16_HANDLER( gx400_yscroll2_word_w );
READ16_HANDLER( gx400_xscroll1_word_r );
READ16_HANDLER( gx400_xscroll2_word_r );
READ16_HANDLER( gx400_yscroll_word_r );
READ16_HANDLER( gx400_yscroll1_word_r );
READ16_HANDLER( gx400_yscroll2_word_r );

extern data16_t *nemesis_yscroll1, *nemesis_yscroll2;


WRITE16_HANDLER( nemesis_palette_word_w );

int irq_on = 0;
int irq1_on = 0;
int irq2_on = 0;
int irq4_on = 0;


MACHINE_INIT( nemesis )
{
	irq_on = 0;
	irq1_on = 0;
	irq2_on = 0;
	irq4_on = 0;
}



INTERRUPT_GEN( nemesis_interrupt )
{
	if (irq_on)
		cpu_set_irq_line(0, 1, HOLD_LINE);
}

WRITE16_HANDLER( salamand_soundlatch_word_w )
{
	if(ACCESSING_LSB) {
		soundlatch_w(offset,data & 0xff);
		cpu_set_irq_line(1,0,HOLD_LINE);
	}

//logerror("z80 data write\n");

//cpu_set_irq_line(1,IRQ_LINE_NMI,PULSE_LINE);
}

static int gx400_irq1_cnt;

INTERRUPT_GEN( konamigt_interrupt )
{
	if (cpu_getiloops() == 0)
	{
		if ( (irq_on) && (gx400_irq1_cnt++ & 1) ) cpu_set_irq_line(0, 1, HOLD_LINE);
	}
	else
	{
		if (irq2_on) cpu_set_irq_line(0, 2, HOLD_LINE);
	}
}

INTERRUPT_GEN( gx400_interrupt )
{
	switch (cpu_getiloops())
	{
		case 0:
			if (irq2_on) cpu_set_irq_line(0, 2, HOLD_LINE);
			break;

		case 1:
			if ( (irq1_on) && (gx400_irq1_cnt++ & 1) ) cpu_set_irq_line(0, 1, HOLD_LINE);
			break;

		case 2:
			if (irq4_on) cpu_set_irq_line(0, 4, HOLD_LINE);
			break;
	}
}

WRITE16_HANDLER( gx400_irq1_enable_word_w )
{
	if (ACCESSING_LSB)
		irq1_on = data & 0x0001;
/*	else
logerror("irq1en = %08x\n",data);*/
}

WRITE16_HANDLER( gx400_irq2_enable_word_w )
{
	if (ACCESSING_LSB)
		irq2_on = data & 0x0001;
/*	else
logerror("irq2en = %08x\n",data);*/
}

WRITE16_HANDLER( gx400_irq4_enable_word_w )
{
	if (ACCESSING_MSB)
		irq4_on = data & 0x0100;
/*	else
logerror("irq4en = %08x\n",data);*/
}

static unsigned char *gx400_shared_ram;

READ16_HANDLER( gx400_sharedram_word_r )
{
	return gx400_shared_ram[offset];
}

WRITE16_HANDLER( gx400_sharedram_word_w )
{
	if(ACCESSING_LSB)
		gx400_shared_ram[offset] = data;
}



INTERRUPT_GEN( salamand_interrupt )
{
	if (irq_on)
		cpu_set_irq_line(0, 1, HOLD_LINE);
}

INTERRUPT_GEN( blkpnthr_interrupt )
{
	if (irq_on)
		cpu_set_irq_line(0, 2, HOLD_LINE);
}

WRITE16_HANDLER( nemesis_irq_enable_word_w )
{
	if(ACCESSING_LSB)
		irq_on = data & 0xff;
}

WRITE16_HANDLER( konamigt_irq_enable_word_w )
{
	if(ACCESSING_LSB)
		irq_on = data & 0xff;
}

WRITE16_HANDLER( konamigt_irq2_enable_word_w )
{
	if(ACCESSING_LSB)
		irq2_on = data & 0xff;
}

READ16_HANDLER( konamigt_input_word_r )
{
/*
	bit 0-7:   steering
	bit 8-9:   brake
	bit 10-11: unknown
	bit 12-15: accel
*/

	int data=readinputport(7);
	int data2=readinputport(6);

	int ret=0x0000;

//	if(data&0x10) ret|=0x0800;			// turbo/gear?
//	if(data&0x80) ret|=0x0400;			// turbo?
	if(data&0x20) ret|=0x0300;			// brake		(0-3)

	if(data&0x40) ret|=0xf000;			// accel		(0-f)

	ret|=data2&0x7f;					// steering wheel, not exactly sure if DIAL works ok.

	return ret;
}

WRITE16_HANDLER( nemesis_soundlatch_word_w )
{
	if(ACCESSING_LSB) {
		soundlatch_w(offset,data & 0xff);

		/* the IRQ should probably be generated by 5e004, but we'll handle it here for now */
		cpu_set_irq_line_and_vector(1,0,HOLD_LINE,0xff);
	}
}

static MEMORY_READ16_START( readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x040000, 0x04ffff, nemesis_characterram_word_r },
	{ 0x050000, 0x0503ff, MRA16_RAM },
	{ 0x050400, 0x0507ff, MRA16_RAM },
	{ 0x050800, 0x050bff, MRA16_RAM },
	{ 0x050c00, 0x050fff, MRA16_RAM },

	{ 0x052000, 0x052fff, nemesis_videoram1b_word_r },
	{ 0x053000, 0x053fff, nemesis_videoram1f_word_r },
	{ 0x054000, 0x054fff, nemesis_videoram2b_word_r },
	{ 0x055000, 0x055fff, nemesis_videoram2f_word_r },
	{ 0x056000, 0x056fff, MRA16_RAM },
	{ 0x05a000, 0x05afff, MRA16_RAM },

	{ 0x05c400, 0x05c401, input_port_4_word_r },	/* DSW0 */
	{ 0x05c402, 0x05c403, input_port_5_word_r },	/* DSW1 */

	{ 0x05cc00, 0x05cc01, input_port_0_word_r },	/* IN0 */
	{ 0x05cc02, 0x05cc03, input_port_1_word_r },	/* IN1 */
	{ 0x05cc04, 0x05cc05, input_port_2_word_r },	/* IN2 */
	{ 0x05cc06, 0x05cc07, input_port_3_word_r },	/* TEST */

	{ 0x060000, 0x067fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },	/* ROM */

	{ 0x040000, 0x04ffff, nemesis_characterram_word_w, &nemesis_characterram, &nemesis_characterram_size },

	{ 0x050000, 0x0503ff, MWA16_RAM, &nemesis_xscroll1 },
	{ 0x050400, 0x0507ff, MWA16_RAM, &nemesis_xscroll2 },
	{ 0x050800, 0x050bff, MWA16_RAM },
	{ 0x050c00, 0x050fff, MWA16_RAM, &nemesis_yscroll },
	{ 0x051000, 0x051fff, MWA16_NOP },		/* used, but written to with 0's */

	{ 0x052000, 0x052fff, nemesis_videoram1b_word_w, &nemesis_videoram1b },	/* VRAM 1 */
	{ 0x053000, 0x053fff, nemesis_videoram1f_word_w, &nemesis_videoram1f },	/* VRAM 1 */
	{ 0x054000, 0x054fff, nemesis_videoram2b_word_w, &nemesis_videoram2b },	/* VRAM 2 */
	{ 0x055000, 0x055fff, nemesis_videoram2f_word_w, &nemesis_videoram2f },	/* VRAM 2 */
	{ 0x056000, 0x056fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x05a000, 0x05afff, nemesis_palette_word_w, &paletteram16 },

	{ 0x05c000, 0x05c001, nemesis_soundlatch_word_w },
	{ 0x05c800, 0x05c801, watchdog_reset16_w },	/* probably */

	{ 0x05e000, 0x05e001, &nemesis_irq_enable_word_w },	/* Nemesis */
	{ 0x05e002, 0x05e003, &nemesis_irq_enable_word_w },	/* Konami GT */
	{ 0x05e004, 0x05e005, MWA16_NOP},	/* bit 8 of the word probably triggers IRQ on sound board */
	{ 0x060000, 0x067fff, MWA16_RAM, &ram },	/* WORK RAM */
MEMORY_END

WRITE_HANDLER( salamand_speech_start_w )
{
        VLM5030_ST ( 1 );
        VLM5030_ST ( 0 );
}

WRITE_HANDLER( gx400_speech_start_w )
{
        /* the voice data is not in a rom but in sound RAM at $8000 */
        VLM5030_set_rom ((memory_region(REGION_CPU2))+ 0x8000);
        VLM5030_ST (1);
        VLM5030_ST (0);
}

static READ_HANDLER( nemesis_portA_r )
{
/*
   bit 0-3:   timer
   bit 4 6:   unused (always high)
   bit 5:     vlm5030 busy
   bit 7:     unused by this software version. Bubble Memory version uses this bit.
*/

	int res = (activecpu_gettotalcycles() / 1024) & 0x2f; // this should be 0x0f, but it doesn't work

	res |= 0xd0;

	if (VLM5030_BSY())
		res |= 0x20;

	return res;
}

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0xe001, 0xe001, soundlatch_r },
	{ 0xe086, 0xe086, AY8910_read_port_0_r },
	{ 0xe205, 0xe205, AY8910_read_port_1_r },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0xa000, 0xafff, k005289_pitch_A_w },
	{ 0xc000, 0xcfff, k005289_pitch_B_w },
	{ 0xe003, 0xe003, k005289_keylatch_A_w },
	{ 0xe004, 0xe004, k005289_keylatch_B_w },
	{ 0xe005, 0xe005, AY8910_control_port_1_w },
	{ 0xe006, 0xe006, AY8910_control_port_0_w },
	{ 0xe106, 0xe106, AY8910_write_port_0_w },
	{ 0xe405, 0xe405, AY8910_write_port_1_w },
MEMORY_END

static MEMORY_READ16_START( konamigt_readmem )
	{ 0x000000, 0x03ffff, MRA16_ROM },
	{ 0x040000, 0x04ffff, nemesis_characterram_word_r },
	{ 0x050000, 0x0503ff, MRA16_RAM },
	{ 0x050400, 0x0507ff, MRA16_RAM },
	{ 0x050800, 0x050bff, MRA16_RAM },
	{ 0x050c00, 0x050fff, MRA16_RAM },

	{ 0x052000, 0x052fff, nemesis_videoram1b_word_r },
	{ 0x053000, 0x053fff, nemesis_videoram1f_word_r },
	{ 0x054000, 0x054fff, nemesis_videoram2b_word_r },
	{ 0x055000, 0x055fff, nemesis_videoram2f_word_r },
	{ 0x056000, 0x056fff, MRA16_RAM },
	{ 0x05a000, 0x05afff, MRA16_RAM },

	{ 0x05c400, 0x05c401, input_port_4_word_r },	/* DSW0 */
	{ 0x05c402, 0x05c403, input_port_5_word_r },	/* DSW1 */

	{ 0x05cc00, 0x05cc01, input_port_0_word_r },	/* IN0 */
	{ 0x05cc02, 0x05cc03, input_port_1_word_r },	/* IN1 */
	{ 0x05cc04, 0x05cc05, input_port_2_word_r },	/* IN2 */
	{ 0x05cc06, 0x05cc07, input_port_3_word_r },	/* TEST */

	{ 0x060000, 0x067fff, MRA16_RAM },
	{ 0x070000, 0x070001, konamigt_input_word_r },
MEMORY_END

static MEMORY_WRITE16_START( konamigt_writemem )
	{ 0x000000, 0x03ffff, MWA16_ROM },	/* ROM */

	{ 0x040000, 0x04ffff, nemesis_characterram_word_w, &nemesis_characterram, &nemesis_characterram_size },

	{ 0x050000, 0x0503ff, MWA16_RAM, &nemesis_xscroll1 },
	{ 0x050400, 0x0507ff, MWA16_RAM, &nemesis_xscroll2 },
	{ 0x050800, 0x050bff, MWA16_RAM },
	{ 0x050c00, 0x050fff, MWA16_RAM, &nemesis_yscroll },
	{ 0x051000, 0x051fff, MWA16_NOP },		/* used, but written to with 0's */

	{ 0x052000, 0x052fff, nemesis_videoram1b_word_w, &nemesis_videoram1b },	/* VRAM 1 */
	{ 0x053000, 0x053fff, nemesis_videoram1f_word_w, &nemesis_videoram1f },	/* VRAM 1 */
	{ 0x054000, 0x054fff, nemesis_videoram2b_word_w, &nemesis_videoram2b },	/* VRAM 2 */
	{ 0x055000, 0x055fff, nemesis_videoram2f_word_w, &nemesis_videoram2f },	/* VRAM 2 */
	{ 0x056000, 0x056fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x05a000, 0x05afff, nemesis_palette_word_w, &paletteram16 },

	{ 0x05c000, 0x05c001, nemesis_soundlatch_word_w },
	{ 0x05c800, 0x05c801, watchdog_reset16_w },	/* probably */

	{ 0x05e000, 0x05e001, &konamigt_irq2_enable_word_w },
	{ 0x05e002, 0x05e003, &konamigt_irq_enable_word_w },
	{ 0x05e004, 0x05e005, MWA16_NOP},	/* bit 8 of the word probably triggers IRQ on sound board */
	{ 0x060000, 0x067fff, MWA16_RAM, &ram },	/* WORK RAM */
MEMORY_END


static MEMORY_READ16_START( gx400_readmem )
	{ 0x000000, 0x00ffff, MRA16_ROM },
	{ 0x010000, 0x01ffff, MRA16_RAM },
	{ 0x020000, 0x0287ff, gx400_sharedram_word_r },
	{ 0x030000, 0x03ffff, nemesis_characterram_word_r },
	{ 0x050000, 0x0503ff, MRA16_RAM },
	{ 0x050400, 0x0507ff, MRA16_RAM },
	{ 0x050800, 0x050bff, MRA16_RAM },
	{ 0x050c00, 0x050fff, MRA16_RAM },
	{ 0x052000, 0x052fff, nemesis_videoram1b_word_r },
	{ 0x053000, 0x053fff, nemesis_videoram1f_word_r },
	{ 0x054000, 0x054fff, nemesis_videoram2b_word_r },
	{ 0x055000, 0x055fff, nemesis_videoram2f_word_r },
	{ 0x056000, 0x056fff, MRA16_RAM },
	{ 0x057000, 0x057fff, MRA16_RAM },
	{ 0x05a000, 0x05afff, MRA16_RAM },
	{ 0x05c402, 0x05c403, input_port_4_word_r },	/* DSW0 */
	{ 0x05c404, 0x05c405, input_port_5_word_r },	/* DSW1 */
	{ 0x05c406, 0x05c407, input_port_3_word_r },	/* TEST */
	{ 0x05cc00, 0x05cc01, input_port_0_word_r },	/* IN0 */
	{ 0x05cc02, 0x05cc03, input_port_1_word_r },	/* IN1 */
	{ 0x05cc04, 0x05cc05, input_port_2_word_r },	/* IN2 */
	{ 0x060000, 0x07ffff, MRA16_RAM },
	{ 0x080000, 0x0cffff, MRA16_ROM },
MEMORY_END

static MEMORY_WRITE16_START( gx400_writemem )
	{ 0x000000, 0x00ffff, MWA16_ROM },
	{ 0x010000, 0x01ffff, MWA16_RAM , &ram },
	{ 0x020000, 0x0287ff, gx400_sharedram_word_w },
	{ 0x030000, 0x03ffff, nemesis_characterram_word_w, &nemesis_characterram, &nemesis_characterram_size },
	{ 0x050000, 0x0503ff, MWA16_RAM, &nemesis_xscroll1 },
	{ 0x050400, 0x0507ff, MWA16_RAM, &nemesis_xscroll2 },
	{ 0x050800, 0x050bff, MWA16_RAM },
	{ 0x050c00, 0x050fff, MWA16_RAM, &nemesis_yscroll },
	{ 0x051000, 0x051fff, MWA16_NOP },		/* used, but written to with 0's */
	{ 0x052000, 0x052fff, nemesis_videoram1b_word_w, &nemesis_videoram1b },	/* VRAM 1 */
	{ 0x053000, 0x053fff, nemesis_videoram1f_word_w, &nemesis_videoram1f },	/* VRAM 1 */
	{ 0x054000, 0x054fff, nemesis_videoram2b_word_w, &nemesis_videoram2b },	/* VRAM 2 */
	{ 0x055000, 0x055fff, nemesis_videoram2f_word_w, &nemesis_videoram2f },	/* VRAM 2 */
	{ 0x056000, 0x056fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x057000, 0x057fff, MWA16_RAM},										/* needed for twinbee */
	{ 0x05a000, 0x05afff, nemesis_palette_word_w, &paletteram16 },
	{ 0x05c000, 0x05c001, nemesis_soundlatch_word_w },
	{ 0x05c800, 0x05c801, watchdog_reset16_w },	/* probably */
	{ 0x05e000, 0x05e001, &gx400_irq2_enable_word_w },	/* ?? */
	{ 0x05e002, 0x05e003, &gx400_irq1_enable_word_w },	/* ?? */
	{ 0x05e004, 0x05e005, MWA16_NOP},	/* bit 8 of the word probably triggers IRQ on sound board */
	{ 0x05e008, 0x05e009, MWA16_NOP },	/* IRQ acknowledge??? */
	{ 0x05e00e, 0x05e00f, &gx400_irq4_enable_word_w },	/* ?? */
	{ 0x060000, 0x07ffff, MWA16_RAM , &ram2},
	{ 0x080000, 0x0cffff, MWA16_ROM },
MEMORY_END

static MEMORY_READ16_START( rf2_gx400_readmem )
	{ 0x000000, 0x00ffff, MRA16_ROM },
	{ 0x010000, 0x01ffff, MRA16_RAM },
	{ 0x020000, 0x0287ff, gx400_sharedram_word_r },
	{ 0x030000, 0x03ffff, nemesis_characterram_word_r },
	{ 0x050000, 0x0503ff, MRA16_RAM },
	{ 0x050400, 0x0507ff, MRA16_RAM },
	{ 0x050800, 0x050bff, MRA16_RAM },
	{ 0x050c00, 0x050fff, MRA16_RAM },
	{ 0x052000, 0x052fff, nemesis_videoram1b_word_r },
	{ 0x053000, 0x053fff, nemesis_videoram1f_word_r },
	{ 0x054000, 0x054fff, nemesis_videoram2b_word_r },
	{ 0x055000, 0x055fff, nemesis_videoram2f_word_r },
	{ 0x056000, 0x056fff, MRA16_RAM },
	{ 0x05a000, 0x05afff, MRA16_RAM },
	{ 0x05c402, 0x05c403, input_port_4_word_r },	/* DSW0 */
	{ 0x05c404, 0x05c405, input_port_5_word_r },	/* DSW1 */
	{ 0x05c406, 0x05c407, input_port_3_word_r },	/* TEST */
	{ 0x05cc00, 0x05cc01, input_port_0_word_r },	/* IN0 */
	{ 0x05cc02, 0x05cc03, input_port_1_word_r },	/* IN1 */
	{ 0x05cc04, 0x05cc05, input_port_2_word_r },	/* IN2 */
	{ 0x060000, 0x067fff, MRA16_RAM },
	{ 0x070000, 0x070001, konamigt_input_word_r },
	{ 0x080000, 0x0cffff, MRA16_ROM },
MEMORY_END

static MEMORY_WRITE16_START( rf2_gx400_writemem )
	{ 0x000000, 0x00ffff, MWA16_ROM },
	{ 0x010000, 0x01ffff, MWA16_RAM , &ram2},
	{ 0x020000, 0x0287ff, gx400_sharedram_word_w },
	{ 0x030000, 0x03ffff, nemesis_characterram_word_w, &nemesis_characterram, &nemesis_characterram_size },
	{ 0x050000, 0x0503ff, MWA16_RAM, &nemesis_xscroll1 },
	{ 0x050400, 0x0507ff, MWA16_RAM, &nemesis_xscroll2 },
	{ 0x050800, 0x050bff, MWA16_RAM },
	{ 0x050c00, 0x050fff, MWA16_RAM, &nemesis_yscroll },
	{ 0x051000, 0x051fff, MWA16_NOP },		/* used, but written to with 0's */
	{ 0x052000, 0x052fff, nemesis_videoram1b_word_w, &nemesis_videoram1b },	/* VRAM 1 */
	{ 0x053000, 0x053fff, nemesis_videoram1f_word_w, &nemesis_videoram1f },	/* VRAM 1 */
	{ 0x054000, 0x054fff, nemesis_videoram2b_word_w, &nemesis_videoram2b },	/* VRAM 2 */
	{ 0x055000, 0x055fff, nemesis_videoram2f_word_w, &nemesis_videoram2f },	/* VRAM 2 */
	{ 0x056000, 0x056fff, MWA16_RAM, &spriteram16, &spriteram_size },
	{ 0x05a000, 0x05afff, nemesis_palette_word_w, &paletteram16 },
	{ 0x05c000, 0x05c001, nemesis_soundlatch_word_w },
	{ 0x05c800, 0x05c801, watchdog_reset16_w },	/* probably */
	{ 0x05e000, 0x05e001, &gx400_irq2_enable_word_w },	/* ?? */
	{ 0x05e002, 0x05e003, &gx400_irq1_enable_word_w },	/* ?? */
	{ 0x05e004, 0x05e005, MWA16_NOP}, /*	bit 8 of the word probably triggers IRQ on sound board */
	{ 0x05e008, 0x05e009, MWA16_NOP },	/* IRQ acknowledge??? */
	{ 0x05e00e, 0x05e00f, &gx400_irq4_enable_word_w },	/* ?? */
	{ 0x060000, 0x067fff, MWA16_RAM, &ram },	/* WORK RAM */
	{ 0x080000, 0x0cffff, MWA16_ROM },
MEMORY_END

static MEMORY_READ_START( gx400_sound_readmem )
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x87ff, MRA_RAM },
	{ 0xe001, 0xe001, soundlatch_r },
	{ 0xe086, 0xe086, AY8910_read_port_0_r },
	{ 0xe205, 0xe205, AY8910_read_port_1_r },
MEMORY_END

static MEMORY_WRITE_START( gx400_sound_writemem )
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x87ff, MWA_RAM, &gx400_shared_ram },
	{ 0xa000, 0xafff, k005289_pitch_A_w },
	{ 0xc000, 0xcfff, k005289_pitch_B_w },
	{ 0xe000, 0xe000, VLM5030_data_w },
	{ 0xe003, 0xe003, k005289_keylatch_A_w },
	{ 0xe004, 0xe004, k005289_keylatch_B_w },
	{ 0xe005, 0xe005, AY8910_control_port_1_w },
	{ 0xe006, 0xe006, AY8910_control_port_0_w },
	{ 0xe030, 0xe030, gx400_speech_start_w },
	{ 0xe106, 0xe106, AY8910_write_port_0_w },
	{ 0xe405, 0xe405, AY8910_write_port_1_w },
MEMORY_END

/******************************************************************************/

static MEMORY_READ16_START( salamand_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },  /* ROM BIOS */
	{ 0x080000, 0x087fff, MRA16_RAM },
	{ 0x090000, 0x091fff, MRA16_RAM },
	{ 0x0c0002, 0x0c0003, input_port_3_word_r },	/* DSW0 */
	{ 0x0c2000, 0x0c2001, input_port_0_word_r },	/* Coins, start buttons, test mode */
	{ 0x0c2002, 0x0c2003, input_port_1_word_r },	/* IN1 */
	{ 0x0c2004, 0x0c2005, input_port_2_word_r },	/* IN2 */
	{ 0x0c2006, 0x0c2007, input_port_4_word_r },	/* DSW1 */
	{ 0x100000, 0x100fff, nemesis_videoram1b_word_r },
	{ 0x101000, 0x101fff, nemesis_videoram1f_word_r },
	{ 0x102000, 0x102fff, nemesis_videoram2b_word_r },
	{ 0x103000, 0x103fff, nemesis_videoram2f_word_r },
	{ 0x120000, 0x12ffff, nemesis_characterram_word_r },
	{ 0x180000, 0x180fff, MRA16_RAM },
	{ 0x190000, 0x1903ff, gx400_xscroll1_word_r },
	{ 0x190400, 0x1907ff, gx400_xscroll2_word_r },
	{ 0x190800, 0x190eff, MRA16_RAM },
	{ 0x190f00, 0x190f7f, gx400_yscroll1_word_r },
	{ 0x190f80, 0x190fff, gx400_yscroll2_word_r },
	{ 0x191000, 0x191fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( salamand_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x080000, 0x087fff, MWA16_RAM, &ram },
	{ 0x090000, 0x091fff, salamander_palette_word_w, &paletteram16 },
	{ 0x0A0000, 0x0A0001, nemesis_irq_enable_word_w },          /* irq enable */
	{ 0x0C0000, 0x0C0001, salamand_soundlatch_word_w },
	{ 0x0C0004, 0x0C0005, MWA16_NOP },        /* Watchdog at $c0005 */
	{ 0x100000, 0x100fff, nemesis_videoram1b_word_w, &nemesis_videoram1b },	/* VRAM 1 */
	{ 0x101000, 0x101fff, nemesis_videoram1f_word_w, &nemesis_videoram1f },	/* VRAM 1 */
	{ 0x102000, 0x102fff, nemesis_videoram2b_word_w, &nemesis_videoram2b },	/* VRAM 2 */
	{ 0x103000, 0x103fff, nemesis_videoram2f_word_w, &nemesis_videoram2f },	/* VRAM 2 */
	{ 0x120000, 0x12ffff, nemesis_characterram_word_w, &nemesis_characterram, &nemesis_characterram_size },
	{ 0x180000, 0x180fff, MWA16_RAM, &spriteram16, &spriteram_size },		/* more sprite ram ??? */
	{ 0x190000, 0x1903ff, gx400_xscroll1_word_w, &nemesis_xscroll1 },
	{ 0x190400, 0x1907ff, gx400_xscroll2_word_w, &nemesis_xscroll2 },
	{ 0x190800, 0x190eff, MWA16_RAM },			/* not used */
	{ 0x190f00, 0x190f7f, gx400_yscroll1_word_w, &nemesis_yscroll1 },
	{ 0x190f80, 0x190fff, gx400_yscroll2_word_w, &nemesis_yscroll2 },
	{ 0x191000, 0x191fff, MWA16_RAM },			/* not used */
MEMORY_END

static MEMORY_READ16_START( blkpnthr_readmem )
	{ 0x000000, 0x07ffff, MRA16_ROM },  /* ROM BIOS */
	{ 0x080000, 0x081fff, MRA16_RAM },
	{ 0x090000, 0x097fff, MRA16_RAM },
	{ 0x0c0002, 0x0c0003, input_port_3_word_r },	/* DSW0 */
	{ 0x0c2000, 0x0c2001, input_port_0_word_r },	/* Coins, start buttons, test mode */
	{ 0x0c2002, 0x0c2003, input_port_1_word_r },	/* IN1 */
	{ 0x0c2004, 0x0c2005, input_port_2_word_r },	/* IN2 */
	{ 0x0c2006, 0x0c2007, input_port_4_word_r },	/* DSW1 */
	{ 0x100000, 0x100fff, nemesis_videoram2f_word_r },
	{ 0x101000, 0x101fff, nemesis_videoram2b_word_r },
	{ 0x102000, 0x102fff, nemesis_videoram1f_word_r },
	{ 0x103000, 0x103fff, nemesis_videoram1b_word_r },
	{ 0x120000, 0x12ffff, nemesis_characterram_word_r },
	{ 0x180000, 0x1803ff, gx400_xscroll2_word_r },
	{ 0x180400, 0x1807ff, gx400_xscroll1_word_r },
	{ 0x180800, 0x180eff, MRA16_RAM },
	{ 0x180f00, 0x180f7f, gx400_yscroll2_word_r },
	{ 0x180f80, 0x180fff, gx400_yscroll1_word_r },
	{ 0x181000, 0x181fff, MRA16_RAM },
	{ 0x190000, 0x190fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( blkpnthr_writemem )
	{ 0x000000, 0x07ffff, MWA16_ROM },
	{ 0x080000, 0x081fff, salamander_palette_word_w, &paletteram16 },
	{ 0x090000, 0x097fff, MWA16_RAM, &ram },
	{ 0x0A0000, 0x0A0001, nemesis_irq_enable_word_w },          /* irq enable */
	{ 0x0C0000, 0x0C0001, salamand_soundlatch_word_w },
	{ 0x0C0004, 0x0C0005, MWA16_NOP },        /* Watchdog at $c0005 */
	{ 0x100000, 0x100fff, nemesis_videoram2f_word_w, &nemesis_videoram2f },	/* VRAM 2 */
	{ 0x101000, 0x101fff, nemesis_videoram2b_word_w, &nemesis_videoram2b },	/* VRAM 2 */
	{ 0x102000, 0x102fff, nemesis_videoram1f_word_w, &nemesis_videoram1f },	/* VRAM 1 */
	{ 0x103000, 0x103fff, nemesis_videoram1b_word_w, &nemesis_videoram1b },	/* VRAM 1 */
	{ 0x120000, 0x12ffff, nemesis_characterram_word_w, &nemesis_characterram, &nemesis_characterram_size },
	{ 0x180000, 0x1803ff, gx400_xscroll2_word_w, &nemesis_xscroll2 },
	{ 0x180400, 0x1807ff, gx400_xscroll1_word_w, &nemesis_xscroll1 },
	{ 0x180800, 0x180eff, MWA16_RAM },			/* not used */
	{ 0x180f00, 0x180f7f, gx400_yscroll2_word_w, &nemesis_yscroll2 },
	{ 0x180f80, 0x180fff, gx400_yscroll1_word_w, &nemesis_yscroll1 },
	{ 0x181000, 0x181fff, MWA16_RAM },			/* not used */
	{ 0x190000, 0x190fff, MWA16_RAM, &spriteram16, &spriteram_size },		/* more sprite ram ??? */
MEMORY_END

static MEMORY_READ16_START( citybomb_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },  /* ROM BIOS */
	{ 0x080000, 0x087fff, MRA16_RAM },
	{ 0x0e0000, 0x0e1fff, MRA16_RAM },
	{ 0x0f0000, 0x0f0001, input_port_4_word_r },	/* DSW1 */
	{ 0x0f0002, 0x0f0003, input_port_2_word_r },	/* IN2 */
	{ 0x0f0004, 0x0f0005, input_port_1_word_r },	/* IN1 */
	{ 0x0f0006, 0x0f0007, input_port_0_word_r },	/* Coins, start buttons, test mode */
	{ 0x0f0008, 0x0f0009, input_port_3_word_r },	/* DSW0 */
	{ 0x0f0020, 0x0f0021, MRA16_NOP },				/* Analog device */
	{ 0x100000, 0x1bffff, MRA16_ROM },  /* ROM BIOS */
	{ 0x200000, 0x20ffff, nemesis_characterram_word_r },
	{ 0x210000, 0x210fff, nemesis_videoram1f_word_r },
	{ 0x211000, 0x211fff, nemesis_videoram1b_word_r },
	{ 0x212000, 0x212fff, nemesis_videoram2f_word_r },
	{ 0x213000, 0x213fff, nemesis_videoram2b_word_r },
	{ 0x300000, 0x3003ff, gx400_xscroll2_word_r },
	{ 0x300400, 0x3007ff, gx400_xscroll1_word_r },
	{ 0x300800, 0x300eff, MRA16_RAM },
	{ 0x300f00, 0x300f7f, gx400_yscroll2_word_r },
	{ 0x300f80, 0x300fff, gx400_yscroll1_word_r },
	{ 0x301000, 0x301fff, MRA16_RAM },
	{ 0x310000, 0x310fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( citybomb_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x080000, 0x087fff, MWA16_RAM, &ram },
	{ 0x0e0000, 0x0e1fff, salamander_palette_word_w, &paletteram16 },
	{ 0x0f0010, 0x0f0011, salamand_soundlatch_word_w },
	{ 0x0f0018, 0x0f0019, MWA16_NOP },			/* Watchdog */
	{ 0x0f0020, 0x0f0021, MWA16_NOP },			/* Analog device */
	{ 0x0f8000, 0x0f8001, nemesis_irq_enable_word_w },          /* irq enable */
	{ 0x100000, 0x1bffff, MWA16_ROM },
	{ 0x200000, 0x20ffff, nemesis_characterram_word_w, &nemesis_characterram, &nemesis_characterram_size },
	{ 0x210000, 0x210fff, nemesis_videoram1f_word_w, &nemesis_videoram1f },	/* VRAM 1 */
	{ 0x211000, 0x211fff, nemesis_videoram1b_word_w, &nemesis_videoram1b },	/* VRAM 1 */
	{ 0x212000, 0x212fff, nemesis_videoram2f_word_w, &nemesis_videoram2f },	/* VRAM 2 */
	{ 0x213000, 0x213fff, nemesis_videoram2b_word_w, &nemesis_videoram2b },	/* VRAM 2 */
	{ 0x300000, 0x3003ff, gx400_xscroll2_word_w, &nemesis_xscroll2 },
	{ 0x300400, 0x3007ff, gx400_xscroll1_word_w, &nemesis_xscroll1 },
	{ 0x300800, 0x300eff, MWA16_RAM },			/* not used */
	{ 0x300f00, 0x300f7f, gx400_yscroll2_word_w, &nemesis_yscroll2 },
	{ 0x300f80, 0x300fff, gx400_yscroll1_word_w, &nemesis_yscroll1 },
	{ 0x301000, 0x301fff, MWA16_RAM },			/* not used */
	{ 0x310000, 0x310fff, MWA16_RAM, &spriteram16, &spriteram_size },		/* more sprite ram ??? */
MEMORY_END

static MEMORY_READ16_START( nyanpani_readmem )
	{ 0x000000, 0x01ffff, MRA16_ROM },  /* ROM BIOS */
	{ 0x040000, 0x047fff, MRA16_RAM },
	{ 0x060000, 0x061fff, MRA16_RAM },
	{ 0x100000, 0x13ffff, MRA16_ROM },  /* ROM BIOS */
	{ 0x070000, 0x070001, input_port_4_word_r },	/* DSW1 */
	{ 0x070002, 0x070003, input_port_2_word_r },	/* IN2 */
	{ 0x070004, 0x070005, input_port_1_word_r },	/* IN1 */
	{ 0x070006, 0x070007, input_port_0_word_r },	/* Coins, start buttons, test mode */
	{ 0x070008, 0x070009, input_port_3_word_r },	/* DSW0 */
	{ 0x200000, 0x200fff, nemesis_videoram1f_word_r },
	{ 0x201000, 0x201fff, nemesis_videoram1b_word_r },
	{ 0x202000, 0x202fff, nemesis_videoram2f_word_r },
	{ 0x203000, 0x203fff, nemesis_videoram2b_word_r },
	{ 0x210000, 0x21ffff, nemesis_characterram_word_r },
	{ 0x300000, 0x300fff, MRA16_RAM },
	{ 0x310000, 0x3103ff, gx400_xscroll2_word_r },
	{ 0x310400, 0x3107ff, gx400_xscroll1_word_r },
	{ 0x310800, 0x310eff, MRA16_RAM },
	{ 0x310f00, 0x310f7f, gx400_yscroll2_word_r },
	{ 0x310f80, 0x310fff, gx400_yscroll1_word_r },
	{ 0x311000, 0x311fff, MRA16_RAM },
MEMORY_END

static MEMORY_WRITE16_START( nyanpani_writemem )
	{ 0x000000, 0x01ffff, MWA16_ROM },
	{ 0x040000, 0x047fff, MWA16_RAM, &ram },
	{ 0x060000, 0x061fff, salamander_palette_word_w, &paletteram16 },
	{ 0x100000, 0x13ffff, MWA16_ROM },
	{ 0x070010, 0x070011, salamand_soundlatch_word_w },
	{ 0x070018, 0x070019, MWA16_NOP },        /* Watchdog */
	{ 0x078000, 0x078001, nemesis_irq_enable_word_w },          /* irq enable */
	{ 0x200000, 0x200fff, nemesis_videoram1f_word_w, &nemesis_videoram1f },	/* VRAM 1 */
	{ 0x201000, 0x201fff, nemesis_videoram1b_word_w, &nemesis_videoram1b },	/* VRAM 1 */
	{ 0x202000, 0x202fff, nemesis_videoram2f_word_w, &nemesis_videoram2f },	/* VRAM 2 */
	{ 0x203000, 0x203fff, nemesis_videoram2b_word_w, &nemesis_videoram2b },	/* VRAM 2 */
	{ 0x210000, 0x21ffff, nemesis_characterram_word_w, &nemesis_characterram, &nemesis_characterram_size },
	{ 0x300000, 0x300fff, MWA16_RAM, &spriteram16, &spriteram_size },		/* more sprite ram ??? */
	{ 0x310000, 0x3103ff, gx400_xscroll2_word_w, &nemesis_xscroll2 },
	{ 0x310400, 0x3107ff, gx400_xscroll1_word_w, &nemesis_xscroll1 },
	{ 0x310800, 0x310eff, MWA16_RAM },			/* not used */
	{ 0x310f00, 0x310f7f, gx400_yscroll2_word_w, &nemesis_yscroll2 },
	{ 0x310f80, 0x310fff, gx400_yscroll1_word_w, &nemesis_yscroll1 },
	{ 0x311000, 0x311fff, MWA16_RAM },			/* not used */
MEMORY_END

static READ_HANDLER( wd_r )
{
	static int a=1;
	a^= 1;
	return a;
}

static WRITE_HANDLER( city_sound_bank_w )
{
	int bank_A=(data&0x3);
	int bank_B=((data>>2)&0x3);
	K007232_set_bank( 0, bank_A, bank_B );
}

static MEMORY_READ_START( sal_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa000, soundlatch_r },
	{ 0xb000, 0xb00d, K007232_read_port_0_r },
	{ 0xc001, 0xc001, YM2151_status_port_0_r },
	{ 0xe000, 0xe000, wd_r }, /* watchdog?? */
MEMORY_END

static MEMORY_WRITE_START( sal_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xb000, 0xb00d, K007232_write_port_0_w },
	{ 0xc000, 0xc000, YM2151_register_port_0_w },
	{ 0xc001, 0xc001, YM2151_data_port_0_w },
	{ 0xd000, 0xd000, VLM5030_data_w },
	{ 0xf000, 0xf000, salamand_speech_start_w },
MEMORY_END

static MEMORY_READ_START( city_sound_readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa000, YM3812_status_port_0_r },
	{ 0xb000, 0xb00d, K007232_read_port_0_r },
	{ 0xd000, 0xd000, soundlatch_r },
MEMORY_END

static MEMORY_WRITE_START( city_sound_writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9800, 0x987f, K051649_waveform_w },
	{ 0x9880, 0x9889, K051649_frequency_w },
	{ 0x988a, 0x988e, K051649_volume_w },
	{ 0x988f, 0x988f, K051649_keyonoff_w },
	{ 0xa000, 0xa000, YM3812_control_port_0_w },
	{ 0xa001, 0xa001, YM3812_write_port_0_w },
	{ 0xb000, 0xb00d, K007232_write_port_0_w },
	{ 0xc000, 0xc000, city_sound_bank_w }, /* 7232 bankswitch */
MEMORY_END

/******************************************************************************/

#define GX400_COINAGE_DIP \
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coin_A ) ) \
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_2C ) ) \
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) ) \
	PORT_DIPSETTING(    0x0f, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x03, DEF_STR( 3C_4C ) ) \
	PORT_DIPSETTING(    0x07, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0x0e, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_5C ) ) \
	PORT_DIPSETTING(    0x0d, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0x0b, DEF_STR( 1C_5C ) ) \
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_6C ) ) \
	PORT_DIPSETTING(    0x09, DEF_STR( 1C_7C ) ) \
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) ) \
	PORT_DIPNAME( 0xf0, 0xf0, DEF_STR( Coin_B ) ) \
	PORT_DIPSETTING(    0x20, DEF_STR( 4C_1C ) ) \
	PORT_DIPSETTING(    0x50, DEF_STR( 3C_1C ) ) \
	PORT_DIPSETTING(    0x80, DEF_STR( 2C_1C ) ) \
	PORT_DIPSETTING(    0x40, DEF_STR( 3C_2C ) ) \
	PORT_DIPSETTING(    0x10, DEF_STR( 4C_3C ) ) \
	PORT_DIPSETTING(    0xf0, DEF_STR( 1C_1C ) ) \
	PORT_DIPSETTING(    0x30, DEF_STR( 3C_4C ) ) \
	PORT_DIPSETTING(    0x70, DEF_STR( 2C_3C ) ) \
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_2C ) ) \
	PORT_DIPSETTING(    0x60, DEF_STR( 2C_5C ) ) \
	PORT_DIPSETTING(    0xd0, DEF_STR( 1C_3C ) ) \
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_4C ) ) \
	PORT_DIPSETTING(    0xb0, DEF_STR( 1C_5C ) ) \
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_6C ) ) \
	PORT_DIPSETTING(    0x90, DEF_STR( 1C_7C ) ) \
	PORT_DIPSETTING(    0x00, "Disabled" )


INPUT_PORTS_START( nemesis )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* TEST */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Version" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x00, "Vs" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	GX400_COINAGE_DIP

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "50k and every 100k" )
	PORT_DIPSETTING(    0x10, "30k" )
	PORT_DIPSETTING(    0x08, "50k" )
	PORT_DIPSETTING(    0x00, "100k" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( nemesuk )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* TEST */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Version" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x00, "Vs" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	GX400_COINAGE_DIP

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "20k and every 70k" )
	PORT_DIPSETTING(    0x10, "30k and every 80k" )
	PORT_DIPSETTING(    0x08, "20k" )
	PORT_DIPSETTING(    0x00, "30k" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


/* This needs to be sorted */
INPUT_PORTS_START( konamigt )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON3 ) /* gear */
	PORT_BIT( 0xef, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* TEST */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	GX400_COINAGE_DIP

	PORT_START	/* DSW1 */
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
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x30, "Easy" )
	PORT_DIPSETTING(    0x20, "Medium" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN6 */
	PORT_ANALOG( 0xff, 0x40, IPT_DIAL, 25, 10, 0x00, 0x7f )

	PORT_START	/* IN7 */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
//	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON4 )
INPUT_PORTS_END


/* This needs to be sorted */
INPUT_PORTS_START( rf2 )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* don't change */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x70, IP_ACTIVE_LOW, IPT_BUTTON3 ) /* gear (0-7) */
	PORT_BIT( 0x8f, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* TEST */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	GX400_COINAGE_DIP

	PORT_START	/* DSW1 */
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
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x30, "Easy" )
	PORT_DIPSETTING(    0x20, "Medium" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* IN6 */
	PORT_ANALOG( 0xff, 0x40, IPT_DIAL, 25, 10, 0x00, 0x7f )

	PORT_START	/* IN7 */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
//	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON4 )

INPUT_PORTS_END


INPUT_PORTS_START( gwarrior )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* TEST */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Version" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x00, "Vs" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	GX400_COINAGE_DIP

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "30k 100k 200k 400k" )
	PORT_DIPSETTING(    0x10, "40k 120k 240k 480k" )
	PORT_DIPSETTING(    0x08, "50k 150k 300k 600k" )
	PORT_DIPSETTING(    0x00, "100k 200k 400k 800k" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( twinbee )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )

	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* TEST */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Version" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x00, "Vs" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	GX400_COINAGE_DIP

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x01, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "20k 100k" )
	PORT_DIPSETTING(    0x10, "30k 120k" )
	PORT_DIPSETTING(    0x08, "40k 140k" )
	PORT_DIPSETTING(    0x00, "50k 160k" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( gradius )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* TEST */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Version" )
	PORT_DIPSETTING(    0x02, "Normal" )
	PORT_DIPSETTING(    0x00, "Vs" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	GX400_COINAGE_DIP

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "20k and every 70k" )
	PORT_DIPSETTING(    0x10, "30k and every 80k" )
	PORT_DIPSETTING(    0x08, "20k only" )
	PORT_DIPSETTING(    0x00, "30k only" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( salamand )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )


	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x0f, 0x0f, DEF_STR( Coinage ) )
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
	PORT_DIPSETTING(    0x00, "Disabled" )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x04, "Coin Slot(s)" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x18, 0x00, "Max Credit(s)" )
	PORT_DIPSETTING(    0x18, "1" )
	PORT_DIPSETTING(    0x10, "3" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "9" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END


INPUT_PORTS_START( lifefrcj )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )


	PORT_START	/* DSW0 */
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
	PORT_DIPSETTING(    0x00, "Disabled" )
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
	PORT_DIPSETTING(    0x00, "Disabled" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x04, 0x04, "Coin Slot(s)" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPNAME( 0x18, 0x18, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "70k and every 200k" )
	PORT_DIPSETTING(    0x10, "100k and every 300k" )
	PORT_DIPSETTING(    0x08, "70k only" )
	PORT_DIPSETTING(    0x00, "100k only" )
	PORT_DIPNAME( 0x60, 0x60, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Medium" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( blkpnthr )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0xc0, 0x80, "Continue" )
	PORT_DIPSETTING(    0xc0, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, "2 Areas" )
	PORT_DIPSETTING(    0x40, "3 Areas" )
	PORT_DIPSETTING(    0x00, "4 Areas" )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )


	PORT_START	/* DSW0 */
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
	PORT_DIPSETTING(    0x00, "Disabled" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x10, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x18, "50k 100k" )
	PORT_DIPSETTING(    0x10, "20k 50k" )
	PORT_DIPSETTING(    0x08, "30k 70k" )
	PORT_DIPSETTING(    0x00, "80k 150k" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( citybomb )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Upright Control" )
	PORT_DIPSETTING(    0x40, "Single" )
	PORT_DIPSETTING(    0x00, "Dual" )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER1 )
	PORT_DIPNAME( 0x80, 0x80, "Device Type" )
	PORT_DIPSETTING(    0x00, "Handle" )
	PORT_DIPSETTING(    0x80, "Joystick" )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
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
	PORT_DIPSETTING(    0x00, "Disabled" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x10, "Qualify" )
	PORT_DIPSETTING(    0x18, "Long" )
	PORT_DIPSETTING(    0x10, "Normal" )
	PORT_DIPSETTING(    0x08, "Short" )
	PORT_DIPSETTING(    0x00, "Very Short" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

INPUT_PORTS_START( nyanpani )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_SERVICE( 0x80, IP_ACTIVE_LOW )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
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
	PORT_DIPSETTING(    0x00, "Disabled" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x03, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "5" )
	PORT_DIPSETTING(    0x00, "7" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x60, "Easy" )
	PORT_DIPSETTING(    0x40, "Normal" )
	PORT_DIPSETTING(    0x20, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
INPUT_PORTS_END

/******************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	2048+1,	/* 2048 characters (+ blank one) */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the two bitplanes are merged in the same nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8     /* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	512,	/* 512 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the two bitplanes are merged in the same nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8     /* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout spritelayout3216 =
{
	32,16,	/* 32*16 sprites */
	256,	/* 256 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the two bitplanes are merged in the same nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4,
		   16*4,17*4, 18*4, 19*4, 20*4, 21*4, 22*4, 23*4,
		   24*4,25*4, 26*4, 27*4, 28*4, 29*4, 30*4, 31*4},
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128,
			8*128, 9*128, 10*128, 11*128, 12*128, 13*128, 14*128, 15*128 },
	256*8     /* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout spritelayout1632 =
{
	16,32,	/* 16*32 sprites */
	256,	/* 256 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the two bitplanes are merged in the same nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4},
	{ 0*64,  1*64,  2*64,  3*64,  4*64,  5*64,  6*64,  7*64,
	  8*64,  9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64,
	 16*64, 17*64, 18*64, 19*64, 20*64, 21*64, 22*64, 23*64,
	 24*64, 25*64, 26*64, 27*64, 28*64, 29*64, 30*64, 31*64},
	256*8     /* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout spritelayout3232 =
{
	32,32,	/* 32*32 sprites */
	128,	/* 128 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the two bitplanes are merged in the same nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4,
		   16*4,17*4, 18*4, 19*4, 20*4, 21*4, 22*4, 23*4,
		   24*4,25*4, 26*4, 27*4, 28*4, 29*4, 30*4, 31*4},
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128,
			8*128,  9*128, 10*128, 11*128, 12*128, 13*128, 14*128, 15*128,
		   16*128, 17*128, 18*128, 19*128, 20*128, 21*128, 22*128, 23*128,
		   24*128, 25*128, 26*128, 27*128, 28*128, 29*128, 30*128, 31*128},
	512*8     /* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout spritelayout816 =
{
	8,16,	/* 16*16 sprites */
	1024,	/* 1024 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the two bitplanes are merged in the same nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
			8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	64*8     /* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout spritelayout168 =
{
	16,8,	/* 16*8 sprites */
	1024,	/* 1024 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the two bitplanes are merged in the same nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4},
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64},
	64*8     /* every sprite takes 128 consecutive bytes */

};

static struct GfxLayout spritelayout6464 =
{
	64,64,	/* 32*32 sprites */
	32,	/* 128 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the two bitplanes are merged in the same nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4,
			8*4, 9*4, 10*4, 11*4, 12*4, 13*4, 14*4, 15*4,
		   16*4,17*4, 18*4, 19*4, 20*4, 21*4, 22*4, 23*4,
		   24*4,25*4, 26*4, 27*4, 28*4, 29*4, 30*4, 31*4,
		   32*4,33*4, 34*4, 35*4, 36*4, 37*4, 38*4, 39*4,
		   40*4,41*4, 42*4, 43*4, 44*4, 45*4, 46*4, 47*4,
		   48*4,49*4, 50*4, 51*4, 52*4, 53*4, 54*4, 55*4,
		   56*4,57*4, 58*4, 59*4, 60*4, 61*4, 62*4, 63*4},

	{ 0*256, 1*256, 2*256, 3*256, 4*256, 5*256, 6*256, 7*256,
			8*256,  9*256, 10*256, 11*256, 12*256, 13*256, 14*256, 15*256,
		   16*256, 17*256, 18*256, 19*256, 20*256, 21*256, 22*256, 23*256,
		   24*256, 25*256, 26*256, 27*256, 28*256, 29*256, 30*256, 31*256,
		   32*256, 33*256, 34*256, 35*256, 36*256, 37*256, 38*256, 39*256,
		   40*256, 41*256, 42*256, 43*256, 44*256, 45*256, 46*256, 47*256,
		   48*256, 49*256, 50*256, 51*256, 52*256, 53*256, 54*256, 55*256,
		   56*256, 57*256, 58*256, 59*256, 60*256, 61*256, 62*256, 63*256},
	2048*8     /* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
    { 0, 0x0, &charlayout,   0, 0x80 },	/* the game dynamically modifies this */
    { 0, 0x0, &spritelayout, 0, 0x80 },	/* the game dynamically modifies this */
    { 0, 0x0, &spritelayout3216, 0, 0x80 },	/* the game dynamically modifies this */
    { 0, 0x0, &spritelayout816, 0, 0x80 },	/* the game dynamically modifies this */
    { 0, 0x0, &spritelayout3232, 0, 0x80 },	/* the game dynamically modifies this */
    { 0, 0x0, &spritelayout1632, 0, 0x80 },	/* the game dynamically modifies this */
    { 0, 0x0, &spritelayout168, 0, 0x80 },	/* the game dynamically modifies this */
    { 0, 0x0, &spritelayout6464, 0, 0x80 },	/* the game dynamically modifies this */
	{ -1 }
};

/******************************************************************************/

static struct AY8910interface ay8910_interface =
{
	2,      		/* 2 chips */
	14318180/8,     /* 1.78975 MHz */
	{ 35, 30 },
	{ nemesis_portA_r, 0 },
	{ 0, 0 },
	{ 0, k005289_control_A_w },
	{ 0, k005289_control_B_w }
};

static struct k005289_interface k005289_interface =
{
	3579545/2,		/* clock speed */
	20,				/* playback volume */
	REGION_SOUND1	/* prom memory region */
};

static void sound_irq(int state)
{
/* Interrupts _are_ generated, I wonder where they go.. */
/*cpu_set_irq_line(1,0,HOLD_LINE);*/
}

static struct YM2151interface ym2151_interface =
{
	1,
	3579545,
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ sound_irq }
};

static struct YM3812interface ym3812_interface =
{
	1,
	3579545,
	{ 35 },
	{ sound_irq },
};

static struct k051649_interface k051649_interface =
{
	3579545/2,	/* Clock */
	45,			/* Volume */
};

static struct VLM5030interface vlm5030_interface =
{
    3579545,       /* master clock  */
    60,            /* volume        */
    REGION_SOUND1, /* memory region  */
    0              /* memory length */
};

static struct VLM5030interface gx400_vlm5030_interface =
{
    3579545,       /* master clock  */
    40,            /* volume        */
    0,             /* memory region (RAM based) */
    0x0800         /* memory length (not sure if correct) */
};

static void volume_callback(int v)
{
	K007232_set_volume(0,0,(v >> 4) * 0x11,0);
	K007232_set_volume(0,1,0,(v & 0x0f) * 0x11);
}

static struct K007232_interface k007232_interface =
{
	1,		/* number of chips */
	3579545,	/* clock */
	{ REGION_SOUND2 },	/* memory regions */
	{ K007232_VOL(10,MIXER_PAN_CENTER,10,MIXER_PAN_CENTER) },	/* volume */
	{ volume_callback }	/* external port callback */
};

/******************************************************************************/

static MACHINE_DRIVER_START( nemesis )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,18432000/2)         /* 9.216 MHz? */
//			14318180/2,	/* From schematics, should be accurate */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_VBLANK_INT(nemesis_interrupt,1)

	MDRV_CPU_ADD(Z80,14318180/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU) /* From schematics, should be accurate */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(nemesis)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(nemesis)
	MDRV_VIDEO_UPDATE(nemesis)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(K005289, k005289_interface)
	MDRV_SOUND_ADD(VLM5030, gx400_vlm5030_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( konamigt )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,18432000/2)         /* 9.216 MHz? */
	MDRV_CPU_MEMORY(konamigt_readmem,konamigt_writemem)
	MDRV_CPU_VBLANK_INT(konamigt_interrupt,2)

	MDRV_CPU_ADD(Z80,14318180/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)        /* 3.579545 MHz */
	MDRV_CPU_MEMORY(sound_readmem,sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(nemesis)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(nemesis)
	MDRV_VIDEO_UPDATE(nemesis)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(K005289, k005289_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( salamand )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,18432000/2)       /* 9.216MHz */
	MDRV_CPU_MEMORY(salamand_readmem,salamand_writemem)
	MDRV_CPU_VBLANK_INT(salamand_interrupt,1)

	MDRV_CPU_ADD(Z80,14318180/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)        /* 3.579545 MHz */
	MDRV_CPU_MEMORY(sal_sound_readmem,sal_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(nemesis)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(nemesis)
	MDRV_VIDEO_UPDATE(salamand)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(VLM5030, vlm5030_interface)
	MDRV_SOUND_ADD(K007232, k007232_interface)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( blkpnthr )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,18432000/2)         /* 9.216 MHz? */
	MDRV_CPU_MEMORY(blkpnthr_readmem,blkpnthr_writemem)
	MDRV_CPU_VBLANK_INT(blkpnthr_interrupt,1)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)        /* 3.579545 MHz */
	MDRV_CPU_MEMORY(sal_sound_readmem,sal_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(nemesis)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(nemesis)
	MDRV_VIDEO_UPDATE(salamand)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(K007232, k007232_interface)
	MDRV_SOUND_ADD(YM2151, ym2151_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( citybomb )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,18432000/2)         /* 9.216 MHz? */
	MDRV_CPU_MEMORY(citybomb_readmem,citybomb_writemem)
	MDRV_CPU_VBLANK_INT(salamand_interrupt,1)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)        /* 3.579545 MHz */
	MDRV_CPU_MEMORY(city_sound_readmem,city_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(nemesis)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(nemesis)
	MDRV_VIDEO_UPDATE(salamand)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(K007232, k007232_interface)
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
	MDRV_SOUND_ADD(K051649, k051649_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( nyanpani )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,18432000/2)         /* 9.216 MHz? */
	MDRV_CPU_MEMORY(nyanpani_readmem,nyanpani_writemem)
	MDRV_CPU_VBLANK_INT(salamand_interrupt,1)

	MDRV_CPU_ADD(Z80, 3579545)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)        /* 3.579545 MHz */
	MDRV_CPU_MEMORY(city_sound_readmem,city_sound_writemem)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(nemesis)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(nemesis)
	MDRV_VIDEO_UPDATE(salamand)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(K007232, k007232_interface)
	MDRV_SOUND_ADD(YM3812, ym3812_interface)
	MDRV_SOUND_ADD(K051649, k051649_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( gx400 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,18432000/2)     /* 9.216MHz */
	MDRV_CPU_MEMORY(gx400_readmem,gx400_writemem)
	MDRV_CPU_VBLANK_INT(gx400_interrupt,3)

	MDRV_CPU_ADD(Z80,14318180/4)        /* 3.579545 MHz */
	MDRV_CPU_MEMORY(gx400_sound_readmem,gx400_sound_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)	/* interrupts are triggered by the main CPU */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(nemesis)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(nemesis)
	MDRV_VIDEO_UPDATE(nemesis)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(K005289, k005289_interface)
	MDRV_SOUND_ADD(VLM5030, gx400_vlm5030_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( twinbee_gx400 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,18432000/2)     /* 9.216MHz */
	MDRV_CPU_MEMORY(gx400_readmem,gx400_writemem)
	MDRV_CPU_VBLANK_INT(gx400_interrupt,3)

	MDRV_CPU_ADD(Z80,14318180/4)        /* 3.579545 MHz */
	MDRV_CPU_MEMORY(gx400_sound_readmem,gx400_sound_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)	/* interrupts are triggered by the main CPU */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(nemesis)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(nemesis)
	MDRV_VIDEO_UPDATE(twinbee)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(K005289, k005289_interface)
	MDRV_SOUND_ADD(VLM5030, gx400_vlm5030_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( rf2_gx400 )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,18432000/2)     /* 9.216MHz */
	MDRV_CPU_MEMORY(rf2_gx400_readmem,rf2_gx400_writemem)
	MDRV_CPU_VBLANK_INT(gx400_interrupt,3)

	MDRV_CPU_ADD(Z80,14318180/4)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)        /* 3.579545 MHz */
	MDRV_CPU_MEMORY(gx400_sound_readmem,gx400_sound_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)	/* interrupts are triggered by the main CPU */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_INIT(nemesis)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048)

	MDRV_VIDEO_START(nemesis)
	MDRV_VIDEO_UPDATE(nemesis)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
	MDRV_SOUND_ADD(K005289, k005289_interface)
	MDRV_SOUND_ADD(VLM5030, gx400_vlm5030_interface)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/* ROM names should be 456D01~456D08 */
ROM_START( nemesis )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )    /* 4 * 64k for code and rom */
	ROM_LOAD16_BYTE( "12a_01.bin",   0x00000, 0x8000, 0x35ff1aaa )
	ROM_LOAD16_BYTE( "12c_05.bin",   0x00001, 0x8000, 0x23155faa )
	ROM_LOAD16_BYTE( "13a_02.bin",   0x10000, 0x8000, 0xac0cf163 )
	ROM_LOAD16_BYTE( "13c_06.bin",   0x10001, 0x8000, 0x023f22a9 )
	ROM_LOAD16_BYTE( "14a_03.bin",   0x20000, 0x8000, 0x8cefb25f )
	ROM_LOAD16_BYTE( "14c_07.bin",   0x20001, 0x8000, 0xd50b82cb )
	ROM_LOAD16_BYTE( "15a_04.bin",   0x30000, 0x8000, 0x9ca75592 )
	ROM_LOAD16_BYTE( "15c_08.bin",   0x30001, 0x8000, 0x03c0b7f5 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for sound */
	ROM_LOAD(      "09c_snd.bin",  0x00000, 0x4000, 0x26bf9636 )

	ROM_REGION( 0x0200,  REGION_SOUND1, 0 )      /* 2x 256 byte for 0005289 wavetable data */
	ROM_LOAD(      "400-a01.fse",  0x00000, 0x0100, 0x5827b1e8 )
	ROM_LOAD(      "400-a02.fse",  0x00100, 0x0100, 0x2f44f970 )
ROM_END

/* ROM names should be 456E01~456E08 */
ROM_START( nemesuk )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )    /* 4 * 64k for code and rom */
	ROM_LOAD16_BYTE( "12a_01.uk",    0x00000, 0x8000, 0xe1993f91 )
	ROM_LOAD16_BYTE( "12c_05.uk",    0x00001, 0x8000, 0xc9761c78 )
	ROM_LOAD16_BYTE( "13a_02.uk",    0x10000, 0x8000, 0xf6169c4b )
	ROM_LOAD16_BYTE( "13c_06.uk",    0x10001, 0x8000, 0xaf58c548 )
	ROM_LOAD16_BYTE( "14a_03.bin",   0x20000, 0x8000, 0x8cefb25f )
	ROM_LOAD16_BYTE( "14c_07.bin",   0x20001, 0x8000, 0xd50b82cb )
	ROM_LOAD16_BYTE( "15a_04.uk",    0x30000, 0x8000, 0x322423d0 )
	ROM_LOAD16_BYTE( "15c_08.uk",    0x30001, 0x8000, 0xeb656266 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for sound */
	ROM_LOAD(      "09c_snd.bin",  0x00000, 0x4000, 0x26bf9636 )

	ROM_REGION( 0x0200,  REGION_SOUND1, 0 )      /* 2x 256 byte for 0005289 wavetable data */
	ROM_LOAD(      "400-a01.fse",  0x00000, 0x0100, 0x5827b1e8 )
	ROM_LOAD(      "400-a02.fse",  0x00100, 0x0100, 0x2f44f970 )
ROM_END

ROM_START( konamigt )
	ROM_REGION( 0x40000, REGION_CPU1, 0 )    /* 4 * 64k for code and rom */
	ROM_LOAD16_BYTE( "c01.rom",      0x00000, 0x8000, 0x56245bfd )
	ROM_LOAD16_BYTE( "c05.rom",      0x00001, 0x8000, 0x8d651f44 )
	ROM_LOAD16_BYTE( "c02.rom",      0x10000, 0x8000, 0x3407b7cb )
	ROM_LOAD16_BYTE( "c06.rom",      0x10001, 0x8000, 0x209942d4 )
	ROM_LOAD16_BYTE( "b03.rom",      0x20000, 0x8000, 0xaef7df48 )
	ROM_LOAD16_BYTE( "b07.rom",      0x20001, 0x8000, 0xe9bd6250 )
	ROM_LOAD16_BYTE( "b04.rom",      0x30000, 0x8000, 0x94bd4bd7 )
	ROM_LOAD16_BYTE( "b08.rom",      0x30001, 0x8000, 0xb7236567 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for sound */
	ROM_LOAD(       "b09.rom",      0x00000, 0x4000, 0x539d0c49 )

	ROM_REGION( 0x0200,  REGION_SOUND1, 0 )      /* 2x 256 byte for 0005289 wavetable data */
	ROM_LOAD(      "400-a01.fse",  0x00000, 0x0100, 0x5827b1e8 )
	ROM_LOAD(      "400-a02.fse",  0x00100, 0x0100, 0x2f44f970 )
ROM_END

ROM_START( rf2 )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )    /* 5 * 64k for code and rom */
	ROM_LOAD16_BYTE( "400-a06.15l",  0x00000, 0x08000, 0xb99d8cff )
	ROM_LOAD16_BYTE( "400-a04.10l",  0x00001, 0x08000, 0xd02c9552 )
	ROM_LOAD16_BYTE( "561-a07.17l",  0x80000, 0x20000, 0xed6e7098 )
	ROM_LOAD16_BYTE( "561-a05.12l",  0x80001, 0x20000, 0xdfe04425 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for sound */
	ROM_LOAD(      "400-e03.5l",   0x00000, 0x02000, 0xa5a8e57d )

	ROM_REGION( 0x0200,  REGION_SOUND1, 0 )      /* 2x 256 byte for 0005289 wavetable data */
	ROM_LOAD(      "400-a01.fse",  0x00000, 0x0100, 0x5827b1e8 )
	ROM_LOAD(      "400-a02.fse",  0x00100, 0x0100, 0x2f44f970 )
ROM_END

ROM_START( twinbee )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )    /* 5 * 64k for code and rom */
	ROM_LOAD16_BYTE( "400-a06.15l",  0x00000, 0x08000, 0xb99d8cff )
	ROM_LOAD16_BYTE( "400-a04.10l",  0x00001, 0x08000, 0xd02c9552 )
	ROM_LOAD16_BYTE( "412-a07.17l",  0x80000, 0x20000, 0xd93c5499 )
	ROM_LOAD16_BYTE( "412-a05.12l",  0x80001, 0x20000, 0x2b357069 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for sound */
	ROM_LOAD(      "400-e03.5l",   0x00000, 0x02000, 0xa5a8e57d )

	ROM_REGION( 0x0200,  REGION_SOUND1, 0 )      /* 2x 256 byte for 0005289 wavetable data */
	ROM_LOAD(      "400-a01.fse",  0x00000, 0x0100, 0x5827b1e8 )
	ROM_LOAD(      "400-a02.fse",  0x00100, 0x0100, 0x2f44f970 )
ROM_END

ROM_START( gradius )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )    /* 5 * 64k for code and rom */
	ROM_LOAD16_BYTE( "400-a06.15l",  0x00000, 0x08000, 0xb99d8cff )
	ROM_LOAD16_BYTE( "400-a04.10l",  0x00001, 0x08000, 0xd02c9552 )
	ROM_LOAD16_BYTE( "456-a07.17l",  0x80000, 0x20000, 0x92df792c )
	ROM_LOAD16_BYTE( "456-a05.12l",  0x80001, 0x20000, 0x5cafb263 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for sound */
	ROM_LOAD(      "400-e03.5l",   0x00000, 0x2000, 0xa5a8e57d )

	ROM_REGION( 0x0200,  REGION_SOUND1, 0 )      /* 2x 256 byte for 0005289 wavetable data */
	ROM_LOAD(      "400-a01.fse",  0x00000, 0x0100, 0x5827b1e8 )
	ROM_LOAD(      "400-a02.fse",  0x00100, 0x0100, 0x2f44f970 )
ROM_END

ROM_START( gwarrior )
	ROM_REGION( 0xc0000, REGION_CPU1, 0 )    /* 5 * 64k for code and rom */
	ROM_LOAD16_BYTE( "400-a06.15l",  0x00000, 0x08000, 0xb99d8cff )
	ROM_LOAD16_BYTE( "400-a04.10l",  0x00001, 0x08000, 0xd02c9552 )
	ROM_LOAD16_BYTE( "578-a07.17l",  0x80000, 0x20000, 0x0aedacb5 )
	ROM_LOAD16_BYTE( "578-a05.12l",  0x80001, 0x20000, 0x76240e2e )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for sound */
	ROM_LOAD(      "400-e03.5l",   0x00000, 0x02000, 0xa5a8e57d )

	ROM_REGION( 0x0200,  REGION_SOUND1, 0 )      /* 2x 256 byte for 0005289 wavetable data */
	ROM_LOAD(      "400-a01.fse",  0x00000, 0x0100, 0x5827b1e8 )
	ROM_LOAD(      "400-a02.fse",  0x00100, 0x0100, 0x2f44f970 )
ROM_END

ROM_START( salamand )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )    /* 64k for code */
	ROM_LOAD16_BYTE( "18b.bin",      0x00000, 0x10000, 0xa42297f9 )
	ROM_LOAD16_BYTE( "18c.bin",      0x00001, 0x10000, 0xf9130b0a )
	ROM_LOAD16_BYTE( "17b.bin",      0x40000, 0x20000, 0xe5caf6e6 )
	ROM_LOAD16_BYTE( "17c.bin",      0x40001, 0x20000, 0xc2f567ea )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for sound */
	ROM_LOAD(      "11j.bin",      0x00000, 0x08000, 0x5020972c )

	ROM_REGION( 0x04000, REGION_SOUND1, 0 )    /* VLM5030 data? */
	ROM_LOAD(      "8g.bin",       0x00000, 0x04000, 0xf9ac6b82 )

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )    /* 007232 data */
	ROM_LOAD(      "10a.bin",      0x00000, 0x20000, 0x09fe0632 )
ROM_END

ROM_START( salamanj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )    /* 64k for code */
	ROM_LOAD16_BYTE( "587j02.18b",   0x00000, 0x10000, 0xf68ee99a )
	ROM_LOAD16_BYTE( "587j05.18c",   0x00001, 0x10000, 0x72c16128 )
	ROM_LOAD16_BYTE( "17b.bin",      0x40000, 0x20000, 0xe5caf6e6 )
	ROM_LOAD16_BYTE( "17c.bin",      0x40001, 0x20000, 0xc2f567ea )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for sound */
	ROM_LOAD(      "11j.bin",      0x00000, 0x08000, 0x5020972c )

	ROM_REGION( 0x04000, REGION_SOUND1, 0 )    /* VLM5030 data? */
	ROM_LOAD(      "8g.bin",       0x00000, 0x04000, 0xf9ac6b82 )

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )    /* 007232 data */
	ROM_LOAD(      "10a.bin",      0x00000, 0x20000, 0x09fe0632 )
ROM_END

ROM_START( lifefrce )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )    /* 64k for code */
	ROM_LOAD16_BYTE( "587-k02.bin",  0x00000, 0x10000, 0x4a44da18 )
	ROM_LOAD16_BYTE( "587-k05.bin",  0x00001, 0x10000, 0x2f8c1cbd )
	ROM_LOAD16_BYTE( "17b.bin",      0x40000, 0x20000, 0xe5caf6e6 )
	ROM_LOAD16_BYTE( "17c.bin",      0x40001, 0x20000, 0xc2f567ea )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for sound */
	ROM_LOAD(      "587-k09.bin",  0x00000, 0x08000, 0x2255fe8c )

	ROM_REGION( 0x04000, REGION_SOUND1, 0 )    /* VLM5030 data? */
	ROM_LOAD(      "587-k08.bin",  0x00000, 0x04000, 0x7f0e9b41 )

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )    /* 007232 data */
	ROM_LOAD(      "10a.bin",      0x00000, 0x20000, 0x09fe0632 )
ROM_END

ROM_START( lifefrcj )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )    /* 64k for code */
	ROM_LOAD16_BYTE( "587-n02.bin",  0x00000, 0x10000, 0x235dba71 )
	ROM_LOAD16_BYTE( "587-n05.bin",  0x00001, 0x10000, 0x054e569f )
	ROM_LOAD16_BYTE( "587-n03.bin",  0x40000, 0x20000, 0x9041f850 )
	ROM_LOAD16_BYTE( "587-n06.bin",  0x40001, 0x20000, 0xfba8b6aa )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for sound */
	ROM_LOAD(      "587-n09.bin",  0x00000, 0x08000, 0xe8496150 )

	ROM_REGION( 0x04000, REGION_SOUND1, 0 )    /* VLM5030 data? */
	ROM_LOAD(      "587-k08.bin",  0x00000, 0x04000, 0x7f0e9b41 )

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )    /* 007232 data */
	ROM_LOAD(      "10a.bin",      0x00000, 0x20000, 0x09fe0632 )
ROM_END

ROM_START( blkpnthr )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )    /* 64k for code */
	ROM_LOAD16_BYTE( "604f02.18b",   0x00000, 0x10000, 0x487bf8da )
	ROM_LOAD16_BYTE( "604f05.18c",   0x00001, 0x10000, 0xb08f8ca2 )
	ROM_LOAD16_BYTE( "604c03.17b",   0x40000, 0x20000, 0x815bc3b0 )
	ROM_LOAD16_BYTE( "604c06.17c",   0x40001, 0x20000, 0x4af6bf7f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for sound */
	ROM_LOAD(      "604a08.11j",   0x00000, 0x08000, 0xaff88a2b )

	ROM_REGION( 0x20000, REGION_SOUND2, 0 )    /* 007232 data */
	ROM_LOAD(      "604a01.10a",   0x00000, 0x20000, 0xeceb64a6 )
ROM_END

ROM_START( citybomb )
	ROM_REGION( 0x1c0000, REGION_CPU1, 0 )    /* 64k for code */
	ROM_LOAD16_BYTE( "g10.rom",      0x000000, 0x10000, 0x26207530 )
	ROM_LOAD16_BYTE( "g09.rom",      0x000001, 0x10000, 0xce7de262 )
	ROM_LOAD16_BYTE( "787g08.15f",   0x100000, 0x20000, 0x6242ef35 )
	ROM_LOAD16_BYTE( "787g07.15d",   0x100001, 0x20000, 0x21be5e9e )
	ROM_LOAD16_BYTE( "787e06.14f",   0x140000, 0x20000, 0xc251154a )
	ROM_LOAD16_BYTE( "787e05.14d",   0x140001, 0x20000, 0x0781e22d )
	ROM_LOAD16_BYTE( "787g04.13f",   0x180000, 0x20000, 0x137cf39f )
	ROM_LOAD16_BYTE( "787g03.13d",   0x180001, 0x20000, 0x0cc704dc )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for sound */
	ROM_LOAD(      "787e02.4h",   0x00000, 0x08000, 0xf4591e46 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )    /* 007232 data */
	ROM_LOAD(      "787e01.1k",   0x00000, 0x80000, 0xedc34d01 )
ROM_END

ROM_START( citybmrj )
	ROM_REGION( 0x1c0000, REGION_CPU1, 0 )    /* 64k for code */
	ROM_LOAD16_BYTE( "787h10.15k",   0x000000, 0x10000, 0x66fecf69 )
	ROM_LOAD16_BYTE( "787h09.15h",   0x000001, 0x10000, 0xa0e29468 )
	ROM_LOAD16_BYTE( "787g08.15f",   0x100000, 0x20000, 0x6242ef35 )
	ROM_LOAD16_BYTE( "787g07.15d",   0x100001, 0x20000, 0x21be5e9e )
	ROM_LOAD16_BYTE( "787e06.14f",   0x140000, 0x20000, 0xc251154a )
	ROM_LOAD16_BYTE( "787e05.14d",   0x140001, 0x20000, 0x0781e22d )
	ROM_LOAD16_BYTE( "787g04.13f",   0x180000, 0x20000, 0x137cf39f )
	ROM_LOAD16_BYTE( "787g03.13d",   0x180001, 0x20000, 0x0cc704dc )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for sound */
	ROM_LOAD(      "787e02.4h",   0x00000, 0x08000, 0xf4591e46 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )    /* 007232 data */
	ROM_LOAD(      "787e01.1k",   0x00000, 0x80000, 0xedc34d01 )
ROM_END

ROM_START( kittenk )
	ROM_REGION( 0x140000, REGION_CPU1, 0 )    /* 64k for code */
	ROM_LOAD16_BYTE( "kitten.15k",   0x000000, 0x10000, 0x8267cb2b )
	ROM_LOAD16_BYTE( "kitten.15h",   0x000001, 0x10000, 0xeb41cfa5 )
	ROM_LOAD16_BYTE( "712b08.15f",   0x100000, 0x20000, 0xe6d71611 )
	ROM_LOAD16_BYTE( "712b07.15d",   0x100001, 0x20000, 0x30f75c9f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for sound */
	ROM_LOAD(      "712e02.4h",   0x00000, 0x08000, 0xba76f310 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )    /* 007232 data */
	ROM_LOAD(      "712b01.1k",   0x00000, 0x80000, 0xf65b5d95 )
ROM_END

ROM_START( nyanpani )
	ROM_REGION( 0x140000, REGION_CPU1, 0 )    /* 64k for code */
	ROM_LOAD16_BYTE( "712j10.15k",   0x000000, 0x10000, 0x924b27ec )
	ROM_LOAD16_BYTE( "712j09.15h",   0x000001, 0x10000, 0xa9862ea1 )
	ROM_LOAD16_BYTE( "712b08.15f",   0x100000, 0x20000, 0xe6d71611 )
	ROM_LOAD16_BYTE( "712b07.15d",   0x100001, 0x20000, 0x30f75c9f )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )    /* 64k for sound */
	ROM_LOAD(      "712e02.4h",   0x00000, 0x08000, 0xba76f310 )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )    /* 007232 data */
	ROM_LOAD(      "712b01.1k",   0x00000, 0x80000, 0xf65b5d95 )
ROM_END



GAMEX(1985, nemesis,  0,        nemesis,       nemesis,  0, ROT0,   "Konami", "Nemesis", GAME_NO_COCKTAIL )
GAMEX(1985, nemesuk,  nemesis,  nemesis,       nemesuk,  0, ROT0,   "Konami", "Nemesis (World?)", GAME_NO_COCKTAIL )
GAMEX(1985, konamigt, 0,        konamigt,      konamigt, 0, ROT0,   "Konami", "Konami GT", GAME_NO_COCKTAIL )
GAMEX(1985, rf2,      konamigt, rf2_gx400,     rf2,      0, ROT0,   "Konami", "Konami RF2 - Red Fighter", GAME_NO_COCKTAIL )
GAMEX(1985, twinbee,  0,        twinbee_gx400, twinbee,  0, ORIENTATION_SWAP_XY, "Konami", "TwinBee", GAME_NO_COCKTAIL )
GAMEX(1985, gradius,  nemesis,  gx400,         gradius,  0, ROT0,   "Konami", "Gradius", GAME_NO_COCKTAIL )
GAMEX(1985, gwarrior, 0,        gx400,         gwarrior, 0, ROT0,   "Konami", "Galactic Warriors", GAME_NO_COCKTAIL )
GAMEX(1986, salamand, 0,        salamand,      salamand, 0, ROT0,   "Konami", "Salamander (version D)", GAME_NO_COCKTAIL )
GAMEX(1986, salamanj, salamand, salamand,      salamand, 0, ROT0,   "Konami", "Salamander (version J)", GAME_NO_COCKTAIL )
GAMEX(1986, lifefrce, salamand, salamand,      salamand, 0, ROT0,   "Konami", "Lifeforce (US)", GAME_NO_COCKTAIL )
GAMEX(1986, lifefrcj, salamand, salamand,      lifefrcj, 0, ROT0,   "Konami", "Lifeforce (Japan)", GAME_NO_COCKTAIL )
GAMEX(1987, blkpnthr, 0,        blkpnthr,      blkpnthr, 0, ROT0,   "Konami", "Black Panther", GAME_NO_COCKTAIL )
GAMEX(1987, citybomb, 0,        citybomb,      citybomb, 0, ROT270, "Konami", "City Bomber (World)", GAME_NO_COCKTAIL )
GAMEX(1987, citybmrj, citybomb, citybomb,      citybomb, 0, ROT270, "Konami", "City Bomber (Japan)", GAME_NO_COCKTAIL )
GAMEX(1988, kittenk,  0,        nyanpani,      nyanpani, 0, ROT0,   "Konami", "Kitten Kaboodle", GAME_NO_COCKTAIL )
GAMEX(1988, nyanpani, kittenk,  nyanpani,      nyanpani, 0, ROT0,   "Konami", "Nyan Nyan Panic (Japan)", GAME_NO_COCKTAIL )
