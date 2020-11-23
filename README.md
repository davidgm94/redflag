# redflag

## TODO LIST

### URGENT PRIORITY

* [x] Admit binary expression in variable assignments

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

### Feature list to implement

* [x] Arrays
* [x] Structs
* [ ] Enums
* [ ] Unions
* [ ] Pointers
* [ ] else if
* [ ] defer
* [ ] for
* [ ] switch
* [ ] Global variables
* [ ] pass language-custom arguments
* [ ] Rest of primitive types
* [ ] Debug information
* [ ] Modules
* [ ] Libraries