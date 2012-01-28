\ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
\ Ngaro VM, ANS Forth (quick and dirty prototype)
\ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
\ Copyright (c) 2011, Marc Simpson
\ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
\ * Benefits from an optimising compiler due to the focus on
\   tight factoring.
\ * Retro will use the host's data stack and an emulated return
\   stack.
\ * Some I/O capabilities are missing.
\ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

vocabulary ngaro also ngaro definitions

\ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

: stack: here swap cells allot  dup constant create , ;
variable ip
1000000 constant image-size
image-size cells allocate throw constant image

12 constant port#
create    port port# cells allot
create handler port# cells allot

100 constant stack-depth
stack-depth stack: rs rsp
stack-depth stack: files filep

: zero    cells erase ;
: reset   port port# zero  handler port# zero ; reset

\ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
\ (These are written as macros for better performance)

: cell/   [ 1 cells ] literal / ;
: `       postpone postpone ; immediate

: ++      1 ` literal ` swap ` +! ; immediate
: ip++    1 ` literal ` ip ` +!  ; immediate \ faster than ip ++

: push  ( na- )
  ` tuck ` @ ` ! ` cell ` swap ` +! ; immediate

: pop   ( a-n )
  ` dup ` @ ` cell ` - ` tuck ` swap ` ! ` @ ; immediate

: rs>     ` rsp ` pop   ; immediate
: >rs     ` rsp ` push  ; immediate

: >image  ` cells ` image ` + ; immediate

\ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
\ Unix time calculation library.
\ Copyright (c) 2011, Charles Childers
\ -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-BEG
: leap-year? ( y -- ? )
  dup 400 mod 0= if drop true  exit then
  dup 100 mod 0= if drop false exit then 4 mod 0= ;

: seconds-per-year ( y -- n )
  leap-year? if 31622400 else 31536000 then ;

: seconds-per-month ( m -- n )
  case
    2 of 28 endof   4 of 30 endof   6 of 30 endof
    9 of 30 endof  11 of 30 endof  31 swap
  endcase 86400 * ;

: get-unix-time ( -- n )
  0 swap  1970 ?do i seconds-per-year  + loop
  swap       1 ?do i seconds-per-month + loop
  swap 1- 86400 * +  swap 3600 * +  swap 60 * + + ;
\ -~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-~-END

\ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
\ File words -- currently just support for inclusion and save.

variable length
create filename 1024 chars allot

: f:tos   filep @ 1 cells -  ;
: files?  files filep @ <>   ;

: f:drop  filep pop close-file throw ;

: f:key   here 1 f:tos @ read-file throw
          0= if f:drop 0 else here c@ -1 then ;

\ like cmove; ( dest source -- n ); source is a retro address
: rmove   over >r begin dup >image @ while
            tuck >image @ over c! 1 chars + swap 1+ repeat
          drop  r> - 1 chars / ;

: f:name  filename swap rmove filename swap dup length ! ;

: f:inc   f:name
          r/o open-file if cr ." No such file." cr drop else
          filep push then ;

: f:save  s" retroImage" w/o create-file throw dup
          image image-size cells rot write-file throw
          close-file throw ;

r/o r/w + w/o + constant r/a

: mode?    case
             0 of r/o endof
             1 of w/o endof
             2 of r/a endof
             3 of r/w endof
           endcase ;

: f:seek   >r s>d r> reposition-file ;

: f:size   file-size 2drop ;

: create?  dup 0 = if drop filename length @ r/w create-file drop then ;

: open-for-append
           r/w open-file if create? else
           dup f:size dup 0 > if 2dup f:seek drop then drop then ;

: f:open   >r f:name r> mode?
           case
             r/o of r/o open-file if drop 0 then endof
             w/o of w/o open-file if drop 0 create? then endof
             r/a of open-for-append              endof
             r/w of r/w open-file if drop 0 then endof
           endcase ;

: f:read   >r filename 1 r> read-file drop 1 = if filename c@ else 0 then ;

: f:write  >r filename c! filename 1 r> write-file drop 1 ;

: f:close  close-file drop 0 ;

: f:pos    file-position 2drop ;

: f:delete f:name delete-file if 0 else -1 then ;

\ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

