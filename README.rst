
iwtftpd
=======

"iwtftpd" is a simple TFTP server for Linux OS. (RFC1350)

Build and Install
-----------------

**Requirements:**

For building needs the Makefile generator and a optional library below.

* cmake
* libpopt

**Build:**
::

  $ cmake .
  $ make

**Install:**
::

  $ mv bin/iwtftpd /PATH_TO_YOUR_SYSTEM

Usage
-----

**iwtftpd [OPTIONS]**

*Options:*

.. csv-table::

   -4,,                     Use only IPv4
   -6,,                     Use only IPv6
   -i, --if=NETDEV,         Use bind interface only
   -d, --datastore=DIRPATH, Path of datastore
   -u, --username=USER,     Username in /etc/passwd
   -v, --verbose,           Verbose mode
   -V, --version,           Show version

Must be run as root. The root directory will be changed to the data store.

The user ID of the process will be changed to *USER* or 'nobody' (by default).

By default, the data store is '/tftpboot'.
You must have created this directory and set the permissions for *USER*.

Uninstall
---------
::

 rm /PATH_TO_YOUR_SYSTEM/iwtftpd
 rm /var/log/iwtftpd.log


