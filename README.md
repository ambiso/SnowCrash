# SnowCrash
SnowCrash takes a file and crushes it into a PNG.  
The file can then be decoded again.

## Installation:
```
make
sudo make install
```
## Examples

Encode watermelon.flac to watermelon.flac.png
```
snowcrash watermelon.flac
snowcrash -e watermelon.flac
```

Decode watermelon.flac.png
```
snowcrash watermelon.flac.png
```


Decode watermelon.flac.png and save it to friday.flac
```
snowcrash -o friday.flac watermelon.flac.png
```

Encode hulk.png, store lovely.png as filename and output to hot.png
```
snowcrash -e hulk.png -f lovely.png -o hot.png
```

## Usage
```
snowcrash [OPTIONS] file
  -e [FILE]   encode a file
  -d [FILE]   decode a file
  -o [FILE]   specify where to store the output
  -f [FILE]   specify what filename to store inside the PNG (cannot be used in conjunction with -d)
  -l          enable legacy mode to en- or decode the traditional way 
              (must use -o, does not store filename or size - you will end up with trailing NULs)
  -u          ignore unusual character warning
If not using -e or -d the mode is determined by the file-extension - if ".png" it's decoded, otherwise encoded.
```
