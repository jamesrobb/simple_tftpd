# Simple Download-Only TFTP Server

This is a simple tftp server written in C that does not permit the upload of files. It supports multiple concurrent downloads, but has a max connection limit which can be changed in code.

This was made for an assignment in a 3rd year university course in computer networking.

# Usage
```
tftpd [port_number] [data_directory]
```

# Implementation

As part of the assignment the implementation will be briefly described.

We maintain an array of size MAX_CONN (some integer) that tracks information about currently connected clients. Every time a read request is received, the server will fill one of the spots in the connections array with a struct that tracks information about the connection (desired file, client port, block number, etc).

When an acknoledgement packet comes in, we search for a client connection in our connections array, if not found we return an error, otherwise we send over the next block of data.

Should a connection not perform any activity for a set amount of time (a timeout), the space it occupies in the connections array is freed.

Finally, when a block less than 512 bytes is sent out, that connection is marked as complete and the space in the array is filled.
