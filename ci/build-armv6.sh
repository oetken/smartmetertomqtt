#!/usr/bin/env bash
# Build for ARMv6 (Raspberry Pi Zero / Pi 1) inside a Raspbian trixie armhf chroot.
# Debian has no ARMv6 port and debian:trixie ships only arm/v5 + arm/v7, so the
# generic container path cannot produce a Pi-Zero-compatible binary.
#
# Usage: sudo ci/build-armv6.sh <output-dir>
# Env:   ROOTFS_TARBALL  optional cached rootfs (tar.zst) to restore/save
set -Eeuo pipefail

OUT="${1:-$(pwd)/dist}"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"

MIRROR="${MIRROR:-http://raspbian.raspberrypi.org/raspbian/}"
SUITE="${SUITE:-trixie}"
ROOTFS="${ROOTFS:-/tmp/raspbian-trixie-armhf}"
CACHE="${ROOTFS_TARBALL:-}"

echo "== bootstrapping $SUITE (armhf) from $MIRROR"

apt-get update -qq
apt-get install -y -qq --no-install-recommends debootstrap qemu-user-static zstd

if [[ -n "$CACHE" && -f "$CACHE" ]]; then
    echo "== restoring cached rootfs"
    rm -rf "$ROOTFS"
    zstd -dc "$CACHE" | tar -x -C "$(dirname "$ROOTFS")"
else
    rm -rf "$ROOTFS"
    debootstrap --arch=armhf --foreign --variant=minbase --no-check-gpg \
        --components=main,contrib,non-free,rpi \
        "$SUITE" "$ROOTFS" "$MIRROR"
    cp /usr/bin/qemu-arm-static "$ROOTFS/usr/bin/"

    chroot "$ROOTFS" /debootstrap/debootstrap --second-stage

    cat > "$ROOTFS/etc/apt/sources.list" <<EOF
deb $MIRROR $SUITE main contrib non-free rpi
deb http://archive.raspberrypi.org/debian/ $SUITE main
EOF
    cp /etc/resolv.conf "$ROOTFS/etc/resolv.conf"

    if [[ -n "$CACHE" ]]; then
        echo "== saving rootfs cache"
        tar -C "$(dirname "$ROOTFS")" -cf - "$(basename "$ROOTFS")" | zstd -3 -T0 > "$CACHE"
    fi
fi

cp /usr/bin/qemu-arm-static "$ROOTFS/usr/bin/"

# archive.raspberrypi.org is signed by the Raspberry Pi key, which is not part of a
# debootstrap'd Raspbian rootfs. Without it apt aborts with "Missing key CF8A1AF5...".
install -d "$ROOTFS/etc/apt/trusted.gpg.d"
curl -fsSL https://archive.raspberrypi.org/debian/raspberrypi.gpg.key \
    | gpg --dearmor > "$ROOTFS/etc/apt/trusted.gpg.d/raspberrypi-archive-keyring.gpg"

# bind mounts for a usable chroot
for m in proc sys dev; do
    mountpoint -q "$ROOTFS/$m" || mount --bind "/$m" "$ROOTFS/$m"
done
mountpoint -q "$ROOTFS/dev/pts" || mount --bind /dev/pts "$ROOTFS/dev/pts"
trap 'umount -R "$ROOTFS/dev/pts" "$ROOTFS/dev" "$ROOTFS/sys" "$ROOTFS/proc" 2>/dev/null || true' EXIT

mkdir -p "$ROOTFS/work"
mountpoint -q "$ROOTFS/work" || mount --bind "$GITHUB_WORKSPACE" "$ROOTFS/work"

chroot "$ROOTFS" /usr/bin/env \
    GITHUB_WORKSPACE=/work \
    QT_MQTT_TAG="${QT_MQTT_TAG:-v6.8.2}" \
    LIBSML_REF="${LIBSML_REF:-}" \
    LIBMBUS_REF="${LIBMBUS_REF:-}" \
    HOME=/root \
    /bin/bash /work/ci/build.sh /work/dist

# build.sh writes directly into the bind-mounted workspace (same as OUT)
