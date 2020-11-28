# redflag

## TODO LIST

### Feature list to implement

* [x] Arrays
* [x] Structs
* [x] Enums
* [x] Pointers
* [x] else if
* [x] for
* [x] switch
* [x] Global variables
* [x] Implemented signed and unsigned types
* [x] Strings
* [x] Libraries
* [ ] Modules
* [ ] Operator precedence / parenthesis expressions
* [ ] Fusion parser and IR and create a true IR
* [ ] Debug information
* [ ] defer
* [ ] pass language-custom arguments
* [ ] Unions

### URGENT PRIORITY

### NORMAL PRIORITY

* [x] Fix char and string literals lexing
* [x] Expression vs statement, block vs compound statement
* [x] Function body is compound statement
* [ ] Amplify function calling
* [ ] Provide sense of scope
* [ ] Symbol types are values
* [ ] Array-based parser (bunch of nodes in dynamic arrays, indices as pointer to node). Profile gains
* [ ] rework reallocation. If there is enough space ahead of the allocated space, just modify block metadata (amplify allocation boundaries) and don't copy already existent data
* [ ] when doing standard library, redesign memcpy/memcmp so they are all safe and check size on ***BOTH*** ends
