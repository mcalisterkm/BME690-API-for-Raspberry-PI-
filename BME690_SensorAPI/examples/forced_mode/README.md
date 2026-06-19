# Forced Mode Configuration

The forced mode example reads sensor and I2C configuration from a parameter file using the config-parser library.

## Usage

```bash
# Use default config file (./690.cfg)
./forced_mode

# Use custom config file
./forced_mode /path/to/custom.cfg
```

## Configuration File Format

Configuration files use simple `key = value` format with spaces around the `=` sign. Lines starting with `#` are comments.

Example:
```cfg
# Sensor configuration
bus = 1
port = 0x76

# Oversampling values
osh = 16
ost = 16
osp = 16

# Filter and ODR
lpf = 0
slp = 8

# Heater settings (forced mode uses scalar values)
heatr_temp = 300
heatr_dur = 100

# Sampling controls
samples = 3000
sleep_sec = 3
```

## Parameter Reference

### I2C Bus Configuration

| Key | Type | Range | Default | Description |
|-----|------|-------|---------|-------------|
| `bus` | uint8 | 0-255 | 1 | I2C bus device number (opens `/dev/i2c-{bus}`) |
| `port` | uint8 | 0x00-0xFF | 0x76 | I2C slave address in hex (e.g., 0x76, 0x77) |

### Sensor Oversampling

Oversampling values reduce noise at the cost of measurement time.

| Key | Alias | Values | Default | Description |
|-----|-------|--------|---------|-------------|
| `osh` | - | 0, 1, 2, 4, 8, 16 | 16 | Humidity oversampling (0=off, N=N× samples) |
| `ost` | - | 0, 1, 2, 4, 8, 16 | 16 | Temperature oversampling |
| `osp` | - | 0, 1, 2, 4, 8, 16 | 16 | Pressure oversampling |

### Filtering

| Key | Values | Default | Description |
|-----|--------|---------|-------------|
| `lpf` | 0, 2, 4, 8, 16, 32, 64, 128 | 0 | IIR filter coefficient (0=off, higher=more filtering) |

### Output Data Rate (ODR) / Standby Duration

| Key | Values | Default | Description |
|-----|--------|---------|-------------|
| `slp` | 0-8 | 8 | ODR/standby enum: 0=0.59ms, 1=62.5ms, 2=125ms, 3=250ms, 4=500ms, 5=1s, 6=10ms, 7=20ms, 8=none |

### Gas Heater Configuration (Forced Mode)

Forced mode uses scalar heater temperature and duration (not profile arrays).

| Key | Alias | Type | Default | Description |
|-----|-------|------|---------|-------------|
| `heatr_temp` | `htp` | uint16 | 300 | Heater temperature in °C |
| `heatr_dur` | `htd` | uint16 | 100 | Heater duration in milliseconds |

### Sampling Controls

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `samples` | uint16 or "NULL" | 3000 | Total number of samples to collect, or "NULL" for infinite loop |
| `sleep_sec` | uint32 | 3 | Sleep duration between samples in seconds |

## Output

The program prints a configuration summary on startup:

```
Config: I2C Bus=1 Addr=0x76, OSH=16 OST=16 OSP=16 Filter=0 ODR=8, HeatrT=300C HeatrD=100ms, samples=3000 sleep_sec=3
```

Or for infinite mode (when `samples = NULL`):

```
Config: I2C Bus=1 Addr=0x76, OSH=16 OST=16 OSP=16 Filter=0 ODR=8, HeatrT=300C HeatrD=100ms, samples=NULL sleep_sec=3
```

Followed by measurement data in CSV format with columns:
```
Sample, TimeStamp(ms), Temperature(deg C), Pressure(Pa), Humidity(%), Gas resistance(ohm), Gas Index, Status
```

## Examples

### Minimal Configuration
```cfg
bus = 1
port = 0x76
```
All sensor parameters use defaults.

### Infinite Loop (Continuous Monitoring)
```cfg
bus = 1
port = 0x76
samples = NULL
sleep_sec = 5
```
Sets `samples = NULL` to run indefinitely, collecting samples every 5 seconds. Press Ctrl+C to stop.

### High-Resolution Air Quality Monitoring
```cfg
bus = 1
port = 0x76
osh = 16
ost = 16
osp = 16
lpf = 4
slp = 8
heatr_temp = 320
heatr_dur = 150
samples = 5000
sleep_sec = 2
```

### Quick Test (Low Oversampling)
```cfg
bus = 1
port = 0x76
osh = 2
ost = 2
osp = 2
lpf = 0
slp = 8
heatr_temp = 300
heatr_dur = 100
samples = 100
sleep_sec = 1
```

### Alternative I2C Address
```cfg
bus = 1
port = 0x77
heatr_temp = 300
heatr_dur = 100
```

## Infinite Loop Mode

To run the sensor indefinitely instead of stopping after a fixed number of samples, set `samples = NULL` in the config:

```cfg
samples = NULL
sleep_sec = 3
```

The program will continue collecting samples until interrupted (Ctrl+C). The config summary will show `samples=NULL` when this mode is active.

## Default Configuration

If no config file is found or fails to load, the program uses built-in defaults:
- **I2C Bus:** 1
- **I2C Address:** 0x76
- **Oversampling:** 16× (all channels)
- **Filter:** Off
- **ODR:** None
- **Heater Temp:** 300°C
- **Heater Duration:** 100ms
- **Samples:** 3000
- **Sleep:** 3 seconds

## Config Parser Library

The configuration parser is implemented in `../common/config_parser/parser.h` and handles:
- Key-value pairs separated by `=`
- Comments starting with `#`
- Value parsing (decimal and hex with 0x prefix)
- Linked-list memory management for parsed entries
