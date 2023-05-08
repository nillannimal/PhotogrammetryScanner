import subprocess
import PySimpleGUI as sg
import sys
import serial


def main():
    wait = True 
    sg.theme('DarkBlue3')

    layout = [[sg.Text('Object Name:'), sg.Input(key='-NAME-')],
            [sg.Text('COM Port:'), sg.Input(key='-COMPORT-')],
            [sg.Text('Phone Path:'), sg.Input(key='-INPUTFILEPATH-')],
            [sg.Button('Run'), sg.Button('Cancel')],
            [sg.Multiline(size=(80, 14), echo_stdout_stderr=True, reroute_stdout=True, autoscroll=True, background_color='white', text_color='black', key='-MLINE-')],]

    window = sg.Window('Photogrammetry Scanner Application', layout)

    while True:
        event, values = window.read()
        if event in (sg.WIN_CLOSED, 'Cancel'):
            break
        if event == 'Run':
            if (wait):
                arduino = serial.Serial(port = values['-COMPORT-'], baudrate= 115200, timeout=.1)
                print("Waiting for Scan to Finish...")
                while(True):
                    data = arduino.readline()[:-2] #the last bit gets rid of the new-line chars
                    window.Refresh()
                    if data.decode() == "Done Scanning":
                        wait = False
                        window.Refresh()
                        print("Scan Done. Transferring and Processing Images...")
                        break
                name = values['-NAME-']
                comport = values['-COMPORT-']
                input_filepath = values['-INPUTFILEPATH-']
                cmd = f"python process_images.py -c {comport} -n {name}"
                cmd_list = cmd.split(' ')
                sp = sg.execute_command_subprocess(cmd_list[0], *cmd_list[1:], pipe_output=True, wait=False)
                results = sg.execute_get_results(sp, timeout=None)
                if results[0] != "None":
                    print(results[0])
                window.Refresh()
        
    window.Close()

if __name__ == "__main__":
    main()