//---------------------------------------------------------------------------//
// Optimize_NEON.S Assembly language functions that can be called from C/C++.
//---------------------------------------------------------------------------//

#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits /* mark stack as non-executable */
#endif

#define _PDU_ASM_ 5 // best : 4

.text
.fpu neon
.arch armv7a
.object_arch armv4
.arm

#define _DU_DCTY    0x400
#define _DU_DCTCb   (64*2+_DU_DCTY)
#define _DU_DCTCr   (64*2+_DU_DCTCb)
#define _ZDU        (64*2+_DU_DCTCr)
#define _RLE        (64*2+_ZDU)
#define _pbitstream (128*4+_RLE)
#define _pos        (4+_pbitstream)
#define _wordnew    (4+_pos)
#define _wordpos    (4+_wordnew)
#define _Ximage     (4+_wordpos)
#define _Yimage     (4+_Ximage)
#define _Y_buffer   (4+_Yimage)
#define _Cb_buffer  (4+_Y_buffer)
#define _Cr_buffer  (4+_Cb_buffer)


/* Supplementary macro for setting function attributes */
.macro asm_function fname
#ifdef __APPLE__
    .func _\fname
    .globl _\fname
_\fname:
#else
    .func \fname
    .global \fname
#ifdef __ELF__
    .hidden \fname
    .type \fname, %function
#endif
\fname:
#endif
.endm

.macro asm_endfunc
	bx	lr
	.endfunc
.endm


asm_function rotate90_32x8
	push    {r4,r5};
	vpush   {q4-q7};
	lsl		r2,r2,#2; // pitch * 4
	lsl		r3,r3,#2; // pitch * 4

	mov		r5,#8;    // r5 = 8
