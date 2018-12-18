/***************************************************************************

	Seibu Sound System v1.02, games using this include:

	Dead Angle       1988?	* "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
	Dynamite Duke    1989	* "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
	Toki             1989	* "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
	Raiden           1990	* "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
	Blood Brothers   1990	  "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
	D-Con            1992	  "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."

	Related sound programs (not implemented yet):

	Cross Shooter    1986?	* "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC." (YM2151 substituted for YM3812)
	Cabal            1988	* "Michel/Seibu    sound 11/04/88" (YM2151 substituted for YM3812, unknown ADPCM)
	Zero Team            	  "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC."
	Legionaire           	  "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC." (YM2151 substituted for YM3812)
	Raiden 2             	  "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC." (YM2151 substituted for YM3812, plus extra MSM6205)
	Raiden DX            	  "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC." (YM2151 substituted for YM3812, plus extra MSM6205)
	Cup Soccer           	  "START UP PROGRAM V1.02 (C)1986 SEIBU KAIHATSU INC." (YM2151 substituted for YM3812, plus extra MSM6205)
	SD Gundam Psycho Salamander "Copyright by King Bee Sol 1991"
	* = encrypted

***************************************************************************/

extern const struct Memory_ReadAddress seibu_sound_readmem[];
extern const struct Memory_WriteAddress seibu_sound_writemem[];
extern const struct Memory_ReadAddress seibu2_sound_readmem[];
extern const struct Memory_WriteAddress seibu2_sound_writemem[];

READ16_HANDLER( seibu_main_word_r );
READ_HANDLER( seibu_main_v30_r );
WRITE16_HANDLER( seibu_main_word_w );
WRITE_HANDLER( seibu_main_v30_w );

WRITE_HANDLER( seibu_irq_clear_w );
WRITE_HANDLER( seibu_rst10_ack_w );
WRITE_HANDLER( seibu_rst18_ack_w );
WRITE_HANDLER( seibu_bank_w );
WRITE_HANDLER( seibu_coin_w );
void seibu_ym3812_irqhandler(int linestate);
void seibu_ym2151_irqhandler(int linestate);
READ_HANDLER( seibu_soundlatch_r );
READ_HANDLER( seibu_main_data_pending_r );
WRITE_HANDLER( seibu_main_data_w );
MACHINE_INIT( seibu_sound_1 );
MACHINE_INIT( seibu_sound_2 );
void seibu_sound_decrypt(int cpu_region,int length);

/**************************************************************************/

#define SEIBU_COIN_INPUTS											\
	PORT_START														\
	PORT_BIT_IMPULSE( 0x01, IP_ACTIVE_HIGH, IPT_COIN1, 4 )			\
	PORT_BIT_IMPULSE( 0x02, IP_ACTIVE_HIGH, IPT_COIN2, 4 )


#define SEIBU_SOUND_SYSTEM_YM3812_HARDWARE(freq1,freq2,region)		\
																	\
static struct YM3812interface ym3812_interface =					\
{																	\
	1,																\
	freq1,															\
	{ 100 },														\
	{ seibu_ym3812_irqhandler },									\
};																	\
																	\
static struct OKIM6295interface okim6295_interface =				\
{																	\
	1,																\
	{ freq2 },														\
	{ region },														\
	{ 40 }															\
}

#define SEIBU_SOUND_SYSTEM_YM2151_HARDWARE(freq1,freq2,region)		\
																	\
static struct YM2151interface ym2151_interface =					\
{																	\
	1,																\
	freq1,															\
	{ 50 },															\
	{ seibu_ym2151_irqhandler },									\
};																	\
																	\
static struct OKIM6295interface okim6295_interface2 =				\
{																	\
	1,																\
	{ freq2 },														\
	{ region },														\
	{ 40 }															\
}

#define SEIBU_SOUND_SYSTEM_CPU(freq)								\
	MDRV_CPU_ADD(Z80, freq)											\
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)									\
	MDRV_CPU_MEMORY(seibu_sound_readmem,seibu_sound_writemem)		\

#define SEIBU2_SOUND_SYSTEM_CPU(freq)								\
	MDRV_CPU_ADD(Z80, freq)											\
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)									\
	MDRV_CPU_MEMORY(seibu2_sound_readmem,seibu2_sound_writemem)		\

#define SEIBU_SOUND_SYSTEM_YM3812_INTERFACE							\
	MDRV_SOUND_ADD(YM3812, ym3812_interface)						\
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)					\

#define SEIBU_SOUND_SYSTEM_YM2151_INTERFACE							\
	MDRV_SOUND_ADD(YM2151, ym2151_interface)						\
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface2)					\

/**************************************************************************/

