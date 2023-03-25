# Moribund

An adventure in computer architecture.

## Architecture

- big-endian

### Registers

- pc (program counter, 2 bytes)
- ps (primary stack selection, int 0-3)
- ss (secondary stack selection, int 0-3)
- os (operating size, int 1,2,4)
- tmp (temporary, 4 bytes)
- tar (jump target, 2 bytes)
- sc (set counter, int 0-3)

### Stacks

- sa (general purpose stack A, 1024 bytes)
- sb (general purpose stack B, 1024 bytes)
- sc (general purpose stack C, 1024 bytes)
- sd (general purpose stack , 1024 bytes)
- there are internal registers to track the tops of the stacks

### Opcodes

| opcode | name | meaning |
|--------|------|---------|
| 0x00   | nop  | does nothing |
| 0xFF   | hlt  | quits program |
| 0x1A   | 1sa  | primary select sa |
| 0x1B   | 1sb  | primary select sb |
| 0x1C   | 1sc  | primary select sc |
| 0x1D   | 1sd  | primary select sd |
| 0x2A   | 2sa  | secondary select sa |
| 0x2B   | 2sb  | secondary select sb |
| 0x2C   | 2sc  | secondary select sc |
| 0x2D   | 2sd  | secondary select sd |
| 0x01   | ss1  | set operating size to 1 byte |
| 0x02   | ss2  | set operating size to 2 bytes |
| 0x04   | ss4  | set operating size to 4 bytes |
| 0x50   | pop  | pop from primary selected stack into tmp |
| 0x51   | psh  | push to primary selected stack from tmp |
| 0x52   | dup  | duplicates top item of primary selected stack |
| 0x5E   | set  | pushes the next operating size's worth of bytes from program source into primary |
| 0x99   | jmp  | jump to address in tar |
| 0x9C   | jcm  | jump to address in tar if primary < secondary |
| 0x9E   | jeq  | jump to address in tar if primary == secondary |
| 0x90   | jzr  | jump to address in tar if primary == 0 |
| 0x80   | add  | stores (primary + secondary) in tmp |
| 0x81   | sub  | stores (primary - secondary) in tmp |
| 0x82   | mul  | stores (primary * secondary) in tmp |
| 0x83   | div  | stores (primary / secondary) in tmp |
| 0x84   | mod  | stores (primary % secondary) in tmp |
| 0x40   | not  | inverts tmp |
| 0x41   | and  | stores bitwise AND of primary and secondary in tmp |
| 0x42   | orr  | stores bitwise OR of primary and secondary in tmp |
| 0x43   | xor  | stores bitwise XOR of primary and secondary in tmp |
| 0x44   | lsh  | logical left shifts tmp |
| 0x45   | rsh  | logical right shifts tmp |

Most operations scale based on operating size.

## TODO

- I/O
- writing to tar somehow
- fetch/swap/rotate stack operations
- unsigned and signed (2's complement) math modes
- maybe graphics? (probably via raylib)
- increase ram size
