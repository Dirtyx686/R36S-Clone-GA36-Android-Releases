package main

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"image"
	"image/color"
	"os"
	"path/filepath"
	"strings"
	"sync"
	"time"

	"github.com/aarzilli/nucular"
	"github.com/aarzilli/nucular/font"
	"github.com/aarzilli/nucular/label"
	"github.com/aarzilli/nucular/style"
	"github.com/ncruces/zenity"
	"golang.org/x/image/font/gofont/goregular"
)

const desktopVersion = "4.2-compact"

var accent = color.RGBA{R: 105, G: 210, B: 205, A: 255}
var muted = color.RGBA{R: 166, G: 179, B: 197, A: 255}
var warning = color.RGBA{R: 255, G: 194, B: 112, A: 255}

type deskSnapshot struct {
	Busy, Partial   bool
	Image, Meta     string
	Manifest        *Manifest
	Disks           []Disk
	Inboxes         []Inbox
	Status, Failure string
	Current, Total  int64
	Logs            []string
}
type desktop struct {
	mu       sync.Mutex
	data     deskSnapshot
	demo     bool
	demoRoot string
	notify   func()
	cancel   context.CancelFunc
	done     chan struct{}
	// The following fields are owned by the window event loop.
	language, tab, diskIndex, inboxIndex int
	files                                []string
}

