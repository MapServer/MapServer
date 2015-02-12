#!/bin/sh

if [ ! -e "/usr/lib/VBoxGuestAdditions" ]; then
    ln -s /opt/VBoxGuestAdditions-4.3.10/lib/VBoxGuestAdditions /usr/lib/VBoxGuestAdditions
fi
