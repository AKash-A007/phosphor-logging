#!/bin/bash
# AEL Verification Script - Simple D-Bus introspection

echo "--- AEL Extension D-Bus Introspection ---"

# Find all log entries with the OEM interface
ENTRIES=$(busctl tree xyz.openbmc_project.Logging --list | grep "xyz.openbmc_project.Logging.Entry.Oem")

if [ -z "$ENTRIES" ]; then
    echo "No AEL-decorated log entries found."
    exit 0
fi

for ENTRY in $ENTRIES; do
    echo "Entry: $ENTRY"
    busctl get-property xyz.openbmc_project.Logging "$ENTRY" \
        xyz.openbmc_project.Logging.Entry.Oem OemAdditionalData
    echo "----------------------------------------"
done
