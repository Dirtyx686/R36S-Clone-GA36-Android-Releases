//go:build windows

package main

import (
	"encoding/binary"
	"encoding/hex"
	"errors"
	"fmt"
	"golang.org/x/sys/windows"
	"os"
	"path/filepath"
	"runtime"
	"strings"
	"syscall"
	"unsafe"
)

var kernel32 = windows.NewLazySystemDLL("kernel32.dll")

// One callback per process; NewCallback storage is not releasable on Windows.
var pageFilesCallback = syscall.NewCallback(func(paths *[]string, _ unsafe.Pointer, name *uint16) uintptr {
	v := windows.UTF16PtrToString(name)
	v = strings.TrimPrefix(v, `\??\`)
	volume := filepath.VolumeName(v)
	if len(volume) == 2 {
		*paths = append(*paths, volume+`\`)
	} else {
		*paths = append(*paths, v)
	}
	return 1
})

func diskInterfaces() ([]string, error) {
	setup := windows.NewLazySystemDLL("setupapi.dll")
	guid := syscall.GUID{Data1: 0x53f56307, Data2: 0xb6bf, Data3: 0x11d0, Data4: [8]byte{0x94, 0xf2, 0, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b}}
	h, _, e := setup.NewProc("SetupDiGetClassDevsW").Call(uintptr(unsafe.Pointer(&guid)), 0, 0, 0x12)
	if h == ^uintptr(0) {
		return nil, e
	}
	defer setup.NewProc("SetupDiDestroyDeviceInfoList").Call(h)
	type iface struct {
		Size     uint32
		Class    syscall.GUID
		Flags    uint32
		Reserved uintptr
	}
	var paths []string
	for i := uint32(0); i < 1024; i++ {
		data := iface{}
		data.Size = uint32(unsafe.Sizeof(data))
		ok, _, e := setup.NewProc("SetupDiEnumDeviceInterfaces").Call(h, 0, uintptr(unsafe.Pointer(&guid)), uintptr(i), uintptr(unsafe.Pointer(&data)))
		if ok == 0 {
			if e == syscall.Errno(259) {
				return paths, nil
			}
			return nil, e
		}
		var size uint32
		setup.NewProc("SetupDiGetDeviceInterfaceDetailW").Call(h, uintptr(unsafe.Pointer(&data)), 0, 0, uintptr(unsafe.Pointer(&size)), 0)
		if size < 8 || size > 65536 {
			return nil, errors.New("Invalid disk interface length")
		}
		buf := make([]byte, size)
		binary.LittleEndian.PutUint32(buf, 8)
		ok, _, e = setup.NewProc("SetupDiGetDeviceInterfaceDetailW").Call(h, uintptr(unsafe.Pointer(&data)), uintptr(unsafe.Pointer(&buf[0])), uintptr(size), 0, 0)
		if ok == 0 {
			return nil, e
		}
		words := unsafe.Slice((*uint16)(unsafe.Pointer(&buf[4])), (len(buf)-4)/2)
		paths = append(paths, syscall.UTF16ToString(words))
	}
	return nil, errors.New("Too many disk interfaces")
}
func nativeIO(f *os.File, code uint32, in []byte, size int) ([]byte, error) {
	out := make([]byte, size)
	var p *byte
	if len(in) > 0 {
		p = &in[0]
	}
	var n uint32
	e := syscall.DeviceIoControl(syscall.Handle(f.Fd()), code, p, uint32(len(in)), &out[0], uint32(len(out)), &n, nil)
	if e != nil {
		return nil, e
	}
	if n > uint32(len(out)) {
		return nil, errors.New("Invalid IOCTL response length")
	}
	return out[:n], nil
}
func nativeVolumeForPath(path string) (string, error) {
	p, e := syscall.UTF16PtrFromString(path)
	if e != nil {
		return "", e
	}
	buf := make([]uint16, 32768)
	ok, _, e := kernel32.NewProc("GetVolumePathNameW").Call(uintptr(unsafe.Pointer(p)), uintptr(unsafe.Pointer(&buf[0])), uintptr(len(buf)))
	if ok == 0 {
		return "", e
	}
	name := make([]uint16, 1024)
	ok, _, e = kernel32.NewProc("GetVolumeNameForVolumeMountPointW").Call(uintptr(unsafe.Pointer(&buf[0])), uintptr(unsafe.Pointer(&name[0])), uintptr(len(name)))
	if ok == 0 {
		return "", e
	}
	return syscall.UTF16ToString(name), nil
}
func protectWindowsDisks(paths []string) (map[uint32]bool, error) {
	protect := map[uint32]bool{}
	buf := make([]uint16, 32768)
	n, _, e := kernel32.NewProc("GetWindowsDirectoryW").Call(uintptr(unsafe.Pointer(&buf[0])), uintptr(len(buf)))
	if n == 0 || n >= uintptr(len(buf)) {
		return nil, fmt.Errorf("Cannot resolve Windows directory: %w", e)
	}
	paths = append(paths, syscall.UTF16ToString(buf), os.TempDir())
	if u := os.Getenv("USERPROFILE"); u != "" {
		paths = append(paths, u)
	}
	// Query active paging files directly; never infer them from a wildcard config.
	pagePaths := new([]string)
	ok, _, e := windows.NewLazySystemDLL("psapi.dll").NewProc("EnumPageFilesW").Call(pageFilesCallback, uintptr(unsafe.Pointer(pagePaths)))
	runtime.KeepAlive(pagePaths)
	if ok == 0 {
		return nil, fmt.Errorf("Cannot enumerate paging-file disks: %w", e)
	}
	paths = append(paths, (*pagePaths)...)
	// Protect the boot-manager volume even if Windows is on a different disk.
	var key syscall.Handle
	k, _ := syscall.UTF16PtrFromString(`SYSTEM\Setup`)
	if e := syscall.RegOpenKeyEx(syscall.HKEY_LOCAL_MACHINE, k, 0, syscall.KEY_READ, &key); e != nil {
		return nil, e
	}
	value, _ := syscall.UTF16PtrFromString("SystemPartition")
	var typ, size uint32
	e = syscall.RegQueryValueEx(key, value, nil, &typ, nil, &size)
	if e == nil && size > 0 {
		if typ != 1 || size > 65536 || size%2 != 0 {
			syscall.RegCloseKey(key)
			return nil, errors.New("Unknown boot-volume configuration")
		}
		raw := make([]byte, size)
		if e = syscall.RegQueryValueEx(key, value, nil, &typ, &raw[0], &size); e != nil {
			syscall.RegCloseKey(key)
			return nil, e
		}
		systemPart := syscall.UTF16ToString(unsafe.Slice((*uint16)(unsafe.Pointer(&raw[0])), len(raw)/2))
		all, err := nativeWinVolumes()
		if err != nil {
			syscall.RegCloseKey(key)
			return nil, err
		}
		found := false
		for _, v := range all {
			dosName, _ := syscall.UTF16PtrFromString(strings.TrimSuffix(strings.TrimPrefix(v.Path, `\\?\`), `\`))
			target := make([]uint16, 32768)
			n, _, _ := kernel32.NewProc("QueryDosDeviceW").Call(uintptr(unsafe.Pointer(dosName)), uintptr(unsafe.Pointer(&target[0])), uintptr(len(target)))
			if n > 0 && strings.EqualFold(syscall.UTF16ToString(target), systemPart) {
				for _, x := range v.Extents {
					protect[x.Disk] = true
				}
				found = true
			}
		}
		if !found {
			syscall.RegCloseKey(key)
			return nil, errors.New("Cannot prove boot-volume disk identity")
		}
	} else if e != nil && e != syscall.ERROR_FILE_NOT_FOUND {
		syscall.RegCloseKey(key)
		return nil, e
	}
	syscall.RegCloseKey(key)
	for _, path := range paths {
		if path == "" {
			continue
		}
		root, e := nativeVolumeForPath(path)
		if e != nil {
			return nil, fmt.Errorf("Cannot protect source/system path %s: %w", path, e)
		}
		f, e := winOpen(strings.TrimSuffix(root, `\`), 0, 0)
		if e != nil {
			return nil, e
		}
		data, e := nativeIO(f, 0x560000, nil, 8+128*24)
		f.Close()
		if e != nil {
			return nil, e
		}
		ext, e := parseWinExtents(data)
		if e != nil {
			return nil, e
		}
		for _, x := range ext {
			protect[x.Disk] = true
		}
	}
	return protect, nil
}
func inventoryDisks(paths ...string) ([]Disk, error) {
	protect, e := protectWindowsDisks(paths)
	if e != nil {
		return nil, e
	}
	links, e := diskInterfaces()
	if e != nil {
		return nil, e
	}
	out := []Disk{}
	seen := map[uint32]bool{}
	for _, link := range links {
		f, e := winOpen(link, syscall.GENERIC_READ, 0)
		if e != nil {
			continue
		}
		d, e := nativeIO(f, 0x2d1080, nil, 12)
		if e != nil || len(d) < 12 {
			f.Close()
			continue
		}
		number := binary.LittleEndian.Uint32(d[4:])
		if seen[number] || protect[number] {
			f.Close()
			continue
		}
		seen[number] = true
		descriptor, e := nativeIO(f, 0x2d1400, make([]byte, 12), 65536)
		if e != nil || len(descriptor) < 36 {
			f.Close()
			continue
		}
		bus := binary.LittleEndian.Uint32(descriptor[28:])
		if bus != 7 && bus != 12 && bus != 13 {
			f.Close()
			continue
		}
		attrs, e := nativeIO(f, 0x700f0, nil, 16)
		if e != nil || len(attrs) < 16 || binary.LittleEndian.Uint64(attrs[8:])&2 != 0 {
			f.Close()
			continue
		}
		size, e := winSize(f)
		if e != nil || size <= 0 {
			f.Close()
			continue
		}
		layout, e := nativeIO(f, 0x70050, nil, 65536)
		f.Close()
		if e != nil || len(layout) < 48 {
			continue
		}
		style := binary.LittleEndian.Uint32(layout)
		if style > 2 {
			continue
		}
		count := binary.LittleEndian.Uint32(layout[4:])
		if count > 128 || len(layout) < 48+int(count)*144 {
			continue
		}
		efi := false
		if style == 1 {
			for i := 0; i < int(count); i++ {
				p := layout[48+i*144+32 : 48+i*144+48]
				if hex.EncodeToString(p) == "28732ac11ff8d211ba4b00a0c93ec93b" {
					efi = true
				}
			}
		}
		if efi {
			continue
		}
		field := func(at int) string {
			off := int(binary.LittleEndian.Uint32(descriptor[at:]))
			if off < 36 || off >= len(descriptor) {
				return ""
			}
			b := descriptor[off:]
			if n := strings.IndexByte(string(b), 0); n >= 0 {
				b = b[:n]
			}
			return strings.TrimSpace(string(b))
		}
		model := strings.TrimSpace(field(12) + " " + field(16))
		serial := field(24)
		signature := hex.EncodeToString(layout[8:24])
		id := fmt.Sprint(number)
		identity := fmt.Sprintf("%s|%s|%s|%d|%d|%s", id, link, serial, size, style, signature)
		out = append(out, Disk{Path: `\\.\PhysicalDrive` + id, ID: id, Model: model, Serial: serial, Bytes: size, Bus: fmt.Sprint(bus), Identity: identity, Safe: true, Raw: style == 2})
	}
	return out, nil
}
func volumeInformation(root string) (label, fs string, err error) {
	p, e := syscall.UTF16PtrFromString(root)
	if e != nil {
		return "", "", e
	}
	l := make([]uint16, 256)
	f := make([]uint16, 256)
	var serial, maxComponent, flags uint32
	ok, _, e := kernel32.NewProc("GetVolumeInformationW").Call(uintptr(unsafe.Pointer(p)), uintptr(unsafe.Pointer(&l[0])), uintptr(len(l)), uintptr(unsafe.Pointer(&serial)), uintptr(unsafe.Pointer(&maxComponent)), uintptr(unsafe.Pointer(&flags)), uintptr(unsafe.Pointer(&f[0])), uintptr(len(f)))
	if ok == 0 {
		return "", "", e
	}
	return syscall.UTF16ToString(l), syscall.UTF16ToString(f), nil
}
func nativeInboxes(exe string) ([]Inbox, error) {
	disks, e := inventoryDisks(exe)
	if e != nil {
		return nil, e
	}
	all, e := nativeWinVolumes()
	if e != nil {
		return nil, e
	}
	out := []Inbox{}
	for _, d := range disks {
		for _, v := range all {
			if len(v.Extents) != 1 || fmt.Sprint(v.Extents[0].Disk) != d.ID || v.Extents[0].Offset != 1732268032 || v.Extents[0].Bytes != 4294967296 {
				continue
			}
			label, fs, e := volumeInformation(v.Path)
			if e != nil || label != "GA36_APPS" || fs != "FAT32" {
				continue
			}
			root := v.Path
			identity := d.Identity
			out = append(out, Inbox{Root: root, Description: d.Model + " · " + d.Path + " · GA36_APPS", Token: identity, Open: func() (string, func(), error) {
				fresh, e := inventoryDisks(exe)
				if e != nil {
					return "", nil, e
				}
				ok := false
				for _, x := range fresh {
					if x.Identity == identity {
						ok = true
					}
				}
				if !ok {
					return "", nil, errors.New("Card changed")
				}
				if e = checkInbox(root); e != nil {
					return "", nil, e
				}
				return root, func() {}, nil
			}})
		}
	}
	return out, nil
}
func nativeSyncInbox(root string) error {
	if !volumeGUIDPattern.MatchString(root) {
		v, e := nativeVolumeForPath(root)
		if e != nil {
			return e
		}
		root = v
	}
	f, e := winOpen(strings.TrimSuffix(root, `\`), syscall.GENERIC_READ|syscall.GENERIC_WRITE, 0)
	if e != nil {
		return e
	}
	defer f.Close()
	return f.Sync()
}

var _ = filepath.Separator
