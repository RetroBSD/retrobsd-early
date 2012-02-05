// Ngaro VM
// Original Ngaro Virtual Machine and Uki framework:
//   Copyright (C) 2008, 2009, 2010 Charles Childers
// Go port
//   Copyright 2009, 2010 JGL

/*
Ngaro virtual machines.

Ngaro is a portable 32-bit virtual machine / emulator for a dual
stack processor and various I/O devices. The instruction set
is concise (31 core instructions), and the basic I/O devices
are kept minimal. For more information see
	http://rx-core.org

In addition to normal ngaro features, this Go version
adds concurrency support: port 13 is used to launch
new cores, and the ports 1 and 2 can be used to
send to/receive from synchronous channels.

   chain: concurrency'
      : :go ( a- ) 1 13 out wait ; ( start new core with ip set to a )
      : go ( "-  ) ' :go ; ( parse a word and run it on a new core )
      : ->c ( xy- ) 2 out wait ; ( send x to channel y )
      : c-> ( x-y ) 1 out wait 1 in ; ( receive y from channel x )
      : delchan ( x- ) dup 1 out 2 out wait ; ( delete channel )
   ;chain

*/
package ngaro

import (
	"os"
)

const (
	// Limits
	stackDepth = 1024
	nports     = 64
)

// The VM type represents a Ngaro virtual machine
type VM struct {
	img    Image
	dump   string
	shrink bool
	ch     map[int32]chan int32
	file   map[int32]*os.File
	term   *Term
}

// New returns a Ngaro virtual machine with the image given
// in img. Dump files will be saved with the name dump.
// shrink indicates if images must be shrinked. term is a
// terminal type, obtained with NewTerm
func New(img Image, dump string, shrink bool, term *Term) *VM {
	vm := VM{
		img,
		dump,
		shrink,
		make(map[int32]chan int32),
		make(map[int32]*os.File),
		term,
	}
	return &vm
}

// Run executes a VM and blocks until it finishes.
func (vm *VM) Run() os.Error {
	return vm.core(0)
}

// Chan returns the channel with the given id. This channel
// can be used to communicate with any running core
func (vm *VM) Chan(id int32) chan int32 {
	if c, ok := vm.ch[id]; ok {
		return c
	}
	vm.ch[id] = make(chan int32)
	return vm.ch[id]
}
