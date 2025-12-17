#!/bin/bash

shopt -s extglob  # enable extended globs for selective cleanup
BUILD_DIR=$1
rm -rf $BUILD_DIR/!(*.bin|*.elf|*.map|bootloader|config|partition_table|customized_partitions|flasher_args.json|download.config|factory)
if [ -d $BUILD_DIR/bootloader ]; then rm -rf $BUILD_DIR/bootloader/!(*.bin); fi
if [ -d $BUILD_DIR/partition_table ]; then rm -rf $BUILD_DIR/partition_table/!(*.bin); fi
shopt -u extglob  # disable extended globs
