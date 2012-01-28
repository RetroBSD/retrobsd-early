-- Ngaro VM for Lua 5.2 and Lua-JIT
-- Copyright (c) 2010 - 2011, Charles Childers
-- ----------------------------------------------------------------------------
-- Comment this out if using Lua 5.2:
local bit32 = require("bit")
-- ----------------------------------------------------------------------------
-- Variables
ip = 0                -- instruction pointer
sp = 0                -- stack pointer
rp = 0                -- return pointer
stack = {}            -- data stack
address = {}          -- return stack
memory = {}           -- simulated ram
ports = {}            -- io ports
fstack = {}           -- track open files
fnext = 0             -- file index
i  = 0                -- generic loop index
-- ----------------------------------------------------------------------------
-- Support Code
function rxGetFileSize(filename)
  fh = assert(io.open(filename, "rb"))
  len = assert(fh:seek("end"))
  fh:close()
  return len
end

function rxDecode(b1, b2, b3, b4)
  if not b4 then error("need four bytes to convert to int",2) end
  local n = b1 + b2*256 + b3*65536 + b4*16777216
  return rxAdjust(n)
end

function rxAdjust(n)
  return (n > 2147483647) and (n - 4294967296) or n
end

function rxWriteByte(s)
  if s == 0 then
    image:write('\0');
  else
    image:write(string.format("%c", s))
  end
end

function rxSaveImage()
  image = io.open('retroImage', 'wb')
  i = 0
  while i < memory[3] do
    rxWriteByte(math.floor(memory[i] % 256))
    rxWriteByte(math.floor(memory[i] / 256))
    rxWriteByte(math.floor(memory[i] / (256 * 256)))
    rxWriteByte(math.floor(memory[i] / (256 * 256 * 256)))
    i = i + 1
  end
  image:close()
end

function rxGetString()
  s = ""
  a = stack[sp]
  sp = sp - 1
  while true do
    s = s .. string.char(memory[a])
    a = a + 1
    if memory[a] == 0 then break end
  end
  return s
end
-- ----------------------------------------------------------------------------
-- simulated device handlers
function rxHandleConsoleDevice()
  if ports[1] == 1 then
    ports[1] = string.byte(io.read(1))
    if ports[1] == 13 then ports[1] = 10 end
  end
  if ports[2] == 1 then
    if stack[sp] < 0 then
      io.write(string.char(27) .. "[2J" .. string.char(27) .. "[1;1H")
    elseif stack[sp] > 0 and stack[sp] < 128 then
      io.write(string.char(stack[sp]))
    end
    sp = sp - 1
    ports[2] = 0
  end
end

function rxHandleFileDevice()
  if ports[4] == 1 then
    rxSaveImage()
    ports[4] = 0
  end
  if ports[4] == -1 then
    ports[4] = 0
    mode = stack[sp]
    sp = sp - 1
    name = rxGetString()
    if mode == 0 then
      fstack[fnext] = assert(io.open(name, "r"))
      ports[4] = fnext * -1
      fnext = fnext + 1
    end
    if mode == 1 then
      fstack[fnext] = assert(io.open(name, "w"))
      ports[4] = fnext * -1
      fnext = fnext + 1
    end
    if mode == 2 then
      fstack[fnext] = assert(io.open(name, "a"))
      ports[4] = fnext * -1
      fnext = fnext + 1
    end
  end
end

function rxHandleQueryDevice()
  if ports[5] == -1 then  -- image size
    ports[5] = 1000000
  end
  if ports[5] == -2 then  -- canvas exists?
    ports[5] = 0
  end
  if ports[5] == -3 then  --  console width
    ports[5] = 0
  end
  if ports[5] == -4 then  -- console height
    ports[5] = 0
  end
  if ports[5] == -5 then  -- stack depth
    ports[5] = sp
  end
  if ports[5] == -6 then  -- address stack depth
    ports[5] = rp
  end
  if ports[5] == -7 then  -- mouse exists?
    ports[5] = 0
  end
  if ports[5] == -8 then  -- time
    ports[5] = os.time()
  end
  if ports[5] == -9 then  -- shutdown vm
    ip = 1000000
    ports[5] = 0
  end
end

function rxHandleDevices()
  if ports[0] == 1 then return end
  if ports[0] == 0 then
    ports[0] = 1
    rxHandleConsoleDevice()
    rxHandleFileDevice()
    rxHandleQueryDevice()
  end
