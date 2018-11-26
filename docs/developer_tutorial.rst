Developer Tutorial
==================

This tutorial will demonstrate how to use `RPi-DeviceAdapters`_ to
write, build, and deploy a simple `Micro-Manager`_ device adapter for
the Raspberry Pi on a laptop or desktop. RPi-DeviceAdapters provides
these capabilities to make development easier; you do not need to
develop new device adapters directly on the Raspberry Pi.

.. _RPi-DeviceAdapters: https://github.com/kmdouglass/RPi-DeviceAdapters
.. _Micro-Manager: https://micro-manager.org/

Linux
-----

Requirements
++++++++++++

- `Git`_
- `Subversion`_ (for the Micro-Manager dependencies)
- `Docker`_
- `Docker Compose`_
- `Make`_
- `QEMU`_

.. _Git: https://git-scm.com/downloads
.. _Subversion: https://subversion.apache.org/
.. _Docker: https://docs.docker.com/install/
.. _Docker Compose: https://docs.docker.com/compose/
.. _Make: https://www.gnu.org/software/make/
.. _QEMU: https://www.qemu.org/

QEMU installation
.................

The QEMU emulator is used to emulate a ARM processor architecture on a x86_64 system. It is setup
as follows:

On Ubuntu, install the emulation packages with the commands:

.. code-block:: console

   $ sudo apt update
   $ sudo install qemu qemu-user-static qemu-user binfmt-support

If you are not using Ubuntu, search for and install these packages in your system's respective
package manager. Next, register QEMU in the build agent:

.. code-block:: console
		
   $ docker run --rm --privileged multiarch/qemu-user-static:register --reset


Setup
+++++

Begin by opening a shell, cloning the RPi-DeviceAdapters repository,
and navigating inside the root directory of the cloned repository.

.. code-block:: console

   $ # For HTTPS, use https://github.com/kmdouglass/RPi-DeviceAdapters.git
   $ git clone git@github.com:kmdouglass/RPi-DeviceAdapters.git
   $ cd RPi-DeviceAdapters

Inside you will find a folder named `ci` (for continuous
integration). This folder contains all the tools necessary for
developing a new device adapter.

Next, we use the `ci/prebuild.sh`_ script to checkout the
Micro-Manager source code and 3rdpartypublic dependencies. These will
be placed into a directory named `/opt/rpi-micromanager`. It is
required by the build tool's `docker-compose.yml`_ file to place the
development files here; let's first create it and set its ownership:

.. code-block:: console

   $ sudo mkdir -p /opt/rpi-micromanager
   $ sudo chown $USER:$USER /opt/rpi-micromanager

If you do not want to place the source code in this directory, then
you can either:

1. create a symlink at `/opt/rpi-micromanager` that points to your
   alternative directory, or
2. modify `docker-compose.yml`_ to point towards your alternative
   directory.

The build container uses `ccache`_ to decrease the build time. ccache
requires that there be a directory in your `$HOME` folder named
`.ccache` to store the cached artifacts; it will automatically be
created for you if it does not already exist when you run the prebuild
script.

Let's run the prebuild script now:
   
.. code-block:: console

   $ ci/prebuild.sh /opt/rpi-micromanager

This step usually takes a few minutes due to the large size of the
3rdpartypublic repository. After it has completed, you should find
the following inside `/opt/rpi-micromanager`:

.. code-block:: console

   $ tree -L 1 /opt/rpi-micromanager
   /opt/rpi-micromanager
   ├── 3rdpartypublic
   └── micro-manager

.. _ci/prebuild.sh: https://github.com/kmdouglass/RPi-DeviceAdapters/blob/master/ci/prebuild.sh
.. _docker-compose.yml: https://github.com/kmdouglass/RPi-DeviceAdapters/blob/master/ci/build/docker-compose.yml
.. _ccache: https://ccache.samba.org/

Writing device adapters
+++++++++++++++++++++++

