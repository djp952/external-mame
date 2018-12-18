extern void mjsikaku_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);
extern void secolove_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);
extern void seiha_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);
extern void crystal2_init_palette(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);
extern void mjsikaku_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
extern int mjsikaku_vh_start(void);
extern int secolove_vh_start(void);
extern int bijokkoy_vh_start(void);
extern int housemnq_vh_start(void);
extern int seiha_vh_start(void);
extern int crystal2_vh_start(void);
extern void mjsikaku_vh_stop(void);

extern WRITE_HANDLER( mjsikaku_palette_w );
extern WRITE_HANDLER( secolove_palette_w );
extern void mjsikaku_radrx_w(int data);
extern void mjsikaku_radry_w(int data);
extern void mjsikaku_sizex_w(int data);
extern void mjsikaku_sizey_w(int data);
extern void mjsikaku_dispflag_w(int data);
extern void mjsikaku_dispflag2_w(int data);
extern void mjsikaku_drawx_w(int data);
extern void mjsikaku_drawy_w(int data);
extern void mjsikaku_scrolly_w(int data);
extern void mjsikaku_romsel_w(int data);
extern void secolove_romsel_w(int data);
extern void iemoto_romsel_w(int data);
extern void crystal2_romsel_w(int data);
