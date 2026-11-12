// Copyright 2026 GA36 project. Original flasher code: MIT.
package main

import (
	"compress/gzip"
	"context"
	"crypto/sha256"
	"encoding/hex"
	"errors"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strings"
	"unsafe"
)

type Manifest struct {
	Format         string `json:"image_format,omitempty"`
	PayloadBytes   int64  `json:"payload_bytes,omitempty"`
	PayloadSHA     string `json:"payload_sha256,omitempty"`
	DescriptorSHA  string `json:"descriptor_sha256,omitempty"`
	NotForFlashing bool   `json:"not_for_flashing,omitempty"`
	Layout         string `json:"layout,omitempty"`
	OS             string `json:"os,omitempty"`
	Version        string `json:"version"`
	File           string `json:"file"`
	CompressedSHA  string `json:"compressed_sha256"`
	ImageSHA       string `json:"image_sha256"`
	HeaderSHA      string `json:"header_sha256"`
	ImageBytes     int64  `json:"image_bytes"`
	Board          string `json:"board"`
}
type Disk struct {
	Path, Model, Serial, ID, Bus, Identity, Seq string
	Bytes                                       int64
	Safe                                        bool
	Raw                                         bool
	Mounts                                      []string
	Volumes                                     []string
}
type Device struct {
	Details    string
	File       *os.File
	Guard      func() error
	ReopenRead func() (*os.File, error)
	Close      func() error
	Finish     func() error
}

func validManifest(m Manifest) error {
	if m.Format != "" && m.Format != compactFormat {
		return errors.New("Unsupported image format")
	}
	if m.Format == compactFormat && (!isDual(m) || m.PayloadBytes <= 0 || m.PayloadBytes > m.ImageBytes || m.PayloadBytes%4096 != 0 || !validSHA(m.PayloadSHA) || !validSHA(m.DescriptorSHA)) {
		return errors.New("Invalid compact image manifest")
	}
	if m.NotForFlashing {
		return errors.New("Image failed device boot test; use the new dual-boot candidate")
	}
	if m.ImageBytes < 4096 || m.ImageBytes%4096 != 0 || m.File != filepath.Base(m.File) || strings.ContainsAny(m.File, "/\\") {
		return errors.New("Ungültiges Image-Paket")
	}
	for _, h := range []string{m.ImageSHA, m.CompressedSHA, m.HeaderSHA} {
		b, e := hex.DecodeString(h)
		if e != nil || len(b) != 32 {
			return errors.New("Image-Paket ist noch nicht freigegeben")
		}
	}
	return nil
}
func eligible(d Disk, m Manifest) bool {
	return d.Safe && d.Bytes >= m.ImageBytes && d.Bytes <= 2*1024*1024*1024*1024
}
func hashFile(f *os.File) (string, error) {
	if _, e := f.Seek(0, 0); e != nil {
		return "", e
	}
	h := sha256.New()
	if _, e := io.Copy(h, f); e != nil {
		return "", e
	}
	return hex.EncodeToString(h.Sum(nil)), nil
}
func validateImage(f *os.File, m Manifest) error {
	if e := validManifest(m); e != nil {
		return e
	}
	h, e := hashFile(f)
	if e != nil {
		return e
	}
	if h != m.CompressedSHA {
		return errors.New("Image-Prüfsumme stimmt nicht. Nichts wurde geschrieben.")
	}
	_, e = f.Seek(0, 0)
	return e
}

// A page-aligned buffer supports direct disk I/O on Linux and Windows.
func alignedBuffer(n int) []byte {
	b := make([]byte, n+4096)
	off := int((4096 - uintptr(unsafe.Pointer(&b[0]))%4096) % 4096)
	return b[off : off+n]
}
func writeFull(w io.Writer, b []byte) error {
	for len(b) > 0 {
		n, e := w.Write(b)
		if e != nil {
			return e
		}
		if n <= 0 || n > len(b) {
			return io.ErrShortWrite
		}
		b = b[n:]
	}
	return nil
}
func flash(ctx context.Context, img *os.File, m Manifest, d *Device, progress func(string, int64, int64)) error {
	if m.Format == compactFormat {
		return flashCompact(ctx, img, m, d, progress)
	}
	if e := validateImage(img, m); e != nil {
		return e
	}
	if e := d.Guard(); e != nil {
		return e
	}
	gz, e := gzip.NewReader(img)
	if e != nil {
		return e
	}
	defer gz.Close()
	gz.Multistream(false)
	if _, e = d.File.Seek(0, 0); e != nil {
		return e
	}
	buf := alignedBuffer(transferBlockBytes)
	h := sha256.New()
	var at int64
	for at < m.ImageBytes {
		if e = ctx.Err(); e != nil {
			return e
		}
		if at%(64*1024*1024) == 0 {
			if e = d.Guard(); e != nil {
				return e
			}
		}
		n := int64(len(buf))
		if n > m.ImageBytes-at {
			n = m.ImageBytes - at
		}
		if _, e = io.ReadFull(gz, buf[:n]); e != nil {
			return fmt.Errorf("Image endet zu früh: %w", e)
		}
		if e = writeFull(d.File, buf[:n]); e != nil {
			return fmt.Errorf("Schreibfehler bei Byte %d: %w", at, e)
		}
		h.Write(buf[:n])
		at += n
		progress("Schreiben", at, m.ImageBytes)
	}
	var extra [1]byte
	n, e := gz.Read(extra[:])
	if n != 0 || e != io.EOF {
		return errors.New("Image-Länge oder GZIP-Abschluss ungültig")
	}
	if hex.EncodeToString(h.Sum(nil)) != m.ImageSHA {
		return errors.New("Entpackte Image-Prüfsumme stimmt nicht")
	}
	if e = d.File.Sync(); e != nil {
		return e
	}
	if e = d.Guard(); e != nil {
		return e
	}
	r, e := d.ReopenRead()
	if e != nil {
		return e
	}
	defer r.Close()
	h = sha256.New()
	at = 0
	for at < m.ImageBytes {
		if e = ctx.Err(); e != nil {
			return e
		}
		if at%(64*1024*1024) == 0 {
			if e = d.Guard(); e != nil {
				return e
			}
		}
		n := int64(len(buf))
		if n > m.ImageBytes-at {
			n = m.ImageBytes - at
		}
		if _, e = io.ReadFull(r, buf[:n]); e != nil {
			return fmt.Errorf("Lesefehler bei der Prüfung: %w", e)
		}
		h.Write(buf[:n])
		at += n
		progress("Zurücklesen und prüfen", at, m.ImageBytes)
	}
	if hex.EncodeToString(h.Sum(nil)) != m.ImageSHA {
		return errors.New("Schreibprüfung fehlgeschlagen. Karte nicht verwenden.")
	}
	return d.Guard()
}
func main() {
	if len(os.Args) > 1 && os.Args[1] == "--native-worker" {
		if e := nativeWorker(os.Stdin, os.Stdout); e != nil {
			fmt.Fprintln(os.Stderr, e)
			os.Exit(1)
		}
		return
	}
	if e := desktopMain(); e != nil {
		fmt.Fprintln(os.Stderr, e)
		os.Exit(1)
	}
}
