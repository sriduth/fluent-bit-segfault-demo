# Segfault Demo

Using clang, `co_swap_function` gets defined in the `.text#` section. Using gcc it gets defined in the `.text` section.
This seems to be a linker script setting - the default linker script puts anything matching `.text*` into the `.text` of
the final output (i think).

If one defines the section as `.custom-section` calls to the `co_swap_function` will fail
with a segmentation fault if it is invoked after an `epoll_wait`.

The exact fault seems to be the `mov [rsi],rsp` which i think is the part where the 'switch' between the co routines happens.

One observation is that `.custom-section` is marked as an `ALLOC` type section not `EXEC` unlike
`.text`.

This reflects in the section to segment mapping.

If `.custom-section` is marked as `EXEC`, it is added to the same segment as `.text`

`readelf -l a.out`:

```
 Section to Segment mapping:
  Segment Sections...
   00
   01     .interp
   02     .interp .note.gnu.build-id .note.ABI-tag .gnu.hash .dynsym .dynstr .gnu.version .gnu.version_r .rela.dyn .rela.plt
   03     .init .plt .text .custom-section .fini
   04     .rodata .eh_frame_hdr .eh_frame
   05     .init_array .fini_array .dynamic .got .got.plt .data .bss
   06     .dynamic
   07     .note.gnu.build-id .note.ABI-tag
   08     .eh_frame_hdr
   09
   10     .init_array .fini_array .dynamic .got
```


If `.custom-section` is `ALLOC` only, it is added to a different section which holds only read only
data.

```
 Section to Segment mapping:
  Segment Sections...
   00
   01     .interp
   02     .interp .note.gnu.build-id .note.ABI-tag .gnu.hash .dynsym .dynstr .gnu.version .gnu.version_r .rela.dyn .rela.plt
   03     .init .plt .text .fini
   04     .rodata .custom-section .eh_frame_hdr .eh_frame             
   05     .init_array .fini_array .dynamic .got .got.plt .data .bss
   06     .dynamic
   07     .note.gnu.build-id .note.ABI-tag
   08     .eh_frame_hdr
   09
   10     .init_array .fini_array .dynamic .got
```


