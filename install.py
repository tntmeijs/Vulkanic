import os
import subprocess
import time
from colorama import init
from colorama import Fore
from git import Repo
from progress.bar import Bar
from progress.spinner import Spinner

clear_console_screen = lambda: os.system('cls' if os.name=='nt' else 'clear')
initialize_colorama = lambda: init()
print_spacing = lambda: print('\n\n')

def print_title(text):
    print(Fore.CYAN + '=== ' + ('=' * len(text)) + ' ===')
    print('=== ', end='')
    print(Fore.YELLOW + text, end='')
    print(Fore.CYAN + ' ===')
    print('=== ' + ('=' * len(text)) + ' ===' + Fore.RESET)

def list_project_info():
    print('Author:\t\tTahar Meijs')
    print('Licence:\tNone - please contact me before using this codebase')
    print('\nThird-party libraries & APIs:\t')
    print('\t> Vulkan')
    print('\t> GLFW')
    print('\t> GLM')
    print('\t> Assimp')

    print_spacing();
    input('Press \"Enter\" to continue...')

def update_submodules():
    repository = Repo('./')

    with Bar('Progress:', max=len(repository.submodules)) as bar:
        for submodule in repository.submodules:
            submodule.update(recursive=True, init=True)
            bar.next()

    print_spacing()
    print(Fore.GREEN + 'Finished updating submodules.' + Fore.RESET)
    print_spacing()
    input('Press \"Enter\" to continue...')

def run_cmake():
    print_title('Starting CMake')
    print_spacing()

    time.sleep(1)
    print('\nThe following projects will be generated:')
    time.sleep(0.5)
    print('\t> Visual Studio 15 2017\t-->\t' + Fore.CYAN + 'Win64' + Fore.RESET)
    time.sleep(0.5)
    print('\t> Visual Studio 16 2019\t-->\t' + Fore.CYAN + 'Win64' + Fore.RESET)
    time.sleep(1)

    print_spacing()
    result = generate_vs_17_win64()
    print_spacing()
    result = generate_vs_19_win64()
    print_spacing()

    if result != 0:
        print(Fore.GREEN + 'Project files have been generated successfully.' + Fore.RESET)
    else:
        print(Fore.RED + 'CMake could not generate all project files properly.' + Fore.RESET)

def generate_vs_17_win64():
    print_title('Visual Studio 15 2017 - Win64')
    return subprocess.run('cmake -B./build_vs_15_2017_win64 -H./ -G \"Visual Studio 15 2017\" -A x64')

def generate_vs_19_win64():
    print_title('Visual Studio 16 2019 - Win64')
    return subprocess.run('cmake -B./build_vs_16_2019_win64 -H./ -G \"Visual Studio 16 2019\" -A x64')

# Application entry point
def main():
    clear_console_screen()
    initialize_colorama()

    print_title('Vulkanic Installer')
    list_project_info()
    clear_console_screen();

    print_title('Updating Submodules')
    update_submodules()
    clear_console_screen();

    run_cmake()

    print_spacing()
    input('Press \"Enter\" to continue...')

if __name__ == '__main__':
    main()
