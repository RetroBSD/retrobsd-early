<?php

/* Ngaro VM 
 * PHP port by Rémy Mouëza from the Python VM.
 *
 * Permission to use, copy, modify, and/or distribute this software 
 * for any purpose with or without fee is hereby granted, provided 
 * that the above copyright notice and this permission notice appear 
 * in all copies. 
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL 
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE 
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL 
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR 
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER 
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
 * PERFORMANCE OF THIS SOFTWARE. 
 * 
 * Copyright (c) 2008 - 2011, Charles Childers 
 * Copyright (c) 2009 - 2011, Luke Parrish 
 * Copyright (c) 2009 - 2010, JGL 
 * Copyright (c) 2010 - 2011, Marc Simpson 
 * Copyright (c) 2010, Jay Skeer 
 * Copyright (c) 2010, Greg Copeland 
 * Copyright (c) 2011, Aleksej Saushev 
 * Copyright (c) 2011, Foucist 
 * Copyright (c) 2011, Erturk Kocalar 
 * Copyright (c) 2011, Kenneth Keating 
 * Copyright (c) 2011, Ashley Feniello 
 * Copyright (c) 2011, Peter Salvi
 *
 */

define ("EXT", 0x0FFFFFFF); // The exit code value.

function rxDivMod ($a, $b) { 
  $x = abs ($a); 
  $y = abs ($b);
  $q = floor ($x / $y);
  $r = $x % $y;

  if ($a < 0 && $b < 0) {
      $r *= -1; 
  } else if ($a > 0 && $b < 0) {
      $q *= -1;
  } else if ($a < 0 && $b > 0) {
      $r *= -1;
      $q *= -1;
  }

  return array ($q, $r); 
}

function rxGetInput (&$inputs) {
    $a = 0; 
    $last = end ($inputs);
    
    if ($last != 0) {
        if (feof ($last)) { 
            array_pop ($inputs);
            $a = 0 ; 
        } else {
            $a = fread ($last, 1);
            $a = ord ($a); 
        }
    } else {
        $a  = ord (fread (STDIN, 1)); 
    }
    return $a ; 
}

function rxHandleDevices (&$ip, &$stack, &$address, 
                          &$ports, &$memory, &$files, &$inputs) 
{ 
    $ports [0] = 1;

    if ($ports [1] == 1) {
        $ports [1] = rxGetInput ($inputs);
        
        if ($ports [1] == 13) {
            $ports [1] = 10; 
        }
    }
    if ($ports [2] == 1) {
        if (end ($stack) > 0 && end ($stack) < 128) { 
            fwrite (STDOUT, 
                    chr (array_pop ($stack))); 
        }
        //fflush (STDOUT);
        $ports [2] = 0; 
    }

    switch ($ports [4]) {

    case 1: // file save
        $f = fopen ('retroImage', 'wb'); 
        $i = 0; 

        while ($i < $memory [3]) {
            fwrite ($f, pack ('i', $memory [$i]));
            $i += 1;
        }
        fclose ($f);
        $ports [4] = 0;
        break; 
    
    case 2: // file include
        $name = '';
        $i    = array_pop ($stack);

        while ($memory [$i] != 0) {
            $name = $name . chr ($memory [$i]);
            $i   += 1; 
        }
        
        if (! file_exists ($name)) {
            $name = dirname (__FILE__) . '/' . $name;
        }

        $f = fopen ($name, 'r'); 
        array_push ($inputs, $f); 
        $ports [4] = 0; 
        break; 
        
    case -1: // file open
        $slot = 0;
        $i    = 1; 
        $name = '';
        
        while ($i < 8) {
            if ($files [$i] == 0) {
                $slot = $i;
            }
            $i += 1;
        }
        $mode = array_pop ($stack); 
        $i    = array_pop ($stack);

        while ($memory [$i] != 0) {
            $name = $name . chr ($memory [$i]); 
            $i   += 1;
        }

        if ($slot > 0) {
            switch ($mode) {
            case 0:
                if (file_exists ($name)) {
                    $files [$slot] = fopen ($name, 'r');
                } else {
                    $slot = 0;
                }
                break;

            case 1:
                $files [$slot] = fopen ($name, 'w');
                break;

            case 2:
                $files [$slot] = fopen ($name, 'a');
                break;

            case 3:
                if (file_exists ($name)) {
                    $files [$slot] = fopen ($name, 'r+');
                } else {
                    $slot = 0;
                }
                break;
            }
        }
        $ports [4] = $slot; 
        break;

    case -2: // file read
        $slot = array_pop ($stack);
        $ports [4] = ord (fread ($files [$slot], 1));
        break;

    case -3: // file write
        $slot = array_pop ($stack);
        fwrite ($files [$slot], chr (array_pop ($stack)));
        $ports [4] = 1;
        break;

    case -4: // file close
        $slot = array_pop ($stack);
        fclose ($files [$slot]);
        $files [$slot] = 0;
        $ports [4] = 0;
        break;

    case -5: // file position
        $slot = array_pop ($stack);
        $ports [4] = ftell ($files [$slot]);
        break;

    case -6: // file seek
        $slot = array_pop ($stack);
        $pos  = array_pop ($stack);
        $ports [4] = fseek ($files [$slot], $pos, 0); // SEEK_SET
        break;

    case -7: // file size
        $slot = array_pop ($stack);
        $at   = ftell ($files [$slot]);
        fseek ($files [$slot], 0, 2); // SEEK_END
        $ports [4] = ftell ($files [$slot]);
        fseek ($files [$slot], $at, 0); // SEEK_SET
        break;

    case -8: // file delete
        $name = '';
        $i    = array_pop ($stack);

        while ($memory [$i] != 0) { 
            $name = $name . chr ($memory [$i]);
            $i   += 1;
        }
        $i     = 0;
        if (file_exists ($name)) {
            unlink ($name);
            $i = 1;
        }
        $ports [4] = $i;
    }

    switch ($ports [5]) {

    case -1: // memory size - zero based index
        $ports [5] = 1000000 - 1;
        break;

    case -2: // canvas exists?
        $ports [5] = 0;
        break;

    case -3: // canvas width
        $ports [5] = 0;
        break;

    case -4: // canvas height
        $ports [5] = 0;
        break;

    case -5: // stack depth
        $ports [5] = count ($stack);
        break;

    case -6: // address stack depth
        $ports [5] = count ($address);
        break;

    case -7: // mouse exists?
        $ports [5] = count ($stack);
        break;

    case -8: // time
        $ports [5] = time (); 
        break;

    case -9: // exit vm
        $ip = EXT;
        $ports [5] = 0;
        break;

    case -10: // environment variable
        $req  = array_pop ($stack);
        $dest = array_pop ($stack);
        $key  = '';

        while ($memory [$req] != 0) {
            $key  = $key . chr ($memory [$req]);
            $req += 1;
        }
        $req = getenv ($key); 
        $req = $req ? $req : "/"; 

        $memory [$dest] =  0;
        $i = 0;
        
        while ($i < count ($req)) {
            $memory [$dest] = ord ($req [$i]);
            $i    += 1; 
            $dest += 1;
            $memory [$dest] = 0;
        }
        $ports [5] = 0;
        break;

    case -11: // console width
        $ports [5] = 0;
        break;

    case -12: // console height
        $ports [5] = 0;
        break;
    }

    return $ip;
}

