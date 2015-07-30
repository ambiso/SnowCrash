# SnowCrash
SnowCrash takes a file and crushes it into a PNG.  
The file can then be decoded again.

## Installation:
```
make
sudo make install
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
  -u          ignore unusual character warning when decoding
If not using -e or -d the mode is determined by the fileextension - if ".png" it's decoded, otherwise encoded.
```
## Examples
```
snowcrash watermelon.flac - will encode watermelon.flac to watermelon.flac.png
snowcrash -e watermelon.flac - identical to the above
snowcrash -o friday.flac watermelon.flac.png - will decode the image-file and save it to friday.flac
snowcrash -e hulk.png -f lovely.png -o hot.png - will encode hulk.png, store lovely.png as filename and output to hot.png
snowcrash hot.png - will decode hot.png and save to lovely.png
```
