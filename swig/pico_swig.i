/* -*- c++ -*- */

#define PICO_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "pico_swig_doc.i"

%{
#include "pico/pico_src.h"
#include "pico/pico_sink.h"
%}

%include "pico/pico_src.h"
GR_SWIG_BLOCK_MAGIC2(pico, pico_src);
%include "pico/pico_sink.h"
GR_SWIG_BLOCK_MAGIC2(pico, pico_sink);