function process (&$memory, &$inputs) {
    $ip      = 0;
    $ext     = EXT;
    $stack   = array (); // 128.
    $address = array (); // 1024
    $ports   = array_fill (0, 12, 0);
    $files   = array_fill (0,  8, 0);

    while ($ip < $ext) {
        $opcode = $memory [$ip];

        if ($opcode <= 30) {
            switch ($opcode) { 
            
            case 0: // nop
                break;
            
            case 1: // lit
                $ip += 1;
                array_push ($stack, $memory [$ip]);
                break;

            case 2: // dup
                array_push ($stack, end ($stack));
                break;

            case 3: // drop
                array_pop ($stack);
                break;

            case 4: // swap
                $a   = array_pop ($stack); 
                $b   = array_pop ($stack);
                array_push ($stack, $a);
                array_push ($stack, $b);
                break;
            
            case 5: // push
                array_push ($address, array_pop ($stack));
                break;

            case 6: // pop
                array_push ($stack, array_pop ($address));
                break;

            case 7: // loop
                array_push ($stack, array_pop ($stack) -1);

                if (end ($stack) != 0 && end ($stack) > -1) {
                    $ip += 1;
                    $ip = $memory [$ip] -1;
                } else {
                    $ip += 1;
                    array_pop ($stack);
                }
                break;

            case 8: // jump
                $ip += 1; 
                $ip = $memory [$ip] -1; 

                if ($memory [$ip + 1] == 0) {
                    $ip += 1;
                    if ($memory [$ip + 1] == 0) { 
                        $ip += 1;
                    }
                }
                break ;

            case 9: // return
                $ip = array_pop ($address);

                if ($memory [$ip + 1] == 0) {
                    $ip += 1; 
                    if ($memory [$ip + 1] == 0) {
                        $ip += 1; 
                    }
                }
                break;

            case 10: // >= jump
                $ip += 1;
                $a   = array_pop ($stack);
                $b   = array_pop ($stack);

                if ($b > $a) {
                    $ip = $memory [$ip] -1;
                }
                break;

            case 11: // <= jump
                $ip += 1;
                $a   = array_pop ($stack);
                $b   = array_pop ($stack);

                if ($b < $a) {
                    $ip = $memory [$ip] -1;
                }
                break;

            case 12: // != jump
                $ip += 1;
                $a   = array_pop ($stack);
                $b   = array_pop ($stack);

                if ($b != $a) {
                    $ip = $memory [$ip] -1;
                }
                break;

            case 13: // == jump
                $ip += 1;
                $a   = array_pop ($stack);
                $b   = array_pop ($stack);

                if ($b == $a) {
                    $ip = $memory [$ip] -1;
                }
                break;

            case 14: // @
                $v = array_pop ($stack);
                array_push ($stack, $memory [$v]);
                break;

            case 15: // !
                $mi = array_pop ($stack);
                $memory [$mi] = array_pop ($stack);
                break;

            case 16: // + 
                //$b = pack ('L', array_pop ($stack) & 0xffffffff); 
                //$a = pack ('L', array_pop ($stack) & 0xffffffff); 
                $b = array_pop ($stack); 
                $a = array_pop ($stack); 
                $r = $a + $b;
                array_push ($stack, $r);
                //$u = array_values (unpack ('l', pack ('L', $r & 0xffffffff)));
                //array_push ($stack, $u [0]);
                break; 
            
            case 17: // -
                $b = array_pop ($stack); 
                $a = array_pop ($stack); 
                $r = $a - $b;
                array_push ($stack, $r);
                //$u = array_values (unpack ('l', pack ('L', $r & 0xffffffff)));
                //array_push ($stack, $u [0]);
                break; 


            case 18: // * 
                $b = array_pop ($stack); 
                $a = array_pop ($stack); 
                $r = $a * $b;
                array_push ($stack, $r);
                //$u = array_values (unpack ('l', pack ('L', $r & 0xffffffff)));
                //array_push ($stack, $u [0]);
                break; 
            
            case 19: // /mod 
                $a = array_pop ($stack); 
                $b = array_pop ($stack); 
                $ar = rxDivMod ($b, $a);
                array_push ($stack, $ar [1]);
                array_push ($stack, $ar [0]);
                //$u1 = array_values (unpack ('l', 
                //                            pack ('L', $ar [1] & 0xffffffff)));
                //$u0 = array_values (unpack ('l', 
                //                            pack ('L', $ar [0] & 0xffffffff)));
                //array_push ($stack, $u1 [0]);
                //array_push ($stack, $u0 [0]);
                break;

            case 20: // and
                $b = array_pop ($stack); 
                $a = array_pop ($stack); 
                array_push ($stack, $a & $b);
                break;
            
            case 21: // or
                $b = array_pop ($stack); 
                $a = array_pop ($stack); 
                array_push ($stack, $a | $b);
                break;
            
            case 22: // xor
                $b = array_pop ($stack); 
                $a = array_pop ($stack); 
                array_push ($stack, $a ^ $b);
                break;
            
            case 23: // <<
                $b = array_pop ($stack); 
                $a = array_pop ($stack); 
                array_push ($stack, $a << $b);
                break;
            
            case 24: // >>
                $b = array_pop ($stack); 
                $a = array_pop ($stack); 
                array_push ($stack, $a >> $b);
                break;

            case 25: // 0;
                if (end ($stack) == 0) {
                    array_pop ($stack);
                    $ip = array_pop ($address);
                }
                break;
            
            case 26: // inc
                array_push ($stack, array_pop ($stack) + 1);
                break;

            case 27: // dec
                array_push ($stack, array_pop ($stack) - 1);
                break;

            case 28: // in
                $t          = array_pop ($stack);
                array_push ($stack, $ports [$t]);
                $ports [$t] = 0;
                break;
            
            case 29: // out
                $pi          = array_pop ($stack);
                $ports [$pi] = array_pop ($stack);
                break;

            case 30: // wait
                // Only call if we have pending I/O
                if ($ports [0] == 0) {
                    $ip = rxHandleDevices ($ip, $stack, $address, $ports, 
                                           $memory, $files, $inputs);
                }
                break;
            }
        } else {
            array_push ($address, $ip);
            $ip = $memory [$ip] -1;

            if ($memory [$ip + 1] == 0) {
                $ip += 1; 
                if ($memory [$ip + 1] == 0) {
                    $ip += 1;
                }
            }
        }
        $ip += 1;
    }
}

function run ($argv) {
    $imageFile = dirname (__FILE__) . '/retroImage';
    $size      = filesize ($imageFile); 
    $cells     = $size / 4; 
    $inputs    = array_fill (0, 12, 0);
    array_push ($inputs, 0);

    $f         = fopen ($imageFile, 'rb');
    $memory    = unpack ('i*', fread ($f, $size));
    fclose ($f);
    
    $memory = array_merge ($memory, array_fill (0, 1000000 - $cells, 0)); 

    if (count ($argv) > 2) {
        $i = 0; 

        while ($i < count ($argv)) {
            if ($argv [$i] == '--with') { 
                $i += 1; 
                array_push ($inputs, fopen ($argv [$i], 'r'));
            }
            $i += 1;
        }
    }

    process ($memory, $inputs);
}

run ($argv);
