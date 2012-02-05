#!/usr/bin/env perl
# Ngaro VM
# Copyright (c) 2010 - 2011, Charles Childers
# ----------------------------------------------------------------------------- Dependencies
use strict;
use warnings;

use POSIX;
use integer;
use Switch;

package Retro;
local $| = 1;
# ----------------------------------------------------------------------------- Variables
our $data    = 0;
our $i       = 0;
our $n       = 0;
our $sp      = 0;
our $rp      = 0;
our $ip      = 0;
our @memory  = ();
our @stack   = ();
our @address = ();
our @ports   = ();
# ----------------------------------------------------------------------------- Save Image
sub rxSaveImage {
  open FILE, ">retroImage" or die #!;
  binmode FILE;
  my $i = 0;
  while ($i < $memory[3]) {
    print FILE pack("i", $memory[$i]);
    $i++;
  }
  close FILE;
}
# ----------------------------------------------------------------------------- I/O Handler
sub rxHandleDevices {
  if ($ports[0] == 0)
  {
    $ports[0] = 1;
    if ($ports[1] == 1)
    {
      $i = getc(STDIN);
      $ports[1] = ord($i);
      if ($ports[1] == 13)
      {
        $ports[1] = 10;
      }
    }
    if ($ports[2] == 1)
    {
      if ($stack[$sp] < 0)
      {
        print "...";
      }
      else
      {
        print chr($stack[$sp]);
      }
      $sp = $sp - 1;
      $ports[2] = 0;
    }

    if ($ports[4] == 1)
    {
      rxSaveImage();
      $ports[4] = 0;
    }

    switch ($ports[5])
    {
      case  -1 { $ports[5] = 1000000; }
      case  -2 { $ports[5] = 0; }
      case  -3 { $ports[5] = 0; }
      case  -4 { $ports[5] = 0; }
      case  -5 { $ports[5] = $sp; }
      case  -6 { $ports[5] = $rp; }
      case  -7 { $ports[5] = 0; }
      case  -8 { $ports[5] = time; }
      case  -9 { $ip = 1000000; }
      case -10 { $ports[5] = 0; }
      case -11 { $ports[5] = 0; }
      case -12 { $ports[5] = 0; }
      case -13 { $ports[5] = 0; }
    }
  }
}
# ----------------------------------------------------------------------------- Process Opcodes
sub rxProcessOpcode {
  my $opcode = $memory[$ip];
  my $a = 0;
  my $b = 0;
  if ($opcode >= 0 && $opcode <= 30)
  {
      if ($opcode == 1)
      {
        $sp++; $ip++;
        $stack[$sp] = $memory[$ip];
      }
      elsif ($opcode == 2)
      {
        $sp++;
        $stack[$sp] = $stack[$sp - 1];
      }
      elsif ($opcode == 3)
      {
        $sp--;
      }
      elsif ($opcode == 4)
      {
        $a = $stack[$sp];
        $stack[$sp] = $stack[$sp - 1];
        $stack[$sp - 1] = $a;
      }
      elsif ($opcode == 5)
      {
        $rp++;
        $address[$rp] = $stack[$sp];
        $sp--;
      }
      elsif ($opcode == 6)
      {
        $sp++;
        $stack[$sp] = $address[$rp];
        $rp--;
      }
      elsif ($opcode == 7)
      {
        $stack[$sp]--;
        if ($stack[$sp] != 0 && $stack[$sp] > -1)
        {
          $ip++;
          $ip = $memory[$ip] - 1;
        }
        else
        {
          $ip++;
          $sp--;
        }
      }
      elsif ($opcode == 8)
      {
        $ip++;
        $ip = $memory[$ip] - 1;
        if ($memory[$ip + 1] == 0) { $ip++; }
        if ($memory[$ip + 1] == 0) { $ip++; }
      }
      elsif ($opcode == 9)
      {
        $ip = $address[$rp];
        $rp--;
        if ($memory[$ip + 1] == 0) { $ip++; }
        if ($memory[$ip + 1] == 0) { $ip++; }
      }
      elsif ($opcode == 10)
      {
        $ip++;
        if ($stack[$sp - 1] > $stack[$sp]) { $ip = $memory[$ip] - 1; }
        $sp = $sp - 2;
      }
      elsif ($opcode == 11)
      {
        $ip++;
        if ($stack[$sp - 1] < $stack[$sp]) { $ip = $memory[$ip] - 1; }
        $sp = $sp - 2;
      }
      elsif ($opcode == 12)
      {
        $ip++;
        if ($stack[$sp - 1] != $stack[$sp]) { $ip = $memory[$ip] - 1; }
        $sp = $sp - 2;
      }
      elsif ($opcode == 13)
      {
        $ip++;
        if ($stack[$sp - 1] == $stack[$sp]) { $ip = $memory[$ip] - 1; }
        $sp = $sp - 2;
      }
      elsif ($opcode == 14)
      {
        $stack[$sp] = $memory[$stack[$sp]];
      }
      elsif ($opcode == 15)
      {
        $memory[$stack[$sp]] = $stack[$sp - 1];
        $sp = $sp - 2;
      }
      elsif ($opcode == 16)
      {
        $stack[$sp - 1] = $stack[$sp] + $stack[$sp - 1];
        $sp--;
      }
      elsif ($opcode == 17)
      {
        $stack[$sp - 1] = $stack[$sp - 1] - $stack[$sp];
        $sp--;
      }
      elsif ($opcode == 18)
      {
        $stack[$sp - 1] = $stack[$sp - 1] * $stack[$sp];
        $sp--;
      }
      elsif ($opcode == 19)
      {
        $a = $stack[$sp];
        $b = $stack[$sp - 1];
        $stack[$sp] = POSIX::floor($b / $a);
        $stack[$sp - 1] = $b % $a;
      }
      elsif ($opcode == 20)
      {
        $a = $stack[$sp];
        $b = $stack[$sp - 1];
        $sp--;
        $stack[$sp] = $b & $a;
      }
      elsif ($opcode == 21)
      {
        $a = $stack[$sp];
        $b = $stack[$sp - 1];
        $sp--;
        $stack[$sp] = $b | $a;
      }
      elsif ($opcode == 22)
      {
        $a = $stack[$sp];
        $b = $stack[$sp - 1];
        $sp--;
        $stack[$sp] = $b ^ $a;
      }
      elsif ($opcode == 23)
      {
        $a = $stack[$sp];
        $b = $stack[$sp - 1];
        $sp--;
        $stack[$sp] = $b << $a;
      }
      elsif ($opcode == 24)
      {
        $a = $stack[$sp];
        $b = $stack[$sp - 1];
        $sp--;
        $stack[$sp] = $b >> $a;
      }
      elsif ($opcode == 25)
      {
        if ($stack[$sp] == 0)
        {
          $sp--;
          $ip = $address[$rp];
          $rp--;
        }
      }
      elsif ($opcode == 26)
      {
        $stack[$sp]++;
      }
      elsif ($opcode == 27)
      {
        $stack[$sp]--;
      }
      elsif ($opcode == 28)
      {
        $a = $stack[$sp];
        $stack[$sp] = $ports[$a];
        $ports[$a] = 0;
      }
      elsif ($opcode == 29)
      {
        $ports[$stack[$sp]] = $stack[$sp - 1];
        $sp = $sp - 2;
      }
      elsif ($opcode == 30)
      {
        rxHandleDevices();
      }
  }
  else                  # call (implicit)
  {
    $rp++;
    $address[$rp] = $ip;
    $ip = $memory[$ip] - 1;
    if ($memory[$ip + 1] == 0) { $ip++; }
    if ($memory[$ip + 1] == 0) { $ip++; }
  }
}
# ----------------------------------------------------------------------------- Load Image
sub rxLoadImage()
{
  my $i = 0;
  open FILE, "retroImage" or die #!;
  binmode FILE;
  while (($n = read(FILE, $data, 4)) != 0) {
    $memory[$i] = int(unpack("i", $data));
    $i++;
  }
  close FILE;
}
# ----------------------------------------------------------------------------- Initialize
sub rxInitialize()
{
  my $i = 0;
  while ($i < 1000000) {
    $memory[$i] = 0;
    $i++;
  }
  $i = 0;
  while ($i < 12) {
    $stack[$i] = 0;
    $address[$i] = 0;
    $ports[$i] = 0;
    $i++;
  }
  while ($i < 128) {
    $stack[$i] = 0;
    $address[$i] = 0;
    $i++;
  }
  while ($i < 1024) {
    $address[$i] = 0;
    $i++;
  }
}
# ----------------------------------------------------------------------------- main loop
sub rxMain()
{
  rxInitialize();
  rxLoadImage();
  while ($ip < 1000000)
  {
    rxProcessOpcode();
    $ip++;
  }
}

rxMain();
