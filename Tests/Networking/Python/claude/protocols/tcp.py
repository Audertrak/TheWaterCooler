import socket
import json
import threading
import time
from typing import Callable, Dict, List, Optional, Tuple
from protocols.base import ProtocolBase
from peer import PeerManager

from message.base import MessageBase
from message.json import JSONMessage
from message.protobuf import SimpleProtobufMessage
from message.mqtt import MQTTMessage
from message.raw import RawMessage

class TCPProtocol(ProtocolBase):
    def __init__(self, peer_id: str, on_peer_discovered: Callable, on_message: Callable,
                port: int = 5556, discovery_port: int = 5557, broadcast_interval: int = 5,
                message_format: Optional[MessageBase] = None,
                peer_manager: Optional[PeerManager] = None):
        super().__init__(peer_id, on_peer_discovered, on_message, peer_manager)
        self.port = port  # Port for direct communication
        self.discovery_port = discovery_port  # Port for peer discovery
        self.broadcast_interval = broadcast_interval
        self.server_socket = None
        self.discovery_socket = None
        self.client_threads = {}  # {thread_id: thread}
        self.last_broadcast_time = 0
        
        # Use the provided message format or default to JSON
        self.message_format = message_format or JSONMessage() 

    def _run(self):
        """Main TCP listener and broadcaster loop"""
        try:
            # Start TCP server for direct communication
            server_thread = threading.Thread(target=self._run_server)
            server_thread.daemon = True
            server_thread.start()
            
            # Start discovery mechanism
            self._run_discovery()
            
        except Exception as e:
            self.log(f"TCP error: {str(e)}")
        finally:
            self._cleanup()
    
    def _run_server(self):
        """Run TCP server for direct communication"""
        try:
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_socket.bind(('', self.port))
            self.server_socket.listen(5)
            self.server_socket.settimeout(1.0)
            
            self.log(f"TCP server listening on port {self.port}")
            
            while self.running:
                try:
                    client_socket, addr = self.server_socket.accept()
                    client_thread = threading.Thread(
                        target=self._handle_client,
                        args=(client_socket, addr)
                    )
                    client_thread.daemon = True
                    client_thread.start()
                    thread_id = id(client_thread)
                    self.client_threads[thread_id] = client_thread
                except socket.timeout:
                    pass
                except Exception as e:
                    if self.running:  # Only log if we're still supposed to be running
                        self.log(f"TCP server error: {str(e)}")
                        
        except Exception as e:
            self.log(f"TCP server error: {str(e)}")
        finally:
            if self.server_socket:
                self.server_socket.close()
                self.server_socket = None
    
    def _run_discovery(self):
        """Run UDP-based discovery mechanism for TCP peers"""
        try:
            self.discovery_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.discovery_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.discovery_socket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            self.discovery_socket.bind(('', self.discovery_port))
            self.discovery_socket.settimeout(1.0)
            
            self.log(f"TCP discovery listening on port {self.discovery_port}")
            
            while self.running:
                # Broadcast presence periodically
                current_time = time.time()
                if current_time - self.last_broadcast_time > self.broadcast_interval:
                    self._broadcast_presence()
                    self.last_broadcast_time = current_time
                
                # Listen for discovery messages
                try:
                    data, addr = self.discovery_socket.recvfrom(4096)
                    self._handle_discovery(data, addr)
                except socket.timeout:
                    pass
                
        except Exception as e:
            self.log(f"TCP discovery error: {str(e)}")
        finally:
            if self.discovery_socket:
                self.discovery_socket.close()
                self.discovery_socket = None
    
    def _broadcast_presence(self):
        """Broadcast TCP peer presence via UDP"""
        if not self.discovery_socket:
            return
            
        message = {
            "type": "discovery",
            "peer_id": self.peer_id,
            "protocol": "tcp",
            "port": self.port  # Include the port for direct TCP connection
        }
        
        try:
            self.discovery_socket.sendto(json.dumps(message).encode(), 
                                        ('<broadcast>', self.discovery_port))
            self.log("Broadcasted TCP presence")
        except Exception as e:
            self.log(f"TCP broadcast error: {str(e)}")
    
    def _handle_discovery(self, data: bytes, addr: Tuple[str, int]):
        """Handle discovery message"""
        try:
            message = json.loads(data.decode())
            sender_ip = addr[0]
            
            if "type" in message and "peer_id" in message and "port" in message:
                peer_id = message["peer_id"]
                peer_port = message["port"]
                
                # Don't process our own messages
                if peer_id == self.peer_id:
                    return
                    
                if message["type"] == "discovery":
                    # Use PeerManager to track the peer
                    peer = self.peer_manager.add_or_update_peer(
                        peer_id,
                        "tcp",
                        ip=sender_ip,
                        port=peer_port
                    )
                    self.log(f"Discovered TCP peer: {peer_id} at {sender_ip}:{peer_port}")
                    self.on_peer_discovered(peer_id, "tcp")
                    
                    # Send a response
                    self._send_discovery_response(sender_ip)
                    
                elif message["type"] == "discovery_response":
                    # Use PeerManager to track the peer
                    peer = self.peer_manager.add_or_update_peer(
                        peer_id,
                        "tcp",
                        ip=sender_ip,
                        port=peer_port
                    )
                    self.log(f"Received TCP discovery response from: {peer_id}")
                    self.on_peer_discovered(peer_id, "tcp")
                    
        except Exception as e:
            self.log(f"Error handling discovery: {str(e)}")

    def _send_discovery_response(self, target_ip: str):
        """Send a discovery response to a specific IP"""
        if not self.discovery_socket:
            return
            
        message = self.message_format.create_discovery_response(
            self.peer_id, "tcp", port=self.port
        )
        
        try:
            data = self.message_format.serialize(message)
            self.discovery_socket.sendto(data, (target_ip, self.discovery_port))
        except Exception as e:
            self.log(f"Error sending TCP discovery response: {str(e)}")
 
    
    def _handle_client(self, client_socket, addr):
        """Handle a client connection"""
        try:
            client_socket.settimeout(10.0)
            
            data = client_socket.recv(4096)
            if not data:
                return
                
            message = json.loads(data.decode())
            if "peer_id" not in message or message["peer_id"] == self.peer_id:
                return
                
            peer_id = message["peer_id"]
            
            # Update peer info using PeerManager
            peer = self.peer_manager.add_or_update_peer(
                peer_id,
                "tcp",
                ip=addr[0],
                port=self.port,
                socket=client_socket
            )
            
            if message["type"] == "message" and "content" in message:
                self.peer_manager.add_message(peer_id, message['content'], "tcp", outgoing=False)
                self.log(f"Received TCP message from {peer_id}: {message['content']}")
                self.on_message(peer_id, message['content'], "tcp")
                
            # Keep connection open for more messages
            while self.running:
                try:
                    data = client_socket.recv(4096)
                    if not data:
                        break
                        
                    message = json.loads(data.decode())
                    if message["type"] == "message" and "content" in message:
                        self.peer_manager.add_message(peer_id, message['content'], "tcp", outgoing=False)
                        self.log(f"Received TCP message from {peer_id}: {message['content']}")
                        self.on_message(peer_id, message['content'], "tcp")
                except socket.timeout:
                    # Send a keepalive
                    keepalive = {"type": "keepalive", "peer_id": self.peer_id}
                    client_socket.sendall(json.dumps(keepalive).encode())
                except Exception as e:
                    self.log(f"Error receiving from {peer_id}: {str(e)}")
                    break
        except Exception as e:
            self.log(f"Client handler error: {str(e)}")
        finally:
            client_socket.close()
            if peer_id:
                self.peer_manager.mark_peer_inactive(peer_id, "tcp")
            
            current_thread_id = threading.get_ident()
            if current_thread_id in self.client_threads:
                del self.client_threads[current_thread_id]
    
    def _send_message_impl(self, peer_id: str, message: str) -> bool:
        """Send a message to a specific peer"""
        peer = self.peer_manager.get_peer(peer_id)
        if not peer or not peer.is_active("tcp"):
            return False
            
        peer_info = peer.get_protocol_info("tcp")
        
        # Check if we already have an open socket
        socket_obj = peer_info.get("socket")
        if socket_obj:
            try:
                data = {
                    "type": "message",
                    "peer_id": self.peer_id,
                    "content": message,
                    "protocol": "tcp"
                }
                socket_obj.sendall(json.dumps(data).encode())
                self.peer_manager.add_message(peer_id, message, "tcp", outgoing=True)
                self.log(f"Sent TCP message to {peer_id} using existing connection")
                return True
            except Exception as e:
                self.log(f"Error sending on existing connection: {str(e)}")
                peer_info.pop("socket", None)
                self.peer_manager.mark_peer_inactive(peer_id, "tcp")
        
        # Create a new connection if needed
        try:
            socket_obj = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            socket_obj.settimeout(5.0)
            socket_obj.connect((peer_info["ip"], peer_info["port"]))
            
            data = {
                "type": "message",
                "peer_id": self.peer_id,
                "content": message,
                "protocol": "tcp"
            }
            socket_obj.sendall(json.dumps(data).encode())
            
            # Update peer info with new socket
            self.peer_manager.add_or_update_peer(
                peer_id,
                "tcp",
                socket=socket_obj
            )
            
            # Start a thread to handle responses
            client_thread = threading.Thread(
                target=self._handle_client,
                args=(socket_obj, (peer_info["ip"], peer_info["port"]))
            )
            client_thread.daemon = True
            client_thread.start()
            thread_id = id(client_thread)
            self.client_threads[thread_id] = client_thread
            
            self.peer_manager.add_message(peer_id, message, "tcp", outgoing=True)
            self.log(f"Sent TCP message to {peer_id} using new connection")
            return True
            
        except Exception as e:
            self.log(f"Error sending TCP message: {str(e)}")
            self.peer_manager.mark_peer_inactive(peer_id, "tcp")
            return False
    
    def _cleanup(self):
        """Clean up resources"""
        if self.server_socket:
            self.server_socket.close()
            self.server_socket = None
            
        if self.discovery_socket:
            self.discovery_socket.close()
            self.discovery_socket = None
            
        # Close all client connections
        for peer_id, peer_info in self.peers.items():
            if "socket" in peer_info and peer_info["socket"]:
                try:
                    peer_info["socket"].close()
                except:
                    pass
                peer_info.pop("socket", None)
