<h1>axembly specifications</h1>

An purely stack based implementation of an assembly-like language with a simple garbage collector.

Notes:<br>
The system stack contains pointers to the actual values in the heap.<br>
Code must stand and end with the following labels .start & .end<br>

Commands
<br>

        PUSH {literal_constant | variable_name} - pushes the value specified on the top of the system stack
                Currently supports - int

                Example: PUSH 5
                         PUSH my_var
        LOAD {variable_name} - sets the variable equal to the top of the stack being mindful of the old value that it pointed to. If this was the last pointer to some value it is deleted.
                Currently supports - int
	
                Example: LOAD my_var

        POP - pops the top of the stack (it also frees the memory pointed to by this pointer if no more variables point to this memory)

        READ {type} - reads from stdin the specified data type and pushes it onto the stack
                Currently supports - int

                Example: READ int

        ADD, SUB, MULT, DIV, MOD - pop the top two stack elements, performs the operation, and pushes the result.
                Currently supports - int

       	PRINT - if given no paramenters it simply prints the top of the stack
              - if you give it a variable name it prints the corresponding value
              - you can also just give it a literal constant

        OP{<, <=, =, >=, >} - comparison operators, pops the top two stack elements, applies the operation, and then pushes the result (0 or 1)
	        Currently supports - int        