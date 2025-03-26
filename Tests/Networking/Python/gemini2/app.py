# app.py
import tkinter as tk
from tkinter import scrolledtext, messagebox, simpledialog
import queue
import threading
import time

from utils import get_lan_ip
from network_handler import NetworkHandler

class P2PTesterApp:
    def __init__(self, root):
        self.root = root
        self.root.title("P2P Test Utility")
        self.root.geometry("700x550")

        self.local_ip = get_lan_ip()
        self.peer_name = f"Peer_{self.local_ip.split('.')[-1]}" # Default name

        # Queue for communication between network threads and GUI
        self.gui_queue = queue.Queue()

        # Network Handler instance
        self.network_handler = NetworkHandler(self.gui_queue, self.peer_name, self.local_ip)

        # --- GUI Elements ---
        # Top Frame (Info)
        info_frame = tk.Frame(root, pady=5)
        info_frame.pack(fill=tk.X)

        tk.Label(info_frame, text="Your IP:").pack(side=tk.LEFT, padx=5)
        self.ip_label = tk.Label(info_frame, text=self.local_ip, fg="blue")
        self.ip_label.pack(side=tk.LEFT, padx=5)

        tk.Label(info_frame, text="Peer Name:").pack(side=tk.LEFT, padx=(20, 5))
        self.name_entry = tk.Entry(info_frame, width=15)
        self.name_entry.insert(0, self.peer_name)
        self.name_entry.pack(side=tk.LEFT, padx=5)
        self.name_entry.bind("<FocusOut>", self.update_name)
        self.name_entry.bind("<Return>", self.update_name)

        # Middle Frame (Peers and Actions)
        mid_frame = tk.Frame(root, pady=5)
        mid_frame.pack(fill=tk.BOTH, expand=True)

        # Peer List
        peer_frame = tk.LabelFrame(mid_frame, text="Discovered Peers", padx=5, pady=5)
        peer_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5)

        self.peer_listbox = tk.Listbox(peer_frame, width=35)
        self.peer_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        peer_scrollbar = tk.Scrollbar(peer_frame, orient=tk.VERTICAL, command=self.peer_listbox.yview)
        peer_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.peer_listbox.config(yscrollcommand=peer_scrollbar.set)
        self.peer_listbox.bind("<<ListboxSelect>>", self.on_peer_select)

        # Actions Frame
        action_frame = tk.Frame(mid_frame, padx=5)
        action_frame.pack(side=tk.LEFT, fill=tk.Y)

        self.send_discovery_btn = tk.Button(action_frame, text="Send Discovery", command=self.send_discovery)
        self.send_discovery_btn.pack(pady=5, fill=tk.X)

        tk.Label(action_frame, text="Message:").pack(pady=(10, 0))
        self.message_entry = tk.Entry(action_frame, width=25)
        self.message_entry.pack(pady=5)
        self.message_entry.bind("<Return>", self.send_message) # Allow sending with Enter key

        self.send_msg_btn = tk.Button(action_frame, text="Send TCP Msg", command=self.send_message, state=tk.DISABLED)
        self.send_msg_btn.pack(pady=5, fill=tk.X)

        # Log Area
        log_frame = tk.LabelFrame(root, text="Log", padx=5, pady=5)
        log_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=(0, 5))

        self.log_area = scrolledtext.ScrolledText(log_frame, wrap=tk.WORD, height=10, state=tk.DISABLED)
        self.log_area.pack(fill=tk.BOTH, expand=True)

        # --- Initialization ---
        self.peers_data = {} # Store full peer data associated with listbox entries
        self.selected_peer_id = None
        self.start_networking()

        # Start processing the GUI queue
        self.root.after(100, self.process_gui_queue)

        # Set up close protocol
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)

    def log(self, message):
        """Appends a message to the log area."""
        self.log_area.config(state=tk.NORMAL)
        timestamp = time.strftime("%H:%M:%S", time.localtime())
        self.log_area.insert(tk.END, f"[{timestamp}] {message}\n")
        self.log_area.see(tk.END) # Auto-scroll
        self.log_area.config(state=tk.DISABLED)

    def update_name(self, event=None):
        """Updates the peer name when the entry loses focus or Enter is pressed."""
        new_name = self.name_entry.get().strip()
        if new_name and new_name != self.peer_name:
            self.peer_name = new_name
            self.network_handler.update_peer_name(new_name)
            self.log(f"Set peer name to: {self.peer_name}")
        else:
            # Revert to the current name if input is empty or unchanged
            self.name_entry.delete(0, tk.END)
            self.name_entry.insert(0, self.peer_name)

    def start_networking(self):
        """Starts the network listeners."""
        self.log("Initializing networking...")
        self.network_handler.start_listeners()

    def send_discovery(self):
        """Sends a manual discovery broadcast."""
        self.log("Sending manual discovery broadcast...")
        self.network_handler.send_discovery_broadcast()

    def on_peer_select(self, event=None):
        """Handles selection changes in the peer listbox."""
        selected_indices = self.peer_listbox.curselection()
        if selected_indices:
            index = selected_indices[0]
            listbox_text = self.peer_listbox.get(index)
            # Extract peer_id from the listbox text (assuming format "Name (IP:Port)")
            try:
                self.selected_peer_id = listbox_text.split('(')[1].split(')')[0]
                self.send_msg_btn.config(state=tk.NORMAL)
                self.log(f"Selected peer: {self.selected_peer_id}")
            except IndexError:
                self.log(f"Error parsing selected peer: {listbox_text}")
                self.selected_peer_id = None
                self.send_msg_btn.config(state=tk.DISABLED)
        else:
            self.selected_peer_id = None
            self.send_msg_btn.config(state=tk.DISABLED)

    def send_message(self, event=None):
        """Sends a TCP message to the selected peer."""
        message = self.message_entry.get().strip()
        if not message:
            self.log("Cannot send empty message.")
            return
        if not self.selected_peer_id:
            self.log("No peer selected to send message.")
            return

        self.log(f"Attempting to send TCP to {self.selected_peer_id}: {message}")
        self.network_handler.send_tcp_message(self.selected_peer_id, message)
        self.message_entry.delete(0, tk.END) # Clear message entry after sending

    def update_peer_list(self, peers_dict):
        """Updates the listbox with discovered peers."""
        self.peers_data = peers_dict
        self.peer_listbox.delete(0, tk.END) # Clear existing list

        # Remember selection
        previously_selected_id = self.selected_peer_id

        sorted_peers = sorted(peers_dict.items(), key=lambda item: item[1]['name'])

        new_selection_index = -1
        for i, (peer_id, data) in enumerate(sorted_peers):
            display_text = f"{data['name']} ({peer_id})"
            self.peer_listbox.insert(tk.END, display_text)
            if peer_id == previously_selected_id:
                new_selection_index = i

        # Re-select if the previously selected peer is still present
        if new_selection_index != -1:
            self.peer_listbox.selection_set(new_selection_index)
            self.peer_listbox.activate(new_selection_index)
            self.send_msg_btn.config(state=tk.NORMAL)
        else:
            # Clear selection if the old peer is gone
            self.selected_peer_id = None
            self.send_msg_btn.config(state=tk.DISABLED)

        # Refresh selection state just in case
        self.on_peer_select()


    def process_gui_queue(self):
        """Processes messages from the network handler queue."""
        try:
            while True: # Process all messages currently in the queue
                message_type, data = self.gui_queue.get_nowait()

                if message_type == "log":
                    self.log(data)
                elif message_type == "peers":
                    self.update_peer_list(data)
                # Add more message types here if needed

        except queue.Empty:
            # Queue is empty, schedule next check
            pass
        finally:
            # Always reschedule the check
            self.root.after(100, self.process_gui_queue)

    def on_closing(self):
        """Handles the window close event."""
        self.log("Close requested. Shutting down networking...")
        self.network_handler.stop_listeners()
        # Give a brief moment for threads to potentially stop before destroying window
        self.root.after(200, self.root.destroy)
        # self.root.destroy() # Destroy immediately if preferred


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
    root = tk.Tk()
    app = P2PTesterApp(root)
    root.mainloop()
