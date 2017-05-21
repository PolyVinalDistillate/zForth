
![zForth](/zforth.png)
![zForth](/PVD.png)

zForth
======


From Wikipedia:

   _A Forth environment combines the compiler with an interactive shell, where
   the user defines and runs subroutines called words. Words can be tested,
   redefined, and debugged as the source is entered without recompiling or
   restarting the whole program. All syntactic elements, including variables
   and basic operators are defined as words. Forth environments vary in how the
   resulting program is stored, but ideally running the program has the same
   effect as manually re-entering the source._

zForth is yet another Forth, but with some special features not found in most
other forths. Note that zForth was written for engineers, not for language
purists or Forth aficionados. Its main intention is to be a lightweight
scripting language for extending embedded applications on small
microprocessors. It is not particularly fast, but should be easy to integrate
on any platform with a few kB's of ROM and RAM.

For a lot of programmers Forth seems to belong to the domain of alien
languages: it does not look like any mainstream language most people encounter,
and is built on a number of philosophies that takes some time to get used to.
Still, it is one of the more efficient ways of bringing a interpreter and
compiler to a platform with restricted resources.

Some of zForth's highlights:

- **Small dictionary**: instead of relying on a fixed cell size, the dictionary is
  written in variable length cells: small and common numbers take less space
  then larger, resulting in 30% to 50% space saving

- **Portable**: zForth is written in 100% ANSI C, and runs on virtually all
  operating systems and all architectures. Tested on x86 Linux/Win32/MS-DOS
  (Turbo-C 1.0!), x86_64, ARM, ARM thumb, MIPS, Atmel AVR and the 8051.

- **Small footprint**: the kernel C code compiles to about 3 or 4 kB of machine
  code, depending on the architecture and chosen cell data types.

- **Tracing**: zForth is able to show a nice trace of what it is doing under the
  hood, see below for an example.

- **VM**: Implemented as a small virtual machine: not the fastest, but safe and
  flexible. Instead of having direct access to host memory, the forth VM memory
  is abstracted, allowing proper boundary checking on memory accesses and stack
  operations.

- **Flexible data types**: at compile time the user is free to choose what C data
  type should be used for the dictionary and the stacks. zForth supports signed
  integer sizes from 16 to 128 bit, but also works seamlessly with floating point
  types like float and double (or even the C99 'complex' type!)

- **Ease interfacing**: calling C code from forth is easy through a host system
  call primitive, and code has access to the stack for exchanging data between
  Forth and C. Calling forth from C is easy, just one function to evaluate forth 
  code.


Source layout
=============
(Changed a wee bit by PolyVinalDistillate)
```
./forth                   : core zforth library and various snippets and examples
./src/zforth              : zforth core source; embed these into your program
./src/PSoC5 with USBUART  : CY8CKIT-059 PSoC Creator project using USBUART

```


Usage
=====

zForth consists of only two files: zforth.c and zforth.h. Add both to your
project and call `zf_init()` and `zf_bootstrap()` during initialisation. Read
forth statements from a file or terminal and pass the strings to `zf_eval()` to
interpret, compile and run the code. Check the embedded documentation in
`zforth.h` for details.

---PolyVinalDistillate---

The PSoC Example project initialises with the core.zf definitions which are
stored in a const char in zForth.c. zForth can be interacted with via a terminal
application connected to the USB UART.

The main modifications to the original source code are that zf_eval() now takes
two parameters: "buf" (as before) containing Forth strings, and "bEnableThrottle"
which is used to implement word-at-a-time execution. Setting "bEnableThrottle" to
0 will cause zf_eval() to operate as if it were the original. Setting it to 1 will
cause it to exit after the first iteration of the inner interpreter "run()" loop.

DO NOT SET  bEnableThrottle to 1 manually!! It requires special handling when used
in this mode. The new function zf_Main_Update_Fxn() implements the special handling 
required. Typical use case is shown below:

```
void main()
{
	zf_init(0);					//Standard init
	zf_bootstrap();				//Standard bootstrap
	zf_eval(ZF_CORE_STR, 0);	//Load core definitions from const string
	
    int nInputBytes = 0;
    unsigned short nBytes = nInputBytes;
    unsigned char pInputBuf[64];
    unsigned char* pBufPos = pInputBuf;
	for(;;) 
    {        
        
        if(nInputBytes <= 0)                            //If there are no bytes...
        {
            nInputBytes = SOURCE_GetBytes(pInputBuf);   //Fill buffer from some source
            pBufPos = pInputBuf;                        //Set buffer pointer to start
        }
        
        nBytes = nInputBytes;                                  //Bytes available
        zf_result r = zf_Main_Update_Fxn(pBufPos, &nBytes);    //Feed data to zForth.
        nInputBytes -= nBytes;                                 //Bytes used subtracted
        pBufPos += nBytes;                                     //Buf position updated

        switch(r)  //Switch on return conditions - indicates Forth errors encountered
        {
        }
        /*
        Perform other system functions unrelated to Forth
        */
    }		
}
```

Tracing (not implemented in the PSoC example)
=======

zForth can write verbose traces of the code it is compiling and running. To enable
tracing, run `./zforth` with the `-t` argument. Tracing can be enabled at run time 
by writing `1` in the `trace` variable:

```
1 trace !
```

Make sure the feature ZF_ENABLE_TRACING is enabled in zfconf.h to compile in
tracing support.


The following symbols are used:

- stack operations are prefixed with a double arrow, `«` means pop, `»` means push.
  for operations on the return stack the arrow is prefixed with an `r`

- the current word being executed is shown in square brackets, the format
  is `[<name>/<address>]`

- lines starting with a + show values being added to the dictionary

- lines starting with a space show the current line being executed, format
  `<address> <data>`

- lines starting with `===` show the creation of a new word


````
: square dup * ;
: test 5 square . ;
test
````

Executing the word `test`:

````
test

r»0 
[test/0326] 
 0326 0001 ┊  (lit) »5 
 0328 031c ┊  square/031c r»810 
 031c 000b ┊  ┊  (dup) «5 »5 »5 
 031d 0007 ┊  ┊  (*) «5 «5 »25 
 031e 0000 ┊  ┊  (exit) r«810 
 032a 0133 ┊  ./0133 r»812 
 0133 0001 ┊  ┊  (lit) »1 
 0135 0019 ┊  ┊  (sys) «1 «25 
 0136 0000 ┊  ┊  (exit) r«812 
 032c 0000 ┊  (exit) r«0 25 
````

This is the trace of the definition of the `square` and `test` words

````
: square dup * ;

r»0 
[:/002c] 
 002c 0003 ┊  (:) 
 002c 0003 ┊  (:) 
=== create 'square'
+0313 0006 ¹ 
+0314 02eb ² 
+0316 0000 s 'square'
===
 002d 0000 ┊  (exit) r«0 
+031c 000b ¹ +dup
+031d 0007 ¹ +*r»0 
[;/0031] 
 0031 0004 ┊  (;) 
+031e 0000 ¹ +exit
===
 0032 0000 ┊  (exit) r«0 

: test 5 square . ;

r»0 
[:/002c] 
 002c 0003 ┊  (:) 
 002c 0003 ┊  (:) 
=== create 'test'
+031f 0004 ¹ 
+0320 0313 ² 
+0322 0000 s 'test'
===
 002d 0000 ┊  (exit) r«0 
+0326 0001 ¹ +lit
+0327 0005 ¹ 
+0328 031c ² +square
+032a 0133 ² +.r»0 
[;/0031] 
 0031 0004 ┊  (;) 
+032c 0000 ¹ +exit
===
 0032 0000 ┊  (exit) r«0 

````

