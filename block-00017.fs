\ J1C cpu compiler 

\ This is a 16-bit architecture, attempting to cram as much functionality
\ as possible into those 16 bits

\ If the top bit is on, it's a CALL or JMP:
\ INSTR-CALL  100A AAAA AAAA AAAA
\ INSTR-JMPZ  101A AAAA AAAA AAAA
\ INSTR-JMPNZ 110A AAAA AAAA AAAA
\ INSTR-JMP   111A AAAA AAAA AAAA
\ ... where A is an address bit (13 bits)

\ If the top 2 bits are '01', it's an ALU instruction
\ INSTR-ALU 01aa aaaa aaaa oooo
\ ... where oooo is the operation for new-T
\ ... and each a bit is an additional operation

\ If the top 2 bits are '00', it's a LIT instruction
\ INSTR-LIT 0000 rsss NNNN NNNN
\ ... where NNNN NNNN is the value field (8-bits)
\ ... and r = 1 indicates to also return
\ ... and each s = 1 means to shift T 8 bits first
\ NB: sss = 000 means add a push the NNNN, anything else changes T

: t-CELL #2 ;
: t-sz $40 ;
variable tgt t-CELL t-sz * allot
variable t-(HERE) #0 t-(HERE) !

: t-HERE  t-(HERE) @ ;
: t-@ t-CELL * tgt + w@ ;
: t-! t-CELL * tgt + w! ;
: t-, t-HERE t-! t-HERE 1+ t-(HERE) ! ;
: t-lastOp t-HERE 1- t-@ ;

: INSTR-NOOP     $0000 ; \ 0000 xxxx xxxx xxxx
: INSTR-LIT      $2000 ; \ 0010 rsss NNNN NNNN
: INSTR-ALU      $4000 ; \ 01aa aaaa aaaa aaaa
: INSTR-CALL     $8000 ; \ 100A AAAA AAAA AAAA
: INSTR-JMPZ     $A000 ; \ 101A AAAA AAAA AAAA
: INSTR-JMPNZ    $C000 ; \ 110A AAAA AAAA AAAA
: INSTR-JMP      $E000 ; \ 111A AAAA AAAA AAAA

: NOOP            INSTR-NOOP     t-, ;
: LIT   $0FFF and INSTR-LIT   or t-, ;
: ALU   $3FFF and INSTR-ALU   or t-, ;
: CALL  $1FFF and INSTR-CALL  or t-, ;
: JMPZ  $1FFF and INSTR-JMPZ  or t-, ;
: JMPNZ $1FFF and INSTR-JMPNZ or t-, ;
: JMP   $1FFF and INSTR-JMP   or t-, ;

\ ALU operations for new-T
: T     $00 ;
: N     $01 ;
: N&T   $02 ;
: N|T   $03 ;
: N^T   $04 ;
: ~T    $05 ;
: N+T   $06 ;
: T-1   $07 ;
: N<T   $08 ;
: N=T   $09 ;
: N<<T  $0A ;
: N>>T  $0B ;
: [T]   $0C ;
: R     $0D ;
: dsp   $0E ;
: Nu<T  $0F ;

\ Other ALU operations
: UNUSED2    $2000 or ;
: R->PC      $1000 or ;
: r-1        $0800 or ;
: r+1        $0400 or ;
: d-1        $0200 or ;
: d+1        $0100 or ;
: UNUSED1    $0080 or ;
: T->R       $0040 or ;
: T->N       $0020 or ;
: N->[T]     $0010 or ;

\ xxxxxx  N>>T  R->PC r-1 r+1 d-1 d+1 T->R T->N T->[N]  alu ;
: t@      [T]                                           alu ;
: tdup    T                       d+1                   alu ;
: tswap   N                                T->N         alu ;
: tdrop   N                   d-1                       alu ;
: tover   N                       d+1                   alu ;
: t1-     T-1                                           alu ;
: t+      N+T                 d-1                       alu ;
: t>r     N               r+1 d-1     T->R              alu ;
: tr@     R                       d+1                   alu ;
: tr>     R           r-1         d+1                   alu ;
: tret    T     R->PC r-1                               alu ;
: t>>     N>>T                d-1                       alu ;
: t<<     N<<T                d-1                       alu ;
: tand    N&T                 d-1                       alu ;
: tor     N|T                 d-1                       alu ;
: txor    N^T                 d-1                       alu ;
: tnot    ~T                                            alu ;
: t=      N=T                 d-1                       alu ;
: t<      N<T                 d-1                       alu ;
: tu<     Nu<T                d-1                       alu ;
: depth   dsp                     d+1                   alu ;
\ xxxxxx  N>>T  R->PC r-1 r+1 d-1 d+1 T->R T->N T->[N]  alu ;

: canAddRet? t-lastOp .  0 ;
: addRet t-lastOp R->PC r-1 or t-HERE 1- t-! ;
: t-ret canAddRet? if addRet else tret then drop ;

: t.1 dup t-@ (.) ',' emit bl emit 1+ ;
: t.8 t.1 t.1 t.1 t.1 t.1 t.1 t.1 t.1 cr ;
: tdump 0 t-sz #8 / begin >r t.8 r> 1- while drop drop ;

: test [ #01 c,  t-HERE , ] CALL ;
tdup t-ret
: t2 [ #01 c,  t-HERE , ] CALL ;
: t3 [ #01 c,  t-HERE , ] CALL ;
