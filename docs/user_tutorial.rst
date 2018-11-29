User Tutorial
=============

This tutorial is an introduction to using the `RPi-DeviceAdapters`_
Docker-based application. The application is built around the
`Micro-Manager`_ Python wrapper and will show you how to run a simple
script that communicates with the Micro-Manager core.

The steps in this tutorial should be executed either in a terminal
running directly on a Raspberry Pi or through ssh. It is assumed that
the Raspberry Pi is running a recent version of the `Raspbian`_
operating system, though the steps listed here may work on other
Linux-based operating systems as well.

.. _RPi-DeviceAdapters: https://github.com/kmdouglass/RPi-DeviceAdapters
.. _Micro-Manager: https://micro-manager.org/
.. _Raspbian: https://www.raspbian.org/

Prerequisites
-------------

Begin by opening a terminal window (also known as a shell).

If you do not already have Docker installed on your Raspberry Pi, you
may install it by running the command:

.. code-block:: console

   $ curl -sSL https://get.docker.com | sh

Follow the steps described in the installation script. After the
installation has finished, you may optionally add your user to the
`Docker group`_ so that you do not need to enter `sudo` before running
Docker commands.

.. code-block:: console

   $ sudo groupadd docker
   $ sudo usermod -aG docker $USER

Log out and log back in for the changes to take effect.

You will also likely want to grab the venv package for Python from the
Raspbian package manager.

.. code-block:: console

   $ sudo apt-get install python3-venv

Create a new virtual environment for the application to isolate it
from the rest of your system:

.. code-block:: console

   $ python3 -m venv ~/venvs/mm

`~` corresponds to your home folder; you may instead replace
`~/venvs/mm` with any directory that you wish. Next, activate the
virtual environment:

.. code-block:: console

   $ source ~/venvs/mm/bin/activate

You should see the name of the venv (in this case, `mm`) at the start
of the command line. Whenever you want to stop working on the project,
type `deactivate`. To reactivate the venv, simply rerun the command
above.
   
.. _Docker group: https://docs.docker.com/install/linux/linux-postinstall/#manage-docker-as-a-non-root-user

Installation
------------

To install RPi-DeviceAdapters into the venv, run the following
command:

.. code-block:: console

   $ pip install tacpho.adapters

It will be assumed throughout the rest of this tutorial that you have
added your user to the Docker group. (See the previous section for
details.)

Next, download the latest Docker image of the application:

.. code-block:: console

   $ mm.py pull

This command may take several minutes before it completes as it
downloads the application from DockerHub. `mm.py` is a convenience
script for interacting with RPi-DeviceAdapters' Docker resources. To
see its help message, type

.. code-block:: console

   $ mm.py --help

Execute a script
----------------

Open your text editor and enter the following code:

.. literalinclude:: ../examples/RPiTutorial.py
   :language: python
   :linenos:

Save the text to a file named `tutorial.py.` This is just a short
Python script that uses the Micro-Manager Python API to load the
tutorial device adapter. It will report the value of a "switch", flip
its value, and then print the new value.

To run the tutorial, enter the following command from the same folder
that contains the script you just saved (be sure that the virtual
environment in which you installed `tacpho.adapters`_ is active).

.. code-block:: console

   $ mm.py run tutorial.py

You should see the output from the script appear in your console.

.. _tacpho.adapters: https://pypi.org/project/tacpho.adapters/

Next steps
----------

Example scripts for other device adapters may be found in the `examples`_ folder of the
RPi-DeviceAdapters root directory. Check out the `Micro-Manager documentation`_ on its Python
interface for more information about interacting with the Micro-Manager core.

Do not forget to update the RPi-DeviceAdapters application when new
versions and device adapters become available by running `mm.py pull`.

.. _examples: https://github.com/kmdouglass/RPi-DeviceAdapters/tree/master/examples
.. _Micro-Manager documentation: https://micro-manager.org/wiki/Using_the_Micro-Manager_python_library
