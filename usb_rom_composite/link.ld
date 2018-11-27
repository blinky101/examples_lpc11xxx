
MEMORY
{
  /* Define each memory region */
  Flash (rx) : ORIGIN = 0x0, LENGTH = 0xC000 /* 48K bytes */
  RAM_main (rwx) : ORIGIN = 0x10000000, LENGTH = 0x1500 /* 8K bytes */
  RAM_msc (rwx) : ORIGIN = 0x10001500, LENGTH = 0x300 /* 8K bytes */
  RAM_cdc (rwx) : ORIGIN = 0x10001800, LENGTH = 0x200 /* 8K bytes */
  RAM_USB (rwx) : ORIGIN = 0x20004000, LENGTH = 0x800 /* 2K bytes */


}

/* Define a symbol for the top of each memory region */
__top_Flash = 0x0 + 0xC000;
__top_RAM_main = 0x10000000 + 0x2000;
__top_RAM_USB = 0x20004000 + 0x800;

