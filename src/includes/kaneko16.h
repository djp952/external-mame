/***************************************************************************

							-= Kaneko 16 Bit Games =-

***************************************************************************/

/* Tile Layers: */

extern data16_t *kaneko16_vram_0,    *kaneko16_vram_1,    *kaneko16_layers_0_regs;
extern data16_t *kaneko16_vscroll_0, *kaneko16_vscroll_1;
extern data16_t *kaneko16_vram_2,    *kaneko16_vram_3,    *kaneko16_layers_1_regs;
extern data16_t *kaneko16_vscroll_2, *kaneko16_vscroll_3;

WRITE16_HANDLER( kaneko16_vram_0_w );
WRITE16_HANDLER( kaneko16_vram_1_w );
WRITE16_HANDLER( kaneko16_vram_2_w );
WRITE16_HANDLER( kaneko16_vram_3_w );

WRITE16_HANDLER( kaneko16_layers_0_regs_w );
WRITE16_HANDLER( kaneko16_layers_1_regs_w );


/* Sprites: */

extern int kaneko16_sprite_type;
extern data16_t kaneko16_sprite_xoffs, kaneko16_sprite_flipx;
extern data16_t kaneko16_sprite_yoffs, kaneko16_sprite_flipy;
extern data16_t *kaneko16_sprites_regs;

READ16_HANDLER ( kaneko16_sprites_regs_r );
WRITE16_HANDLER( kaneko16_sprites_regs_w );

void kaneko16_draw_sprites(struct mame_bitmap *bitmap, int pri);

/* Pixel Layer: */

extern data16_t *kaneko16_bg15_select, *kaneko16_bg15_reg;

READ16_HANDLER ( kaneko16_bg15_select_r );
WRITE16_HANDLER( kaneko16_bg15_select_w );

READ16_HANDLER ( kaneko16_bg15_reg_r );
WRITE16_HANDLER( kaneko16_bg15_reg_w );

void berlwall_init_palette(unsigned char *obsolete,unsigned short *colortable,const unsigned char *color_prom);


/* Priorities: */

typedef struct
{
	int tile[4];
	int sprite[4];
}	kaneko16_priority_t;

extern kaneko16_priority_t kaneko16_priority;


/* Machine */

int kaneko16_vh_start_sprites(void);
int kaneko16_vh_start_1xVIEW2(void);
int kaneko16_vh_start_2xVIEW2(void);
int berlwall_vh_start(void);
int sandscrp_vh_start_1xVIEW2(void);

void kaneko16_vh_stop(void);
void berlwall_vh_stop(void);

void kaneko16_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);

void kaneko16_init_machine(void);


/* in drivers/galpani2.c */

void galpani2_mcu_run(void);

/* in vidhrdw/galpani2.c */

extern data16_t *galpani2_bg8_0,         *galpani2_bg8_1;
extern data16_t *galpani2_palette_0,     *galpani2_palette_1;
extern data16_t *galpani2_bg8_regs_0,    *galpani2_bg8_regs_1;
extern data16_t *galpani2_bg8_0_scrollx, *galpani2_bg8_1_scrollx;
extern data16_t *galpani2_bg8_0_scrolly, *galpani2_bg8_1_scrolly;

extern data16_t *galpani2_bg15;

void galpani2_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int  galpani2_vh_start(void);
void galpani2_vh_stop(void);
void galpani2_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);

WRITE16_HANDLER( galpani2_palette_0_w );
WRITE16_HANDLER( galpani2_palette_1_w );

READ16_HANDLER ( galpani2_bg8_regs_0_r );
READ16_HANDLER ( galpani2_bg8_regs_1_r );
WRITE16_HANDLER( galpani2_bg8_regs_0_w );
WRITE16_HANDLER( galpani2_bg8_regs_1_w );
WRITE16_HANDLER( galpani2_bg8_0_w );
WRITE16_HANDLER( galpani2_bg8_1_w );

WRITE16_HANDLER( galpani2_bg15_w );
