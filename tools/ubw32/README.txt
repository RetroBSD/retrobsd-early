'ubw32' is a simple command-line tool for communicating with the UBW32
bootloader and downloading new firmware.  The UBW32 (32 bit USB Bit Whacker)
info page is here:

	http://www.schmalzhaus.com/UBW32/

Though not yet tested for such, the program will likely work with any device
based on the Microchip HID Bootloader; the UBW32 was simply the first such
device I'd been exposed to, so that name was adopted here.

To build the ubw32 program for Mac, you'll need to have Xcode (Apple's
developer tools) already installed.  The IDE portion is not used here, just
the command line interface.

Assuming you're reading this as the README.txt alongside the source code,
to compile ubw32, in the Terminal window type:

	make

Then install with the command:

	sudo make install

This will copy the 'ubw32' executable to /usr/local/bin so you don't need
to specify a complete path to the program each time.

To build the demo UBW32 program, you'll need to have the Microchip C32
compiler already installed, as documented here:

	http://www.paintyourdragon.com/uc/osxpic32/

(You can ignore the 'Hardware' section of the above page; everything needed
for getting code into the UBW32 is entirely self-contained!)

To build the demo program, type the following two commands:

	cd example
	make

Then, to transfer the demo program to the UBW32, connect the device via
USB while holding the PRG button (or, if already connected, press the
PRG and RESET buttons simultaneously).  The green and white status LEDs
should be alternately flashing.  Then type:

	make write

This will overwrite the firmware currently on the UBW32, so make sure you
have a copy of the original firmware HEX file if you want to restore the
original Bit Whacker functionality!  (D32.hex, available from the page
referenced at the top of this document.  You'll probably want to keep it
around anyway, because it's nifty.)

To create your own program firmware for the UBW32, you can use either the
Microchip C Compiler for PIC32 (for Windows), or the Mac or Linux ports as
described in the C32 document link above.

When creating your own UBW32 programs, it is ABSOLUTELY IMPERATIVE that you
place the file 'procdefs.ld' alongside your source (copy this from the
example directory).  Failure to do so will almost certainly result in your
UBW32 getting 'bricked' and requiring re-flashing the bootloader (which, if
you don't have a PICkit 2 or similar PIC programmer, you're screwed and will
have to track one down to borrow or buy one from Microchip for $35).

  #######################################################################
  #                                                                     #
  #     DO NOT FORGET THE procdefs.ld FILE!  YOU HAVE BEEN WARNED!      #
  #  I assume no liability for hosing your bootloader if this happens.  #
  #                                                                     #
  #######################################################################

The ubw32 program is then used to transfer your compiled code to the UBW32
device.  It's a fairly straightforward program with just a few options, each
preceded by a hyphen.  For instance, to display program help, type:

	ubw32 -help

Or alternately:

	ubw32 -?

Most important is the -write command (this can also be abbreviated -w),
which specifies the name of a HEX file to download to the device, e.g.:

	ubw32 -write test.hex

If you try the above command while in the example directory, you might
receive an error message to the effect that the UBW32 cannot be found.
This is because the UBW32 needs to be placed in Bootloader mode before
transferring new code.  Press the PRG and RESET buttons simultaneously,
then try again:

	ubw32 -write test.hex

If all goes as intended, the program space on the UBW32 will first be erased
and the test program downloaded and verified.  But once done, you won't see
anything else happen; the green and white LEDs are still alternately flashing
as before.  This is because the UBW32 is still in Bootloader mode.  To run
the new code, press the RESET button (you should then see four LEDs fading
in sequence).  Alternately, you can issue a reset from the command line:

	ubw32 -reset

Or perform the write-followed-by-reset in one step like this:

	ubw32 -write test.hex -reset

A few other commands are available.  If you're impatient and want to skip the
write verification step, add '-noverify' (or just '-n').  To erase the UBW32
program space without writing any new code to it, use '-erase' ('-e')
(you can erase without writing a HEX file, but there is no option to write
a HEX file without erasing; it is an implicit step).  Finally, if for some
reason your UBW32 (or other Microchip HID Bootloader device) is using a USB
vendor and product ID different from the defaults (04d8 and 003c,
respectively - you can find these in System Profiler on the Mac), the
'-vendor' ('-v') and '-product' ('-p') commands allow alternate values to
be used (each expects a hexidecimal value).  A full command line might be:

	ubw32 -vendor 04d8 -product 003c -write test.hex -noverify -reset

Commands are not necessarily issued in the order received; there is an
implicit sensible order for each step which the program will follow.  For
instance, putting the '-reset' command first will not actually reset the
device before attempting other steps; this wouldn't work, as the UBW32 will
no longer be in Bootloader mode.

If multiple commands are received, only the last value will be used, e.g.:

	ubw32 -write foo.hex -write bar.hex -write baz.hex

will NOT write three HEX files to the device; only the last in line
(baz.hex in this case) will be used.

For Mac users, only OS X 10.5 (and later) is supported; this will not work
in 10.4 or earlier OS X releases.

For Linux users, you'll need to run the ubw32 program as root (e.g.
'sudo ubw32', or change the setuid bit of the program after installation),
or it might be possible to avoid this if you can decipher the mojo of
"HID blacklisting".  Also note that multiple sequential invocations of the
program may hang (for example, calling once with '-write' command, then a
separate invocation for '-reset').  I've not figured out the cause of this;
an interim solution is to simply manually reset the UBW32 into bootloader
mode prior to each program invocation.

For Windows users: there's no Windows port of this program, as Microchip's
own PDFUSB.exe program already handles this task.

3/16/2009 - Phillip Burgess - pburgess@dslextreme.com
