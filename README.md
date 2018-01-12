# Boot Update Manager: Loader and Utilities


Overview
--------


EFI-System partition directory layout
-------------------------------------


Update/Fallback State-machine
-------------------------------


State-File Format
-----------------


Boot Update Manager Flow Chart
-------------------------------


    root BUM
    --------


    configuration BUM
    ------------------


User Space Utilities
--------------------


Example Utility Usage
---------------------

1) State initialization during installation:

2) State operation immediately after complete sucessful boot

3) State query to determine which partition to update

4) State operation before initiating update

5) State operation after completing update before reboot


Example: RTS Hypervisor
-----------------------

    Run-Time Environment
    --------------------


    Boot Chain (Including BUM)
    --------------------------


    Partition Layout
    ----------------


    Notes on Installation
    ---------------------


    Complete Set of Boot Objects for RTS-Hypervised Launch
    ------------------------------------------------------


    Chain-of-Trust for Verification
    -------------------------------


    Update Package for A/B Update
    ------------------------------

    package manifest:
        signed configuration BUM
        signed boot loader for A configuration
        signed boot loader for B configuration
        hyperviser binary image
        hypervisor license file
        hypervisor configurtaion file for A configuration
        hypervisor configurtaion file for B configuration
        kernel image
        bootmgfw.efi

Building the Utilities
----------------------

Current (temporary) build instruction for user-level utilities:

        export BUILD_TYPE=executable
        export ARCH=amd64
        bash -c build/build.sh

Building the Boot Update Manager
--------------------------------


