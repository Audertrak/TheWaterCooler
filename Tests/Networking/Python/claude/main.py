import zeroconf
import ifaddr
import typing_extensions
import async_timeout
import tkinter as tk
from gui import P2PTesterGUI
from peer import PeerManager

def main():
    root = tk.Tk()

    peer_manager = PeerManager()

    app = P2PTesterGUI(root, peer_manager)

    root.mainloop()

if __name__ == "__main__":
    main()
