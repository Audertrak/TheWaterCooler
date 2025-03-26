import tkinter as tk
from tkinter import ttk, scrolledtext

import socket
import uuid
import threading
import time
import json

from pathlib import Path
from typing import Dict, List

from peer import *
from identity import PeerIdentity, IdentityDialog

from protocols.base import ProtocolBase
from protocols.udp import UDPProtocol
from protocols.tcp import TCPProtocol
from protocols.mdns import MDNSProtocol
from protocols.winapi import WindowsProtocol

from message.base import MessageBase
from message.raw import RawMessage
from message.json import JSONMessage
from message.protobuf import SimpleProtobufMessage
from message.mqtt import MQTTMessage

class P2PTesterGUI:
    def __init__(self, root, peer_manager=None):
        self.root = root
        self.root.title("P2P Network Tester")
        self.root.geometry("1080x800")
        
        self.peer_id = f"peer-{uuid.uuid4().hex[:8]}"
        self.identity = PeerIdentity(display_name=socket.gethostname())
        self.peer_id = self.identity.peer_id
        
        # Create PeerManager instance
        self.peer_manager = peer_manager or PeerManager()
        
        self.protocols: Dict[str, ProtocolBase] = {}
        self.active_protocols = set()
        
        # Initialize message formats
        self.message_formats = {
            "raw": RawMessage(),
            "json": JSONMessage(),
            "protobuf": SimpleProtobufMessage(),
            "mqtt": MQTTMessage()
        }
        self.current_message_format = "json"
        
        self._create_widgets()
        self._init_protocols()

    def _create_widgets(self):
        """Create GUI widgets"""
        # Top frame for peer ID and controls
        top_frame = ttk.Frame(self.root, padding=10)
        top_frame.pack(fill=tk.X)
        
        ttk.Label(top_frame, text="Peer ID:").pack(side=tk.LEFT, padx=5)
        
        self.peer_id_var = tk.StringVar(value=self.peer_id)
        peer_id_entry = ttk.Entry(top_frame, textvariable=self.peer_id_var, width=30)
        peer_id_entry.pack(side=tk.LEFT, padx=5)
        
        self.update_id_btn = ttk.Button(top_frame, text="Update ID", 
                                       command=self._update_peer_id)
        self.update_id_btn.pack(side=tk.LEFT, padx=5)
        
        # Frame for protocol controls
        protocol_frame = ttk.LabelFrame(self.root, text="Protocols", padding=10)
        protocol_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        
        # Create a notebook for protocol tabs
        self.protocol_notebook = ttk.Notebook(protocol_frame)
        self.protocol_notebook.pack(fill=tk.BOTH, expand=True)
        
        # UDP tab
        udp_tab = ttk.Frame(self.protocol_notebook)
        self.protocol_notebook.add(udp_tab, text="UDP")
        self._create_protocol_tab(udp_tab, "udp")
        
        # TCP tab
        tcp_tab = ttk.Frame(self.protocol_notebook)
        self.protocol_notebook.add(tcp_tab, text="TCP")
        self._create_protocol_tab(tcp_tab, "tcp")
        
        # mDNS tab
        mdns_tab = ttk.Frame(self.protocol_notebook)
        self.protocol_notebook.add(mdns_tab, text="mDNS")
        self._create_protocol_tab(mdns_tab, "mdns")

        # Windows tab
        windows_tab = ttk.Frame(self.protocol_notebook)
        self.protocol_notebook.add(windows_tab, text="Windows")
        self._create_protocol_tab(windows_tab, "windows")
        
        # Bottom frame for peers and messaging
        bottom_frame = ttk.Frame(self.root, padding=10)
        bottom_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        
        # Split into left (peers) and right (messaging) panels
        left_panel = ttk.Frame(bottom_frame)
        left_panel.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        right_panel = ttk.Frame(bottom_frame)
        right_panel.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)
        
        # Peers list
        peers_frame = ttk.LabelFrame(left_panel, text="Discovered Peers", padding=10)
        peers_frame.pack(fill=tk.BOTH, expand=True)
        
        peers_scroll = ttk.Scrollbar(peers_frame)
        peers_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.peers_list = tk.Listbox(peers_frame, yscrollcommand=peers_scroll.set)
        self.peers_list.pack(fill=tk.BOTH, expand=True)
        self.peers_list.bind('<<ListboxSelect>>', self._on_peer_selected)
        
        peers_scroll.config(command=self.peers_list.yview)
        
        # Messaging panel
        messaging_frame = ttk.LabelFrame(right_panel, text="Messaging", padding=10)
        messaging_frame.pack(fill=tk.BOTH, expand=True)
        
        # Message history
        history_frame = ttk.Frame(messaging_frame)
        history_frame.pack(fill=tk.BOTH, expand=True)
        
        self.message_history = scrolledtext.ScrolledText(history_frame, 
                                                        state='disabled',
                                                        height=15)
        self.message_history.pack(fill=tk.BOTH, expand=True)
        
        # Message sending
        send_frame = ttk.Frame(messaging_frame)
        send_frame.pack(fill=tk.X, pady=10)
        
        self.message_var = tk.StringVar()
        message_entry = ttk.Entry(send_frame, textvariable=self.message_var)
        message_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)
        
        # Protocol selection for sending
        self.send_protocol_var = tk.StringVar(value="any")
        protocol_combo = ttk.Combobox(send_frame, textvariable=self.send_protocol_var,
                                      values=["any", "udp", "tcp", "mdns", "windows"],
                                      width=10)
        protocol_combo.pack(side=tk.LEFT, padx=5)
        
        # message format selection
        self.message_format_var = tk.StringVar(value="json")
        format_label = ttk.Label(send_frame, text="Format:")
        format_label.pack(side=tk.LEFT)
        format_combo = ttk.Combobox(send_frame, textvariable=self.message_format_var,
                                    values=list(self.message_formats.keys()),
                                    width=10)
        format_combo.pack(side=tk.LEFT, padx=5)
        format_combo.bind("<<ComboboxSelected>>", lambda e: self._change_message_format())

        self.send_btn = ttk.Button(send_frame, text="Send", 
                                  command=self._send_message,
                                  state=tk.DISABLED)
        self.send_btn.pack(side=tk.LEFT, padx=5)
        
        self.broadcast_btn = ttk.Button(send_frame, text="Broadcast", 
                                       command=self._broadcast_message)
        self.broadcast_btn.pack(side=tk.LEFT, padx=5)
        
        # Status bar
        self.status_var = tk.StringVar(value="Ready")
        status_bar = ttk.Label(self.root, textvariable=self.status_var, 
                              relief=tk.SUNKEN, anchor=tk.W)
        status_bar.pack(side=tk.BOTTOM, fill=tk.X)

        # identity management button
        identity_btn = ttk.Button(top_frame, text="Manage Identity",
                                command=self._show_identity_dialog)
        identity_btn.pack(side=tk.LEFT, padx=5)
        
        # Bind window close event
        self.root.protocol("WM_DELETE_WINDOW", self._on_closing)
        
    def _create_protocol_tab(self, parent, protocol_name):
        """Create a tab for a specific protocol"""
        # Top controls
        control_frame = ttk.Frame(parent, padding=5)
        control_frame.pack(fill=tk.X)
        
        enable_var = tk.BooleanVar(value=False)
        enable_check = ttk.Checkbutton(control_frame, text=f"Enable {protocol_name.upper()}",
                                      variable=enable_var,
                                      command=lambda: self._toggle_protocol(protocol_name, enable_var.get()))
        enable_check.pack(side=tk.LEFT)
        
        # Protocol-specific settings can be added here
        
        # Log display
        log_frame = ttk.LabelFrame(parent, text=f"{protocol_name.upper()} Log", padding=5)
        log_frame.pack(fill=tk.BOTH, expand=True, pady=5)
        
        log_text = scrolledtext.ScrolledText(log_frame, height=15, state='disabled')
        log_text.pack(fill=tk.BOTH, expand=True)
        
        # Store references
        setattr(self, f"{protocol_name}_enable_var", enable_var)
        setattr(self, f"{protocol_name}_log", log_text)
        
        
    def _init_protocols(self):
        """Initialize protocol handlers"""
        default_format = self.message_formats[self.current_message_format]
        
        self.protocols = {
            "udp": UDPProtocol(
                self.peer_id,
                self._on_peer_discovered,
                self._on_message_received,
                message_format=default_format,
                peer_manager=self.peer_manager
            ),
            "tcp": TCPProtocol(
                self.peer_id,
                self._on_peer_discovered,
                self._on_message_received, 
                message_format=default_format,
                peer_manager=self.peer_manager
            ),
            "mdns": MDNSProtocol(
                self.peer_id,
                self._on_peer_discovered,
                self._on_message_received,
                message_format=default_format,
                peer_manager=self.peer_manager
            ),
            "windows": WindowsProtocol(
                self.peer_id,
                self._on_peer_discovered,
                self._on_message_received,
                message_format=default_format,
                peer_manager=self.peer_manager
            )
        }

        # Start a thread to update the logs periodically
        log_updater = threading.Thread(target=self._update_logs_thread)
        log_updater.daemon = True
        log_updater.start()

    def _change_message_format(self):
        """Change the message format for all protocols"""
        format_name = self.message_format_var.get()
        if format_name not in self.message_formats:
            return
        
        self.current_message_format = format_name
        new_format = self.message_formats[format_name]

        # update all active protocols with the new message format
        for protocol_name in self.active_protocols:
            protocol = self.protocols.get(protocol_name)
            if protocol:
                was_active = protocol_name in self.active_protocols
                if was_active:
                    self._toggle_protocol(protocol_name, False)

                protocol.message_format = new_format

                if was_active:
                    self._toggle_protocol(protocol_name, True)

        self.status_var.set(f"Message format changed to {format_name.upper()}")

    def _toggle_protocol(self, protocol_name, enabled):
        """Enable or disable a protocol"""
        protocol = self.protocols.get(protocol_name)
        if not protocol:
            return
            
        if enabled:
            if protocol_name not in self.active_protocols:
                protocol.message_format = self.message_formats[self.current_message_format]
                protocol.start()
                self.active_protocols.add(protocol_name)
                self.status_var.set(f"{protocol_name.upper()} protocol enabled")
        else:
            if protocol_name in self.active_protocols:
                protocol.stop()
                self.active_protocols.remove(protocol_name)
                self.status_var.set(f"{protocol_name.upper()} protocol disabled")
    
    def _update_peer_id(self):
        """Update the peer ID"""
        new_id = self.peer_id_var.get().strip()
        if not new_id:
            return
            
        old_id = self.peer_id
        self.peer_id = new_id
        
        # Update all active protocols
        for protocol_name in self.active_protocols:
            protocol = self.protocols.get(protocol_name)
            if protocol:
                protocol.stop()
                protocol.peer_id = new_id
                protocol.message_format = self.message_formats[self.current_message_format]
                protocol.start()
                
        self.status_var.set(f"Peer ID updated from {old_id} to {new_id}")
    
    def _on_peer_discovered(self, peer_id: str, protocol: str):
        """Callback when a peer is discovered"""
        peer = self.peer_manager.get_peer(peer_id)
        if peer:
            # Store current selection
            current_selection = self.peers_list.curselection()
            selected_peer = self.peers_list.get(current_selection[0]) if current_selection else None
            
            # Update peers list display
            protocols = ", ".join(peer.get_active_protocols())
            
            # Check if peer is already in list
            for i in range(self.peers_list.size()):
                if self.peers_list.get(i).startswith(peer_id):
                    self.peers_list.delete(i)
                    self.peers_list.insert(i, f"{peer_id} ({protocols})")
                    break
            else:
                # Add new peer to list
                self.peers_list.insert(tk.END, f"{peer_id} ({protocols})")
                
            # Restore selection if the previously selected peer is still in the list
            if selected_peer:
                for i in range(self.peers_list.size()):
                    if self.peers_list.get(i) == selected_peer:
                        self.peers_list.selection_set(i)
                        break

    def _on_message_received(self, peer_id: str, message: str, protocol: str):
        """Callback when a message is received"""
        self.message_history.config(state='normal')
        timestamp = time.strftime("%H:%M:%S")
        self.message_history.insert(tk.END, 
                                   f"[{timestamp}] {peer_id} ({protocol}): {message}\n")
        self.message_history.see(tk.END)
        self.message_history.config(state='disabled')
    
    def _on_peer_selected(self, event):
        """Handle peer selection from the list"""
        selection = self.peers_list.curselection()
        if selection:
            peer_entry = self.peers_list.get(selection[0])
            peer_id = peer_entry.split(" ")[0]  # Extract peer ID
            
            # Enable send button if there are active protocols
            if self.active_protocols:
                self.send_btn.config(state=tk.NORMAL)
            else:
                self.send_btn.config(state=tk.DISABLED)
        else:
            self.send_btn.config(state=tk.DISABLED)
    
    def _send_message(self):
        """Send a message to the selected peer"""
        selection = self.peers_list.curselection()
        if not selection:
            return
            
        message = self.message_var.get().strip()
        if not message:
            return
            
        peer_entry = self.peers_list.get(selection[0])
        peer_id = peer_entry.split(" ")[0]  # Extract peer ID
        
        protocol_choice = self.send_protocol_var.get()
        peer = self.peer_manager.get_peer(peer_id)
        
        if not peer:
            self.log(f"DEBUG: Peer {peer_id} not found in peer manager")
            self.log(f"DEBUG: Known peers: {list(self.peer_manager.get_all_peers().keys())}")
            self.status_var.set(f"Unknown peer: {peer_id}")
            return

        self.log(f"DEBUG: Found peer {peer_id} in peer manager")
        active_protocols = peer.get_active_protocols()
        self.log(f"DEBUG: Peer protocols: {active_protocols}")
        self.log(f"DEBUG: Our active protocols: {self.active_protocols}")

    def _broadcast_message(self):
        """Broadcast a message to all peers"""
        message = self.message_var.get().strip()
        if not message:
            return
            
        protocol_choice = self.send_protocol_var.get()
        sent = False
        
        if protocol_choice == "any":
            # Try each active protocol
            for protocol_name in self.active_protocols:
                protocol = self.protocols.get(protocol_name)
                if protocol:
                    try:
                        protocol.broadcast_message(message)
                        sent = True
                    except Exception as e:
                        self.log(f"Error broadcasting on {protocol_name}: {str(e)}")
                        
            if sent:
                # Add to message history
                self.message_history.config(state='normal')
                timestamp = time.strftime("%H:%M:%S")
                self.message_history.insert(tk.END, 
                                        f"[{timestamp}] You (broadcast): {message}\n")
                self.message_history.see(tk.END)
                self.message_history.config(state='disabled')
                
                self.message_var.set("")  # Clear message input
                self.status_var.set("Message broadcast successfully")
            else:
                self.status_var.set("Failed to broadcast message on any protocol")
                
        else:
            # Use the specified protocol
            if protocol_choice in self.active_protocols:
                protocol = self.protocols.get(protocol_choice)
                if protocol:
                    try:
                        protocol.broadcast_message(message)
                        
                        # Add to message history
                        self.message_history.config(state='normal')
                        timestamp = time.strftime("%H:%M:%S")
                        self.message_history.insert(tk.END, 
                                                f"[{timestamp}] You ({protocol_choice} broadcast): {message}\n")
                        self.message_history.see(tk.END)
                        self.message_history.config(state='disabled')
                        
                        self.message_var.set("")  # Clear message input
                        self.status_var.set(f"Message broadcast on {protocol_choice}")
                    except Exception as e:
                        self.status_var.set(f"Error broadcasting on {protocol_choice}: {str(e)}")
            else:
                self.status_var.set(f"Protocol {protocol_choice} is not active")
    
    def _update_logs_thread(self):
        """Thread to update protocol logs periodically"""
        while True:
            try:
                self._update_protocol_logs()
            except Exception as e:
                print(f"Error updating logs: {str(e)}")
            time.sleep(0.5)  # Update every half second
    
    def _update_protocol_logs(self):
        """Update the protocol logs in the GUI without disturbing selections"""
        for protocol_name, protocol in self.protocols.items():
            log_widget = getattr(self, f"{protocol_name}_log", None)
            if log_widget:
                # Get current view position
                current_view = log_widget.yview()
                
                # Store cursor position if widget has focus
                has_focus = log_widget.focus_get() == log_widget
                if has_focus:
                    cursor_pos = log_widget.index(tk.INSERT)
                
                # Update content
                log_widget.config(state='normal')
                current_text = log_widget.get('1.0', tk.END).strip()
                new_text = "\n".join(protocol.log_messages)
                
                # Only update if content actually changed
                if current_text != new_text:
                    log_widget.delete('1.0', tk.END)
                    log_widget.insert(tk.END, new_text)
                    
                    # Restore view position
                    log_widget.yview_moveto(current_view[0])
                    
                    # Restore cursor if widget had focus
                    if has_focus:
                        log_widget.mark_set(tk.INSERT, cursor_pos)
                
                log_widget.config(state='disabled') 

    def _on_closing(self):
        """Handle window closing event"""
        # Stop all protocols
        for protocol_name, protocol in self.protocols.items():
            protocol.stop()
            
        self.root.destroy()

    def _show_identity_dialog(self):
        dialog = IdentityDialog(self.root, self.identity, peer_manager=self.peer_manager)
        self.root.wait_window(dialog)
        if dialog.result:
            self.identity = dialog.result
            self.peer_id = self.identity.peer_id
            self.peer_id_var.set(self.peer_id)
            self._update_peer_id()
            self.status_var.set(f"Identity updated: {self.identity.display_name}")

    def log(self, message: str):
        """Add a message to all protocol logs for debugging"""
        for protocol_name in self.protocols:
            log_widget = getattr(self, f"{protocol_name}_log", None)
            if log_widget:
                log_widget.config(state='normal')
                timestamp = time.strftime("%H:%M:%S")
                log_widget.insert(tk.END, f"[{timestamp}] {message}\n")
                log_widget.see(tk.END)
                log_widget.config(state='disabled')