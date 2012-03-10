# gntp-send
=========

* a command line binary for sending notifications to Growl
* a library for integrating Growl into you C or C++ based applications

## Platforms
---------

Sending notifications from Windows/Linux/ Mac is supported. Unixes in general should be supported but are untested.

Notifications may be received by Growl on Mac or GrowlForWindows on Windows.

* windows
* linux /mac


## C Functions
-----------

```c
int growl(server, appname, notify, title, message, icon, password, url)
```

Send tcp notification. Currently this is supported only by GrowlForWindows

* `server` - hostname where Growl is running, port can optionally be specified e.g `localhost:23053`
* `appame` - name for application sending notification 
* `tite` - notification title
* `message` -  notification text
* `icon` - optional url or local file path for notification icon or NULL
* `password` - password for Growl
* `url` - website to direct user to if they click notification or NULL

```c
int growl_udp(server, appname, notify, title, message, icon, password, url)
```

Send udp notification. This is supported by both GrowlForWindows and Mac Growl.

As above except icon and url are ignored.

## C++ Objects
-----------

```cpp
Growl *grow = new Growl(protocol, password, appname, notifications, notifications_count);
growl->Notify(notification1, title, message);
growl->Notify(notification2, title, message);
```

## Building for MinGW
------------------

MinGW is basically gcc for Windows. make is required to build using MinGW.

To build the gntp-send.exe executable and libraries required for integration run

```cmd
    mingw32-make -f Makefile.w32
```

## Building for Visual Studio
--------------------------

nmake is required to build using Visual Studio.

To build the gntp-send.exe executable and libraries required for integration run

```cmd
    nmake -f Makefile.msc
```

## Building for UNIX
-----------------

Max OS X is basically a Unix varient and is covered by these instructions.

gcc/g++ and make are required for building.

To build gntp-send and the libraries required for integration run

```cmd
    make -f Makefile
```

## Precompiled Binaries
--------------------

Peter Sinnott provided windows binaries, see:  
https://github.com/psinnott/gntp-send

For ubuntu linux, use ppa:
https://launchpad.net/~mattn/+archive/gntp-send

## License
-------

gntp-send and libraries distributed under BSD license.

## Contributors
----------

Please fork on github, and send me pull-requests.

Note to keep my code style.

Authors
-------

Yasuhiro Matsumoto `<mattn.jp@gmail.com>`

Peter Sinnott `<link@redbrick.dcu.ie>`

Dither `<dithersky@myopera.com>`
