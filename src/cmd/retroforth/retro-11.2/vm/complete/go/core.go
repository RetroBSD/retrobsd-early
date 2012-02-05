package ngaro

import (
	"fmt"
	"os"
)

const (
	// Instruction set
	Nop = iota
	Lit
	Dup
	Drop
	Swap
	Push
	Pop
	Loop
	Jump
	Return
	GtJump
	LtJump
	NeJump
	EqJump
	Fetch
	Store
	Add
	Sub
	Mul
	Dinod
	And
	Or
	Xor
	ShL
	ShR
	ZeroExit
	Inc
	Dec
	In
	Out
	Wait
)

func (vm *VM) core(ip int32) (err os.Error) {
	var port [nports]int32
	var sp, rsp int
	var tos int32
	var data, addr [stackDepth]int32
	defer func() {
		if v := recover(); v != nil {
			err = os.NewError(fmt.Sprint(v))
		}
	}()
	for ; int(ip) < len(vm.img); ip++ {
		switch vm.img[ip] {
		case Nop:
		case Lit:
			sp++
			ip++
			data[sp] = vm.img[ip]
		case Dup:
			sp++
			data[sp] = tos
		case Drop:
			sp--
		case Swap:
			data[sp], data[sp-1] = data[sp-1], tos
		case Push:
			rsp++
			addr[rsp] = tos
			sp--
		case Pop:
			sp++
			data[sp] = addr[rsp]
			rsp--
		case Loop:
			data[sp]--
			if data[sp] != 0 && data[sp] > -1 {
				ip++
				ip = vm.img[ip] - 1
			} else {
				ip++
				sp--
			}
		case Jump:
			ip++
			ip = vm.img[ip] - 1
		case Return:
			ip = addr[rsp]
			rsp--
		case GtJump:
			ip++
			if data[sp-1] > tos {
				ip = vm.img[ip] - 1
			}
			sp = sp - 2
		case LtJump:
			ip++
			if data[sp-1] < tos {
				ip = vm.img[ip] - 1
			}
			sp = sp - 2
		case NeJump:
			ip++
			if data[sp-1] != tos {
				ip = vm.img[ip] - 1
			}
			sp = sp - 2
		case EqJump:
			ip++
			if data[sp-1] == tos {
				ip = vm.img[ip] - 1
			}
			sp = sp - 2
		case Fetch:
			data[sp] = vm.img[tos]
		case Store:
			vm.img[tos] = data[sp-1]
			sp = sp - 2
		case Add:
			data[sp-1] += tos
			sp--
		case Sub:
			data[sp-1] -= tos
			sp--
		case Mul:
			data[sp-1] *= tos
			sp--
		case Dinod:
			data[sp] = data[sp-1] / tos
			data[sp-1] = data[sp-1] % tos
		case And:
			data[sp-1] &= tos
			sp--
		case Or:
			data[sp-1] |= tos
			sp--
		case Xor:
			data[sp-1] ^= tos
			sp--
		case ShL:
			data[sp-1] <<= uint(tos)
			sp--
		case ShR:
			data[sp-1] >>= uint(tos)
			sp--
		case ZeroExit:
			if tos == 0 {
				sp--
				ip = addr[rsp]
				rsp--
			}
		case Inc:
			data[sp]++
		case Dec:
			data[sp]--
		case In:
			data[sp] = port[tos]
			port[tos] = 0
		case Out:
			port[0] = 0
			port[tos] = data[sp-1]
			sp = sp - 2
		case Wait:
			sp -= vm.wait(data[:sp+1], addr[:rsp+1], port[:])
		default:
			rsp++
			addr[rsp] = ip
			ip = vm.img[ip] - 1
		}
		tos = data[sp]
	}
	return
}
