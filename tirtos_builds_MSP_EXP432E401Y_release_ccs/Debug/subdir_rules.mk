################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
build-90018911:
	@$(MAKE) --no-print-directory -Onone -f subdir_rules.mk build-90018911-inproc

build-90018911-inproc: ../release.cfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: XDCtools'
	"C:/ti/ccs1280/xdctools_3_62_01_16_core/xs" --xdcpath="C:/ti/simplelink_msp432e4_sdk_4_20_00_12/source;C:/ti/simplelink_msp432e4_sdk_4_20_00_12/kernel/tirtos/packages;" xdc.tools.configuro -o configPkg -t ti.targets.arm.elf.M4F -p ti.platforms.msp432:MSP432E401Y -r release -c "C:/ti/ccs1280/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

configPkg/linker.cmd: build-90018911 ../release.cfg
configPkg/compiler.opt: build-90018911
configPkg: build-90018911


