int  funkyjet_vh_start(void);
void funkyjet_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);

WRITE16_HANDLER( funkyjet_pf2_data_w );
WRITE16_HANDLER( funkyjet_pf1_data_w );
WRITE16_HANDLER( funkyjet_control_0_w );

extern data16_t *funkyjet_pf1_data;
extern data16_t *funkyjet_pf2_data;
extern data16_t *funkyjet_pf1_row;
