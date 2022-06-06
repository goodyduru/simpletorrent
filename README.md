# Simpletorrent

Simpletorrent is a BitTorrent CLI client that downloads files. It is written in C.

It's a project I worked on to improve my understanding of C and networking. It's a very basic implementation of the Bittorrent protocol (version one).

### Things I've Learnt
- The BitTorrent protocol
- Sockets programming
- Byte arrays
- Using a Debugger (LLDB)
- HTTP Protocol
- Endianess
- Open Addressing Hash Table
- Concurrency in C
- Bencode Parsing

### What this can't do
Because this project is for my learning, I didn't implement the following:
- Distributed Hash Tree
- Peer Exchange
- A network listener (this allows other peers to connect to you without you initiating)
- Rarest piece handler
- Reconnection to more peers at scheduled times

### Compilation
This program was written and tested on a Mac OS X (Big Sur). I believe it should be easily compiled on other Unix-like OS with minimal changes. Before you compile, make sure you change the openssl location [here](https://github.com/goodyduru/simpletorrent/blob/075c1460c81a1cb66281497638dd96489477941a/src/Makefile#L5) and [here](https://github.com/goodyduru/simpletorrent/blob/075c1460c81a1cb66281497638dd96489477941a/src/Makefile#L11) to the correct one on your system. Once that is done, type the following in your terminal
```
    export LDFLAGS=-L/usr/local/opt/openssl/lib
    export CPPFLAGS=-I/usr/local/opt/openssl/include
    export PKG_CONFIG_PATH=/usr/local/opt/openssl/lib/pkgconfig
    LD_LIBRARY_PATH=/usr/local/opt/openssl/lib:"${LD_LIBRARY_PATH}"
    CPATH=/usr/local/opt/openssl/include:"${CPATH}"
    PKG_CONFIG_PATH=/usr/local/opt/openssl/lib/pkgconfig:"${PKG_CONFIG_PATH}"
    make
```

### Running the program
All you just have to is run:
`./simpletorrent {path_to_torrent_file}`

This will download the file to the same location as your binary.

### Sources
I'm so grateful to
[Gallexis](https://github.com/gallexis/pytorrent) (80% of this program design was inspired by this) and the 
[BitTorrent Unofficial Spec](https://wiki.theory.org/index.php/BitTorrentSpecification). Thank you guys.

### Contributions
Contributions are welcome, you can easily ask me questions here. Pull requests are also welcome. We are all here to learn!

