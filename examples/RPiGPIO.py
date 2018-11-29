"""Example script that demonstrates features of the RPiGPIO device adapter.

Kyle M. Douglass, 2018
kyle.m.douglass@gmail.com

"""
import time

import MMCorePy


PIN = 4
"""int: The pin number refers to the Broadcom numbering scheme.
"""

# Initialize the GPIO device; Pin is a pre-init property.
mmc = MMCorePy.CMMCore()
mmc.loadDevice("gpio", "RPiGPIO", "RPiGPIO")
mmc.setProperty("gpio", "Pin", "{:d}".format(PIN))
mmc.initializeAllDevices()
print("Using pin: {}".format(mmc.getProperty("gpio", "Pin")))


# Retrieve the state of the pin.
print("State: {}".format(mmc.getProperty("gpio", "State")))

# Set the pin to the "On" state.
time.sleep(1)
mmc.setProperty("gpio", "State", "1")
print("State: {}".format(mmc.getProperty("gpio", "State")))

# Set the pin to the "Off" state.
time.sleep(1)
mmc.setProperty("gpio", "State", "0")
print("State: {}".format(mmc.getProperty("gpio", "State")))
