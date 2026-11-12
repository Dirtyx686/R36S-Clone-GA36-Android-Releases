package main

import (
	"encoding/binary"
	"errors"
	"fmt"
	"regexp"
)

type winExtent struct {
	Disk          uint32
	Offset, Bytes int64
}
type winVolume struct {
	Path    string
	Extents []winExtent
}

var volumeGUIDPattern = regexp.MustCompile(`(?i)^\\\\\?\\Volume\{[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\}\\$`)

func parseWinExtents(b []byte) ([]winExtent, error) {
	if len(b) < 8 {
		return nil, errors.New("Volume extent header truncated")
	}
	n := int(binary.LittleEndian.Uint32(b))
	if n < 1 || n > 128 || len(b) < 8+n*24 {
		return nil, errors.New("Invalid volume extent count/length")
	}
	out := make([]winExtent, 0, n)
	for i := 0; i < n; i++ {
		at := 8 + i*24
		x := winExtent{binary.LittleEndian.Uint32(b[at:]), int64(binary.LittleEndian.Uint64(b[at+8:])), int64(binary.LittleEndian.Uint64(b[at+16:]))}
		if x.Offset < 0 || x.Bytes <= 0 || x.Offset > 1<<63-1-x.Bytes {
			return nil, errors.New("Invalid volume extent range")
		}
		out = append(out, x)
	}
	return out, nil
}
func targetWinVolumes(all []winVolume, disk uint32, capacity int64) ([]string, error) {
	out := []string{}
	seen := map[string]bool{}
	for _, v := range all {
		hit := false
		for _, x := range v.Extents {
			if x.Disk == disk {
				hit = true
			}
		}
		if !hit {
			continue
		}
		if !volumeGUIDPattern.MatchString(v.Path) || seen[v.Path] {
			return nil, errors.New("Invalid or duplicate target volume GUID")
		}
		for _, x := range v.Extents {
			if x.Disk != disk {
				return nil, errors.New("Target belongs to a multi-disk volume")
			}
			if x.Offset < 0 || x.Bytes <= 0 || x.Offset > capacity || x.Bytes > capacity-x.Offset {
				return nil, fmt.Errorf("Target volume exceeds card capacity")
			}
		}
		seen[v.Path] = true
		out = append(out, v.Path)
	}
	return out, nil
}
