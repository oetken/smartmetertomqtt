#!/usr/bin/env bash
# Build SmartMeterToMqtt + libsml + libmbus + QtMqtt and emit .deb files.
# Intended to run as root inside a Debian/Raspbian trixie environment (container or chroot).
#
# Usage: ci/build.sh <output-dir>
set -Eeuo pipefail

OUT="${1:-$(pwd)/dist}"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"

: "${QT_MQTT_TAG:=v6.8.2}"
: "${LIBSML_REF:=dc096b40b0c0b90c7def374e92a9d186ed276c30}"
: "${LIBMBUS_REF:=ac52f3f0aa9a73eb8bf0b9cc4464be117ebd20d6}"

export DEBIAN_FRONTEND=noninteractive

echo "== arch: $(dpkg --print-architecture)  QtMqtt: $QT_MQTT_TAG"

# The Raspberry Pi archive key is only present when ci/build-armv6.sh injected it.
# Fall back to a trusted source entry rather than failing the whole build.
if ! apt-get update -qq; then
    echo "!! apt update failed, retrying with archive.raspberrypi.org trusted=yes" >&2
    sed -i 's|^deb http://archive.raspberrypi.org/debian/|deb [trusted=yes] http://archive.raspberrypi.org/debian/|' /etc/apt/sources.list
    apt-get update -qq
fi
apt-get install -y -qq --no-install-recommends \
    git cmake build-essential dpkg-dev devscripts debhelper dh-make dh-exec \
    autoconf automake libtool fakeroot pkg-config pandoc \
    uuid-dev libudev-dev \
    qt6-base-dev qt6-base-private-dev qt6-tools-dev qt6-serialport-dev

work=$(mktemp -d)
cd "$work"

########################################
# libsml
########################################
git clone --quiet https://github.com/volkszaehler/libsml
cd libsml
git checkout --quiet "$LIBSML_REF"
sed -i 's/.*\(\s\)-C\(\s\)test$//g' Makefile
sed -i 's/.*\(\s\)-C\(\s\)examples$//g' Makefile
dpkg-buildpackage -b --no-sign
dpkg -i ../*.deb >/dev/null
cp ../*.deb "$OUT"/
cd "$work"

########################################
# libmbus
########################################
git clone --quiet https://github.com/rscada/libmbus
cd libmbus
git checkout --quiet "$LIBMBUS_REF"
./build-deb.sh
dpkg -i ../*.deb >/dev/null
cp ../*.deb "$OUT"/
cd "$work"

########################################
# QtMqtt (not packaged by Debian/Raspbian)
########################################
git clone --quiet --depth 1 --branch "$QT_MQTT_TAG" https://github.com/qt/qtmqtt
cmake -S qtmqtt -B qtmqtt/build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr \
      -DCMAKE_INSTALL_LIBDIR=lib/$(gcc -print-multiarch)
cmake --build qtmqtt/build --parallel "$(nproc)"
cmake --install qtmqtt/build --prefix /usr

# package the freshly installed files into libqt6mqtt6_<tag>_<arch>.deb
stage=$(mktemp -d)
DESTDIR="$stage" cmake --install qtmqtt/build --prefix /usr >/dev/null
mkdir -p "$stage/DEBIAN"
arch=$(dpkg --print-architecture)
ver=${QT_MQTT_TAG#v}
cat > "$stage/DEBIAN/control" <<EOF
Package: libqt6mqtt6
Version: ${ver}-1
Section: libs
Priority: optional
Architecture: ${arch}
Maintainer: SmartMeterToMqtt build <none@none.de>
Description: Qt 6 MQTT library (built from upstream ${QT_MQTT_TAG})
EOF
dpkg-deb --build --root-owner-group "$stage" "$OUT/libqt6mqtt6_${ver}-1_${arch}.deb" >/dev/null
rm -rf "$stage"

########################################
# SmartMeterToMqtt
########################################
cd "$GITHUB_WORKSPACE"
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build --parallel "$(nproc)"
(cd build && cpack -G DEB)

cp build/*.deb "$OUT"/

########################################
# verification
########################################
echo "== verification =="
bin=build/SmartMeterToMqtt
file "$bin"
readelf -A "$bin" 2>/dev/null | grep -E "Tag_CPU_arch|Tag_ABI_VFP_args" || true
echo "missing libs (should be empty):"
ldd "$bin" | grep "not found" || true
"$bin" --version

cd "$OUT"
sha256sum ./*.deb > SHA256SUMS
ls -l
