#!/usr/bin/env python
"""
A simple peer-to-peer testing app using Tkinter and standard library networking.
This application starts a simple TCP server in a background thread and provides
a GUI to send messages to another peer. Each peer logs incoming messages and 
other status messages in a text window.
"""

import socket
import threading
import tkinter as tk
from tkinter import ttk
import queue


class NetworkHandler:
    """
    This class handles starting a TCP server and sending messages as a client.
    It runs its server loop on a background thread so that the GUI remains responsive.
    """

    def __init__(self, log_callback, port=5000):
        """
        Initialize the network handler.

        Args:
            log_callback: A function that accepts a string message for logging.
            port (int): The TCP port to listen on for incoming peer connections.
        """
        self.log_callback = log_callback
        self.port = port
        self.running = False
        self.server_thread = None
        self.server_socket = None

    def start_server(self):
        """Start the TCP server on a new background thread."""
        if not self.running:
            self.running = True
            self.server_thread = threading.Thread(
                target=self._server_loop, daemon=True
            )
            self.server_thread.start()
        else:
            self.log_callback("Server is already running.")

    def _server_loop(self):
        """The server loop that listens for incoming TCP connections."""
        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            # Allow immediate reuse of the port
            self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_socket.bind(("", self.port))
            self.server_socket.listen(5)
            self.log_callback(f"Server started on port {self.port}")
        except Exception as e:
            self.log_callback(f"Failed to start server: {e}")
            self.running = False
            return

        while self.running:
            try:
                conn, addr = self.server_socket.accept()
                self.log_callback(f"Connection established from {addr}")
                # Start a dedicated thread to handle communication with the client.
                client_thread = threading.Thread(
                    target=self._handle_client, args=(conn, addr), daemon=True
                )
                client_thread.start()
            except Exception as e:
                if self.running:
                    self.log_callback(f"Server error: {e}")

        # Clean up the server socket when stopping.
        if self.server_socket:
            self.server_socket.close()

    def _handle_client(self, conn, addr):
        """
        Handle an individual client's connection.

        Args:
            conn: The connection socket.
            addr: The client's address.
        """
        while self.running:
            try:
                # Receive up to 1024 bytes. Adjust the buffer size as needed.
                data = conn.recv(1024)
                if not data:
                    break
                message = data.decode("utf-8")
                self.log_callback(f"Received from {addr}: {message}")
            except Exception as e:
                self.log_callback(f"Error with client {addr}: {e}")
                break
        self.log_callback(f"Connection closed with {addr}")
        conn.close()

    def send_message(self, target_ip, target_port, message):
        """
        Send a message as a client to a target peer.

        Args:
            target_ip (str): The IP address of the target peer.
            target_port (int): The TCP port of the target peer.
            message (str): The message to send.
        """
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((target_ip, target_port))
            s.sendall(message.encode("utf-8"))
            s.close()
            self.log_callback(
                f"Sent message to {(target_ip, target_port)}: {message}"
            )
        except Exception as e:
            self.log_callback(f"Error sending message: {e}")

    def stop(self):
        """Stop the server."""
        self.running = False
        if self.server_socket:
            try:
                self.server_socket.close()
            except Exception as e:
                self.log_callback(f"Error closing server socket: {e}")


