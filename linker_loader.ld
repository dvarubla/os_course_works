ENTRY(load)
SECTIONS
{
    . = 0x7C00;
    _text1_addr = .;
    .text1 _text1_addr : AT(_text1_addr)
    {
        _text1 = .;
        */stage1.c.o (.text*);
	*/common.c.o (.text*);
        _text1_end = .;
    }
    /*Учитывается то, что %ds = _text1_addr*/
    .data1 (SIZEOF(.text1)) : AT(LOADADDR (.text1) + SIZEOF (.text1))
    {
        _data1 = .;
        */stage1.c.o (.bss* .data* .rodata* COMMON);
	*/common.c.o (.bss* .data* .rodata* COMMON);
        _data1_end = .;
    }
    /*Иначе, по крайней мере, эмулятор не "видит" дискету*/
    .sig : AT(0x7DFE)
    {
        SHORT(0xaa55);
    }
    _text2_addr=LOADADDR(.sig)+ SIZEOF(.sig);
    .text2 _text2_addr : AT(_text2_addr)
    {
        _text2 = .;
        */stage2.c.o (.text*);
	*/common2.c.o (.text*);
        _text2_end = .;
    }
    /*Учитывается то, что %ds = _text2_addr*/
    .data2 (SIZEOF(.text2)) : AT(ADDR(.text2) + SIZEOF(.text2))
    {
        _data2 = .;
        */stage2.c.o (.bss* .data* .rodata* COMMON);
	*/common2.c.o (.bss* .data* .rodata* COMMON);
        _data2_end = .;
    }
    /DISCARD/ :
    {        
	* (.*);
    }
}
