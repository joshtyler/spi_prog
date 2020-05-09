# spi_prog

Versatile utility to program SPI flash chips.

It can program SPI flash using an FTDI device (which supports the MPSSE engine), or using a UART as a bridge to talk to a wishbone accessible SPI master ([UART wishbone bridge verilog](https://github.com/joshtyler/hdl_common/blob/master/synth/wishbone/serial_wb_master.sv), [wishbone SPI master verilog](https://github.com/joshtyler/hdl_common/blob/master/synth/wishbone/wb_to_spi_master.sv)).

Many options are supported through the CLI such as:
* Programming
* Reading
* Verification
* Reading flash ID
* Reading status registers

Output of help, showing options:
```# ./spi_prog -h
Simple programmer for SPI flash. Multiple operations are supported, and are executed in the order listed in -h
Usage:
  ./spi_prog [OPTION...]

  -h, --help           Print help
  -m, --mode arg       Which device will do the programming. FTDI or wbuart
  -d, --readid         Read the ID bytes of the flash
  -s, --readstatregs   Read the status registers
  -c, --customcmd arg  Execute a custom command (comma separated values, no
                       whitespace)
  -w, --write          Write a file to the flash
  -r, --read           Read flash to file
  -v, --verify         Verify against a file
  -a, --address arg    Address to read from/write to. Must be aligned with
                       sector size (default: 0)
  -i, --infile arg     File to write to flash/verify against (use with -w or
                       -v)
  -o, --outfile arg    File to save data read from flash to (use with -r)
  -l, --readlen arg    Length to read back from flash. (use with -r, but not
                       -w or -v. In these cases lengh is implicit)

 FTDI mode. Use with -t FTDI options:
      --ftdidev arg   Device string, in ftdi_usb_open_string() format.
                      (default: i:0x0403:0x6010)
      --iface arg     Used for mult-interface FTDI chips: A,B,C or D
                      (default: A)
      --xtalFreq arg  FTDI IC crystal frequency either 60MHz or 12MHz. Used
                      for clock divider calculation (default: 12MHz)
      --progfreq arg  Desired programming frequency. Max 6MHz for 12MHz
                      clock. Max 30MHz for 60MHz clock (default: 6MHz)

 wbuart mode. Use with -m wbuart options:
      --uartdev arg   Serial port device string
      --baud arg      Serial port baud rate
      --compaddr arg  Address of wishbone SPI component
```
