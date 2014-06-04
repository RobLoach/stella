//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//============================================================================

#ifndef CARTRIDGEDASH_HXX
#define CARTRIDGEDASH_HXX

class System;

#include "bspf.hxx"
#include "Cart.hxx"

#ifdef DEBUGGER_SUPPORT
class CartridgeDASHWidget;
//  #include "CartDASHWidget.hxx"
#endif

/**
 Cartridge class for new tiling engine "Boulder Dash" format games with RAM.
 Kind of a combination of 3F and 3E, with better switchability.
 B.Watson's Cart3E was used as a template for building this implementation.

 Because a single bank number is used to define both the destination (0-3)
 AND the type (ROM/RAM) there are only 5 bits left to indicate the actual bank
 number. This sets the limits of 32K ROM and 16K RAM.

 D7			RAM/ROM flag (1=RAM)
 D6D5			indicate the bank number (0-3)
 D4D3D2D1D0	indicate the actual # (0-31) from the image/ram

 Hotspot 0x3F is used for bank-switching, with the encoded bank # as above.

 ROM:

 In this scheme, the 4K address space is broken into four 1K ROM segments.
 living at 0x1000, 0x1400, 0x1800, 0x1C00 (or, same thing, 0xF000... etc.),
 and four 512 byte RAM segments, living at 0x1000, 0x1200, 0x1400, 0x1600
 with write-mirrors +0x800 of these.  The last 1K ROM ($FC00-$FFFF) segment
 is initialised to point to the FIRST 1K of the ROM image, but it may be
 switched out at any time.  Note, this is DIFFERENT to 3E which switches in
 the UPPER bank and this bank is fixed.  This allows variable sized ROM
 without having to detect size. First bank (0) in ROM is the default fixed
 bank mapped to $FC00.

 The system requires the reset vectors to be valid on a reset, so either the
 hardware first switches in the first bank, or the programmer must ensure
 that the reset vector is present in ALL ROM banks which might be switched
 into the last bank area.  Currently the latter (programmer onus) is required,
 but it would be nice for the cartridge hardware to auto-switch on reset.

 ROM switching (write of block+bank number to $3F) D7=0 and D6D5 upper 2 bits of bank #
 indicates the destination segment (0-3, corresponding to $F000, $F400, $F800, $FC00),
 and lower 5 bits indicate the 1K bank to switch in.  Can handle 32 x 1K ROM banks (32K total).

 D7 D6 D5   D4D3D2D1D0
 0  0  0	x x x x x		switch a 1K ROM bank xxxxx to $F000
 0  0  1					switch a 1K ROM bank xxxxx to $F400
 0  1  0					switch a 1K ROM bank xxxxx to $F800
 0  1  1					switch a 1K ROM bank xxxxx to $FC00

 RAM switching (write of segment+bank number to $3F) with D7=1 and D6D5 upper 2 bits of bank #
 indicates the destination RAM segment (0-3, corresponding to $F000, $F200, $F400, $F600).
 Note that this allows contiguous 2K of RAM to be configured by setting 4 consecutive RAM segments
 each 512 bytes with consecutive addresses.  However, as the write address of RAM is +0x800, this
 invalidates ROM access as described below.

 can handle 32 x 512 byte RAM banks (16K total)

 D7 D6 D5    D4D3D2D1D0
 1  0  0     x x x x x		switch a 512 byte RAM bank xxxxx to $F000 with write @ $F800
 0  1					switch a 512 byte RAM bank xxxxx to $F200 with write @ $FA00
 1  0					switch a 512 byte RAM bank xxxxx to $F400 with write @ $FC00
 1  1					switch a 512 byte RAM bank xxxxx to $F600 with write @ $FE00

 It is possible to switch multiple RAM banks and ROM banks together

 For example,
 F000-F1FF   RAM bank A (512 byte READ)
 F200-F3FF   high 512 bytes of ROM bank previously loaded at F000
 F400        ROM bank 0 (1K)
 F800        RAM bank A (512 byte WRITE)
 FA00-FBFF   high 512 bytes of ROM bank previously loaded at F400
 FC00        ROM bank 1

 This example shows 512 bytes of RAM, and 2 1K ROM banks and two 512 byte ROM
 bank halves.

 Switching RAM blocks (D7D6 of $3E) partially invalidates ROM blocks, as below...

 RAM block    Invalidates ROM block
 0        0 (lower half), 2 (lower half)
 1        0 (upper half), 2 (upper half)
 2        1 (lower half), 3 (upper half)
 3        1 (upper half), 3 (lower half)

 For example, RAM block 1 uses address $F200-$F3FF and $FA00-$FBFF
 ROM block 0 uses address $F000-$F3FF, and ROM block 2 uses address $F800-$FBFF
 Switching in RAM block 1 makes F200-F3FF ROM inaccessible, however F000-F1FF is
 still readable.  So, care must be paid.

 TODO: THe partial reading of ROM blocks switched out by RAM is not yet implemented!!

 This crazy RAM layout is useful as it allows contiguous RAM to be switched in,
 up to 2K in one sequentially accessible block. This means you CAN have 2K of
 consecutive RAM. If you don't detect ROM write area, then you would have NO ROM
 switched in (don't forget to copy your reset vectors!)

 @author  Andrew Davie
 */

