#!/bin/sh

cp $BASE_DIR/../custom-scripts/S41network-config $BASE_DIR/target/etc/init.d
chmod +x $BASE_DIR/target/etc/init.d/S41network-config

# Copy HTTP server files
cp $BASE_DIR/../custom-scripts/server/* $BASE_DIR/target/root
