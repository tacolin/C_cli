# Quagga-like command line interface

This is a quagga-like (or cisco-like?) command line interface.

It will open a tcp socket to listen / accept new telnet connection.

Linux only.

## File Descriptions

| File        | Descriptions                        |
|-------------|-------------------------------------|
| array.c     | like quagga vector structure        |
| cli.c       | cli library implementations         |
| tcp.c       | a wrapper of tcp socket             |
| history.c   | history structure and functionality |
| cli_test.c  | test program                        |

## How to compile?

If you want to compile a program and run directly:

    $ gcc array.c cli.c tcp.c history.c cli_test.c -o cli_test

If you want to build a library:

    $ gcc --shared array.c cli.c tcp.c history.c -o libcli.so

## How to test?

    $ ./compile.sh
    $ ./cli_test

Open another terminal

    $ telnet 0 5000

## Document?

You could read the cli_test.c. Most kinds of usage will be in there.