Writing a general purpose Micro-Manager device adapter is outside the
scope of this tutorial; help may be found on the `Micro-Manager
website`_ and `the mailing list`_. Here we discuss how to build a
simple device adapter for the Raspberry Pi. The device adapter will
have a single property that can be switched between two states: `on`
and `off`.

Navigate to the device adapters folder inside the RPi-DeviceAdapters
folder.

.. code-block:: console

   $ cd src/DeviceAdapters

For the sake of this tutorial, delete the folder named
`RPiTutorial`. We will recreate it and its contents next.

.. code-block:: console

   $ rm -rf RPiTutorial

   # Recreate the (empty) folder
   $ mkdir RPiTutorial

Next, create three empty files named `RPiTutorial.h`,
`RPiTutorial.cpp`, and `Makefile.am` inside this folder.

.. code-block:: console

   $ cd RPiTutorial
   $ touch RPiTutorial.h RPiTutorial.cpp Makefile.am

With your text editor, open the file named **RPiTutorial.h**, and
enter the following code:

.. literalinclude:: ../src/DeviceAdapters/RPiTutorial/RPiTutorial.h
   :language: cpp
   :linenos:

The most important method defined in this header file is
`OnSwitchOnOff(MM::PropertyBase* pProp, MM::ActionType eAct)`, which
is the callback method that is called whenever the switch is
flipped. The internal state of the switch is stored in the private
variable `switch_`. All other methods are required by the
`CGenericBase` API.

Now let's implement the switch. Open the file **RPiTutorial.cpp** and
enter the following lines:

.. literalinclude:: ../src/DeviceAdapters/RPiTutorial/RPiTutorial.cpp
   :language: cpp
   :linenos:

Most of this code is boilerplate, i.e. code that is required by the
MMDevice API but that does not directly affect the functionality that
the user sees. The property that implements the On/Off switch is
created here:

.. literalinclude:: ../src/DeviceAdapters/RPiTutorial/RPiTutorial.cpp
   :language: cpp
   :linenos:
   :lines: 118-123

Its switching behavior is defined here:

.. literalinclude:: ../src/DeviceAdapters/RPiTutorial/RPiTutorial.cpp
   :language: cpp
   :linenos:
   :lines: 148-165

Now, open `Makefile.am` and add the following lines:

.. literalinclude:: ../src/DeviceAdapters/RPiTutorial/Makefile.am
   :linenos:

This file instructs Autotools how to create the Makefile when the code
is compiled.

.. _Micro-Manager website: https://micro-manager.org/wiki/Building_Micro-Manager_Device_Adapters
.. _the mailing list: https://micro-manager.org/wiki/Micro-Manager%20Community

Building the libraries
++++++++++++++++++++++

To build the Micro-Manager core and device adapter that we just wrote,
we first need to add RPiTutorial to the list of device adapters in
`src/DeviceAdapters/Makefile.am` and
`src/DeviceAdapters/configure.ac`. Here is what `Makefile.am` looks
like:

.. literalinclude:: ../src/DeviceAdapters/Makefile.am
   :linenos:

And here is an excerpt of the relevant part of `configure.ac` that
should be modified. In both files, the list of DeviceAdapters should
be in alphabetical order.

.. literalinclude:: ../src/DeviceAdapters/configure.ac
   :linenos:
   :lines: 412-419

Now that we have written our device adapter and updated the Autotools
files, we need to merge our code with the Micro-Manager source
code. This is easily performed with the `ci/merge.sh`_ utility script:

.. code-block:: console

   $ ci/merge.sh /opt/rpi-micromanager

Each time you change the code you can run this script and it will copy
only the changed files into the appropriate directories of
`/opt/rpi-micromanager/` (or whatever directory you pass as an
argument).

The final step is to compile Micro-Manager and the libraries for our
device adapter. If this is the first time you are doing this, it may
take a long time (around half an hour). Subsequent compilations should
be three or four times faster because the compiler cache will have
been built.

To begin compilation, change into the `ci/build` directory.

.. code-block:: console

   $ cd ../../../ci/build

