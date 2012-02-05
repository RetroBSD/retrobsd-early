#!/usr/bin/env ruby

# Ngaro VM
# Copyright (c) 2010 - 2011, Charles Childers
# Copyright (c) 2011, Foucist
# -----------------------------------------------------------------------------
# RubyFORTH -- Copyright (C) 2007-8, Marc Simpson (GPL). 
# 
# Stack module
# 
# Relicensed for Retro under the ISC License (thanks Marc!)
# -----------------------------------------------------------------------------
class Stack
  def initialize(size)
    @contents = Array.new(size)
    @offset = -1
  end
  def nth(n)
    @contents[@offset - n]
  end
  def tos
    if @offset < 0
      puts "Error: stack underflow"
      false
    else 
      @contents[@offset]
    end
  end
  def push(n)
    @offset += 1
    @contents[@offset] = n
  end
  def pop
    if @offset < 0
      puts "Error: stack underflow"
      @offset = -1
      throw("toplevel")
      return
    end
    @offset -= 1
    @contents[@offset + 1]
  end
  def depth
    @offset + 1
  end 
  def swap
    x = self.pop ; y = self.pop
    self.push(x) ; self.push(y)
  end
end

@ip = 0                     # instruction pointer
@stack   = Stack.new(128)   # data stack
@address = Stack.new(1024)  # return stack
@memory  = {}               # simulated ram
@ports   = {}               # io ports
@files   = {}               # file handles
@inputs  = {}               # input sources
@isp     = 0                # input source pointer

# -----------------------------------------------------------------------------
# Workaround for keyboard input to let the same source run under
# Ruby 1.8 and 1.9
#
def get_char()
  a = 0
  if @isp == 0 then
    case RUBY_VERSION.split('.')[1]
      when "9" then a = STDIN.getbyte
      when "8" then a = STDIN.getc
    end
  else
    begin
      case RUBY_VERSION.split('.')[1]
        when "9" then a = @inputs[@isp].readbyte
        when "8" then a = @inputs[@isp].readchar
      end
    rescue EOFError
      a = 0
      @inputs[@isp].close
      @isp = @isp - 1
    end
  end
  return a
end
# -----------------------------------------------------------------------------
def do_env()
  req = @stack.pop
  dest = @stack.pop
  key = ''
  while @memory[req] != 0 do
    key = key + @memory[req].chr
    req = req + 1
  end
  req = ENV[key]
  @memory[dest] = 0
  req.each_byte do |c|
    @memory[dest] = c
    dest = dest + 1
    @memory[dest] = 0
  end
  return 0
end
# -----------------------------------------------------------------------------
def handle_devices()
  if @ports[0] == 1 then return end
  if @ports[0] == 0 then
    @ports[0] = 1
    case @ports[1]
    when 1 then
      @ports[1] = get_char()
      if @ports[1] == 13 then @ports[1] = 10 end
    end
  end
  if @ports[2] == 1 then
    if @stack.tos < 0 then
      puts "\e[H\e[2J"
    elsif @stack.tos > 0 and @stack.tos < 128 then
      print @stack.tos.chr
      STDOUT.flush
    end
    @stack.pop
    @ports[2] = 0
  end

  # File I/O
  case @ports[4]
  when  1 then   # save image
    f = File.new("retroImage", "wb")
    (0..@memory[3]).each do |i|
      f.write([@memory[i]].pack("i"))
    end
    f.close
    @ports[4] = 0
  when  2 then   # include file
    name = ""
    i = @stack.pop
    while @memory[i] != 0 do
      name = name + @memory[i].chr
      i = i + 1
    end
    if File::exists?(name) then
      @isp = @isp + 1
      @inputs[@isp] = File.open(name, 'r')
    end
    @ports[4] = 0
  when -1 then   # file open
    slot = 1
    i = 1
    while i < 8 do
      if @files[i] == 0 then
        slot = i
      end
      i = i + 1
    end
    mode = @stack.pop
    name = ""
    i = @stack.pop
    while @memory[i] != 0 do
      name = name + @memory[i].chr
      i = i + 1
    end
    @files[slot] = case mode
                   when 0 then if File::exists?(name) then File.open(name, 'r') else slot = 0 end
                   when 1 then File.open(name, 'w')
                   when 2 then File.open(name, 'a')
                   when 3 then if File::exists?(name) then File.open(name, 'r+') else slot = 0 end
                   end
    @ports[4] = slot
  when -2 then   # file read
    slot = @stack.pop
    case RUBY_VERSION.split('.')[1]
      when "9" then @ports[4] = @files[slot].readbyte
      when "8" then @ports[4] = @files[slot].readchar
    end
  when -3 then   # file write
    slot = @stack.pop
    key = @stack.pop
    @files[slot].write(key.chr)
    @ports[4] = 1
  when -4 then   # file close
    slot = @stack.pop
    @files[slot].close
    @files[slot] = 0
    @ports[4] = 0
  when -5 then   # file position
    slot = @stack.pop
    @ports[4] = @files[slot].pos
  when -6 then   # file seek
    slot = @stack.pop
    offset = @stack.pop
    @files[slot].seek(offset, IO::SEEK_SET)
    @ports[4] = 0
  when -7 then   # file size
    slot = @stack.pop
    offset = @files[slot].pos
    @files[slot].seek(0, IO::SEEK_END)
    @ports[4] = @files[slot].pos
    @files[slot].seek(offset, IO::SEEK_SET)
  when -8 then   # file delete
    name = ""
    i = @stack.pop
    while @memory[i] != 0 do
      name = name + @memory[i].chr
      i = i + 1
    end
    i = 0
    if File::exists?(name) then
      File.delete(name)
      i = 1
    end
    @ports[4] = i
  end

  # Capability Queries
  @ports[5] = case @ports[5]
              when -1 then 1000000 # memory size
              when -2 then 0       # canvas exists?
              when -3 then 0       # canvas width
              when -4 then 0       # canvas height
              when -5 then @stack.depth   # stack depth
              when -6 then @address.depth # address stack depth
              when -7 then 0       # mouse exists?
              when -8 then Time.now.to_i        # time
              when -9 then (@ip = 1000000) && 0 # exit vm
              when -10 then do_env # environment
              when -11 then 0      # console width
              when -12 then 0      # console height
              when -13 then 32     # bits per cell
              end
