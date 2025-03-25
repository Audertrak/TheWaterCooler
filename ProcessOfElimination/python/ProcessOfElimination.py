import tkinter as tk
from tkinter import simpledialog, messagebox
import socket
from logic import PoE_Meeting
from gui import PoE_UI
from network import PoE_Server, PoE_Client

def get_local_ip():
    hostname = socket.gethostname()
    local_ip = socket.gethostbyname(hostname)
    return local_ip

def start_host():
    root = tk.Tk()
    meeting = PoE_Meeting()

    custom_word = tk.simpledialog.askstring(
            "Custom Word", "Enter a word (letters only):", parent=root
    )
    if not custom_word or not custom_word.isalpha():
        tk.messagebox.showerror("Error", "Invalid word: please try again")
        root.destroy()
        return

    meeting.pose_problem(custom_word)
    
    # Create UI first without network
    ui = PoE_UI(root, meeting, is_host=True)
    
    # Create server with UI reference
    server = PoE_Server(meeting, ui)
    success = server.start()
    
    if success:
        # Update UI with server reference
        ui.network = server
        
        # Start the meeting
        meeting.start_meeting()
        ui.update_display()
        
        # Show server IP address
        ip_address = get_local_ip()
        messagebox.showinfo("Server Information", 
                           f"Server started successfully!\n\nIP Address: {ip_address}\nPort: 5555\n\n"
                           f"Share this information with other players so they can connect.")
        
        root.protocol("WM_DELETE_WINDOW", lambda: on_close(root, server))
        root.mainloop()
    else:
        messagebox.showerror("Error", "Failed to start server.")
        root.destroy()

def start_client():
    server_ip = simpledialog.askstring("Connect to Server", "Enter server IP address:")
    if not server_ip:
        return
    
    root = tk.Tk()
    meeting = PoE_Meeting()
    
    # Create UI
    ui = PoE_UI(root, meeting, is_host=False)
    
    # Create client
    client = PoE_Client(meeting, ui, server_ip)
    success = client.connect()
    
    if success:
        # Update UI with client reference
        ui.network = client
        
        root.protocol("WM_DELETE_WINDOW", lambda: on_close(root, client))
        root.mainloop()
    else:
        messagebox.showerror("Error", "Failed to connect to server.")
        root.destroy()

def on_close(root, network):
    if network:
        network.stop()
    root.destroy()

def main():
    # Create selection window
    root = tk.Tk()
    root.title("PoE_ meeting")
    root.geometry("300x200")
    root.resizable(False, False)
    
    label = tk.Label(root, text="Choose an option:", font=("Arial", 14))
    label.pack(pady=20)
    
    host_button = tk.Button(root, text="Host meeting", width=20, command=lambda: [root.destroy(), start_host()])
    host_button.pack(pady=10)
    
    join_button = tk.Button(root, text="Join meeting", width=20, command=lambda: [root.destroy(), start_client()])
    join_button.pack(pady=10)
    
    root.mainloop()

if __name__ == "__main__":
    main()

