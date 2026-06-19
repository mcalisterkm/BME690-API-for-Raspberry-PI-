# BME690 Sequential Mode with Configuration File Support

Sequential mode runs the environmental sensors first and then the hot plate, which minimises the impact of the hot plate on temp/humidity, but gives a longer cycle time. It supports profile-based heater operation with explicit duration values in milliseconds per profile entry.

## Usage

### Basic Usage
Run with default configuration:
```bash
./sequential_mode
```

### Custom Configuration
Run with custom config file:
```bash
./sequential_mode /path/to/custom.cfg
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
| `ost` | uint16 | 0, 1, 2, 4, 8, 16 | 2 | Temperature oversampling |
| `osp` | uint16 | 0, 1, 2, 4, 8, 16 | 1 | Pressure oversampling |
| `lpf` | uint16 | 0, 2, 4, 8, 16, 32, 64, 128 | 0 | IIR filter coefficient |
| `slp` | uint16 | 0-8 | 8 | Output data rate (ODR) |

#### Heater Configuration

| Parameter | Type | Description |
|-----------|------|-------------|
| `temp_prof` | array | 10 temperature values (deg C) for heater profile (comma-separated, no spaces) |
| `dur_prof` | array | 10 absolute duration values (ms), one per profile stage (comma-separated, no spaces) |

**Note:** Sequential mode uses explicit duration values in milliseconds (`dur_prof`) instead of multipliers.

**Default Profiles:**
- Temperature: `200, 240, 280, 320, 360, 360, 320, 280, 240, 200` (deg C)
- Duration: `100, 100, 100, 100, 100, 100, 100, 100, 100, 100` (ms)

#### Sampling Configuration

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `samples` | uint16 or "NULL" | 1-65535 or NULL | 300 | Number of valid data points to collect; "NULL" enables infinite loop |
| `sleep_sec` | uint32 | 0-4294967295 | 0 | Sleep duration (seconds) between sampling cycles |

## Output Format

### Config Summary (First Line)
```
bus: 1, port: 0x76, osh: 16, ost: 2, osp: 1
```

### Data Output (CSV Format)
```
Sample, TimeStamp(ms), Temperature(deg C), Pressure(Pa), Humidity(%), Gas resistance(ohm), Status, Profile index, Measurement index
1,1234567890,25.45,101325.00,45.23,150000.00,0xb0,0,0
2,1234568001,25.46,101326.00,45.24,151000.00,0xb0,1,1
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
samples = 300
sleep_sec = 1
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
samples = 20
sleep_sec = 0
```

### Alternative I2C Address
```cfg
bus = 1
port = 0x77
osh = 16
ost = 2
osp = 1
lpf = 0
slp = 8
samples = 300
sleep_sec = 0
```

### Custom Heater Profile
```cfg
bus = 1
port = 0x76
osh = 16
ost = 2
osp = 1
lpf = 0
slp = 8
temp_prof = 220,260,300,340,360,360,340,300,260,220
dur_prof = 80,100,120,140,160,160,140,120,100,80
samples = 300
sleep_sec = 0
```

### Infinite Loop Mode
```cfg
bus = 1
port = 0x76
osh = 16
ost = 2
osp = 1
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

## Default Values

When the config file cannot be loaded or a parameter is missing, these defaults are applied:

| Parameter | Default |
|-----------|---------|
| bus | 1 |
| port | 0x76 |
| osh | 16 |
| ost | 2 |
| osp | 1 |
| lpf | 0 |
| slp | 8 |
| samples | 300 |
| sleep_sec | 0 |
| temp_prof | 200,240,280,320,360,360,320,280,240,200 |
| dur_prof | 100,100,100,100,100,100,100,100,100,100 |

## Configuration Parser Reference

The configuration parser is located in `../common/config_parser/parser.h` and supports:
- Key-value pairs with `=` separator
- Comment lines starting with `#`
- Whitespace handling around keys and values
- Array parsing with comma-separated values
- NULL string comparison for infinite loop mode
