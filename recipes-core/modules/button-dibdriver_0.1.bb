SUMMARY = "Simple button driver for BeagleBone Green"
DESCRIPTION = "nthn"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"


TARGET_CC_ARCH += "${LDFLAGS}"

inherit module

SRC_URI = "file://button-dibdriver.c file://Makefile"

S = "${WORKDIR}"
