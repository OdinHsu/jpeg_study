;
; jquanti.asm - sample data conversion and quantization (SSE2)
;
; Copyright 2009 Pierre Ossman <ossman@cendio.se> for Cendio AB
;
; Based on
; x86 SIMD extension for IJG JPEG library
; Copyright (C) 1999-2006, MIYASAKA Masaru.
; For conditions of distribution and use, see copyright notice in jsimdext.inc
;
; This file should be assembled with NASM (Netwide Assembler),
; can *not* be assembled with Microsoft's MASM or any compatible
; assembler (including Borland's Turbo Assembler).
; NASM is available from http://nasm.sourceforge.net/ or
; http://sourceforge.net/project/showfiles.php?group_id=6208
;
; [TAB8]

%include "jsimdext.inc"
%include "jdct.inc"

; --------------------------------------------------------------------------
        SECTION SEG_TEXT
        BITS    32

; For some reason, the OS X linker does not honor the request to align the
; segment unless we do this.
        align   16
