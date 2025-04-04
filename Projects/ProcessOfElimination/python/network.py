import socket
import threading
import json
import time
import random
import subprocess
import webbrowser
import os
from urllib.parse import quote

try:
    from zeroconf import Zeroconf, ServiceInfo, ServiceBrowser, ServiceListener
    ZEROCONF_AVAILABLE = True
except ImportError:
    print("Zeroconf not available; service discovery will not work")
    ZEROCONF_AVAILABLE = False

# Constants for networking modes
class NetworkMode:
    ZEROCONF = "zeroconf"
    UDP_BROADCAST = "udp_broadcast"
    DIRECT = "direct"

if ZEROCONF_AVAILABLE:
    class DiscoveryListener(ServiceListener):
        """Listener for Zeroconf service discovery"""
        def __init__(self):
            self.services = {}
            self.event = threading.Event()
        
        def add_service(self, zeroconf, service_type, name):
            info = zeroconf.get_service_info(service_type, name)
            if info:
                self.services[name] = info
                self.event.set()
        
        def update_service(self, zeroconf, service_type, name):
            self.add_service(zeroconf, service_type, name)
        
        def remove_service(self, zeroconf, service_type, name):
            if name in self.services:
                del self.services[name]

class PoE_Server:
    def __init__(self, meeting, ui, host='0.0.0.0', port=None):
        self.meeting = meeting
        self.ui = ui
        self.host = host

        # try commonly available ports
        self.port = port if port else self.find_available_port([
            8080,  # Common HTTP alternate
            80,    # HTTP - often open
            443,   # HTTPS - often open 
            5000,  # Common development port
            5555,  # Original port
            random.randint(49152, 65535)  # Random high port as last resort
        ])

        # TCP socket for server
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        # discover mechanisms
        self.network_mode = None
        self.zeroconf = None
        self.service_info = None
        self.udp_server = None
        self.discovery_thread = None
        
        # Initialize clients list
        self.clients = []

        # service constants
        self.SERVICE_TYPE = "_poe._tcp.local."
        self.SERVICE_NAME = f"POE_{socket.gethostname()}_{self.port}"
        self.UDP_DISCOVERY_PORT = 5556

    def find_available_port(self, port_list):
        """Try each port in the list and return the first available one"""
        for port in port_list:
            temp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            temp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            try:
                temp_socket.bind((self.host, port))
                temp_socket.close()
                return port
            except:
                temp_socket.close()
                continue
        return 5555  # Default fallback
    
    # Add this method to help with connection sharing
    def show_connection_info(self):
        """Display connection info and provide easy sharing options"""
        hostname = socket.gethostname()
        local_ip = socket.gethostbyname(hostname)
        
        try:
            # Try to get the external IP address (will only work if we have internet access)
            external_ip = None
            try:
                import urllib.request
                external_ip = urllib.request.urlopen('https://api.ipify.org').read().decode('utf-8')
            except:
                pass
                
            # Create a message with connection details
            message = f"\n=== CONNECTION INFORMATION ===\n"
            message += f"Local IP: {local_ip}\n"
            message += f"Port: {self.port}\n"
            
            if external_ip:
                message += f"External IP: {external_ip} (only works outside your network)\n"
                
            message += "\nShare this information with others who want to join.\n"
            message += "They should use the Local IP if they're on the same network.\n"
            
            # Add easy share options
            share_text = f"Join my Process of Elimination meeting at {local_ip}:{self.port}"
            message += f"\nShare options:\n"
            
            # Generate a QR code command if qrcode is available
            try:
                import qrcode
                img = qrcode.make(share_text)
                qr_path = "poe_connect.png"
                img.save(qr_path)
                message += f"- QR Code saved to: {qr_path}\n"
            except ImportError:
                message += "- Install 'qrcode' package for QR code sharing\n"
            
            # Email sharing
            email_link = f"mailto:?subject=Process%20of%20Elimination%20Meeting&body={quote(share_text)}"
            message += f"- Email: {email_link}\n"
            
            print(message)
            
            # Offer to open browser with the instructions
            if self.ui and hasattr(self.ui, "show_message"):
                self.ui.show_message("Connection Information", message)
                
        except Exception as e:
            print(f"Error generating connection info: {e}")
        

    
    def start(self):
        try:
            self.server.bind((self.host, self.port))
            self.server.listen(5)
            self.running = True

            if self.start_zeroconf():
                self.network_mode = NetworkMode.ZEROCONF
            elif self.start_udp_discovery():
                self.network_mode = NetworkMode.UDP_BROADCAST
            else:
                self.network_mode = NetworkMode.DIRECT
                print("Falling back to direct connection")
            
            # Get local IP address for display
            hostname = socket.gethostname()
            local_ip = socket.gethostbyname(hostname)
            print(f"Server started on {local_ip}:{self.port} using {self.network_mode}")
            
            # Start listening for connections in a separate thread
            thread = threading.Thread(target=self.accept_connections)
            thread.daemon = True
            thread.start()

            # After server start is successful:
            self.show_connection_info()
            
            # Consider opening firewall ports if we have admin access
            self.try_open_firewall_port()
            
            return True
        except Exception as e:
            print(f"Error starting server: {e}")
            return False
        
    def try_open_firewall_port(self):
        """Attempt to open the firewall port if possible"""
        try:
            if os.name == 'nt':  # Windows
                # Check if we're running with admin privileges
                admin = False
                try:
                    admin = ctypes.windll.shell32.IsUserAnAdmin() != 0
                except:
                    pass
                
                if admin:
                    # Try to add firewall rule using netsh
                    app_path = sys.executable  # Python interpreter path
                    rule_name = f"ProcessOfElimination_{self.port}"
                    
                    # Delete any existing rule with this name
                    subprocess.run(
                        f'netsh advfirewall firewall delete rule name="{rule_name}"',
                        shell=True, stderr=subprocess.DEVNULL, stdout=subprocess.DEVNULL
                    )
                    
                    # Add new rule
                    subprocess.run(
                        f'netsh advfirewall firewall add rule name="{rule_name}" dir=in action=allow protocol=TCP localport={self.port} program="{app_path}" enable=yes',
                        shell=True
                    )
                    print(f"Added firewall rule for port {self.port}")
                else:
                    print("Not running as administrator - firewall rule not added")
            
        except Exception as e:
            print(f"Could not configure firewall: {e}")

    def start_zeroconf(self):
        """Start zeroconf service discovery"""
        if not ZEROCONF_AVAILABLE:
            print("Zeroconf not available; service discovery will not work")
            return False
        
        try:
            self.zeroconf = Zeroconf()
            ip_addr = socket.gethostbyname(socket.gethostname())
            addresses = [socket.inet_aton(ip_addr)]
            properties = {
                'version': '1.0',
                'hostname': socket.gethostname(),
                'app': 'ProcessOfElimination'
            }

            self.service_info = ServiceInfo(
                self.SERVICE_TYPE,
                f"{self.SERVICE_NAME}.{self.SERVICE_TYPE}",
                addresses=addresses,
                port=self.port,
                properties=properties,
                server=socket.gethostname() + ".local.",
            )

            # Register the service directly here instead of calling separate method
            self.zeroconf.register_service(self.service_info)
            print(f"Zeroconf service registered as: {self.SERVICE_NAME}")

            return True
        except Exception as e:
            print(f"Error starting zeroconf: {e}")
            if self.zeroconf:
                self.zeroconf.close()
                self.zeroconf = None
            return False

    def start_udp_discovery(self):
        """Start UDP broadcast discovery"""
        try:
            self.udp_server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.udp_server.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            self.udp_server.bind(('0.0.0.0', self.UDP_DISCOVERY_PORT))

            # Start thread to listen for discovery requests - fixed the method name
            self.discovery_thread = threading.Thread(target=self.handle_discovery_requests)
            self.discovery_thread.daemon = True
            self.discovery_thread.start()

            print(f"UDP discovery started on port {self.UDP_DISCOVERY_PORT}")

            return True
        except Exception as e:
            print(f"Error starting UDP discovery: {e}")
            if self.udp_server:
                self.udp_server.close()
                self.udp_server = None
            return False
        
    def handle_discovery_requests(self):
        """listen for and respond to UDP discovery requests"""
        while self.running:
            try:
                data, addr = self.udp_server.recvfrom(1024)
                request = data.decode('utf-8')

                if request == "DISCOVER_PROCESS_OF_ELIMINATION_SERVER":
                    hostname = socket.gethostname()
                    local_ip = socket.gethostbyname(hostname)
                    response = json.dumps({
                        "type": "server_info",
                        "host": local_ip,
                        "port": self.port,
                        "name": self.SERVICE_NAME
                    }).encode('utf-8')
                    self.udp_server.sendto(response, addr)
                    print(f"Responding to discovery request from {addr}")
            except Exception as e:
                if self.running:
                    print(f"Error handling discovery request: {e}")
                    time.sleep(1)

    def register_service(self):
        """ register service using zeroconf/bonjour """
        try:
            self.zeroconf = Zeroconf()
            service_type = "_processOfElimination._tcp.local."
            service_name = f"Performing reductive analysis on {socket.gethostname()}:{self.port}.{service_type}"
            ip_addr = socket.gethostbyname(socket.gethostname())
            addresses = [socket.inet_aton(ip_addr)]
            properties = {}

            self.service_info = ServiceInfo(
                service_type,
                service_name,
                addresses=addresses,
                port=self.port,
                properties=properties,
                server=socket.gethostname() + ".local",
            )
            self.zeroconf.register_service(self.service_info)
            print(f"Zeroconf service registed as: {service_name}")

        except Exception as e:
            print(f"Error with service: {e}")
    
    def accept_connections(self):
        while self.running:
            try:
                client, address = self.server.accept()
                print(f"Connection from {address}")
                
                # Add client to list
                self.clients.append(client)
                
                # Start thread to handle client
                thread = threading.Thread(target=self.handle_client, args=(client,))
                thread.daemon = True
                thread.start()
                
                # Send current meeting state to new client
                self.send_meeting_state(client)
            except Exception as e:
                if self.running:
                    print(f"Error accepting connection: {e}")
                    time.sleep(1)
    
    def handle_client(self, client):
        while self.running:
            try:
                data = client.recv(1024).decode('utf-8')
                if not data:
                    break
                
                message = json.loads(data)
                self.process_message(message)
            except Exception as e:
                print(f"Error handling client: {e}")
                break
        
        # Remove disconnected client
        if client in self.clients:
            self.clients.remove(client)
    
    def process_message(self, message):
        if message["type"] == "guess":
            letter = message["letter"]
            self.meeting.propose_solution(letter)
            
            # Update UI and broadcast new state
            self.ui.update_display()
            self.broadcast_meeting_state()
    
    def send_meeting_state(self, client=None):
        meeting_state = self.meeting.get_meeting_state()
        message = {
            "type": "meeting_state",
            "data": {
                "actual_word": meeting_state["word"],
                "display_word": meeting_state["display_word"],
                "guessed_letters": meeting_state["guessed_letters"],
                "incorrect_guesses": meeting_state["incorrect_guesses"],
                "max_incorrect": meeting_state["max_incorrect"],
                "state": meeting_state["state"].value
            }
        }
        
        data = json.dumps(message).encode('utf-8')
        
        if client:
            try:
                client.send(data)
            except Exception as e:
                print(f"Error sending to client: {e}")
                if client in self.clients:
                    self.clients.remove(client)
        else:
            # Broadcast to all clients
            self.broadcast(data)
    
    def broadcast_meeting_state(self):
        self.send_meeting_state()
    
    def broadcast(self, message):
        disconnected_clients = []
        
        for client in self.clients:
            try:
                client.send(message)
            except Exception as e:
                print(f"Error broadcasting to client: {e}")
                disconnected_clients.append(client)
        
        # Remove disconnected clients
        for client in disconnected_clients:
            if client in self.clients:
                self.clients.remove(client)
    
    def start_new_meeting(self):
        self.meeting.reset_meeting()
        self.meeting.start_meeting()
        self.ui.update_display()
        self.broadcast_meeting_state()
    
    def stop(self):
        self.running = False

        for client in self.clients:
            try:
                client.close()
            except:
                pass

        try:
            self.server.close()
        except:
            pass

        if self.network_mode == NetworkMode.ZEROCONF and self.zeroconf:
            try:
                self.zeroconf.unregister_service(self.service_info)
                self.zeroconf.close()
            except Exception as e:
                print(f"Could not unregister service {e}")

        if self.udp_server:
            try:
                self.udp_server.close()
            except:
                pass

