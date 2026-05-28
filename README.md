# hsh (Hou Shell) - A Robust, Memory-Safe Unix Shell in C

I'm a first-year undergraduate student majoring in Computer Science at Hangzhou Dianzi University, and I'm thrilled to have completed this little project called hsh. hsh is a modern Unix-like terminal shell implemented from scratch (I've always enjoyed building these cute little gadgets from the ground up). Actually, I don't want to describe my shell in overly technical terms; after all, it's just a toy I created on a whim as a college student. My shell uses a Lexer-Parser-Executor architecture, and I believe that while providing multi-level pipelining, conditional control chains, I/O redirection and so on, it won't suffer from annoying problems like memory leaks. After all, the valgrind tool tells me that this shell is memory-safe (perhaps my test cases aren't extreme). In short, I think I gained a lot while writing this lovely shell. My Linux system programming skills seem to have improved significantly, and it has also given me some engineering awareness .



## Core Features

I realized I shouldn't be satisfied with just a shell that uses `strtok` to parse strings typed by the user; such a simplistic approach often has significant limitations. So, I devised a plan: I divided the shell into four modules, which I called the reader, lexical analyzer, parser, and command executor. Each module performs its specific function, working together to make the shell run.The following are some characteristics of these four modules.

- **reader**: I chose to use `getline` to dynamically read and process input boundaries (`EOF`/`EINTR`). In C system programming (especially writing shell scripts or handling text input of unknown length), `getline` (POSIX.1-2008 standard) is almost a "silver bullet" for standard input reading. The beautiful function `getline` simplified my reader writing, although I still spent a lot of time on details like signal handling and error handling; perhaps these details are part of the charm of system programming.
- **Lexer**: I think this part demonstrates the intelligence of lexical analysis compared to shells using `strtok`. The built-in state machine supports single and double quote handling, escape characters, and in-place string compression optimization, accurately separating command parameters from special operators. To be honest, I spent some time considering implementing these features. I seem to have used some compiler principles knowledge, which is really useful and fascinating, especially when applied in practice.
- **Parser**: This part inevitably requires building a binary tree, and experiencing the beauty of recursion in the process of building the tree is a wonderful thing.I use a recursive descent algorithm to construct the Abstract Syntax Tree (AST).Strictly supports operator precedence:
  - `;` (command sequence)
  - `&&` / `||` (logical condition control)
  - `|` (multi-level pipes)
  - `<` / `> `/` >>` (input, output, and append redirection)
- **Executor:** This part starts to interact with the operating system kernel (requiring the use of system calls such as `fork`, `exec`, `wait`, `pipe`, `dup`, etc.). I adopted a flat pipeline scheduling algorithm to dynamically manage the child process cluster and pipe descriptors (this process was the most rewarding for me, because it requires a lot of deep thinking). Of course, I will not forget the short-circuit evaluation of the logical control chain.

Besides these four modules, my shell's memory management is rock-solid. Even with the global introduction of `setjmp`/`longjmp` exception escape paths, it still maintains perfect dynamic array auto-resize and tree-structured resource recursive release mechanisms, passing boundary and malicious input stress tests. Furthermore, I think the terminal control is quite elegant. It captures `SIGINT` (Ctrl+C) to prevent the main process from exiting and disables control character echoing (`ECHOCTL`) at the underlying termios level, providing an extremely clean terminal interaction experience.



## Architecture Design

```
[User Input] 
     │
     ▼
┌───────────┐
│ reader.c  │ ──► Use `getline` to dynamically read and process input boundaries (`EOF`/`EINTR`).
└───────────┘
     │
     ▼ (Command String)
┌───────────┐
│  lexer.c  │ ──► State machine-based token segmentation generates a flat array of tokens.
└───────────┘
     │
     ▼ (Token Stream)
┌───────────┐
│ parser.c  │ ──► Recursive descent parsing to construct an abstract syntax tree (AST)
└───────────┘
     │
     ▼ (Abstract Syntax Tree)
┌───────────┐
│executor.c │ ──► `fork`/`execvp` execution, short-circuit evaluation, pipelining and redirection
└───────────┘
     │
     ▼
[Memory reclamation]   ──► The main loop executes `cleanup_iteration` to release all heap resources for this iteration.
```



## Compilation and Execution

To compile and run this shell, I assume your machine has Linux installed. Regardless of your distribution (I use Arch Linux), as long as you have the glibc C library, gcc compiler, and make build tools installed, you can execute the following commands to compile and run it.

```bash
make
```

```
./bin/hsh
```



## Memory Test Report (Valgrind Status)

As a C programming enthusiast, I've always been extremely concerned about memory usage. Frankly, memory issues are truly awful. Therefore, one of the core design principles of this project is absolute memory safety. After completing this shell script, I conducted rigorous memory load tests on complex use cases, including complex key combinations, extremely long multi-level pipes, malicious syntax errors, and empty input.

I used the valgrind tool to perform memory testing on my shell.

```bash
valgrind --leak-check=full --show-leak-kinds=all ./bin/hsh
```

These are some test cases.

```bash
ls > out.txt |
cat < in.txt && || echo hello
echo a b c d e f g h i j k l m n o p q r s t u v w x y z 1 2 3 4 5
ls -l -a -h --color=always . .. /tmp /usr /bin > f1 > f2 > f3 > f4
echo hello | cat | cat | cat | cat | cat | cat | cat | cat | cat
echo "hello
Press Enter 5 times
```

No matter how complex the command execution path is, or if an exception occurs midway due to a syntax error, all heap memory allocations within hsh can be perfectly released (0 bytes in 0 blocks remaining), and there are 0 illegal memory access errors throughout the entire runtime lifecycle (0 errors).





