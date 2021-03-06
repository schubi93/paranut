/*************************************************************************

  This file is part of the ParaNut project.

  Copyright (C) 2010-2019 Alexander Bahle <alexander.bahle@hs-augsburg.de>
                          Gundolf Kiefer <gundolf.kiefer@hs-augsburg.de>
      Hochschule Augsburg, University of Applied Sciences

  Description:
    This is the top-level component of the ParaNut.
    It contains the following sub-modules:
    - 1 memory unit (MEMU)
    for each CPU:
    - 1 instruction fetch unit (IFU)
    - 1 execution unit (EXU)
    - 1 load-store unit (LSU)

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this 
     list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 *************************************************************************/


#ifndef _PARANUT_
#define _PARANUT_

#include "dm.h"
#include "exu.h"
#include "ifu.h"
#include "intc.h"
#include "jtag_dtm.h"
#include "lsu.h"
#include "memu.h"

#include <systemc.h>


SC_MODULE (MParanut) {
    // SC_HAS_PROCESS(paranut_wb);

    // Ports (WISHBONE master)...
    sc_in<bool>         clk_i; // clock input
    sc_in<bool>         rst_i; // reset

    sc_out<bool>        cyc_o; // cycle valid output
    sc_out<bool>        stb_o; // strobe output
    sc_out<bool>        we_o; // indicates write transfer
    sc_out<sc_uint<CFG_MEMU_BUSIF_WIDTH/8> > sel_o; // byte select outputs
    sc_in<bool>         ack_i; // normal termination
    sc_in<bool>         err_i; // termination w/ error (presently unsupported)
    sc_in<bool>         rty_i; // termination w/ retry (presently unsupported)

    sc_out<TWord>       adr_o; // address bus outputs
    sc_in<sc_uint<CFG_MEMU_BUSIF_WIDTH> >        dat_i; // input data bus
    sc_out<sc_uint<CFG_MEMU_BUSIF_WIDTH> >       dat_o; // output data bus

    // External interrupt sources
    sc_in<sc_uint<CFG_NUT_EX_INT> > ex_int;

    // JTAG TAP
    sc_in<bool>     tck;
    sc_in<bool>     tms;
    sc_in<bool>     tdi;
    sc_out<bool>    tdo;

    // Constructor/Destructor...
    SC_CTOR (MParanut) {
        InitSubmodules ();
        InitInterconnectMethod ();
    }
    ~MParanut () { FreeSubmodules (); }

    // Functions...
    void Trace (sc_trace_file * tf, int levels = 1);
    void DisplayStatistics (const int num = 0) { exu[num]->DisplayStatistics (); }

    bool IsHalted () { return exu[0]->IsHalted (); }

    // Processes...
    void InterconnectMethod ();

    // Submodules...
    MMemu *memu;
    MDebugModule *dm;
    MDtm *dtm;
    MIntC *intc;
    MIfu *ifu[CFG_NUT_CPU_MODE2_CORES];
    MExu *exu[CFG_NUT_CPU_CORES];
    MLsu *lsu[CFG_NUT_CPU_CORES];

    protected:
    // Connecting signals...

    //   MEMU: read ports (rp), write ports (wp)...
    sc_signal<bool> rp_rd[2 * CFG_NUT_CPU_CORES], rp_direct[2 * CFG_NUT_CPU_CORES];
    sc_signal<sc_uint<4> > rp_bsel[2 * CFG_NUT_CPU_CORES];
    sc_signal<bool> rp_ack[2 * CFG_NUT_CPU_CORES];
    sc_signal<TWord> rp_adr[2 * CFG_NUT_CPU_CORES];
    sc_signal<TWord> rp_data[2 * CFG_NUT_CPU_CORES];

    sc_signal<bool> wp_wr[CFG_NUT_CPU_CORES], wp_direct[CFG_NUT_CPU_CORES];
    sc_signal<sc_uint<4> > wp_bsel[CFG_NUT_CPU_CORES];
    sc_signal<bool> wp_ack[CFG_NUT_CPU_CORES];
    sc_signal<bool> wp_lres_scond[CFG_NUT_CPU_CORES];
    sc_signal<bool> wp_scond_ok[CFG_NUT_CPU_CORES];
    sc_signal<bool> wp_writeback[CFG_NUT_CPU_CORES], wp_invalidate[CFG_NUT_CPU_CORES];
    sc_signal<TWord> wp_adr[CFG_NUT_CPU_CORES];
    sc_signal<TWord> wp_data[CFG_NUT_CPU_CORES];
    sc_signal<bool> wb_ack;
    sc_signal<sc_uint<CFG_MEMU_BUSIF_WIDTH> > wb_dat;

    //   IFU ...
    sc_signal<bool> ifu_next[CFG_NUT_CPU_MODE2_CORES], ifu_jump[CFG_NUT_CPU_MODE2_CORES],
        ifu_flush[CFG_NUT_CPU_MODE2_CORES],
        ifu_reset[CFG_NUT_CPU_CORES]; // todo: remove ifu_reset for mode 1 only CoPUs
    sc_signal<TWord> ifu_jump_adr[CFG_NUT_CPU_MODE2_CORES]; // jump adress
    sc_signal<bool> ifu_ir_valid[CFG_NUT_CPU_MODE2_CORES], ifu_npc_valid[CFG_NUT_CPU_MODE2_CORES];
    sc_signal<TWord> ifu_ir[CFG_NUT_CPU_MODE2_CORES], ifu_pc[CFG_NUT_CPU_MODE2_CORES],
        ifu_npc[CFG_NUT_CPU_MODE2_CORES]; // expected to be registered (fast) outputs

    //   LSU ...
    sc_signal<bool> lsu_rd[CFG_NUT_CPU_CORES], lsu_wr[CFG_NUT_CPU_CORES], lsu_flush[CFG_NUT_CPU_CORES];
    sc_signal<bool> lsu_lres_scond[CFG_NUT_CPU_CORES], lsu_cache_writeback[CFG_NUT_CPU_CORES],
        lsu_cache_invalidate[CFG_NUT_CPU_CORES];
    sc_signal<bool> lsu_ack[CFG_NUT_CPU_CORES], lsu_align_err[CFG_NUT_CPU_CORES],
        lsu_scond_ok[CFG_NUT_CPU_CORES];
    sc_signal<sc_uint<2> > lsu_width[CFG_NUT_CPU_CORES]; // "00" = word, "01" = byte, "10" = half word
    sc_signal<bool> lsu_exts[CFG_NUT_CPU_CORES];
    sc_signal<TWord> lsu_adr[CFG_NUT_CPU_CORES];
    sc_signal<TWord> lsu_rdata[CFG_NUT_CPU_CORES];
    sc_signal<TWord> lsu_wdata[CFG_NUT_CPU_CORES];

    // from CePU
    sc_signal<sc_uint<CFG_NUT_CPU_CORES> > m3_pnce;
    sc_signal<sc_uint<CFG_NUT_CPU_CORES> > m3_pnlm;
    sc_signal<TWord> cepu_pnifadr;
    sc_signal<sc_uint<CFG_NUT_CPU_CORES> > m3_pnifadren;
    sc_signal<sc_uint<CFG_NUT_CPU_CORES> > m3_pnxsel;
    // to CePU
    sc_signal<bool> m3_ir_request;
    sc_signal<sc_uint<5> > m3_ir_id;
    sc_signal<sc_uint<CFG_NUT_CPU_CORES> > m3_pnhaltreq;
    sc_signal<sc_uint<CFG_NUT_CPU_CORES> > m3_pnx;
    sc_signal<bool> m3_ex_i;
    // others...
    sc_signal<bool> m3_icache_enable, m3_dcache_enable;
    sc_signal<bool> m3_ir_ack;
    sc_signal<bool> m3_ir_enable;

    //   EXU ...
    sc_signal<bool> exu_haltreq[CFG_NUT_CPU_CORES];
    sc_signal<bool> exu_enable[CFG_NUT_CPU_CORES];
    sc_signal<bool> exu_ifu_reset[CFG_NUT_CPU_CORES];
    sc_signal<bool> exu_linked[CFG_NUT_CPU_CORES];
    sc_signal<bool> exu_ex_o[CFG_NUT_CPU_CORES];
    sc_signal<bool> exu_xsel[CFG_NUT_CPU_CORES];
    sc_signal<sc_uint<5> > exu_cause[CFG_NUT_CPU_CORES - 1];
    sc_signal<sc_uint<32> > exu_epc[CFG_NUT_CPU_CORES - 1];
    sc_signal<bool> exu_sync[CFG_NUT_CPU_CORES];
    sc_signal<bool> exu_dbg_req[CFG_NUT_CPU_CORES];

    //   DM ...
    sc_signal<sc_uint<CFG_NUT_CPU_CORES> > dbg_req;
    sc_signal<bool> dbg_reset;
    // from DM
    sc_signal<bool> dbg_ack_i;
    sc_signal<sc_uint<CFG_MEMU_BUSIF_WIDTH> > dbg_dat_i;

    //   DTM ...
    sc_signal<sc_uint<DTM_ADDR_WIDTH> > dmi_adr; // address output
    sc_signal<sc_uint<32> > dmi_dat_o; // output data
    sc_signal<sc_uint<32> > dmi_dat_i; // input data
    sc_signal<bool> dmi_rd, dmi_wr;


    //   ParaNut reset ...
    sc_signal<bool> reset;

    // Methods...
    void InitSubmodules ();
    void FreeSubmodules ();
    void InitInterconnectMethod ();

    // Helpers...
};


#endif
