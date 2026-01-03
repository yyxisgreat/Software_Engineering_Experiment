# Use an official Python runtime as a parent image
FROM python:3.11-slim

# Set working directory inside the container
WORKDIR /app

# Copy Python implementation into container
COPY python/ /app/

# Install any dependencies (tkinter is part of python3-full; may need additional libs on some systems)
# For simplicity, we rely on the base image providing tkinter. If missing, install python3-tk at build time.
# RUN apt-get update && apt-get install -y python3-tk && rm -rf /var/lib/apt/lists/*

# By default run the GUI application. When running headless or testing, override CMD to run run_tests.py
CMD ["python3", "netdisk_gui.py"]