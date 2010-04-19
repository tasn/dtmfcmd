DESCRIPTION = "A dtmf remote control"
HOMEPAGE = "http://www.binaryvision.co.il"
SECTION = "system/applications"
LICENSE = "GPLv2"
SRCNAME = "dtmfcmd"
RDEPENDS += " alsa-utils-aplay alsa-utils-alsactl alsa-utils-amixer"
DEPENDS += " glib"
PV = "0.4.1"
PR = "r0"

S = "${WORKDIR}/trunk"
inherit autotools 

#SRC_URI = "svn:///home/tom/projects/svn/dtmfcmd;module=./trunk;proto=file"
SRC_URI = "file://../trunk"
FILES_${PN} += "${datadir} ${sysconfdir}"



