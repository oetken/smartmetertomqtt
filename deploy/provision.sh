#!/usr/bin/env bash
# Provision a fresh Raspberry Pi OS (trixie, armhf) card for SmartMeterToMqtt.
#
# Usage (on the new card, as root):
#   sudo ./deploy/provision.sh /path/to/debs [ /path/to/smartmeter.json ]
#
# The debs come from the "Build packages" GitHub workflow (artifacts, or the
# release assets of a v* tag):
#   libsml1_*.deb  libmbus1_*.deb  libqt6mqtt6_*.deb  smartmetertomqtt_*.deb
set -Eeuo pipefail

DEB_DIR="${1:?usage: provision.sh <deb-dir> [config.json]}"
CONF="${2:-}"
CONFIG=/etc/smartmeter.json

[[ $EUID -eq 0 ]] || { echo "run as root" >&2; exit 1; }

echo "== system"
cat /etc/os-release | grep -E '^(PRETTY_NAME|VERSION_ID)='
dpkg --print-architecture
uname -m

if [[ "$(dpkg --print-architecture)" != "armhf" ]]; then
    echo "!! expected armhf" >&2
fi

echo "== stop any previous instance before touching packages"
systemctl stop smartmeter 2>/dev/null || true
systemctl disable smartmeter 2>/dev/null || true

echo "== install runtime libraries"
apt-get update -qq
dpkg -i "$DEB_DIR"/libsml1_*.deb "$DEB_DIR"/libmbus1_*.deb "$DEB_DIR"/libqt6mqtt6_*.deb
apt-get install -y -f -qq   # pull any missing Qt6 runtime dependencies

echo "== install application (postinst must not start it yet)"
dpkg -i "$DEB_DIR"/smartmetertomqtt_*.deb || true
systemctl stop smartmeter 2>/dev/null || true

echo "== configuration"
if [[ -n "$CONF" ]]; then
    install -m 644 "$CONF" "$CONFIG"
    echo "   restored $CONF"
elif [[ ! -f "$CONFIG" ]]; then
    install -m 644 /etc/smartmeter.json.default "$CONFIG"
    echo "   created from package default - EDIT IT before starting the service"
fi

python3 - "$CONFIG" <<'PY'
import json, sys
p = sys.argv[1]
with open(p) as fh:
    cfg = json.load(fh)

mqtt = cfg.setdefault("MQTT", {})
changed = []

# v1.0.0 reads clientId (case sensitive); older configs use clientid.
if not mqtt.get("clientId"):
    mqtt["clientId"] = mqtt.get("clientid", "StromzählerPi")
    changed.append("clientId")

# Never let the app publish HA discovery: every M-Bus record field would become
# an entity and the existing YAML sensors would be duplicated.
if mqtt.get("haDiscoveryPrefix") != "smartmeter_discovery_disabled":
    mqtt["haDiscoveryPrefix"] = "smartmeter_discovery_disabled"
    changed.append("haDiscoveryPrefix")

with open(p, "w") as fh:
    json.dump(cfg, fh, indent=4, ensure_ascii=False)
    fh.write("\n")
print("   patched:", ", ".join(changed) or "nothing")
PY

echo "== serial devices"
ls -l /dev/serial/by-id/ || echo "!! no /dev/serial/by-id entries - check the USB hub"
for dev in $(python3 -c "
import json
cfg=json.load(open('$CONFIG'))
print(' '.join(s.get('device','') for s in cfg.get('MessageSources',[])))
"); do
    [[ -e "$dev" ]] && echo "   ok      $dev" || echo "   MISSING $dev"
done

echo "== enable + start"
systemctl enable smartmeter
systemctl start smartmeter
sleep 5
systemctl --no-pager --full status smartmeter | head -12
echo
journalctl -u smartmeter -n 15 --no-pager

cat <<EOF

Next steps:
  * watch values:   journalctl -u smartmeter -f
  * verify MQTT:    mosquitto_sub -h <broker> -u <user> -P <pass> -t 'smartmeter/#' -v
  * check in HA:    the 11 sensor.smartmeter_* entities must keep their ids
  * rollback:       power off, put the old card back in
EOF
