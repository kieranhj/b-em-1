#include <allegro.h>
#include "b-em.h"

#include "model.h"
#include "cmos.h"
#include "mem.h"
#include "tube.h"
#include "NS32016/32016.h"
#include "6502tube.h"
#include "65816.h"
#include "arm.h"
#include "x86_tube.h"
#include "z80.h"

int I8271, WD1770, BPLUS, x65c02, MASTER, MODELA, OS01, compactcmos;
int curtube;
int oldmodel;

MODEL models[NUM_MODELS] =
{
/*       Name                        8271  1770  65c02  B+  Master  SWRAM  A  OS 0.1  Compact  OS      ROM dir   CMOS           ROM setup function         Second processor*/
        {"BBC A w/OS 0.1",           1,    0,    0,     0,  0,      0,     1, 1,      0,       "",     "a01",    "",            mem_romsetup_os01,         -1},
        {"BBC B w/OS 0.1",           1,    0,    0,     0,  0,      0,     0, 1,      0,       "",     "a01",    "",            mem_romsetup_os01,         -1},
        {"BBC A",                    1,    0,    0,     0,  0,      0,     1, 0,      0,       "os",   "a",      "",            NULL,                      -1},
        {"BBC B w/8271 FDC",         1,    0,    0,     0,  0,      0,     0, 0,      0,       "os",   "b",      "",            NULL,                      -1},
        {"BBC B w/8271+SWRAM",       1,    0,    0,     0,  0,      1,     0, 0,      0,       "os",   "b",      "",            NULL,                      -1},
        {"BBC B w/1770 FDC",         0,    1,    0,     0,  0,      1,     0, 0,      0,       "os",   "b1770",  "",            NULL,                      -1},
        {"BBC B US",                 1,    0,    0,     0,  0,      0,     0, 0,      0,       "usmos","us",     "",            NULL,                      -1},
        {"BBC B German",             1,    0,    0,     0,  0,      0,     0, 0,      0,       "deos", "us",     "",            NULL,                      -1},
        {"BBC B+ 64K",               0,    1,    0,     1,  0,      0,     0, 0,      0,       "bpos", "bp",     "",            NULL,                      -1},
        {"BBC B+ 128K",              0,    1,    0,     1,  0,      0,     0, 0,      0,       "bpos", "bp",     "",            mem_romsetup_bplus128,     -1},
        {"BBC Master 128",           0,    1,    1,     0,  1,      0,     0, 0,      0,       "",     "master", "cmos.bin",    mem_romsetup_master128,    -1},
        {"BBC Master 512",           0,    1,    1,     0,  1,      0,     0, 0,      0,       "",     "master", "cmos.bin",    mem_romsetup_master128,     3},
        {"BBC Master Turbo",         0,    1,    1,     0,  1,      0,     0, 0,      0,       "",     "master", "cmos.bin",    mem_romsetup_master128,     0},
        {"BBC Master Compact",       0,    1,    1,     0,  1,      0,     0, 0,      1,       "",     "compact","cmosc.bin",   mem_romsetup_mastercompact,-1},
        {"ARM Evaluation System",    0,    1,    1,     0,  1,      0,     0, 0,      0,       "",     "master", "cmosa.bin",   mem_romsetup_master128,     1},
        {"BBC Master 128 w/MOS 3.5", 0,    1,    1,     0,  1,      0,     0, 0,      0,       "",     "master", "cmos350.bin", mem_romsetup_master128_35, -1},
        {"",0,0,0,0,0,0,0,0,0,"","","",0,0}
};

static int _modelcount = 0;
char *model_get()
{
        return models[_modelcount++].name;
}

extern cpu_debug_t n32016_cpu_debug;

TUBE tubes[NUM_TUBES]=
{
        {"6502", tube_6502_init,  tube_6502_reset, &tube6502_cpu_debug  },
        {"ARM",  tube_arm_init,   arm_reset,       &tubearm_cpu_debug   },
        {"Z80",  tube_z80_init,   z80_reset,       &tubez80_cpu_debug   }, 
        {"80186",tube_x86_init,   x86_reset,       &tubex86_cpu_debug   },
        {"65816",tube_65816_init, w65816_reset,    &tube65816_cpu_debug },
        {"32016",tube_32016_init, n32016_reset,    &n32016_cpu_debug    },
        {"",0,0}
};

void model_check(void) {
    const int defmodel = 3;
    
    if (curmodel < 0 || curmodel >= NUM_MODELS) {
        log_warn("No model #%d, using #%d (%s) instead", curmodel, defmodel, models[defmodel].name);
        curmodel = defmodel;
    }
    if (models[curmodel].tube != -1)
        curtube = models[curmodel].tube;
    if (curtube < -1 || curtube >= NUM_TUBES) {
        log_warn("No tube #%d, running with no tube instead", curtube);
        curtube = -1;
    }
}

void model_init()
{
        char t[512],t2[512];
        log_debug("Starting emulation as %s\n",models[curmodel].name);
        I8271       = models[curmodel].I8271;
        WD1770      = models[curmodel].WD1770;
        BPLUS       = models[curmodel].bplus;
        x65c02      = models[curmodel].x65c02;
        MASTER      = models[curmodel].master;
        MODELA      = models[curmodel].modela;
        OS01        = models[curmodel].os01;
        compactcmos = models[curmodel].compact;

        model_check();
        
        getcwd(t, 511);
        append_filename(t2, exedir, "roms", 511);
        chdir(t2);
        mem_clearroms();
        if (models[curmodel].romsetup) models[curmodel].romsetup();

        mem_loadroms(models[curmodel].os, models[curmodel].romdir);
//        if (ideenable) loadiderom();
        if (curtube!=-1)
            tubes[curtube].init();
        tube_reset();
        chdir(t);
        
        cmos_load(models[curmodel]);
        if (models[curmodel].swram) mem_fillswram();
}
