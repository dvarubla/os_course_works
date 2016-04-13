ENTRY(main);
SECTIONS
{
    . = 0x8600;
    /*размер кода программы в секторах*/
    _size = (SIZEOF(.text) + SIZEOF(.data) + SIZEOF(.size) + SIZEOF(.data_loc));
    _size = (_size % 512 == 0)? _size / 512 : _size / 512+1;
    
    .size : AT(0x8600)
    {	
	SHORT(_size);
    }
    .data_loc : AT(LOADADDR(.size) + SIZEOF(.size))
    {
	SHORT(_data_start);
    }
    /*Учитывается то, что %сs = 0x0860*/
    .text (SIZEOF(.size) + SIZEOF(.data_loc)) : AT(LOADADDR(.data_loc) + SIZEOF(.data_loc))
    {
        _text = .;
	*/prologue.c.o (.text*);
        *(.text*);
        _text_end = .;
    }
    /*значение сегмента %ds*/
    _data_start = (LOADADDR(.text) + SIZEOF (.text)) / 16 + 1;
    .data (0) : AT(_data_start * 16)
    {
        _data = .;
        *(.bss);
        *(.bss*);
        *(.data);
        *(.rodata*);
        *(COMMON)
        _data_end = .;
    }
    /DISCARD/ :
    {        
	*(*);
    }
}
