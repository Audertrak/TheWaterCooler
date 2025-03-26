# network_handler.py
import socket
import threading
import json
import logging
import time
from queue import Queue

# Constants
BROADCAST_PORT = 50000 # Port for UDP discovery broadcasts
TCP_PORT = 50001       # Port for direct TCP communication
BROADCAST_ADDR = "<broadcast>" # Standard broadcast address
# BROADCAST_ADDR = "255.255.255.255" # Alternative, sometimes needed
BUFFER_SIZE = 1024
DISCOVERY_INTERVAL = 10 # Seconds between automatic discovery broadcasts

class NetworkHandler:
    def __init__(self, gui_queue: Queue, peer_name: str, local_ip: str):
        self.gui_queue = gui_queue
        self.peer_name = peer_name
        self.local_ip = local_ip
        self.tcp_port = TCP_PORT # Could be made dynamic later
        self.peers = {} # Dictionary to store discovered peers {peer_id: {"name": name, "ip": ip, "tcp_port": port, "last_seen": timestamp}}
        self.running = True
        self.listener_threads = []
        self.discovery_timer_thread = None
        self.lock = threading.Lock() # To protect access to self.peers

    def _log_to_gui(self, message):
        """Safely put log messages onto the GUI queue."""
        self.gui_queue.put(("log", message))

    def _update_peer_list_gui(self):
        """Safely put the updated peer list onto the GUI queue."""
        with self.lock:
            # Create a copy to avoid issues if dict changes during iteration in GUI
            peers_copy = dict(self.peers)
        self.gui_queue.put(("peers", peers_copy))

    def _handle_discovery_message(self, data, addr):
        """Process received discovery messages."""
        try:
            message = json.loads(data.decode('utf-8'))
            peer_ip = addr[0] # Use the source IP from the packet
            # Ignore messages from self
            if peer_ip == self.local_ip and message.get("tcp_port") == self.tcp_port:
                return

            peer_id = f"{peer_ip}:{message.get('tcp_port', 'N/A')}"
            name = message.get("name", "Unknown")
            tcp_port = message.get("tcp_port")

            if not tcp_port:
                self._log_to_gui(f"Received discovery from {name} ({peer_ip}) without TCP port.")
                return

            with self.lock:
                is_new = peer_id not in self.peers
                self.peers[peer_id] = {
                    "name": name,
                    "ip": peer_ip,
                    "tcp_port": tcp_port,
                    "last_seen": time.time()
                }

            if is_new:
                self._log_to_gui(f"Discovered peer: {name} ({peer_id})")
                self._update_peer_list_gui()
                # Optional: Send a direct response back (unicast UDP)
                # self.send_discovery_message(peer_ip) # Be careful of loops

        except json.JSONDecodeError:
            self._log_to_gui(f"Received invalid JSON broadcast from {addr}")
        except Exception as e:
            self._log_to_gui(f"Error handling discovery from {addr}: {e}")

    def _udp_listener(self):
        """Listens for UDP broadcasts."""
        udp_socket = None
        try:
            udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
            # Allow reuse of address/port
            udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            # Enable broadcasting mode (needed for some systems to receive)
            udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            udp_socket.bind(("", BROADCAST_PORT)) # Bind to all interfaces
            self._log_to_gui(f"UDP Listener started on port {BROADCAST_PORT}")

            while self.running:
                try:
                    # Set a timeout so the loop can check self.running
                    udp_socket.settimeout(1.0)
                    data, addr = udp_socket.recvfrom(BUFFER_SIZE)
                    self._log_to_gui(f"Received UDP from {addr}")
                    self._handle_discovery_message(data, addr)
                except socket.timeout:
                    continue # Just loop again to check self.running
                except Exception as e:
                    if self.running: # Avoid logging errors during shutdown
                        self._log_to_gui(f"UDP Listener error: {e}")
                        time.sleep(1) # Avoid busy-looping on persistent errors

        except Exception as e:
             self._log_to_gui(f"Failed to start UDP Listener: {e}")
        finally:
            if udp_socket:
                udp_socket.close()
            self._log_to_gui("UDP Listener stopped.")

    def _handle_tcp_client(self, conn, addr):
        """Handles an incoming TCP connection."""
        self._log_to_gui(f"TCP connection established with {addr}")
        try:
            while self.running:
                data = conn.recv(BUFFER_SIZE)
                if not data:
                    break # Connection closed by peer
                message = data.decode('utf-8')
                self._log_to_gui(f"Received TCP from {addr}: {message}")
                # Optionally process the message further (e.g., parse JSON)
        except ConnectionResetError:
             self._log_to_gui(f"TCP connection reset by {addr}")
        except Exception as e:
            if self.running:
                self._log_to_gui(f"Error handling TCP client {addr}: {e}")
        finally:
            conn.close()
            self._log_to_gui(f"TCP connection closed with {addr}")

    def _tcp_listener(self):
        """Listens for incoming TCP connections."""
        tcp_socket = None
        try:
            tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            tcp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            tcp_socket.bind(("", self.tcp_port)) # Bind to all interfaces
            tcp_socket.listen(5) # Listen for up to 5 incoming connections
            self._log_to_gui(f"TCP Listener started on port {self.tcp_port}")

            while self.running:
                try:
                    # Set a timeout so the loop can check self.running
                    tcp_socket.settimeout(1.0)
                    conn, addr = tcp_socket.accept()
                    # Start a new thread to handle this specific client
                    client_thread = threading.Thread(
                        target=self._handle_tcp_client,
                        args=(conn, addr),
                        daemon=True # Allow program exit even if threads are running
                    )
                    client_thread.start()
                except socket.timeout:
                    continue # Just loop again to check self.running
                except Exception as e:
                    if self.running:
                        self._log_to_gui(f"TCP Listener error: {e}")
                        time.sleep(1) # Avoid busy-looping

        except Exception as e:
            self._log_to_gui(f"Failed to start TCP Listener: {e}")
        finally:
            if tcp_socket:
                tcp_socket.close()
            self._log_to_gui("TCP Listener stopped.")

    def _periodic_discovery(self):
        """Sends discovery broadcasts periodically."""
        while self.running:
            self.send_discovery_broadcast()
            # Wait for the interval, checking self.running frequently
            for _ in range(DISCOVERY_INTERVAL):
                if not self.running:
                    break
                time.sleep(1)

    def start_listeners(self):
        """Starts UDP and TCP listeners in separate threads."""
        self.running = True

        udp_thread = threading.Thread(target=self._udp_listener, daemon=True)
        tcp_thread = threading.Thread(target=self._tcp_listener, daemon=True)
        self.discovery_timer_thread = threading.Thread(target=self._periodic_discovery, daemon=True)

        self.listener_threads.extend([udp_thread, tcp_thread]) # Don't add timer thread here

        udp_thread.start()
        tcp_thread.start()
        self.discovery_timer_thread.start()

        self._log_to_gui("Network listeners starting...")

    def stop_listeners(self):
        """Stops all running network threads."""
        self._log_to_gui("Stopping network listeners...")
        self.running = False
        # Wait briefly for threads to check self.running
        time.sleep(1.1)
        # No need to explicitly join daemon threads if program is exiting
        # If you need guaranteed cleanup before proceeding, use join:
        # for thread in self.listener_threads:
        #     thread.join(timeout=2.0)
        # if self.discovery_timer_thread:
        #     self.discovery_timer_thread.join(timeout=2.0)
        self.listener_threads = []
        self.discovery_timer_thread = None
        self._log_to_gui("Network listeners should be stopped.")


    def send_discovery_broadcast(self, target_ip=BROADCAST_ADDR):
        """Sends a UDP broadcast message for discovery."""
        message = json.dumps({
            "name": self.peer_name,
            "tcp_port": self.tcp_port
        })
        udp_socket = None
        try:
            udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
            udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            # Important: Enable broadcast mode for sending
            udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

            udp_socket.sendto(message.encode('utf-8'), (target_ip, BROADCAST_PORT))
            if target_ip == BROADCAST_ADDR:
                 self._log_to_gui(f"Sent discovery broadcast")
            # else:
            #      self._log_to_gui(f"Sent discovery reply to {target_ip}")

        except Exception as e:
            self._log_to_gui(f"Error sending UDP broadcast to {target_ip}: {e}")
        finally:
            if udp_socket:
                udp_socket.close()

    def send_tcp_message(self, peer_id: str, message: str):
        """Sends a TCP message to a specific peer."""
        peer_info = None
        with self.lock:
            peer_info = self.peers.get(peer_id)

        if not peer_info:
            self._log_to_gui(f"Error: Peer {peer_id} not found.")
            return

        host = peer_info["ip"]
        port = peer_info["tcp_port"]

        tcp_socket = None
        try:
            # Create a new socket for each message (simple approach)
            # Could be optimized to reuse connections later
            tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            tcp_socket.settimeout(5.0) # Timeout for connection attempt
            tcp_socket.connect((host, port))
            tcp_socket.sendall(message.encode('utf-8'))
            self._log_to_gui(f"Sent TCP to {peer_info['name']} ({host}:{port}): {message}")
        except socket.timeout:
             self._log_to_gui(f"Timeout connecting to {peer_info['name']} ({host}:{port})")
        except ConnectionRefusedError:
             self._log_to_gui(f"Connection refused by {peer_info['name']} ({host}:{port})")
        except Exception as e:
            self._log_to_gui(f"Error sending TCP to {peer_info['name']} ({host}:{port}): {e}")
        finally:
            if tcp_socket:
                tcp_socket.close()

    def update_peer_name(self, new_name):
        """Updates the peer name used in discovery messages."""
        self.peer_name = new_name
        self._log_to_gui(f"Peer name updated to: {new_name}")
        # Optionally send an immediate broadcast with the new name
        # self.send_discovery_broadcast()

    def get_peers(self):
        """Returns a copy of the current peer list."""
        with self.lock:
            return dict(self.peers)