The build will be performed inside a Docker container that contains
the build dependencies and `QEMU`_, the emulator for the ARM processor
architecture. Having a Docker image that is already configured for
compilation ensures that you will have the proper dependencies without
having to manually configure your environment. To download the Docker
image from Dockerhub, run the following command.

.. code-block:: console

   $ docker-compose pull

Finally, begin the compilation by running

.. code-block:: console

   $ docker-compose up

If all goes well, then you will find the build artifacts in
`/opt/rpi-micromanager/build` at the end of the compilation:

.. code-block:: console

   $ tree /opt/rpi-micromanager/build
   /opt/rpi-micromanager/build/
   ├── lib
   │   └── micro-manager
   │       ├── libmmgr_dal_DemoCamera.la
   │       ├── libmmgr_dal_DemoCamera.so.0
   │       ├── libmmgr_dal_RPiGPIO.la
   │       ├── libmmgr_dal_RPiGPIO.so.0
   │       ├── libmmgr_dal_RPiTutorial.la
   │       ├── libmmgr_dal_RPiTutorial.so.0
   │       ├── _MMCorePy.la
   │       ├── MMCorePy.py
   │       └── _MMCorePy.so
   └── share
       └── micro-manager
           └── MMConfig_demo.cfg

(The contents of your build directory may be different depending on
what device adapters were built.) The libraries for the RPiTutorial
device adapter are the files `libmmgr_dal_RPiTutorial.la` and
`libmmgr_dal_RPiTutorial.so.0`. In addition, RPi-DeviceAdapters builds
the Micro-Manager Python wrapper. The relevant files for the wrapper
are `_MMCorePy.*` and `MMCorePy.py`. The Python wrapper may be
imported into a Python script to gain access to the methods in the
Micro-Manager core.

Whenever you make changes to your code during development, you will
need to run the `ci/merge.sh` script to copy the changes into
`/opt/rpi-micromanager` before recompiling. It would also be good to
occassionally pull any updates to the build container by running
`docker-compose pull`, but this should rarely be necessary.

.. _ci/merge.sh: https://github.com/kmdouglass/RPi-DeviceAdapters/blob/master/ci/merge.sh
.. _QEMU: https://www.qemu.org/

Deploying the app
+++++++++++++++++

At this point, you may transfer the compiled librariers to your
Raspberry Pi for use. However, manual transfers of the libraries can
be cumbersome. Furthermore, it can be diffcult for others to benefit
from your work if they have to recompile your source code on their
own. For these reasons, RPi-DeviceAdapters provides tools to create a
Docker-based app that can easily be uploaded and downloaded from
`Docker Hub`_ for on-demand use.

To create the app, first navigate to the `ci/app` folder.

.. code-block:: console

   $ cd ../app

Next, use the Makefile to build the Docker image that contains the
app.

.. code-block:: console

   $ make build

While creating the image, the Makefile will copy the contents of
`/opt/rpi-micromanager/build` into the image and configure the Python
environment. (You will need to edit the Makefile and change the
location of the build artifacts if you are using a directory other
than `/opt/rpi-micromanager`.)

Let's verify that the image has been built. Though your output will
differ slightly, you should see something similar to the output found
below.

.. code-block:: console

   $ docker image ls
   REPOSITORY                        TAG                                              IMAGE ID            CREATED             SIZE
   kmdouglass/rpi-micromanager       build                                            24d67a46e281        5 days ago          745MB

At this point, you will need to create a `Docker Hub`_ account if you
do not already have one and login via the command line.

.. code-block:: console

   $ docker login

The final step before uploading the image is to retag it so that it
points to your Docker Hub repository and not the default one.

.. code-block:: console

   $ docker tag kmdouglass/rpi-micromanager USERNAME/rpi-micromanager

Here, `USERNAME` is your Docker Hub username. Finally, we can upload
the image:

.. code-block:: console

   $ docker push USERNAME/rpi-micromanager

and, from the Raspberry Pi, download the image:

.. code-block:: console

   $ docker pull USERNAME/rpi-micromanager

You will need Docker installed on your Raspberry Pi to pull the image.

.. _Docker Hub: https://hub.docker.com/
