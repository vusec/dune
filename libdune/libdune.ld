SECTIONS { 
    libdune_text : ALIGN (4096)
    { 
        __start_libdune_text = .;
        *libdune.a:(.text .text.* .gnu.linkonce.d.*)
        __stop_libdune_text = .;
        . = ALIGN (4096);
    }
}
INSERT AFTER .bss

SECTIONS { 
    libdune_rodata : ALIGN (4096)
    { 
        __start_libdune_rodata = .;
        *libdune.a:(.rodata .rodata.* .gnu.linkonce.d.*)
        __stop_libdune_rodata = .;
        . = ALIGN (4096);
    }
}
INSERT AFTER libdune_text

SECTIONS { 
    libdune_data : ALIGN (4096)
    { 
        __start_libdune_data = .;
        *libdune.a:(.data .data.* .gnu.linkonce.d.*)
        __stop_libdune_data = .;
        . = ALIGN (4096);
    }
}
INSERT AFTER libdune_rodata

SECTIONS { 
    libdune_bss : ALIGN (4096)
    { 
        __start_libdune_bss = .;
        *libdune.a:(.bss .bss.* .gnu.linkonce.d.*)
        __stop_libdune_bss = .;
        . = ALIGN (4096);
    }
}
INSERT AFTER libdune_data

