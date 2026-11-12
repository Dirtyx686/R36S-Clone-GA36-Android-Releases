//go:build windows

package main

import (
	"fmt"
	"golang.org/x/sys/windows"
	"strings"
	"syscall"
	"unsafe"
)

// Enumerate actual Windows volume GUIDs, including volumes without drive letters.
// Never infer ordering or ownership from the drive letter or AccessPaths list.
func nativeWinVolumes() ([]winVolume, error) {
	k := windows.NewLazySystemDLL("kernel32.dll")
	first := k.NewProc("FindFirstVolumeW")
	next := k.NewProc("FindNextVolumeW")
	closeSearch := k.NewProc("FindVolumeClose")
	buffer := make([]uint16, 1024)
	h, _, e := first.Call(uintptr(unsafe.Pointer(&buffer[0])), uintptr(len(buffer)))
	if h == ^uintptr(0) {
		if e == syscall.Errno(18) {
			return []winVolume{}, nil
		}
		return nil, fmt.Errorf("Volume enumeration failed: %w", e)
	}
	defer closeSearch.Call(h)
	out := []winVolume{}
	for count := 0; count < 1024; count++ {
		path := syscall.UTF16ToString(buffer)
		if !volumeGUIDPattern.MatchString(path) {
			return nil, fmt.Errorf("Invalid Windows volume GUID")
		}
		p, _ := syscall.UTF16PtrFromString(path)
		kind, _, _ := k.NewProc("GetDriveTypeW").Call(uintptr(unsafe.Pointer(p)))
		// An optical drive has no writable SD-card extents. Other unknown
		// volume types still fail closed if physical ownership cannot be proven.
		if kind == 5 {
			goto nextVolume
		}
		{
			f, e := winOpen(strings.TrimSuffix(path, `\`), 0, 0)
			if e != nil {
				return nil, fmt.Errorf("Volume mapping unavailable for %s: %w", path, e)
			}
			data := make([]byte, 8+128*24)
			var n uint32
			e = syscall.DeviceIoControl(syscall.Handle(f.Fd()), 0x560000, nil, 0, &data[0], uint32(len(data)), &n, nil)
			f.Close()
			if e != nil {
				return nil, fmt.Errorf("Cannot prove physical volume ownership for %s: %w", path, e)
			}
			if n > uint32(len(data)) {
				return nil, fmt.Errorf("Invalid extent length")
			}
			ext, e := parseWinExtents(data[:n])
			if e != nil {
				return nil, e
			}
			out = append(out, winVolume{path, ext})
		}
	nextVolume:
		ok, _, e := next.Call(h, uintptr(unsafe.Pointer(&buffer[0])), uintptr(len(buffer)))
		if ok == 0 {
			if e == syscall.Errno(18) {
				return out, nil
			}
			return nil, fmt.Errorf("Volume enumeration interrupted: %w", e)
		}
	}
	return nil, fmt.Errorf("Unexpected volume count; refusing raw writes")
}
