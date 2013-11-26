SOURCE_DIR=
PARBUILD=-j10
BRANCH=syneto
CHANGESET=
BUILD_SCRIPT=./illumos-local.sh
WORKING_DIR=$(shell pwd)
DOWNLOAD_URL=http://devel.dev.syneto.net/GPL-Sources/system-storage
IPSDIR=/tank/storage-os/pkg-repos/illumos-syneto/$(BRANCH)

# Tools
SED=/usr/gnu/bin/sed
ZFS=/usr/sbin/zfs
WGET=/usr/bin/wget
TAR=/usr/gnu/bin/tar

.PHONY: $(IPSDIR) all

all: $(IPSDIR) setup_build_env setup_closed_binaries
	@echo "Making all on branch $(BRANCH)"
	@echo "Changeset is $(CHANGESET)"
	exit 0
	chown admin:staff $(WORKING_DIR)
	if /opt/onbld/bin/nightly -n $(BUILD_SCRIPT); then \
		rm -rf $(IPSDIR).orig; \
	else \
		echo "Cleaning up after failure"; \
		rm -rf $(IPSDIR); \
		mv $(IPSDIR).orig $(IPSDIR); \
		echo "Build has failed. Please check log/$(ls log| sort -r | head -n1)/nightly.log for more details"; \
		exit 1; \
	fi

update_build_script:
	@echo "Generating new build script $(BUILD_SCRIPT)"
	$(SED) -e "s@^export CODEMGR_WS=.*@export CODEMGR_WS=$(WORKING_DIR)@" \
		-e "s@^export PKGARCHIVE=.*@export PKGARCHIVE=$(IPSDIR)@" \
		./illumos.sh > $(BUILD_SCRIPT)

$(IPSDIR):
	@echo "Creating ips repo dir $(IPSDIR)"
	mkdir -p $(IPSDIR)
	chown admin:staff $(IPSDIR)
#Save original repository in case build fails -> we want a sane pkg repo always
	mv $(IPSDIR) $(IPSDIR).orig

setup_build_env: setup_closed_binaries update_build_script
	@echo "Setting up build environment ..."
	if [ ! -L ./bldenv.sh ]; then \
		ln -s usr/src/tools/scripts/bldenv.sh . ; \
	fi
	if [ ! -e .setup ]; then \
		MAKE=/usr/bin/make ksh93 bldenv.sh -d $(BUILD_SCRIPT) -c "cd usr/src && dmake setup"; \
		touch .setup; \
	fi

setup_closed_binaries: download_closed_binaries
	if [ ! -d closed ]; then \
		echo "Setting up illumos closed binaries ..."; \
		$(TAR) xvpf on-closed-bins.i386.tar.bz2 ; \
        $(TAR) xvpf on-closed-bins-nd.i386.tar.bz2 ; \
	fi

download_closed_binaries:
	$(WGET) -q -nc $(DOWNLOAD_URL)/on-closed-bins-nd.i386.tar.bz2
	$(WGET) -q -nc $(DOWNLOAD_URL)/on-closed-bins.i386.tar.bz2

