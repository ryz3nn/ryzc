# Note from dependablec.org - Eskil Steenberg

## Introduction 
- Elevate warning to error or ignore it based of preference

## Concept
- As-if: Compiler can executed in different order as long as give right output (reoder, pipelining, ...)

- Observed behavior (file i/o, i/o, state, return value) is only dependable thing.
- Undefine behavior: compiler assumed UB not be happened, if condition is UB, it can remove all code from this branch
- Principle: Just don't do that - by using small reliable subset of c98
- Common UB traps:
    - Signed interger overflow
    - Overshift
    - Alias violate
    - Read unalloacted memory, out-of-bound array
    - Dereference NULL pointer

## Keyword
- Flow control: Can not declare variable in if/for like for(int i = 0)
- auto and reserved keyword (from both c and c++) is not dependable

## Types
- Little endian: is popular on most machine. With this can can read from smallest number to biggest. 
    - 1337: seven thirty three hundred one thounsand (little)
            one thounsand thirty three hundred seven (big)

- Float: some architect handle float in different ways rather than FPU. so compare float after make calculation may be not safe.<br>
x = 6.0/2; if (x == 3.0) // might not be safe
- Bool: type isn't existed in C89, author recommend use int for boolean: 0 is false and not 0 is true

### Initialize variable
- Always init var from begin of scope, if not it can ignore init phase (goto before init, but in label use this variable which hasn't been initialzed)
- Priortize declare array with size
- Empty string (\0) - NULL (0x0 ~ (void*)0) - Zero value (0)
- Don't init everything with 0 as default, can write custom pattern for debug


## Operator
- Check float whether is Not-a-Number by compare itself<br>
    (f != f) // f is NaN
- <, > with pointer with different provenance is not dependable<br>
    int a[2] b, pa0*, pa1*; pb < pa1 // Not dependable: pb is pointer to variable b, and pa1 is pointer to element 2 of array
- Shift: some UBs: shift with sign number, or shift with range larger tha itself. Tips: mask number of bits need shift with range of Operator be shifted
