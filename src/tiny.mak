# a tiny compile is without Neogeo games
COREDEFS += -DTINY_COMPILE=1
#COREDEFS += -DTINY_NAME="driver_pairsten,driver_mmcamera,driver_kanatuen,driver_idhimitu,driver_msjiken,driver_hanamomo,driver_telmahjn,driver_gionbana,driver_mjfocus,driver_mjfocusm,driver_peepshow,driver_mgmen89,driver_scandal,driver_scandalm,driver_mjnanpas,driver_mjnanpaa,driver_mjnanpau,driver_bananadr,driver_mladyhtr,driver_chinmoku,driver_maiko,driver_club90s,driver_club90sa,driver_hanaoji,driver_crystalg,driver_crystal2,driver_citylove,driver_apparel, driver_secolove,driver_housemnq,driver_housemn2,driver_seiha,   driver_seiham,  driver_bijokkoy,driver_iemoto,  driver_ojousan, driver_orangec, driver_bijokkog,driver_kaguya,  driver_otonano, driver_mjsikaku,driver_mjsikakb,driver_mjcamera,driver_korinai"
#COREDEFS += -DTINY_POINTER="&driver_pairsten,&driver_mmcamera,&driver_kanatuen,&driver_idhimitu,&driver_msjiken,&driver_hanamomo,&driver_telmahjn,&driver_gionbana,&driver_mjfocus,&driver_mjfocusm,&driver_peepshow,&driver_mgmen89,&driver_scandal,&driver_scandalm,&driver_mjnanpas,&driver_mjnanpaa,&driver_mjnanpau,&driver_bananadr,&driver_mladyhtr,&driver_chinmoku,&driver_maiko,&driver_club90s,&driver_club90sa,&driver_hanaoji,&driver_crystalg,&driver_crystal2,&driver_citylove,&driver_apparel, &driver_secolove,&driver_housemnq,&driver_housemn2,&driver_seiha,   &driver_seiham,  &driver_bijokkoy,&driver_iemoto,  &driver_ojousan, &driver_orangec, &driver_bijokkog,&driver_kaguya,  &driver_otonano, &driver_mjsikaku,&driver_mjsikakb,&driver_mjcamera,&driver_korinai"
COREDEFS += -DTINY_NAME="driver_bbbxing,driver_desertwr,driver_gametngk,driver_tetrisp,driver_p47aces,driver_gratia,driver_hayaosi1,driver_kirarast,driver_f1superb,driver_tp2m32"
COREDEFS += -DTINY_POINTER="&driver_bbbxing,&driver_desertwr,&driver_gametngk,&driver_tetrisp,&driver_p47aces,&driver_gratia,&driver_hayaosi1,&driver_kirarast,&driver_f1superb,&driver_tp2m32"


# uses these CPUs
CPUS+=M68000@
CPUS+=M68010@
CPUS+=M68EC020@
CPUS+=M68020@
CPUS+=Z80@
CPUS+=M6502@
CPUS+=M65C02@
CPUS+=M6809@
CPUS+=M6800@
CPUS+=TMS34010@
CPUS+=TMS34020@
CPUS+=TMS32010@
CPUS+=TMS32025@
CPUS+=UPD7810@
CPUS+=V70@

# uses these SOUNDs
SOUNDS+=DAC@
SOUNDS+=AY8910@
SOUNDS+=YM2203@
SOUNDS+=YM2151_ALT@
SOUNDS+=YM2608@
SOUNDS+=YM2610@
SOUNDS+=YM2610B@
SOUNDS+=YM2612@
SOUNDS+=YM3438@
SOUNDS+=YM2413@
SOUNDS+=YM3812@
SOUNDS+=YMZ280B@
SOUNDS+=YM3526@
SOUNDS+=Y8950@
SOUNDS+=SN76477@
SOUNDS+=SN76496@
SOUNDS+=ADPCM@
SOUNDS+=OKIM6295@
SOUNDS+=MSM5205@
SOUNDS+=CUSTOM@

#OBJS = $(OBJ)/machine/nb1413m3.o $(OBJ)/vidhrdw/nbmj8688.o $(OBJ)/drivers/nbmj8688.o $(OBJ)/vidhrdw/nbmj8891.o $(OBJ)/drivers/nbmj8891.o
OBJS = $(OBJ)/vidhrdw/ms32.o $(OBJ)/drivers/ms32.o $(OBJ)/vidhrdw/tetrisp2.o $(OBJ)/drivers/tetrisp2.o

# MAME specific core objs
COREOBJS += $(OBJ)/driver.o $(OBJ)/cheat.o