end
-- ----------------------------------------------------------------------------
-- process opcodes
function rxProcessOpcode()
  local a = 0
  local b = 0
  local c = 0
  local d = 0
  local opcode = memory[ip]
  if     opcode ==  0 then     -- nop
  elseif opcode ==  1 then     -- lit
    sp = sp + 1
    ip = ip + 1
    stack[sp] = memory[ip]
  elseif opcode ==  2 then     -- dup
    sp = sp + 1
    stack[sp] = stack[sp - 1]
  elseif opcode ==  3 then     -- drop
    sp = sp - 1
  elseif opcode ==  4 then     -- swap
    a = stack[sp]
    stack[sp] = stack[sp - 1]
    stack[sp - 1] = a
  elseif opcode ==  5 then     -- push
    rp = rp + 1
    address[rp] = stack[sp]
    sp = sp - 1
  elseif opcode ==  6 then     -- pop
    sp = sp + 1
    stack[sp] = address[rp]
    rp = rp - 1
  elseif opcode ==  7 then     -- loop
    stack[sp] = stack[sp] - 1
    if stack[sp] ~= 0 and stack[sp] > -1 then
      ip = ip + 1
      ip = memory[ip] - 1
    else
      ip = ip + 1
      sp = sp - 1
    end
  elseif opcode ==  8 then     -- jump
    ip = ip + 1
    ip = memory[ip] - 1
    if memory[ip + 1] == 0 then ip = ip + 1 end
    if memory[ip + 1] == 0 then ip = ip + 1 end
  elseif opcode ==  9 then     -- return
    ip = address[rp]
    rp = rp - 1
  elseif opcode == 10 then     -- > jump
    ip = ip + 1
    if stack[sp - 1] > stack[sp] then ip = memory[ip] - 1 end
    sp = sp - 2
  elseif opcode == 11 then     -- < jump
    ip = ip + 1
    if stack[sp - 1] < stack[sp] then ip = memory[ip] - 1 end
    sp = sp - 2
  elseif opcode == 12 then     -- != jump
    ip = ip + 1
    if stack[sp - 1] ~= stack[sp] then ip = memory[ip] - 1 end
    sp = sp - 2
  elseif opcode == 13 then     -- == jump
    ip = ip + 1
    if stack[sp - 1] == stack[sp] then ip = memory[ip] - 1 end
    sp = sp - 2
  elseif opcode == 14 then     -- @
    stack[sp] = memory[stack[sp]]
  elseif opcode == 15 then     -- !
    memory[stack[sp]] = stack[sp - 1]
    sp = sp - 2
  elseif opcode == 16 then     -- +
    stack[sp - 1] = stack[sp] + stack[sp - 1]
    sp = sp - 1
  elseif opcode == 17 then     -- -
    stack[sp - 1] = stack[sp - 1] - stack[sp]
    sp = sp - 1
  elseif opcode == 18 then     -- *
    stack[sp - 1] = stack[sp - 1] * stack[sp]
    sp = sp - 1
  elseif opcode == 19 then     -- /mod
    b = stack[sp]
    a = stack[sp - 1]
    x = math.abs(b)
    y = math.abs(a)
    q = rxAdjust(math.floor(y / x))
    r = rxAdjust(y % x)
    if a < 0 and b < 0 then
      r = r * -1
    elseif a > 0 and b < 0 then
      q = q * -1
    elseif a < 0 and b > 0 then
      r = r * -1
      q = q * -1
    end
    stack[sp] = q
    stack[sp - 1] = r
  elseif opcode == 20 then     -- and
    stack[sp - 1] = rxAdjust(bit32.band(stack[sp - 1], stack[sp]))
    sp = sp - 1
  elseif opcode == 21 then     -- or
    stack[sp - 1] = rxAdjust(bit32.bor(stack[sp - 1], stack[sp]))
    sp = sp - 1
  elseif opcode == 22 then     -- xor
    stack[sp - 1] = rxAdjust(bit32.bxor(stack[sp - 1], stack[sp]))
    sp = sp - 1
  elseif opcode == 23 then     -- <<
    stack[sp - 1] = rxAdjust(bit32.lshift(stack[sp - 1], stack[sp]))
    sp = sp - 1
  elseif opcode == 24 then     -- >>
    stack[sp - 1] = rxAdjust(bit32.rshift(stack[sp - 1], stack[sp]))
    sp = sp - 1
  elseif opcode == 25 then     -- 0;
    if stack[sp] == 0 then
      sp = sp - 1
      ip = address[rp]
      rp = rp - 1
    end
  elseif opcode == 26 then     -- inc
    stack[sp] = stack[sp] + 1
  elseif opcode == 27 then     -- dec
    stack[sp] = stack[sp] - 1
  elseif opcode == 28 then     -- in
    stack[sp] = ports[stack[sp]]
  elseif opcode == 29 then     -- out
    ports[stack[sp]] = stack[sp - 1]
    sp = sp - 2
  elseif opcode == 30 then     -- wait
    rxHandleDevices()
  else                         -- call (implicit)
    rp = rp + 1
    address[rp] = ip
    ip = memory[ip] - 1
    if memory[ip + 1] == 0 then ip = ip + 1 end
    if memory[ip + 1] == 0 then ip = ip + 1 end
  end
end
-- ----------------------------------------------------------------------------
-- Load retroImage into memory
-- \ cells = size of image file (in cells)
-- \ image = pointer to retroImage file
function rxLoadImage(file)
  cells = rxGetFileSize(file) / 4
  image = io.open(file, 'rb')
  while i < cells do
    memory[i] = rxDecode(image:read(4):byte(1,4))
    i = i + 1
  end
  image:close()
  while i < 1000000 do
    memory[i] = 0
    i = i + 1
  end
  i = 0
  while i < 12 do
    stack[i] = 0
    address[i] = 0
    ports[i] = 0
    i = i + 1
  end
  while i < 128 do
    stack[i] = 0
    address[i] = 0
    i = i + 1
  end
  while i < 1024 do
    address[i] = 0
    i = i + 1
  end
  ports[0] = 1
end

-- ----------------------------------------------------------------------------
-- Process the image
rxLoadImage("retroImage")
while ip < 1000000 do
  rxProcessOpcode()
  ip = ip + 1
end