class CartridgeDASH: public Cartridge {
  friend class CartridgeDASHWidget;

public:
  /**
   Create a new cartridge using the specified image and size

   @param image     Pointer to the ROM image
   @param size      The size of the ROM image
   @param settings  A reference to the various settings (read-only)
   */
  CartridgeDASH(const uInt8* image, uInt32 size, const Settings& settings);

  /**
   Destructor
   */
  virtual ~CartridgeDASH();

public:
  /**
   Reset device to its power-on state
   */
  void reset();

  /**
   Install cartridge in the specified system.  Invoked by the system
   when the cartridge is attached to it.

   @param system The system the device should install itself in
   */
  void install(System& system);

  /**
   Install pages for the specified bank in the system.

   @param bank The bank that should be installed in the system
   */
  bool bank(uInt16 bank);

  /**
   Get the current bank.
   */
  uInt16 bank() const;

  /**
   Query the number of banks supported by the cartridge.
   */
  uInt16 bankCount() const;

  /**
   Patch the cartridge ROM.

   @param address  The ROM address to patch
   @param value    The value to place into the address
   @return    Success or failure of the patch operation
   */
  bool patch(uInt16 address, uInt8 value);

  /**
   Access the internal ROM image for this cartridge.

   @param size  Set to the size of the internal ROM image data
   @return  A pointer to the internal ROM image data
   */
  const uInt8* getImage(int& size) const;

  /**
   Save the current state of this cart to the given Serializer.

   @param out  The Serializer object to use
   @return  False on any errors, else true
   */
  bool save(Serializer& out) const;

  /**
   Load the current state of this cart from the given Serializer.

   @param in  The Serializer object to use
   @return  False on any errors, else true
   */
  bool load(Serializer& in);

  /**
   Get a descriptor for the device name (used in error checking).

   @return The name of the object
   */
  string name() const {
    return "CartridgeDASH";
  }

#ifdef DEBUGGER_SUPPORT
  /**
   Get debugger widget responsible for accessing the inner workings
   of the cart.
   */
  CartDebugWidget* debugWidget(GuiObject* boss, const GUI::Font& lfont,
      const GUI::Font& nfont, int x, int y, int w, int h)
  {
    return 0; //new CartridgeDASHWidget(boss, lfont, nfont, x, y, w, h, *this);
  }
#endif

public:
  /**
   Get the byte at the specified address

   @return The byte at the specified address
   */
  uInt8 peek(uInt16 address);

  /**
   Change the byte at the specified address to the given value

   @param address  The address where the value should be stored
   @param value    The value to be stored at the address
   @return         True if the poke changed the device address space, else false
   */
  bool poke(uInt16 address, uInt8 value);

private:

  uInt32 mySize;        // Size of the ROM image
  uInt8* myImage;       // Pointer to a dynamically allocated ROM image of the cartridge

  Int16 bankInUse[4];     // bank being used for ROM/RAM (-1 = undefined)

  static const uInt16 BANK_SWITCH_HOTSPOT = 0x3F;			// writes to this address cause bankswitching

  static const uInt8 BANK_BITS = 5;                         // # bits for bank
  static const uInt8 BIT_BANK_MASK = (1 << BANK_BITS) - 1;  // mask for those bits
  static const uInt8 BITMASK_ROMRAM = 0x80;   // flags ROM or RAM bank switching (D7--> 1==RAM)

  static const uInt16 RAM_BANK_COUNT = 32;
  static const uInt16 RAM_BANK_TO_POWER = 9;    // 2^n = 512
  static const uInt16 RAM_BANK_SIZE = (1 << RAM_BANK_TO_POWER);
  static const uInt16 BITMASK_RAM_BANK = (RAM_BANK_SIZE - 1);
  static const uInt32 RAM_TOTAL_SIZE = RAM_BANK_COUNT * RAM_BANK_SIZE;

  static const uInt16 ROM_BANK_TO_POWER = 10;   // 2^n = 1024
  static const uInt16 ROM_BANK_SIZE = (1 << ROM_BANK_TO_POWER);
  static const uInt16 BITMASK_ROM_BANK = (ROM_BANK_SIZE -1);

  static const uInt16 ROM_BANK_COUNT = 32;

  static const uInt16 RAM_WRITE_OFFSET = 0x800;

  static const Int16 BANK_UNDEFINED = -1;       // bank is undefined and inaccessible

  uInt8 myRAM[RAM_TOTAL_SIZE];
};

#endif