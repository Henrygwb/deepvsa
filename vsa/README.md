# reverse-from-coredump
Reverse Execution From CoreDump

## Prerequirement

### libelf / libdisasm

    $ sudo apt-get install libelf1 libelf-dev

library to read and write ELF files

### custome-tailor libdisasm

Now we use a custome-tailor [libdisasm](https://github.com/junxzm1990/libdsiasm). The corresponding installation process is as follows:

```sh
cd libdsiasm
./configure
make
sudo make install

sudo ldconfig
```

### autoconf / automake

    $ sudo apt-get install autoconf automake

## Building

```
$ ./autogen.sh
$ ./configure
$ make
```

## Usage

    $ ./src/reverse coredump binary_path inversed_instruction_trace

**Make sure binary file and all the library files are in the binary path**

### Test

```
$ ./src/reverse testsuites/latex2rtf/core testsuites/library/ testsuites/latex2rtf/inst.reverse
```

### Clean

```
$ make clean
$ make distclean
```

## One Note

- convert all the operands with type op_offset to type op_expression.

## Instructions Handler

- [x] add
- [x] sub
- [ ] mul
- [ ] div
- [x] inc
- [x] dec
- [ ] shl
- [ ] shr
- [ ] rol
- [ ] ror
- [x] and
- [ ] or  (sub)
- [ ] xor (sub)
- [ ] not (sub)
- [x] neg
- [x] return
- [x] call
- [x] jmp
- [x] jcc
- [x] push
- [x] pop
- [ ] pushregs
- [ ] popregs
- [ ] pushflags
- [ ] popflags
- [ ] enter
- [x] leave
- [x] test
- [x] cmp
- [x] mov
- [x] lea
- [ ] movcc (Examples: setz)
- [x] xchg
- [ ] xchgcc (Exmaples: cmpxchg)
- [x] strcmp
- [x] strload
- [x] strmov
- [x] strstore
- [x] translate
- [x] bittest
- [ ] bitset
- [ ] bitclear
- [ ] cpuid
- [x] nop
