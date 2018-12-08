"""Example script that demonstrates features of the Video4Linux device adapter.

Kyle M. Douglass, 2018
kyle.m.douglass@gmail.com

"""
import numpy as np
import MMCorePy

# Initialize the camera
mmc = MMCorePy.CMMCore()
mmc.loadDevice("camera", "video4linux2", "Video4Linux2")
mmc.initializeAllDevices()
mmc.setCameraDevice("camera")

# Snap an image and store it in a NumPy array
mmc.snapImage()
img = mmc.getImage()
