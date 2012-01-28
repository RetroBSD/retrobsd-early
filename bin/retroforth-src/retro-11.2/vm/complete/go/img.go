package ngaro

import (
	"encoding/binary"
	"os"
)

// Ngaro Image
type Image []int32

// Load returns an Image of the given size
func Load(filename string, size int) (Image, os.Error) {
	f, err := os.OpenFile(filename, os.O_RDONLY, 0)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	img := make([]int32, size)
	// we cannot just use binary.Read because we don't know
	// the size of the file, and it will return an error if EOF is
	// found before reading the whole slice, so we read chunks
	// of 512 bytes and use binary.LittleEndian.Uint32
	buf := make([]byte, 512) // 512%4 == 0
	for i := 0; err == nil; {
		var n int
		if n, err = f.Read(buf); err != nil && err != os.EOF {
			return nil, err
		}
		for b := buf[:n]; len(b) > 0; b = b[4:] {
			if len(b) < 4 { // partial value, discard the image
				return nil, os.NewError("partial value in image file")
			}
			img[i] = int32(binary.LittleEndian.Uint32(b[:4]))
			if i++; i == len(img) {
				return img, nil
			}
		}
	}
	return img, nil
}

func (img Image) save(filename string, shrink bool) os.Error {
	f, err := os.OpenFile(filename, os.O_WRONLY|os.O_CREATE, 0666)
	if err != nil {
		return err
	}
	defer f.Close()
	if shrink {
		img = img[0:img[3]]
	}
	return binary.Write(f, binary.LittleEndian, img)
}

func (img Image) string(offset int32) string {
	off := int(offset)
	last := off
	for ; last < len(img) && img[last] != 0; last++ {
	}
	str := make([]byte, last-off)
	for i, _ := range str {
		str[i] = byte(img[off+i])
	}
	return string(str)
}
