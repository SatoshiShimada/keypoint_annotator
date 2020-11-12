#!/bin/sh

# memo
#
# 1. install Qt (>= 5.9.5)
# 2. fix error
#   - install libxcb-xinerama0 from here(https://launchpad.net/ubuntu/xenial/amd64/libxcb-xinerama0/1.11.1-1ubuntu1)

#QMAKE=/opt/Qt/5.15.0/gcc_64/bin/qmake
QMAKE=qmake

${QMAKE} -project
echo 'QT += widgets' >> *.pro
${QMAKE}

