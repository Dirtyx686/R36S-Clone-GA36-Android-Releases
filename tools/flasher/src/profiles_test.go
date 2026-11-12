package main

import (
	"os"
	"path/filepath"
	"testing"
)

func TestGA11SDKProfile(t *testing.T) {
	p := apkFixture(t, 30, "armeabi-v7a", false)
	if _, e := inspectAPKForSDK(p, 24); e == nil {
		t.Fatal("GA7 accepted API30")
	}
	if _, e := inspectAPKForSDK(p, 30); e != nil {
		t.Fatal(e)
	}
	p = apkFixture(t, 31, "armeabi-v7a", false)
	if _, e := inspectAPKForSDK(p, 30); e == nil {
		t.Fatal("GA11 accepted API31")
	}
}
func TestGA11Inbox(t *testing.T) {
	root := inboxFixture(t)
	os.WriteFile(filepath.Join(root, "GA36-APPS.TXT"), []byte("GA36_CLEAN_ANDROID_11_V2\n"), 0600)
	p := apkFixture(t, 30, "armeabi-v7a", false)
	if _, e := queueAPK(root, p); e != nil {
		t.Fatal(e)
	}
	os.WriteFile(filepath.Join(root, "GA36-APPS.TXT"), []byte("GA36_CLEAN_ANDROID_7_V2\n"), 0600)
	if _, e := queueAPK(root, p); e == nil {
		t.Fatal("wrong SDK")
	}
}
