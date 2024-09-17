/* Section for RTT control block and buffers */
SECTIONS
{
    .rttmem ORIGIN(RTTMEM) (NOLOAD):
	{
		KEEP(*(.rttmem))
		. = ABSOLUTE(ORIGIN(RTTMEM) + LENGTH(RTTMEM) - 256);
		__rttmem_rttcb = .;
		. = ALIGN(4);
	} > RTTMEM

}