func (d *desktop) l(de, en string) string {
	if d.language == 1 {
		return en
	}
	return de
}
func (d *desktop) snapshot() deskSnapshot {
	d.mu.Lock()
	defer d.mu.Unlock()
	s := d.data
	s.Disks = append([]Disk(nil), s.Disks...)
	s.Inboxes = append([]Inbox(nil), s.Inboxes...)
	s.Logs = append([]string(nil), s.Logs...)
	return s
}
func (d *desktop) update(fn func(*deskSnapshot)) {
	d.mu.Lock()
	fn(&d.data)
	d.mu.Unlock()
	if d.notify != nil {
		d.notify()
	}
}
func appendLog(s *deskSnapshot, msg string) {
	for _, line := range strings.Split(strings.TrimSpace(msg), "\n") {
		if line != "" {
			s.Logs = append(s.Logs, time.Now().Format("15:04:05")+"  "+line)
		}
	}
	if len(s.Logs) > 500 {
		s.Logs = append([]string(nil), s.Logs[len(s.Logs)-500:]...)
	}
}
func (d *desktop) begin(status string, work func(context.Context) error) {
	d.mu.Lock()
	if d.data.Busy {
		d.mu.Unlock()
		return
	}
	ctx, cancel := context.WithCancel(context.Background())
	d.cancel = cancel
	done := make(chan struct{})
	d.done = done
	d.data.Busy = true
	d.data.Failure = ""
	d.data.Partial = false
	d.data.Current = 0
	d.data.Total = 0
	d.data.Status = status
	appendLog(&d.data, status)
	d.mu.Unlock()
	go func() {
		defer close(done)
		defer cancel()
		e := work(ctx)
		d.update(func(s *deskSnapshot) {
			s.Busy = false
			if e != nil {
				s.Failure = e.Error()
				s.Status = "Error / Fehler"
				appendLog(s, e.Error())
			} else {
				s.Status = "Done / Fertig"
				s.Partial = false
				appendLog(s, "Done / Fertig")
			}
		})
	}()
}
func (d *desktop) event(v nativeEvent) {
	d.update(func(s *deskSnapshot) {
		if v.Phase != "" {
			s.Status = v.Phase
			s.Current = v.Current
			s.Total = v.Total
		}
		if v.Partial {
			s.Partial = true
		}
		if v.Message != "" {
			appendLog(s, v.Message)
		}
	})
}
func (d *desktop) stop() {
	d.mu.Lock()
	c := d.cancel
	d.mu.Unlock()
	if c != nil {
		c()
	}
}
func (d *desktop) refresh() {
	s := d.snapshot()
	d.begin(d.l("Karten werden gesucht …", "Looking for cards …"), func(ctx context.Context) error {
		var disks []Disk
		var inboxes []Inbox
		if d.demo {
			disks = []Disk{{Path: "DEMO", Model: "Demo microSD · no physical device", Serial: "DEMO-64", Bytes: 64e9, Identity: "DEMO-ONLY", Safe: true}}
			inboxes = []Inbox{{Token: "DEMO-ONLY", Root: d.demoRoot, Description: "GA36_APPS · DEMO · 64 GB"}}
		} else {
			exe, e := os.Executable()
			if e != nil {
				return e
			}
			var e1, e2 error
			disks, e1 = listDisks(exe, s.Image, s.Meta)
			inboxes, e2 = listInboxes(exe)
			if e1 != nil {
				return e1
			}
			if e2 != nil {
				d.event(nativeEvent{Message: e2.Error()})
			}
		}
		d.update(func(s *deskSnapshot) {
			s.Disks = disks
			s.Inboxes = inboxes
			appendLog(s, fmt.Sprintf("%d cards / Karten · %d GA36_APPS", len(disks), len(inboxes)))
		})
		return ctx.Err()
	})
}
func (d *desktop) inspect(path, meta string) {
	d.update(func(s *deskSnapshot) { s.Manifest = nil; s.Image = path; s.Meta = meta })
	d.begin(d.l("Image prüfen …", "Verifying image …"), func(ctx context.Context) error {
		m, e := loadNativeImage(path, meta)
		if e != nil {
			return e
		}
		if e = ctx.Err(); e != nil {
			return e
		}
		d.update(func(s *deskSnapshot) { s.Manifest = &m; appendLog(s, "SHA-256 OK · GA"+m.OS+" · "+m.Version) })
		return nil
	})
}
func (d *desktop) preset(osVersion string) {
	exe, e := os.Executable()
	if e != nil {
		d.fail(e)
		return
	}
	folder := "GA" + osVersion
	if osVersion == "7+11" {
		folder = "DualBoot-Compact"
	}
	meta := filepath.Join(filepath.Dir(exe), "..", folder, "IMAGE-INFOS.json")
	if root := os.Getenv("GA36_DEMO_ROM_ROOT"); d.demo && root != "" {
		meta = filepath.Join(root, folder, "IMAGE-INFOS.json")
	}
	b, e := os.ReadFile(meta)
	if e != nil {
		d.chooseImage()
		return
	}
	var m Manifest
	if e = json.Unmarshal(b, &m); e != nil {
		d.fail(e)
		return
	}
	d.inspect(filepath.Join(filepath.Dir(meta), m.File), meta)
}
func (d *desktop) fail(e error) {
	if e != nil && !errors.Is(e, zenity.ErrCanceled) {
		d.update(func(s *deskSnapshot) { s.Failure = e.Error(); appendLog(s, e.Error()) })
	}
}
func (d *desktop) chooseImage() {
	path, e := zenity.SelectFile(zenity.Title(d.l("GA7-/GA11-/Dual-Boot-Image auswählen", "Select GA7 / GA11 / dual-boot image")), zenity.FileFilter{Name: "GA36 image", Patterns: []string{"*.ga36.gz", "*.img.gz"}})
	if e != nil {
		d.fail(e)
		return
	}
	meta := filepath.Join(filepath.Dir(path), "IMAGE-INFOS.json")
	if _, e = os.Stat(meta); e != nil {
		meta, e = zenity.SelectFile(zenity.Title("IMAGE-INFOS.json"), zenity.FileFilter{Name: "JSON", Patterns: []string{"*.json"}})
		if e != nil {
			d.fail(e)
			return
		}
	}
	d.inspect(path, meta)
}
func (d *desktop) selectFiles(obb bool) {
	title := d.l("APKs auswählen", "Select APKs")
	pattern := "*.apk"
	if obb {
		title = d.l("OBBs für ein Spiel auswählen", "Select OBBs for one game")
		pattern = "*.obb"
	}
	paths, e := zenity.SelectFileMultiple(zenity.Title(title), zenity.FileFilter{Name: strings.TrimPrefix(pattern, "*."), Patterns: []string{pattern}})
	if e != nil {
		d.fail(e)
		return
	}
	d.files = paths
}
func (d *desktop) runJob(j nativeJob) {
	demo := d.demo
	demoRoot := d.demoRoot
	d.begin(d.l("Vorgang startet …", "Starting operation …"), func(ctx context.Context) error {
		if !demo {
			return platformJob(ctx, j, d.event)
		}
		// Demonstration is deliberately isolated from all real device functions.
		if j.Kind == "flash" {
			for _, phase := range []string{"DEMO · Writing", "DEMO · Read-back verification"} {
				for n := 0; n <= 100; n++ {
					select {
					case <-ctx.Done():
						return ctx.Err()
					case <-time.After(25 * time.Millisecond):
					}
					d.event(nativeEvent{Phase: phase, Current: int64(n), Total: 100})
				}
			}
			return nil
		}
		if j.Kind == "log" {
			d.event(nativeEvent{Message: "DEMO · APK and OBB queues are processed on the next console boot."})
			return nil
		}
		for _, path := range j.Files {
			if e := ctx.Err(); e != nil {
				return e
			}
			if j.Kind == "apks" {
				info, e := queueAPK(demoRoot, path)
				if e != nil {
					return e
				}
				d.event(nativeEvent{Message: "DEMO · APK queued: " + info.Package})
			} else {
				pkg, e := queueOBB(demoRoot, path)
				if e != nil {
					return e
				}
				d.event(nativeEvent{Message: "DEMO · OBB queued: " + pkg})
			}
		}
		return nil
	})
}
func (d *desktop) flashSelected(s deskSnapshot) {
	if s.Manifest == nil || d.diskIndex < 0 || d.diskIndex >= len(s.Disks) {
		return
	}
	disk := s.Disks[d.diskIndex]
	if !eligible(disk, *s.Manifest) {
		return
	}
	text := fmt.Sprintf(d.l("ALLE DATEN auf dieser Karte werden gelöscht.\n\n%s\n%s · %s\n%.1f GB\n\nGA%s · %s\n\nDieses Ziel wirklich vollständig überschreiben?", "ALL DATA on this card will be erased.\n\n%s\n%s · %s\n%.1f GB\n\nGA%s · %s\n\nOverwrite this entire target?"), disk.Path, disk.Model, disk.Serial, float64(disk.Bytes)/1e9, s.Manifest.OS, s.Manifest.Version)
	if d.demo {
		text = "DEMO · No physical device will be written.\n\n" + text
	}
	if e := zenity.Question(text, zenity.Title(d.l("Karte überschreiben?", "Erase card?")), zenity.OKLabel(d.l("Löschen und schreiben", "Erase and write")), zenity.CancelLabel(d.l("Abbrechen", "Cancel"))); e != nil {
		d.fail(e)
		return
	}
	d.runJob(nativeJob{Kind: "flash", ImagePath: s.Image, ManifestPath: s.Meta, DiskIdentity: disk.Identity})
}
func (d *desktop) inboxJob(s deskSnapshot, kind string) {
	if d.inboxIndex < 0 || d.inboxIndex >= len(s.Inboxes) {
		return
	}
	v := s.Inboxes[d.inboxIndex]
	d.runJob(nativeJob{Kind: kind, InboxToken: v.Token, InboxName: v.Description, Files: append([]string(nil), d.files...)})
}
func row(w *nucular.Window, h int, text string) {
	w.Row(h).Dynamic(1)
	w.Label(text, label.Align("LC"))
}
func wrap(w *nucular.Window, h int, text string, col color.RGBA) {
	width := w.LayoutAvailableWidth() - 16
	if width < 80 {
		width = 80
	}
	face := w.Master().Style().Font
	var lines []string
	for _, paragraph := range strings.Split(text, "\n") {
		line := ""
		for _, word := range strings.Fields(paragraph) {
			next := word
			if line != "" {
				next = line + " " + word
			}
			if line != "" && nucular.FontWidth(face, next) > width {
				lines = append(lines, line)
				line = ""
			}
			if line != "" {
				line += " "
			}
			line += word
			for nucular.FontWidth(face, line) > width {
				chars := []rune(line)
				cut := len(chars) - 1
				for cut > 1 && nucular.FontWidth(face, string(chars[:cut])) > width {
					cut--
				}
				lines = append(lines, string(chars[:cut]))
				line = string(chars[cut:])
			}
		}
		if line != "" {
			lines = append(lines, line)
		}
	}
	for _, line := range lines {
		w.Row(24).Dynamic(1)
		w.LabelColored(line, label.Align("LC"), col)
	}
	if rest := h - len(lines)*28; rest > 0 {
		w.Row(rest).Dynamic(1)
		w.Spacing(1)
	}
}
func title(w *nucular.Window, text string) {
	w.Row(38).Dynamic(1)
	w.LabelColored(text, label.Align("LC"), accent)
}
func (d *desktop) render(w *nucular.Window) {
	s := d.snapshot()
	w.Row(48).Dynamic(2)
	w.LabelColored("GA36  /  DESKTOP FLASHER  "+desktopVersion, label.Align("LC"), accent)
	lang := w.ComboSimple([]string{"Deutsch", "English"}, d.language, 28)
	if lang != d.language {
		d.language = lang
		d.savePreferences()
	}
	w.Row(42).Dynamic(4)
	for i, t := range []string{d.l("01  System schreiben", "01  Write system"), d.l("02  APKs hinzufügen", "02  Add APKs"), d.l("03  OBB-Spieldaten", "03  OBB game data"), d.l("04  Protokolle", "04  Logs")} {
		selected := d.tab == i
		if w.SelectableLabel(t, label.Align("CC"), &selected) && !s.Busy {
			if d.tab != i {
				d.files = nil
			}
			d.tab = i
		}
	}
	if d.demo {
		wrap(w, 30, d.l("DEMO · Keine echten Datenträger. Alle Aktionen sind isoliert.", "DEMO · No physical devices. All actions are isolated."), warning)
	}
	if s.Busy {
		title(w, d.l("Vorgang läuft", "Operation in progress"))
		wrap(w, 45, d.phase(s.Status), accent)
		p := 0
		if s.Total > 0 {
			p = int(s.Current * 1000 / s.Total)
		}
		w.Row(25).Dynamic(1)
		w.Progress(&p, 1000, false)
		if s.Total > 0 {
			row(w, 28, fmt.Sprintf("%.1f %% · %.2f / %.2f GiB", float64(p)/10, float64(s.Current)/(1<<30), float64(s.Total)/(1<<30)))
		}
		wrap(w, 50, d.l("Karte angeschlossen lassen. Nach Abbruch beim Schreiben ist das System unvollständig.", "Keep the card connected. Cancelling a write leaves an incomplete system."), muted)
		w.Row(40).Static(220)
		if w.ButtonText(d.l("Abbrechen", "Cancel")) {
			d.stop()
		}
	} else {
		switch d.tab {
		case 0:
			d.renderFlash(w, s)
		case 1, 2:
			d.renderImport(w, s)
		case 3:
			d.renderLogs(w, s)
		}
		if s.Failure != "" {
			wrap(w, 60, s.Failure, warning)
		}
		if s.Partial {
			wrap(w, 48, d.l("Schreibvorgang unvollständig. Karte erst nach erneutem vollständigem Schreiben starten.", "Write incomplete. Rewrite the card fully before booting it."), warning)
		}
	}
	row(w, 30, d.phase(s.Status))
	if d.tab != 3 {
		w.Row(112).Dynamic(1)
		if g := w.GroupBegin("recent-log", nucular.WindowBorder|nucular.WindowNoHScrollbar); g != nil {
			start := len(s.Logs) - 3
			if start < 0 {
				start = 0
			}
			for _, v := range s.Logs[start:] {
				wrap(g, 28, v, muted)
			}
			g.GroupEnd()
		}
	}
}
func (d *desktop) renderFlash(w *nucular.Window, s deskSnapshot) {
	title(w, d.l("Ein sauberes System für deine Konsole.", "A clean system for your console."))
	wrap(w, 35, d.l("Dual Boot: GA7 und GA11 auf einer Karte. Image prüfen, schreiben und vollständig zurücklesen.", "Dual boot: GA7 and GA11 on one card. Verify the image, write the card and verify every written byte."), muted)
	w.Row(40).Dynamic(2)
	if w.ButtonText("Dual Boot · GA7 + GA11") {
		d.preset("7+11")
	}
	if w.ButtonText(d.l("Image auswählen …", "Choose image …")) {
		d.chooseImage()
	}
	if s.Manifest != nil {
		row(w, 30, "SHA-256 OK · GA"+s.Manifest.OS+" · "+s.Manifest.Version)
		if s.Manifest.Format == compactFormat {
			row(w, 28, fmt.Sprintf(d.l("Kompakt: %.2f GiB schreiben + %.2f GiB prüfen · Zielkarte ab 64 GB", "Compact: write %.2f GiB + verify %.2f GiB · target 64 GB or larger"), float64(s.Manifest.PayloadBytes)/(1<<30), float64(s.Manifest.PayloadBytes)/(1<<30)))
		}
	} else {
		row(w, 30, d.l("Noch kein geprüftes Image gewählt.", "No verified image selected."))
	}
	wrap(w, 34, s.Image, muted)
	title(w, d.l("Zielkarte · Dual Boot ab 64 GB", "Target card · dual boot 64 GB or larger"))
	w.Row(38).Dynamic(2)
	names := []string{}
	for _, v := range s.Disks {
		names = append(names, fmt.Sprintf("%s · %.1f GB · %s", v.Model, float64(v.Bytes)/1e9, v.Path))
	}
	if len(names) > 0 {
		if d.diskIndex >= len(names) {
			d.diskIndex = 0
		}
		d.diskIndex = w.ComboSimple(names, d.diskIndex, 32)
	} else {
		w.Label(d.l("Keine geeignete Karte gefunden", "No eligible card found"), label.Align("LC"))
	}
	if w.ButtonText(d.l("Karten neu suchen", "Refresh cards")) {
		d.refresh()
	}
	if d.diskIndex < len(s.Disks) {
		v := s.Disks[d.diskIndex]
		row(w, 28, d.l("Seriennummer: ", "Serial number: ")+v.Serial)
	}
	wrap(w, 50, d.l("Alle Daten auf der Zielkarte werden gelöscht. Größere Karten werden vollständig genutzt; Android erweitert den Datenbereich beim ersten Start.", "All data on the target card will be erased. Larger cards are fully used; Android expands the data area on first boot."), warning)
	if s.Manifest != nil && d.diskIndex < len(s.Disks) && eligible(s.Disks[d.diskIndex], *s.Manifest) {
		w.Row(46).Dynamic(1)
		if w.ButtonText(d.l("Karte schreiben und prüfen …", "Write and verify card …")) {
			d.flashSelected(s)
		}
	}
}
func (d *desktop) inboxChoice(w *nucular.Window, s deskSnapshot) bool {
	w.Row(38).Dynamic(2)
	names := []string{}
	for _, v := range s.Inboxes {
		names = append(names, v.Description)
	}
	if len(names) > 0 {
		if d.inboxIndex >= len(names) {
			d.inboxIndex = 0
		}
		d.inboxIndex = w.ComboSimple(names, d.inboxIndex, 32)
	} else {
		w.Label(d.l("Keine GA36_APPS-Karte gefunden", "No GA36_APPS card found"), label.Align("LC"))
	}
	if w.ButtonText(d.l("Karten neu suchen", "Refresh cards")) {
		d.refresh()
	}
	return len(names) > 0
}
func (d *desktop) renderImport(w *nucular.Window, s deskSnapshot) {
	obb := d.tab == 2
	if obb {
		title(w, d.l("OBB-Spieldaten auf die Karte.", "OBB game data on your card."))
	} else {
		title(w, d.l("APKs über den Kartenleser hinzufügen.", "Add APKs through your card reader."))
	}
	wrap(w, 60, d.l("Dieser Import braucht eine ältere GA36_APPS-Karte. Das Dual-Boot-Abbild hat keinen GA36_APPS-Bereich.", "This import requires an older GA36_APPS card. The dual-boot image has no GA36_APPS area."), muted)
	ok := d.inboxChoice(w, s)
	w.Row(40).Dynamic(1)
	if w.ButtonText(d.l("Dateien auswählen …", "Choose files …")) {
		d.selectFiles(obb)
	}
	if obb {
		wrap(w, 48, d.l("main.<Version>.<Paket>.obb und patch.<Version>.<Paket>.obb für jeweils ein Spiel wählen. APK und Spieldaten müssen zusammenpassen.", "Select main.<version>.<package>.obb and patch.<version>.<package>.obb for one game. APK and data must match."), muted)
	} else {
		wrap(w, 48, d.l("ARM-32-Bit- oder Java-Apps. Mindest-Android-Version wird passend zu GA7 oder GA11 geprüft.", "ARM 32-bit or Java apps. Minimum Android version is checked against GA7 or GA11."), muted)
	}
	row(w, 28, fmt.Sprintf(d.l("%d Dateien ausgewählt", "%d files selected"), len(d.files)))
	for i, path := range d.files {
		if i == 4 {
			row(w, 26, "…")
			break
		}
		row(w, 26, filepath.Base(path))
	}
	if ok && len(d.files) > 0 {
		w.Row(44).Dynamic(1)
		if w.ButtonText(d.l("Auf Karte bereitstellen", "Queue on card")) {
			kind := "apks"
			if obb {
				kind = "obbs"
			}
			d.inboxJob(s, kind)
		}
	}
}
func (d *desktop) renderLogs(w *nucular.Window, s deskSnapshot) {
	title(w, d.l("Alles im Blick.", "Everything in view."))
	ok := d.inboxChoice(w, s)
	w.Row(38).Dynamic(2)
	if ok {
		if w.ButtonText(d.l("INSTALL.TXT von Karte lesen", "Read INSTALL.TXT from card")) {
			d.inboxJob(s, "log")
		}
	} else {
		w.Label("INSTALL.TXT", label.Align("LC"))
	}
	if w.ButtonText(d.l("Protokoll speichern …", "Save log …")) {
		path, e := zenity.SelectFileSave(zenity.Title(d.l("Protokoll speichern", "Save log")), zenity.Filename("GA36-Flasher.log"), zenity.ConfirmOverwrite())
		if e == nil {
			e = os.WriteFile(path, []byte(strings.Join(s.Logs, "\n")+"\n"), 0600)
		}
		d.fail(e)
	}
	w.Row(330).Dynamic(1)
	if g := w.GroupBegin("session-log", nucular.WindowBorder|nucular.WindowNoHScrollbar); g != nil {
		for _, v := range s.Logs {
			wrap(g, 34, v, muted)
		}
		g.GroupEnd()
	}
}
func (d *desktop) phase(s string) string {
	if d.language == 1 {
		switch s {
		case "Schreiben":
			return "Writing"
		case "Synchronisieren":
			return "Flushing to card"
		case "Zurücklesen und prüfen":
			return "Read-back verification"
		}
		return s
	}
	switch s {
	case "Synchronisieren":
		return "Schreibdaten werden synchronisiert"
	case "Verifying image":
		return "Image wird geprüft"
	case "Writing", "Schreiben":
		return "Karte wird geschrieben"
	case "Read-back verification", "Lesetest", "Zurücklesen und prüfen":
		return "Karte wird zurückgelesen und geprüft"
	case "Expanding partition":
		return "Datenpartition wird angepasst"
	case "Preparing APKs":
		return "APKs werden vorbereitet"
	case "Preparing OBBs":
		return "OBB-Spieldaten werden vorbereitet"
	}
	return s
}
func configPath() string {
	root, e := os.UserConfigDir()
	if e != nil {
		return ""
	}
	return filepath.Join(root, "GA36-Flasher")
}
func (d *desktop) savePreferences() {
	if d.demo {
		return
	}
	root := configPath()
	if root == "" {
		return
	}
	if os.MkdirAll(root, 0700) != nil {
		return
	}
	b, _ := json.Marshal(struct{ Language int }{d.language})
	_ = os.WriteFile(filepath.Join(root, "desktop.json"), b, 0600)
}
func desktopMain() error {
	d := &desktop{}
	for _, v := range os.Args[1:] {
		if v == "--demo" {
			d.demo = true
		}
	}
	if !d.demo {
		if e := desktopPrivilege(); e != nil {
			return e
		}
		b, _ := os.ReadFile(filepath.Join(configPath(), "desktop.json"))
		var p struct{ Language int }
		if json.Unmarshal(b, &p) == nil && p.Language == 1 {
			d.language = 1
		}
	} else {
		root, e := os.MkdirTemp("", "ga36-desktop-demo-")
		if e != nil {
			return e
		}
		d.demoRoot = root
		defer os.RemoveAll(root)
		if e = os.WriteFile(filepath.Join(root, "GA36-APPS.TXT"), []byte("GA36_CLEAN_ANDROID_11_V2\n"), 0600); e != nil {
			return e
		}
	}
	if d.demo {
		for _, n := range []string{"APKS", "OBB"} {
			if e := os.Mkdir(filepath.Join(d.demoRoot, n), 0700); e != nil {
				return e
			}
		}
	}
	wnd := nucular.NewMasterWindowSize(0, "GA36 Flasher · Desktop "+desktopVersion, image.Pt(1100, 860), d.render)
	theme := style.FromTheme(style.DarkTheme, 1.0)
	theme.Font, _ = font.NewFace(goregular.TTF, 18)
	theme.Text.Padding = image.Pt(4, 0)
	theme.NormalWindow.Padding = image.Pt(18, 14)
	theme.GroupWindow.Padding = image.Pt(12, 6)
	theme.NormalWindow.Background = color.RGBA{R: 22, G: 30, B: 43, A: 255}
	theme.NormalWindow.FixedBackground = style.MakeItemColor(theme.NormalWindow.Background)
	theme.GroupWindow.Background = color.RGBA{R: 28, G: 39, B: 55, A: 255}
	theme.GroupWindow.FixedBackground = style.MakeItemColor(theme.GroupWindow.Background)
	theme.Text.Color = color.RGBA{R: 230, G: 235, B: 243, A: 255}
	theme.Button.Rounding = 7
	theme.Selectable.Rounding = 7
	wnd.SetStyle(theme)
	d.notify = wnd.Changed
	wnd.OnClose(d.stop)
	d.refresh()
	wnd.Main()
	d.stop()
	d.mu.Lock()
	done := d.done
	d.mu.Unlock()
	if done != nil {
		<-done
	}
	if !d.demo {
		root := configPath()
		if root != "" && os.MkdirAll(root, 0700) == nil {
			_ = os.WriteFile(filepath.Join(root, "last-session.log"), []byte(strings.Join(d.snapshot().Logs, "\n")+"\n"), 0600)
		}
	}
	return nil
}
