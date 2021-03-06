/*************************************************************************

  This file is part of the ParaNut project.

  Copyright (C) 2010-2019 Alexander Bahle <alexander.bahle@hs-augsburg.de>
                          Gundolf Kiefer <gundolf.kiefer@hs-augsburg.de>
      Hochschule Augsburg, University of Applied Sciences

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


#include "intc.h"

#include <assert.h>

#ifndef __SYNTHESIS__
void MIntC::Trace (sc_trace_file *tf, int level) {
    if (!tf || trace_verbose) printf ("\nSignals of Module \"%s\":\n", name ());

    // Ports...
    TRACE (tf, clk);
    TRACE (tf, reset);
    //   to/from EXU ...
    TRACE (tf, ir_request);
    TRACE (tf, ir_ack);
    TRACE (tf, ir_id);
    TRACE (tf, ir_enable);
    //   from external interrupt sources
    TRACE (tf, ex_int);
    //   internal registers...
    TRACE (tf, state);
    TRACE (tf, irq_reg);
}
#endif

void MIntC::OutputMethod () {
    if (state.read () == IntPending) {
        ir_request = 1;
    } else {
        ir_request = 0;
    }

    ir_id = id_reg.read ();
}

void MIntC::TransitionMethod () {
    sc_uint<5> id;
    sc_uint<CFG_NUT_EX_INT> irq_var;
    sc_uint<2> next_state;

    // Read input signals/ports
    irq_var = irq_reg.read ();
    irq_var |= ex_int.read ();
    next_state = state.read ();

    // Preset output signals/ports
    id = 0;
    // Determine ID (lowest wins)
    for (int n = CFG_NUT_EX_INT - 1; n >= 0; n--) {
        if (irq_var[n]) {
            id = (sc_uint<1> (1), sc_uint<4> (n));
        }
    }

    // State machine
    switch (state.read ()) {
    case IntIdle:
        id_reg = id;
        if (irq_var.or_reduce () & ir_enable) {
            next_state = IntPending;
        }
        break;
    case IntPending:
        if (ir_ack) {
            next_state = IntHandled;
        }
        break;
    case IntHandled:
        irq_var[id_reg.read () (3, 0)] = 0;
        next_state = IntIdle;
        break;
    default:
        break;
    }


    // Handle reset (must dominate)...
    if (reset) {
        next_state = IntIdle;
        id = 0;
        irq_var = 0;
    }

    // Write back values to signals ...
    state = next_state;
    irq_reg = irq_var;
}
