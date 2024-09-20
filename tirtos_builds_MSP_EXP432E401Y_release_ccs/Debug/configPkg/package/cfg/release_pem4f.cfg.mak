# invoke SourceDir generated makefile for release.pem4f
release.pem4f: .libraries,release.pem4f
.libraries,release.pem4f: package/cfg/release_pem4f.xdl
	$(MAKE) -f C:\Users\priva\ONEDRI~1\Documents\ccs_workspace\tirtos_builds_MSP_EXP432E401Y_release_ccs/src/makefile.libs

clean::
	$(MAKE) -f C:\Users\priva\ONEDRI~1\Documents\ccs_workspace\tirtos_builds_MSP_EXP432E401Y_release_ccs/src/makefile.libs clean

