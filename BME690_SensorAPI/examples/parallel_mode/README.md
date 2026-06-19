# BME690 Parallel Mode with Configuration File Support

Parallel mode runs the hot plate and the environmental sensors at the same time, which can distort the temp/humidity readings.  It allows simultaneous measurement of temperature, pressure, humidity, and gas resistance using a heater temperature and duration profile.
Allowing a sleep time between cycles can minimise this.
## Usage

### Basic Usage
Run with default configuration:
```bash
./parallel_mode
```

### Custom Configuration
Run with custom config file:
```bash
./parallel_mode /path/to/custom.cfg
```

## Configuration File Format

Configuration files use simple `key = value` format with `#` for comments:

```cfg
# Comment
key = value
```

### Parameters

#### I2C Configuration

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `bus` | uint8 | 0-255 | 1 | I2C bus number (e.g., 1 for /dev/i2c-1) |
| `port` | hex | 0x00-0xFF | 0x76 | I2C slave address |

#### Sensor Configuration

| Parameter | Type | Values | Default | Description |
|-----------|------|--------|---------|-------------|
| `osh` | uint16 | 0, 1, 2, 4, 8, 16 | 16 | Humidity oversampling |
| `ost` | uint16 | 0, 1, 2, 4, 8, 16 | 16 | Temperature oversampling |
| `osp` | uint16 | 0, 1, 2, 4, 8, 16 | 16 | Pressure oversampling |
| `lpf` | uint16 | 0, 2, 4, 8, 16, 32, 64, 128 | 0 | IIR filter coefficient |
| `slp` | uint16 | 0-8 | 8 | Output data rate (ODR) |

#### Heater Configuration

| Parameter | Type | Description |
|-----------|------|-------------|
| `temp_prof` | array | 10 temperature values (°C) for heater profile (comma-separated, no spaces) |
| `mul_prof` | array | 10 multiplier values for heater duration (comma-separated, no spaces) |

**Note:** Heater durations are calculated as `multiplier × 140ms`. For example, `mul_prof = 5` means 700ms at that temperature stage. The 140ms base duration is the standard measurement period for parallel mode.

**Default Profiles:**
- Temperature: `320,100,100,100,200,200,200,320,320,320` (°C)
- Multipliers: `5,2,10,30,5,5,5,5,5,5`

#### Sampling Configuration

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `samples` | uint16 or "NULL" | 1-65535 or NULL | 50 | Number of valid data points to collect; "NULL" enables infinite loop |
| `sleep_sec` | uint32 | 0-4294967295 | 0 | Sleep duration (seconds) between sampling cycles |

## Output Format

### Config Summary (First Line)
```
bus: 1, port: 0x76, osh: 16, ost: 16, osp: 16
```

### Data Output (CSV Format)
```
Sample, TimeStamp(ms), Temperature(deg C), Pressure(Pa), Humidity(%%), Gas resistance(ohm), Status, Gas index, Meas index
1, 1234567890, 25.45, 101325.00, 45.23, 150000.00, 0xb0, 0, 0
2, 1234568001, 25.46, 101326.00, 45.24, 151000.00, 0xb0, 0, 1
```

## Example Configurations

### High-Resolution Measurement
```cfg
bus = 1
port = 0x76
osh = 16
ost = 16
osp = 16
lpf = 127
slp = 8
samples = 100
sleep_sec = 2
```

### Quick Test
```cfg
bus = 1
port = 0x76
osh = 1
ost = 1
osp = 1
lpf = 0
slp = 0
samples = 10
sleep_sec = 0
```

### Alternative I2C Address
```cfg
bus = 1
port = 0x77
osh = 16
ost = 16
osp = 16
lpf = 0
slp = 8
samples = 50
sleep_sec = 1
```

### Custom Heater Profile
```cfg
bus = 1
port = 0x76
osh = 16
ost = 16
osp = 16
lpf = 0
slp = 8
temp_prof = 300,150,150,150,250,250,250,300,300,300
mul_prof = 3,3,3,3,3,3,3,3,3,3
samples = 50
sleep_sec = 0
```

### Infinite Loop Mode
```cfg
bus = 1
port = 0x76
osh = 16
ost = 16
osp = 16
lpf = 0
slp = 8
samples = NULL
sleep_sec = 5
```

## Infinite Loop Mode

Set `samples = NULL` to collect data indefinitely until interrupted (Ctrl+C):

```cfg
samples = NULL
sleep_sec = 3
```

The output will display all data points continuously with 3-second intervals between sampling cycles.

## Default Values

When the config file cannot be loaded or a parameter is missing, these defaults are applied:

| Parameter | Default |
|-----------|---------|
| bus | 1 |
| port | 0x76 |
| osh, ost, osp | 16 |
| lpf | 0 |
| slp | 8 |
| samples | 50 |
| sleep_sec | 0 |
| temp_prof | 320,100,100,100,200,200,200,320,320,320 |
| mul_prof | 5,2,10,30,5,5,5,5,5,5 |

## Configuration Parser Reference

The configuration parser is located in `../common/config_parser/parser.h` and supports:
- Key-value pairs with `=` separator
- Comment lines starting with `#`
- Whitespace handling around keys and values
- Array parsing with comma-separated values
- NULL string comparison for infinite loop mode

## Troubleshooting

### Config file not found
If the config file doesn't exist, the program will print a warning and use default values:
```
Warning: Could not load config file, using defaults
```

### Invalid I2C address
Verify the I2C address matches your sensor configuration:
- `0x76` - Default (SDO connected to GND)
- `0x77` - Alternative (SDO connected to VDDIO)

### Heater profile issues
Ensure `temp_prof` and `mul_prof` both contain exactly 10 comma-separated values. The multiplier values are multiplied by 140ms to determine actual heating duration.

## Platform Notes

- **Raspberry Pi:** Tested on Raspberry Pi 4 with BME690 connected to I2C-1
- **Linux:** Requires `/dev/i2c-X` device access (run with sudo if needed)
- **Dependencies:** Standard C library, Linux I2C dev tools

## Building

From the parallel_mode directory:
```bash
make clean
make
./parallel_mode
```
