;
; jquanti.asm - sample data conversion and quantization (64-bit SSE2)
;
; Copyright 2009 Pierre Ossman <ossman@cendio.se> for Cendio AB
; Copyright 2009 D. R. Commander
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

section .data
zigzag  db      0, 1, 5, 6,14,15,27,28
		db 		2, 4, 7,13,16,26,29,42
		db		3, 8,12,17,25,30,41,43
		db		9,11,18,24,31,40,44,53
		db		10,19,23,32,39,45,52,54
		db		20,22,33,38,46,51,55,60
		db		21,34,37,47,50,56,59,61
		db		35,36,48,49,57,58,62,63 
		
; --------------------------------------------------------------------------
        SECTION SEG_TEXT
        BITS    64

%define _pbitstream 0x0600
%define _pos        (8+_pbitstream)
%define _wordnew    (8+_pos)
%define _wordpos    (8+_wordnew)
%define _DCTCb      (_pbitstream - 0x180)
%define _DCTCr      (_pbitstream - 0x100)
%define _ZDU        (_pbitstream - 0x80)
%define _RLE		(4+_wordpos)

; --------------------------------------------------------------------------
; void process_DU(cinfo_t *cinfo,INT16 *DU,SWORD *DC,HuffmanTable_t* HT)
;
;
	align	16
	global	EXTN(process_DU)	
%ifdef MACHO	

%define cinfo    rdi
%define DU       rdx
%else
%define cinfo    rdi
%define DU       rdx
%endif

%define DC   	 r8
%define HT 		 r9
%define HTDC 	 HT 
%define HTAC 	 (HT + 48)

EXTN(process_DU):
	push rbp
	push r13

	push rdi
    push rbx  
	push rsi
	push r12
	push r14
		
	push r15
	

%ifdef MACHO  
    push    r10
    push    r11   
	mov		r8,rdx;
	mov		r9,rcx;
	mov		rdx,rsi;  // rdi : cinfo
%else
  	mov 	rdi,rcx	
%endif

;	for(i=0;i<64;i+=4) {
;       cinfo->ZDU[zigzag[i]]   = DU[i];   cinfo->ZDU[zigzag[i+1]] = DU[i+1];
;       cinfo->ZDU[zigzag[i+2]] = DU[i+2]; cinfo->ZDU[zigzag[i+3]] = DU[i+3];
;   }

 	mov 	rsi,zigzag 
	lea 	rax,[DU]
	mov 	bl,10h
	lea r12,[cinfo+_ZDU]	  
huff_DCfun1:
	movzx 	r10,byte  [rsi] 
	mov 	r14,qword [rax]	
	add 	rsi,4  
	mov 	word [r12+r10*2],r14w  
	movzx 	r11,byte  [rsi-3] 
	shr 	r14,16	
	mov 	word [r12+r11*2],r14w
    movzx 	r10,byte  [rsi-2] 
 	shr 	r14,16	
 	movzx 	r11,byte  [rsi-1]      
	mov 	word [r12+r10*2],r14w
	add 	rax,8	
	shr 	r14,16
    mov 	word [r12+r11*2],r14w 	    
    dec 	bl  	   		      		
	jne 	huff_DCfun1
	
	mov r14w,word  [DU]
	mov r13w,word  [DC]
	mov word [DC],r14w ;*DC    = DU[0];
	sub r14w,r13w
	mov word [cinfo+_ZDU],r14w ; cinfo->ZDU[0] = DU[0] - *DC
	mov r15,0 ;nRLE = 0
; r2 = _mm_set1_epi16(0x0000);
; p = (__m128i*) cinfo->ZDU;
    
; r1 = _mm_load_si128(p++);    // load data 0~7
; r1 = _mm_cmpeq_epi16(r1, r2);
  	subps xmm1,xmm1 
  	movdqa	xmm0, XMMWORD [cinfo + _ZDU + 10h]  
   	xorps       xmm1,xmm1  
; r1 = _mm_cmpeq_epi16(r1, r2);
    pcmpeqw     xmm0,xmm1  

