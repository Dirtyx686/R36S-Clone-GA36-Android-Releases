package main

import (
	"bytes"
	"compress/gzip"
	"context"
	"crypto/sha256"
	"encoding/binary"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"os"
)

const compactFormat = "GA36-extents-v1"
const compactMagic = "GA36-EXTENTS-1\x00\x00"
const transferBlockBytes = 4 * 1024 * 1024

type imageExtent struct {
	Offset int64  `json:"offset"`
	Length int64  `json:"length"`
	SHA    string `json:"sha256"`
}
type compactPlan struct {
	Schema       string        `json:"schema"`
	Layout       string        `json:"layout"`
	TargetBytes  int64         `json:"target_bytes"`
	BlockBytes   int64         `json:"block_bytes"`
	PayloadBytes int64         `json:"payload_bytes"`
	ReferenceSHA string        `json:"reference_image_sha256"`
	Extents      []imageExtent `json:"extents"`
}

func validSHA(s string) bool { b, e := hex.DecodeString(s); return e == nil && len(b) == 32 }
func rangeCovered(extents []imageExtent, start, end int64) bool {
	at := start
	for _, e := range extents {
		if e.Offset+e.Length <= at {
			continue
		}
		if e.Offset > at {
			return false
		}
		if e.Offset+e.Length > at {
			at = e.Offset + e.Length
		}
		if at >= end {
			return true
		}
	}
	return at >= end
}
func validateCompactPlan(p compactPlan, m Manifest) error {
	if !isDual(m) || m.Format != compactFormat || p.Schema != compactFormat || p.Layout != m.Layout || p.TargetBytes != m.ImageBytes || p.BlockBytes != 4096 || p.ReferenceSHA != m.ImageSHA || p.PayloadBytes != m.PayloadBytes || len(p.Extents) == 0 || len(p.Extents) > 16384 {
		return errors.New("Unsupported compact image profile")
	}
	var end, total int64
	for _, e := range p.Extents {
		if e.Offset < end || e.Offset < 0 || e.Length <= 0 || e.Offset%4096 != 0 || e.Length%4096 != 0 || e.Offset >= p.TargetBytes || e.Length > p.TargetBytes-e.Offset || !validSHA(e.SHA) {
			return errors.New("Compact extent overlaps, is unaligned or exceeds image bounds")
		}
		if total > p.TargetBytes-e.Length {
			return errors.New("Compact payload overflow")
		}
		total += e.Length
		end = e.Offset + e.Length
	}
	if total != p.PayloadBytes || total <= 0 || total > p.TargetBytes {
		return errors.New("Compact payload length mismatch")
	}
	// Only free blocks inside these exact two ext4 regions may be omitted.
	// Copy every byte outside them, including unknown boot gaps and the tail.
	for _, r := range [][2]int64{{0, 658505728}, {1732268032, 3221225472}, {63892881408, dualImageBytes}, {658505728, 658509824}, {3221225472, 3221229568}} {
		if !rangeCovered(p.Extents, r[0], r[1]) {
			return errors.New("Compact image omits a required boot, filesystem-header or tail range")
		}
	}
	return nil
}
func readCompact(img *os.File, m Manifest) (*gzip.Reader, compactPlan, error) {
	var p compactPlan
	if _, e := img.Seek(0, 0); e != nil {
		return nil, p, e
	}
	z, e := gzip.NewReader(img)
	if e != nil {
		return nil, p, e
	}
	z.Multistream(false)
	fail := func(err error) (*gzip.Reader, compactPlan, error) { z.Close(); return nil, p, err }
	var magic [16]byte
	if _, e = io.ReadFull(z, magic[:]); e != nil {
		return fail(e)
	}
	if string(magic[:]) != compactMagic {
		return fail(errors.New("Invalid compact image magic"))
	}
	var size [4]byte
	if _, e = io.ReadFull(z, size[:]); e != nil {
		return fail(e)
	}
	n := binary.LittleEndian.Uint32(size[:])
	if n == 0 || n > 2*1024*1024 {
		return fail(errors.New("Compact descriptor too large"))
	}
	desc := make([]byte, n)
	if _, e = io.ReadFull(z, desc); e != nil {
		return fail(e)
	}
	h := sha256.Sum256(desc)
	if hex.EncodeToString(h[:]) != m.DescriptorSHA {
		return fail(errors.New("Compact descriptor checksum mismatch"))
	}
	decoder := json.NewDecoder(bytes.NewReader(desc))
	decoder.DisallowUnknownFields()
	if e = decoder.Decode(&p); e != nil {
		return fail(e)
	}
	var extra any
	if decoder.Decode(&extra) != io.EOF {
		return fail(errors.New("Trailing compact descriptor data"))
	}
	if e = validateCompactPlan(p, m); e != nil {
		return fail(e)
	}
	return z, p, nil
}
func inspectCompactHeader(img *os.File, m Manifest) error {
	z, _, e := readCompact(img, m)
	if e != nil {
		return e
	}
	defer z.Close()
	b := make([]byte, 4096)
	if _, e = io.ReadFull(z, b); e != nil {
		return e
	}
	return validateLayoutHeader(m, b)
}

