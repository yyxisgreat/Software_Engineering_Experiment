#!/usr/bin/env bash
# Build the Docker image for the backup/restore tool.

set -e

# Build the Docker image
docker build -t backup_restore_gui .

echo "Docker image 'backup_restore_gui' built successfully."
echo "You can run the GUI via: docker run -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix backup_restore_gui"
echo "To run tests inside the container: docker run --rm backup_restore_gui python3 run_tests.py"