;  result |= _mm_movemask_epi8(r1)<<16;
   	pmovmskb    esi,xmm0  

   	movdqa      xmm0, XMMWORD [cinfo + _ZDU]  
   	shl         esi,10h  
   	pcmpeqw     xmm0,xmm1  
   	pmovmskb    eax,xmm0  
   	movzx       r11d,ax  
   	or          esi,r11d  
	not esi ; esi = result
		
	;r14w => cinfo->ZDU
	cmp 	r14w,0
	jne 	process_DU_1
	mov 	ebx,dword [HTDC]
  	mov 	dword [cinfo+ _RLE + r15],ebx
  	add 	r15,4
  	jmp 	process_DU_0
process_DU_1:
  ; r14w = cinfo->ZDU[0]
   	movsx r11d,r14w	
   	movsx r14d,r14w
   	sar 	r11d,31 ; mask  = ((int)cinfo->ZDU[0])>>31;  // -1 or 0
   	add 	r14d,r11d ;r14 => value  value = cinfo->ZDU[0] + mask;
   	xor 	r11d,r14d 
  
   	bsr 	r12d,r11d
   	movsx r12,r12d
   	add 	r12,1 ;cat   = (BYTE)(32 - clz(value^mask));
 
   	mov 	r11d,dword [HTDC + r12*4]
   	shl 	r14d,16
   	mov 	dword [cinfo+ _RLE + r15],r11d
   	or 	r14d,r12d ; data  = (value<<16)|cat;
   	mov 	dword [cinfo+ _RLE + r15 + 4],r14d
   	add 	r15,8
process_DU_0:	
  	mov 	rax,1
  
	lea  	r13,[HTAC]
   	mov 	rcx,0

  	shr 	esi,2
  	je 		process_DU_2
  
   
process_DU_3:  
  
   	bsf 	ecx,esi
  	sar 	rcx,1

  	mov 	r11,rcx ;n 
  	add 	rax,rcx ;i += n;
 
  	movsx 	rbx,word [cinfo+_ZDU+rax*2]
  	add 	rax,1 ; i + +
  	mov 	r12,rbx
  	sar 	rbx,31 ;mask  = ((int)cinfo->ZDU[ i + n ])>>31;
  	add 	r12,rbx ; value = cinfo->ZDU[ i + n ] + mask;
  	xor 	rbx,r12
  	bsr 	r10,rbx 
  	add 	r10,1 ;cat   = (BYTE)(32 - clz(value^mask));
  	shl 	r12,16
  	or 		r12,r10 ; data  = (value<<16)|cat; 
  	shl 	r11,4
  	add 	r10,r11

  	mov 	ebx,dword [r13+r10*4]
  	mov 	dword [cinfo+ _RLE + r15 + 4],r12d
  	mov 	dword [cinfo+ _RLE + r15 ],ebx
  	add 	r15,8
  	add 	rcx,1
  	shl 	rcx,1
  	shr 	esi,cl
  	jne 	process_DU_3
process_DU_2:

 	mov 	rbx,16  
 	sub 	rbx,rax ; rbx = nrzeroes = (16-i);

 	mov 	r10d,dword [r13+0xf0*4] ;M16zeroes
 	lea 	r8,[cinfo+_ZDU+20h] ; p = (__m128i*) (cinfo->ZDU+16);
 	mov 	r9,3

process_DU_4: 
;  r1 = _mm_load_si128(p++);    // load data 0~7
;  r1 = _mm_cmpeq_epi16(r1, r2);
    
  	movdqa	xmm0, XMMWORD [r8+10h]  
   	xorps   xmm1,xmm1  
;   r1 = _mm_cmpeq_epi16(r1, r2);
    pcmpeqw     xmm0,xmm1  

;  result |= _mm_movemask_epi8(r1)<<16;
   	pmovmskb    esi,xmm0  

   	movdqa      xmm0, XMMWORD [r8]  
   	shl         esi,10h  
   	mov rax,0 ; i = 0
   	pcmpeqw     xmm0,xmm1  
   	pmovmskb    r12d,xmm0  
   	movzx       r11d,r12w  
   	or          esi,r11d  

	not 	esi ;esi = result
	test 	esi,esi
	je 		process_DU_6
