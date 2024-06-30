https://github.com/JonathanDotCel/bootcard_igr

# Credits

Entirely based on nox's SD2PSXMAN test app:
https://github.com/qnox32/sd2psxman-testing
With thanks to both nox and El_isra!

And reboot code from Based_skid:
https://www.psx-place.com/resources/reboot-ps2.661/update?update=1062

# License

This is a personal project not officially endorsed by 8bm.
See the license for more details.


# What is it?

An IGR (intergrated reset) binary, which will mount your default MCP2 boot card then restart the PS2.

You can use it 2 ways:

1: Use it as your `IGR.ELF` for OPL
OPL will launch it when you hit the IGR key combo, restart the MCP2, and then restart the PS2

2: Install it as a FMCB menu item
Scroll to it and hit `X` when FMCB starts to load your default boot card on the MCP2


# How to get the file onto the PS2:

If you have a compatible USB stick, you can skip this section and go straight to the setup.

The release zip contains the file `/PS2/mcp2igr/mcp2igr-1.mc2`
You can mount this on your MCP2 (first slot) to copy the file to your OEM card (2nd slot):
e.g. copy the folder onto your SD card and mount it.
`sdcard:/PS2/mcp2igr/mcp2igr-1.mc2`

From there, just open the regular Sony memcard browser and copy the file to the 2nd slot.
You might also be able to use PCSX2, mymc++, etc to do this.

You can remove this from your MCP2 when you're done, we're just using it to load the file.

`Note: this VMC does not include FMCB! That's up to you, it contains only the IGR.`

So on the first slot you'd have:
- MCP2
- No OPL settings (or it'll default to this, you want settings on the 2nd slot)
- No FMCB

On the 2nd slot you'd have:
- OEM memory card
- OPL settings
- FMCB Settings
- FMCB

# Alternate way (If you have a working USB stick)

You can just set it up to launch directly from the USB stick, but just in case:

Copy the `igr` folder onto your USB stick
E.g.
`mass:/igr/igr.elf`
`mass:/igr/icon.icn`
`mass:/igr/icon.sys`

Open uLaunchElf's file browser, navigate to
- `FileBrowser`
- `mass:/`
- Scroll down to `igr` (just highlight, don't open)
- Hit `R1` to open the context menu
- Select `Copy`
- Hit `Tri` to back out
- Select `mc1:/`
- Hit 'R1` to open the context menu
- Select `Paste` (not `mcPaste`)


# Option 1: OPL's IGR feature

1: Have the `igr` folder/item on your OEM memcard in the 2nd slot, or on your USB stick.

2: Boot into OPL

3: Hit `Start` to bring up the settings (Not `Tri`)

4: Select the top item in the list: `Settings`

5: Scroll down to `IGR Path` and enter e.g:

`mass:/igr/igr.elf` <-- USB

`mc0:/igr/igr.elf`  <-- left slot, use the other one:

`mc1:/igr/igr.elf`  <-- right slot

Note: if you're not sure about paths, you can use uLaunchElf or wLaunchElf to explore

6: Hit `Start`

7: Scroll down to `OK`

8: Scroll down to `Save Changes` 

Note: make sure it saves to `mc1` (oem card) or `mass`, if it says `mc0` (mcp2) OPL won't find settings when you mount different VMCs.

9: Hit `Circle` to return to your game list

10: Launch a game

11: Get to a part where the game is listening for pad input (cutscene, gameplay, menu - generally not loading screens)

12: Hold `L1+R1+L2+R2+Start+Select`

You should see a black screen with a bit of text, as the MCP2 loads the boot card, then the PS2 will reboot.
If it's taking longer than about a second, something's up. Corrupt memory card, wrong path or something.

# Option 2: As an FMCB menu item

   
1: Fire up FMCB

2: Go to `Free MCBoot Configurator` (usually at the bottom)

3: Pick `X` or `O`

4: `Load CNF From Mass` or `Load CNF from MC1` 

(whichever has your current settings)

5: Scroll down to `Configure OSDSYS options...`

6: Scroll down to `Configure item 1:`

7: Scroll `right` to a blank entry and hit `X` or `O` (whichever you chose)

8: Set the name to whatever 

(e.g. `Boot MC` - cancel button is backspace, square is space)

9: Scroll down to `Path 1`

10: Select e.g. `mass:/igr/igr.elf`

11: Scroll down to `Return`

12: Scroll down to `Return` once more

13: Scroll down to `Save CNF to Mass` or `Save CNF to MC1` (or both)

14: Reboot


You can now select e.g. `Boot MC` from FMCB to boot your defualt MCP2 boot card.

You should see a black screen with a bit of text, as the MCP2 loads the boot card, then the PS2 will reboot.
If it's taking longer than about a second, something's up. Corrupt memory card, wrong path or something.

