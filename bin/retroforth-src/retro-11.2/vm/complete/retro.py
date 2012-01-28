#!/usr/bin/env python

# Ngaro VM
# Copyright (c) 2010 - 2011, Charles Childers
# Optimizations and process() rewrite by Greg Copeland
# -----------------------------------------------------------------------------
import os, sys, math, time, struct
from struct import pack, unpack

set_termio = False
try:
    import termios
    set_termio = True
except: pass

EXIT = 0x0FFFFFFF
# -----------------------------------------------------------------------------
def rxDivMod( a, b ):
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


def rxGetInput( inputs ):
  a = 0
  if inputs[-1] != 0:
    a = inputs[-1].read(1)
    if len(a) == 0:
      inputs.pop()
      a = 0
    else:
      a = ord(a)
  else:
    a = ord(sys.stdin.read(1))
  return a

# -----------------------------------------------------------------------------
def rxHandleDevices( ip, stack, address, ports, memory, files, inputs ):
  ports[0] = 1
  if ports[1] == 1:
    ports[1] = rxGetInput( inputs )
    if ports[1] == 13:
      ports[1] = 10
  if ports[2] == 1:
    if stack[-1] > 0 and stack[-1] < 128:
      sys.stdout.write(chr(stack.pop()))
    sys.stdout.flush()
    ports[2] = 0
  if ports[4] ==  1:    # file save
    f = open('retroImage', 'wb')
    i = 0
    while i < memory[3]:
      f.write(struct.pack('i', memory[i]))
      i += 1
    f.close()
    ports[4] = 0
  if ports[4] ==  2:    # file include
    name = ''
    i = stack.pop()
    while memory[i] != 0:
      name = name + chr(memory[i])
      i += 1
    f = open(name, 'r')
    inputs.append( f )
    ports[4] = 0
  if ports[4] == -1:    # file open
    slot = 0
    i = 1
    name = ''
    while i < 8:
      if files[i] == 0:
        slot = i
      i += 1
    mode = stack.pop()
    i = stack.pop()
    while memory[i] != 0:
      name = name + chr(memory[i])
      i += 1
    if slot > 0:
      if mode == 0:
        if os.path.exists(name):
          files[slot] = open(name, 'r')
        else:
          slot = 0
      elif mode == 1:
        files[slot] = open(name, 'w')
      elif mode == 2:
        files[slot] = open(name, 'a')
      elif mode == 3:
        if os.path.exists(name):
          files[slot] = open(name, 'r+')
        else:
          slot = 0
    ports[4] = slot;
  elif ports[4] == -2:  # file read
    slot = stack.pop()
    ports[4] = ord(files[slot].read(1))
  elif ports[4] == -3:  # file write
    slot = stack.pop()
    files[slot].write(chr(stack.pop()))
    ports[4] = 1
  elif ports[4] == -4:  # file close
    slot = stack.pop()
    files[slot].close()
    files[slot] = 0
    ports[4] = 0
  elif ports[4] == -5:  # file position
    slot = stack.pop()
    ports[4] = files[slot].tell()
  elif ports[4] == -6:  # file seek
    slot = stack.pop();
    pos  = stack.pop();
    ports[4] = files[slot].seek(pos, 0);
  elif ports[4] == -7:  # file size
    slot = stack.pop()
    at = files[slot].tell()
    files[slot].seek(0, 2) # SEEK_END
    ports[4] = files[slot].tell()
    files[slot].seek(at, 0) # SEEK_SET
  if ports[4] == -8:    # file delete
    name = ''
    i = stack.pop()
    while memory[i] != 0:
      name = name + chr(memory[i])
      i = i + 1
    i = 0
    if os.path.exists(name):
      os.remove(name)
      i = 1
    ports[4] = i
  if ports[5] == -1:  # memory size - zero based index
    ports[5] = 1000000 - 1
  elif ports[5] == -2:  # canvas exists?
    ports[5] = 0
  elif ports[5] == -3:  # canvas width
    ports[5] = 0
  elif ports[5] == -4:  # canvas height
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
  elif ports[5] == -10: # environment variable
    req = stack.pop()
    dest = stack.pop()
    key = ''
    while memory[req] != 0:
      key = key + chr(memory[req])
      req += 1
    req = os.getenv(key, default="/")
    memory[dest] = 0
    i = 0
    while i < len(req):
      memory[dest] = ord(req[i])
      i += 1
      dest += 1
      memory[dest] = 0
    ports[5] = 0
  elif ports[5] == -11: # console width
    ports[5] = 0
  elif ports[5] == -12: # console height
    ports[5] = 0
  elif ports[5] == -13: # bits per cell
    ports[5] = 32

  return ip


# -----------------------------------------------------------------------------
def process( memory, inputs ):
  ip = 0
  ext = EXIT
  stack = [] * 128
  address = [] * 1024
  ports = [0] * 12
  files = [0] * 8

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
        if memory[ip + 1] == 0:
          ip += 1
          if memory[ip + 1] == 0:
            ip += 1

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
        stack[-1] = unpack('=l', pack('=L', stack[-1] & 0xffffffff))[0]

      elif opcode == 17:   # -
        t = stack.pop()
        stack[-1] -= t
        stack[-1] = unpack('=l', pack('=L', stack[-1] & 0xffffffff))[0]

      elif opcode == 18:   # *
        t = stack.pop()
        stack[-1] *= t
        stack[-1] = unpack('=l', pack('=L', stack[-1] & 0xffffffff))[0]

      elif opcode == 19:   # /mod
        a = stack[-1]
        b = stack[-2]
        stack[-1], stack[-2] = rxDivMod( b, a )
        stack[-1] = unpack('=l', pack('=L', stack[-1] & 0xffffffff))[0]
        stack[-2] = unpack('=l', pack('=L', stack[-2] & 0xffffffff))[0]

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
        t = stack[-1]
        stack[-1] = ports[t]
        ports[t] = 0

      elif opcode == 29:   # out
        pi = stack.pop()
        ports[ pi ] = stack.pop()

      elif opcode == 30:   # wait
        # Only call if we have pending I/O
        if ports[0] == 0:
          ip = rxHandleDevices( ip, stack, address, ports, memory, files, inputs )

    else:
      address.append( ip )
      ip = memory[ip] - 1
      if memory[ip + 1] == 0:
        ip += 1
        if memory[ip + 1] == 0:
          ip += 1

    ip += 1


# -----------------------------------------------------------------------------
def run():
  cells = int(os.path.getsize('retroImage') / 4)
  inputs = [0] * 12
  inputs.append(0)

  f = open( 'retroImage', 'rb' )
  memory = list(struct.unpack( cells * 'i', f.read() ))
  f.close()

  remaining = 1000000 - cells
  memory.extend( [0] * remaining )

  if len(sys.argv) > 2:
    i = 0
    while i < len(sys.argv):
      if sys.argv[i] == "--with":
        i += 1
        inputs.append( open( sys.argv[i], 'r' ) )
      i += 1

  if set_termio:
    old = termios.tcgetattr(sys.stdin.fileno())
    t = termios.tcgetattr(sys.stdin.fileno())
    t[3] = t[3] & ~(termios.ICANON | termios.ECHO | termios.ISIG)
    t[0] = 0
    termios.tcsetattr(sys.stdin.fileno(), termios.TCSANOW, t)
  try: process( memory, inputs )
  except: pass
  if set_termio:
    termios.tcsetattr(sys.stdin.fileno(), termios.TCSANOW, old)

# -----------------------------------------------------------------------------
if __name__ == "__main__":
  run()