class P2PTestApp(tk.Tk):
    """
    The main Tkinter application for the peer-to-peer test utility.
    """

    def __init__(self):
        super().__init__()
        self.title("P2P Test Utility")
        self.geometry("600x400")
        self.protocol("WM_DELETE_WINDOW", self.on_close)

        # A thread-safe queue to gather log messages from background threads.
        self.log_queue = queue.Queue()
        # Create the network handler with logging callback.
        self.network_handler = NetworkHandler(self.queue_log, port=5000)

        self.create_widgets()
        # Start the TCP server immediately.
        self.network_handler.start_server()
        # Begin polling the log queue to update the GUI.
        self.poll_log_queue()

    def create_widgets(self):
        """Create and layout the GUI components."""
        main_frame = ttk.Frame(self)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)

        # Protocol selection (stub for future expansion: currently only TCP is enabled)
        protocol_frame = ttk.LabelFrame(main_frame, text="Protocol Settings")
        protocol_frame.pack(fill=tk.X, padx=5, pady=5)

        ttk.Label(protocol_frame, text="Protocol:").grid(
            row=0, column=0, sticky=tk.W, padx=5, pady=5
        )
        self.protocol_var = tk.StringVar(value="TCP")
        protocol_options = ("TCP", "UDP", "HTTP", "mDNS", "WebSockets")  # etc.
        protocol_menu = ttk.OptionMenu(
            protocol_frame, self.protocol_var, self.protocol_var.get(), *protocol_options
        )
        protocol_menu.grid(row=0, column=1, sticky=tk.W, padx=5, pady=5)

        # Peer connection settings
        connection_frame = ttk.LabelFrame(main_frame, text="Peer Connection")
        connection_frame.pack(fill=tk.X, padx=5, pady=5)

        ttk.Label(connection_frame, text="Target IP:").grid(
            row=0, column=0, sticky=tk.W, padx=5, pady=5
        )
        self.target_ip_entry = ttk.Entry(connection_frame)
        self.target_ip_entry.insert(0, "127.0.0.1")
        self.target_ip_entry.grid(row=0, column=1, sticky=tk.W, padx=5, pady=5)

        ttk.Label(connection_frame, text="Target Port:").grid(
            row=1, column=0, sticky=tk.W, padx=5, pady=5
        )
        self.target_port_entry = ttk.Entry(connection_frame)
        self.target_port_entry.insert(0, "5000")
        self.target_port_entry.grid(row=1, column=1, sticky=tk.W, padx=5, pady=5)

        ttk.Label(connection_frame, text="Message:").grid(
            row=2, column=0, sticky=tk.W, padx=5, pady=5
        )
        self.message_entry = ttk.Entry(connection_frame, width=40)
        self.message_entry.grid(row=2, column=1, sticky=tk.W, padx=5, pady=5)

        send_button = ttk.Button(
            connection_frame, text="Send Message", command=self.send_message
        )
        send_button.grid(row=3, column=0, columnspan=2, pady=5)

        # Log display area
        log_frame = ttk.LabelFrame(main_frame, text="Log / Debug")
        log_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        self.log_text = tk.Text(log_frame, height=10, state=tk.DISABLED)
        self.log_text.pack(fill=tk.BOTH, expand=True)

    def queue_log(self, message: str):
        """
        Place a log message into the thread-safe queue.
        This method can be safely called from any thread.
        """
        self.log_queue.put(message)

    def poll_log_queue(self):
        """Poll the log queue and update the log text widget periodically."""
        try:
            while True:
                message = self.log_queue.get_nowait()
                self.append_log(message)
        except queue.Empty:
            pass
        # Check again after 100 milliseconds.
        self.after(100, self.poll_log_queue)

    def append_log(self, message: str):
        """Append a log message to the log text area."""
        self.log_text.configure(state=tk.NORMAL)
        self.log_text.insert(tk.END, message + "\n")
        self.log_text.configure(state=tk.DISABLED)
        # Automatically scroll to the bottom.
        self.log_text.see(tk.END)

    def send_message(self):
        """
        Read the target IP, port, message, and protocol (if applicable) from the UI 
        and call the network handler to send the message.
        """
        target_ip = self.target_ip_entry.get().strip()
        try:
            target_port = int(self.target_port_entry.get().strip())
        except ValueError:
            self.append_log("Invalid port number.")
            return
        message = self.message_entry.get().strip()

        protocol = self.protocol_var.get()
        if protocol != "TCP":
            self.append_log(f"Protocol {protocol} not implemented "
                            f"(only TCP is supported for now).")
            return

        # Call the network handler's send_message method.
        threading.Thread(
            target=self.network_handler.send_message,
            args=(target_ip, target_port, message),
            daemon=True,
        ).start()

    def on_close(self):
        """Clean-up on closing the application."""
        self.network_handler.stop()
        self.destroy()


if __name__ == "__main__":
    app = P2PTestApp()
    app.mainloop()
