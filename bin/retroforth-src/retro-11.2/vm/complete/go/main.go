package main

import (
	"ngaro"
	"exec"
	"flag"
	"fmt"
	"io"
	"os"
)

var Usage = func() {
	fmt.Fprint(os.Stderr, `
Gonga usage:
	gonga [options] [image file]

Gonga is the Go version of the Ngaro virtual machine.

If no image file is specified in the command line
gongaImage will be loaded, retroImage if that fails.

Options:
`)
	flag.PrintDefaults()
}

var size = flag.Int("s", 50000, "image size")
var dump = flag.String("d", "gongaImage", "image dump file")
var shrink = flag.Bool("shrink", false, "shrink image dump file")
var tty = flag.Bool("t", true, "input / output is tty")

type withFiles []*os.File

func (wf *withFiles) String() string {
	return fmt.Sprint(*wf)
}

func (wf *withFiles) Set(value string) bool {
	if f, err := os.OpenFile(value, os.O_RDONLY, 0666); err == nil {
		nwf := append(*wf, f)
		wf = &nwf
		return true
	}
	return false
}

// Default terminal
func noClear(w io.Writer)                          {}
func vt100Dimensions() (width int32, height int32) { return 80, 24 }

// Tty terminal
func ttyClear(w io.Writer) { fmt.Fprint(w, "\033[2J\033[1;1H") }
func ttyDimensions() (width int32, height int32) {
	if err := exec.Command("/bin/stty", "-F", "/dev/tty", "size").Run(); err == nil {
//		fmt.Fscan(p.Stdout, &width, &height)
		return
	}
	return vt100Dimensions()
}

func main() {
	var wf withFiles
	flag.Var(&wf, "w", "input files")
	flag.Usage = Usage
	flag.Parse()

	var img []int32
	var err os.Error

	switch flag.NArg() {
	case 0:
		img, err = ngaro.Load("gongaImage", *size)
		if err != nil {
			img, err = ngaro.Load("retroImage", *size)
		}
	case 1:
		img, err = ngaro.Load(flag.Arg(0), *size)
	default:
		fmt.Fprintln(os.Stderr, "too many arguments")
		flag.Usage()
		os.Exit(2)
	}
	if err != nil {
		fmt.Fprintln(os.Stderr, "error starting gonga: ", err.String())
		os.Exit(1)
	}

	// Reverse wf and add os.Stdin
	rs := make([]io.Reader, 0, len(wf)+1)
	for i, _ := range wf {
		rs = append(rs, wf[len(wf)-1-i])
	}
	input := io.MultiReader(append(rs, os.Stdin)...)
	clr := noClear
	dim := vt100Dimensions
	if *tty { // Set raw mode
		exec.Command("/bin/stty", "-F", "/dev/tty", "-echo", "-icanon", "min", "1").Run()
		clr = ttyClear
		dim = ttyDimensions
	}

	// Run a new VM
	term := ngaro.NewTerm(clr, dim, input, os.Stdout)
	vm := ngaro.New(img, *dump, *shrink, term)
	err = vm.Run()
	if *tty { // Unset raw mode
		exec.Command("/bin/stty", "-F", "/dev/tty", "echo", "cooked").Run()
	}
	if err != nil {
		os.Exit(1)
	}
}
