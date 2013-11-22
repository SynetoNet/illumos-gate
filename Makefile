SOURCE_DIR=
PARBUILD=-j10
BRANCH=syneto
IPS_REPO_DIR=/tank/build/repos/illumos-syneto/$(BRANCH)

WORKING_DIR=$(shell pwd)
IPSFS=$WORKING_DIR/packages

ZFS=/usr/sbin/zfs
BUILD_SCRIPT=./illumos.sh
SED=/usr/bin/sed

.PHONY: $(IPS_REPO_DIR) all

all: $(IPS_REPO_DIR) update_build_script
	@echo "Making all on branch $(BRANCH)"
	if sudo /opt/onbld/bin/nightly -n ./illumos-local.sh; then \
		rm -rf $(IPS_REPO_DIR).orig; \
	else \
		rm -rf $(IPS_REPO_DIR); \
		mv $(IPS_REPO_DIR).orig $(IPS_REPO_DIR); \
	fi; \

update_build_script:
	@echo "Updating build script $(BUILD_SCRIPT)"
	$(SED) -e "s@^export CODEMGR_WS=.*@export CODEMGR_WS=$(WORKING_DIR)@" \
	 -e "s@^export PKGARCHIVE=.*@export PKGARCHIVE=$(IPS_REPO_DIR)@" \
	 ./illumos.sh > ./illumos-local.sh

$(IPS_REPO_DIR):
	@echo "Creating ips repo dir $(IPS_REPO_DIR)"
	mkdir -p $(IPS_REPO_DIR)
#Save original repository in case build fails -> we want a sane pkg repo always
	mv $(IPS_REPO_DIR) $(IPS_REPO_DIR).orig
