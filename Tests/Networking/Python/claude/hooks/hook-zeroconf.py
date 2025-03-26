
"""PyInstaller hook for zeroconf module."""

from PyInstaller.utils.hooks import collect_submodules, collect_all

# list zeroconf submodules for pyinstaller
hiddenimports = collect_submodules('zeroconf')

# collect all data files for zeroconf
datas, binaries, hiddenimports_2 = collect_all('zeroconf')

# merge hidden imports
hiddenimports.extend(hiddenimports_2)