end
# -----------------------------------------------------------------------------
def process()
  case @memory[@ip] # opcode
  when 0 then 0  # nop
  when 1 then  # lit
    @ip = @ip + 1
    @stack.push(@memory[@ip])
  when 2 then  # dup
    @stack.push(@stack.tos)
  when 3 then  # drop
    @stack.pop
  when 4 then  # swap
    @stack.swap
  when 5 then  # push
    @address.push(@stack.pop)
  when 6 then  # pop
    @stack.push(@address.pop)
  when 7 then  # loop
    @stack.push(@stack.pop - 1)
    if @stack.tos != 0 and @stack.tos > -1 then
      @ip = @ip + 1
      @ip = @memory[@ip] - 1
    else
      @ip = @ip + 1
      @stack.pop
    end
  when 8 then  # jump
    @ip = @ip + 1
    @ip = @memory[@ip] - 1
    if @memory[@ip + 1] == 0 then @ip = @ip + 1 end
    if @memory[@ip + 1] == 0 then @ip = @ip + 1 end
  when 9 then  # return
    @ip = @address.pop
    if @memory[@ip + 1] == 0 then @ip = @ip + 1 end
    if @memory[@ip + 1] == 0 then @ip = @ip + 1 end
  when 10 then  # >= jump
    @ip = @ip + 1
    if @stack.nth(1) > @stack.tos then
      @ip = @memory[@ip] - 1
    end
    @stack.pop
    @stack.pop
  when 11 then  # <= jump
    @ip = @ip + 1
    if @stack.nth(1) < @stack.tos then
      @ip = @memory[@ip] - 1
    end
    @stack.pop
    @stack.pop
  when 12 then  # != jump
    @ip = @ip + 1
    if @stack.nth(1) != @stack.tos then
      @ip = @memory[@ip] - 1
    end
    @stack.pop
    @stack.pop
  when 13 then  # == jump
    @ip = @ip + 1
    if @stack.nth(1) == @stack.tos then
      @ip = @memory[@ip] - 1
    end
    @stack.pop
    @stack.pop
  when 14 then  # @
    @stack.push(@memory[@stack.pop])
  when 15 then  # !
    @memory[@stack.pop] = @stack.pop
  when 16 then  # +
    @stack.push(@stack.pop + @stack.pop)
  when 17 then  # -
    @stack.swap
    @stack.push(@stack.pop - @stack.pop)
  when 18 then  # *
    @stack.swap
    @stack.push(@stack.pop * @stack.pop)
  when 19 then  # /mod
    b = @stack.pop
    a = @stack.pop
    x = b.abs
    y = a.abs
    q = (y / x).floor
    r =  y % x
    if a < 0 and b < 0 then
      r *= -1
    elsif a > 0 and b < 0 then
      q *= -1
    elsif a < 0 and b > 0 then
      r *= -1
      q *= -1
    end
    @stack.push(r)
    @stack.push(q)
  when 20 then  # and
    a = @stack.pop
    b = @stack.pop
    @stack.push(b & a)
  when 21 then  # or
    a = @stack.pop
    b = @stack.pop
    @stack.push(b | a)
  when 22 then  # xor
    a = @stack.pop
    b = @stack.pop
    @stack.push(b ^ a)
  when 23 then  # <<
    a = @stack.pop
    b = @stack.pop
    @stack.push(b << a)
  when 24 then  # >>
    a = @stack.pop
    b = @stack.pop
    @stack.push(b >> a)
  when 25 then  # 0;
    if @stack.tos == 0 then
      @stack.pop
      @ip = @address.pop
    end
  when 26 then  # inc
    @stack.push(@stack.pop + 1)
  when 27 then  # dec
    @stack.push(@stack.pop - 1)
  when 28 then  # in
    a = @stack.pop
    @stack.push(@ports[a])
    @ports[a] = 0
  when 29 then  # out
    @ports[@stack.pop] = @stack.pop
  when 30 then  # wait
    handle_devices()
  else                     # call (implicit)
    @address.push(@ip)
    @ip = @memory[@ip] - 1
    if @memory[@ip + 1] == 0 then @ip = @ip + 1 end
    if @memory[@ip + 1] == 0 then @ip = @ip + 1 end
  end
end
# -----------------------------------------------------------------------------
@cells = (File.size("retroImage") / 4)
File.open("retroImage", "r") do |f|
  (0..(@cells-1)).each do |i|
    @memory[i] = f.sysread(4).unpack("i")[0]
  end
end
# -----------------------------------------------------------------------------
(@cells..1000000).each do |i|
  @memory[i] = 0
end
(0..12).each do |i|
  @inputs[i] = 0
  @ports[i] = 0
end
# -----------------------------------------------------------------------------
ARGV.each do |a|
  if a != "--with" then
    @isp = @isp + 1
    @inputs[@isp] = File.open(a, 'r')
  end
end
# -----------------------------------------------------------------------------
while @ip < 1000000 do
  process()
  @ip = @ip + 1
end
