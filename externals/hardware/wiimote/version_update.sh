#!/bin/sh

VERSION=$(cat VERSION)

echo updating to version: $VERSION

sed -e "s|^VERSION=.*$|VERSION=${VERSION}|" -i Makefile
sed -e "s| VERSION .*;$| VERSION ${VERSION};|" -i wiimote-meta.pd



