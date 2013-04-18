#!/bin/sh
strip -o debian/i386/usr/bin/PedometerLog src/PedometerLog
cp PedometerLogScript debian/i386/usr/bin/
find debian/i386/usr/bin/ -type f -exec chmod 755 {} \;
fakeroot dpkg --build debian/i386/ .