: register: ( xt port ) ' swap cells handler + ! ;

: (ready) 1 port ! ;
: >port   cells port + ;
: port?   0 port# 1+ within ;

: port[   ` >port ` dup ` >r ` @ ; immediate
: ]port   ` r> ` ! ` (ready) ; immediate

: reader  files? if f:key 0= if recurse then else key then ;
: input   port[ 1 = if reader else 0 then ]port ;
1 register: input

: output  port[ 1 = if emit then 0 ]port ;
2 register: output

: file-io port[ case
             1 of f:save 0 endof
             2 of f:inc  0 endof
            -1 of f:open   endof
            -2 of f:read   endof
            -3 of f:write  endof
            -4 of f:close  endof
            -5 of f:pos    endof
            -6 of f:seek   endof
            -7 of f:size   endof
            -8 of f:delete endof
          0 swap
          endcase ]port ;
4 register: file-io

: vm-info port[ case
            -1 of image-size                endof
            -5 of depth                     endof
            -6 of rsp @ rs - cell/          endof
            -8 of time&date get-unix-time   endof
            -9 of image-size ip ! 0         endof
            0 swap ( default )
          endcase ]port ;
5 register: vm-info

\ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

: handle  dup cells handler + @ dup if execute else 2drop then ;
: ?handle dup >port @ if handle else drop then ;
: i/o     port# 1 do i ?handle loop ;

\ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

: (adjust)  ip @ >image dup @ 0 u< if drop image-size ip ! else
              cell+ dup @ 0= if ip ++ then
              cell+ @ 0= if ip ++ then then ;

: (jmp)     ` ip ` @ ` >image ` @ ` 1- ` ip ` ! ; immediate

: vm_nop ;
: vm_lit    ip++ ip @ >image @ ;
\ Native dup, drop, swap
: vm_push   >rs ;
: vm_pop    rs> ;
: vm_loop   1-  dup 0> if ip++ (jmp) else ip++ drop then ;
: vm_jmp    ip++ (jmp) (adjust) ;
: vm_ret    rs> ip ! ;
: vm_>jmp   ip++  > if (jmp) then ;
: vm_<jmp   ip++  < if (jmp) then ;
: vm_<>jmp  ip++ <> if (jmp) then ;
: vm_=jmp   ip++  = if (jmp) then ;
: vm_@      >image @ ;
: vm_!      >image ! ;
\ Native +, -, *
: vm_/mod   swap s>d rot sm/rem ;
\ Native and, or, xor, shl (lshift), shr (rshift)
: vm_0exit  dup 0= if drop rs> ip ! then ;
\ Native inc (1+), dec (1-)
: vm_in     >port dup @ swap 0 swap ! ;
: vm_out    dup port? if >port ! else 2drop then ;
: vm_wait   port @ 1 <> if i/o then ;

create opcodes
' vm_nop     ,    ' vm_lit   ,   ' dup      ,
' drop       ,    ' swap     ,   ' vm_push  ,
' vm_pop     ,    ' vm_loop  ,   ' vm_jmp   ,
' vm_ret     ,    ' vm_>jmp  ,   ' vm_<jmp  ,
' vm_<>jmp   ,    ' vm_=jmp  ,   ' vm_@     ,
' vm_!       ,    ' +        ,   ' -        ,
' *          ,    ' vm_/mod  ,   ' and      ,
' or         ,    ' xor      ,   ' lshift   ,
' rshift     ,    ' vm_0exit ,   ' 1+       ,
' 1-         ,    ' vm_in    ,   ' vm_out   ,
' vm_wait    ,

\ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

: process
  ip @ >image @ dup 0 31 within if
    cells opcodes + @ execute else
      ip @ >rs  1- ip !  (adjust) then ;

: (retro)
  -1 ip !  begin ip++ process ip @ image-size >= until ;

: load-image
  s" retroImage" r/o open-file throw dup
  image image-size cells rot read-file nip throw
  close-file throw ;

: load-error  ." Sorry, couldn't load retroImage." cr bye ;
: run-error   cr ." Runtime error, exiting Retro." cr bye ;
: retro       ['] load-image catch if load-error then
              ['] (retro) catch if run-error then ;

retro
bye

\ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
