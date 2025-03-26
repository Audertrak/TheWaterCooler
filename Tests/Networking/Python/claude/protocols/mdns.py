import socket
import json
import threading
import time
from typing import Callable, Dict, Tuple, Optional
from .base import ProtocolBase

from message.base import MessageBase
from message.json import JSONMessage
from message.protobuf import SimpleProtobufMessage
from message.mqtt import MQTTMessage
from message.raw import RawMessage

class MDNSProtocol(ProtocolBase):
    def __init__(self, peer_id: str, on_peer_discovered: Callable, on_message: Callable,
                 service_name: str = "_p2ptester._tcp.local.", port: int = 5558,
                 message_format: Optional[MessageBase] = None):
        super().__init__(peer_id, on_peer_discovered, on_message)
        self.service_name = service_name
        self.port = port
        self.server_socket = None
        self.zeroconf_thread = None
        
        # Use the provided message format or default to JSON
        self.message_format = message_format or JSONMessage() 
        
    def _run(self):
        """Main mDNS listener and advertising loop"""
        try:
            # Since we're sticking with standard library only,
            # we're implementing a simplified version of mDNS
            # For a real implementation, you'd use zeroconf library
            
            # Start TCP server for direct communication
            server_thread = threading.Thread(target=self._run_server)
            server_thread.daemon = True
            server_thread.start()
            
            # Simulate mDNS using UDP multicast
            self._run_mdns_simulation()
            
        except Exception as e:
            self.log(f"mDNS error: {str(e)}")
        finally:
            self._cleanup()
    
    def _run_server(self):
        """Run TCP server for mdns-discovered connections"""
        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_socket.bind(('', self.port))
            self.server_socket.listen(5)
            self.server_socket.settimeout(1.0)
            
            self.log(f"mDNS TCP server listening on port {self.port}")
            
            while self.running:
                try:
                    client_socket, addr = self.server_socket.accept()
                    client_thread = threading.Thread(
                        target=self._handle_client,
                        args=(client_socket, addr)
                    )
                    client_thread.daemon = True
                    client_thread.start()
                except socket.timeout:
                    pass
                except Exception as e:
                    if self.running:  # Only log if we're still supposed to be running
                        self.log(f"mDNS TCP server error: {str(e)}")
                        
        except Exception as e:
            self.log(f"mDNS TCP server error: {str(e)}")
        finally:
            if self.server_socket:
                self.server_socket.close()
                self.server_socket = None
    
    def _run_mdns_simulation(self):
        """Simulate mDNS using UDP multicast (simplified for standard library)"""
        # This is a very simplified implementation - real mDNS is more complex
        multicast_group = '224.0.0.251'  # Standard mDNS multicast group
        multicast_port = 5353  # Standard mDNS port
        
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            
            # Bind to the multicast port
            sock.bind(('', multicast_port))
            
            # Tell the operating system to add the socket to the multicast group
            # on all interfaces
            group = socket.inet_aton(multicast_group)
            mreq = socket.inet_aton(multicast_group) + socket.inet_aton('0.0.0.0')
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
            
            # Set a timeout so the socket doesn't block indefinitely
            sock.settimeout(1.0)
            
            # Announce ourselves immediately
            self._send_mdns_announcement(sock, multicast_group, multicast_port)
            last_announcement = time.time()
            
            self.log(f"mDNS simulation started")
            
            while self.running:
                # Periodically re-announce our presence (every 60 seconds)
                current_time = time.time()
                if current_time - last_announcement > 60:
                    self._send_mdns_announcement(sock, multicast_group, multicast_port)
                    last_announcement = current_time
                
                # Receive incoming announcements
                try:
                    data, address = sock.recvfrom(1024)
                    self._handle_mdns_packet(data, address)
                except socket.timeout:
                    pass
                    
        except Exception as e:
            self.log(f"mDNS simulation error: {str(e)}")
        finally:
            sock.close()
    
    def _send_mdns_announcement(self, sock, multicast_group, multicast_port):
        """Send a simulated mDNS announcement"""
        message = {
            "type": "mdns_announcement",
            "peer_id": self.peer_id,
            "service": self.service_name,
            "ip": self._get_local_ip(),
            "port": self.port,
            "txt": {"app": "p2p_tester", "version": "1.0"}
        }
        
        try:
            sock.sendto(json.dumps(message).encode(), (multicast_group, multicast_port))
            self.log("Sent mDNS announcement")
        except Exception as e:
            self.log(f"Error sending mDNS announcement: {str(e)}")
    
    def _handle_mdns_packet(self, data, address):
        """Handle received mDNS packet"""
        try:
            message = json.loads(data.decode())
            
            if "type" in message and message["type"] == "mdns_announcement":
                peer_id = message.get("peer_id")
                
                # Ignore our own announcements
                if peer_id == self.peer_id:
                    return
                    
                # Store peer information
                if peer_id and peer_id not in self.peers:
                    self.peers[peer_id] = {
                        "ip": message.get("ip", address[0]), 
                        "port": message.get("port", self.port)
                    }
                    self.log(f"mDNS discovered peer: {peer_id}")
                    self.on_peer_discovered(peer_id, "mdns")
                    
        except Exception as e:
            self.log(f"Error handling mDNS packet: {str(e)}")
    
    def _handle_client(self, client_socket, addr):
        """Handle a client connection initiated via mDNS"""
        try:
            client_socket.settimeout(10.0)
            
            # First message should be identification
            data = client_socket.recv(4096)
            if not data:
                return
                
            message = json.loads(data.decode())
            if "peer_id" not in message or message["peer_id"] == self.peer_id:
                return
                
            peer_id = message["peer_id"]
            
            # Update peer info
            if peer_id not in self.peers:
                self.peers[peer_id] = {"ip": addr[0], "port": self.port}
                self.on_peer_discovered(peer_id, "mdns")
                
            self.peers[peer_id]["socket"] = client_socket
            
            if message["type"] == "message" and "content" in message:
                self.log(f"Received mDNS message from {peer_id}: {message['content']}")
                self.on_message(peer_id, message['content'], "mdns")
                
            # Keep connection open for more messages
            while self.running:
                try:
                    data = client_socket.recv(4096)
                    if not data:
                        break
                        
                    message = json.loads(data.decode())
                    if message["type"] == "message" and "content" in message:
                        self.log(f"Received mDNS message from {peer_id}: {message['content']}")
                        self.on_message(peer_id, message['content'], "mdns")
                except socket.timeout:
                    # Send a keepalive
                    keepalive = {"type": "keepalive", "peer_id": self.peer_id}
                    client_socket.sendall(json.dumps(keepalive).encode())
                except Exception as e:
                    self.log(f"Error receiving from {peer_id}: {str(e)}")
                    break
                    
        except Exception as e:
            self.log(f"mDNS client handler error: {str(e)}")
        finally:
            client_socket.close()
    
    def _send_message_impl(self, peer_id: str, message: str) -> bool:
        """Send a message to a specific peer discovered via mDNS"""
        if peer_id not in self.peers:
            return False
            
        peer_info = self.peers[peer_id]
        
        # Check if we already have an open socket
        if "socket" in peer_info and peer_info["socket"]:
            try:
                data = {
                    "type": "message",
                    "peer_id": self.peer_id,
                    "content": message,
                    "protocol": "mdns"
                }
                peer_info["socket"].sendall(json.dumps(data).encode())
                self.log(f"Sent mDNS message to {peer_id} using existing connection")
                return True
            except Exception as e:
                self.log(f"Error sending on existing connection: {str(e)}")
                # Connection may be closed, remove it
                peer_info.pop("socket", None)
        
        # Create a new connection
        try:
            socket_obj = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            socket_obj.settimeout(5.0)
            socket_obj.connect((peer_info["ip"], peer_info["port"]))
            
            # Send the message
            data = {
                "type": "message",
                "peer_id": self.peer_id,
                "content": message,
                "protocol": "mdns"
            }
            socket_obj.sendall(json.dumps(data).encode())
            
            # Store the socket for future use
            peer_info["socket"] = socket_obj
            
            # Start a thread to handle responses
            client_thread = threading.Thread(
                target=self._handle_client,
                args=(socket_obj, (peer_info["ip"], peer_info["port"]))
            )
            client_thread.daemon = True
            client_thread.start()
            
            self.log(f"Sent mDNS message to {peer_id} using new connection")
            return True
            
        except Exception as e:
            self.log(f"Error sending mDNS message: {str(e)}")
            return False
    
    def _get_local_ip(self):
        """Get the local IP address"""
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            # doesn't even have to be reachable
            s.connect(('10.255.255.255', 1))
            IP = s.getsockname()[0]
        except Exception:
            IP = '127.0.0.1'
        finally:
            s.close()
        return IP
            
    def _cleanup(self):
        """Clean up resources"""
        if self.server_socket:
            self.server_socket.close()
            self.server_socket = None
            
        # Close all client connections
        for peer_id, peer_info in self.peers.items():
            if "socket" in peer_info and peer_info["socket"]:
                try:
                    peer_info["socket"].close()
                except:
                    pass
                peer_info.pop("socket", None)
