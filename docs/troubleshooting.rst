Troubleshooting
===============

"Failed to open /dev/video0" when using the Video4Linux device adapter
----------------------------------------------------------------------

If you do not see a device file named `video0` inside the `/dev` folder of your Raspberry Pi, then
you may need to load the bcm2835-v4l2 kernel module.

.. code-block:: console

   $ sudo rpi-update

   # Restart the Pi, then run the following command
   $ sudo modprobe bcm2835-v4l2

Verify that `video0` exists by looking for the `/dev/video0` output from the following command. (No
output means that the file is not present.)

.. code-block:: console

   $ ls /dev | grep video

To ensure that the bcm2835-v4l2 kernel module is loaded at startup, add it to `modprobe.d`_.

.. _modprobe.d: https://linux.die.net/man/5/modprobe.d