func verifyCompactFile(ctx context.Context, path string, m Manifest, progress func(nativeEvent)) error {
	f, e := os.Open(path)
	if e != nil {
		return e
	}
	defer f.Close()
	if e = ctx.Err(); e != nil {
		return e
	}
	if e = validateImage(f, m); e != nil {
		return e
	}
	z, p, e := readCompact(f, m)
	if e != nil {
		return e
	}
	defer z.Close()
	buf := make([]byte, transferBlockBytes)
	all := sha256.New()
	var at int64
	for i, ex := range p.Extents {
		left := ex.Length
		h := sha256.New()
		for left > 0 {
			if e = ctx.Err(); e != nil {
				return e
			}
			n := int64(len(buf))
			if left < n {
				n = left
			}
			if _, e = io.ReadFull(z, buf[:n]); e != nil {
				return e
			}
			if i == 0 && left == ex.Length {
				if n < 4096 {
					return errors.New("Compact prefix too short")
				}
				if e = validateLayoutHeader(m, buf[:4096]); e != nil {
					return e
				}
			}
			h.Write(buf[:n])
			all.Write(buf[:n])
			left -= n
			at += n
			progress(nativeEvent{Phase: "Verifying compact payload", Current: at, Total: p.PayloadBytes})
		}
		if hex.EncodeToString(h.Sum(nil)) != ex.SHA {
			return errors.New("Compact extent checksum mismatch")
		}
	}
	var extra [1]byte
	if n, err := z.Read(extra[:]); n != 0 || err != io.EOF {
		return errors.New("Compact length/footer mismatch")
	}
	if at != p.PayloadBytes || hex.EncodeToString(all.Sum(nil)) != m.PayloadSHA {
		return errors.New("Compact payload checksum mismatch")
	}
	return nil
}
func flashCompact(ctx context.Context, img *os.File, m Manifest, d *Device, progress func(string, int64, int64)) error {
	if e := validateImage(img, m); e != nil {
		return e
	}
	if e := ctx.Err(); e != nil {
		return e
	}
	z, p, e := readCompact(img, m)
	if e != nil {
		return e
	}
	defer z.Close()
	return transferCompact(ctx, z, p, m, d, progress)
}

// The caller validates the complete descriptor before entering this function.
// Only its explicitly listed ranges are touched. Omitted filesystem-free space
// is neither read as assumed-zero data nor used to satisfy verification.
func transferCompact(ctx context.Context, z io.Reader, p compactPlan, m Manifest, d *Device, progress func(string, int64, int64)) error {
	if e := ctx.Err(); e != nil {
		return e
	}
	if e := d.Guard(); e != nil {
		return e
	}
	buf := alignedBuffer(transferBlockBytes)
	whole := sha256.New()
	var at int64
	var nextGuard int64
	for i, ex := range p.Extents {
		if e := d.Guard(); e != nil {
			return e
		}
		if _, e := d.File.Seek(ex.Offset, 0); e != nil {
			return e
		}
		left := ex.Length
		h := sha256.New()
		for left > 0 {
			if e := ctx.Err(); e != nil {
				return e
			}
			n := int64(len(buf))
			if left < n {
				n = left
			}
			if _, e := io.ReadFull(z, buf[:n]); e != nil {
				return fmt.Errorf("Compact payload truncated: %w", e)
			}
			if i == 0 && left == ex.Length {
				if n < 4096 {
					return errors.New("Compact boot prefix too short")
				}
				if e := validateLayoutHeader(m, buf[:4096]); e != nil {
					return e
				}
			}
			if at >= nextGuard {
				if e := d.Guard(); e != nil {
					return e
				}
				nextGuard = at + 64*1024*1024
			}
			if e := writeFull(d.File, buf[:n]); e != nil {
				return fmt.Errorf("Compact write failed at target byte %d: %w", ex.Offset+ex.Length-left, e)
			}
			h.Write(buf[:n])
			whole.Write(buf[:n])
			left -= n
			at += n
			progress("Schreiben", at, p.PayloadBytes)
		}
		if hex.EncodeToString(h.Sum(nil)) != ex.SHA {
			return errors.New("Compact extent payload checksum mismatch")
		}
	}
	var extra [1]byte
	if n, e := z.Read(extra[:]); n != 0 || e != io.EOF {
		return errors.New("Compact payload length or gzip footer invalid")
	}
	if at != p.PayloadBytes || hex.EncodeToString(whole.Sum(nil)) != m.PayloadSHA {
		return errors.New("Compact payload checksum mismatch")
	}
	if e := ctx.Err(); e != nil {
		return e
	}
	progress("Synchronisieren", at, p.PayloadBytes)
	if e := d.File.Sync(); e != nil {
		return e
	}
	if e := d.Guard(); e != nil {
		return e
	}
	r, e := d.ReopenRead()
	if e != nil {
		return e
	}
	defer r.Close()
	whole = sha256.New()
	at = 0
	nextGuard = 0
	for _, ex := range p.Extents {
		if e := d.Guard(); e != nil {
			return e
		}
		if _, e := r.Seek(ex.Offset, 0); e != nil {
			return e
		}
		h := sha256.New()
		left := ex.Length
		for left > 0 {
			if e := ctx.Err(); e != nil {
				return e
			}
			n := int64(len(buf))
			if left < n {
				n = left
			}
			if at >= nextGuard {
				if e := d.Guard(); e != nil {
					return e
				}
				nextGuard = at + 64*1024*1024
			}
			if _, e = io.ReadFull(r, buf[:n]); e != nil {
				return fmt.Errorf("Compact readback failed: %w", e)
			}
			h.Write(buf[:n])
			whole.Write(buf[:n])
			left -= n
			at += n
			progress("Zurücklesen und prüfen", at, p.PayloadBytes)
		}
		if hex.EncodeToString(h.Sum(nil)) != ex.SHA {
			return errors.New("Written compact extent failed readback verification")
		}
	}
	if at != p.PayloadBytes || hex.EncodeToString(whole.Sum(nil)) != m.PayloadSHA {
		return errors.New("Compact full written-range verification failed")
	}
	return d.Guard()
}
