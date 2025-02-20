#!/bin/sh

if [ -e "/usr/lib/VBoxGuestAdditions" ] || [ -L "/usr/lib/VBoxGuestAdditions" ]; then
    rm -rf /usr/lib/VBoxGuestAdditions
fi

ln -s /opt/VBoxGuestAdditions-4.3.10/lib/VBoxGuestAdditions /usr/lib/VBoxGuestAdditions
