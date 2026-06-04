#
# Makefile — PS4 controller gadget for Raspberry Pi Zero W
#
# Usage:
#   make            # builds kernel.img (RASPPI=1 default)
#   make RASPPI=1   # explicit target for Pi Zero / Zero W
#   make clean
#
# Requires the Circle libraries to be built first:
#   cd circle && ./makeall RASPPI=1 && cd ..
# Or let this Makefile build them automatically via the lib rules below.
#

RASPPI    ?= 1
CIRCLEHOME = circle

INCLUDE = -I src

OBJS = $(patsubst src/%.cpp,src/%.o,$(wildcard src/*.cpp))

LIBS = $(CIRCLEHOME)/lib/usb/gadget/libusbgadget.a \
       $(CIRCLEHOME)/lib/usb/libusb.a \
       $(CIRCLEHOME)/lib/libcircle.a

include $(CIRCLEHOME)/Rules.mk

# ---------------------------------------------------------------------------
# Auto-build Circle libraries when they are missing
# ---------------------------------------------------------------------------

$(CIRCLEHOME)/lib/libcircle.a:
	$(MAKE) -C $(CIRCLEHOME)/lib RASPPI=$(RASPPI)

$(CIRCLEHOME)/lib/usb/libusb.a:
	$(MAKE) -C $(CIRCLEHOME)/lib/usb RASPPI=$(RASPPI)

$(CIRCLEHOME)/lib/usb/gadget/libusbgadget.a:
	$(MAKE) -C $(CIRCLEHOME)/lib/usb/gadget RASPPI=$(RASPPI)

-include $(DEPS)
