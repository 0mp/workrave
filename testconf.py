#!/usr/bin/python

import os;
import sys;

options = [ "gconf",
            "xml",
            "gnome",
            "distribution",
            "gstreamer",
            "dbus",
            "exercises",
            "debug"]

for i in range(0, 255) :
    conf = "./configure ";
    for j in range(0, 6) :
        if i & (1 << j) :
            conf = conf + "--disable-" + options[j] + " "
        else :
            conf = conf + "--enable-" + options[j] + " "

    sys.stderr.write(conf + "\n\n\n");
    os.system(conf);
    os.system("make");
    os.system("make distclean");
    

