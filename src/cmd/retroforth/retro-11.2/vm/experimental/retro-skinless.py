#!/usr/bin/env python
# Ngaro VM
# Copyright (c) 2010, Charles Childers
# Optimizations and process() rewrite by Greg Copeland
# -----------------------------------------------------------------------------
import os, sys, math, time

EXIT = 0x0FFFFFFF
# -----------------------------------------------------------------------------
def rx_divmod(a, b):
  x = abs(a)
  y = abs(b)
  q, r = divmod(x, y)

  if a < 0 and b < 0:
    r *= -1
  elif a > 0 and b < 0:
    q *= -1
  elif a < 0 and b > 0:
    r *= -1
    q *= -1

  return q, r

def handle_devices( ip, stack, address, ports, memory, files ):
  ports[0] = 1
  if ports[1] == 1:
    ports[1] = ord(sys.stdin.read(1))
    if ports[1] == 13:
      ports[1] = 10
  if ports[2] == 1:
    if stack[-1] > 0 and stack[-1] < 128:
      sys.stdout.write(chr(stack.pop()))
    ports[2] = 0
  if ports[5] == -1:  # memory size - zero based index
    ports[5] = 1000000 - 1
  elif ports[5] == -2:  # canvas exists?
    ports[5] = 0
  elif ports[5] == -3:  # screen width
    ports[5] = 0
  elif ports[5] == -4:  # screen height
    ports[5] = 0
  elif ports[5] == -5:  # stack depth
    ports[5] = len(stack)
  elif ports[5] == -6:  # address stack depth
    ports[5] = len(address)
  elif ports[5] == -7:  # mouse exists?
    ports[5] = len(stack)
  elif ports[5] == -8:  # time
    ports[5] = int(time.time())
  elif ports[5] == -9:  # exit vm
    ip = EXIT
    ports[5] = 0

  return ip


# -----------------------------------------------------------------------------
def process( memory ):
  ip = 0
  ext = EXIT
  stack = []
  address = []
  ports = [0]*1024
  files = [0]*8

  while ip < ext:
    opcode = memory[ip]

    # If opcode is not one we know,
    # jump past it. We have 30-opcodes.
    if opcode <= 30:
      if   opcode ==  0:   # nop
        pass

      elif opcode ==  1:   # lit
        ip += 1
        stack.append( memory[ip] )

      elif opcode ==  2:   # dup
        stack.append( stack[-1] )

      elif opcode ==  3:   # drop
        stack.pop()

      elif opcode ==  4:   # swap
        a = stack[-2]
        stack[-2] = stack[-1]
        stack[-1] = a

      elif opcode ==  5:   # push
        address.append( stack.pop() )

      elif opcode ==  6:   # pop
        stack.append( address.pop() )

      elif opcode ==  7:   # loop
        stack[-1] -= 1
        if stack[-1] != 0 and stack[-1] > -1:
          ip += 1
          ip = memory[ip] - 1
        else:
          ip += 1
          stack.pop()

      elif opcode ==  8:   # jump
        ip += 1
        ip = memory[ip] - 1
        if memory[ip + 1] == 0:
          ip += 1
          if memory[ip + 1] == 0:
            ip += 1

      elif opcode ==  9:   # return
        ip = address.pop()

      elif opcode == 10:   # >= jump
        ip += 1
        a = stack.pop()
        b = stack.pop()
        if b > a:
          ip = memory[ip] - 1

      elif opcode == 11:   # <= jump
        ip += 1
        a = stack.pop()
        b = stack.pop()
        if b < a:
          ip = memory[ip] - 1

      elif opcode == 12:   # != jump
        ip += 1
        a = stack.pop()
        b = stack.pop()
        if b != a:
          ip = memory[ip] - 1

      elif opcode == 13:   # == jump
        ip += 1
        a = stack.pop()
        b = stack.pop()
        if b == a:
          ip = memory[ip] - 1

      elif opcode == 14:   # @
        stack[-1] = memory[stack[-1]]

      elif opcode == 15:   # !
        mi = stack.pop()
        memory[mi] = stack.pop()

      elif opcode == 16:   # +
        t = stack.pop()
        stack[ -1 ] += t

      elif opcode == 17:   # -
        t = stack.pop()
        stack[-1] -= t

      elif opcode == 18:   # *
        t = stack.pop()
        stack[-1] *= t

      elif opcode == 19:   # /mod
        a = stack[-1]
        b = stack[-2]
        stack[-1], stack[-2] = rx_divmod( b, a )

      elif opcode == 20:   # and
        t = stack.pop()
        stack[-1] &= t

      elif opcode == 21:   # or
        t = stack.pop()
        stack[-1] |= t

      elif opcode == 22:   # xor
        t = stack.pop()
        stack[-1] ^= t

      elif opcode == 23:   # <<
        t = stack.pop()
        stack[-1] <<= t

      elif opcode == 24:   # >>
        t = stack.pop()
        stack[-1] >>= t

      elif opcode == 25:   # 0;
        if stack[-1] == 0:
          stack.pop()
          ip = address.pop()

      elif opcode == 26:   # inc
        stack[-1] += 1

      elif opcode == 27:   # dec
        stack[-1] -= 1

      elif opcode == 28:   # in
        stack[-1] = ports[stack[-1]]

      elif opcode == 29:   # out
        pi = stack.pop()
        ports[ pi ] = stack.pop()

      elif opcode == 30:   # wait
        # Only call if we have pending I/O
        if ports[0] == 0:
          ip = handle_devices( ip, stack, address, ports, memory, files )

    else:
      address.append( ip )
      ip = memory[ip] - 1
      if memory[ip + 1] == 0:
        ip += 1

    ip += 1


# -----------------------------------------------------------------------------
def run():
  fileName = 'retroImage'
  sz = os.path.getsize(fileName)
  f = open( fileName, 'rb' )
  data = f.read()
  f.close()

  memory = [0]*1000000
  for i in range(0, sz, 4):
    b1 = ord(data[i])
    b2 = ord(data[i+1])
    b3 = ord(data[i+2])
    b4 = ord(data[i+3])
    n = b1 + (b2<<8) + (b3<<16) + (b4<<24)
    if n > 2147483647:
      n = n - 4294967296
    memory[i/4] = n

  process( memory )


# -----------------------------------------------------------------------------
if __name__ == "__main__":
  run()
