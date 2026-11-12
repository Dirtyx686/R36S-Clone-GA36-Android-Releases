//go:build linux

package main

import (
	"bufio"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"os"
	"os/exec"
	"strings"
	"sync"
)

func platformJob(ctx context.Context, j nativeJob, emit func(nativeEvent)) error {
	if os.Geteuid() == 0 {
		return executeNativeJob(ctx, j, emit)
	}
	exe, e := os.Executable()
	if e != nil {
		return e
	}
	cmd := exec.Command("pkexec", exe, "--native-worker")
	in, e := cmd.StdinPipe()
	if e != nil {
		return e
	}
	out, e := cmd.StdoutPipe()
	if e != nil {
		return e
	}
	var errout strings.Builder
	cmd.Stderr = &errout
	if e = cmd.Start(); e != nil {
		return e
	}
	defer in.Close()
	waited := false
	defer func() {
		if !waited {
			in.Close()
			out.Close()
			_ = cmd.Wait()
		}
	}()
	var writeMu sync.Mutex
	done := make(chan struct{})
	defer close(done)
	go func() {
		select {
		case <-ctx.Done():
			writeMu.Lock()
			fmt.Fprintln(in, "CANCEL")
			writeMu.Unlock()
		case <-done:
		}
	}()
	writeMu.Lock()
	e = json.NewEncoder(in).Encode(j)
	writeMu.Unlock()
	if e != nil {
		return e
	}
	scanner := bufio.NewScanner(out)
	scanner.Buffer(make([]byte, 4096), 1048576)
	var last string
	finished := false
	for scanner.Scan() {
		var v nativeEvent
		if e = json.Unmarshal(scanner.Bytes(), &v); e != nil {
			return e
		}
		emit(v)
		if v.Done {
			finished = true
			last = v.Error
		}
	}
	wait := cmd.Wait()
	waited = true
	if scanner.Err() != nil {
		return scanner.Err()
	}
	if last != "" {
		return errors.New(last)
	}
	if wait != nil {
		return fmt.Errorf("Administrator/device service: %s %w", errout.String(), wait)
	}
	if !finished {
		return errors.New("Device service ended without result")
	}
	return nil
}
func desktopPrivilege() error { return nil }
