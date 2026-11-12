package main

import (
	"bytes"
	"compress/gzip"
	"context"
	"encoding/binary"
	"encoding/json"
	"errors"
	"io"
	"os"
	"path/filepath"
	"testing"
)

func compactPlanFixture() (compactPlan, Manifest) {
	p := compactPlan{Schema: compactFormat, Layout: "GA36-dual-v1", TargetBytes: dualImageBytes, BlockBytes: 4096, ReferenceSHA: digest([]byte("reference")), Extents: []imageExtent{
		{0, 658509824, digest([]byte("first"))}, {1732268032, 3221229568 - 1732268032, digest([]byte("second"))}, {63892881408, 16777216, digest([]byte("tail"))}}}
	for _, e := range p.Extents {
		p.PayloadBytes += e.Length
	}
	m := dualManifest()
	m.Format = compactFormat
	m.PayloadBytes = p.PayloadBytes
	m.ImageSHA = p.ReferenceSHA
	m.PayloadSHA = digest([]byte("payload"))
	m.DescriptorSHA = digest([]byte("descriptor"))
	return p, m
}
func TestCompactPlanRetainsEveryNonFilesystemRange(t *testing.T) {
	p, m := compactPlanFixture()
	if e := validateCompactPlan(p, m); e != nil {
		t.Fatal(e)
	}
}
func TestCompactPlanRejectsUnsafeRanges(t *testing.T) {
	for _, name := range []string{"overlap", "negative", "unaligned", "zero", "outside", "missing-prefix", "missing-startup-super", "missing-pool-super", "missing-tail", "bad-hash", "wrong-reference", "wrong-layout", "wrong-length", "wrong-profile"} {
		t.Run(name, func(t *testing.T) {
			p, m := compactPlanFixture()
			switch name {
			case "overlap":
				p.Extents[1].Offset = 4096
			case "negative":
				p.Extents[0].Offset = -4096
			case "unaligned":
				p.Extents[1].Offset++
			case "zero":
				p.Extents[1].Length = 0
			case "outside":
				p.Extents[2].Length = 1 << 62
			case "missing-prefix":
				p.Extents[0].Offset = 4096
				p.Extents[0].Length -= 4096
				p.PayloadBytes -= 4096
				m.PayloadBytes = p.PayloadBytes
			case "missing-startup-super":
				p.Extents[0].Length -= 4096
				p.PayloadBytes -= 4096
				m.PayloadBytes = p.PayloadBytes
			case "missing-pool-super":
				p.Extents[1].Length -= 4096
				p.PayloadBytes -= 4096
				m.PayloadBytes = p.PayloadBytes
			case "missing-tail":
				p.Extents = p.Extents[:2]
				p.PayloadBytes -= 16777216
				m.PayloadBytes = p.PayloadBytes
			case "bad-hash":
				p.Extents[1].SHA = "bad"
			case "wrong-reference":
				p.ReferenceSHA = digest([]byte("other"))
			case "wrong-layout":
				p.Layout = "other"
			case "wrong-length":
				p.PayloadBytes += 4096
			case "wrong-profile":
				m.OS = "7"
			}
			if validateCompactPlan(p, m) == nil {
				t.Fatal("unsafe plan accepted")
			}
		})
	}
}
func compactPackage(t *testing.T, p compactPlan, m Manifest, payload []byte) (*os.File, Manifest) {
	t.Helper()
	desc, e := json.Marshal(p)
	if e != nil {
		t.Fatal(e)
	}
	var packed bytes.Buffer
	z := gzip.NewWriter(&packed)
	z.Write([]byte(compactMagic))
	binary.Write(z, binary.LittleEndian, uint32(len(desc)))
	z.Write(desc)
	z.Write(payload)
	z.Close()
	path := filepath.Join(t.TempDir(), "compact.img.gz")
	if e = os.WriteFile(path, packed.Bytes(), 0600); e != nil {
		t.Fatal(e)
	}
	m.File = filepath.Base(path)
	m.CompressedSHA = digest(packed.Bytes())
	m.DescriptorSHA = digest(desc)
	f, e := os.Open(path)
	if e != nil {
		t.Fatal(e)
	}
	t.Cleanup(func() { f.Close() })
	return f, m
}
func TestCompactMalformedDescriptorDoesNotWrite(t *testing.T) {
	p, m := compactPlanFixture()
	p.Extents[1].Offset = 4096
	f, m := compactPackage(t, p, m, nil)
	d, path := testDevice(t)
	before, _ := os.ReadFile(path)
	if e := flash(context.Background(), f, m, d, func(string, int64, int64) {}); e == nil {
		t.Fatal("bad compact map accepted")
	}
	after, _ := os.ReadFile(path)
	if !bytes.Equal(before, after) {
		t.Fatal("wrote before descriptor validation")
	}
}
func TestCompactDescriptorHashDoesNotWrite(t *testing.T) {
	p, m := compactPlanFixture()
	f, m := compactPackage(t, p, m, nil)
	m.DescriptorSHA = digest([]byte("wrong"))
	d, path := testDevice(t)
	before, _ := os.ReadFile(path)
	if e := flash(context.Background(), f, m, d, func(string, int64, int64) {}); e == nil {
		t.Fatal("bad descriptor hash accepted")
	}
	after, _ := os.ReadFile(path)
	if !bytes.Equal(before, after) {
		t.Fatal("wrote corrupt descriptor")
	}
}
func TestCompactRejectsWrongBootPrefixBeforeWrite(t *testing.T) {
	p, m := compactPlanFixture()
	f, m := compactPackage(t, p, m, make([]byte, transferBlockBytes))
	d, path := testDevice(t)
	before, _ := os.ReadFile(path)
	if e := flash(context.Background(), f, m, d, func(string, int64, int64) {}); e == nil {
		t.Fatal("wrong original prefix accepted")
	}
	after, _ := os.ReadFile(path)
	if !bytes.Equal(before, after) {
		t.Fatal("wrote before boot-prefix check")
	}
}
func compactTransferFixture(t *testing.T) (compactPlan, Manifest, []byte, *Device, string, []byte) {
	t.Helper()
	root := os.Getenv("GA36_SOURCE_ROOT")
	if root == "" {
		t.Skip("GA36_SOURCE_ROOT supplies the exact original prefix for transfer tests")
	}
	f, e := os.Open(filepath.Join(root, "Gemeinsam/bootloader-prefix.raw"))
	if e != nil {
		t.Fatal(e)
	}
	head := make([]byte, 4096)
	_, e = io.ReadFull(f, head)
	f.Close()
	if e != nil {
		t.Fatal(e)
	}
	first := append(head, bytes.Repeat([]byte{0x35}, 1024*1024)...)
	last := make([]byte, 8192)
	payload := append(append([]byte(nil), first...), last...)
	p := compactPlan{PayloadBytes: int64(len(payload)), Extents: []imageExtent{{0, int64(len(first)), digest(first)}, {4 * 1024 * 1024, int64(len(last)), digest(last)}}}
	m := dualManifest()
	m.Format = compactFormat
	m.PayloadBytes = p.PayloadBytes
	m.PayloadSHA = digest(payload)
	path := filepath.Join(t.TempDir(), "dirty-target.img")
	dirty := bytes.Repeat([]byte{0xab}, 6*1024*1024)
	if e = os.WriteFile(path, dirty, 0600); e != nil {
		t.Fatal(e)
	}
	target, e := os.OpenFile(path, os.O_RDWR, 0)
	if e != nil {
		t.Fatal(e)
	}
	t.Cleanup(func() { target.Close() })
	d := &Device{File: target, Guard: func() error { return nil }, ReopenRead: func() (*os.File, error) { return os.Open(path) }, Close: target.Close, Finish: target.Sync}
	return p, m, payload, d, path, dirty
}
func TestCompactWritesAllocatedZerosAndPreservesFreeGaps(t *testing.T) {
	p, m, payload, d, path, dirty := compactTransferFixture(t)
	var phase string
	var count int64
	if e := transferCompact(context.Background(), bytes.NewReader(payload), p, m, d, func(s string, n, total int64) {
		phase = s
		count = n
		if total != p.PayloadBytes {
			t.Fatal("progress counts full card instead of payload")
		}
	}); e != nil {
		t.Fatal(e)
	}
	got, _ := os.ReadFile(path)
	n := p.Extents[0].Length
	end := p.Extents[1].Offset + p.Extents[1].Length
	if !bytes.Equal(got[:n], payload[:n]) || !bytes.Equal(got[p.Extents[1].Offset:end], payload[n:]) {
		t.Fatal("extent data differs")
	}
	if !bytes.Equal(got[n:p.Extents[1].Offset], dirty[n:p.Extents[1].Offset]) || !bytes.Equal(got[end:], dirty[end:]) {
		t.Fatal("unlisted free space changed")
	}
	if phase != "Zurücklesen und prüfen" || count != p.PayloadBytes {
		t.Fatal("incomplete readback progress")
	}
}
func TestCompactDetectsSecondExtentReadbackCorruption(t *testing.T) {
	p, m, payload, d, path, _ := compactTransferFixture(t)
	d.ReopenRead = func() (*os.File, error) {
		if _, e := d.File.WriteAt([]byte{0xff}, p.Extents[1].Offset); e != nil {
			return nil, e
		}
		return os.Open(path)
	}
	if transferCompact(context.Background(), bytes.NewReader(payload), p, m, d, func(string, int64, int64) {}) == nil {
		t.Fatal("missed corruption in later extent")
	}
}
func TestCompactChangedCardDoesNotWrite(t *testing.T) {
	p, m, payload, d, path, dirty := compactTransferFixture(t)
	d.Guard = func() error { return errors.New("card changed") }
	if transferCompact(context.Background(), bytes.NewReader(payload), p, m, d, func(string, int64, int64) {}) == nil {
		t.Fatal("card identity ignored")
	}
	got, _ := os.ReadFile(path)
	if !bytes.Equal(got, dirty) {
		t.Fatal("wrote changed card")
	}
}
func TestCompactCancellationStopsBeforeNextExtent(t *testing.T) {
	p, m, payload, d, path, dirty := compactTransferFixture(t)
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	e := transferCompact(ctx, bytes.NewReader(payload), p, m, d, func(s string, _, _ int64) {
		if s == "Schreiben" {
			cancel()
		}
	})
	if !errors.Is(e, context.Canceled) {
		t.Fatal(e)
	}
	got, _ := os.ReadFile(path)
	off := p.Extents[1].Offset
	if !bytes.Equal(got[off:], dirty[off:]) {
		t.Fatal("wrote after cancellation")
	}
}
func TestCompactTruncationAndTrailingPayloadReject(t *testing.T) {
	for _, which := range []string{"truncated", "trailing", "payload-hash", "extent-hash"} {
		t.Run(which, func(t *testing.T) {
			p, m, payload, d, _, _ := compactTransferFixture(t)
			switch which {
			case "truncated":
				payload = payload[:len(payload)-1]
			case "trailing":
				payload = append(payload, 1)
			case "payload-hash":
				m.PayloadSHA = digest([]byte("wrong"))
			case "extent-hash":
				p.Extents[1].SHA = digest([]byte("wrong"))
			}
			if transferCompact(context.Background(), bytes.NewReader(payload), p, m, d, func(string, int64, int64) {}) == nil {
				t.Fatal("accepted corrupt stream")
			}
		})
	}
}
func TestActualCompactFileRestore(t *testing.T) {
	root := os.Getenv("GA36_COMPACT_ROM_ROOT")
	dest := os.Getenv("GA36_COMPACT_TEST_DIR")
	if root == "" || dest == "" {
		t.Skip("set compact root/test directory for full file restore")
	}
	meta := filepath.Join(root, "DualBoot-Compact/IMAGE-INFOS.json")
	b, e := os.ReadFile(meta)
	if e != nil {
		t.Fatal(e)
	}
	var m Manifest
	if e = json.Unmarshal(b, &m); e != nil {
		t.Fatal(e)
	}
	image := filepath.Join(filepath.Dir(meta), m.File)
	if _, e = loadNativeImage(image, meta); e != nil {
		t.Fatal(e)
	}
	if e = os.MkdirAll(dest, 0700); e != nil {
		t.Fatal(e)
	}
	path := filepath.Join(dest, "simulated-card.img")
	target, e := os.OpenFile(path, os.O_RDWR|os.O_CREATE|os.O_EXCL, 0600)
	if e != nil {
		t.Fatal(e)
	}
	defer target.Close()
	st, e := target.Stat()
	if e != nil || !st.Mode().IsRegular() {
		t.Fatal("test target must be a new regular file")
	}
	if e = target.Truncate(m.ImageBytes); e != nil {
		t.Fatal(e)
	}
	samplesFile := os.Getenv("GA36_COMPACT_FREE_RANGES")
	b, e = os.ReadFile(samplesFile)
	if e != nil {
		t.Fatal(e)
	}
	var gaps [][2]int64
	if e = json.Unmarshal(b, &gaps); e != nil {
		t.Fatal(e)
	}
	dirty := bytes.Repeat([]byte{0xd7}, 4096)
	var samples []int64
	for _, gap := range gaps {
		for _, at := range []int64{gap[0], gap[0] + gap[1] - 4096} {
			if _, e = target.WriteAt(dirty, at); e != nil {
				t.Fatal(e)
			}
			samples = append(samples, at)
		}
	}
	d := &Device{File: target, Guard: func() error {
		st, e := target.Stat()
		if e != nil {
			return e
		}
		if !st.Mode().IsRegular() || st.Size() != m.ImageBytes {
			return errors.New("test file identity changed")
		}
		return nil
	}, ReopenRead: func() (*os.File, error) {
		if e := target.Sync(); e != nil {
			return nil, e
		}
		return os.Open(path)
	}, Close: target.Close, Finish: target.Sync}
	img, e := os.Open(image)
	if e != nil {
		t.Fatal(e)
	}
	defer img.Close()
	if e = flash(context.Background(), img, m, d, func(string, int64, int64) {}); e != nil {
		t.Fatal(e)
	}
	if e = finishImageLayout(d, m.ImageBytes, m); e != nil {
		t.Fatal(e)
	}
	for _, at := range samples {
		got := make([]byte, 4096)
		if _, e = target.ReadAt(got, at); e != nil || !bytes.Equal(got, dirty) {
			t.Fatal("free-block sample unexpectedly overwritten", at, e)
		}
	}
	report := map[string]any{"regular_file_only": true, "target": path, "target_bytes": m.ImageBytes, "written_and_verified_bytes": m.PayloadBytes, "nonzero_free_block_samples_preserved": len(samples), "original_header_check": "passed", "physical_card_access": false}
	b, _ = json.MarshalIndent(report, "", "  ")
	if e = os.WriteFile(filepath.Join(dest, "restore-report.json"), append(b, '\n'), 0600); e != nil {
		t.Fatal(e)
	}
}
