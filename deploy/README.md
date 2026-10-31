# Provisioning a new smartmeter card

Goal: build and test the v1.0.0 stack on a **second card / second Pi Zero** without
touching the production card, then swap.

## 1. Flash

Raspberry Pi Imager → **Raspberry Pi OS Lite (32-bit)**, image
`2026-10-06-raspios-trixie-armhf-lite.img.xz` (Trixie, ARMv6, Pi Zero supported).

In the Imager's customisation (gear icon):

| Setting | Value |
|---|---|
| hostname | `gaszaehler-test` (do **not** reuse `gaszaehler` while both run) |
| user | `pi`, SSH public key of the workstation |
| SSH | enabled |
| WLAN | 2.4 GHz only (Pi Zero W has no 5 GHz) |
| locale/timezone | Europe/Berlin |

## 2. First boot

```bash
ssh pi@gaszaehler-test.3nno.de
sudo apt update && sudo apt full-upgrade -y
sudo reboot
```

## 3. Get the packages

From the green **Build packages** workflow (or the release of a `v*` tag):

```bash
# on the workstation
gh run download -n debs-armv6 -D debs      # or: unzip the artifact
scp debs/*.deb debs/SHA256SUMS pi@gaszaehler-test:~/
```

Sanity check on the Pi:

```bash
sha256sum -c SHA256SUMS
dpkg-deb -I smartmetertomqtt_*.deb | grep -E 'Package|Version|Depends'
```

`Depends:` must list the Qt6 modules, `libsml1`, `libmbus1` and `libqt6mqtt6`.

## 4. Provision

Copy the production config over first (it holds broker credentials), then:

```bash
scp /path/to/smartmeter.json pi@gaszaehler-test:~/
scp -r deploy/ pi@gaszaehler-test:~/
ssh pi@gaszaehler-test 'sudo ~/deploy/provision.sh ~/debs ~/smartmeter.json'
```

The script:

- stops/removes any previous service instance,
- installs `libsml1`, `libmbus1`, `libqt6mqtt6`, then `smartmetertomqtt`,
- restores `/etc/smartmeter.json` and patches it:
  - `clientId` (v1.0.0 reads it case-sensitively),
  - `haDiscoveryPrefix = smartmeter_discovery_disabled` (never publish discovery),
- checks every serial device from the config exists,
- enables and starts `smartmeter`, prints status + the last log lines.

## 5. Verify

```bash
journalctl -u smartmeter -f          # expect "MQTT Connected", no "ClientId must be supplied"
mosquitto_sub -h 10.10.200.10 -u enno -P '***' -t 'smartmeter/#' -v
```

In Home Assistant the 11 `sensor.smartmeter_*` entities must keep their ids and keep
updating; `homeassistant/#` must stay empty.

> Only one instance may talk to the meters. If the test card runs on a second Pi,
> move the USB hub over or stop the service on the production Pi first — two
> processes must never poll the same M-Bus.

## 6. Swap

```bash
ssh pi@gaszaehler 'sudo systemctl stop smartmeter'   # optional, power off instead
```
Power off both, move the flashed card into the production Pi, boot, verify again.
Keep the old card untouched as rollback.

## Rollback

Power off, put the original card back. Nothing on the old card was modified.
