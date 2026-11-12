package main

import (
	"encoding/binary"
	"testing"
)

func TestNativeVolumeOwnership(t *testing.T) {
	guid := `\\?\Volume{01234567-89ab-cdef-0123-456789abcdef}\`
	other := `\\?\Volume{01234567-89ab-cdef-0123-456789abcdee}\`
	b := make([]byte, 32)
	binary.LittleEndian.PutUint32(b, 1)
	binary.LittleEndian.PutUint32(b[8:], 2)
	binary.LittleEndian.PutUint64(b[16:], 1048576)
	binary.LittleEndian.PutUint64(b[24:], 4294967296)
	ext, e := parseWinExtents(b)
	if e != nil {
		t.Fatal(e)
	}
	paths, e := targetWinVolumes([]winVolume{{other, []winExtent{{0, 0, 100000}}}, {guid, ext}}, 2, 64000000000)
	if e != nil || len(paths) != 1 || paths[0] != guid {
		t.Fatal(paths, e)
	}
	if _, e = targetWinVolumes([]winVolume{{guid, []winExtent{{2, 0, 100}, {0, 0, 100}}}}, 2, 64000000000); e == nil {
		t.Fatal("spanned disk admitted")
	}
	if _, e = targetWinVolumes([]winVolume{{guid, []winExtent{{2, 1000, 1000}}}}, 2, 1500); e == nil {
		t.Fatal("beyond capacity admitted")
	}
	if _, e = targetWinVolumes([]winVolume{{guid, ext}, {guid, ext}}, 2, 64000000000); e == nil {
		t.Fatal("duplicate admitted")
	}
	if _, e = targetWinVolumes([]winVolume{{`C:\`, ext}}, 2, 64000000000); e == nil {
		t.Fatal("drive letter accepted as GUID")
	}
	if _, e = parseWinExtents(b[:20]); e == nil {
		t.Fatal("truncated extent admitted")
	}
	binary.LittleEndian.PutUint32(b, 129)
	if _, e = parseWinExtents(b); e == nil {
		t.Fatal("overflow count admitted")
	}
}
