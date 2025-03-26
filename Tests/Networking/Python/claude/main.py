import tkinter as tk
from gui import P2PTesterGUI

def main():
    root = tk.Tk()
    app = P2PTesterGUI(root)
    root.mainloop()

if __name__ == "__main__":
    main()
