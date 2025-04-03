#!/bin/bash

sudo apt update && sudo apt upgrade -y
pip install Pillow exifread
python3 ./scorpion.py
