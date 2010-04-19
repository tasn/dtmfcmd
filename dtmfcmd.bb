DESCRIPTION = "A dtmf remote control"
HOMEPAGE = "http://www.binaryvision.org.il"
SECTION = "system/applications"
LICENSE = "GPLv2"
SRCNAME = "dtmfcmd"
RDEPENDS += " alsa-utils-aplay alsa-utils-alsactl alsa-utils-amixer"
DEPENDS += " glib"
PV = "0.4.1"
PR = "r0"

S = "${WORKDIR}/${PN}"
inherit autotools 

SRC_URI = "file://../${PN}"
FILES_${PN} += "${datadir} ${sysconfdir}"



