package main

import (
	"bufio"
	"compress/gzip"
	"context"
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"os"
	"path/filepath"
	"strings"
	"time"
)

type nativeJob struct {
	Kind         string
	ImagePath    string
	ManifestPath string
	DiskIdentity string
	InboxToken   string
	InboxName    string
	Files        []string
}
type nativeEvent struct {
	Phase          string
	Current, Total int64
	Message        string
	Done           bool
	Error          string
	Partial        bool
}

func loadNativeImage(path, meta string) (Manifest, error) {
	var m Manifest
	b, e := os.ReadFile(meta)
	if e != nil {
		return m, e
	}
	if len(b) > 65536 {
		return m, errors.New("Verification file too large")
	}
	if e = json.Unmarshal(b, &m); e != nil {
		return m, e
	}
	if e = validManifest(m); e != nil {
		return m, e
	}
	if m.OS == "" {
		m.OS = "7"
	}
	if !supportedLayout(m) {
		return m, errors.New("Unsupported GA7/GA11 image layout")
	}
	if filepath.Base(path) != m.File {
		return m, errors.New("Image name does not match IMAGE-INFOS.json")
	}
	f, e := os.Open(path)
	if e != nil {
		return m, e
	}
	defer f.Close()
	if e = validateImage(f, m); e != nil {
		return m, e
	}
	if m.Format == compactFormat {
		if e = inspectCompactHeader(f, m); e != nil {
			return m, e
		}
		return m, nil
	}
	gz, e := gzip.NewReader(f)
	if e != nil {
		return m, e
	}
	defer gz.Close()
	header := make([]byte, 4096)
	if _, e = io.ReadFull(gz, header); e != nil {
		return m, e
	}
	sum := sha256.Sum256(header)
	if hex.EncodeToString(sum[:]) != m.HeaderSHA {
		return m, errors.New("Image header SHA-256 mismatch")
	}
	if e = validateLayoutHeader(m, header); e != nil {
		return m, e
	}
	return m, nil
}
func executeNativeJob(ctx context.Context, j nativeJob, emit func(nativeEvent)) error {
	exe, e := os.Executable()
	if e != nil {
		return e
	}
	exe, e = filepath.EvalSymlinks(exe)
	if e != nil {
		return e
	}
	if j.Kind == "flash" {
		emit(nativeEvent{Phase: "Verifying image"})
		m, e := loadNativeImage(j.ImagePath, j.ManifestPath)
		if e != nil {
			return e
		}
		if e = ctx.Err(); e != nil {
			return e
		}
		disks, e := listDisks(exe, j.ImagePath, j.ManifestPath)
		if e != nil {
			return e
		}
		var target *Disk
		for _, d := range disks {
			if d.Identity == j.DiskIdentity && eligible(d, m) {
				v := d
				target = &v
			}
		}
		if target == nil {
			return errors.New("Card identity changed or target is protected")
		}
		dev, e := openDevice(*target, exe, j.ImagePath, j.ManifestPath)
		if e != nil {
			return e
		}
		defer dev.Close()
		if dev.Details != "" {
			emit(nativeEvent{Message: dev.Details})
		}
		f, e := os.Open(j.ImagePath)
		if e != nil {
			return e
		}
		defer f.Close()
		emit(nativeEvent{Phase: "Writing", Partial: true})
		lastPhase := ""
		lastProgress := time.Time{}
		if e = flash(ctx, f, m, dev, func(phase string, n, total int64) {
			if phase == lastPhase && n != total && time.Since(lastProgress) < 100*time.Millisecond {
				return
			}
			lastPhase = phase
			lastProgress = time.Now()
			emit(nativeEvent{Phase: phase, Current: n, Total: total, Partial: true})
		}); e != nil {
			return e
		}
		if e = ctx.Err(); e != nil {
			return e
		}
		emit(nativeEvent{Phase: "Checking boot layout", Partial: true})
		if e = finishImageLayout(dev, target.Bytes, m); e != nil {
			return e
		}
		if e = dev.Finish(); e != nil {
			return e
		}
		return dev.Close()
	}
	if j.Kind != "apks" && j.Kind != "obbs" && j.Kind != "log" {
		return errors.New("Unknown operation")
	}
	if len(j.Files) > 64 {
		return errors.New("Maximum 64 files per operation")
	}
	vs, e := listInboxes(exe)
	if e != nil {
		return e
	}
	var selected *Inbox
	for _, v := range vs {
		if v.Token == j.InboxToken && v.Description == j.InboxName {
			copy := v
			selected = &copy
		}
	}
	if selected == nil || j.InboxToken == "" {
		return errors.New("GA36_APPS card changed or not found")
	}
	root, close, e := selected.Open()
	if e != nil {
		return e
	}
	defer close()
	if e = checkInbox(root); e != nil {
		return e
	}
	if j.Kind == "log" {
		f, e := os.Open(filepath.Join(root, "INSTALL.TXT"))
		if e != nil {
			return e
		}
		defer f.Close()
		st, e := f.Stat()
		if e != nil {
			return e
		}
		if st.Size() > 256*1024 {
			f.Seek(st.Size()-256*1024, 0)
		}
		data, e := io.ReadAll(io.LimitReader(f, 256*1024))
		if e != nil {
			return e
		}
		emit(nativeEvent{Message: string(data)})
		return nil
	}
	if len(j.Files) == 0 {
		return errors.New("No files selected")
	}
	if j.Kind == "apks" {
		fail := 0
		for i, path := range j.Files {
			if e = ctx.Err(); e != nil {
				return e
			}
			emit(nativeEvent{Phase: "Preparing APKs", Current: int64(i), Total: int64(len(j.Files))})
			info, e := queueAPK(root, path)
			if e != nil {
				fail++
				emit(nativeEvent{Message: filepath.Base(path) + ": " + e.Error()})
			} else {
				emit(nativeEvent{Message: "APK queued: " + info.Package})
			}
		}
		emit(nativeEvent{Phase: "Preparing APKs", Current: int64(len(j.Files)), Total: int64(len(j.Files))})
		if fail > 0 {
			return fmt.Errorf("%d APKs rejected; see log", fail)
		}
		return nil
	}
	// Validate the entire OBB batch before committing a queue directory.
	var pkg string
	var files []OBBFile
	for _, path := range j.Files {
		p, fs, e := inspectOBB(path)
		if e != nil {
			return e
		}
		if pkg != "" && pkg != p {
			return errors.New("Select OBB files for one game at a time")
		}
		pkg = p
		files = append(files, fs...)
	}
	temp, e := os.MkdirTemp("", "ga36-native-obbs-")
	if e != nil {
		return e
	}
	defer os.RemoveAll(temp)
	seen := map[string]bool{}
	for i, file := range files {
		if e = ctx.Err(); e != nil {
			return e
		}
		if seen[file.Name] {
			return errors.New("Duplicate OBB file")
		}
		seen[file.Name] = true
		emit(nativeEvent{Phase: "Preparing OBBs", Current: int64(i), Total: int64(len(files))})
		if e = verifiedCopy(file.Path, filepath.Join(temp, file.Name), file.SHA); e != nil {
			return e
		}
	}
	_, e = queueOBB(root, temp)
	if e == nil {
		emit(nativeEvent{Phase: "Preparing OBBs", Current: int64(len(files)), Total: int64(len(files)), Message: "OBB queued: " + pkg})
	}
	return e
}
func nativeWorker(in io.Reader, out io.Writer) error {
	r := bufio.NewReader(io.LimitReader(in, 1048577))
	line, e := r.ReadString('\n')
	if e != nil || len(line) > 1048576 {
		return errors.New("Invalid job input")
	}
	var j nativeJob
	dec := json.NewDecoder(strings.NewReader(line))
	dec.DisallowUnknownFields()
	if e = dec.Decode(&j); e != nil {
		return e
	}
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	go func() {
		line, e := r.ReadString('\n')
		if e != nil || line == "CANCEL\n" {
			cancel()
		}
	}()
	encoder := json.NewEncoder(out)
	e = executeNativeJob(ctx, j, func(v nativeEvent) { encoder.Encode(v) })
	v := nativeEvent{Done: true}
	if e != nil {
		v.Error = e.Error()
	}
	encoder.Encode(v)
	return e
}
