# FukYorBrane

## Description & Specifications
FukYorBrane is a cross between [BrainFuck](https://esolangs.org/wiki/Brainfuck) and [CoreWars](http://corewar.co.uk/).
Basically, you have two FYB programs
running simultaneusly.  The data buffer for each of them is the opponent's program buffer.  The
objective is to cause the opponent's program to quit with a "bomb."  The changes you make to the
opponent's program buffer must be committed (see `!`) before they will have an effect.
Comments can be put into the program either the same way as BF (anything that isn't a command is a
comment) and/or by surrounding comments in ().  The advantage of () is that the comments can
contain `.`, `,`, `:`, `;`, etc.

The program buffers will always be of the same length - if one is shorter, it will be padded at the
end with nops (see `%`).  When a program pointer reaches the end of a program, it simply goes back to
the beginning.

There are 17 commands in FukYorBrane:

COMMAND | VALUE | EFFECT
------- | ----- | -------
    `%` |     0 | nop, does nothing
    `+` |     1 | increments the current value in the opponent's program buffer, looping if necessary
    `<` |     2 | moves the data pointer to the left (*not* looping, if it goes below 0, it stays at 0)
    `>` |     3 | moves the data pointer to the right (looping if necessary)
    `,` |     4 | reserved but has no effect
    `.` |     5 | reserved but has no effect
    `!` |     6 | commit the current value to the opponent's active program buffer
    `?` |     7 | uncommit the current value (replace the current value with the value in the original program)
    `[` |     8 | start a brainfuck-style loop (loop while the value at the current location in the opponent's program buffer is not nop)
    `]` |     9 | end of a `[` loop
    `{` |    10 | loop while the opponent does not have a program pointer at the current location
    `}` |    11 | end of a `}` loop
    `:` |    12 | start a thread.  The parent thread will skip over the matching `;`, the child will loop between this and the matching `;`<br>*NOTE:* any given `:` will only fork once, then it's spent
    `;` |    13 | end of a `;` loop
    N/A |    14 | **BOMB!**  This is an important one.  If *any* program pointer (even if you have several from threading) hits this, you lose!<br>*NOTE:* You cannot set a bomb in your own program, so it doesn't have a character.
    `*` |    15 | exit - end the current thread.  If you're not in the topmost thread, you can only exit at the very end of a thread: `:......*;`<br>*NOTE:* If the current thread is the only thread, you lose!
    `@` |    16 | defect: instead of editing the opponent's program buffer, this thread will edit your own.  You can later defect back.<br>*NOTE:* If you commit a defect, *you defect* (this is to try to prevent pure manglers from being effective)


### Notes on special behavior

 - If a closing loop is found with no matching opening loop it will not only
   "ignore" the instruction but "stumble", that is, ignore the next instruction,
   too! This can be quite important for the game.  
   The reasonign behind this could have been for making sure programs run better
   when changed runtime, ie. at least terminate. It also could've just been a
   programming error.  
   *NOTE:* this "feature" might be removed in a future,
   "API-breaking" version of the program.

## Simple Stragegy
One of the most simple FukYorBrane programs is:

```
{>}[+]+++++++++++++!
```

This very simply:

 * `{>}`: tries to find the opponent's program pointer
 * `[+]`: blanks the value where you found it (as in BrainFuck, but looping values)
 * `+++++++++++++!`: sets the value there to 14 (bomb)

This simple program isn't very effective, as it has tight loops.  Tight loops
are generally best avoided, because of how bombs are usually placed.  If the
enemy finds you and places a bomb, and you're in a tight loop, you'll hit the
bomb long before you can do something about the enemy.

A slightly more complex FukYorBrane program:

```
:{>}[+[+[+[+[+[+[+[+[+[+[+[+[+[+[+]]]]]]]]]]]]]]]+++++++++++++!>%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%;
:@[>+++]!;*
```

Let's break this down:

 * `>}`: This is the same as before (and still a tight loop, which is bad).
 * `[+[+[+[+[+[+[+[+[+[+[+[+[+[+[+]]]]]]]]]]]]]]]`:This is one way to zero a
   value without a tight loop
 * `+++++++++++++!`: set the bomb
 * `>`: since we already have a bomb here, go on
 * `%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%`...: by setting a lot of nops, we
    make the :; loop less tight
 * `:@[>+++]!;`: this is a simple bomb finding algorithm:
    * it defects,
    * then loops,
    * nop'ing any bombs
    * and committing if they were a bomb

Because you can defect, you can have a self-replicating program.  But that's up
to future hackers to produce ;)

## Program Styles
Gregor has only written two major styles of programs:

 * Manglers and
 * Seekers.

Of Manglers, there are several subtypes:

 * Pure mangler: All this does is destroy code with no attempt at any logic.
   Very quick.  It's also the shortest code: `+!>`
 * Logic mangler: Usually look for loops, disable loop beginnings or ends.  That
   way, program pointers fly through the entire program, so you can bomb anywhere.

Seekers basically seek out program pointers (`{>}`), then try to bomb where the
program pointer was, since the pointer is likely to come back to the same point.
Easily catchable with a false thread.

## Looping
It is worth mentioning how many ticks different loop styles take.

If you have a loop of the style `[A]B` (where A and B are sections of code)  
It will run like so (where every character that isn't `A` or `B` is a tick):

```
[A][A][A][B...
```


If you have a loop of the style `{A}B`  
It will run like so

```
{A}{A}{A}{B...
```

However, if you have a loop of the style `A:B;C`  
It will run like so

```
A:B;B;B;B;B;B`...
  C...
```


Important to note is that while `[` and `{` will take a tick every loop, `:`
will take a tick only on the first loop.

Where loops are unmatched, any applicable jump will be skipped.  That is, in a
situation like `[A[B]`, where the `[` before `A` doesn't have a match, if it hit
that `[` and decided to jump, it would not.  
It would run like this:

```
[A[B][B][B].....
```

in either situation.  
The situation is similar for ending brackets.  This: `[A]B]C`, would run like this:

```
[A][A][A]...B]C
```

If you produce a `:` somewhere where one has already existed, it's "spent"
status will be maintained. That is, if one program did this: `:A;B`, and later,
the second program turned that into `%A;B`, then yet later, the second program
turned it back into `:A;B`. The `:` before the `A` would still be spent, and
would not fork again.  If a `:` is produced somewhere where there was not one
before, it defaults to unspent.

## Building & Running

### Building for ...
 - Linux:  
   Run GNU Make in the root directory of the project. Required tools:
    - GNU Make (if you want to use the Makefile)
    - GCC/Clang (any C99-compatible compiler should do)
    - GLibC (any C99-Std C lib should do, too)
 - OS X:  
   See instructions for Linux. If you have not set up developer tools yet, it
   will prompt you to install these.
 - Windows:  
   There are three ways of compiling for Windows:
    - cross-compiling on Linux:
      Install the MinGW toolchain for the distro of your choice and in the first
      few lines of the Makefile edit the `CC`-variable as needed, eg.  
      `CC = x86_64-w64-mingw32-cc`  
      Now Run `$ make clean && make WIN=1`
    - Installing MinGW on a Windows machine, adding the compiler to your
      `%PATH%`-variable and editing `CC` to match your needs.
    - Import the `fukyorbrane.c` into Visual Studio and compile it *with the C++
      compiler*. The MS Visual C compiler does not support many modern C
      features we use.  
      The most easy way to compile in "C++ -Mode" is to rename the file to
      `fukyorbrane.cc` -- this however will link against VSC++RT and not against
      the VSCRT.

### Running:

To run `program1.fyb` against `program2.fyb`:  
`$ fukyorbrane [program1.fyb] [program2.fyb] [v/t]`

 - first, second param: the FukYorBrane programs
 - `v`: Output 'verbose' board, ie. complete source-code and
   program-/data-pointers each round.
 - `t`: Output turn number

### Licensing and original authorship
The original code for FukYorBrane was retrieved from
[Voxelperfect](http://esoteric.voxelperfect.net/files/fyb/),
it is licensed under the
[MIT License](https://opensource.org/licenses/MIT).
