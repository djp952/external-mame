/***************************************************************************

	Atari Klax hardware

****************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"
#include "klax.h"



/*************************************
 *
 *	Video system start
 *
 *************************************/

VIDEO_START( klax )
{
	static const struct ataripf_desc pfdesc =
	{
		0,			/* index to which gfx system */
		64,32,		/* size of the playfield in tiles (x,y) */
		32,1,		/* tile_index = x * xmult + y * ymult (xmult,ymult) */

		0x100,		/* index of palette base */
		0x100,		/* maximum number of colors */
		0,			/* color XOR for shadow effect (if any) */
		0,			/* latch mask */
		0,			/* transparent pen mask */

		0x01fff,	/* tile data index mask */
		0xf0000,	/* tile data color mask */
		0x08000,	/* tile data hflip mask */
		0,			/* tile data vflip mask */
		0			/* tile data priority mask */
	};

	static const struct atarimo_desc modesc =
	{
		1,					/* index to which gfx system */
		1,					/* number of motion object banks */
		1,					/* are the entries linked? */
		0,					/* are the entries split? */
		0,					/* render in reverse order? */
		0,					/* render in swapped X/Y order? */
		0,					/* does the neighbor bit affect the next object? */
		8,					/* pixels per SLIP entry (0 for no-slip) */
		8,					/* number of scanlines between MO updates */

		0x000,				/* base palette entry */
		0x100,				/* maximum number of colors */
		0,					/* transparent pen index */

		{{ 0x00ff,0,0,0 }},	/* mask for the link */
		{{ 0 }},			/* mask for the graphics bank */
		{{ 0,0x0fff,0,0 }},	/* mask for the code index */
		{{ 0 }},			/* mask for the upper code index */
		{{ 0,0,0x000f,0 }},	/* mask for the color */
		{{ 0,0,0xff80,0 }},	/* mask for the X position */
		{{ 0,0,0,0xff80 }},	/* mask for the Y position */
		{{ 0,0,0,0x0070 }},	/* mask for the width, in tiles*/
		{{ 0,0,0,0x0007 }},	/* mask for the height, in tiles */
		{{ 0,0,0,0x0008 }},	/* mask for the horizontal flip */
		{{ 0 }},			/* mask for the vertical flip */
		{{ 0 }},			/* mask for the priority */
		{{ 0 }},			/* mask for the neighbor */
		{{ 0 }},			/* mask for absolute coordinates */

		{{ 0 }},			/* mask for the ignore value */
		0,					/* resulting value to indicate "ignore" */
		0					/* callback routine for ignored entries */
	};

	/* initialize the playfield */
	if (!ataripf_init(0, &pfdesc))
		return 1;

	/* initialize the motion objects */
	if (!atarimo_init(0, &modesc))
		return 1;
	return 0;
}



/*************************************
 *
 *	Latch write handler
 *
 *************************************/

WRITE16_HANDLER( klax_latch_w )
{
}



/*************************************
 *
 *	Overrendering
 *
 *************************************/

static int overrender_callback(struct ataripf_overrender_data *data, int state)
{
	/* we need to check tile-by-tile, so always return OVERRENDER_SOME */
	if (state == OVERRENDER_BEGIN)
	{
		/* by default, draw anywhere the MO pen was non-0 */
		data->drawmode = TRANSPARENCY_NONE;
		data->drawpens = 0;
		data->maskpens = 0x0001;

		/* only need to query if we are modifying the color */
		return OVERRENDER_SOME;
	}

	/* handle a query */
	else if (state == OVERRENDER_QUERY)
		return (data->pfcolor == 15) ? OVERRENDER_YES : OVERRENDER_NO;
	return 0;
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

VIDEO_UPDATE( klax )
{
	/* draw the layers */
	ataripf_render(0, bitmap, cliprect);
	atarimo_render(0, bitmap, cliprect, overrender_callback, NULL);
}
