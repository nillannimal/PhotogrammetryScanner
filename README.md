# Smartphone-Based Photogrammetry 3D Scanner

This project provides a low-cost 3D scanner using a smartphone and photogrammetry. The 3D scanner automatically takes pictures of an object, which are then processed by Meshroom to produce a 3D scan.

## Dependencies

- Python 3.x
- PySimpleGUI
- Meshroom
- win32com (pywin32)
- pythoncom (pywin32)
- pyserial

## Installing Meshroom

1. Download the latest version of Meshroom from [the official website](https://alicevision.org/#meshroom).
2. Extract the downloaded archive to your desired location.

## Setup

1. Clone this repository to your local machine.
2. Move the `photogrammetry_scanner.py` and `process_images.py` files to the Meshroom installation folder.

## Usage

1. Connect your smartphone and Arduino to your computer.
2. Run the GUI using the following command:

'''
python photogrammetry_scanner.py
'''

3. Enter the following information in the GUI:

- **COM Port**: The COM port of your Arduino. (e.g., COM3)
- **Object Name**: The name of the object you want to scan.
- **Phone Path**: The path to your phone's internal storage where the photos will be stored. (e.g., "This PC\Apple iPhone\Internal Storage\DCIM")

4. Click the "Run" button to start the scanning process. The scanner will automatically take pictures of the object and process them using Meshroom.
