//go:build windows

package main

import (
	"context"
	"errors"
	"os"
	"syscall"
	"unsafe"
)

func platformJob(ctx context.Context, j nativeJob, emit func(nativeEvent)) error {
	return executeNativeJob(ctx, j, emit)
}
func desktopPrivilege() error {
	v, _, _ := shell.NewProc("IsUserAnAdmin").Call()
	if v != 0 {
		return nil
	}
	exe, e := os.Executable()
	if e != nil {
		return e
	}
	verb, _ := syscall.UTF16PtrFromString("runas")
	file, _ := syscall.UTF16PtrFromString(exe)
	r, _, _ := shell.NewProc("ShellExecuteW").Call(0, uintptr(unsafe.Pointer(verb)), uintptr(unsafe.Pointer(file)), 0, 0, 1)
	if r <= 32 {
		return errors.New("Administrator permission not granted")
	}
	os.Exit(0)
	return nil
}
