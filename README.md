# Tetra Receiver

Receive multiple TETRA streams at once and send the bits out via UDP.

The tetra streams can be decoding using [`tetra-rx` from the osmocom tetra project](https://github.com/osmocom/osmo-tetra) or [`decoder` from the tetra-kit project](https://gitlab.com/larryth/tetra-kit).

Usage with tetra-rx: `socat STDIO UDP-LISTEN:42000 | stdbuf -i0 -o0 tetra-rx /dev/stdin`

## Usage
The program can be either used with command line arguments or with a TOML config file, the contents are described in the section below.

```
Receive multiple TETRA streams at once and send the bits out via UDP
Usage:
  tetra-receiver [OPTION...]

  -h, --help                  Print usage
      --config-file arg       Instead of these options read the config from
                              a config file (default: "")
      --rf arg                RF gain (default: 10)
      --if arg                IF gain (default: 10)
      --bb arg                BB gain (default: 10)
      --device-string arg     additional device arguments for osmosdr, see
                              https://projects.osmocom.org/projects/gr-osmos
                              dr/wiki/GrOsmoSDR (default: "")
      --center-frequency arg  Center frequency of the SDR (default: 0)
      --offsets arg           Offsets of the TETRA streams
      --samp-rate arg         Sample rate of the sdr (default: 1000000)
      --udp-start arg         Start UDP port. Each stream gets its own UDP
                              port, starting at udp-start (default: 42000)
```

## Toml Config Format

When decoding TETRA streams the downlink and uplink are often a number of MHz apart.
To solve the problem of decoding multiple uplinks and downlinks without having big FIR filters operating at the SDRs sampling rate, one wants to first decimate into two smaller streams. One for all uplink and downlink channels respectively.

Therefore this application supports multiple stages of decimation.

The config has mandatory global arguments `CenterFrequency`, `DeviceString` and `SampleRate` for the SDR.
The optional argumens `RFGain`, `IFGain` and `BBGain` are for setting the gains of the SDR, by default these are zero.

Specify an optional table with the name `Prometheus` and the values `Host` and `Port` to send metrics about the currently received signal strength to a prometheus server.

If a table specifies `Frequency`, `Host` and `Port`, the signal is directly decoded from the SDR.
If it is specified in a subtable, it is decoded from the decimated signal described by the associtated table.

If a table specifies `Frequency` and `SampleRate`, the signal from the SDR is first decimated by the given parameters and then passed to the decoders specified in the subtables.

```
CenterFrequency = unsigned int
DeviceString = "string"
SampleRate = unsigned int
RFGain = unsigned int (default 0)
IFGain = unsigned int (default 0)
BBGain = unsigned int (default 0)

[Prometheus]
Host = "string" (default 127.0.0.1)
Port = unsigned int (default 9010)

[DecimateA]
Frequency = unsigned int
SampleRate = unsigned int

[DecimateA.Stream0]
Frequency = unsigned int
Host = "string"
Port = unsigned int

[DecimateA.Stream1]
Frequency = unsigned int
Host = "string"
Port = unsigned int

[Stream2]
Frequency = unsigned int
Host = "string"
Port = unsigned int
```

## Prometheus
The power of each stream can be exported when setting the `Prometheus` config table.

The magnitude of the stream is filtered to match the frequency of the polling interval.
