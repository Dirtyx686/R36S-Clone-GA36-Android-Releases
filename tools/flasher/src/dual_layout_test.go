package main

import (
	"bytes"
	"encoding/json"
	"io"
	"os"
	"path/filepath"
	"testing"
)

func dualManifest() Manifest {
	return Manifest{OS: "7+11", Layout: "GA36-dual-v1", ImageBytes: dualImageBytes, HeaderSHA: dualHeaderSHA}
}
func TestDualProfileRequiresExactLayoutAndCapacity(t *testing.T) {
	m := dualManifest()
	if !supportedLayout(m) {
		t.Fatal("dual layout rejected")
	}
	for _, bad := range []Manifest{{OS: "7+11", ImageBytes: dualImageBytes}, {OS: "7", Layout: "GA36-dual-v1", ImageBytes: dualImageBytes}, {OS: "7+11", Layout: "GA36-dual-v1", ImageBytes: baseImageBytes}} {
		if supportedLayout(bad) {
			t.Fatal("ambiguous layout accepted", bad)
		}
	}
	if eligible(Disk{Safe: true, Bytes: 32000000000}, m) {
		t.Fatal("32GB accepted for dual")
	}
	for _, size := range []int64{dualImageBytes, 64000000000, 128000000000} {
		if !eligible(Disk{Safe: true, Bytes: size}, m) {
			t.Fatal(size)
		}
	}
	if e := validateLayoutHeader(m, headerFixture()); e == nil {
		t.Fatal("DOS MBR accepted as original dual prefix")
	}
}
func TestDualFinishPreservesEntireOriginalHeader(t *testing.T) {
	root := os.Getenv("GA36_SOURCE_ROOT")
	if root == "" {
		t.Skip("set GA36_SOURCE_ROOT for working prefix")
	}
	f, e := os.Open(filepath.Join(root, "Gemeinsam", "bootloader-prefix.raw"))
	if e != nil {
		t.Fatal(e)
	}
	defer f.Close()
	header := make([]byte, 4096)
	if _, e = io.ReadFull(f, header); e != nil {
		t.Fatal(e)
	}
	if header[510] == 0x55 && header[511] == 0xaa {
		t.Fatal("unexpected DOS MBR fixture")
	}
	for _, size := range []int64{dualImageBytes, 128000000000} {
		d, _ := testDevice(t)
		d.File.WriteAt(header, 0)
		if e = finishImageLayout(d, size, dualManifest()); e != nil {
			t.Fatal(e)
		}
		got := make([]byte, 4096)
		d.File.ReadAt(got, 0)
		if !bytes.Equal(header, got) {
			t.Fatal("original boot prefix changed")
		}
	}
	d, _ := testDevice(t)
	bad := append([]byte(nil), header...)
	bad[511] ^= 1
	d.File.WriteAt(bad, 0)
	if e = finishImageLayout(d, dualImageBytes, dualManifest()); e == nil {
		t.Fatal("changed boot header accepted")
	}
}
func TestActualDualImagePackage(t *testing.T) {
	root := os.Getenv("GA36_DUAL_ROM_ROOT")
	if root == "" {
		t.Skip("set GA36_DUAL_ROM_ROOT for dual image")
	}
	meta := filepath.Join(root, "DualBoot", "IMAGE-INFOS.json")
	b, e := os.ReadFile(meta)
	if e != nil {
		t.Fatal(e)
	}
	var m Manifest
	if e = json.Unmarshal(b, &m); e != nil {
		t.Fatal(e)
	}
	got, e := loadNativeImage(filepath.Join(filepath.Dir(meta), m.File), meta)
	if e != nil {
		t.Fatal(e)
	}
	if !isDual(got) {
		t.Fatal("not dual")
	}
}

func TestFailedDeviceImageIsNotOfferedForFlashing(t *testing.T) {
	m := dualManifest()
	m.NotForFlashing = true
	if validManifest(m) == nil {
		t.Fatal("failed hardware candidate accepted")
	}
}
