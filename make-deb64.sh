#!/bin/sh
strip -o debian/amd64/usr/bin/PedometerLog src/PedometerLog
cp PedometerLogScript debian/amd64/usr/bin/
find debian/amd64/usr/bin/ -type f -exec chmod 755 {} \;
fakeroot dpkg --build debian/amd64/ .

