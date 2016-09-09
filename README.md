# Simple Download-Only TFTP Server

This is a simple tftp server written in C that does not permit the upload of files. It supports multiple concurrent downloads, but has a max connection limit which can be changed in code.

This was made for an assignment in a 3rd year university course in computer networking.

# Usage
```
tftpd [port_number] [data_directory]
```