process_DU_5:	
  	bsf 	ecx,esi
  	sar 	rcx,1 ;  n = ctz(result)/2;
  	add 	rbx,rcx ;  nrzeroes += n;
  	sub 	rbx,16
	jl 		process_DUEX
	mov 	dword [cinfo+ _RLE + r15 ],r10d
  	add 	r15,4
  	sub 	rbx,16
	jl 		process_DUEX
	mov 	dword [cinfo+ _RLE + r15 ],r10d
  	add 	r15,4
  	sub 	rbx,16
	jl 		process_DUEX
	mov 	dword [cinfo+ _RLE + r15 ],r10d
  	add 	r15,4
  	sub 	rbx,16
process_DUEX:
  	add 	rbx,16	
  	add 	rax,rcx ; i = i + n
    movsx 	r14,word [r8+rax*2]
  	add 	rax,1 ; i + +
  	mov 	r12,r14
  	sar 	r14,31 ;mask  = ((int)cinfo->ZDU[ i + n ])>>31;
  	add 	r12,r14 ; value = cinfo->ZDU[ i + n ] + mask;
  	xor 	r14,r12
  	bsr 	r14,r14 
  	add 	r14,1 ;cat   = (BYTE)(32 - clz(value^mask));
  	shl 	r12,16
  	or 		r12,r14 ; data  = (value<<16)|cat; 
  	shl 	rbx,4
  	add 	r14,rbx

  	mov 	ebx,dword [r13+r14*4]
  	mov 	dword [cinfo+ _RLE + r15 + 4],r12d
  	mov 	dword [cinfo+ _RLE + r15 ],ebx
  	add 	r15,8
  	add 	rcx,1
  	shl 	rcx,1
  	mov 	rbx,0
  	shr 	esi,cl
  	je 		process_DUEX1
  	cmp 	rax,16
  	jl 		process_DU_5
process_DUEX1:  
  	add 	rbx,16
  	sub 	rbx,rax
   	jl 		process_DU_5
   	jmp 	process_DU_7
process_DU_6:
	add 	rbx,16
	sub 	rbx,rax
process_DU_7:	
	add 	r8,32
	sub 	r9,1
	jne process_DU_4
  	movsx 	rbx,word [cinfo + _ZDU + 63*2]
  	cmp 	rbx,0
  	jne 	process_DU_END
  	mov 	r10d,dword [r13] ;M16zeroes
  	mov 	dword [cinfo+ _RLE + r15 ],r10d
  	add r15,4
process_DU_END: 

%ifdef MACHO	
	lea rdi,[cinfo] 
	mov rsi,r15
	shr rsi,2
	pop r11
	pop r10
%else
	lea rcx,[cinfo] 
	mov rdx,r15
	shr rdx,2
%endif
 	call EXTN(writeblock)
 
  	pop r15
  	pop r14 
  	pop r12
	pop rsi
	pop rbx
  	pop rdi;
  	pop r13 
	pop rbp

	ret

; --------------------------------------------------------------------------
;
; GLOBAL(void)
; void writeblock(cinfo_t* cinfo,int num);
;  cinfo : struct 
;  num   : number of componments in cinfo->RLE
; MACHO:
; RDI = cinfo_t* cinfo
; RSI = int      num
; VC++ :
; RCX = bitstring bs
; RDX = cinfo_t *cinfo
; --------------------------------------------------------------------------
; psudo code :
; void writeblock(cinfo_t* cinfo,int num)
; {
;    int i;
;    for(i = 0; i< num ;i++) writebits(cinfo->RLE[i],cinfo);
; };
;
	align	16
	global	EXTN(writeblock)

%ifdef MACHO	
%define cinfo  rdi
%define num    rsi
%else
%define cinfo  rdi
%define num    rdx
%endif

%define i      		r8
%define wordnew   	r9
%define wordpos		al
%define value    	r10d
%define value.w     r10w
%define value.q     r10

%define pbyte     	r9
%define pos		  	r10
%define pos.d	  	r10d

EXTN(writeblock):
	push	rbp;
%ifndef MACHO
	push	rdi;      
	mov		rdi,rcx;  // rdi : cinfo