class PoE_Client:
    def __init__(self, meeting, ui, host=None, port=5555):
        self.meeting = meeting
        self.ui = ui
        self.host = host
        self.port = port
        
        # TCP client
        self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.running = False
        
        # Discovery mechanisms
        self.network_mode = None
        self.zeroconf = None
        self.discovered_servers = []
        
        # Service constants
        self.SERVICE_TYPE = "_poe._tcp.local."
        self.UDP_DISCOVERY_PORT = 5556
    
    def connect(self):
        """Connect with automatic server discovery"""
        if self.host:
            if ":" in self.host:
                host_parts = self.host.split(":")
                if len(host_parts) == 2 and host_parts[1].isdigit():
                    self.host = host_parts[0]
                    self.port = int(host_parts[1])

                if self.connect_direct(self.host, self.port):
                    return True
                
                common_ports = [80, 443, 8080, 5000, 5555]
                for port in common_ports:
                    if port != self.port and self.connect_direct(self.host, port):
                        return True
        
        # Try discovery methods in order
        if self.discover_via_zeroconf():
            self.network_mode = NetworkMode.ZEROCONF
            return True
        
        if self.discover_via_udp():
            self.network_mode = NetworkMode.UDP_BROADCAST
            return True
        
        print("All automatic discovery methods failed.")
        return False
    
    def connect_direct(self, host, port):
        """Direct connection to specified host:port"""
        try:
            self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.client.connect((host, port))
            self.host = host
            self.port = port
            self.running = True
            self.network_mode = NetworkMode.DIRECT
            
            # Start thread to receive messages
            thread = threading.Thread(target=self.receive_messages)
            thread.daemon = True
            thread.start()
            
            print(f"Connected directly to {host}:{port}")
            return True
        except Exception as e:
            print(f"Direct connection failed: {e}")
            return False
    
    def discover_via_zeroconf(self):
        """Discover servers using Zeroconf"""
        try:
            print("Searching for servers via Zeroconf...")
            self.zeroconf = Zeroconf()
            listener = DiscoveryListener()
            browser = ServiceBrowser(self.zeroconf, self.SERVICE_TYPE, listener)
            
            # Wait for services to be discovered (with timeout)
            found = listener.event.wait(timeout=5.0)
            
            if found and listener.services:
                # Get the first discovered service
                service_name = next(iter(listener.services))
                service_info = listener.services[service_name]
                
                # Get host and port
                if service_info.addresses:
                    host = socket.inet_ntoa(service_info.addresses[0])
                    port = service_info.port
                    print(f"Found server via Zeroconf: {host}:{port}")
                    
                    # Connect to the discovered server
                    result = self.connect_direct(host, port)
                    
                    # Clean up Zeroconf
                    browser.cancel()
                    self.zeroconf.close()
                    self.zeroconf = None
                    
                    return result
            
            # Clean up Zeroconf if no server found
            browser.cancel()
            self.zeroconf.close()
            self.zeroconf = None
            print("No servers found via Zeroconf")
            return False
            
        except Exception as e:
            print(f"Zeroconf discovery failed: {e}")
            if self.zeroconf:
                self.zeroconf.close()
                self.zeroconf = None
            return False
    
    def discover_via_udp(self):
        """Discover servers using UDP broadcast"""
        try:
            print("Searching for servers via UDP broadcast...")
            udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            udp_socket.settimeout(5.0)  # Set timeout for discovery
            
            # Send discovery broadcast
            broadcast_message = "DISCOVER_PROCESS_OF_ELIMINATION_SERVER"
            udp_socket.sendto(broadcast_message.encode('utf-8'), ('<broadcast>', self.UDP_DISCOVERY_PORT))
            
            # Wait for responses with timeout
            start_time = time.time()
            self.discovered_servers = []
            
            while time.time() - start_time < 5.0:  # 5 second timeout
                try:
                    data, addr = udp_socket.recvfrom(1024)
                    response = json.loads(data.decode('utf-8'))
                    
                    if response["type"] == "server_info":
                        self.discovered_servers.append(response)
                        print(f"Found server via UDP: {response['host']}:{response['port']}")
                except socket.timeout:
                    break
                except Exception as e:
                    print(f"Error receiving UDP response: {e}")
            
            udp_socket.close()
            
            # Connect to the first discovered server
            if self.discovered_servers:
                server = self.discovered_servers[0]
                return self.connect_direct(server["host"], server["port"])
            
            print("No servers found via UDP broadcast")
            return False
            
        except Exception as e:
            print(f"UDP discovery failed: {e}")
            return False

    def receive_messages(self):
        while self.running:
            try:
                data = self.client.recv(1024).decode('utf-8')
                if not data:
                    break
                
                message = json.loads(data)
                self.process_message(message)
            except Exception as e:
                if self.running:
                    print(f"Error receiving message: {e}")
                    break
        
        self.disconnect()
    
    def process_message(self, message):
        if message["type"] == "meeting_state":
            data = message["data"]
            
            # Update meeting state based on server data
            from logic import Transcript
            
            # Update guessed letters
            self.meeting.word = data["actual_word"]
            self.meeting.guessed_letters = set(data["guessed_letters"])
            self.meeting.incorrect_guesses = data["incorrect_guesses"]
            self.meeting.state = Transcript(data["state"])
            
            # Update UI
            self.ui.root.after(0, self.ui.update_display)
    
    def propose_solution(self, letter):
        message = {
            "type": "guess",
            "letter": letter
        }
        
        self.send_message(message)
    
    def send_message(self, message):
        try:
            data = json.dumps(message).encode('utf-8')
            self.client.send(data)
        except Exception as e:
            print(f"Error sending message: {e}")
            self.disconnect()
    
    def disconnect(self):
        self.running = False
        try:
            self.client.close()
        except:
            pass

