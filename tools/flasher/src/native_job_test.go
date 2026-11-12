package main

import (
	"bytes"
	"compress/gzip"
	"context"
	"encoding/json"
	"os"
	"path/filepath"
	"strings"
	"testing"
	"time"
)

func TestDesktopImageRejectsWrongHeaderAndProfile(t *testing.T) {
	root := t.TempDir()
	head := headerFixture()
	var zipped bytes.Buffer
	z := gzip.NewWriter(&zipped)
	z.Write(head)
	z.Close()
	path := filepath.Join(root, "sample.img.gz")
	meta := filepath.Join(root, "IMAGE-INFOS.json")
	os.WriteFile(path, zipped.Bytes(), 0600)
	m := Manifest{OS: "11", File: filepath.Base(path), ImageBytes: baseImageBytes, ImageSHA: digest(head), HeaderSHA: digest([]byte("wrong")), CompressedSHA: digest(zipped.Bytes())}
	put := func() { b, _ := json.Marshal(m); os.WriteFile(meta, b, 0600) }
	put()
	if _, e := loadNativeImage(path, meta); e == nil || !strings.Contains(e.Error(), "header SHA") {
		t.Fatal("unverified header accepted", e)
	}
	m.HeaderSHA = digest(head)
	m.OS = "12"
	put()
	if _, e := loadNativeImage(path, meta); e == nil {
		t.Fatal("wrong OS accepted")
	}
	m.OS = "7"
	m.ImageBytes = baseImageBytes - 4096
	put()
	if _, e := loadNativeImage(path, meta); e == nil {
		t.Fatal("wrong image length accepted")
	}
	m.ImageBytes = baseImageBytes
	m.CompressedSHA = digest([]byte("wrong"))
	put()
	if _, e := loadNativeImage(path, meta); e == nil {
		t.Fatal("bad compressed SHA accepted")
	}
}
func TestWorkerRejectsMalformedRequests(t *testing.T) {
	for _, input := range []string{"", "{}", "{\"Unrecognized\":true}\n", strings.Repeat("a", 1048577) + "\n"} {
		var output bytes.Buffer
		if e := nativeWorker(strings.NewReader(input), &output); e == nil {
			t.Fatal("malformed request accepted")
		}
	}
}
func TestDesktopDemoNeverEnumeratesOrWritesDevices(t *testing.T) {
	root := inboxFixture(t)
	d := &desktop{demo: true, demoRoot: root}
	d.refresh()
	<-d.done
	s := d.snapshot()
	if len(s.Disks) != 1 || s.Disks[0].Path != "DEMO" || len(s.Inboxes) != 1 {
		t.Fatal(s)
	}
	d.runJob(nativeJob{Kind: "flash", DiskIdentity: "a malicious physical target"})
	time.Sleep(60 * time.Millisecond)
	d.stop()
	<-d.done
	if s := d.snapshot(); s.Busy || s.Failure != context.Canceled.Error() {
		t.Fatal("cancel not propagated", s.Status, s.Failure)
	}
	entries, _ := os.ReadDir(root)
	if len(entries) != 3 {
		t.Fatal("demonstration wrote unexpected files")
	}
}
func TestDesktopJobIsSingleFlight(t *testing.T) {
	d := &desktop{}
	started := make(chan struct{})
	release := make(chan struct{})
	d.begin("first", func(context.Context) error { close(started); <-release; return nil })
	<-started
	called := false
	d.begin("second", func(context.Context) error { called = true; return nil })
	close(release)
	<-d.done
	if called {
		t.Fatal("started a second job while busy")
	}
}
func TestActualNativeImageProfiles(t *testing.T) {
	root := os.Getenv("GA36_ROM_ROOT")
	if root == "" {
		t.Skip("set GA36_ROM_ROOT for real GA7 and GA11 packages")
	}
	for _, system := range []string{"7", "11"} {
		meta := filepath.Join(root, "GA"+system, "IMAGE-INFOS.json")
		b, e := os.ReadFile(meta)
		if e != nil {
			t.Fatal(e)
		}
		var m Manifest
		if e = json.Unmarshal(b, &m); e != nil {
			t.Fatal(e)
		}
		got, e := loadNativeImage(filepath.Join(filepath.Dir(meta), m.File), meta)
		if m.NotForFlashing {
			if e == nil {
				t.Fatal("known failed image accepted", system)
			}
			continue
		}
		if e != nil || got.OS != system {
			t.Fatal("real image rejected", system, e)
		}
	}
}
