What is this?
---

This repository contains code, drawings and schematics for a device that controls a heating element. This device will throttle the AC current of a device to the duty ratio configured via Wifi. 

The device has features that I could not find in commercial devices;

- Wifi connectivity, for both monitoring and control
- Victron Energy compatible drivers
- Internal temperature monitoring
- External temperature sensor

![perfboard](/media/perfboard_v1.jpg)
![enclosure](/media/enclosure_v1.jpg)
![schematics](/media/schematics_v1.png)
![footprint](/media/footprint_v1.png)

Use case
---
My use case is to heat water when I have superfluous solar energy. The victron driver knows when it is a good time to start pumping solar energy into heating water. 

Network operation
---
If the device has no Wifi access point configured, it will create one named. "heater load configuration AP". You can then point your web browser to http://10.0.0.1 to set the Wifi credentials and maximum connected load. Reboot the device to see it connect to the confgured AP. The device will advertise itself using MDNS, so you can use mdns-scan or Avahi broswer to get the IP information.

The device has a JSON API on the URL : http://esp32-XXXXXX.local/state the response and payload you can send it looks like this. The included web interface also uses this JSON endpoint.

```json
{
  "onPhaseCount": 1,
  "offPhaseCount": 2,
  "wifissid": "Landlubber",
  "loadWatts": 60,
  "targetWatts": 20,
  "triacTemperatureSensor": 34.63259125,
  "externalTemperatureSensor": null
}
```

The device also has a over the air update mechanism. You can point your browser to the web interface and upload a new firmware.

Victron Operation
---
TBD

Temperature calibration
----
Get the current known temperature from the UI, then record the "ratio" to the current reading. Apply that ratio to all readings. Default ratio is 1.0.

Electrical details
---
We use a TRIAC along with a TRIAC driver to select a certain count of phases from mains AC power.  This avoids the sharp voltage spikes typically associated with TRIACs driven by PWM. This should result in low electrical noise, and low heat loss from the TRIAC. 

The external temperature sensor is just a NTC 10K thermistor with a JST 2.0mm 2 pin connector. 

BOM
---
TBD

TODO
---
- [ ] Add temperature limit
- [ ] Test with higher loads
- [ ] Low power mode
- [ ] Autodetect AC frequency at boot time. Sanity checks for 60Hz
- [ ] Make compatible with 50HZ AC mains. Can it work on 220V ?
- [ ] Test on inverter
- [ ] Monitor TRIAC temperature. Shutdown if too hot.
- [ ] If cannot connect to AP after a few minutes, it should fallback to the temporary AP, to allow re-configuration.
- [ ] Secure configuration page with password.
- [ ] Reduce size of device with printed PCB. Modify enclosure
