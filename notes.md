Install xc16 1.25
Install zip
`export PATH=$PATH:/opt/microchip/mplabx/v3.65/mplab_ide/bin:/opt/microchip/mplabx/v3.65/mplab_ide/`
#Run MPLAB IDEX
#Install MPLAB Code Configurator from Tools->Plugins
Run peripheral-libraries-for-pic24-and-dspic-v2.00-linux-installer.run
`apt install make`
Change all firmware/lib* projects to large code and large data
Change app_layer_1 to large code and large data

To build:
docker run --rm -it -v `pwd`:/ioio --entrypoint=/bin/bash catalia/mplab:1.25-2
cd /ioio/ytai
./tools/make-all all
./tools/make-ioio-bundle firmware/app_layer_v1/dist mcb-2 IOIO0030
                         -------------------------- ----- --------
                                      |               |      +--IOIO version
                                      |               +--Name of .ioio file to write
                                      +--Dir to build into .ioio file

ioiodude:
ground boot pin (near the sparkfun logo)
plug usb into PC (yellow light comes on, in bootloader mode)
remove ground from step 1
./ioiodude --port=/dev/ttyACM0 write ytai/mcb-13.zip./ioiodude --port=/dev/ttyACM0 write ytai/mcb-13.zip

logging:
sudo cat /dev/ttyUSB0 | sed -u '/^\s*$/d' | ts