%endif

	xor		i,i; // i = 0;
.loop_num:
	mov		wordpos,byte [cinfo+_wordpos];  // load cinfo->wordpos	
	mov		wordnew,qword [cinfo+_wordnew]; // load cinfo->wordnew
.L1:
	mov		ecx,dword [cinfo+i*4+_RLE];     // load value & len
	mov		value,ecx; 				
	shr		value,cl;					    // value = bs >> len
	movzx	value.q,value.w;			    // clear 63:16
	add		wordpos,cl;						// wordpos += len
	cmp		wordpos,64;
	jge		.w_1;							// al >= 64
	shl		wordnew,cl;						// wordnew = wordnew << len
	shl		value.q,cl;						// value >> (16-len)
	shr		value,16;
	or		wordnew,value.q;
	inc		i;
	cmp		i,num;
	jl		.L1;

	mov		byte [cinfo+_wordpos],wordpos;	// Save wordpos
	mov		qword [cinfo+_wordnew],wordnew;	// Save wordnew
	
%ifndef MACHO
	pop		rdi;
%endif
	pop		rbp
    ret
    
.w_1:
	mov		ch,cl;						// backup len
	sub		wordpos,cl;					// wordpos -= len
	mov		cl,64;
	sub		cl,wordpos;	  				// shift = 64 - wordpos
	shl		wordnew,cl;					// wordnew = wordnew << shift
	shl		value.q,cl;
	shr		value.q,16;					// value >> (16 - shift)
	or		wordnew,value.q;           	// (cinfo->wordnew << shift) | (value >> (16-shift))
	mov		rax,wordnew;
	sub		ch,cl;						// len -= shift;
	mov		byte [cinfo+_wordpos],ch;	// save wordpos
	shr		ecx,16;						// high word of ecx is value
	mov		qword [cinfo+_wordnew],rcx; // save wordnew
	
	;// Write 8 bytes to memory
	mov		pos.d,dword [cinfo + _pos];	// higher dword of value.q is zero
	mov		pbyte,qword [cinfo + _pbitstream];
	
	mov		cl,9;
.loop:	
	sub		cl,1;
	jz		.exit;
	rol		rax,8;
	mov		byte [pbyte,pos],al;
	inc		pos;
	cmp		al,0xFF;
	jne		.loop;
	mov		byte [pbyte,pos],0;
	inc		pos;
	jmp		.loop;
	
.exit:	

	mov		dword [cinfo + _pos],pos.d;
	
	inc		i;
	cmp		i,num;
	jl		.loop_num;
%ifndef MACHO
	pop		rdi;
%endif
	pop		rbp
    ret
    
; --------------------------------------------------------------------------
;
; GLOBAL(void)
; flashbits(cinfo_t *cinfo)
;
    global	EXTN(flashbits)
	%ifdef MACHO	

%define cinfo    rdi

%else

%define cinfo    rdi

%endif
EXTN(flashbits):

	push rbp
	push rsi	 
%ifndef MACHO	
  	push rdi
  	mov		cinfo,rcx;	
%endif

  	mov esi,ecx
  	mov  cl,byte byte [cinfo+_wordpos]
	mov  rax,qword [cinfo+_wordnew]
	cmp  cl,0
	je   flashbits_2
	mov  edx,dword [cinfo +  _pbitstream];
	mov  r8,qword [cinfo +_pos];
	ror  rax,cl;    
	add rdx,r8
flashbits_1:
	rol  rax,8;
	mov  byte [edx],al;
	inc  r8d;
	inc  rdx
	cmp  al,0xFF;
	jne  flashbits_Next1;
	mov  byte [edx],0;
	inc  r8;
	inc edx
flashbits_Next1:
	sub  cl,8
	jg   flashbits_1
	mov  qword [cinfo + _pos],r8;

flashbits_2:
	mov  qword [cinfo+_wordnew],0
	mov  byte byte [cinfo+_wordpos],0
%ifndef MACHO	
  	pop rdi
%endif
	pop rsi
	pop rbp
	ret;   

; For some reason, the OS X linker does not honor the request to align the
; segment unless we do this.
        align   16
