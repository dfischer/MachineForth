\ boot-strap stuff ...
\ #01 => lit, #02 => clit
\ #13 => jmp, #14 => jmpz, #15 => jmpnz, #17 => ret
(h) dw (here) #01 c, , #17 c,

\ constant
#04 dw cell #02 c, c, #17 c, inline

\ define some variables
dw s $01 c, (h) @ 0 , #17 c, dw (s) $01 c, , #17 c,
dw d $01 c, (h) @ 0 , #17 c, dw (d) $01 c, , #17 c,

\ ok, now we can start writing the language in itself
: >s (s) ! ; : >d (d) ! ;
: here (h) @ ; inline

: [ 0 state ! ; immediate
: ] 1 state ! ; immediate

\ program control
: if   #14 c, here 0 , ; immediate
: else #13 c, here swap 0 , here swap ! ; immediate
: then here swap ! ; immediate
: leave #17 c, ; immediate

: begin here ; immediate
: again #13 c, , ; immediate
: until #14 c, , ; immediate
: while #15 c, , ; immediate

\ Machine Forth words
: @+ s @ s 4 + >s ; : c@+ s c@ s 1+ >s ;
: !+ d ! d 4 + >d ; : c!+ d c! d 1+ >d ;

\ core words
: allot here + (here) ! ;
: space $20 emit ; : bl $20 ; : cr #10 emit ;
: . space (.) ;
: hex $10 base ! ; : decimal #10 base ! ; : binary %10 base ! ;
: hex.     base @ hex     swap . base ! ;
: decimal. base @ decimal swap . base ! ;
: strlen ( a -- n ) >s 0 >r begin c@+ if r> 1+ >r then while r> ;
: count ( a1 -- a2 n ) 1+ dup 1- c@ ;
: type ( addr n -- ) swap >s begin c@+ emit 1- dup while drop ;

: >> begin swap 2/ swap 1- dup while drop ;
: << begin swap 2* swap 1- dup while drop ;
: w@ dup c@ swap 1+ c@ #8 << or ;
: w! over over c! swap #8 >> swap 1+ c! ;

\ navigating the dictionary
: >xt @ ;        : >flags 4 + c@ ;      
: >len 5 + ;     : >name 6 + ;
: >next #36 - ;

: .word  ( addr -- ) dup >len space count type ;
: .wordl ( addr -- ) cr dup >xt hex. dup >flags . dup >len space count type ;
: words-each last 0 begin drop ( do something ) >next dup >xt dup while drop ;
: words      last 0 begin drop    dup .word     >next dup >xt dup while drop ;
: wordsl     last 0 begin drop    dup .wordl    >next dup >xt dup while drop ;

\ for the REPL
: .. dup . ;
: .s space '(' emit >r >r >r >r .. r> .. r> .. r> .. r> .. space ')' emit ;
: pow ( n1 n2 -- n3 ) 1 >r begin over r> * >r 1- dup while drop drop r> ;
: .2 dup base @       < if '0' emit then (.) ;
: .3 dup base @ 2 pow < if '0' emit then .2 ;
: .4 dup base @ 3 pow < if '0' emit then .3 ;
: mil ( n1 -- n2 ) #1000 #1000 * * ;
: timer gettick ;
: elapsed swap - 1000 /mod space (.) '.' emit .3 ;
: bench ( n -- ) timer swap begin 1- dup while drop timer elapsed ;
: benches ( n count -- ) begin over bench 1- dup while drop ;
: main 100 mil 10 benches ;

\ 17 load
