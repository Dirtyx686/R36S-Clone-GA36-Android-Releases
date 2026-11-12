//go:build linux

package main

import (
	"bytes"
	"context"
	"os"
	"syscall"
	"testing"
)

func TestSystemAndImageMountProtection(t *testing.T) {
	for _, m := range []string{"/", "/boot", "/boot/efi", "/home", "/usr", "/var"} {
		if !protectedMount(m, "/run/media/user/image/image.gz") {
			t.Fatal(m)
		}
	}
	if !protectedMount("/run/media/user/image", "/run/media/user/image/image.gz") {
		t.Fatal("image source unprotected")
	}
	if protectedMount("/run/media/user/target", "/run/media/user/image/image.gz") {
		t.Fatal("target unnecessarily rejected")
	}
}

func TestCompactDirectWriteAndReadback(t *testing.T) {
	p, m, payload, d, path, _ := compactTransferFixture(t)
	direct, e := os.OpenFile(path, os.O_RDWR|syscall.O_DIRECT, 0)
	if e != nil {
		t.Fatal(e)
	}
	defer direct.Close()
	d.File = direct
	d.ReopenRead = func() (*os.File, error) {
		if e := direct.Sync(); e != nil {
			return nil, e
		}
		return os.OpenFile(path, os.O_RDONLY|syscall.O_DIRECT, 0)
	}
	if e = transferCompact(context.Background(), bytes.NewReader(payload), p, m, d, func(string, int64, int64) {}); e != nil {
		t.Fatal(e)
	}
}
func TestDirectReadbackOnTemporaryFile(t *testing.T) {
	f, m, _ := fixture(t)
	d, path := testDevice(t)
	d.ReopenRead = func() (*os.File, error) {
		if e := d.File.Sync(); e != nil {
			return nil, e
		}
		return os.OpenFile(path, os.O_RDONLY|syscall.O_DIRECT, 0)
	}
	if e := flash(context.Background(), f, m, d, func(string, int64, int64) {}); e != nil {
		t.Fatal(e)
	}
}
