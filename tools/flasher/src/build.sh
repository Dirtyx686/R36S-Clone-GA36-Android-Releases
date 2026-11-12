#!/bin/sh
set -eu
cd -- "$(dirname -- "$0")"
export CGO_ENABLED=0 GOTOOLCHAIN=local
mkdir -p bin
go test -mod=vendor -tags nucular_shiny ./...
GOOS=linux GOARCH=amd64 go build -mod=vendor -buildvcs=false -trimpath -tags nucular_shiny -o bin/GA36-Flasher-Linux-4.2-Compact .
