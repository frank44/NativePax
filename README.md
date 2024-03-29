<h1>Paxembly specifications</h1>

A purely stack based implementation of an assembly-like language with a simple garbage collector and auto-up-casting. 

Notes:<br>
The system stack contains pointers to the actual values in the heap.<br>
Labels are lines with a single token than begins with a '.'<br>
All code must contain a .START label (this label will serve as the program's entry point/main) <br>
The up-cast hierarchy is string > double > int <br>

Commands
<br>

        PUSH {literal_constant | variable_name} - pushes the value specified on the top of the system stack
                Currently supports - int, double, string

                Example: PUSH 5
                         PUSH my_var
                         PUSH "hello world"
                         PUSH 3.141592
        LOAD {variable_name} - sets the variable equal to the top of the stack being mindful of the old value that it pointed to. If this was the last pointer to some value it is deleted.
                Currently supports - int, double, string
	
                Example: LOAD my_var

        POP - pops the top of the stack (it also frees the memory pointed to by this pointer if no more variables point to this memory)

        READ {type} - reads from stdin the specified data type and pushes it onto the stack
                Currently supports - int, double, string

                Example: READ int   
                         READ double - gets the next token and interprets it as a double
                         READ string - gets the next raw string token from stdin

        ADD, SUB, MULT, DIV, MOD - pop the top two stack elements, performs the operation, and pushes the result.
                Currently supports - int, double, string (Note: strings only support ADD)

       	PRINT - if given no paramenters it simply prints the top of the stack
              - if you give it a variable name it prints the corresponding value
              - you can also just give it a literal constant

        OP{<, <=, =, >=, >, !=} - comparison operators, pops the top two stack elements, applies the operation, and then pushes the result (0 or 1)
                Currently supports - int, double, string
                
                Example: OP= (checks for equality)
                
        JMP {label} - jump to the corresponding label.     

        JZ {label} - jump if zero is at the top of the stack, note that if activated the zero is removed during the jump.

        RET - jumps back a to just after the previous JMP call
        
        EXIT - stops the program immediately
        
        