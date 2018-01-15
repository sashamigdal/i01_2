i01
===

Version Control
---------------

This project uses Git for version control.  Please feel free to create
branches as you wish, but make sure that the ``master`` branch builds
before pushing/propagating changes to that branch remotely.

Development Environment
-----------------------

This project relies on Vagrant_ and VirtualBox_ to create reproducible,
consistent build environments.  After installing those, including the
VirtualBox Extension Pack, run the command ``vagrant up`` anywhere in the Git
repository.  This will spawn a virtual machine and install all external
dependencies (required to develop, build, and test) onto that VM.  Once that
has completed, you should be able to run ``vagrant ssh`` to access the
development/build VM.

.. _Vagrant: http://www.vagrantup.com/
.. _VirtualBox: http://www.virtualbox.org/

The Git repository will be mounted in the ``/vagrant`` directory on the VM.
We recommend that you edit on your host machine, and build by running:

::

    vagrant ssh -- /vagrant/build.sh

To destroy the VM, run ``vagrant destroy``.  You will lose all changes made in
the VM outside of the mounted/shared ``/vagrant`` directory.  Be careful!

Should you wish to maintain a long-running VM, you can use ``vagrant suspend``
and ``vagrant resume`` to pause/unpause the VM.

Remember that you must run ``vagrant provision`` to apply any changes to the
development environment to an existing VM.  If there have been changes to the
Vagrantfile, ``vagrant reload`` will restart the VM and apply that new
configuration.

As a courtesy (and to save yourself time), please run ``bootstrap-hooks.sh``
to set up local Git hooks that will test if your code builds and tests before
committing.  (You can temporarily disable this hook for a commit by adding
``--no-verify`` to your ``git commit`` command.)

Building and Testing
--------------------

This project uses CMake to build, and GTest and CTest for testing.  The script
``build.sh`` is a shell wrapper to build the complete project (possibly in all
three popular compilers: gcc, icc, and clang).

Tests are run with ``make test`` from the CMake-generated Makefiles.
``build.sh`` runs tests if the build succeeded.

TODO: Implement continuous integration.

Dependencies
------------

These dependencies are installed by Puppet automatically during provisioning:

+------------+-----------+
| Dependency | Version   |
+============+===========+
| CMake      | >= 2.8    |
+------------+-----------+
| Doxygen    |           |
+------------+-----------+
| Boost      | >= 1.55.0 |
+------------+-----------+
| Eigen      | >= 3.2.0  |
+------------+-----------+
| IntelTBB   | >= 4.2    |
+------------+-----------+
| catdoc     |           |
+------------+-----------+
| libpcap    | >=1.5.3   |
+------------+-----------+
| jemalloc   | >=3.6.0   |
+------------+-----------+
| GTest      | >=1.7.0   |
+------------+-----------+
| Wt         |           |
+------------+-----------+


