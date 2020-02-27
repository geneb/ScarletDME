The "qmclient" and "qmserver" files in this directory should be copied to the
/etc/xinetd.d directory.  If you don't have this directory on your system,
you'll need to install the xinetd package for your system.

This directory also contains a file named "services".  The contents of this
file should be added to your /etc/services file.

Once you've done these things, you can restart xinetd via either 
"/sbin/service xinetd start" (or restart) or "/sbin/systemctl restart xinetd"
If your system supports neither of those methods, you'll need to consult the
documentation for your distribution.

