//go:build windows

package main

import (
	"encoding/binary"
	"errors"
	"fmt"
	"golang.org/x/sys/windows"
	"os"
	"strconv"
	"strings"
	"sync"
	"syscall"
	"unsafe"
)

var shell = windows.NewLazySystemDLL("shell32.dll")

func listDisks(paths ...string) ([]Disk, error) { return inventoryDisks(paths...) }
func winOpen(path string, access, flags uint32) (*os.File, error) {
	p, e := syscall.UTF16PtrFromString(path)
	if e != nil {
		return nil, e
	}
	h, e := syscall.CreateFile(p, access, syscall.FILE_SHARE_READ|syscall.FILE_SHARE_WRITE, nil, syscall.OPEN_EXISTING, flags, 0)
	if e != nil {
		return nil, e
	}
	return os.NewFile(uintptr(h), path), nil
}
func winIo(f *os.File, code uint32, out []byte) error {
	var p *byte
	if len(out) > 0 {
		p = &out[0]
	}
	var n uint32
	return syscall.DeviceIoControl(syscall.Handle(f.Fd()), code, nil, 0, p, uint32(len(out)), &n, nil)
}
func winSize(f *os.File) (int64, error) {
	var b [8]byte
	e := winIo(f, 0x7405c, b[:])
	return int64(binary.LittleEndian.Uint64(b[:])), e
}
func openDevice(d Disk, paths ...string) (*Device, error) {
	if !d.Safe {
		return nil, errors.New("Geschütztes Laufwerk")
	}
	n, e := strconv.Atoi(d.ID)
	if e != nil || n < 0 || n > 999 {
		return nil, errors.New("Ungültige Datenträgernummer")
	}
	fresh, e := listDisks(paths...)
	if e != nil {
		return nil, e
	}
	found := false
	for _, x := range fresh {
		if x.Path == d.Path && x.Identity == d.Identity && x.Safe {
			found = true
		}
	}
	if !found {
		return nil, errors.New("Ziel hat sich geändert")
	}
	allVolumes, e := nativeWinVolumes()
	if e != nil {
		return nil, e
	}
	volumes, e := targetWinVolumes(allVolumes, uint32(n), d.Bytes)
	if e != nil {
		return nil, e
	}
	var locks []*os.File
	cleanup := func() {
		for _, f := range locks {
			winIo(f, 0x9001c, nil)
			f.Close()
		}
	}
	for _, v := range volumes {
		if !strings.HasPrefix(v, `\\?\Volume{`) || !strings.HasSuffix(v, `}\`) {
			cleanup()
			return nil, errors.New("Unbekannter Volumepfad")
		}
		f, e := winOpen(strings.TrimSuffix(v, `\`), syscall.GENERIC_READ|syscall.GENERIC_WRITE, 0)
		if e != nil {
			cleanup()
			return nil, e
		}
		locks = append(locks, f)
		if e = winIo(f, 0x90018, nil); e != nil {
			cleanup()
			return nil, fmt.Errorf("Karte ist noch in Benutzung: %w", e)
		}
		if e = winIo(f, 0x90020, nil); e != nil {
			cleanup()
			return nil, e
		}
	}
	f, e := winOpen(d.Path, syscall.GENERIC_READ|syscall.GENERIC_WRITE, 0x80000000|0x20000000)
	if e != nil {
		cleanup()
		return nil, e
	}
	s, e := winSize(f)
	if e != nil || s != d.Bytes {
		f.Close()
		cleanup()
		return nil, errors.New("Kartengröße hat sich geändert")
	}
	var once sync.Once
	var closed error
	closeFn := func() error { once.Do(func() { closed = f.Close(); cleanup() }); return closed }
	guard := func() error {
		size, e := winSize(f)
		if e != nil || size != d.Bytes {
			return errors.New("Karte getrennt oder geändert")
		}
		return nil
	}
	return &Device{Details: fmt.Sprintf("Windows: %d Ziel-Volumes gesperrt und ausgehängt; direkter Schreibzugriff vorbereitet.", len(volumes)), File: f, Guard: guard, Close: closeFn, Finish: func() error { return f.Sync() }, ReopenRead: func() (*os.File, error) {
		if e := f.Sync(); e != nil {
			return nil, e
		}
		return winOpen(d.Path, syscall.GENERIC_READ, 0x20000000)
	}}, nil
}
func listInboxes(exe string) ([]Inbox, error) { return nativeInboxes(exe) }
func inboxFree(root string) (uint64, error) {
	p, e := syscall.UTF16PtrFromString(root)
	if e != nil {
		return 0, e
	}
	var n uint64
	r, _, err := windows.NewLazySystemDLL("kernel32.dll").NewProc("GetDiskFreeSpaceExW").Call(uintptr(unsafe.Pointer(p)), uintptr(unsafe.Pointer(&n)), 0, 0)
	if r == 0 {
		return 0, err
	}
	return n, nil
}
func syncInbox(root string) error { return nativeSyncInbox(root) }