loop1:
	mov		r4,r0;	// load 4x8 image
	vld1.8	{q0,q1},[r4],r2;
	vld1.8	{q2,q3},[r4],r2;
	vld1.8	{q4,q5},[r4],r2;
	vld1.8	{q6,q7},[r4],r2;

	lsl     r4,r2,#2;   // r4 = (src_pitch *4)*4
	add		r4,r0,r4;   // src  += pitch
	pld		[r4,#32];
	add		r4,r4,r2;
	pld		[r4,#32];
	add		r4,r4,r2;
	pld		[r4,#32];
	add		r4,r4,r2;
	pld		[r4,#32];

	vzip.32	q1,q3;
	vzip.32	q5,q7;
	vzip.32	q0,q2;
	vzip.32	q4,q6;

	mov		r4,r1;

	vswp	d6,d15;
	vswp	d6,d7;
	vst1.8	{q3},[r4],r3;
	vswp	d14,d15;
	vst1.8	{q7},[r4],r3;

	vswp	d2,d11;
	vswp	d2,d3;
	vst1.8	{q1},[r4],r3;
	vswp	d10,d11;
	vst1.8	{q5},[r4],r3;

	vswp	d4,d13;
	vswp	d4,d5;
	vst1.8	{q2},[r4],r3;
	vswp	d12,d13;
	vst1.8	{q6},[r4],r3;

	vswp	d0,d9;
	vswp	d0,d1;
	vst1.8	{q0},[r4],r3;
	vswp	d8,d9;
	vst1.8	{q4},[r4],r3;

//------------------------
	subs    r5,r5,#1; 	// r5--
	add		r1,r1,#16;  // dest += 4*4
	lsl     r4,r2,#2;   // r4 = (src_pitch *4)*4
	add		r0,r0,r4;   // src  += pitch

	bne     loop1;

	vpop	{q4-q7};
	pop     {r4,r5};

asm_endfunc


//----------------------------------------------
// void writebits(bitstring bs,cinfo_t *cinfo);
//
// r0 = bitstring bs
// r1 = cinfo_t *cinfo
//

asm_function writebits_asm

cinfo   .req r1
wordnew .req r0
wordpos .req r5

	push    {r4,r5};

    and     r2,r0,#0xff;                // r2 = len
    rsb     r3,r2,#16;                  // r3 = 16 - len
    lsl     r3,r0,r3;                   // r3 = value, align to bit 31
    ldr     wordpos,[cinfo,#_wordpos];  // load  wordpos
    add     r4,r5,r2;                   // wordpos + len
    cmp     r4,#32;
    ldr     wordnew,[cinfo,#_wordnew];  // load wordnew
    bgt     .writebits_L1;              // >= 32
    str     r4,[r1,#_wordpos];          // save wordpos
    lsl     wordnew,wordnew,r2;         // wordnew << len
    rsb     r4,r2,#32;                  // r4 = 32 - len
    lsr     r4,r3,r4;                   // value >> 32 - len
    orr     wordnew,wordnew,r4;
    str     wordnew,[r1,#_wordnew];     // save wordnew

	pop     {r4,r5};                    // return
	bx	    lr;
.writebits_L1:
    rsb     r4,wordpos,#32;             // r4: shift = 32 - wordpos
    lsl     wordnew,wordnew,r4;         // wordnew << shift
    lsr     r5,r3,wordpos;              // r5 = value >> wordpos ( shift out )
    orr     r5,r5,wordnew;              // output data

    lsl     r3,r3,r4;                   // value << shift
    sub     r2,r2,r4;                   // len -= shift
    str     r2,[cinfo,#_wordpos];       // save wordpos
    rsb     r2,r2,#32;                  // 32 - wordpos;
    lsr     r3,r3,r2;                   // value >> (32 - wordpos)
    str     r3,[cinfo,#_wordnew];       // wordnew

    ldr     r0,[cinfo,#_pbitstream];    // r0 = pbitstream
    ldr     r2,[cinfo,#_pos];           // r2 = pos

    add     r2,r0,r2;                   // pbitstream + pos

    mov     r4,#0xff000000;

    mov     r3, r5, lsr #24;
    cmp     r4, r3, lsl #24;
    strb    r3,[r2],#1;
    streqb  r4,[r2],#1;

    mov     r3, r5, lsr #16;
    cmp     r4, r3, lsl #24;
    strb    r3,[r2],#1;
    streqb  r4,[r2],#1;

    mov     r3, r5, lsr #8;
    cmp     r4, r3, lsl #24;
    strb    r3,[r2],#1;
    streqb  r4,[r2],#1;

    cmp     r4, r5, lsl #24;
    strb    r5,[r2],#1;
    streqb  r4,[r2],#1;

    sub     r2, r2,r0;           // r2 - r0
    str     r2,[r1,#_pos];       // r2 = pos

	pop     {r4,r5};

    .unreq cinfo
    .unreq wordnew
    .unreq wordpos
asm_endfunc


//---------------------------------------------------------------------
// void load_data_units_from_buffer(cinfo_t *cinfo,WORD xpos,WORD ypos)
asm_function load_data_units_from_buffer_asm

    cinfo    .req r0
    pitch    .req r3
    location .req r4

    push    {r4,r5,r6,r7};

    // location = (ypos*cinfo->Ximage) + xpos;

    ldr     pitch,[cinfo,#_Ximage];
    mul     r4,pitch,r2;            // r4 = pitch*ypos, r2 = ypos
    add     location,r4,r1;         // r4 += xpos     , r1 = xpos

    // prepare pointer
    add     r2,cinfo,#_DU_DCTY;     // r2 -> DU_DCTY
    ldr     r1,[cinfo,#_Y_buffer];  // r1 -> Y_buffer
    add     r1,r1,location;         // r1 += location

    add     r7,cinfo,#_DU_DCTCr;    // r7 -> DU_DCTCr
    ldr     r6,[cinfo,#_Cr_buffer]; // r6 -> Cr_buffer
    add     r6,r6,location;         // r6 += location

    add     r5,cinfo,#_DU_DCTCb;    // r5 -> DU_DCTCb
    ldr     r0,[cinfo,#_Cb_buffer]; // r4 -> Cb_buffer
    add     r4,r0,location;         // r4 += location

    vmov.u16 q0,#0x8000;          // Q1 = 0x8000,...,0x8000

    // Y
    mov     r0,#8;                // r0 = 0
.L1:
    vld1.8   d3,[r1],pitch;       // load 8 bytes data , r1 += pitch
    pld		[r4,#8];
    vzip.8   d2,d3;               // Q0
    vadd.s16 q1,q0;
    vshr.s16 q1,q1,#8;            // shift right 8
    vst1.8   {q1},[r2]!;          // write back and r2 += 16

    vld1.8   d5,[r4],pitch;       // load 8 bytes data , r4 += pitch
    pld		[r6,#8];
    vzip.8   d4,d5;               // Q2
    vadd.s16 q2,q0;
    vshr.s16 q2,q2,#8;            // shift right 8
    vst1.8   {q2},[r5]!;          // write back and r5 += 16

    subs     r0,r0,#1;            // r0--

    vld1.8   d7,[r6],pitch;       // load 8 bytes data , r6 += pitch
    pld		[r1,#8];
    vzip.8   d6,d7;               // Q3
    vadd.s16 q3,q0;
    vshr.s16 q3,q3,#8;            // shift right 8
    vst1.8   {q3},[r7]!;          // write back and r7 += 16

    bne      .L1;

    pop     {r4,r5,r6,r7};
asm_endfunc

#if _PDU_ASM_ == 1

.balign 16
invzigzag:
   .byte  0, 1, 8,16, 9, 2, 3,10;
   .byte 17,24,32,25,18,11, 4, 5;
   .byte 12,19,26,33,40,48,41,34;
   .byte 27,20,13, 6, 7,14,21,28;
   .byte 35,42,49,56,57,50,43,36;
   .byte 29,22,15,23,30,37,44,51;
   .byte 58,59,52,45,38,31,39,46;
   .byte 53,60,61,54,47,55,62,63;
//-------------------------------------------------------------------------
//   void process_DU(cinfo_t *cinfo,INT16 *DU,SWORD *DC,HuffmanTable_t* HT)
//   r0 -> cinfo
//   r1 -> DU
//   r2 -> DC
//   r3 -> HT
#define  _cinfo 0
#define  _DU    4

asm_function Process_DU_asm
        push    {r4,r5,r6,r7};
        sub     sp,sp,#8;
        str     r0,[sp,#_cinfo];
        str     r1,[sp,#_DU];

        ldrsh   r5,[r2];  // r5 = *DC
        ldrsh   r4,[r1];  // r4 = DU[0]
        sub     r5,r4,r5; // r5 = DU[0] - *DC
        strh    r4,[r2];  // *DC = DU[0]

        add     r0,r0,#_RLE; // r0 -> RLE
        pld     [r1,#128];

// Encode DC
        cmp     r5,#0;
        bne     .EncodeDC.L1;
        ldr     r4,[r3];            // r0 = HTDC[0]
        str     r4,[r0],#4;         // write_rle
        b       .EncodeDC.L2;
.EncodeDC.L1:
        add     r2,r5,r5, asr #31;  // r2 : value = Diff + mask
        eor     r4,r2,r5, asr #31;  // r4 = value ^ mask
        clz     r4,r4;
        rsb     r5,r4,#32;          // r5 = cat
        ldr     r4,[r3,r5,lsl #2];  // r4 = HTDC[cat]
        orr     r5,r5,r2,lsl #16;   // r5 = value << 16 | cat
        stmia   r0!, {r4,r5};
.EncodeDC.L2:

        str     r0,[sp,#_DU];    // keep write RLE address here

// ZigZag
        ldr     r0,[sp,#_cinfo];
        add     r0,r0,#_ZDU;     // r0 -> ZDU, r1->DU
        adr     r2,invzigzag;    // r2 -> invzigzag

        pld     [r2,#64];

        add     r4,r0,#128;      // r4 ->end of invzigzag
        mov     r7,r0;

.InvZigZag.L1:
        ldrh    r5,[r2],#2;     // load zigzag

        lsl     r6,r5,#24;
        lsr     r6,r6,#23;      // r6 -> zigzag[0]
        ldrh    r6,[r1,r6];     // load src[0]

        lsr     r5,r5,#7;       // r5 -> zigzag[1]
        ldrh    r5,[r1,r5];     // load src[1]

        orrs    r6,r6,r5,lsl #16;
        addne   r7,r0,#0;

        str     r6,[r0],#4;

        ///////////////////////////////////////
        ldrh    r5,[r2],#2;     // load zigzag

        lsl     r6,r5,#24;
        lsr     r6,r6,#23;      // r6 -> zigzag[0]
        ldrh    r6,[r1,r6];     // load src[0]

        lsr     r5,r5,#7;       // r5 -> zigzag[1]
        ldrh    r5,[r1,r5];     // load src[1]

        orrs    r6,r6,r5,lsl #16;
        addne   r7,r0,#0;

        str     r6,[r0],#4;
        ///////////////////////////////////////
        cmp     r4,r0;
        bne     .InvZigZag.L1;

// check exact point of end0pos
        ldrh    r6,[r7,#2];
        cmp     r6,#0;
        addne   r7,r7,#2;
// ----------------------------
        sub     r0,#128;
        pld     [r0,#128];          // preload ZDU

        add     r3,#(12*4);         // r3->HTAC, r0 -> ZDU, r7 -> end0pos, r1 : nrzeros
        pld     [r3,#1024];         // preload HTAC

        ldr     r2,[sp,#_DU];       // r2->RLE
        eor     r1,r1,r1;           // r1 = 0

// Encode AC
        cmp     r0,r7;              // end0pos = 0
        add     r7,r7,#2;           // r7 -> end0pos+1
        add     r0,r0,#2;           // r0 -> ZDU[1]
        bne     .EncodeAC_L1;
        ldr     r4,[r3];            // r4 = HTAC[0x00] ( EOB )
        str     r4,[r2],#4;         // write_rle
        b       .EncodeAC_end;

.EncodeAC_L1:
        ldrsh   r4,[r0],#2;         // r4 = ZDU[i]
        cmp     r4,#0;
        bne     .EncodeAC_L2;
        cmp     r0,r7;              // end
        add     r1,r1,#1;           // nrzeros++;
        bne     .EncodeAC_L1;
        b       .EncodeAC_L3;

.M16Zeroes_loop:
        ldr     r5,[r3,#(0xF0*4)];  // r5 = HTAC[0xF0] ( M16zeroes )
        str     r5,[r2],#4;         // write_rle
        sub     r1,r1,#16;          // nrzeroes -= 16
.EncodeAC_L2:
        cmp     r1,#16;
        bge     .M16Zeroes_loop;

        add     r5,r4,r4, asr #31;  // r5 : value = ZDU + mask
        eor     r4,r5,r4, asr #31;  // r4 = value ^ mask
        clz     r4,r4;
        rsb     r6,r4,#32;          // r6 = cat
        orr     r5,r6,r5,lsl #16;   // r5 = value<<16 | cat
        add     r6,r6,r1,lsl #4;    // r6 = nrzeros*16 + cat
        ldr     r4,[r3,r6,lsl #2];  // r4 = HTAC[nrzeros*16 + cat]
        stmia   r2!, {r4,r5};       // write_rle

        cmp     r0,r7;              // compare end
        eor     r1,r1,r1;           // nrzeros = 0
        bne     .EncodeAC_L1;

.EncodeAC_L3:
        ldr     r0,[sp,#_cinfo];
        add     r0,r0,#(_ZDU+64*2);
        cmp     r0,r7;              // r7 -> last ZDU : last is not zero ( check r7 not ZDU[64] )
        beq     .EncodeAC_end;
        ldr     r4,[r3];            // r4 = HTAC[0x00] ( EOB )
        str     r4,[r2],#4;         // write_rle

.EncodeAC_end:
        // Write block
        ldr     r0,[sp,#_cinfo];
        add     r0,r0,#_RLE;
        sub     r0,r2,r0;
        lsr     r0,r0,#2; // return nRLE

        add     sp,sp,#8;
        pop     {r4,r5,r6,r7};
asm_endfunc

#endif

#if _PDU_ASM_ == 2

.balign 16
invzigzag:
   .byte  0, 1, 8,16, 9, 2, 3,10;
   .byte 17,24,32,25,18,11, 4, 5;
   .byte 12,19,26,33,40,48,41,34;
   .byte 27,20,13, 6, 7,14,21,28;
   .byte 35,42,49,56,57,50,43,36;
   .byte 29,22,15,23,30,37,44,51;
   .byte 58,59,52,45,38,31,39,46;
   .byte 53,60,61,54,47,55,62,63;
// Merge zigzag and RLE , performance poor than previous one !!
//-------------------------------------------------------------------------
//   void process_DU(cinfo_t *cinfo,INT16 *DU,SWORD *DC,HuffmanTable_t* HT)
//   r0 -> cinfo
//   r1 -> DU
//   r2 -> DC
//   r3 -> HT
#define  _cinfo 0
#define  _DU    4

asm_function Process_DU_asm
        push    {r4,r5,r6,r7,r8};
        sub     sp,sp,#8;
        str     r0,[sp,#_cinfo];
        str     r1,[sp,#_DU];

        ldrsh   r5,[r2];  // r5 = *DC
        ldrsh   r4,[r1];  // r4 = DU[0]
        sub     r5,r4,r5; // r5 = DU[0] - *DC
        strh    r4,[r2];  // *DC = DU[0]

        add     r0,r0,#_RLE; // r0 -> RLE

// Encode DC
        cmp     r5,#0;
        bne     .EncodeDC.L1;
        ldr     r4,[r3];            // r0 = HTDC[0]
        str     r4,[r0],#4;         // write_rle
        b       .EncodeDC.L2;
.EncodeDC.L1:
        add     r2,r5,r5, asr #31;  // r2 : value = Diff + mask
        eor     r4,r2,r5, asr #31;  // r4 = value ^ mask
        lsl     r5,r2,#16;          // r6 = value << 16
        clz     r4,r4;
        rsb     r2,r4,#32;          // r2 = cat
        ldr     r4,[r3,r2,lsl #2];  // r4 = HTDC[cat]
        orr     r5,r5,r2;
        stmia   r0!, {r4,r5};
.EncodeDC.L2:

//        str     r0,[sp,#_DU];    // keep write RLE address here

// ZigZag & Encode AC
        adr     r2,invzigzag;    // r2 -> invzigzag

        add     r8,r2,#64;
        pld     [r1,#128];
        pld     [r2,#64];

        add     r3,#(12*4);         // r3->HTAC, r0 -> ZDU, r7 -> end0pos, r1 : nrzeros
        pld     [r3,#1024];         // preload HTAC

        add     r2,r2,#1;
        eor     r6,r6,r6;        // r6 : nrzeros = 0

.InvZigZag.L1:
        cmp     r8,r2;
        beq     .EncodeAC_exit;
        ldrb    r5,[r2],#1;      // load zigzag
        lsl     r5,r5,#1;        // r5 -> zigzag[1]
        ldrsh   r5,[r1,r5];      // load src[1]

        cmp     r5,#00;
        bne     .M16zeroes_L;
        add     r6,r6,#1;        // nrzeros ++
        b       .InvZigZag.L1;
// not zero
.M16zeroes_L:
        cmp     r6,#16;
        blt     .EncodeAC_L1;
        ldr     r4,[r3,#(0xF0*4)];  // r6 = HTAC[0xF0] ( M16zeroes )
        str     r4,[r0],#4;         // write_rle
        sub     r6,r6,#16;
        b       .M16zeroes_L;
.EncodeAC_L1:
        add     r7,r5,r5, asr #31;  // r7 : value = ZDU + mask
        eor     r4,r7,r5, asr #31;  // r4 = value ^ mask
        lsl     r5,r7,#16;          // r5 = value << 16
        clz     r7,r4;
        rsb     r4,r7,#32;          // r4 = cat
        orr     r5,r5,r4;           // r5 = value<<16 | cat
        add     r6,r4,r6,lsl #4;    // r1 = nrzeros*16 + cat
        ldr     r4,[r3,r6,lsl #2];  // r4 = HTAC[nrzeros*16 + cat]
        stmia   r0!, {r4,r5};       // write_rle
        eor     r6,r6,r6;           // nrzeros = 0;
        b       .InvZigZag.L1;
.EncodeAC_exit:
        ldrh    r5,[r1,#63*2];      // get last elements of DU
        cmp     r5,#0
        bne     .EncodeAC_end;
        ldr     r4,[r3];            // r4 = HTAC[0x00] ( EOB )
        str     r4,[r0],#4;         // write_rle
.EncodeAC_end:

        // Write block
        ldr     r2,[sp,#_cinfo];
        add     r2,r2,#_RLE;
        sub     r0,r0,r2;
        lsr     r0,r0,#2;

        add     sp,sp,#8;
        pop     {r4,r5,r6,r7,r8};
asm_endfunc
#endif

#if _PDU_ASM_ == 3

.balign 16
zigzag:
	.byte     0, 1, 5, 6,14,15,27,28;
	.byte	  2, 4, 7,13,16,26,29,42;
	.byte	  3, 8,12,17,25,30,41,43;
	.byte	  9,11,18,24,31,40,44,53;
	.byte	 10,19,23,32,39,45,52,54;
	.byte	 20,22,33,38,46,51,55,60;
	.byte	 21,34,37,47,50,56,59,61;
	.byte	 35,36,48,49,57,58,62,63;

// Using neon vclz for value and cat caculate, performance poor than others
//-------------------------------------------------------------------------
//   void process_DU(cinfo_t *cinfo,INT16 *DU,SWORD *DC,HuffmanTable_t* HT)
//   r0 -> cinfo
//   r1 -> DU
//   r2 -> DC
//   r3 -> HT
#define  _cinfo 0
#define  _DU    4
#define  _DATA  16

asm_function Process_DU_asm
        push    {r4,r5,r6,r7,r8};
        vpush   {q4};
        sub     sp,sp,#272;     // 16 + 256
        str     r0,[sp,#_cinfo];// Save cinfo in memory
        str     r1,[sp,#_DU];   // Save DU in memory

        ldrsh   r5,[r2];        // r5 = *DC
        ldrsh   r4,[r1];        // r4 = DU[0]
        sub     r5,r4,r5;       // r5 = DU[0] - *DC
        strh    r4,[r2];        // *DC = DU[0]
        strh    r5,[r1];        // Save Diff in DU[0]

        add     r0,r0,#_ZDU;    // r0 -> ZDU
        adr     r2,zigzag;      // r2 -> zigzag

        // ZigZag
        // r0 -> ZDU, r1 -> DU, r2 -> zigzag
        add     r7,r2,#64;
.ZigZag_L1:
        //----------------------------------------
        ldr     r4,[r1],#4;     // load src
        ldrh    r5,[r2],#2;     // load zigzag
        lsl     r6,r5,#24;
        lsr     r6,r6,#23;      // r6 -> zigzag[0]
        lsr     r5,r5,#7;       // r5 -> zigzag[1]
        strh    r4,[r0,r6];     // store dest[0]
        lsr     r4,r4,#16;
        strh    r4,[r0,r5];     // store dest[1]
        //----------------------------------------
        cmp     r7,r2;
        bne     .ZigZag_L1;

        pld     [r0,#128];      // r0->ZDU

        // Compute value and cat by NEON
        vmov.I16 q4,#15;        // cls ( counting leading sign )
        add      r4,r0,#128;
//        ldr      r5,[sp,#_cinfo];
//        add      r5,r5,#_RLE;
        add      r5,sp,#_DATA;   // r5 -> temp[64]
.Value_L1:
        vld1.8   {q1},[r0]!;     // load ZDU[0]~ZDU[7] to q1
        vshr.S16 q0,q1,#15;      // q0 = ZDU >> 15
        vadd.S16 q1,q1,q0;       // q1 = value
        vcls.S16 q0,q1;          // q0 = cls(value ^ mask)
        vsub.S16 q0,q4,q0;       // q0 = 15 - q1
        vst2.16  {q0,q1},[r5]!;

        vld1.8   {q3},[r0]!;     // load ZDU[8]~ZDU[15] to q3
        vshr.S16 q2,q3,#15;      // q2 = ZDU >> 15
        vadd.S16 q3,q3,q2;       // q3 = value
        vcls.S16 q2,q3;          // q2 = cls(value ^ mask)
        vsub.S16 q2,q4,q2;       // q2 = 15 - q3
        vst2.16  {q2,q3},[r5]!;

        cmp     r0,r4;
        bne     .Value_L1;

        ldr     r2,[sp,#_cinfo];
        add     r2,r2,#_RLE;    // r2 -> RLE, r3 -> HTDC
        add     r0,sp,#_DATA;   // r0 -> temp[64]

        // Encode DC
        ldr     r5,[r0],#4;     // r5 = {value,cat}
        cmp     r5,#0;
        bne     .EncodeDC_L1;
        ldr     r4,[r3];        // r4 = HTDC[0]
        str     r4,[r2],#4;     // write_rle
        b       .EncodeDC_L2;
.EncodeDC_L1:
        and     r6,r5,#0xFF;
        ldr     r4,[r3,r6,lsl #2]; // r4 = HTDC[cat]
        stmia   r2!,{r4,r5};
.EncodeDC_L2:

        // Encode AC
        add     r3,r3,#(12*4);   // r3 -> HTAC, r0 -> temp[i]
        pld     [r3,#1024];      // preload HTAC

        add     r7,r0,#(63*4);   // r7 -> end of temp
        eor     r6,r6,r6;        // r6 = nrzeros

.EncodeAC_Loop:

        ldr     r5,[r0],#4;      // r5 = {value,cat}
        cmp     r5,#0;
        bne     .EncodeAC_L1;
        cmp     r7,r0;
        add     r6,r6,#1;        // nrzeros++
        beq     .EncodeAC_L2;
        b       .EncodeAC_Loop;
.M16Zeros_Loop:
        ldr     r4,[r3,#(0xF0*4)];  // r4 = HTAC[0xF0] ( M16zeroes )
        str     r4,[r2],#4;         // write_rle
        sub     r6,r6,#16;
.EncodeAC_L1:
        cmp     r6,#16
        bge     .M16Zeros_Loop;

        and     r4,r5,#0xFF;        // r4 = cat
        add     r6,r4,r6,lsl #4;    // nrzeros*16+cat
        ldr     r4,[r3,r6,lsl #2];  // r4 = HTAC[nrzeros*16+cat]
        stmia   r2!, {r4,r5};
        cmp     r7,r0;
        eor     r6,r6,r6;           // nrzeros = 0
        bne     .EncodeAC_Loop;
.EncodeAC_L2:
        cmp     r5,#0;
        bne     .EncodeAC_end;
        ldr     r4,[r3];            // r4 = HTAC[0x00] ( EOB )
        str     r4,[r2],#4;         // write_rle
.EncodeAC_end:

        ldr     r0,[sp,#_cinfo];
        add     r0,r0,#_RLE;        // r0 -> RLE

        sub     r0,r2,r0;
        lsr     r0,r0,#2;

        add     sp,sp,#272; // 16 + 256
        vpop    {q4};
        pop     {r4,r5,r6,r7,r8};
asm_endfunc

#endif


#if _PDU_ASM_ == 4

.balign 16
invzigzag:
   .byte  0, 1, 8,16, 9, 2, 3,10;
   .byte 17,24,32,25,18,11, 4, 5;
   .byte 12,19,26,33,40,48,41,34;
   .byte 27,20,13, 6, 7,14,21,28;
   .byte 35,42,49,56,57,50,43,36;
   .byte 29,22,15,23,30,37,44,51;
   .byte 58,59,52,45,38,31,39,46;
   .byte 53,60,61,54,47,55,62,63;

// Using neon vclz for value and cat caculate with end0pos
//-------------------------------------------------------------------------
//   void process_DU(cinfo_t *cinfo,INT16 *DU,SWORD *DC,HuffmanTable_t* HT)
//   r0 -> cinfo
//   r1 -> DU
//   r2 -> DC
//   r3 -> HT
#define  _cinfo 0
#define  _DU    4
#define  _DATA  16

asm_function Process_DU_asm
        push    {r4,r5,r6,r7,r8};
        vpush   {q4};
        sub     sp,sp,#272;     // 16 + 256
        str     r0,[sp,#_cinfo];// Save cinfo in memory
        str     r1,[sp,#_DU];   // Save DU in memory

        ldrsh   r5,[r2];        // r5 = *DC
        ldrsh   r4,[r1];        // r4 = DU[0]
        sub     r5,r4,r5;       // r5 = DU[0] - *DC
        strh    r4,[r2];        // *DC = DU[0]
        strh    r5,[r1];        // Save Diff in DU[0]

        add     r0,r0,#_ZDU;    // r0 -> ZDU
        adr     r2,invzigzag;   // r2 -> invzigzag

        // ZigZag
        // r0 -> ZDU, r1 -> DU, r2 -> invzigzag

        pld     [r1,#128];
        pld     [r2,#64];

        add     r4,r0,#128;      // r4 ->end of invzigzag
        mov     r7,r0;
.InvZigZag_L1:
        ldrh    r5,[r2],#2;     // load zigzag

        lsl     r6,r5,#24;
        lsr     r6,r6,#23;      // r6 -> zigzag[0]
        lsr     r5,r5,#7;       // r5 -> zigzag[1]

        ldrh    r6,[r1,r6];     // load src[0]
        ldrh    r5,[r1,r5];     // load src[1]

        orrs    r6,r6,r5, lsl #16;
        addne   r7,r0,#0;
        str     r6,[r0],#4;
        /////////////////////////////////////////
        ldrh    r5,[r2],#2;     // load zigzag

        lsl     r6,r5,#24;
        lsr     r6,r6,#23;      // r6 -> zigzag[0]
        lsr     r5,r5,#7;       // r5 -> zigzag[1]

        ldrh    r6,[r1,r6];     // load src[0]
        ldrh    r5,[r1,r5];     // load src[1]

        orrs    r6,r6,r5, lsl #16;
        addne   r7,r0,#0;
        str     r6,[r0],#4;
        /////////////////////////////////////////
        cmp     r4,r0;
        bne     .InvZigZag_L1;
// check exact point of end0pos
        ldrh    r6,[r7,#2];
        cmp     r6,#0;
        addne   r7,r7,#2;
// ----------------------------
        add     r7,r7,#2;

        sub     r0,r0,#128;     // rewind r0 to ZDU
        pld     [r0,#128];      // r0->ZDU

        // Compute value and cat by NEON , vcls : vector counting leading sign
        vmov.I16 q4,#15;
        add      r5,sp,#_DATA;   // r5 -> temp[64]
.Value_L1:
        vld1.8   {q1},[r0]!;     // load ZDU[0]~ZDU[7] to q1
        vshr.S16 q0,q1,#15;      // q0 = ZDU >> 15
        vadd.S16 q1,q1,q0;       // q1 = value + mask
        vcls.S16 q0,q1;          // q0 = cls(value)
        vsub.S16 q0,q4,q0;       // q0 = 15 - q1
        vst2.16  {q0,q1},[r5]!;

        vld1.8   {q3},[r0]!;     // load ZDU[8]~ZDU[15] to q3
        vshr.S16 q2,q3,#15;      // q2 = ZDU >> 15
        vadd.S16 q3,q3,q2;       // q3 = value
        vcls.S16 q2,q3;          // q2 = cls(value ^ mask)
        vsub.S16 q2,q4,q2;       // q2 = 15 - q3
        vst2.16  {q2,q3},[r5]!;

        cmp     r0,r7;
        blt     .Value_L1;

        ldr     r2,[sp,#_cinfo];
        add     r0,r2,#_ZDU;
        sub     r7,r7,r0;       // number of elements in ZDU[]
        add     r2,r2,#_RLE;    // r2 -> RLE, r3 -> HTDC

        add     r0,sp,#_DATA;   // r0 -> temp[64]
        pld     [r0,#256];

        add     r7,r0,r7, lsl #1;

        // Encode DC
        ldr     r5,[r0],#4;     // r5 = {value,cat}
        cmp     r5,#0;
        bne     .EncodeDC_L1;
        ldr     r4,[r3];        // r4 = HTDC[0]
        str     r4,[r2],#4;     // write_rle
        b       .EncodeDC_L2;
.EncodeDC_L1:
        and     r6,r5,#0xFF;
        ldr     r4,[r3,r6,lsl #2]; // r4 = HTDC[cat]
        stmia   r2!,{r4,r5};
.EncodeDC_L2:

        // Encode AC
        add     r3,r3,#(12*4);   // r3 -> HTAC, r0 -> temp[i]
        pld     [r3,#1024];      // preload HTAC

        eor     r6,r6,r6;        // r6 = nrzeros
        cmp     r7,r0;           // all AC is zero
        ble    .EncodeAC_L2;     // end when r7 <= r0
.EncodeAC_Loop:
        ldr     r5,[r0],#4;      // r5 = {value,cat}
        cmp     r5,#0;
        bne     .EncodeAC_L1;
        cmp     r7,r0;
        add     r6,r6,#1;        // nrzeros++
        beq     .EncodeAC_L2;
        b       .EncodeAC_Loop;

.M16Zeros_Loop:
        ldr     r4,[r3,#(0xF0*4)];  // r4 = HTAC[0xF0] ( M16zeroes )
        str     r4,[r2],#4;         // write_rle
        sub     r6,r6,#16;
.EncodeAC_L1:
        cmp     r6,#16
        bge     .M16Zeros_Loop;

        and     r4,r5,#0xFF;        // r4 = cat
        add     r6,r4,r6,lsl #4;    // nrzeros*16+cat
        ldr     r4,[r3,r6,lsl #2];  // r4 = HTAC[nrzeros*16+cat]
        stmia   r2!, {r4,r5};
        cmp     r0,r7;
        eor     r6,r6,r6;           // nrzeros = 0
        bne     .EncodeAC_Loop;
.EncodeAC_L2: // check last EOB
        add     r0,sp,#(_DATA+64*4);// r0 -> temp[64]
        cmp     r7,r0;
        beq     .EncodeAC_end;
        ldr     r4,[r3];            // r4 = HTAC[0x00] ( EOB )
        str     r4,[r2],#4;         // write_rle
.EncodeAC_end:

        ldr     r0,[sp,#_cinfo];
        add     r0,r0,#_RLE;        // r0 -> RLE

        sub     r0,r2,r0;
        lsr     r0,r0,#2;           // return nRLE

        add     sp,sp,#272; // 16 + 256
        vpop    {q4};
        pop     {r4,r5,r6,r7,r8};
asm_endfunc

#endif

#if _PDU_ASM_ == 5

.balign 16
invzigzag:
   .byte  0, 1, 8,16, 9, 2, 3,10;
   .byte 17,24,32,25,18,11, 4, 5;
   .byte 12,19,26,33,40,48,41,34;
   .byte 27,20,13, 6, 7,14,21,28;
   .byte 35,42,49,56,57,50,43,36;
   .byte 29,22,15,23,30,37,44,51;
   .byte 58,59,52,45,38,31,39,46;
   .byte 53,60,61,54,47,55,62,63;

// Using neon vclz for value and cat caculate with end0pos
//-------------------------------------------------------------------------
//   void process_DU(cinfo_t *cinfo,INT16 *DU,SWORD *DC,HuffmanTable_t* HT)
//   r0 -> cinfo
//   r1 -> DU
//   r2 -> DC
//   r3 -> HT
#define  _cinfo 0
#define  _DU    4
#define  _DATA  16

asm_function Process_DU_asm
        push    {r4,r5,r6,r7,r8};
        vpush   {q4};
        sub     sp,sp,#272;     // 16 + 256
        str     r0,[sp,#_cinfo];// Save cinfo in memory
        str     r1,[sp,#_DU];   // Save DU in memory

        ldrsh   r5,[r2];        // r5 = *DC
        ldrsh   r4,[r1];        // r4 = DU[0]
        sub     r5,r4,r5;       // r5 = DU[0] - *DC
        strh    r4,[r2];        // *DC = DU[0]
        strh    r5,[r1];        // Save Diff in DU[0]

        add     r0,r0,#_ZDU;    // r0 -> ZDU
        adr     r2,invzigzag;   // r2 -> invzigzag

        // ZigZag
        // r0 -> ZDU, r1 -> DU, r2 -> invzigzag

        pld     [r1,#128];
        pld     [r2,#64];

        add     r4,r0,#128;      // r4 ->end of invzigzag
        mov     r7,r0;
.InvZigZag_L1:
        /////////////////////////////////////////
        ldr     r8,[r2],#4;     // load zigzag

        lsl     r6,r8,#24;
        lsl     r5,r8,#16;
        lsr     r6,r6,#23;      // r6 -> zigzag[0]
        lsr     r5,r5,#23;      // r5 -> zigzag[1]

        ldrh    r6,[r1,r6];     // load src[0]
        ldrh    r5,[r1,r5];     // load src[1]

        orrs    r6,r6,r5, lsl #16;
        addne   r7,r0,#0;
        str     r6,[r0],#4;
        /////////////////////////////////////////
        lsl     r6,r8,#8;
        lsr     r6,r6,#23;      // r6 -> zigzag[2]
        lsr     r5,r8,#23;      // r5 -> zigzag[3]

        ldrh    r6,[r1,r6];     // load src[2]
        ldrh    r5,[r1,r5];     // load src[3]

        orrs    r6,r6,r5, lsl #16;
        addne   r7,r0,#0;
        str     r6,[r0],#4;
        /////////////////////////////////////////
        cmp     r4,r0;
        bne     .InvZigZag_L1;
// check exact point of end0pos
        ldrh    r6,[r7,#2];
        cmp     r6,#0;
        add     r7,r7,#2;
        addne   r7,r7,#2;
// ----------------------------
        sub     r0,r0,#128;     // rewind r0 to ZDU
        pld     [r0,#128];      // r0 -> ZDU
        mov     r1,r0;          // r1 -> ZDU
        add     r2,r0,#(_RLE - _ZDU);    // r2 -> RLE, r3 -> HTDC

        // Compute value and cat by NEON , vcls : vector counting leading sign
        vmov.I16 q4,#15;
        add      r8,sp,#_DATA;   // r8 -> temp[64]
.Value_L1:
        vld1.8   {q1,q2},[r0]!;  // load ZDU[0]~ZDU[15] to q1,q2
        vshr.S16 q0,q1,#15;      // q0 = ZDU >> 15
        vadd.S16 q1,q1,q0;       // q1 = value + mask
        vcls.S16 q0,q1;          // q0 = cls(value)
        vsub.S16 q0,q4,q0;       // q0 = 15 - q1
        vst2.16  {q0,q1},[r8]!;
        vshr.S16 q3,q2,#15;      // q3 = ZDU >> 15
        vadd.S16 q3,q2,q3;       // q3 = value + mask
        vcls.S16 q2,q3;          // q2 = cls(value)
        vsub.S16 q2,q4,q2;       // q2 = 15 - q3
        vst2.16  {q2,q3},[r8]!;

        cmp     r0,r7;
        blt     .Value_L1;

        sub     r7,r7,r1;       // number of elements*2 in ZDU[]

        add     r1,sp,#_DATA;   // r1 -> temp[64]
        pld     [r1,#256];

        add     r7,r1,r7, lsl #1;

        // Encode DC
        ldr     r5,[r1],#4;     // r5 = {value,cat}
        cmp     r5,#0;
        bne     .EncodeDC_L1;
        ldr     r4,[r3];        // r4 = HTDC[0]
        str     r4,[r2],#4;     // write_rle
        b       .EncodeDC_L2;
.EncodeDC_L1:
        and     r6,r5,#0xFF;
        ldr     r4,[r3,r6,lsl #2]; // r4 = HTDC[cat]
        stmia   r2!,{r4,r5};
.EncodeDC_L2:

        // Encode AC
        add     r3,r3,#(12*4);   // r3 -> HTAC, r0 -> temp[i]
        pld     [r3,#1024];      // preload HTAC

        eor     r6,r6,r6;        // r6 = nrzeros
        cmp     r7,r1;           // all AC is zero
        ble    .EncodeAC_L2;     // end when r7 <= r0
.EncodeAC_Loop:
        ldr     r5,[r1],#4;      // r5 = {value,cat}
        cmp     r5,#0;
        bne     .EncodeAC_L1;
        cmp     r7,r1;
        add     r6,r6,#1;        // nrzeros++
        beq     .EncodeAC_L2;
        b       .EncodeAC_Loop;

.M16Zeros_Loop:
        ldr     r4,[r3,#(0xF0*4)];  // r4 = HTAC[0xF0] ( M16zeroes )
        str     r4,[r2],#4;         // write_rle
        sub     r6,r6,#16;
.EncodeAC_L1:
        cmp     r6,#16
        bge     .M16Zeros_Loop;

        and     r4,r5,#0xFF;        // r4 = cat
        add     r6,r4,r6,lsl #4;    // nrzeros*16+cat
        ldr     r4,[r3,r6,lsl #2];  // r4 = HTAC[nrzeros*16+cat]
        stmia   r2!, {r4,r5};
        cmp     r1,r7;
        eor     r6,r6,r6;           // nrzeros = 0
        bne     .EncodeAC_Loop;
.EncodeAC_L2: // check last EOB
        add     r0,sp,#(_DATA+64*4);// r0 -> temp[64]
        cmp     r7,r0;
        beq     .EncodeAC_end;
        ldr     r4,[r3];            // r4 = HTAC[0x00] ( EOB )
        str     r4,[r2],#4;         // write_rle
.EncodeAC_end:

        ldr     r0,[sp,#_cinfo];
        add     r0,r0,#_RLE;        // r0 -> RLE

        sub     r0,r2,r0;
        lsr     r0,r0,#2;           // return nRLE

        add     sp,sp,#272; // 16 + 256
        vpop    {q4};
        pop     {r4,r5,r6,r7,r8};
asm_endfunc

#endif