import sys
import os
from PIL import Image
from PIL.ExifTags import TAGS, GPSTAGS
import exifread


# Convert GPS coordinates from degrees, minutes, seconds (DMS) to decimal format
def convert_gps_to_decimal(coord, ref):
    if not coord or len(coord) != 3:
        return None

    degrees = float(coord[0])
    minutes = float(coord[1]) / 60.0
    seconds = float(coord[2]) / 3600.0
    decimal = degrees + minutes + seconds

    # South and West coordinates should be negative
    if ref in ['S', 'W']:
        decimal *= -1

    return decimal

# Extracts EXIF metadata from a JPEG image, including GPS data
def extract_exif_metadata(file_path):
    image = Image.open(file_path)
    exif_data = image._getexif()
    
    if not exif_data:
        print("No EXIF metadata found.")
        return

    gps_info = {}
    
    for tag_id, data in exif_data.items():
        tag_name = TAGS.get(tag_id, tag_id)
        if tag_name == "GPSInfo":
            for gps_tag_id, gps_data in data.items():
                gps_tag_name = GPSTAGS.get(gps_tag_id, gps_tag_id)
                gps_info[gps_tag_name] = gps_data
        else:
            print(f"{tag_name:25}: {data}")

    # Extract GPS coordinates if available
    if "GPSLatitude" in gps_info and "GPSLongitude" in gps_info:
        lat = convert_gps_to_decimal(gps_info["GPSLatitude"], gps_info.get("GPSLatitudeRef", "N"))
        lon = convert_gps_to_decimal(gps_info["GPSLongitude"], gps_info.get("GPSLongitudeRef", "E"))
        print(f"GPS Coordinates: {lat}, {lon}")

# Extracts basic metadata using Pillow for PNG, BMP, and GIF
def extract_pil_metadata(file_path):
    with Image.open(file_path) as img:
        print(f"Format: {img.format}")
        print(f"Size: {img.size}")
        print(f"Mode: {img.mode}")
        print("Metadata:")
        for key, value in img.info.items():
            print(f"  {key}: {value}")

        if hasattr(img, "is_animated"):
            print("Image is Animated:", img.is_animated)
            print("Frames in Image:", img.n_frames)


def process_image(file_path):
    """Determines the file format and extracts metadata accordingly."""
    ext = os.path.splitext(file_path)[-1].lower()
    
    print("###########################################")
    print(f"{file_path}")
    print("###########################################\n")
    
    if ext in ['.jpg', '.jpeg']:
        extract_exif_metadata(file_path)
    elif ext in ['.png', '.bmp', '.gif']:
        extract_pil_metadata(file_path)
    else:
        print("Unsupported file format.")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 ./scorpion.py <file1> <file2> ...")
    else:
        for file_path in sys.argv[1:]:
            if os.path.exists(file_path):
                process_image(file_path)
            else:
                print("###########################################")
                print(f"File not found: {file_path}")
                print("###########################################")
            print(f"\n")
