# drawing-lang-interpreter
Practical assignments for the XDU compiler course: 
The toy interpreter of function drawing language, 
written by XDU *Zhang Yi(19030500131)*.

### Introduction
This is an interpreter of a function drawing language. 
The language uses a simple syntax to specify various 
details of the function to be drawn.

I plan to use C++(C++17) to complete the interpreter and use 
some third-party libraries, including
+ [OpenCV](https://github.com/opencv/opencv): Used to generate a drawn image of the function.
+ [googletest](https://github.com/google/googletest): Used for unit test.

This is my first attempt to write a toy interpreter, 
so obviously it will have many shortcomings. I try to 
make myself able to deal with various practical 
situations, for which I refer to the implementation of 
LLVM/clang a lot.

### Grammar
It will be completed when I write the parser.

### Development progress
1. Write basic facilities.
   + `string_ref`: This is a similar implementation 
   of LLVM `StringRef` class.
   + `file_manager`: This class represents a single 
   source file and manages the lifetime of the buffer.
2. Lexer
   + `token_kind`: Defines the kind of token. 