import win32pipe
import win32file
import win32security
import win32api
import win32net
import win32con
import win32netcon
import time
import os
import re  # Add regex module
import threading
import socket
import json
from typing import Optional, Dict
from ldap3 import Server, Connection, SUBTREE

from protocols.base import ProtocolBase
from peer import PeerIdentity

class WindowsProtocol(ProtocolBase):
    """Combined Windows protocol using both Named Pipes and AD"""
    
    def __init__(self, peer_id: str, on_peer_discovered, on_message, **kwargs):
        super().__init__(peer_id, on_peer_discovered, on_message, **kwargs)
        
        # Windows identity info
        try:
            self.domain = win32api.GetDomainName()
            self.username = win32api.GetUserNameEx(win32con.NameSamCompatible)
            self.computer_name = win32api.GetComputerName()
        except Exception as e:
            self.log(f"Error getting Windows identity info: {str(e)}")
            self.domain = ""
            self.username = ""
            self.computer_name = socket.gethostname()
        
        # Named pipe settings
        self.pipe_name = r"\\.\pipe\p2ptester"
        self.pipe_handles = {}
        
        # Update peer identity with Windows info
        self._update_peer_identity()
        
    def _update_peer_identity(self):
        """Update peer identity with Windows-specific information"""
        if self.peer_manager and hasattr(self.peer_manager, 'identity'):
            identity = self.peer_manager.identity
            identity.windows_username = self.username
            identity.windows_domain = self.domain
            identity.windows_computer = self.computer_name
            
    def _run(self):
        """Main protocol loop"""
        try:
            # Start named pipe server
            self._start_pipe_server()
            
            # Start discovery in a separate thread
            discovery_thread = threading.Thread(target=self._discovery_thread)
            discovery_thread.daemon = True
            discovery_thread.start()
            
            # Main loop for handling pipe connections
            while self.running:
                try:
                    self._handle_pipe_connections()
                except Exception as e:
                    self.log(f"Pipe connection error: {str(e)}")
                time.sleep(0.1)
                
        except Exception as e:
            self.log(f"Windows protocol error: {str(e)}")
        finally:
            self._cleanup()

    def _discovery_thread(self):
        """Thread to handle peer discovery"""
        try:
            # Try domain discovery first
            try:
                self._discover_domain_peers()
            except Exception as e:
                self.log(f"Domain discovery failed: {str(e)}")
                
            # As fallback, use simple hostname discovery for local network
            while self.running:
                try:
                    self._discover_local_peers()
                    time.sleep(30)  # Check every 30 seconds
                except Exception as e:
                    self.log(f"Local discovery error: {str(e)}")
                    time.sleep(60)
        except Exception as e:
            self.log(f"Discovery thread error: {str(e)}")

    def _discover_local_peers(self):
        """Simple hostname-based discovery for local network"""
        try:
            # Try localhost first as a test
            self._try_connect_to_computer("localhost")
            
            try:
                # Try to get neighboring computers via NetShareEnum
                handle = win32net.NetServerEnum(None, 100, win32netcon.SV_TYPE_WORKSTATION)
                for server in handle[0]:
                    hostname = server['name']
                    if hostname != self.computer_name:
                        self.log(f"Found neighboring computer: {hostname}")
                        self._try_connect_to_computer(hostname)
            except Exception as e:
                self.log(f"NetServerEnum failed: {str(e)}")
            
            # Try additional discovery methods appropriate for corporate networks
            try:
                # Use environment variables that might have nearby computer names
                if 'USERDOMAIN' in os.environ:
                    self.log(f"Trying USERDOMAIN: {os.environ['USERDOMAIN']}")
                    self._try_connect_to_computer(os.environ['USERDOMAIN'])
                
                # Try to identify computers used frequently
                recent_servers = []
                try:
                    import winreg
                    key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, 
                                        r"Software\Microsoft\Windows\CurrentVersion\Explorer\ComputerDescriptions")
                    i = 0
                    while True:
                        try:
                            computer = winreg.EnumKey(key, i)
                            recent_servers.append(computer)
                            i += 1
                        except OSError:
                            break
                    winreg.CloseKey(key)
                except Exception as e:
                    self.log(f"Could not enumerate recent computers: {str(e)}")
                    
                for computer in recent_servers:
                    self.log(f"Trying recent computer: {computer}")
                    self._try_connect_to_computer(computer)
                    
            except Exception as e:
                self.log(f"Advanced discovery failed: {str(e)}")
                
        except Exception as e:
            self.log(f"Local discovery error: {str(e)}")

    def _discover_domain_peers(self):
        """Discover peers using Network Browser or Active Directory"""
        try:
            # Check if we're in a domain
            is_domain = self.domain and self.domain != ""
            
            if is_domain:
                self.log(f"In domain: {self.domain}, attempting AD discovery")
                
                # Get domain controller info using nltest first - most reliable in corporate environments
                dc_address = None
                try:
                    import subprocess
                    result = subprocess.run(['nltest', '/dsgetdc:' + self.domain], 
                                           capture_output=True, text=True, timeout=5)
                    if result.returncode == 0:
                        self.log(f"Domain controller info: {result.stdout}")
                        
                        # Parse the IP address from nltest output
                        for line in result.stdout.splitlines():
                            if "Address" in line:
                                # Extract IP from format like "Address: \\10.128.130.12"
                                ip_match = re.search(r'\\\\(\d+\.\d+\.\d+\.\d+)', line)
                                if ip_match:
                                    dc_address = ip_match.group(1)
                                    self.log(f"Found DC IP via nltest: {dc_address}")
                                    break
                    else:
                        self.log(f"Failed to find DC using nltest: {result.stderr}")
                except Exception as e:
                    self.log(f"Could not get DC info via nltest: {str(e)}")
                
                # Try to find and connect to domain controller
                try:
                    # Method 1: NetGetDCName
                    try:
                        dc_name = win32net.NetGetDCName(None, None)
                        self.log(f"Found DC via NetGetDCName: {dc_name}")
                        # Clean up DC name - remove backslashes from UNC path format (\\server)
                        if dc_name.startswith('\\\\'):
                            clean_dc_name = dc_name[2:]
                            self.log(f"Cleaned DC name: {clean_dc_name}")
                            dc_address = clean_dc_name
                    except Exception as e:
                        self.log(f"NetGetDCName failed: {str(e)}")
                    
                    # Method 2: DNS lookup
                    if not dc_address:
                        try:
                            dc_address = socket.gethostbyname(f"{self.domain}")
                            self.log(f"Found DC via DNS lookup: {dc_address}")
                        except Exception as e:
                            self.log(f"DNS lookup failed: {str(e)}")
                    
                    # Method 3: LDAP SRV record
                    if not dc_address:
                        try:
                            import dns.resolver
                            answers = dns.resolver.resolve(f"_ldap._tcp.{self.domain}", 'SRV')
                            if answers:
                                dc_name = str(answers[0].target).rstrip('.')
                                dc_address = socket.gethostbyname(dc_name)
                                self.log(f"Found DC via LDAP SRV: {dc_address}")
                        except Exception as e:
                            self.log(f"LDAP SRV lookup failed: {str(e)}")
                    
                    if not dc_address:
                        self.log("Could not find domain controller, falling back to network discovery")
                        self._use_network_discovery()
                        return
                    
                    # Try to connect to LDAP
                    self.log(f"Attempting LDAP connection to {dc_address}")

                    # Ensure the DC address is in the correct format (no backslashes)
                    if dc_address and dc_address.startswith('\\\\'):
                        dc_address = dc_address[2:]
                        self.log(f"Reformatted DC address for LDAP: {dc_address}")

                    try:
                        server = Server(dc_address)
                        conn = Connection(server)
                        bind_result = conn.bind()
                        self.log(f"LDAP bind result: {bind_result}")
                        
                        base_dn = f"DC={self.domain.replace('.', ',DC=')}"
                        self.log(f"Successfully connected to LDAP, base DN: {base_dn}")
                        
                        # Rest of the AD search code
                        while self.running:
                            try:
                                # Search for computers
                                conn.search(
                                    base_dn,
                                    '(&(objectClass=computer)(operatingSystem=Windows*))',
                                    SUBTREE,
                                    attributes=['dNSHostName', 'name']
                                )
                                
                                for entry in conn.entries:
                                    hostname = entry.dNSHostName.value or entry.name.value
                                    if hostname and hostname != self.computer_name:
                                        self._try_connect_to_computer(hostname)
                                        
                                time.sleep(30)  # Check every 30 seconds
                            except Exception as e:
                                self.log(f"AD search error: {str(e)}")
                                time.sleep(60)  # Wait longer on error
                                break  # Exit the loop on error

                    except Exception as e:
                        self.log(f"Domain discovery unavailable: {str(e)}")
                        self._use_network_discovery()
                
                except Exception as e:
                    self.log(f"Domain controller connection failed: {str(e)}")
                    self._use_network_discovery()
            else:
                self.log("Not on a domain, using network browser")
                self._use_network_discovery()
                    
        except Exception as e:
            self.log(f"Peer discovery error: {str(e)}")
            self._discover_local_peers()

    def _use_network_discovery(self):
        """Use Windows network browser to find computers"""
        try:
            while self.running:
                try:
                    # Try NetServerEnum first
                    success = False
                    try:
                        resume = 0
                        servers, _, resume = win32net.NetServerEnum(
                            None, 100, win32netcon.SV_TYPE_ALL, None, resume, 1000
                        )
                        
                        if servers:
                            for server in servers:
                                hostname = server['name']
                                if hostname != self.computer_name:
                                    self._try_connect_to_computer(hostname)
                            
                            success = True
                    except Exception as e:
                        self.log(f"NetServerEnum failed: {str(e)}")
                        
                    # If NetServerEnum failed, try active scan
                    if not success:
                        self.log("Falling back to network scan")
                        self._scan_network()
                    
                    time.sleep(60)  # Check every minute
                    
                except Exception as e:
                    self.log(f"Network browser error: {str(e)}")
                    time.sleep(120)  # Wait longer on error
                    
        except Exception as e:
            self.log(f"Network discovery error: {str(e)}")
            # Fall back to basic discovery
            self._discover_local_peers()

    def _try_connect_to_computer(self, hostname):
        """Try to connect to a computer via named pipe"""
        try:
            pipe_path = f"\\\\{hostname}\\pipe\\p2ptester"
            
            # Try non-blocking connect
            handle = None
            try:
                # Just check if pipe exists
                handle = win32file.CreateFile(
                    pipe_path,
                    win32file.GENERIC_READ | win32file.GENERIC_WRITE,
                    0, None,
                    win32file.OPEN_EXISTING,
                    win32file.FILE_FLAG_OVERLAPPED,
                    None
                )
                
                # If we get here, pipe exists - register peer
                peer_id = f"win-{hostname}"  # Temporary ID until we get real one
                
                # Add peer to manager
                self.peer_manager.add_or_update_peer(
                    peer_id,
                    "windows",
                    computer_name=hostname,
                    hostname=hostname
                )
                
                # Notify discovery
                self.on_peer_discovered(peer_id, "windows")
                self.log(f"Discovered Windows peer: {hostname}")
                
                # Send identification message
                identity_msg = {
                    "type": "identity",
                    "peer_id": self.peer_id,
                    "computer_name": self.computer_name
                }
                message_bytes = (json.dumps(identity_msg) + "\n").encode()
                win32file.WriteFile(handle, message_bytes)
                
            except Exception as e:
                # Pipe doesn't exist or not accessible - ignore
                pass
                
            finally:
                if handle:
                    win32file.CloseHandle(handle)
                    
        except Exception as e:
            # Ignore connection failures - normal for computers that don't run our app
            pass
            
    def _start_pipe_server(self):
        """Start named pipe server"""
        try:
            security_attributes = win32security.SECURITY_ATTRIBUTES()
            security_descriptor = win32security.SECURITY_DESCRIPTOR()
            security_attributes.SECURITY_DESCRIPTOR = security_descriptor
            
            # Create DACL with permissions
            dacl = win32security.ACL()
            
            # Add Everyone group for local network access
            everyone_sid = win32security.CreateWellKnownSid(win32security.WinWorldSid)
            dacl.AddAccessAllowedAce(
                win32security.ACL_REVISION,
                win32con.GENERIC_READ | win32con.GENERIC_WRITE,
                everyone_sid
            )
            
            # Also add authenticated users
            auth_users_sid = win32security.CreateWellKnownSid(win32security.WinAuthenticatedUserSid)
            dacl.AddAccessAllowedAce(
                win32security.ACL_REVISION,
                win32con.GENERIC_READ | win32con.GENERIC_WRITE,
                auth_users_sid
            )
            
            security_descriptor.SetSecurityDescriptorDacl(1, dacl, 0)
            
            self.pipe_handles["server"] = win32pipe.CreateNamedPipe(
                self.pipe_name,
                win32pipe.PIPE_ACCESS_DUPLEX,
                win32pipe.PIPE_TYPE_MESSAGE | win32pipe.PIPE_READMODE_MESSAGE | win32pipe.PIPE_WAIT,
                win32pipe.PIPE_UNLIMITED_INSTANCES,
                65536, 65536,
                0,
                security_attributes
            )
            
            self.log("Named pipe server started successfully")
            
        except Exception as e:
            self.log(f"Error starting pipe server: {str(e)}")
            raise
            
    def _send_message_impl(self, peer_id: str, message: str) -> bool:
        """Implementation for sending a message to a peer"""
        try:
            peer = self.peer_manager.get_peer(peer_id)
            if not peer:
                self.log(f"No peer found with ID {peer_id}")
                return False

            # Get peer's computer name from stored info
            peer_info = peer.get_protocol_info("windows")
            if not peer_info or "computer_name" not in peer_info:
                self.log(f"No Windows connection info for peer {peer_id}")
                return False

            target_computer = peer_info["computer_name"]
            pipe_path = f"\\\\{target_computer}\\pipe\\p2ptester"

            try:
                # Open the pipe
                handle = win32file.CreateFile(
                    pipe_path,
                    win32file.GENERIC_WRITE,
                    0, None,
                    win32file.OPEN_EXISTING,
                    0, None
                )

                # Format message with peer ID and protocol
                formatted_message = {
                    "sender_id": self.peer_id,
                    "message": message,
                    "protocol": "windows",
                    "computer_name": self.computer_name
                }

                # Send the message
                message_bytes = (json.dumps(formatted_message) + "\n").encode()
                win32file.WriteFile(handle, message_bytes)
                win32file.CloseHandle(handle)
                return True

            except Exception as e:
                self.log(f"Error sending message to {target_computer}: {str(e)}")
                return False

        except Exception as e:
            self.log(f"Send message error: {str(e)}")
            return False

    def _handle_pipe_connections(self):
        """Handle incoming pipe connections"""
        try:
            # Wait for a client to connect
            win32pipe.ConnectNamedPipe(self.pipe_handles["server"], None)

            # Read the message
            result, data = win32file.ReadFile(self.pipe_handles["server"], 65536)
            if result == 0:  # Success
                try:
                    message_data = json.loads(data.decode().strip())
                    sender_id = message_data["sender_id"]
                    message = message_data["message"]
                    computer_name = message_data.get("computer_name")

                    # Update peer info
                    if computer_name:
                        self.peer_manager.add_or_update_peer(
                            sender_id, 
                            "windows",
                            computer_name=computer_name
                        )

                    # Notify message received
                    self.on_message(sender_id, message, "windows")

                except json.JSONDecodeError:
                    self.log("Received invalid message format")

            # Disconnect and wait for next client
            win32pipe.DisconnectNamedPipe(self.pipe_handles["server"])

        except Exception as e:
            self.log(f"Error handling pipe connection: {str(e)}")

    def _cleanup(self):
        """Clean up resources"""
        for handle in self.pipe_handles.values():
            try:
                win32file.CloseHandle(handle)
            except:
                pass
        self.pipe_handles.clear()

    def _scan_network(self):
        """Scan local network for potential peers"""
        try:
            # Get local IP
            local_ip = socket.gethostbyname(socket.gethostname())
            base_ip = '.'.join(local_ip.split('.')[:3])
            self.log(f"Scanning network: {base_ip}.* from {local_ip}")
            
            # Check common ports for our service on nearby IPs
            for i in range(1, 255):
                if not self.running:
                    break
                    
                target_ip = f"{base_ip}.{i}"
                if target_ip == local_ip:
                    continue  # Skip our own IP
                    
                # Try to connect to our pipe name
                try:
                    pipe_path = f"\\\\{target_ip}\\pipe\\p2ptester"
                    handle = win32file.CreateFile(
                        pipe_path,
                        win32file.GENERIC_READ | win32file.GENERIC_WRITE,
                        0, None,
                        win32file.OPEN_EXISTING,
                        win32file.FILE_FLAG_OVERLAPPED,
                        None
                    )
                    
                    if handle:
                        peer_id = f"win-{target_ip}"
                        self.peer_manager.add_or_update_peer(
                            peer_id, 
                            "windows",
                            computer_name=target_ip,
                            ip_address=target_ip
                        )
                        self.on_peer_discovered(peer_id, "windows")
                        self.log(f"Discovered peer at IP: {target_ip}")
                        win32file.CloseHandle(handle)
                except:
                    pass  # Ignore connection errors - normal for IPs without our service
                    
                # Don't scan too aggressively to avoid triggering network security
                time.sleep(0.05)
                
        except Exception as e:
            self.log(f"Network scan error: {str(e)}")