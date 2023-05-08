from win32com.shell import shell, shellcon
from win32com.propsys import propsys
from datetime import datetime,timedelta
import pythoncom
import os
import time
import serial
import argparse
import subprocess
import shutil
import time

# Adapted from code by Lassi Niemistö
# Source: https://gitlab.com/lassi.niemisto/iphone-photo-dump

DATE_PROP_KEY = propsys.PSGetPropertyKeyFromName("System.DateModified")
DATE_ARG_PARSE_STR = '%Y-%m-%d'
DATE_PROP_PARSE_STR = '%Y/%m/%d:%H:%M:%S.%f'
MAX_NUM_FILES = 96

def recurse_and_get_ishellfolder(base_ishellfolder, path):
    splitted_path = path.split("\\", 1)

    for pidl in base_ishellfolder:
        if base_ishellfolder.GetDisplayNameOf(pidl, shellcon.SHGDN_NORMAL) == splitted_path[0]:
            break

    folder = base_ishellfolder.BindToObject(pidl, None, shell.IID_IShellFolder)

    if len(splitted_path) > 1:
        # More to recurse
        return recurse_and_get_ishellfolder(folder, splitted_path[1])
    else:
        return folder


def move_files(args,newer_than_datetime):
    main_folder = recurse_and_get_ishellfolder(shell.SHGetDesktopFolder(), args.input)
    files_to_transfer = []
    for photo_folder_pidl in main_folder:
        folder_name = main_folder.GetDisplayNameOf(photo_folder_pidl, shellcon.SHGDN_NORMAL)
        folder = main_folder.BindToObject(photo_folder_pidl, None, shell.IID_IShellFolder)
        
        for pidl in folder:
            child_name = folder.GetDisplayNameOf(pidl, shellcon.SHGDN_NORMAL)
            ext_lower = os.path.splitext(child_name)[1].lower()

            file_mod_date = getmodified_datetime_by_pidl(folder, pidl)
            if file_mod_date > newer_than_datetime:
                 files_to_transfer.append((pidl, child_name,file_mod_date,folder,folder_name)) 
           
        
    files_to_transfer = sorted(files_to_transfer, key=lambda x: x[2], reverse=True)[:MAX_NUM_FILES]

    for pidl, child_name,file_mod_date,folder,folder_name in files_to_transfer:
        print("Transferring file: " + child_name)
        move_file_by_pidl(args.output, folder, pidl, child_name, folder_name + "_")


def move_file_by_pidl(dest_dir, src_ishellfolder, src_pidl, src_filename, name_prefix):
    filename = name_prefix + src_filename + ".jpg"  # Avoid conflicts
    dest_fullpath = dest_dir + os.sep + filename
    tries = 2
    i = 1
    while True:
        res = move_file_by_pidl_to_path(src_ishellfolder, src_pidl, dest_dir, filename)
        if res:
            if not os.path.isfile(dest_fullpath):
                print(" -> Move operation returned ok but file did not appear")
            break
        if i < tries:
            i += 1
            time.sleep(3)
        else:
            print(" -> Failed to transfer " + src_filename)
            break


def getmodified_datetime_by_pidl(src_ishellfolder, src_pidl):
    fidl = shell.SHGetIDListFromObject(src_ishellfolder)  # Grab the PIDL from the folder object
    si = shell.SHCreateShellItem(fidl, None, src_pidl)  # Create a ShellItem of the source file
    ps = propsys.PSGetItemPropertyHandler(si)
    date_str = ps.GetValue(DATE_PROP_KEY).ToString()
    return datetime.strptime(date_str, DATE_PROP_PARSE_STR)


def move_file_by_pidl_to_path(src_ishellfolder, src_pidl, dst_path, dst_filename):
    pidl_folder_dst, flags = shell.SHILCreateFromPath(dst_path, 0)
    dst_ishellfolder = shell.SHGetDesktopFolder().BindToObject(pidl_folder_dst, None, shell.IID_IShellFolder)

    fidl = shell.SHGetIDListFromObject(src_ishellfolder)  # Grab the PIDL from the folder object
    didl = shell.SHGetIDListFromObject(dst_ishellfolder)  # Grab the PIDL from the folder object

    si = shell.SHCreateShellItem(fidl, None, src_pidl)  # Create a ShellItem of the source file
    dst = shell.SHCreateItemFromIDList(didl)

    pfo = pythoncom.CoCreateInstance(shell.CLSID_FileOperation, None, pythoncom.CLSCTX_ALL, shell.IID_IFileOperation)
    pfo.SetOperationFlags(shellcon.FOF_NOCONFIRMATION | shellcon.FOF_SILENT | shellcon.FOF_NOERRORUI)
    pfo.MoveItem(si, dst, dst_filename) # Schedule an operation to be performed
    pfo.PerformOperations()
    return not pfo.GetAnyOperationsAborted()

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--input", help="input path, e.g. \"This PC\Apple iPhone\Internal Storage\DCIM\"", required=False)
    parser.add_argument("-o", "--output", help="output directory, must exist", required= False)
    parser.add_argument("-c", "--COM", help="COM PORT, must exist", required=True)
    parser.add_argument("-n", "--name", help="Object Name, must exist", required=True)

    args = parser.parse_args()
    if not args.input:
        args.input = "This PC\Apple iPhone\Internal Storage\DCIM"
    
    if not os.path.exists(args.name):
        os.mkdir(args.name)

    # Get the directory of the folder
    folder_dir = os.path.abspath(args.name)
    args.output = folder_dir

    # Get today's date
    today = datetime.now().date()

    # Calculate yesterday's date
    yesterday = today - timedelta(days=3)

    # Format yesterday's date as string in "YYYY-MM-DD" format
    yesterday_str = yesterday.strftime("%Y-%m-%d")
    print("Only moving files newer than " + yesterday_str)
    newer_than_datetime = datetime.strptime(yesterday_str, DATE_ARG_PARSE_STR)
    move_files(args,newer_than_datetime)

    #Create Folder for InstantNerf
    # Define the path to the folder containing the images
    folder_path = folder_dir

    # Define the path to the new folder where the selected images will be moved
    new_folder_path = os.path.join(folder_path, "Nerf_Images")

    # Create the new folder if it doesn't exist
    if not os.path.exists(new_folder_path):
        os.mkdir(new_folder_path)

    # Get a list of all the files in the folder
    file_list = os.listdir(folder_path)

    # Filter the list to only include image files
    image_list = [f for f in file_list if f.lower().endswith(".jpg") or f.lower().endswith(".jpeg") or f.lower().endswith(".png")]

    # Sort the images by date modified
    image_list.sort(key=lambda x: os.path.getmtime(os.path.join(folder_path, x)))

    # Select the images from index 50 to 70
    selected_images = image_list[50:71]

    # Move the selected images to the new folder
    for image in selected_images:
        src_path = os.path.join(folder_path, image)
        dst_path = os.path.join(new_folder_path, image)
        shutil.copy2(src_path, dst_path)

    print()
    print("Running Meshroom...")

    # Call Meshroom
    input_file = args.name
    output_file =  args.name + "_result"
    command = f"meshroom_batch --input {input_file} --output {output_file}"
    subprocess.run(command, shell=True)


